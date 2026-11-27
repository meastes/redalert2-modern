#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest yr-native-tank-bunker (lane P55; built with -DRA2_NATIVE_YR=ON): a tank in a tank bunker and the bunker
let go of each other when either is destroyed or the bunker is sold.

The cause of YR Allied 5's crash (lane P52, tick ~95,600; ctest yr-native-campaign-crash-all05): Yuri's AI sold two
tank bunkers (NATBNK) with lasher tanks inside. BuildingClass::Do_MISSION_DECONSTRUCTION (glue/building_native.inc, a
NON-EXACT draft from TibSun's text, which has no bunkers) did not let the tank out, so the tank's BunkerLinkedItem kept
pointing at the deleted bunker, and when the tank died UnitClass::Take_Damage called ClearBunker through it. gamemd.exe
calls UnloadBunker there (0x0044AAA4); the draft now does too.

Through the menus into a skirmish (yr_standins.py's route), harness orders (glue/yr_harness.cpp):
  frame 60   a tank bunker (NATBNK) spawned for the player; 81 the player's first Grizzly (MTNK) clicked onto it;
  frame 700  the tank and the bunker name each other (BunkerLinkedItem); 710 the tank is destroyed inside (ClearBunker
             from UnitClass::Take_Damage): 720 the bunker names nothing;
  frame 730  an IFV (FV) spawned and clicked onto the bunker; 1350 linked; 1360 the bunker is destroyed (UnloadBunker
             from BuildingClass::Take_Damage): 1370 the IFV names nothing and is alive;
  frame 1380 a new bunker spawned, the IFV clicked onto it; 2000 linked; 2010 the bunker sold; 2400 the IFV names
             nothing (before the fix: the sold bunker, deleted); 2410 the IFV is destroyed: no fault.

  yr_tank_bunker.py --binary <yr-native> --data-dir <game folder> --work <dir>
Skipped (77) without the installed game (gamemd.exe in --data-dir) and for the Windows build.
"""
import argparse
import importlib
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tests'))
import game_process  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
standins = importlib.import_module('yr_standins')
ORDERS = ('60 spawn NATBNK 6 0; 81 click MTNK NATBNK; '
          '700 bunker MTNK; 701 bunker NATBNK; 710 kill MTNK; 720 bunker NATBNK; '
          '730 spawn FV 3 3; 750 click FV NATBNK; 1350 bunker FV; 1351 bunker NATBNK; 1360 kill NATBNK; 1370 bunker FV; '
          '1380 spawn NATBNK 6 0; 1400 click FV NATBNK; 2000 bunker FV; 2010 sell NATBNK; 2400 bunker FV; 2410 kill FV; '
          '2500 bunker FV')
BUNKER = re.compile(r'^harness-order (\d+) bunker (\S+): .*$|^harness-bunker (\S+) (\S+)', re.M)
BAD = re.compile(r'unrecovered|\btrap\b|signal \d+|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:', re.I)
# (frame, techno type, what its BunkerLinkedItem must name)
WANT = [(700, 'MTNK', 'NATBNK'), (701, 'NATBNK', 'MTNK'), (720, 'NATBNK', 'none'),
        (1350, 'FV', 'NATBNK'), (1351, 'NATBNK', 'FV'), (1370, 'FV', 'none'),
        (2000, 'FV', 'NATBNK'), (2400, 'FV', 'none'), (2500, 'FV', 'gone')]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--seconds', type=int, default=120)
    a = ap.parse_args()
    if not a.data_dir or not os.path.exists(os.path.join(a.data_dir, 'gamemd.exe')):
        print('skipped: no Yuri\'s Revenge install (gamemd.exe) in --data-dir')
        return 77
    if a.binary.lower().endswith('.exe'):
        print('skipped: run on the Mac build (the Windows build has the same game code)')
        return 77
    shutil.rmtree(a.work, ignore_errors=True)
    os.makedirs(os.path.join(a.work, 'w'))
    with open(os.path.join(a.work, 'w', 'RA2MD.INI'), 'w') as f:
        f.write(standins.SETTINGS)
    script = os.path.join(a.work, 'bunker.script')
    with open(script, 'w') as f:
        f.write(standins.SCRIPT)
    env = dict(os.environ, RA2_NATIVE_MOVIE_SKIP='1', RA2_NATIVE_IGNORE_FOCUS='1', RA2_NATIVE_MSG_LOG='0',
               RA2_NATIVE_ORDERS=ORDERS)
    env.pop('RA2_NATIVE_FORCE_END', None)
    env.pop('RA2_NATIVE_TRAP_CONTINUE', None)
    cmd = [a.binary, '--data-dir', a.data_dir, '--run-to=never', '--run-seconds=%d' % a.seconds,
           '--write-dir=' + os.path.join(a.work, 'w'), '--input-script=' + script, '--game-args=-noaudio -CD.']
    with open(os.path.join(a.work, 'run.log'), 'w') as log:
        game_process.run(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=a.seconds + 120)
    text = open(os.path.join(a.work, 'run.log'), errors='replace').read()
    failures = []
    # the bunker reports, by the frame of the order that wrote them (the harness logs `harness-order <frame> ...`
    # after each order's own lines)
    seen = {}
    pending = []
    for line in text.splitlines():
        m = re.match(r'^harness-bunker (\S+) (\S+) ', line) or re.match(r'^harness-bunker (\S+) (gone)$', line)
        if m:
            pending.append((m.group(1), m.group(2)))
            continue
        d = re.match(r'^harness-order (\d+) bunker ', line)
        if d:
            seen[int(d.group(1))] = pending
            pending = []
    for frame, kind, want in WANT:
        got = [link for (k, link) in seen.get(frame, []) if k == kind]
        if want not in ('none', 'gone'):
            got = [link for link in got if link != 'none']   # the player's other tanks of the type (the start's)
        print('frame %d: %s names %s (want %s)' % (frame, kind, got, want))
        if got != [want]:
            failures.append('frame %d: %s\'s BunkerLinkedItem is %s, not %s' % (frame, kind, got or 'not reported', want))
    for unit in ('MTNK', 'FV'):
        if not re.search(r'^harness-click %s action \d+ -> NATBNK ok$' % unit, text, re.M):
            failures.append('the %s was not sent into the bunker (the click failed)' % unit)
    if not re.search(r'^harness-sell NATBNK ', text, re.M):
        failures.append('the bunker was not sold')
    if len(re.findall(r'^harness-kill \S+ result ', text, re.M)) < 3:
        failures.append('not every kill order ran')
    bad = sorted(set(l for l in text.splitlines() if BAD.search(l) and not l.startswith('input-script')))
    if bad:
        failures.append('traps or faults: %s' % '; '.join(bad[:5]))
    for f in failures:
        print('FAIL: ' + f)
    print('%s: a tank bunker and its tank let go of each other (%s)' % ('FAIL' if failures else 'OK', a.work))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
