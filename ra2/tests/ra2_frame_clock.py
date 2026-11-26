#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest ra2-native-frame-clock (lane P33): the step server's game clock runs one 16 ms tick a loop frame.

Lane P31 found two preloads of one recording (Allied 11, the reveal's Harrier strike) part: the reveal's radar movie
held the scenario from game frame 19,338 in one run and 19,259 in another, because what the game paces by its clock
(the EVA speech before the movie, the stream fade, the Bink movie's frames, a trigger's pause) ran on the wall clock while
the step server counts loop frames, uncapped: on a busier machine the same wait spans fewer loop frames, so every later
input landed at another game frame. Under the frame clock (RA2_NATIVE_FRAME_CLOCK=1, port/src/port_core.cpp) the
game's clock advances 16 ms a loop frame and stands still between frames.

Pass: Allied 1 reached through the real menus twice (ra2/tools/ra2_step.py start --clock frame), once with each loop
frame slowed by 5 ms (RA2_NATIVE_STEP_SLOW), as a slower machine; in both the game's clock (the `state` line's clock, in
16 ms ticks) advances exactly 600 over 600 ticks, and the game frames agree. Skipped (77) without the installed game.
POSIX (the step server's socket).

  ra2_frame_clock.py --binary <ra2-native> --data-dir <game folder> [--game-exe <game.exe>] --work <dir>
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
TICKS = 600
LINE = re.compile(r'^scenario \S+ loop (\d+) game (\d+) .* clock (\d+)$', re.M)


def run(a, work, slow):
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    start = [sys.executable, '-B', STEP, 'start', '--binary', a.binary, '--data-dir', a.data_dir, '--work', work,
             '--side', 'allied', '--mission', '1', '--seconds', '600', '--clock', 'frame']
    if a.game_exe:
        start += ['--game-exe', a.game_exe]
    env = dict(os.environ, RA2_NATIVE_STEP_SLOW=str(slow))
    r = subprocess.run(start, capture_output=True, text=True, timeout=400, env=env)
    if r.returncode != 0:
        return None, 'the step server did not start (%s)' % os.path.join(work, 'game.log')
    try:
        replies = ra2_step.send(work, ['state', 'advance %d' % TICKS, 'state'], timeout=300)
    finally:
        subprocess.run([sys.executable, '-B', STEP, 'stop', '--work', work], capture_output=True, timeout=60)
    found = [LINE.search('\n'.join(replies[k])) for k in (0, 2)]
    if not all(found):
        return None, 'no state line with the clock'
    (t0, g0, c0), (t1, g1, c1) = [tuple(int(x) for x in m.groups()) for m in found]
    return (t1 - t0, g1 - g0, c1 - c0), ''


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--game-exe', default='')
    ap.add_argument('--work', required=True)
    a = ap.parse_args()
    if not a.data_dir or not os.path.isfile(os.path.join(a.data_dir, 'game.exe')):
        print('frame-clock: skipped: no installed game (game.exe) in --data-dir')
        return SKIP
    bad = []
    seen = []
    for slow in (0, 5):
        got, why = run(a, os.path.join(os.path.abspath(a.work), 'slow%d' % slow), slow)
        if got is None:
            bad.append('slow %d ms: %s' % (slow, why))
            continue
        ticks, frames, clock = got
        print('frame-clock: slow %d ms: %d ticks, %d game frames, clock +%d' % (slow, ticks, frames, clock))
        if ticks != TICKS or clock != TICKS:
            bad.append('slow %d ms: the clock advanced %d over %d ticks (want %d)' % (slow, clock, ticks, TICKS))
        seen.append(got)
    if len(seen) == 2 and seen[0] != seen[1]:
        bad.append('the slowed run went another way: %s against %s' % (seen[1], seen[0]))
    for b in bad:
        print('frame-clock: FAIL: %s' % b)
    print('frame-clock: %s' % ('FAIL' if bad else 'OK'))
    if not bad:
        shutil.rmtree(a.work, ignore_errors=True)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
