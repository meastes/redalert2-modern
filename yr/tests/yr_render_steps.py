#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest yr-native-render-steps (lane P74; built with -DRA2_NATIVE_YR=ON): two steps of Tactical::Render's foreground pass
(glue/tactical_native.inc, gamemd.exe 0x006D3D10) that the draft left out and lane P74 put back:

* the airstrike designator's laser (0x006D47FB-0x006D48F1): a techno whose airstrike paints a building draws a laser
  from its fire coordinate onto the building (TechnoClass::FUN_00705860);
* the Psychic Sensor's intent lines (0x006D4764): for a techno not the player's whose target or destination a powered
  Psychic Sensor of the player's covers (FUN_0043B150), FootClass::FUN_004DC340 draws its line.

Through the menus into a skirmish (yr_standins.py's route), harness orders (glue/yr_harness.cpp):
  frame 60    the player gets a Tesla reactor (NAPOWR) and a Psychic Sensor (NAPSIS); an enemy power plant (GAPOWR)
              and two enemy Rhinos (HTNK) are placed near them, and Boris (BORIS) for the player;
  frame 70    Boris is ordered onto the enemy power plant (`fire BORIS GAPOWR`), which calls his airstrike in;
  frame 90..  the airstrike's target (`airstrike BORIS`) and the foreground pass's counts (`rendercounts`).
Before lane P74 both counts stayed 0 (the draft drew neither).

  yr_render_steps.py --binary <yr-native> --data-dir <game folder> --work <dir>
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
ORDERS = ('60 spawn NAPOWR 6 0; 61 spawn NAPSIS 0 6; 62 spawn GAPOWR enemy near NAPSIS 4 4; '
          '63 spawn HTNK enemy near NAPSIS 3 0; 64 spawn HTNK enemy near NAPSIS 0 3; 65 spawn BORIS near NAPSIS -2 0; '
          '70 fire BORIS GAPOWR; 90 airstrike BORIS; 91 rendercounts; 150 airstrike BORIS; 151 rendercounts; '
          '300 rendercounts')
BAD = re.compile(r'unrecovered|\btrap\b|signal \d+|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:', re.I)


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
    script = os.path.join(a.work, 'render.script')
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

    for b in ('NAPOWR', 'NAPSIS', 'GAPOWR', 'HTNK', 'BORIS'):
        if not re.search(r'^harness-spawn %s h\d+ ' % b, text, re.M):
            failures.append('%s not spawned' % b)
    targets = re.findall(r'^harness-airstrike BORIS target (\S+) rtti (-?\d+)$', text, re.M)
    print('airstrike targets: %s' % targets)
    if not any(t == 'GAPOWR' for t, _ in targets):
        failures.append('Boris\'s airstrike never painted the enemy power plant (%s)' % targets)
    counts = [(int(x), int(y)) for x, y in re.findall(r'^harness-rendercounts laser (\d+) intent (\d+)$', text, re.M)]
    print('render counts (laser, intent): %s' % counts)
    if not counts:
        failures.append('no render counts logged')
    else:
        laser, intent = counts[-1]
        if laser == 0:
            failures.append('the airstrike designator\'s laser was never drawn (gamemd.exe 0x006D47FB)')
        if intent == 0:
            failures.append('no Psychic Sensor intent line was drawn (gamemd.exe 0x006D4764)')

    bad = [line for line in text.splitlines() if BAD.search(line)]
    if bad:
        failures.append('trap/crash: %s' % bad[:3])
    for f in failures:
        print('FAIL: ' + f)
    print('yr-native-render-steps: %s' % ('FAILED' if failures else 'ok'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
