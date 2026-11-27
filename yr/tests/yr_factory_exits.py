#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest yr-native-factory-exits (lane P61; built with -DRA2_NATIVE_YR=ON): YR's factory exits as gamemd.exe's
BuildingClass::Exit_Object (0x00443C60) makes them: the Cloning Vats copy a soldier from a barracks, and an Airforce
Command puts each new aircraft on a free pad.

* Cloning (0x004449D9-0x00444A51): a soldier leaving a barracks is copied by each of the house's Cloning Vats (a new one
  of its type, put out of the vat by its own Exit_Object). The draft (glue/building_native.inc, TibSun's text) had no
  cloning.
* Pads (0x00443CB4-0x00444091): an aircraft goes onto a pad while the building has a free radio link (HasFreeLink: the
  Airforce Command's four pads take one each); the draft took the first link only (In_Radio_Contact), so a second
  Harrier made while the first held its pad was put at the map edge and flew in (seen at 9,65 with the Command at 53,98).

Through the menus into a skirmish (yr_standins.py's route), harness orders (glue/yr_harness.cpp):
  frame 60    the player gets Yuri's Cloning Vats (NACLON), a barracks (GAPILE) and an Airforce Command (GAAIRC);
  frame 70    the player's Initiates (INIT) counted; 75 one leaves the barracks (`produce GAPILE INIT`);
  frame 200   two more Initiates (the barracks' and the vat's clone);
  frame 210   a Harrier (ORCA) made at the Airforce Command, 211 where it is (`produced`); 220, 221 a second one.
Both Harriers start on the Command's pads (within a few cells of it), not at the map edge.

  yr_factory_exits.py --binary <yr-native> --data-dir <game folder> --work <dir>
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
ORDERS = ('60 spawn NACLON 6 0; 61 spawn GAPILE 0 6; 62 spawn GAAIRC -6 0; 70 owner INIT; 75 produce GAPILE INIT; '
          '200 owner INIT; 210 produce GAAIRC ORCA; 211 produced; 220 produce GAAIRC ORCA; 221 produced')
BAD = re.compile(r'unrecovered|\btrap\b|signal \d+|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:', re.I)


def block(text, frame, order):
    key = 'harness-order %d %s' % (frame, order)
    return text.split(key)[0].rsplit('harness-order', 1)[-1] if key in text else ''


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--seconds', type=int, default=60)
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
    script = os.path.join(a.work, 'exits.script')
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

    def inits(frame):
        return len(re.findall(r'^harness-owner INIT h\d+ p ', block(text, frame, 'owner INIT'), re.M))

    for b in ('NACLON', 'GAPILE', 'GAAIRC'):
        if not re.search(r'^harness-spawn %s h\d+ ' % b, text, re.M):
            failures.append('%s not spawned' % b)
    before, after = inits(70), inits(200)
    print('cloning: the player\'s Initiates %d -> %d' % (before, after))
    if not re.search(r'^harness-produce GAPILE INIT exit 2 ', text, re.M):
        failures.append('the Initiate did not leave the barracks')
    if after != before + 2:
        failures.append('the Cloning Vats did not copy the Initiate (%d -> %d, want +2)' % (before, after))

    for frame in (211, 221):
        m = re.search(r'^harness-produced ORCA cell (-?\d+),(-?\d+) factory (-?\d+),(-?\d+) ',
                      block(text, frame, 'produced'), re.M)
        if not m:
            failures.append('frame %d: no Harrier made' % frame)
            continue
        x, y, fx, fy = (int(v) for v in m.groups())
        print('pads: frame %d Harrier at %d,%d, the Command at %d,%d' % (frame, x, y, fx, fy))
        if abs(x - fx) > 3 or abs(y - fy) > 3:
            failures.append('frame %d: the Harrier is at %d,%d, not on a pad of the Command at %d,%d' % (frame, x, y, fx, fy))

    bad = [line for line in text.splitlines() if BAD.search(line)]
    if bad:
        failures.append('trap/crash: %s' % bad[:3])
    for f in failures:
        print('FAIL: ' + f)
    print('yr-native-factory-exits: %s' % ('FAILED' if failures else 'ok'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
