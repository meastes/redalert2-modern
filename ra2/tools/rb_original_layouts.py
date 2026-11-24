#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Field maps for the original's saves over the rebuild units (lane P4, branch ports-on-rebuild).

tools/original_layouts.py run unchanged in its RA2 mode (the vtable pointers by address
from manifests/rtti_classes.json: the rebuild's classes carry the original RTTI names) over the
generated rebuild units (rb_units.py's gen: units.cmake, units/, hdr/, mingw/), whose record
layouts are dumped with the rebuild units' flags (rb_probe.py: the YR dialect, ra2/sdk
first, -DRA2_NATIVE_RB=1) instead of the src-based build's overlay. The shared tool is imported,
not edited: only its `dump` is replaced, so the src and YR modes are untouched.

    python3 ra2/tools/rb_original_layouts.py --gen build/rb-native/gen --out <file.cpp>
        [--report r.json] [--members m.json] [--types ra2/data/original_save_types.txt] [-j N] [--check]

The type list defaults to ra2/data/original_save_types.txt (main's list under the rebuild's
names). The output defines ra2port_original_layouts/ops, which port/src/original_save.cpp
otherwise has as weak empty tables (the trial then refuses to save in the original's format).
"""
import subprocess
import sys
import threading
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import original_layouts as ol   # noqa: E402

sys.path.insert(0, str(HERE))
from rb_probe import flags as rb_flags   # noqa: E402

TYPES = ROOT / 'ra2/data/original_save_types.txt'
GEN = None
LOCK = threading.Lock()


def i686_support(gen, cache):
    """ol.yr_i686_support with the rebuild gen's rewritten mingw headers (the YR sdk is shared)."""
    d = cache / 'i686'
    (d / 'ov').mkdir(parents=True, exist_ok=True)
    with LOCK:   # the dumps run in threads: make the support files once
        fake = cache / 'build'
        fake.mkdir(exist_ok=True)
        link = fake / 'gen-yr'
        if not link.is_symlink():
            link.symlink_to(gen)
        return ol.yr_i686_support(fake, cache)


def rb_args(unit, triple, cache):
    base = [f for f in rb_flags(GEN) if f not in ('-O2', '-arch', 'arm64', '-fno-zero-initialized-in-bss', '-ferror-limit=50')]
    # rb_flags is a flat list: drop the native prelude pair and the mingw include, re-added per triple.
    out, skip = [], 0
    for i, f in enumerate(base):
        if skip:
            skip -= 1
            continue
        if f in ('-include', '-idirafter'):
            skip = 1
            continue
        out.append(f)
    dump = ['-fsyntax-only', '-Xclang', '-fdump-record-layouts-complete', '-Xclang', '-fdump-record-layouts-canonical']
    if triple == 'i686':
        d = i686_support(GEN, cache)
        return (['clang++', '-target', 'i686-pc-windows-msvc', '-ffreestanding', '-U_MSC_VER', '-D_X86_=1',
                 '-DRA2PORT_ALLOW_HOST', '-I', str(ol.YR_SDK / 'i686')] + out + dump +
                ['-include', str(d / 'prelude_i686.hpp'), '-include', str(d / 'yr_prelude_i686.h'),
                 '-idirafter', str(d / 'ov'), '-idirafter', str(GEN / 'mingw'), str(unit)])
    if triple == 'win64':
        # The x86_64 Windows build (lane P13; ra2/ra2.cmake passes --target windows --cxx): as rb_probe.py --target
        # windows compiles the units.
        return ([ol.CXX['win64'], '-include', str(ol.YR_SDK / 'yr_win_first.h'), '-include', str(ROOT / 'platform/win/dialect_win.hpp')]
                + out + dump + ['-include', str(ol.PRELUDE), '-include', str(ol.YR_SDK / 'yr_prelude.h'),
                                '-isystem', str(GEN / 'mingw'), str(unit)])
    return (['clang++', '-arch', 'arm64'] + out + dump +
            ['-include', str(ol.PRELUDE), '-include', str(ol.YR_SDK / 'yr_prelude.h'),
             '-idirafter', str(GEN / 'mingw'), str(unit)])


def dump(build, unit, triple, cache):
    out = cache / ('%s.%s.txt' % (unit.stem, triple))
    if out.exists() and out.stat().st_mtime >= unit.stat().st_mtime:
        return out.read_text(errors='replace')
    p = subprocess.run(rb_args(unit, triple, cache), capture_output=True, text=True, errors='replace')
    out.write_text(p.stdout)
    return p.stdout


def main():
    global GEN
    argv = sys.argv[1:]
    if '--gen' not in argv:
        raise SystemExit('rb_original_layouts: --gen <rb_units.py output> is required')
    i = argv.index('--gen')
    GEN = Path(argv[i + 1]).resolve()
    if GEN.name != 'gen':
        raise SystemExit('rb_original_layouts: --gen must be the directory named gen (the tool reads <build>/gen)')
    argv[i:i + 2] = ['--build', str(GEN.parent)]
    if '--types' not in argv:
        argv += ['--types', str(TYPES)]
    ol.dump = dump
    sys.argv = [sys.argv[0]] + argv
    return ol.main()


if __name__ == '__main__':
    sys.exit(main())
