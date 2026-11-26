#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate the native (arm64, clang) view of the rebuild tree at its pinned commit (lane P1).

From redalert2-decomp's branch ports-on-rebuild (native/rb/tools/rb_units.py; adapted from its
native/yr/tools/yr_units.py). The rebuild compiles each src/<unit>.cpp as one translation unit
against ONE header directory, include/. This tool never edits the vendored copy of the rebuild
(vendor/ra2-rebuild: src/, include/, at the commit its PIN names) and writes:

  OUT/hdr/...             include/ after ra2/fixups.json and the port rewrites
  OUT/units/<name>.cpp    one native TU per src/*.cpp, fixed up and port-rewritten
  OUT/units.cmake         RB_NATIVE_UNITS for CMake
  OUT/report.json         the units and what was skipped

The port rewrites are the RA2 native tree's (tools/native_units.py port()), as YR uses them.
Fixups (ra2/fixups.json) take the yr form: {file, old, new[, regex]} text substitutions,
{file, skip_unit}, {file, append} (a ra2/glue file added to a unit), and
{file, cut_function: <regex of the definition's head>} which removes the definition through its
closing brace (the rebuild carries no per-function markers), {file, prepend: text} (at the top of
the unit) and {file, after_includes: text} (after the unit's last #include line: declarations of
explicit specialisations a unit defines after their first use, which ISO C++ requires first).

    python3 ra2/tools/ra2_units.py --out build/mac/gen
"""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import host_tools  # noqa: E402
import native_units  # noqa: E402

native_units.WIN64_PARAMS = False   # mingw's WPARAM/LPARAM types (as yr_units.py)
MSG_PARAMS = re.compile(r'\(\s*((?:struct\s+)?HWND(?:__)?\s*\*?\s*\w*)\s*,'
                        r'\s*(?:unsigned\s+int|unsigned|UINT)\b(\s*&?\s*\w*)\s*,'
                        r'\s*(?:unsigned\s+int|unsigned|UINT|WPARAM)\b(\s*&?\s*\w*)\s*,'
                        r'\s*(?:LONG|long|LPARAM)\b(\s*&?\s*\w*)\s*\)')
WINDOW_LONG = re.compile(r'\b(Set|Get)WindowLong(?=\s*\()')
FOR_INIT = re.compile(r'([;{}](?:[ \t]*(?://[^\n]*)?\n)*[ \t]*)for[ \t]*\([ \t]*'
                      r'((?:const[ \t]+)?(?:unsigned[ \t]+|signed[ \t]+)?[A-Za-z_][\w:]*(?:<[^;()]*?>)?[ \t\*&]+'
                      r'[A-Za-z_]\w*[ \t]*=[^;()]*(?:\([^;()]*\)[^;()]*)*);')

RB = ROOT / 'ra2'
VENDOR = ROOT / 'vendor/ra2-rebuild'
PIN = VENDOR / 'PIN'
FIXUPS_PATH = RB / 'fixups.json'
INCLUDE_LINE = re.compile(r'^[ \t]*#[ \t]*include\b[^\n]*\n', re.M)
HEADER_SUFFIXES = ('.h', '.hh', '.hpp', '.inl', '.c')   # .c: MIDL interface-id files units include


def read(path):
    return path.read_text(encoding='latin-1')


def pinned_tree(out):
    """The vendored rebuild tree (vendor/ra2-rebuild) and the commit its PIN names."""
    pin = next(l.split()[1] for l in PIN.read_text().split('\n') if l.startswith('commit '))
    return pin, VENDOR


def load_fixups():
    rows = json.loads(FIXUPS_PATH.read_text())['fixups'] if FIXUPS_PATH.exists() else []
    for r in rows:
        r['hits'] = 0
        if r.get('skip_unit') or 'append' in r or 'prepend' in r or 'after_includes' in r:
            r['rx'] = None
        elif 'cut_function' in r:
            r['rx'] = re.compile(r['cut_function'], re.M)
        else:
            r['rx'] = re.compile(r['old'] if r.get('regex') else re.escape(r['old']), re.M)
    return rows


def cut_definitions(text, rx, why):
    """Remove each definition whose head RX matches, through its balanced closing brace."""
    out, pos, n = [], 0, 0
    for m in rx.finditer(text):
        if m.start() < pos:
            continue
        i = text.find('{', m.end())
        if i < 0:
            continue
        depth, j = 0, i
        while j < len(text):
            c = text[j]
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        out.append(text[pos:m.start()])
        out.append('/* native: definition cut (ra2/fixups.json: %s) */' % why)
        pos = j + 1
        n += 1
    out.append(text[pos:])
    return ''.join(out), n


def applies(r, key):
    f = r['file']
    return f == '*' or f.lower() == key.lower() or (f.endswith('/*') and key.lower().startswith(f[:-1].lower()))


def apply_fixups(key, text, fixups):
    """key: 'hdr/<name>' or 'src/<name>'"""
    text = WINDOW_LONG.sub(r'\1WindowLongA', text)
    text = MSG_PARAMS.sub(lambda m: '(%s, UINT%s, WPARAM%s, LPARAM%s)' % m.groups(), text)
    text = FOR_INIT.sub(lambda m: '%s%s; for(;' % (m.group(1), m.group(2)), text)
    for r in fixups:
        if r['rx'] is None or not applies(r, key):
            continue
        if 'cut_function' in r:
            text, n = cut_definitions(text, r['rx'], r.get('why', '')[:80])
        else:
            text, n = r['rx'].subn((lambda m, r=r: m.expand(r['new'])) if r.get('regex') else (lambda m, r=r: r['new']), text)
        r['hits'] += n
    for r in fixups:
        if 'after_includes' in r and r['file'].lower() == key.lower():
            last = None
            for m in INCLUDE_LINE.finditer(text):
                last = m
            at = last.end() if last else 0
            text = text[:at] + '// native (ra2/fixups.json):\n' + r['after_includes'] + '\n' + text[at:]
            r['hits'] += 1
        if 'prepend' in r and r['file'].lower() == key.lower():
            text = '// native (ra2/fixups.json):\n' + r['prepend'] + '\n' + text
            r['hits'] += 1
    for r in fixups:
        if 'append' in r and r['file'].lower() == key.lower():
            text += '\n#line 1 "ra2/glue/%s"\n' % r['append'] + read(RB / 'glue' / r['append'])
            r['hits'] += 1
    return text


def emit(target, text):
    target.parent.mkdir(parents=True, exist_ok=True)
    if not target.exists() or target.read_text(encoding='latin-1') != text:
        target.write_text(text, encoding='latin-1')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--out', required=True)
    ap.add_argument('--only', default='', help='comma-separated unit names (probe runs)')
    a = ap.parse_args()
    out = Path(a.out).resolve()
    pin, tree = pinned_tree(out)
    inc = tree / 'include'
    headers = {}
    for h in inc.rglob('*'):
        if h.is_file() and h.suffix.lower() in HEADER_SUFFIXES:
            headers[h.relative_to(inc).as_posix().lower()] = h
    fixups = load_fixups()
    for k, p in headers.items():
        text = native_units.port(apply_fixups('hdr/' + k, read(p), fixups))
        emit(out / 'hdr' / k, text)
        spelled = p.relative_to(inc).as_posix()
        if spelled != k:   # case-insensitive includes (VC6 on Windows): both spellings
            emit(out / 'hdr' / spelled, text)
    only = set(x for x in a.only.split(',') if x)
    units, skipped = [], []
    for src in sorted((tree / 'src').glob('*.cpp')):
        rel = src.name
        if only and rel not in only and src.stem not in only:
            continue
        key = 'src/' + rel
        skip = [r for r in fixups if r.get('skip_unit') and r['file'].lower() == key.lower()]
        if skip:
            skipped.append(rel)
            for r in skip:
                r['hits'] += 1
            continue
        target = out / 'units' / rel
        emit(target, '#line 1 "vendor/ra2-rebuild/src/%s"\n' % rel + native_units.port(apply_fixups(key, read(src), fixups)))
        units.append(target)
    unused = [r.get('old', r.get('cut_function', r['file'])) for r in fixups
              if not r['hits'] and (not only or r['file'] == '*' or r['file'].startswith('hdr/'))]
    if unused:
        raise SystemExit('ra2_units: fixups that match nothing: %s' % unused)
    for stale in ([] if only else (out / "units").glob("*.cpp")):   # units left out since the last run
        if stale not in units:
            stale.unlink()
    # Link order: the original's _initterm order (ra2/data/initterm.json, rb_rekey.py). The
    # compiler emits each unit's initialisers and ld64 keeps input order, so the units without one
    # come first (source order), then each unit at its first original initialiser (as main's
    # native_units.py initterm_order). In the rebuild every initialiser is the unit's own static
    # object (main's "explicit" entries are named init functions of its split files): no calls.
    order = RB / 'data/initterm.json'
    if order.exists():
        first = {}
        for e in json.loads(order.read_text())['entries']:
            if not e.get('unmapped'):
                first.setdefault(Path(e['source']).name, e['index'])
        units.sort(key=lambda u: (u.name in first, first.get(u.name, 0)))
    emit(out / 'units.cmake', host_tools.cmake_list('RB_NATIVE_UNITS', units))
    emit(out / 'report.json', json.dumps({'pin': pin, 'headers': len(headers), 'units': len(units),
                                          'skipped': skipped}, indent=1) + '\n')
    print('ra2_units: pin %s, %d headers, %d units (%d left out) -> %s' % (pin[:10], len(headers), len(units), len(skipped), out))


if __name__ == '__main__':
    main()
