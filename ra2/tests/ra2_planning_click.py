#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest ra2-native-planning-click (lane P65): the step server clicks map points only inside the tactical view, and
planning mode holds and then releases orders as game.exe does.

Yuri's Revenge's lane P56 found it there (yr/tests/yr_planning_click.py): a move click at a cell near the map's south
edge, which the view cannot be centred on, resolved through Tactical::Coord_To_Pixel, which answers true up to 180
pixels below the view (and 360 beside it), to a pixel on the command bar's Planning Mode button; from then on every
order was a planned waypoint. Red Alert 2 1.006 has the same command bar and planning mode, and its resolver
(ra2/glue/harness_input_native.inc RB_Resolve, "@=X,Y" and "@ID+dx,dy") had the same gap: a point below the view was
on the command bar, one beside it on the sidebar. Such points are now refused (no click; the log says "outside the
tactical view").

Allied 1 through the menus (ra2/tools/ra2_step.py start --mission 1), advanced to tick 900 and on until one of the
player's units (Tanya) is at rest at X,Y, the view centred on it (look):
  1. a click at @=X+k,Y+k (30k pixels below the view's middle, k the least that passes the view's bottom: on the command
     bar) and one at @=X+j,Y-j (60j pixels to its right, past the view's right edge: on the sidebar): both refused,
     planning mode stays off (the step server's `state` line, which also gives the view's centre cell and size; the
     offsets are taken from that centre, which a map edge can hold off the unit);
  2. the unit selected, the bar's Planning Mode button pressed (@cmd=PlanningMode): planning mode on; a move click
     (@=X+3,Y-3): the unit stays where it is (a planned waypoint);
  3. the button pressed again: planning mode off and the unit goes to the planned cell;
  4. a plain move click (@=X-3,Y+3): the unit goes there.
Skipped (77) without the installed game. POSIX (the step server's socket).

  ra2_planning_click.py --binary <ra2-native> --data-dir <game folder> [--game-exe <game.exe>] --work <dir>
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
STEP = os.path.join(HERE, '..', 'tools', 'ra2_step.py')
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
import ra2_step  # noqa: E402

SKIP = 77
OBJ = re.compile(r'^obj (\d+) (\S+) (\S+) .*? @(-?\d+),(-?\d+) s\d+/\d+ m(-?\d+)(.*)$')
PLANNING = re.compile(r'^planning (\d)$', re.M)
VIEW = re.compile(r'^view \S+ centre @(-?\d+),(-?\d+) tactical (\d+)x(\d+) ', re.M)
REFUSED = re.compile(r'^step: @(\S+) is at (-?\d+),(-?\d+), outside the tactical view \((\d+)x(\d+)\): not clicked$', re.M)


def units(work):
    """The player's mobile units: {id: ((x, y), kind, type, mission)}."""
    out = {}
    for line in ra2_step.send(work, ['objects mine'])[0]:
        m = OBJ.match(line)
        if m and m.group(2) in ('veh', 'inf'):
            out[int(m.group(1))] = ((int(m.group(4)), int(m.group(5))), m.group(2), m.group(3), int(m.group(6)))
    return out


def click(spec):
    return ['input move %s' % spec, 'input +2 ldown %s' % spec, 'input +6 lup %s' % spec]


def log(work):
    return open(os.path.join(work, 'game.log'), errors='replace').read()


def planning(work):
    found = PLANNING.findall('\n'.join(ra2_step.send(work, ['state'])[0]))
    return int(found[-1]) if found else -1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--game-exe', default='')
    ap.add_argument('--work', required=True)
    a = ap.parse_args()
    if not a.data_dir or not os.path.isfile(os.path.join(a.data_dir, 'game.exe')):
        print('skipped: no installed game (game.exe) in --data-dir')
        return SKIP
    if a.binary.lower().endswith('.exe') or os.name != 'posix':
        print('skipped: run on the Mac build (the step server\'s socket; the Windows build has the same game code)')
        return SKIP
    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    start = [sys.executable, '-B', STEP, 'start', '--binary', a.binary, '--data-dir', a.data_dir, '--work', work,
             '--side', 'allied', '--mission', '1', '--seconds', '3600', '--clock', 'frame']
    if a.game_exe:
        start += ['--game-exe', a.game_exe]
    r = subprocess.run(start, capture_output=True, text=True, timeout=400)
    if r.returncode != 0:
        print('FAIL: the step server did not start (%s)' % os.path.join(work, 'game.log'))
        return 1
    failures = []
    try:
        failures = check(work)
    finally:
        subprocess.run([sys.executable, '-B', STEP, 'stop', '--work', work], capture_output=True, timeout=60)
    for f in failures:
        print('FAIL: ' + f)
    print('%s: clicks below and beside the view refused; planning mode holds and releases orders (%s)'
          % ('FAIL' if failures else 'OK', work))
    return 1 if failures else 0


def check(work):
    failures = []
    ra2_step.send(work, ['advance 900'])
    mine = units(work)
    if not mine:
        return ['no unit of the player\'s at tick 900']
    # a vehicle if there is one, else infantry; the one nearest the middle of them (not one at the map's edge)
    pool = {k: v for k, v in mine.items() if v[1] == 'veh'} or mine
    cells = [v[0] for v in pool.values()]
    mx, my = sum(c[0] for c in cells) / len(cells), sum(c[1] for c in cells) / len(cells)
    unit = min(pool, key=lambda k: abs(pool[k][0][0] - mx) + abs(pool[k][0][1] - my))
    # at rest (guarding, mission 5): the mission's opening moves are over
    for _ in range(30):
        if mine.get(unit) and mine[unit][3] == 5:
            break
        ra2_step.send(work, ['advance 100'])
        mine = units(work)
    if not mine.get(unit):
        return ['the player\'s unit %d is gone' % unit]
    (x, y), kind, name, mission = mine[unit]
    print('unit %d (%s %s) at %d,%d, mission %d' % (unit, kind, name, x, y, mission))

    # 1. cells below and beside the view: no click, planning mode stays off. From the view's centre cell a cell +1,+1 is
    # 30 pixels lower on the screen and +1,-1 60 pixels further right.
    ra2_step.send(work, ['look %d %d' % (x, y), 'advance 5'])
    view = VIEW.search('\n'.join(ra2_step.send(work, ['state'])[0]))
    if not view:
        return ['no view line in the step server\'s state']
    cx, cy, width, height = (int(g) for g in view.groups())
    kb = (height // 2 + 90) // 30 + 1   # 90 pixels past the edge: a raised cell is drawn higher
    kr = (width // 2 + 30) // 60 + 1
    below, beside = '=%d,%d' % (cx + kb, cy + kb), '=%d,%d' % (cx + kr, cy - kr)
    print('view %dx%d centred on %d,%d: below @%s, beside @%s' % (width, height, cx, cy, below, beside))
    before = len(REFUSED.findall(log(work)))
    ra2_step.send(work, click('@' + below) + ['advance 30'] + click('@' + beside) + ['advance 30'])
    refused = REFUSED.findall(log(work))[before:]
    mode = planning(work)
    print('1. below and beside the view: refused %s, planning mode %d' % (refused or 'none', mode))
    specs = set(r[0] for r in refused)
    for spec, where in ((below, 'below'), (beside, 'beside')):
        if spec not in specs:
            failures.append('a click at @%s (%s the view) was not refused' % (spec, where))
    if mode != 0:
        failures.append('planning mode is %s after the clicks outside the view' % ('on' if mode == 1 else 'not reported'))

    # 2. planning mode by its button: a move click is a planned waypoint, the unit stays
    ra2_step.send(work, ['look %d %d' % (x, y)] + click('@^%d' % unit) + ['advance 20'] + click('@cmd=PlanningMode') +
                  ['advance 20'])
    mode = planning(work)
    ra2_step.send(work, click('@=%d,%d' % (x + 3, y - 3)) + ['advance 150'])
    held = units(work).get(unit)
    print('2. planning mode %d, the unit after a move click: %s' % (mode, held))
    if mode != 1:
        failures.append('the Planning Mode button did not start planning mode')
    elif held is None or held[0] != (x, y):
        failures.append('in planning mode the unit moved at once: %s -> %s' % ((x, y), held))

    # 3. planning mode ended: the planned move is carried out
    ra2_step.send(work, click('@cmd=PlanningMode') + ['advance 300'])
    mode = planning(work)
    done = units(work).get(unit)
    print('3. planning mode %d, the unit: %s' % (mode, done))
    if mode != 0:
        failures.append('the Planning Mode button did not end planning mode')
    if done is None or done[0] == (x, y):
        failures.append('the planned move was not carried out when planning mode ended: %s' % (done,))

    # 4. a plain move order
    here = done[0] if done else (x, y)
    ra2_step.send(work, ['look %d %d' % here] + click('@^%d' % unit) + ['advance 20'] +
                  click('@=%d,%d' % (here[0] - 3, here[1] + 3)) + ['advance 300'])
    moved = units(work).get(unit)
    print('4. the unit after a move click: %s' % (moved,))
    if moved is None or moved[0] == here:
        failures.append('the unit did not answer a move order: still at %s' % (here,))
    bad = sorted(set(l for l in log(work).splitlines() if re.search(r'stopped: (signal|trap|fault)|unrecovered .* reached', l)))
    if bad:
        failures.append('traps or faults: %s' % '; '.join(bad[:5]))
    return failures


if __name__ == '__main__':
    sys.exit(main())
