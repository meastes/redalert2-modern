#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Storage and the run-time load table for the initialised globals of the rebuild link (lane P1, item 2).

The rebuild counterpart of yr/tools/yr_data.py (and main's tools/original_data.py):
for each initialised .data/.rdata global the rebuild units read and no unit defines
(GEN/rb_unsourced_initialised.tsv, rb_data.py: names bound to addresses by the rebuild catalog),
this writes GEN/rb_original_data.cpp (names, addresses, extents and shapes only, never bytes):

  * its storage under its exact symbol, sized for the native layout;
  * a row of ra2_original_globals, which the port (load_original_data,
    port/src/original_image.cpp) fills from the user's game.exe before the static
    initialisers: bytes (scalars, scalar arrays, GUIDs), ptr / ptrs (data pointers become native
    pointers: the storage of the global there, else the mapped image);
  * ra2_original_storage: every global's storage by original address (these and the .bss storage
    of rb_bss_globals.cpp), so pointers to globals reach the native objects.

Declarations (type, pointer depth, array bounds) are read from the generated units and headers
with yr_data.py's reader; a global whose type is not read or not portable gets zero storage and is
listed, as there.

    python3 ra2/tools/rb_original_data.py --gen build/rb-native/gen [--exe game.exe]
"""
import argparse
import bisect
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'yr/tools'))
sys.path.insert(0, str(Path(__file__).resolve().parent))
import data_globals   # noqa: E402
import original_data  # noqa: E402
import yr_data        # noqa: E402  (text_index, enum_values, dims, POD_STRUCTS: unchanged)
import rb_data        # noqa: E402

CAP = 0x10000
# Plain records the rebuild declares (no pointers; the VC6 layout is the native one), with RA2's sizes.
POD_SIZES = {'TheaterDataType': 0x68,   # theater.h: chars, floats, a double and ints
             'ZGradStruct': 24,         # _zbuffer.h: five ints and a bool
             'MouseStruct': 24,         # mouse.h: six ints
             'PixelFXTypeStruct': 40}   # pixelfx.h: ten ints (lane P14: zero, every pixel effect drew black)
# Records of 4-byte pointers and ints, loaded as pointer words (each word an original address made native, or 0
# when it is not one), which is their native layout too: two pointers and an int padded to a word. Lane P14: the
# movie lists were zero, so the Movies menu's list was empty (Play Movie did nothing; the played port's
# menu_movie check) and no campaign movie was ever recorded as unlocked.
POINTER_WORD_RECORDS = {'MovieEntry': 12}   # movieentry.h: Name, Label, Side (Side read by nothing)


def tsv(path):
    lines = [l for l in path.read_text().split('\n') if l]
    head = lines[0].split('\t')
    return [dict(zip(head, l.split('\t'))) for l in lines[1:]]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--gen', type=Path, required=True)
    ap.add_argument('--exe', type=Path, default=None)
    a = ap.parse_args()
    gen = a.gen.resolve()
    if a.exe:
        data_globals.sections(a.exe)    # the pinned target (hash-checked)
    _, refs, _ = rb_data.load_catalog(gen)
    starts = sorted(set(refs.values()))
    wanted = sorted((int(r['address'], 16), r['symbol'], r['name'], int(r['extent']))
                    for r in tsv(gen / 'rb_unsourced_initialised.tsv'))
    bss = []
    for line in (gen / 'rb_bss_globals.cpp').read_text().split('\n'):
        m = re.match(r'alignas\(16\) unsigned char (\w+)\[(\d+)\] __asm__\("([^"]+)"\);  // (0x[0-9A-F]+)', line)
        if m:
            bss.append((int(m.group(4), 16), int(m.group(2)) // 2, m.group(3)))
    index = yr_data.text_index(gen)
    host_lines = (ROOT / 'platform/sdl/main.cpp').read_text(encoding='latin-1').splitlines()
    yr_data.ENUM_VALUES.update(yr_data.enum_values(gen))
    enums = set()
    for path in (gen / 'hdr').rglob('*'):
        if path.is_file():
            enums.update(re.findall(r'\benum\s+(\w+)\s*(?::\s*\w+\s*)?\{', path.read_text(encoding='latin-1')))
    defs, out, places, aliases, report = [], [], [], [], defaultdict(list)
    primary = {}
    for n, (address, symbol, name, gap) in enumerate(wanted):
        if address in primary:
            aliases.append('__asm__(".globl %s\\n%s = %s");   // %s, the same address' % (symbol, symbol, primary[address], name))
            continue
        primary[address] = symbol
        short = name.split('::')[-1]
        lines = (index.get('::'.join(name.split('::')[-2:]), []) if '::' in name else []) or index.get(short, [])
        decl = original_data.declaration(short, lines)
        if decl is None:
            # Names only the host declares (the launcher handshake strings, platform/sdl/main.cpp;
            # lane P2: unloaded, the host created a mutex with an empty name and the game's
            # IsLauncherRunning answered no).
            decl = original_data.declaration(short, [l for l in host_lines if short in l])
        if decl is None and re.match(r'(?:IID|CLSID|LIBID|FMTID)_\w+$', short):
            decl = ('GUID', 0, False)
        gap = min(gap, CAP)
        shape, extent, storage = None, gap, 2 * gap
        if decl:
            base, stars, is_array = decl
            base = ' '.join(w for w in base.split() if w not in ('EXTERN_C', 'const', 'volatile', 'static'))
            count = yr_data.dims(short, lines) if is_array else 1
            if stars == 1:
                extent = 4 * (count if count else max(1, gap // 4))
                shape, storage = (1 if not is_array else 2), 2 * extent
            elif stars == 0 and base in original_data.SCALARS:
                extent = original_data.SCALAR_WIDTH.get(base, 4) * count if count else gap
                shape, storage = 0, extent
            elif stars == 0 and base in enums:
                extent = 4 * count if count else gap
                shape, storage = 0, extent
            elif stars == 0 and base in ('GUID', 'IID', 'CLSID', '_GUID'):
                extent = 16 * (count or 1)
                shape, storage = 0, extent
            elif stars == 0 and base in POINTER_WORD_RECORDS:
                extent = POINTER_WORD_RECORDS[base] * count if count else gap
                shape, storage = 2, 2 * extent
            elif stars == 0 and base in POD_SIZES:
                # Records of chars, ints and floats only: the same bytes natively (lane P2: the theater
                # table unloaded gave no tile sets and the skirmish map's Load_Tiles faulted).
                extent = POD_SIZES[base] * count if count else gap
                shape, storage = 0, extent
        storage = max(storage, 8)
        var = 'rb_g%d' % n
        defs.append('alignas(16) unsigned char %s[%d] __asm__("%s");   // %s 0x%08X%s' % (
            var, storage, symbol, name, address, '' if shape is not None else ' (not loaded: type not read or not portable)'))
        places.append((address, extent, var, 1 if shape in (1, 2) else 0))
        if shape is None:
            report['zero'].append(name)
            continue
        out.append('    {%s, 0x%08X, %d, %d},   // %s' % (var, address, extent, shape, name))
        report['loaded'].append(name)
    externs = ['extern unsigned char rb_bss_%d[] __asm__("%s");' % (k, sym) for k, (_, _, sym) in enumerate(bss)]
    places += [(addr, size, 'rb_bss_%d' % k, 0) for k, (addr, size, _) in enumerate(bss)]
    text = ['// Generated by ra2/tools/rb_original_data.py (lane P1): storage for the initialised globals the',
            '// rebuild units read and no unit defines, and the table the port fills from the user\'s game.exe',
            '// (names, addresses, extents; no bytes). Addresses from the rebuild catalog at ra2/PIN.',
            '#include <stdint.h>',
            'struct ra2_original_global { void *storage; uint32_t address; uint32_t extent; uint32_t shape; };',
            'struct ra2_original_place { uint32_t address; uint32_t extent; void *storage; uint32_t pointers; };',
            'extern "C" {'] + defs + externs + aliases + [
            'extern const ra2_original_global ra2_original_globals[] = {'] + out + ['    {0, 0, 0, 0}};',
            'extern const ra2_original_place ra2_original_storage[] = {'] + [
            '    {0x%08X, %d, %s, %d},' % p for p in sorted(places)] + ['    {0, 0, 0, 0}};', '}', '']
    (gen / 'rb_original_data.cpp').write_text('\n'.join(text))
    (gen / 'rb_original_data.json').write_text(json.dumps({k: sorted(v) for k, v in report.items()}, indent=1) + '\n')
    print('rb_original_data: %d initialised globals (%d loaded from game.exe, %d zero); %d .bss storages placed' % (
        len(primary), len(report['loaded']), len(report['zero']), len(bss)))


if __name__ == '__main__':
    main()
