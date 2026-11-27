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
  * the RA2 units' compile flags (ra2/tools/ra2_probe.py --print-flags, read by ra2/ra2.cmake into SHELL: options)
    keep every path whole under CMake's SHELL: splitting (lane P48: codex1's PC compiled ahtoi.cpp with
    -include C:ra2wcodex1...yrsdkyr_win_first.h, the backslashes taken as escapes), and the old form would not;
  * every generator that runs nm over the build's objects keeps each command line within Windows's 32,767
    characters (lane P51: codex1's PC failed with WinError 206 at link_closure.py and yr_aliases.py, 400 paths
    per call), while short paths still batch as before (so a Mac build's output is unchanged);
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


def shell_options(cmake, flags, tmp, block):
    """The words clang gets for FLAGS (';'-separated) through BLOCK (ra2.cmake's pairing loop) and SHELL: splitting."""
    script = tmp / 'shell.cmake'
    script.write_text('set(rb_unit_flags [==[%s]==])\n%s\n'
                      'foreach(o IN LISTS rb_unit_options)\n'
                      '  if(o MATCHES "^SHELL:(.*)$")\n'
                      '    separate_arguments(w UNIX_COMMAND "${CMAKE_MATCH_1}")\n'
                      '  else()\n'
                      '    set(w "${o}")\n'
                      '  endif()\n'
                      '  foreach(x IN LISTS w)\n'
                      '    message(STATUS "word=${x}")\n'
                      '  endforeach()\n'
                      'endforeach()\n' % (flags, block))
    p = subprocess.run([cmake, '-P', str(script)], capture_output=True, text=True)
    if p.returncode:
        return None
    return [l.split('word=', 1)[1] for l in (p.stdout + p.stderr).splitlines() if 'word=' in l]


def ra2_flags_check(cmake, tmp, problems):
    """ra2_probe.py's Windows flags with a Windows checkout's paths, through ra2.cmake's own loop."""
    sys.path.insert(0, str(ROOT / 'ra2/tools'))
    import ra2_probe  # noqa: E402
    text = (ROOT / 'ra2/ra2.cmake').read_text()
    start = text.index('set(rb_unit_options "")')
    block = text[start:text.index('endforeach()', start) + len('endforeach()')]
    win_root = PureWindowsPath(r'C:\ra2\w\codex1\build\redalert2-modern')
    gen = win_root / 'build' / 'windows-one' / 'ra2' / 'gen'
    saved = ra2_probe.ROOT
    ra2_probe.ROOT = win_root
    try:
        fixed = ra2_probe.cmake_flags(gen, True)
        raw = ';'.join(ra2_probe.flags(gen, True))   # the form before lane P48
    finally:
        ra2_probe.ROOT = saved
    want = ['C:/ra2/w/codex1/build/redalert2-modern/yr/sdk/yr_win_first.h',
            'C:/ra2/w/codex1/build/redalert2-modern/platform/win/dialect_win.hpp',
            'C:/ra2/w/codex1/build/redalert2-modern/port/include/ra2port/prelude.hpp',
            'C:/ra2/w/codex1/build/redalert2-modern/build/windows-one/ra2/gen/mingw']
    words = shell_options(cmake, fixed, tmp, block)
    if words is None:
        problems.append('ra2.cmake flags: cmake -P failed on the pairing loop')
        return
    for w in want:
        if w not in words:
            problems.append('ra2.cmake flags: %s is not one word of %r' % (w, words[:6]))
    for i, w in enumerate(words):
        if w in ('-include', '-isystem', '-idirafter') and (i + 1 >= len(words) or not words[i + 1].startswith('C:/')):
            problems.append('ra2.cmake flags: %s is followed by %r' % (w, words[i + 1] if i + 1 < len(words) else None))
    if any('\\' in w for w in words):
        problems.append('ra2.cmake flags: a backslash reached clang: %r' % [w for w in words if '\\' in w][:2])
    # A checkout path with a space stays one word.
    spaced = shell_options(cmake, '-include;C:/ra2/a b/x.h', tmp, block)
    if spaced != ['-include', 'C:/ra2/a b/x.h']:
        problems.append('ra2.cmake flags: a path with a space split: %r' % spaced)
    old = shell_options(cmake, raw, tmp, block)
    if old is not None and want[0] in old:
        problems.append('the raw backslash flags kept their path: the check cannot see the bug')


WINDOWS_COMMAND_LIMIT = 32767   # CreateProcess, with the terminating NUL


def long_command_check(problems):
    """The nm callers under a fake subprocess.run: 2,000 objects at codex1's path length, each command measured
    as Windows would receive it."""
    import importlib
    deep = r'C:\ra2\w\codex1\redalert2-modern\build\windows-native\CMakeFiles\rb_units_objs.dir\gen\units'
    paths = ['%s\\unit_%04d.cpp.obj' % (deep, i) for i in range(2000)]
    width = len(subprocess.list2cmdline(['llvm-nm', '-g'] + paths))
    if width <= WINDOWS_COMMAND_LIMIT:
        problems.append('long_command_check: %d characters of paths is not over the limit' % width)
    for count in (1, 50, 400):
        runs = list(host_tools.batches(['llvm-nm', '-g'], paths, count))
        if [p for r in runs for p in r] != paths or max(map(len, runs)) > count:
            problems.append('batches(count=%d) lost, reordered or overfilled paths' % count)
        worst = max(len(subprocess.list2cmdline(['llvm-nm', '-g'] + r)) for r in runs)
        if worst > host_tools.COMMAND_LIMIT:
            problems.append('batches(count=%d): a %d-character command line' % (count, worst))
    short = ['u%d.o' % i for i in range(1000)]
    if list(host_tools.batches(['nm', '-g'], short, 400)) != [short[i:i + 400] for i in range(0, 1000, 400)]:
        problems.append('batches() of short paths differs from the fixed 400-path runs')
    alone = 'C:\\' + 'x' * 40000 + '.o'
    if list(host_tools.batches(['nm'], ['a.o', alone, 'b.o'], 400)) != [['a.o'], [alone], ['b.o']]:
        problems.append('batches() did not run an over-long path alone')

    for folder in ('tools', 'yr/tools', 'ra2/tools'):
        if str(ROOT / folder) not in sys.path:
            sys.path.insert(0, str(ROOT / folder))
    seen = []

    def fake_run(args, *rest, **kw):
        line = subprocess.list2cmdline([str(a) for a in args])
        seen.append((len(line), sum(1 for a in args if str(a) in path_set)))
        return subprocess.CompletedProcess(args, 0, '', '')
    path_set = set(paths)
    calls = [('link_closure', lambda m: m.nm_symbols(paths)),
             ('link_closure', lambda m: m.windows_system_exports('Z:/no-sysroot', libs=paths)),
             ('yr_aliases', lambda m: m.nm(paths)),
             ('ra2_data', lambda m: m.nm_defined(paths)),
             ('weaken_duplicates', lambda m: m.definitions(paths)),
             ('yr_member_aliases', lambda m: m.unit_data_symbols(paths))]
    real_run = subprocess.run
    for name, call in calls:
        del seen[:]
        subprocess.run = fake_run
        try:
            call(importlib.import_module(name))
        except Exception as x:   # noqa: BLE001 (reported as a problem)
            problems.append('%s: %r under the fake nm' % (name, x))
            continue
        finally:
            subprocess.run = real_run
        nm_runs = [s for s in seen if s[1]]
        if sum(n for _, n in nm_runs) != len(paths):
            problems.append('%s: nm saw %d of %d paths' % (name, sum(n for _, n in nm_runs), len(paths)))
        if nm_runs and max(w for w, _ in nm_runs) > WINDOWS_COMMAND_LIMIT:
            problems.append('%s: a %d-character command line (WinError 206)' % (name, max(w for w, _ in nm_runs)))


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

        ra2_flags_check(a.cmake, tmp, problems)
        long_command_check(problems)

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
