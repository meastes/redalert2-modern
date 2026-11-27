#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest yr-native-planning-click (lane P56; built with -DRA2_NATIVE_YR=ON): the player's units answer orders after the
step server is asked to click a map cell below the tactical view, and planning mode holds and then releases orders as
gamemd.exe does.

Found by campaign lane P52 in Allied 5: from tick 94,642 every prism tank kept guarding whatever it was ordered (the
order lines drawn, nothing done), the new ones at the war factories too; a save loaded the units free again. Planning
mode was on: a move click at @=163,140 (a cell near the map's south edge, which the view cannot be centred on)
resolved to 316,568, the command bar's Planning Mode button, because Tactical::Coord_To_Pixel answers true up to 180
pixels below the view. In planning mode a move click is a planned waypoint (its line drawn), carried out only when
planning mode ends (PLAN_COMMIT), so the game did as gamemd.exe does; planning mode is not saved, so a load ended it.
The step server's map points (yr_step_resolve_point "@=X,Y" and "@ID+dx,dy") are now clicked only inside the view.

Allied 1 through the menus (ra2/tools/ra2_step.py start --game yr --mission 1), advanced to tick 2,400; one of the
player's tanks (MTNK) at X,Y, the view centred on it (look):
  1. a click at @=X+10,Y+10 (300 pixels below the view's middle, on the command bar): refused (no click), planning mode
     stays off;
  2. the tank selected, the bar's Planning Mode button pressed (@cmd=PlanningMode): planning mode on; a move click
     (@=X+3,Y-3): the tank stays where it is (the stuck pattern, a planned waypoint);
  3. the button pressed again: planning mode off and the tank drives to the planned cell;
  4. a plain move click (@=X-3,Y+3): the tank drives there.
Skipped (77) without the installed game (gamemd.exe in --data-dir) and for the Windows build. POSIX (the step server's
socket).

  yr_planning_click.py --binary <yr-native> --data-dir <game folder> --work <dir>
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
STEP = os.path.join(HERE, '..', '..', 'ra2', 'tools', 'ra2_step.py')
sys.path.insert(0, os.path.join(HERE, '..', '..', 'ra2', 'tools'))
import ra2_step  # noqa: E402

SKIP = 77
OBJ = re.compile(r'^obj (\d+) (\S+) (\S+) \S+ @(-?\d+),(-?\d+) s\d+/\d+ m(-?\d+)(.*)$')
PLANS = re.compile(r'^harness-plans mode (\d) ', re.M)
REFUSED = re.compile(r'^step: @(\S+) is at (-?\d+),(-?\d+), outside the tactical view \((\d+)x(\d+)\): not clicked$', re.M)


def tanks(work):
    out = {}
    for line in ra2_step.send(work, ['objects mine'])[0]:
        m = OBJ.match(line)
        if m and m.group(2) == 'veh' and m.group(3) == 'MTNK':
            out[int(m.group(1))] = ((int(m.group(4)), int(m.group(5))), int(m.group(6)))
    return out


def click(spec):
    return ['input move %s' % spec, 'input +2 ldown %s' % spec, 'input +6 lup %s' % spec]


def log(work):
    return open(os.path.join(work, 'game.log'), errors='replace').read()


def planning(work):
    ra2_step.send(work, ['order plans'])
    found = PLANS.findall(log(work))
    return int(found[-1]) if found else -1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--work', required=True)
    a = ap.parse_args()
    if not a.data_dir or not os.path.exists(os.path.join(a.data_dir, 'gamemd.exe')):
        print('skipped: no Yuri\'s Revenge install (gamemd.exe) in --data-dir')
        return SKIP
    if a.binary.lower().endswith('.exe') or os.name != 'posix':
        print('skipped: run on the Mac build (the step server\'s socket; the Windows build has the same game code)')
        return SKIP
    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    start = [sys.executable, '-B', STEP, 'start', '--game', 'yr', '--binary', a.binary, '--data-dir', a.data_dir,
             '--work', work, '--side', 'allied', '--mission', '1', '--seconds', '900', '--clock', 'frame']
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
    print('%s: units answer orders after a click below the view; planning mode holds and releases orders (%s)'
          % ('FAIL' if failures else 'OK', work))
    return 1 if failures else 0


def check(work):
    failures = []
    ra2_step.send(work, ['advance 2400'])
    mine = tanks(work)
    if not mine:
        return ['no tank (MTNK) of the player\'s at tick 2,400']
    # the tank nearest the middle of the player's tanks (not one at the map's edge)
    cells = [c for c, _ in mine.values()]
    mx, my = sum(c[0] for c in cells) / len(cells), sum(c[1] for c in cells) / len(cells)
    tank = min(mine, key=lambda k: abs(mine[k][0][0] - mx) + abs(mine[k][0][1] - my))
    (x, y), _ = mine[tank]
    print('tank %d at %d,%d' % (tank, x, y))

    # 1. a cell below the view: no click, planning mode stays off
    ra2_step.send(work, ['look %d %d' % (x, y)] + click('@=%d,%d' % (x + 10, y + 10)) + ['advance 30'])
    refused = REFUSED.findall(log(work))
    mode = planning(work)
    print('1. below the view: refused %s, planning mode %d' % (refused[-1:] or 'none', mode))
    if not refused:
        failures.append('a click at @=%d,%d (below the view) was not refused' % (x + 10, y + 10))
    if mode != 0:
        failures.append('planning mode is on after the click below the view')

    # 2. planning mode by its button: a move click is a planned waypoint, the tank stays
    ra2_step.send(work, ['look %d %d' % (x, y)] + click('@^%d' % tank) + ['advance 20'] + click('@cmd=PlanningMode') +
                  ['advance 20'])
    mode = planning(work)
    ra2_step.send(work, click('@=%d,%d' % (x + 3, y - 3)) + ['advance 150'])
    held = tanks(work).get(tank)
    print('2. planning mode %d, the tank after a move click: %s' % (mode, held))
    if mode != 1:
        failures.append('the Planning Mode button did not start planning mode')
    elif held is None or held[0] != (x, y):
        failures.append('in planning mode the tank moved at once: %s -> %s' % ((x, y), held))

    # 3. planning mode ended: the planned move is carried out
    ra2_step.send(work, click('@cmd=PlanningMode') + ['advance 300'])
    mode = planning(work)
    done = tanks(work).get(tank)
    print('3. planning mode %d, the tank: %s' % (mode, done))
    if mode != 0:
        failures.append('the Planning Mode button did not end planning mode')
    if done is None or done[0] == (x, y):
        failures.append('the planned move was not carried out when planning mode ended: %s' % (done,))

    # 4. a plain move order
    here = done[0] if done else (x, y)
    ra2_step.send(work, ['look %d %d' % here] + click('@^%d' % tank) + ['advance 20'] +
                  click('@=%d,%d' % (here[0] - 3, here[1] + 3)) + ['advance 300'])
    moved = tanks(work).get(tank)
    print('4. the tank after a move click: %s' % (moved,))
    if moved is None or moved[0] == here:
        failures.append('the tank did not answer a move order: still at %s' % (here,))
    bad = sorted(set(l for l in log(work).splitlines() if re.search(r'stopped: (signal|trap|fault)|unrecovered .* reached', l)))
    if bad:
        failures.append('traps or faults: %s' % '; '.join(bad[:5]))
    return failures


if __name__ == '__main__':
    sys.exit(main())
