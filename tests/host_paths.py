#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The generated CMake lists and #line paths on a Windows host (lane P42; no Windows needed).

On a Windows PC the generators' paths come back from pathlib as C:\\ra2\\...; written raw into gen/units.cmake,
CMake refused them ("Invalid character escape '\\w'", ra2/ra2.cmake's include). This checks, under this host's
cmake -P:
  * tools/host_tools.py cmake_list() with Windows spellings (backslashes, a drive, spaces) parses and names the
    files in forward slashes, while the old raw form fails (so the check would see the bug come back);
  * ra2/tools/ra2_units.py and yr/tools/yr_coff.py write lists through it (one unit, into a scratch folder);
  * line_directive() never leaves a backslash in a #line;
  * yr/data/host_c_headers.txt, the list a Windows host uses in place of the macOS SDK, equals what the SDK
    leaves out (skipped without xcrun).

    python3 tests/host_paths.py [--cmake <cmake>] [--mingw <llvm-mingw>]
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path, PureWindowsPath

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import host_tools  # noqa: E402

WINDOWS_PATHS = [r'C:\ra2\w\codex1\redalert2-modern\build\windows-native\gen\units\aircraft.cpp',
                 PureWindowsPath(r'C:\ra2\w\codex1\redalert2-modern\build\gen\units\unit.cpp'),
                 r'D:\Program Files\x\nested\tab\t.cpp',
                 'C:/already/forward/b.cpp']
ONLY = 'aircraft,gdlgsupp,netdlg2,netshare,wonline'


def cmake_reads(cmake, listing, var, expect, tmp):
    """None when `include(listing)` under cmake -P gives VAR == expect, else the problem."""
    script = tmp / 'check.cmake'
    script.write_text('include("%s")\nlist(LENGTH %s n)\nmessage(STATUS "n=${n}")\nforeach(p IN LISTS %s)\n'
                      '  message(STATUS "item=${p}")\nendforeach()\n' % (host_tools.cmake_path(listing), var, var))
    p = subprocess.run([cmake, '-P', str(script)], capture_output=True, text=True)
    if p.returncode:
        return 'cmake -P failed: %s' % (p.stderr.strip().splitlines() or ['?'])[0]
    got = [l.split('item=', 1)[1] for l in (p.stdout + p.stderr).splitlines() if 'item=' in l]
    if got != expect:
        return 'cmake read %r, expected %r' % (got[:3], expect[:3])
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--cmake', default=shutil.which('cmake') or 'cmake')
    ap.add_argument('--mingw', default=os.environ.get('RA2_LLVM_MINGW', ''), help='llvm-mingw release folder')
    a = ap.parse_args()
    problems = []
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        expect = [str(p).replace('\\', '/') for p in WINDOWS_PATHS]

        listing = tmp / 'units.cmake'
        listing.write_text(host_tools.cmake_list('RB_NATIVE_UNITS', WINDOWS_PATHS))
        why = cmake_reads(a.cmake, listing, 'RB_NATIVE_UNITS', expect, tmp)
        if why:
            problems.append('cmake_list: ' + why)
        raw = tmp / 'raw.cmake'   # the form before lane P42
        raw.write_text('set(RB_NATIVE_UNITS\n%s)\n' % ''.join('  "%s"\n' % u for u in WINDOWS_PATHS))
        if cmake_reads(a.cmake, raw, 'RB_NATIVE_UNITS', expect, tmp) is None:
            problems.append('the raw backslash form parsed: the check cannot see the bug')
        for bad in ('C:\\a"b.cpp', 'C:\\${X}.cpp', 'C:\\a;b.cpp'):
            try:
                host_tools.cmake_list('X', [bad])
                problems.append('cmake_list accepted %r' % bad)
            except SystemExit:
                pass
        for p in WINDOWS_PATHS:
            line = host_tools.line_directive(p)
            if '\\' in line or not line.startswith('#line 1 "C:/') and not line.startswith('#line 1 "D:/'):
                problems.append('line_directive(%r) = %r' % (p, line))

        # The generators themselves: one RA2 unit, then yr_coff.py's list over it.
        gen = tmp / 'gen'
        # (gdlgsupp and netdlg2: the units the '*' fixups need, which --only runs still check)
        p = subprocess.run([sys.executable, '-B', str(ROOT / 'ra2/tools/ra2_units.py'), '--out', str(gen), '--only', ONLY],
                           capture_output=True, text=True)
        if p.returncode:
            problems.append('ra2_units.py --only %s failed: %s' % (ONLY, p.stderr.strip()[-300:]))
        else:
            units = sorted(str(u) for u in (gen.resolve() / 'units').glob('*.cpp'))
            listed = [l.strip().strip('"') for l in (gen / 'units.cmake').read_text().splitlines()[1:-1]]
            why = cmake_reads(a.cmake, gen / 'units.cmake', 'RB_NATIVE_UNITS', listed, tmp)
            if why or sorted(listed) != units:
                problems.append('ra2_units.py units.cmake: %s' % (why or listed))
            p = subprocess.run([sys.executable, '-B', str(ROOT / 'yr/tools/yr_coff.py'), '--gen', str(gen), '--out',
                                str(gen / 'coff_units.cmake'), '--var', 'RB_NATIVE_UNITS'], capture_output=True, text=True)
            if p.returncode:
                problems.append('yr_coff.py failed: %s' % p.stderr.strip()[-300:])
            else:
                coff = (gen / 'coff_units.cmake').read_text()
                listed = [l.strip().strip('"') for l in coff.splitlines()[1:-1]]
                why = cmake_reads(a.cmake, gen / 'coff_units.cmake', 'RB_NATIVE_UNITS', listed, tmp)
                if why or len(listed) != len(units):
                    problems.append('yr_coff.py coff_units.cmake: %s' % (why or listed))

        # The Windows host's header list against the macOS SDK it stands for.
        xcrun = shutil.which('xcrun')
        include = Path(a.mingw) / 'generic-w64-mingw32/include' if a.mingw else None
        if xcrun and include and include.is_dir():
            sdk = subprocess.run([xcrun, '--sdk', 'macosx', '--show-sdk-path'], capture_output=True, text=True).stdout.strip()
            fresh = tmp / 'host_c_headers.txt'
            subprocess.run([sys.executable, '-B', str(ROOT / 'yr/tools/yr_sdk.py'), '--mingw', str(include), '--out', str(tmp / 'x'),
                            '--host-headers', sdk + '/usr/include', '--write-host-list', str(fresh)], check=True, capture_output=True)
            tracked = ROOT / 'yr/data/host_c_headers.txt'
            if fresh.read_text() != tracked.read_text():
                problems.append('yr/data/host_c_headers.txt differs from this Mac SDK\'s list (rerun yr_sdk.py --write-host-list)')
        else:
            print('host_paths: header list check skipped (no xcrun or no --mingw / RA2_LLVM_MINGW)')
    for x in problems:
        print('host_paths: ' + x)
    print('host_paths: %s' % ('FAIL' if problems else 'ok'))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
