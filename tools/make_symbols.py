#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write vendor/ra2-rebuild/symbols.tsv: the small name/address index the port's build reads in place of the
rebuild catalog (rebuild/manifest/catalog in redalert2-decomp, about 57 MB).

Two kinds of rows, in the catalog's own order (the readers keep the first row of a name, as they did over
the catalog):

    region  <entry>  <symbol>  <population>  <source>     a function or data region the rebuild defines
    ref     <address>  <name>  <population>               a name a population's objects reference

Names and addresses only; no bytes of any executable. Run by tools/update_rebuild.sh in a redalert2-decomp
checkout (it needs that checkout's tools/original_target/catalog_io.py):

    python3 tools/make_symbols.py --decomp <checkout> --catalog <extracted rebuild/manifest/catalog> --out symbols.tsv
"""
import argparse
import sys
from pathlib import Path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--decomp', type=Path, required=True)
    ap.add_argument('--catalog', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    sys.path.insert(0, str(a.decomp / 'tools/original_target'))
    import catalog_io  # noqa: E402
    cat = catalog_io.load(a.catalog, references=True)
    lines = ['# kind\taddress\tname\tpopulation\tsource']
    for r in cat['regions']:
        lines.append('region\t0x%08X\t%s\t%s\t%s' % (int(r['entry']), r['symbol'], r['population'], r['source']))
    for pop, table in cat['population_references'].items():
        for name, addr in table.items():
            lines.append('ref\t0x%08X\t%s\t%s' % (int(addr), name, pop))
    a.out.write_text('\n'.join(lines) + '\n')
    print('make_symbols: %d regions, %d references' % (len(cat['regions']),
                                                      sum(len(t) for t in cat['population_references'].values())))


if __name__ == '__main__':
    main()
