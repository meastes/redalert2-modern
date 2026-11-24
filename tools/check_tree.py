#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Repository hygiene: what may be tracked here, checked on the working tree (tracked and untracked files that
.gitignore does not exclude), so it can run before a commit.

    python3 tools/check_tree.py [--game-exe <game.exe>] [--scan <file>...]

Checks:
  * every vendor/ folder (ra2-rebuild, ra2-decomp, tibsun) equals its PIN list (no edits to the pinned text, no file added or missing);
  * no game data or program: refuses game file extensions and names (.mix, .bik, .vqa, .tlb, .sav, game.exe,
    gamemd.exe, BINKW32.DLL, ...), signing material (.mobileprovision, .p12, .cer) and any file over 8 MiB;
  * no machine-specific paths (/Users/..., /Volumes/...) in tracked text;
  * every own source file (outside vendor/) carries an SPDX line in its first 15 lines;
  * THIRD_PARTY.md names each vendor folder.
With --game-exe (the user's own game.exe, read only, never stored) it also samples the executable's code (.text)
and resources (.rsrc) in 64-byte pieces (those with at least 24 different byte values, one per KB) and requires
that none appears in any checked file, nor in the --scan files (built binaries, say). .data/.rdata are not
sampled: tables the port computes (CRC-32, fast-math) equal the original's by construction.
Exit 0 when clean, 1 with the problems listed.
"""
import argparse
import hashlib
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAX_BYTES = 8 << 20
GAME_EXTENSIONS = {'.mix', '.mmx', '.exe', '.dll', '.tlb', '.mpr', '.csf', '.pkt', '.bik', '.vqa', '.shp', '.pal',
                   '.aud', '.wav', '.sav', '.yrm', '.vxl', '.hva', '.tem', '.sno', '.urb', '.des', '.lun', '.ubn',
                   '.dsk', '.mobileprovision', '.p12', '.cer', '.keychain'}
GAME_NAMES = {'game.exe', 'gamemd.exe', 'binkw32.dll', 'ra2.tlb', 'ra2md.tlb', 'blowfish.dll', 'ra2.ini', 'ra2md.ini'}
OWN_SOURCES = {'.cpp', '.hpp', '.h', '.inc', '.mm', '.c', '.py', '.sh', '.cmake'}
LOCAL_PATH = re.compile(rb'/(?:Users|Volumes)/[A-Za-z0-9]')


def files():
    out = subprocess.run(['git', '-C', str(ROOT), 'ls-files', '-co', '--exclude-standard', '-z'],
                         capture_output=True, check=True).stdout
    return sorted({ROOT / f for f in out.decode('utf-8', 'surrogateescape').split('\0') if f and (ROOT / f).is_file()})


def check_vendor(problems):
    return sum(check_pin(ROOT / 'vendor' / d.name, problems) for d in sorted((ROOT / 'vendor').iterdir()) if d.is_dir())


def check_pin(vendor, problems):
    tag = 'vendor/%s' % vendor.name
    pin = vendor / 'PIN'
    if not pin.is_file():
        problems.append('%s/PIN is missing' % tag)
        return 0
    listed = {}
    for line in pin.read_text().split('\n'):
        if line.startswith('sha256 '):
            _, digest, name = line.split(' ', 2)
            listed[name] = digest
    have = {p.relative_to(vendor).as_posix() for p in vendor.rglob('*') if p.is_file() and p.name != 'PIN'}
    for name in sorted(have - set(listed)):
        problems.append('%s/%s is not in PIN' % (tag, name))
    for name in sorted(set(listed) - have):
        problems.append('%s/%s is listed in PIN but missing' % (tag, name))
    for name in sorted(have & set(listed)):
        if hashlib.sha256((vendor / name).read_bytes()).hexdigest() != listed[name]:
            problems.append('%s/%s differs from its PIN digest' % (tag, name))
    return len(listed)


def samples(exe):
    data = exe.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    count, opt = struct.unpack_from('<H', data, pe + 6)[0], struct.unpack_from('<H', data, pe + 20)[0]
    out = set()
    for i in range(count):
        o = pe + 24 + opt + 40 * i
        name = data[o:o + 8].rstrip(b'\0').decode('latin-1')
        if name not in ('.text', '.rsrc'):
            continue
        raw_size, raw_ptr = struct.unpack_from('<II', data, o + 16)
        sec = data[raw_ptr:raw_ptr + raw_size]
        for k in range(0, len(sec) - 64, 1024):
            piece = sec[k:k + 64]
            if len(set(piece)) >= 24:
                out.add(piece)
    return out


def scan(paths, pieces, problems):
    by_prefix = {}
    for piece in pieces:
        by_prefix.setdefault(piece[:16], []).append(piece)
    for path in paths:
        data = path.read_bytes()
        hits = 0
        for k in range(len(data) - 63):
            cands = by_prefix.get(data[k:k + 16])
            if cands and data[k:k + 64] in cands:
                hits += 1
        if hits:
            problems.append('%s holds %d sampled 64-byte pieces of game.exe' % (path, hits))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--game-exe', type=Path)
    ap.add_argument('--scan', type=Path, nargs='*', default=[])
    a = ap.parse_args()
    problems = []
    tree = files()
    pinned = check_vendor(problems)
    third_party = (ROOT / 'THIRD_PARTY.md').read_text() if (ROOT / 'THIRD_PARTY.md').is_file() else ''
    for d in sorted(p for p in (ROOT / 'vendor').iterdir() if p.is_dir()):
        if 'vendor/%s' % d.name not in third_party:
            problems.append('THIRD_PARTY.md does not name vendor/%s' % d.name)
    for path in tree:
        rel = path.relative_to(ROOT).as_posix()
        if path.suffix.lower() in GAME_EXTENSIONS or path.name.lower() in GAME_NAMES:
            problems.append('%s: a game, program or signing file' % rel)
        size = path.stat().st_size
        if size > MAX_BYTES:
            problems.append('%s: %d bytes, over the %d-byte limit' % (rel, size, MAX_BYTES))
        data = path.read_bytes()
        if not rel.startswith('vendor/') and rel != 'tools/check_tree.py' and LOCAL_PATH.search(data):
            problems.append('%s: a machine-specific path (%s)' % (rel, LOCAL_PATH.search(data).group(0).decode()))
        if not rel.startswith('vendor/') and (path.suffix in OWN_SOURCES or path.name == 'CMakeLists.txt'):
            head = b'\n'.join(data.split(b'\n')[:15])
            if b'SPDX-License-Identifier' not in head:
                problems.append('%s: no SPDX line' % rel)
    if a.game_exe:
        pieces = samples(a.game_exe)
        scan(tree + a.scan, pieces, problems)
        print('check_tree: %d pieces of game.exe sampled; %d files scanned' % (len(pieces), len(tree) + len(a.scan)))
    for p in problems:
        print('check_tree: ' + p)
    print('check_tree: %d files, %d vendored files checked against PIN: %s' % (
        len(tree), pinned or 0, 'clean' if not problems else '%d problem(s)' % len(problems)))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
