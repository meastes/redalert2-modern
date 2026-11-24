#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Native storage for the globals the rebuild units read but no unit defines (lane P1, item 2).

The rebuild counterpart of tools/data_globals.py: the same rules, keyed by the rebuild's
names. Each undefined data symbol of the rebuild link (link_closure.py --list-undefined) is bound to
its original address through the rebuild catalog's reference tables (vendor/ra2-rebuild/symbols.tsv,
derived from rebuild/manifest/catalog at the vendored commit by tools/make_symbols.py; MSVC name ->
address) or an address-suffixed name, and to its section through the target's section layout
(ra2/data/target_sections.tsv, or --exe: the pinned target's PE section headers, hash-checked).

  * zero-initialised (.bss, past the section's file-backed part): GEN/rb_bss_globals.cpp, zero
    storage of the original extent (to the next known symbol) doubled for LP64, 16-byte aligned;
    other names of the same address are assembler aliases of that storage, or, when a link input
    already defines the address under another name, rows of GEN/rb_bss_aliases.tsv (ld64 aliases).
  * initialised (.data/.rdata): never defined from the executable; listed in
    GEN/rb_unsourced_initialised.tsv (address, extent, names, the unit that references it) for the
    run-time load from the user's game.exe (as main's original_data.py, the next step).

    python3 ra2/tools/rb_data.py --gen build/rb-native/gen --undefined U.txt [--exe game.exe]
"""
import argparse
import bisect
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import data_globals  # noqa: E402  (sections, msvc_key, demangle: unchanged)


SYMBOLS = ROOT / 'vendor/ra2-rebuild/symbols.tsv'
SECTIONS = ROOT / 'ra2/data/target_sections.tsv'


def target_sections(exe=None):
    """The target's sections (name, start, end, raw_end): from EXE's headers when given, else the layout file."""
    if exe:
        return data_globals.sections(exe)
    out = []
    for line in SECTIONS.read_text().split('\n'):
        if line and not line.startswith('#'):
            name, start, end, raw_end = line.split('\t')
            out.append(dict(name=name, start=int(start, 16), end=int(end, 16), raw_end=int(raw_end, 16)))
    return out


def load_catalog(gen=None):
    """The catalog's regions and reference tables, as symbols.tsv carries them (the same first-wins order)."""
    regions, refs, users = [], {}, {}
    pop_source = {}
    ref_rows = []
    for line in SYMBOLS.read_text().split('\n'):
        if not line or line.startswith('#'):
            continue
        f = line.split('\t')
        if f[0] == 'region':
            r = dict(entry=int(f[1], 16), symbol=f[2], population=f[3], source=f[4])
            regions.append(r)
            pop_source.setdefault(r['population'], r['source'])
        else:
            ref_rows.append((f[2], int(f[1], 16), f[3]))
    for name, addr, pop in ref_rows:
        refs.setdefault(name, addr)
        users.setdefault(name, pop_source.get(pop))
    for r in regions:
        refs.setdefault(r['symbol'], int(r['entry']))
    return dict(regions=regions), refs, users


def defined_symbols(objs):
    out = subprocess.run(['nm', '-g', '--defined-only', '-j'] + objs, capture_output=True, text=True).stdout
    return sorted({s for s in out.split('\n') if s and not s.endswith(':')})


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--gen', type=Path, required=True)
    ap.add_argument('--undefined', type=Path, required=True)
    ap.add_argument('--exe', type=Path, default=None)
    a = ap.parse_args()
    gen = a.gen.resolve()
    secs = target_sections(a.exe)
    _, refs, users = load_catalog(gen)
    by_key = {}
    for name, addr in refs.items():
        k = data_globals.msvc_key(name)
        if k:
            by_key.setdefault(k[0], []).append((name, addr, k[1]))
    known = sorted(set(refs.values()))

    def section(addr):
        return next((s for s in secs if s['start'] <= addr < s['end']), None)

    symbols = [s.strip() for s in a.undefined.read_text().split('\n') if s.strip()]
    names = data_globals.demangle(symbols)
    bss, unsourced, unbound = [], [], []
    for sym, dem in zip(symbols, names):
        if sym.startswith('__Z'):
            if '(' in dem or dem.startswith(('vtable for', 'typeinfo', 'VTT for', 'guard variable')):
                continue
            name = dem
        else:
            name = sym[1:]
        cands = [c for c in by_key.get(name, []) if c[2] != 'func']
        addr, msvc = (cands[0][1], cands[0][0]) if cands else (None, None)
        if addr is None:
            m = re.search(r'_(?:0x)?([0-9a-fA-F]{6,8})$', name)
            if m and 0x79A000 <= int(m.group(1), 16) < 0xB2D000:
                addr = int(m.group(1), 16)
        if addr is None:
            unbound.append(dict(symbol=sym, name=name, reason='no catalog reference'))
            continue
        s = section(addr)
        if not s or s['name'] not in ('.data', '.rdata'):
            unbound.append(dict(symbol=sym, name=name, address='0x%08X' % addr,
                                reason='not data (%s)' % (s['name'] if s else 'outside the image')))
            continue
        i = bisect.bisect_right(known, addr)
        nxt = known[i] if i < len(known) else s['end']
        row = dict(symbol=sym, name=name, address='0x%08X' % addr, section=s['name'],
                   extent=max(1, min(nxt, s['end']) - addr), msvc=msvc, user=users.get(msvc) if msvc else None)
        (bss if addr >= s['raw_end'] else unsourced).append(row)

    # One storage per original address (as data_globals.py).
    objs = [p for p in (gen / 'link_inputs.txt').read_text().split('\n') if p.endswith('.o')]
    defined_at = {}
    dsyms = defined_symbols(objs)
    for sym, dem in zip(dsyms, data_globals.demangle(dsyms)):
        name = dem if sym.startswith('__Z') else sym[1:]
        if '(' in name:
            continue
        for c in by_key.get(name, []):
            if c[2] != 'func':
                defined_at.setdefault(c[1], sym)
    external, groups = [], {}
    for row in bss:
        ad = int(row['address'], 16)
        if ad in defined_at and defined_at[ad] != row['symbol']:
            external.append((row['symbol'], defined_at[ad], row['address']))
        else:
            groups.setdefault(row['address'], []).append(row)
    lines = ['// generated by ra2/tools/rb_data.py (lane P1): zero-initialised globals (.bss of the',
             '// original) that the rebuild units read but no unit defines. Storage only: original extent',
             '// (to the next known symbol) doubled for LP64, 16-byte aligned, zero as the loader gives .bss.',
             '// Names and addresses from the rebuild catalog at ra2/PIN; no bytes from the executable.', '']
    for k, (address, rows) in enumerate(sorted(groups.items())):
        rows = sorted(rows, key=lambda r: (re.match(r'^(DAT_|data_|unknown_)', r['name']) is not None, r['name']))
        owner = rows[0]
        size = max(16, 2 * max(r['extent'] for r in rows))
        lines.append('alignas(16) unsigned char rb_bss_%04d[%d] __asm__("%s");  // %s %s'
                     % (k, size, owner['symbol'], owner['address'], owner['name'][:80]))
        for other in rows[1:]:
            lines.append('__asm__(".globl %s\\n.set %s, %s\\n");  // same address: %s'
                         % (other['symbol'], other['symbol'], owner['symbol'], other['name'][:80]))
    (gen / 'rb_bss_globals.cpp').write_text('\n'.join(lines) + '\n')
    (gen / 'rb_bss_aliases.tsv').write_text('alias\ttarget\taddress\tkind\tevidence\n' + ''.join(
        '%s\t%s\t%s\tdata\tsame address\n' % e for e in sorted(external, key=lambda e: e[2])))
    cols = ('address', 'section', 'extent', 'symbol', 'name', 'msvc', 'user')
    (gen / 'rb_unsourced_initialised.tsv').write_text('\t'.join(cols) + '\n' + ''.join(
        '\t'.join(str(r[c] or '') for c in cols) + '\n' for r in sorted(unsourced, key=lambda r: r['address'])))
    (gen / 'rb_data_report.json').write_text(json.dumps(dict(
        bss_names=len(bss), bss_storages=len(groups), bss_external_aliases=len(external),
        unsourced_initialised=len(unsourced), unbound=unbound), indent=1) + '\n')
    print('rb_data: %d zero-initialised names (%d storages, %d aliases of other inputs); %d initialised '
          'listed as unsourced; %d data names not bound' % (len(bss), len(groups), len(external), len(unsourced),
                                                            len(unbound)))


if __name__ == '__main__':
    main()
