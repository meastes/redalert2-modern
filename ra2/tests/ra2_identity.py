#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Frame identity between the old src-based port and this port (lane P8, branch ports-on-rebuild).

One mission is started the same way in both binaries, with the same seed and the same orders, and the
mission soak's per-period digests are compared (main's native/link/harness_soak.cpp in the src-based port,
glue/harness_input_native.inc in this port: the same hash over every techno's type, house, coordinate,
strength and mission, every house's money and the scenario's random generator, in the same order):

  campaign  Single Player, New Campaign, the side's emblem (main's tests/mission_soak.py and campaign_chain.py
            route): the side's first mission (Allied 1 or Soviet 1) is played (no forced end).
  skirmish  Single Player, Skirmish, Start Game (ra2_routes.py's START), the sides and colours fixed in RA2.INI (lane P10).

Both run with RA2_NATIVE_SOAK=<period>, RA2_NATIVE_SOAK_SEED (CustomSeed for Init_Random), RA2_NATIVE_SOAK_SPEED=0
(as fast as the machine draws), RA2_NATIVE_SOAK_END=<frames>@1 (--run-to=soak-end), each in a fresh write folder,
the user's installed game read at run time. The report names the first digest that differs (frame, the two
lines) and, with --dump, both runs' soak-techno lines at that frame (RA2_NATIVE_SOAK_DUMP) and the first techno
that differs.

  ra2_identity.py --src <ra2-native> --rb <ra2-native> --data-dir <game folder> --game-exe <game.exe>
                 --work <dir> [--route campaign|skirmish] [--side allied|soviet] [--frames 3000] [--period 50]
                 [--orders "<loop frame> <order>; ..."] [--dump]

Exit 0 when every digest both runs reached is the same, 1 on a divergence, a fault or a run that did not start,
77 without the installed game.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tests'))
import game_process  # noqa: E402

SKIP = 77
SEED = 20261121
BAD = re.compile(r'unrecovered|\btrap\b|signal \d+|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:', re.I)
DIGEST = re.compile(r'^soak-digest (\d+) (\S+) technos (\d+) player (\d+) credits (-?\d+) rng (\S+) view (-?\d+),(-?\d+)', re.M)
EMBLEM = {'allied': (433, 179), 'soviet': (429, 448)}


def script(route, side):
    if route == 'campaign':
        return ['s1+1500 click 829 302', 's2+1500 click 831 305', 's3+2000 click %d %d' % EMBLEM[side]]
    return ['s1+1000 click 834 304', 's2+1000 click 834 388', 's3+1500 click 834 346']


# Lane P10: the skirmish start made repeatable. The setup's sides and colours default to Random (-2), and Start Game resolves
# them (SessionClass::Assign_Random_Choices: Random_Side, Random_Color) from Scen->RandomNumber before Select_Game's Init_Random
# applies CustomSeed (RA2_NATIVE_SOAK_SEED). That generator still holds Init_Game's own Init_Random seed, GetTickCount()
# (CustomSeed is still 0 then), so each run drew its own human side (Americans or Arabs, ...) and computer side: the two
# start-ups P9 saw (52 or 60 technos at frame 50). The original reads the choices from RA2.INI ([MultiPlayer] Side, Color:
# SessionClass::Read_MultiPlayer_Settings; [Skirmish] Slot01=type,side,color: SessionOptionsClass::Read), so each run's write
# folder gets the installed RA2.INI with them fixed: the human Americans in colour 0, one computer (type 6, the default) Russians
# in colour 1. Both binaries then start the same game every run (no change to either port).
SKIRMISH_INI = '\n[MultiPlayer]\nSide=%s\nColor=0\n\n[Skirmish]\nSlot01=6,%d,1\n'


def skirmish_ini(data_dir, write, sides='Americans,8'):
    text = ''
    src = os.path.join(data_dir, 'RA2.INI')
    if os.path.isfile(src):
        with open(src, errors='replace') as f:
            text = f.read()
    with open(os.path.join(write, 'RA2.INI'), 'w') as f:
        human, computer = sides.split(',')
        f.write(text.rstrip('\n') + '\n' + SKIRMISH_INI % (human, int(computer)))


def run(name, binary, a, work, dump_at=None):
    d = os.path.join(work, name)
    shutil.rmtree(d, ignore_errors=True)
    write = os.path.join(d, 'write')
    os.makedirs(write)
    path = os.path.join(d, 'input.script')
    with open(path, 'w') as f:
        f.write('\n'.join(script(a.route, a.side)) + '\n')
    if a.route == 'skirmish':
        skirmish_ini(a.data_dir, write, a.sides)
    env = dict(os.environ, RA2_GAME_EXE=a.game_exe, RA2_NATIVE_CRASH_TRACE='1', RA2_NATIVE_IGNORE_FOCUS='1',
               RA2_NATIVE_MOVIE_SKIP='1', RA2_NATIVE_SOAK=str(a.period), RA2_NATIVE_SOAK_SEED=str(SEED),
               RA2_NATIVE_SOAK_SPEED='0', RA2_NATIVE_SOAK_END='%d@1' % a.frames)
    for k in ('RA2_NATIVE_ORDERS', 'RA2_NATIVE_ENHANCEMENTS', 'RA2_NATIVE_TRAP_CONTINUE', 'RA2_NATIVE_SOAK_DUMP',
              'RA2_NATIVE_FORCE_END'):
        env.pop(k, None)
    if a.orders:
        env['RA2_NATIVE_ORDERS'] = a.orders
    if dump_at is not None:
        env['RA2_NATIVE_SOAK_DUMP'] = str(dump_at)
    cmd = [binary, '--data-dir', a.data_dir, '--write-dir=' + write, '--game-args=-noaudio -CD.',
           '--run-to=soak-end', '--run-seconds=%d' % a.seconds, '--input-script=' + path]
    try:
        p = game_process.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace',
                             timeout=a.seconds + 90)
        out = p.stdout
    except subprocess.TimeoutExpired as x:
        out = x.stdout.decode('utf-8', 'replace') if isinstance(x.stdout, bytes) else (x.stdout or '')
    with open(os.path.join(d, 'run.log'), 'w') as f:
        f.write(out)
    return out


def digests(out):
    return {int(m.group(1)): m.group(0) for m in DIGEST.finditer(out)}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--src', required=True)
    ap.add_argument('--rb', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--game-exe', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--route', default='campaign', choices=['campaign', 'skirmish'])
    ap.add_argument('--side', default='allied', choices=sorted(EMBLEM))
    ap.add_argument('--frames', type=int, default=3000)
    ap.add_argument('--period', type=int, default=50)
    ap.add_argument('--seconds', type=int, default=600)
    ap.add_argument('--orders', default='')
    ap.add_argument('--dump', action='store_true')
    ap.add_argument('--sides', default='Americans,8',
                    help="skirmish: the human's house name and the computer's house index (RA2.INI's Side and Slot01)")
    ap.add_argument('--reuse-src', default='',
                    help='a folder holding an earlier src run (src/run.log, src-dump/run.log) of the same route, frames and orders: '
                         'the src-based port is not run again')
    a = ap.parse_args()
    if not os.path.isfile(os.path.join(a.data_dir, 'game.exe')) or not os.path.isfile(a.game_exe):
        print('identity: skipped: no installed game or no pinned game.exe')
        return SKIP
    work = os.path.abspath(a.work)
    os.makedirs(work, exist_ok=True)
    outs = {}
    for name, binary in (('src', a.src), ('rb', a.rb)):
        kept = os.path.join(a.reuse_src, name, 'run.log') if a.reuse_src and name == 'src' else ''
        if kept and os.path.isfile(kept):
            with open(kept, errors='replace') as f:
                outs[name] = f.read()
            continue
        outs[name] = run(name, binary, a, work)
    bad = []
    for name, out in outs.items():
        hits = [l for l in out.splitlines() if BAD.search(l) and not l.startswith('input-script')]
        if hits:
            bad.append('%s: fault or trap: %s' % (name, hits[0][:160]))
    ds, dt = digests(outs['src']), digests(outs['rb'])
    if not ds or not dt:
        bad.append('no digests (src %d, rb %d)' % (len(ds), len(dt)))
    common = sorted(set(ds) & set(dt))
    first = next((f for f in common if ds[f].split(' view ')[0] != dt[f].split(' view ')[0]), None)
    view_first = next((f for f in common if ds[f] != dt[f]), None)
    print('identity: %s %s, %d digests in common (src %d, rb %d), last common frame %s' %
          (a.route, a.side if a.route == 'campaign' else '', len(common), len(ds), len(dt), common[-1] if common else '-'))
    if first is None:
        print('identity: every common digest is the same (state)%s' %
              ('' if view_first is None else '; the view differs first at frame %d' % view_first))
    else:
        print('identity: first divergence at frame %d' % first)
        print('  src:   %s' % ds[first])
        print('  rb:    %s' % dt[first])
        prev = [f for f in common if f < first]
        if prev:
            print('  (same at frame %d)' % prev[-1])
        bad.append('divergence at frame %d' % first)
        if a.dump:
            frame = prev[-1] + 1 if prev else first
            # every techno at each frame from the last equal digest to the first unequal one
            rng = '%d-%d' % (frame, first)
            os.environ['RB_DUMP_FRAMES'] = rng
            dumps = {}
            for name, binary in (('src', a.src), ('rb', a.rb)):
                kept = os.path.join(a.reuse_src, name + '-dump', 'run.log') if a.reuse_src and name == 'src' else ''
                if kept and os.path.isfile(kept) and ('soak-techno %d ' % frame) in open(kept, errors='replace').read():
                    with open(kept, errors='replace') as f:
                        out = f.read()
                else:
                    out = run(name + '-dump', binary, a, work, dump_at=rng)
                dumps[name] = [l for l in out.splitlines() if l.startswith('soak-techno ')]
            norm = re.compile(r'^(soak-techno \d+ #\d+ \S+ h-?\d+ -?\d+,-?\d+,-?\d+ s-?\d+ m-?\d+)')
            a_lines = [norm.match(l).group(1) for l in dumps['src'] if norm.match(l)]
            b_lines = [norm.match(l).group(1) for l in dumps['rb'] if norm.match(l)]
            for x, y in zip(a_lines, b_lines):
                if x != y:
                    print('  first techno difference:\n    src:   %s\n    rb:    %s' % (x, y))
                    break
            else:
                print('  techno dumps the same over %s (%d lines each)' % (rng, min(len(a_lines), len(b_lines))))
    for b in bad:
        print('identity: FAIL: %s' % b)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
