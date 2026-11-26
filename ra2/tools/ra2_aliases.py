#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Function address aliases for the rebuild link (lane P2, P1's next step "address aliases").

The rebuild often names one original function twice: a unit declares it under one name (movie.cpp's
Set_Primary_Audio_Format, 0x0040A6E0) and another unit defines it under another (drv_dsnd.cpp's
AudioSetPrimaryFormat). VC6 resolved both to the address; natively the declared name is undefined
and the link closure made it a trap. This binds each undefined function of the link to the function
the rebuild defines at the same original address, through the rebuild catalog (manifests/catalog
at the vendored commit, as vendor/ra2-rebuild/symbols.tsv: its reference tables name -> address, its regions address -> the defining
symbol), as an ld64 alias row (link_closure.py --aliases).

A row is written only when it is safe to call through: the undefined name's qualified name maps to
exactly one address; the region there names one defining symbol whose Mach-O spelling is defined in
the link inputs (one match by qualified name and arity); and both take the same number of arguments,
counting a non-static member's `this`, in the same shape (each argument a pointer or reference, or
not: a placeholder declaration that passes integers where the definition takes pointers stays a
trap). A member and a free function of one address with that shape are the same code, as VC6's
identical-code folding or the rebuild's two names left them. The rest stay traps.

    python3 ra2/tools/ra2_aliases.py --gen build/mac/gen --undefined U.txt --inputs link_inputs.txt
        [--extra rb_bss_aliases.tsv] --out rb_aliases.tsv
"""
import argparse
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/original_target'))
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(Path(__file__).resolve().parent))
import data_globals  # noqa: E402
import ra2_data       # noqa: E402


def split_params(text):
    """Top-level comma-separated parts of a demangled parameter list."""
    depth, cur, out = 0, '', []
    for ch in text:
        if ch in '<(':
            depth += 1
        elif ch in '>)':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip())
            cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return [p for p in out if p != 'void']


def parse_itanium(demangled):
    """(qualified name, parameter count) of a demangled C++ function, or (name, None) for C."""
    if '(' not in demangled:
        return demangled, None
    # The qualified name ends at the parameter list's '(' at depth 0.
    depth = 0
    for i, ch in enumerate(demangled):
        if ch == '<':
            depth += 1
        elif ch == '>':
            depth -= 1
        elif ch == '(' and depth == 0:
            name = demangled[:i]
            rest = demangled[i + 1:]
            break
    else:
        return demangled, None
    depth, end = 1, 0
    for j, ch in enumerate(rest):
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
            if depth == 0:
                end = j
                break
    params = split_params(rest[:end])
    return name, len(params) - (1 if params == ['...'] else 0) if params else 0


BASIC = {'int', 'unsigned', 'signed', 'char', 'short', 'long', 'bool', 'float', 'double', 'void', 'const', 'volatile', 'wchar_t'}


def class_names(gen):
    """Class and struct names the generated headers and units define, less namespace names: a
    qualifier naming one of them makes a function a member (its `this` an argument)."""
    classes, spaces = set(), set()
    for path in list((gen / 'hdr').rglob('*.h')) + list((gen / 'units').glob('*.cpp')):
        text = path.read_text(encoding='latin-1')
        classes.update(re.findall(r'\b(?:class|struct)\s+(?:__declspec\([^)]*\)\s*)?(\w+)\s*(?::[^;{]*)?\{', text))
        spaces.update(re.findall(r'\bnamespace\s+(\w+)\s*\{', text))
    return classes - spaces


def shape(dem, q, classes):
    """The argument shape a call passes: 'p' for a pointer or reference (and a member's this),
    's' for anything else, in order; None for a C name."""
    if '(' not in dem:
        return None
    inner = dem[len(q) + 1:]
    depth, end = 1, len(inner)
    for j, ch in enumerate(inner):
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
            if depth == 0:
                end = j
                break
    params = split_params(inner[:end])
    owner = re.sub(r'<.*$', '', q.rsplit('::', 1)[0].split('::')[-1]) if '::' in q else None
    out = 'p' if owner in classes else ''
    return out + ''.join('p' if ('*' in t or '&' in t) else ('v' if t == '...' else 's') for t in params)


def effective_arity(q, n, classes):
    if n is None:
        return None
    owner = q.rsplit('::', 1)[0].split('::')[-1] if '::' in q else None
    owner = re.sub(r'<.*$', '', owner) if owner else None
    return n + (1 if owner in classes else 0)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--gen', type=Path, required=True)
    ap.add_argument('--undefined', type=Path, required=True)
    ap.add_argument('--inputs', type=Path, required=True, help='the link inputs (objects, archives) whose definitions are targets')
    ap.add_argument('--extra', type=Path, nargs='*', default=[], help='alias TSVs copied into the output first')
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()

    cat, refs, _users = ra2_data.load_catalog(a.gen)
    classes = class_names(a.gen)
    by_name = defaultdict(set)
    decorated = defaultdict(set)
    for name, addr in refs.items():
        k = data_globals.msvc_key(name)
        if k and k[1] != 'data':
            by_name[k[0]].add(int(addr))
            decorated[k[0]].add((int(addr), name))
    region_at = defaultdict(set)
    for r in cat['regions']:
        region_at[int(r['entry'])].add(r['symbol'])

    undefined = [s for s in a.undefined.read_text().split('\n') if s]
    inputs = [l for l in a.inputs.read_text().split('\n') if l]
    defined = ra2_data.nm_defined(inputs)
    dem_defined = data_globals.demangle([s[1:] if s.startswith('_') else s for s in defined])
    by_qual = defaultdict(list)
    for sym, dem in zip(defined, dem_defined):
        q, n = parse_itanium(dem)
        by_qual[q].append((sym, n, dem))
    dem_undef = data_globals.demangle([s[1:] if s.startswith('_') else s for s in undefined])

    rows, skipped = [], defaultdict(int)
    for sym, dem in zip(undefined, dem_undef):
        q, n = parse_itanium(dem)
        addrs = by_name.get(q, set())
        if len(addrs) > 1:
            # Overloads: the address whose decorated name spells every named argument type of this
            # declaration (W4TActionType@@ for an enum TActionType, V/U<Name>@@ for a class).
            names = [w for w in re.findall(r'[A-Za-z_]\w*', dem[len(q):]) if w not in BASIC]
            picked = {ad for ad, nm in decorated[q] if names and all('%s@@' % w in nm for w in names)}
            if len(picked) == 1:
                addrs = picked
        if len(addrs) != 1:
            skipped['no single address' if not addrs else 'several addresses'] += 1
            continue
        addr = next(iter(addrs))
        targets = region_at.get(addr, set())
        if len(targets) != 1:
            skipped['no defining region' if not targets else 'several regions'] += 1
            continue
        tsym = next(iter(targets))
        tk = data_globals.msvc_key(tsym)
        if not tk:
            skipped['target not keyed'] += 1
            continue
        cands = by_qual.get(tk[0], [])
        want = shape(dem, q, classes)
        matches = [csym for csym, cn, cdem in cands
                   if csym != sym and shape(cdem, tk[0], classes) == want]
        if len(matches) != 1:
            skipped['target not defined once' if not matches else 'several targets'] += 1
            continue
        rows.append((sym, matches[0], addr, q, tk[0]))

    lines = ['alias\ttarget\taddress\tkind\tevidence']
    for extra in a.extra:
        if extra.exists():
            lines += [l for l in extra.read_text().split('\n') if l and not l.startswith('alias\t')]
    for sym, target, addr, q, tq in rows:
        lines.append('%s\t%s\t0x%08X\tfunction\tsame address in the rebuild catalog (%s = %s)' % (sym, target, addr, q, tq))
    a.out.write_text('\n'.join(lines) + '\n')
    print('ra2_aliases: %d function aliases by address (%s)' % (
        len(rows), ', '.join('%d %s' % (v, k) for k, v in sorted(skipped.items()))))


if __name__ == '__main__':
    main()
