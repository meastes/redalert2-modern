#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest ra2-native-walls (lane P30): walls bought and placed by a player's clicks, checked on the cells.

Lane P29 read a wall clicked onto a legal cell as lost (the factory's finished wall gone, no wall on the cell); lane P30
traced a live click (HouseClass::Place_Object, BuildingClass::Unlimbo's wall branch, OverlayClass::Mark, MapClass::
Place_Wall) and found the wall placed: the step server's `terrain` showed the cell's land type, which a wall overlay leaves
clear, as in the original (OverlayTypeClass reads no Land= for walls). This check keeps it so: Soviet 7 reached through
the real menus (ra2/tools/ra2_step.py start), the recorded clicks (ra2/tests/regress/walls-sov07.replay: the defense tab,
the wall cameo twice, a wall at 75,64 and one at 78,64) run as the live step server's preload, then the server is asked
for the cells.

Pass: 75,64 and 78,64 hold the NAWALL overlay, so do 76,64 and 77,64 between them (the original's line fill,
MapClass::Place_Wall), 74,64 and 79,64 do not, and `terrain` draws the four as walls. Skipped (77) without the installed
game. POSIX (the step server's socket).

  ra2_walls.py --binary <ra2-native> --data-dir <game folder> [--game-exe <game.exe>] --work <dir>
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
STEP = os.path.join(HERE, '..', 'tools', 'ra2_step.py')
REPLAY = os.path.join(HERE, 'regress', 'walls-sov07.replay')
SKIP = 77
WALLS = [75, 76, 77, 78]
CLEAR = [74, 79]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--game-exe', default='')
    ap.add_argument('--work', required=True)
    a = ap.parse_args()
    if not a.data_dir or not os.path.isfile(os.path.join(a.data_dir, 'game.exe')):
        print('walls: skipped: no installed game (game.exe) in --data-dir')
        return SKIP
    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    py = sys.executable
    start = [py, '-B', STEP, 'start', '--binary', a.binary, '--data-dir', a.data_dir, '--work', work, '--side', 'soviet',
             '--mission', '7', '--seconds', '600', '--preload', REPLAY]
    if a.game_exe:
        start += ['--game-exe', a.game_exe]
    bad = []
    try:
        r = subprocess.run(start, capture_output=True, text=True, timeout=400)
        print(r.stdout.strip())
        if r.returncode != 0:
            print('walls: FAIL: the step server did not start (%s)' % os.path.join(work, 'game.log'))
            return 1
        r = subprocess.run([py, '-B', STEP, 'send', '--work', work, 'advance 20', 'cell 76 64 3', 'terrain 73 64 80 64'],
                           capture_output=True, text=True, timeout=300)
        out = r.stdout
        print(out.strip())
        row = re.search(r'^row 64:(.*)$', out, re.M)
        if not row:
            bad.append('no cell row 64')
        else:
            cells = row.group(1).split()   # x 73..79
            for x in WALLS + CLEAR:
                has = '+NAWALL' in cells[x - 73]
                if has != (x in WALLS):
                    bad.append('cell %d,64 %s (%s)' % (x, 'holds a wall' if has else 'holds no wall', cells[x - 73]))
        t = re.search(r'^t +64 (.*)$', out, re.M)
        if not t or t.group(1)[2:6] != 'WWWW':
            bad.append('terrain does not draw the four walls: %r' % (t.group(1) if t else None))
    finally:
        subprocess.run([py, '-B', STEP, 'stop', '--work', work], capture_output=True, timeout=60)
    log = os.path.join(work, 'game.log')
    with open(log, errors='replace') as f:
        text = f.read()
    hit = re.search(r'unrecovered|\btrap\b|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:|assert', text, re.I)
    if hit:
        bad.append('fault or trap: %s' % text[hit.start():hit.start() + 160].splitlines()[0])
    for b in bad:
        print('walls: FAIL: %s' % b)
    print('walls: %s (%s)' % ('FAIL' if bad else 'OK', log))
    if not bad:
        shutil.rmtree(os.path.join(work, 'w'), ignore_errors=True)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
