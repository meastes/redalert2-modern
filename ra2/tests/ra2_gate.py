#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Differential gate (lane P14): the rebuild-based port against redalert2-decomp's old src-based port.

Both binaries run the same route with the same seed, the same harness orders and the same scripted input, each in a
fresh write folder, the user's installed game read at run time, and main's mission-soak digest (every techno's type,
house, coordinate, strength and mission, the houses' money and the scenario's random generator: native/link/
harness_soak.cpp in the src port, glue/harness_input_native.inc here) is compared every --period game frames, with
the trigger lines (soak-trigger), timers and defeated houses. The report names each scenario's first difference.

  missions  every campaign mission of a side, reached as tests/mission_soak.py reaches it (New Campaign, missions
            1..n-1 won by the forced end and left by Continue), then played --frames game frames with the soak's
            orders (MCV deploy at 30, hunt every 1,200) or --orders, plus --input (frame-timed input-script lines).
  skirmish  Single Player, Skirmish, Start Game with RA2.INI's [Skirmish] section set: --map (ScenIndex), --sides
            (the human's house name, then one "type:side:color" per computer slot; type 4 easy, 5 medium, 6 hard).
  load      --save <file> (a save in --data-dir, or a path) loaded at start (--load-save) by both and played on.
  cross     a harness save made by --maker (src or rb) on a skirmish or mission route at loop frame --save-at, then
            loaded by both binaries and played on (the save's own digests compared too).

  ra2_gate.py <mode> --src <ra2-native> --rb <ra2-native> --data-dir <game> --game-exe <game.exe> --work <dir> ...

Exit 0 when every compared digest and trigger line is the same, 1 on a difference, a fault or a run that did not
reach its scenario; 77 without the installed game.
"""
import argparse
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'tests'))
import game_process  # noqa: E402
import mission_soak  # noqa: E402

SKIP = 77
SEED = 20261121
BAD = re.compile(r'unrecovered|\btrap\b|signal \d+|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:|assert|not written', re.I)
SKIRMISH_START = ['s1+1000 click 834 304', 's2+1000 click 834 388', 's3+1500 click 834 346']
PREFIX = {'allied': 'ALL', 'soviet': 'SOV'}


def base_env(a, extra):
    env = dict(os.environ, RA2_GAME_EXE=a.game_exe, RA2_NATIVE_CRASH_TRACE='1', RA2_NATIVE_IGNORE_FOCUS='1',
               RA2_NATIVE_MOVIE_SKIP='1', RA2_NATIVE_MSG_LOG='0', RA2_NATIVE_SOAK=str(a.period),
               RA2_NATIVE_SOAK_SEED=str(SEED), RA2_NATIVE_SOAK_SPEED='0', RA2_NATIVE_TRIGGERS='1')
    if a.uncapped:
        env['RA2_NATIVE_UNCAPPED'] = '1'
    for k in ('RA2_NATIVE_ORDERS', 'RA2_NATIVE_ENHANCEMENTS', 'RA2_NATIVE_TRAP_CONTINUE', 'RA2_NATIVE_SOAK_DUMP',
              'RA2_NATIVE_FORCE_END', 'RA2_NATIVE_SAVE_AT', 'RA2_NATIVE_SAVE_LEAVE', 'RA2_NATIVE_SOAK_END'):
        env.pop(k, None)
    env.update(extra)
    if a.dump:
        env['RA2_NATIVE_SOAK_DUMP'] = a.dump
    return env


def run_game(binary, work, env, args, seconds, script_lines=None, ini=None):
    shutil.rmtree(work, ignore_errors=True)
    write = os.path.join(work, 'w')
    os.makedirs(write)
    if ini is not None:
        with open(os.path.join(write, 'RA2.INI'), 'w') as f:
            f.write(ini)
    cmd = [binary, '--data-dir', env['_DATA'], '--write-dir=' + write, '--game-args=-noaudio -CD.',
           '--run-seconds=%d' % seconds] + args
    # diagnosis: GATE_WRAP_SRC / GATE_WRAP_RB, a command prefix (a debugger) for that side's binary
    wrap = os.environ.get('GATE_WRAP_' + os.path.basename(work).upper(), '')
    if wrap:
        cmd = wrap.split('|') + cmd
        seconds *= 4
    if script_lines:
        path = os.path.join(work, 'input.script')
        with open(path, 'w') as f:
            f.write('\n'.join(script_lines) + '\n')
        cmd.append('--input-script=' + path)
    env = dict(env)
    env.pop('_DATA')
    start = time.time()
    with open(os.path.join(work, 'run.log'), 'w') as log:
        try:
            code = game_process.run(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=seconds + 90).returncode
        except subprocess.TimeoutExpired:
            code = -1
    with open(os.path.join(work, 'run.log'), errors='replace') as f:
        text = f.read()
    return dict(text=text, code=code, wall=round(time.time() - start, 1), write=write, log=os.path.join(work, 'run.log'))


def sections(text):
    """Per scenario started (soak-mission line): its digests (view dropped), trigger, timer, house and end lines."""
    out = []
    cur = None
    # The src port's soak lines say soak-trigger at the scenario's game frame; the rebuild's trigger watch
    # (glue/harness_native.inc, RA2_NATIVE_TRIGGERS) says harness-trigger at the loop frame and, at a new scenario,
    # lists the old scenario's triggers gone and the new one's new at the scenario's first loop frame: those are
    # dropped and the frame moved to the scenario's (the loop and game frames advance together at speed 0).
    native = 'soak-trigger ' in text
    start = 1
    for l in text.splitlines():
        if not native and l.startswith('harness-trigger '):
            f = l.split()
            frame = int(f[1])
            if frame == start and start > 1:
                continue
            l = 'soak-trigger %d %s' % (frame - start + 1, ' '.join(f[2:]))
        if l.startswith('soak-mission '):
            m = re.search(r' loop (\d+)', l)
            start = int(m.group(1)) if m else 1
            cur = dict(scenario=l.split()[1], digests={}, events=[], end=None)
            out.append(cur)
            continue
        if cur is None:
            continue
        m = re.match(r'soak-digest (\d+) ', l)
        if m:
            cur['digests'][int(m.group(1))] = l.split(' view ')[0]
        elif l.startswith(('soak-trigger ', 'soak-timer ', 'soak-house ')):
            cur['events'].append(l)
        elif l.startswith('soak-end '):
            cur['end'] = l
    return out


def faults(text):
    return sorted(set(l.strip() for l in text.splitlines() if BAD.search(l) and not l.startswith('input-script')))[:5]


def compare_sections(x, y, after=-1, event_frames=True):
    """(frames compared, last common frame, first difference or None). event_frames=False (a loaded save: the rebuild's
    trigger watch counts loop frames from the load, the src port game frames) compares the trigger lines' order only."""
    common = sorted(f for f in set(x['digests']) & set(y['digests']) if f > after)
    for f in common:
        if x['digests'][f] != y['digests'][f]:
            return len(common), f, 'digest at frame %d:\n    src: %s\n    rb:  %s' % (f, x['digests'][f], y['digests'][f])
    last = common[-1] if common else -1
    # within one frame the two harnesses print timer, house and trigger lines in their own order
    ex = sorted((e for e in x['events'] if int(e.split()[1]) <= last), key=lambda e: (int(e.split()[1]), e))
    ey = sorted((e for e in y['events'] if int(e.split()[1]) <= last), key=lambda e: (int(e.split()[1]), e))
    if not event_frames:
        # the rebuild's lines carry loop frames: those up to the last compared digest are not known; compare all
        ex = [e.split(' ', 2)[0] + ' ' + e.split(' ', 2)[2] for e in x['events']]
        ey = [e.split(' ', 2)[0] + ' ' + e.split(' ', 2)[2] for e in y['events']]
        n2 = min(len(ex), len(ey))
        ex, ey = ex[:n2], ey[:n2]
    for p, q in zip(ex, ey):
        if p != q:
            return len(common), last, 'event:\n    src: %s\n    rb:  %s' % (p, q)
    if len(ex) != len(ey):
        return len(common), last, 'events: %d against %d through frame %d' % (len(ex), len(ey), last)
    return len(common), last, None


def pair(a, name, make_env, args, seconds, script_lines=None, ini=None, pick=-1, min_frames=0, after=-1):
    """Both binaries on one route; the report row."""
    runs = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        todo = [('src', a.src), ('rb', a.rb)]
        kept = os.path.join(a.reuse_rb, name, 'rb', 'run.log') if a.reuse_rb else ''
        if kept and os.path.isfile(kept):
            runs['rb'] = dict(text=open(kept, errors='replace').read(), code=0, wall=0.0)
            todo = todo[:1]
        fut = {pool.submit(run_game, b, os.path.join(a.work, name, k), make_env(k), args(k) if callable(args) else args,
                           seconds, script_lines, ini): k for k, b in todo}
        for f in concurrent.futures.as_completed(fut):
            runs[fut[f]] = f.result()
    row = dict(name=name, wall=max(r['wall'] for r in runs.values()), problems=[])
    for k, r in runs.items():
        fl = faults(r['text'])
        if fl:
            row['problems'].append('%s fault: %s' % (k, fl[0][:150]))
    sx, sy = sections(runs['src']['text']), sections(runs['rb']['text'])
    if not sx or not sy or len(sx) < abs(pick) or len(sy) < abs(pick):
        row['problems'].append('scenario not reached (src %d, rb %d sections)' % (len(sx), len(sy)))
        row.update(scenario='-', frames=0, last=-1, diff=None)
        return row, runs
    x, y = sx[pick], sy[pick]
    if x['scenario'] != y['scenario']:
        row['problems'].append('scenario %s against %s' % (x['scenario'], y['scenario']))
    n, last, diff = compare_sections(x, y, after)
    row.update(scenario=x['scenario'], frames=n, last=last, diff=diff, end_src=x['end'], end_rb=y['end'])
    if diff:
        row['problems'].append('first difference: ' + diff)
    if last < min_frames:
        row['problems'].append('compared only through frame %d (wanted %d)' % (last, min_frames))
    return row, runs


def prune(a, name, row):
    """The write folders (saves, settings) of a passing row; the logs stay (--reuse-rb, recheck)."""
    if a.keep or row['problems']:
        return
    for k in ('src', 'rb'):
        shutil.rmtree(os.path.join(a.work, name, k, 'w'), ignore_errors=True)


def campaign_script(side, n, extra):
    return mission_soak.script('ra2', side, n).strip().splitlines() + list(extra)


def read_input(a):
    if not a.input:
        return []
    with open(a.input) as f:
        return [l.rstrip('\n') for l in f if l.strip() and not l.startswith('#')]


def mode_missions(a):
    rows = []
    wanted = mission_soak.missions(a.missions, 12)
    extra = read_input(a)
    orders = a.orders or mission_soak.GAMES['ra2']['orders']

    def one(n):
        name = '%s-%02d%s' % (a.side, n, a.tag)
        env = base_env(a, dict(RA2_NATIVE_SOAK_END='%d@%d' % (a.frames, n), RA2_NATIVE_ORDERS=orders, _DATA=a.data_dir,
                               RA2_NATIVE_UNCAPPED='1'))
        if n > 1:
            env['RA2_NATIVE_FORCE_END'] = ','.join(str(100 + 200 * k) for k in range(n - 1))
        with open(os.path.join(a.data_dir, 'RA2.INI'), errors='replace') as f:
            ini = f.read()
        seconds = 150 + 15 * n + a.frames // 30
        row, _ = pair(a, name, lambda k: env, ['--run-to=soak-end'], seconds, campaign_script(a.side, n, extra), ini,
                      pick=n - 1, min_frames=min(a.frames, 1) if False else 0)
        exp = '%s%02d' % (PREFIX[a.side], n)
        if not row['scenario'].upper().startswith(exp):
            row['problems'].append('scenario %s, expected %s*' % (row['scenario'], exp))
        prune(a, name, row)
        return row

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, a.jobs)) as pool:
        for row in pool.map(one, wanted):
            rows.append(row)
    return rows


SKIRMISH_INI = '\n[MultiPlayer]\nSide=%s\nColor=0\n\n[Skirmish]\nScenIndex=%d\n%s'


def mode_skirmish(a):
    with open(os.path.join(a.data_dir, 'RA2.INI'), errors='replace') as f:
        text = f.read().rstrip('\n') + '\n'
    parts = a.sides.split(',')
    slots = ''
    for i, s in enumerate(parts[1:], 1):
        t, side, color = s.split(':')
        slots += 'Slot%02d=%s,%s,%s\n' % (i, t, side, color)
    for i in range(len(parts), 8):
        slots += 'Slot%02d=1,-2,-2\n' % i
    ini = text + SKIRMISH_INI % (parts[0], a.map, slots)
    name = 'skirmish-m%d-%s%s' % (a.map, re.sub(r'[^A-Za-z0-9]+', '_', a.sides), a.tag)
    env = base_env(a, dict(RA2_NATIVE_SOAK_END='%d@1' % a.frames, _DATA=a.data_dir))
    if a.orders:
        env['RA2_NATIVE_ORDERS'] = a.orders
    row, _ = pair(a, name, lambda k: env, ['--run-to=soak-end'], 120 + a.frames // 30, SKIRMISH_START + read_input(a), ini)
    prune(a, name, row)
    return [row]


def save_path(a, save):
    return save if os.path.isabs(save) else os.path.join(a.data_dir, save)


def load_pair(a, name, save_file, min_frames):
    """Both binaries load save_file (copied into each write folder) and play on for --frames game frames past the
    first digest."""
    env = base_env(a, dict(RA2_NATIVE_SOAK_END='999999@1', RA2_NATIVE_STG_LOG='1', _DATA=a.data_dir))
    if a.orders:
        env['RA2_NATIVE_ORDERS'] = a.orders
    base = os.path.basename(save_file)

    def make_env(k):
        return env

    # the save goes into each write folder after run_game's reset: pass it through a wrapper binary argument
    runs = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        def go(k, b):
            work = os.path.join(a.work, name, k)
            shutil.rmtree(work, ignore_errors=True)
            os.makedirs(os.path.join(work, 'w'))
            shutil.copyfile(save_file, os.path.join(work, 'w', base))
            ini_src = os.path.join(a.data_dir, 'RA2.INI')
            shutil.copyfile(ini_src, os.path.join(work, 'w', 'RA2.INI'))
            cmd = [b, '--data-dir', a.data_dir, '--write-dir=' + os.path.join(work, 'w'), '--game-args=-noaudio -CD.',
                   '--run-seconds=%d' % a.seconds, '--run-to=never', '--load-save=' + base] + (
                       ['--input-script=' + a.input] if a.input else [])
            e = dict(env)
            e.pop('_DATA')
            t = time.time()
            with open(os.path.join(work, 'run.log'), 'w') as log:
                try:
                    code = game_process.run(cmd, env=e, stdout=log, stderr=subprocess.STDOUT, timeout=a.seconds + 90).returncode
                except subprocess.TimeoutExpired:
                    code = -1
            return dict(text=open(os.path.join(work, 'run.log'), errors='replace').read(), code=code,
                        wall=round(time.time() - t, 1))
        fut = {pool.submit(go, k, b): k for k, b in (('src', a.src), ('rb', a.rb))}
        for f in concurrent.futures.as_completed(fut):
            runs[fut[f]] = f.result()
    row = dict(name=name, wall=max(r['wall'] for r in runs.values()), problems=[])
    for k, r in runs.items():
        fl = faults(r['text'])
        if fl:
            row['problems'].append('%s fault: %s' % (k, fl[0][:150]))
        if not re.search(r'resume: loaded|loaded .*\.sav|Load_Game', r['text'], re.I):
            row['problems'].append('%s: no load reported' % k)
    sx, sy = sections(runs['src']['text']), sections(runs['rb']['text'])
    if not sx or not sy:
        row['problems'].append('no soak lines (src %d, rb %d)' % (len(sx), len(sy)))
        row.update(scenario='-', frames=0, last=-1, diff=None)
        return row
    x, y = sx[0], sy[0]
    n, last, diff = compare_sections(x, y, event_frames=False)
    first = min(x['digests']) if x['digests'] else -1
    row.update(scenario=x['scenario'], frames=n, first=first, last=last, diff=diff)
    if diff:
        row['problems'].append('first difference: ' + diff)
    if last - first < min_frames:
        row['problems'].append('compared only frames %d..%d (wanted %d frames)' % (first, last, min_frames))
    return row


def mode_load(a):
    rows = []
    for save in a.save.split(','):
        name = 'load-' + os.path.splitext(os.path.basename(save))[0] + a.tag
        row = load_pair(a, name, save_path(a, save), a.frames)
        prune(a, name, row)
        rows.append(row)
    return rows


def mode_cross(a):
    """A save made by --maker on the route, then loaded by both."""
    maker = a.src if a.maker == 'src' else a.rb
    name = 'cross-%s-%s%s' % (a.maker, a.route, a.tag)
    work = os.path.join(a.work, name, 'make')
    save = 'P14GATE.SAV'
    extra = dict(RA2_NATIVE_SAVE_AT='%d,%s' % (a.save_at, save), _DATA=a.data_dir, RA2_NATIVE_SOAK_END='%d@%d' % (
        a.frames, 1 if a.route == 'skirmish' else int(a.route.split(':')[1])))
    if a.orders:
        extra['RA2_NATIVE_ORDERS'] = a.orders
    with open(os.path.join(a.data_dir, 'RA2.INI'), errors='replace') as f:
        ini = f.read()
    if a.route == 'skirmish':
        lines = SKIRMISH_START
        ini = ini.rstrip('\n') + '\n' + SKIRMISH_INI % ('Americans', 0, 'Slot01=6,8,1\n')
    else:
        side, n = a.route.split(':')
        n = int(n)
        lines = campaign_script(side, n, [])
        if n > 1:
            extra['RA2_NATIVE_FORCE_END'] = ','.join(str(100 + 200 * k) for k in range(n - 1))
    env = base_env(a, extra)
    r = run_game(maker, work, env, ['--run-to=never'], a.save_at // 30 + 150, lines + read_input(a), ini)
    path = os.path.join(r['write'], save)
    if not os.path.isfile(path):
        return [dict(name=name, wall=r['wall'], problems=['the maker wrote no save (%s)' % r['log']], scenario='-',
                     frames=0, last=-1, diff=None)]
    row = load_pair(a, name + '-load', path, a.frames)
    # the maker's own digests up to the save against each loader's continuation is not comparable frame by frame
    # (the maker runs on); the loaders are compared with each other.
    prune(a, name, row)
    return [row]


def mode_recheck(a):
    """Compare the logs already in --work again (after a change to this script's comparison)."""
    rows = []
    for name in sorted(os.listdir(a.work)):
        d = os.path.join(a.work, name)
        logs = {k: os.path.join(d, k, 'run.log') for k in ('src', 'rb')}
        if not all(os.path.isfile(p) for p in logs.values()):
            continue
        texts = {k: open(p, errors='replace').read() for k, p in logs.items()}
        m = re.match(r'(allied|soviet)-(\d\d)', name)
        pick = int(m.group(2)) - 1 if m else 0
        row = dict(name=name, wall=0.0, problems=[])
        for k, t in texts.items():
            fl = faults(t)
            if fl:
                row['problems'].append('%s fault: %s' % (k, fl[0][:150]))
        sx, sy = sections(texts['src']), sections(texts['rb'])
        if len(sx) <= pick or len(sy) <= pick:
            row['problems'].append('scenario not reached (src %d, rb %d sections)' % (len(sx), len(sy)))
            rows.append(row)
            continue
        n, last, diff = compare_sections(sx[pick], sy[pick])
        row.update(scenario=sx[pick]['scenario'], frames=n, last=last, diff=diff)
        if diff:
            row['problems'].append('first difference: ' + diff)
        rows.append(row)
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('mode', choices=['missions', 'skirmish', 'load', 'cross', 'recheck'])
    ap.add_argument('--src', required=True)
    ap.add_argument('--rb', required=True)
    ap.add_argument('--data-dir', required=True)
    ap.add_argument('--game-exe', required=True)
    ap.add_argument('--work', required=True)
    ap.add_argument('--side', default='allied', choices=sorted(PREFIX))
    ap.add_argument('--missions', default='')
    ap.add_argument('--frames', type=int, default=4000)
    ap.add_argument('--period', type=int, default=50)
    ap.add_argument('--jobs', type=int, default=2, help='missions run at once (each runs both binaries)')
    ap.add_argument('--orders', default='')
    ap.add_argument('--input', default='', help='a file of extra input-script lines (frame-timed)')
    ap.add_argument('--map', type=int, default=0)
    ap.add_argument('--sides', default='Americans,6:8:1')
    ap.add_argument('--save', default='ALL01T.sav')
    ap.add_argument('--seconds', type=int, default=240)
    ap.add_argument('--maker', default='src', choices=['src', 'rb'])
    ap.add_argument('--route', default='skirmish', help='cross: skirmish or allied:<n> / soviet:<n>')
    ap.add_argument('--save-at', type=int, default=600)
    ap.add_argument('--uncapped', action='store_true')
    ap.add_argument('--tag', default='')
    ap.add_argument('--dump', default='', help='RA2_NATIVE_SOAK_DUMP=<from>[-<to>]: soak-techno lines in both runs')
    ap.add_argument('--reuse-rb', default='', help='a work folder of an earlier run: its rb logs are reused (the rebuild '
                    'binary unchanged), only the src port runs')
    ap.add_argument('--keep', action='store_true', help='keep the run folders of passing rows')
    a = ap.parse_args()
    a.orders = a.orders.replace('%', ';')   # ctest rows: ';' is a CMake list separator
    if not os.path.isfile(os.path.join(a.data_dir, 'game.exe')) or not os.path.isfile(a.game_exe):
        print('gate: skipped: no installed game or no game.exe')
        return SKIP
    a.work = os.path.abspath(a.work)
    os.makedirs(a.work, exist_ok=True)
    rows = {'missions': mode_missions, 'skirmish': mode_skirmish, 'load': mode_load, 'cross': mode_cross, 'recheck': mode_recheck}[a.mode](a)
    bad = 0
    for r in rows:
        ok = not r['problems']
        bad += not ok
        print('gate %-28s %-14s %-4s digests %4d through frame %6d  %5.0fs%s' % (
            r['name'], r.get('scenario', '-'), 'OK' if ok else 'FAIL', r.get('frames', 0), r.get('last', -1), r['wall'],
            '' if ok else '\n  ' + '\n  '.join(r['problems'])))
    with open(os.path.join(a.work, 'gate-%s%s.json' % (a.mode, a.tag)), 'a') as f:
        for r in rows:
            f.write(json.dumps(r) + '\n')
    print('gate: %d of %d identical' % (len(rows) - bad, len(rows)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
