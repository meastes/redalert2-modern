#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the generated rebuild units (ra2_units.py) with the native YR build's flags (lane P1).

A probe outside CMake: each OUT/units/*.cpp with clang (-fsyntax-only, or -c into OUT/obj), the
flags of yr_units_objs (the RA2 dialect, the port prelude and the YR prelude, the mingw-w64 headers
rewritten by yr/tools/yr_sdk.py). Writes OUT/probe.json: per unit ok/failed, error counts and
the first errors, and a tally of error sites (file:line) across units.

    python3 ra2/tools/ra2_probe.py --gen build/mac/gen [--obj] [-j 6] [--only a,b]
"""
import argparse
import collections
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import host_tools  # noqa: E402
# The llvm-mingw toolchain whose Win32/DirectX headers the units read (rewritten by yr/tools/yr_sdk.py into the
# build folder, never copied into the repository): RA2_LLVM_MINGW, else an extracted release under toolchains/.
MINGW = os.environ.get('RA2_LLVM_MINGW', '')


def flags(gen, windows=False):
    sdk = ROOT / 'yr/sdk'
    if windows:
        # The x86_64 Windows build (lane P13, as the YR units' there: yr/yr.cmake): yr_win_first.h and
        # platform/win/dialect_win.hpp first, the rewritten Win32 headers ahead of the toolchain's (-isystem),
        # the toolchain's own target instead of -arch arm64.
        out = ['-include', str(sdk / 'yr_win_first.h'), '-include', str(ROOT / 'platform/win/dialect_win.hpp')]
        for f in flags(gen):
            out.append(f)
        i = out.index('-arch')
        del out[i:i + 2]
        i = out.index('-idirafter')
        out[i] = '-isystem'
        # A function that falls off its end without a return (VC6 left eax as it was) returns as on the Mac, whose
        # clang has -fno-strict-return by default; mingw's clang trapped there (int3: lane P19, Yuri's Revenge's
        # TechnoClass::VoiceDeploy at an MCV's deploy click, "Red Alert 2 stopped" on codex1's PC).
        out.append('-fno-strict-return')
        return out
    return ['-DRA2_NATIVE_YR=1', '-DRA2_NATIVE_RB=1', '-DWIN32', '-D_WIN32', '-D__GNUC_MINOR__=2', '-D__GNUC__=4',
            '-I' + str(ROOT / 'ra2/sdk'), '-I' + str(sdk), '-I' + str(gen / 'hdr'),
            '-O2', '-DNDEBUG', '-std=c++17', '-arch', 'arm64',
            '-fms-extensions', '-fms-compatibility', '-fdelayed-template-parsing', '-Xclang', '-fno-wchar',
            '-fshort-wchar', '-fwrapv', '-fno-strict-aliasing', '-fno-delete-null-pointer-checks', '-mms-bitfields',
            '-Wno-incompatible-ms-struct', '-fwritable-strings', '-fno-rounding-math', '-Wno-error=address-of-temporary',
            '-Wno-register', '-Wno-c++11-narrowing', '-w', '-ferror-limit=50',
            '-include', str(ROOT / 'port/include/ra2port/prelude.hpp'),
            '-include', str(sdk / 'yr_prelude.h'),
            '-idirafter', str(gen / 'mingw'), '-fno-zero-initialized-in-bss']


def cmake_flags(gen, windows=False):
    """The units' flags as ra2/ra2.cmake reads them (';'-separated), every path in forward slashes (lane P48).

    ra2.cmake passes each two-word option (-include <path>, ...) as one SHELL: option, which CMake splits as a Unix
    shell would: on a Windows host pathlib's backslashes were taken as escapes and dropped, so clang was asked for
    C:ra2w...yrsdkyr_win_first.h (codex1's PC, ahtoi.cpp and audattr.cpp, the first units built)."""
    return ';'.join(host_tools.slashed(f) for f in flags(gen, windows))


ERR = re.compile(r'^(.*?):(\d+):\d+: (?:fatal )?error: (.*)$', re.M)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--gen', required=True)
    ap.add_argument('--asan', action='store_true', help='AddressSanitizer objects (OUT/obj[-nm]-asan), for diagnosing heap faults')
    ap.add_argument('--print-flags', action='store_true',
                    help="make the rewritten mingw headers if needed, print the units' compile flags (';'-separated, for CMake) and stop")
    ap.add_argument('--target', choices=('apple', 'windows'), default='apple',
                    help="windows: the x86_64 Windows build's flags and headers (the macOS SDK's C headers named by --host-headers)")
    ap.add_argument('--host-headers', default='', help='the Windows build: <macOS SDK>/usr/include (yr_sdk.py --host-headers)')
    ap.add_argument('--obj', action='store_true', help='compile to OUT/obj/*.o instead of -fsyntax-only')
    ap.add_argument('-j', type=int, default=6)
    ap.add_argument('--only', default='')
    ap.add_argument('--non-matching', action='store_true',
                    help="compile the rebuild's parked drafts too (-DRA2_NON_MATCHING; objects in OUT/obj-nm, probe-nm.json)")
    a = ap.parse_args()
    gen = Path(a.gen).resolve()
    if not (gen / 'mingw/windows.h').exists():
        roots = [Path(MINGW)] if MINGW else sorted(ROOT.glob('toolchains/llvm-mingw-*'))
        mingw = [inc for r in roots for inc in sorted(r.glob('**/generic-w64-mingw32/include'))[:1]]
        if not mingw:
            raise SystemExit('ra2_probe: no llvm-mingw toolchain (set RA2_LLVM_MINGW to the extracted '
                             'llvm-mingw-20261020-ucrt-macos-universal folder, or extract it under toolchains/)')
        subprocess.run([sys.executable, '-B', str(ROOT / 'yr/tools/yr_sdk.py'), '--mingw', str(mingw[0]),
                        '--out', str(gen / 'mingw')] + (['--host-headers', a.host_headers] if a.host_headers else []),
                       check=True, stdout=sys.stderr)
    if a.target == 'windows' and not (gen / 'mingw-i686/windows.h').exists():
        # The original-layout dumps' i686 side (ra2_original_layouts.py) reads the headers as the Mac build rewrites them
        # (RA2_LONG, the C library's own headers kept): a second tree beside the Windows build's.
        roots = [Path(MINGW)] if MINGW else sorted(ROOT.glob('toolchains/llvm-mingw-*'))
        mingw = [inc for r in roots for inc in sorted(r.glob('**/generic-w64-mingw32/include'))[:1]]
        subprocess.run([sys.executable, '-B', str(ROOT / 'yr/tools/yr_sdk.py'), '--mingw', str(mingw[0]),
                        '--out', str(gen / 'mingw-i686')], check=True, stdout=sys.stderr)
    if a.print_flags:
        print(cmake_flags(gen, a.target == 'windows'), end='')
        return
    only = set(x for x in a.only.split(',') if x)
    units = sorted(u for u in (gen / 'units').glob('*.cpp') if not only or u.stem in only or u.name in only)
    base = ['clang++'] + flags(gen) + (['-DRA2_NON_MATCHING'] if a.non_matching else [])
    objdir = gen / ('obj-nm' if a.non_matching else 'obj')
    if a.asan:
        base += ['-fsanitize=address', '-fsanitize-recover=address', '-fno-sanitize-address-use-after-scope', '-fno-omit-frame-pointer', '-g1']   # Westwood's straws unlink from destroyed neighbours (benign)
        objdir = Path(str(objdir) + '-asan')
    objdir.mkdir(exist_ok=True)

    def one(u):
        cmd = base + (['-c', '-o', str(objdir / (u.stem + '.o'))] if a.obj else ['-fsyntax-only']) + [str(u)]
        p = subprocess.run(cmd, capture_output=True, text=True, errors='replace')
        errs = ERR.findall(p.stderr)
        return u.stem, p.returncode, errs, p.stderr

    results = {}
    with ThreadPoolExecutor(a.j) as ex:
        for stem, rc, errs, stderr in ex.map(one, units):
            results[stem] = {'ok': rc == 0, 'errors': len(errs),
                             'first': ['%s:%s: %s' % (os.path.basename(f), l, m) for f, l, m in errs[:6]]}
            if rc != 0 and not errs:
                results[stem]['first'] = [stderr[-600:]]
    sites = collections.Counter()
    for stem, r in results.items():
        for e in set(x.split(': ', 1)[0] for x in r['first']):
            sites[e] += 1
    ok = sum(r['ok'] for r in results.values())
    prev = {}
    pj = gen / ('probe-nm.json' if a.non_matching else 'probe.json')
    if pj.exists() and only:
        prev = json.loads(pj.read_text()).get('units', {})
    prev.update(results)
    okall = sum(r['ok'] for r in prev.values())
    pj.write_text(json.dumps({'compiled': okall, 'units_probed': len(prev), 'sites': sites.most_common(60),
                              'units': prev}, indent=1) + '\n')
    print('ra2_probe: %d of %d probed now compile (%d of %d overall)' % (ok, len(results), okall, len(prev)))
    for s, n in sites.most_common(25):
        print('  %4d  %s' % (n, s))


if __name__ == '__main__':
    main()
