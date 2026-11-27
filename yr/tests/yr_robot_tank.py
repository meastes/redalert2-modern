#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest yr-native-robot-tank (lane P53; built with -DRA2_NATIVE_YR=ON): a Robot Tank (ROBO) built while its house has
a powered Robot Control Center (GAROBO, PowersUnit=ROBO) is active: it leaves the War Factory and drives on.

Found by campaign lane P52 in Allied 4: the robot tanks stayed deactivated inside the war factory and blocked every
later vehicle. HouseClass::PoweredUnitCenters counts the house's powered PowersUnit= buildings
(BuildingClass::FUN_004549B0 registers one through HouseClass::FUN_0050E010 when its power state changes); a
PoweredUnit= vehicle whose house has none deactivates itself (UnitClass::Per_Cell_Process, PCP_END).

Through the menus into a skirmish (yr_standins.py's route), harness orders (glue/yr_harness.cpp):
  frame 60   a Power Plant (GAPOWR) spawned, 90 a Robot Control Center, 120 a War Factory;
  frame 200  a Robot Tank produced in the factory; `robots` logs the house's powered centres, the tank's deactivated
             flag and each centre's registration; `produced` the tank's cell and the factory's mission;
  frame 420  the tank is outside the factory's foundation, active, and the factory has finished unloading;
  frame 430  the tank is ordered 6 cells on (`move`); at 560 it has moved from where it stood.

  yr_robot_tank.py --binary <yr-native> --data-dir <game folder> --work <dir>
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
ORDERS = ('60 spawn GAPOWR; 90 spawn GAROBO; 120 spawn GAWEAP; 180 robots; 200 produce GAWEAP ROBO; 201 produced; '
          '201 robots; 260 produced; 320 produced; 420 produced; 420 robots; 430 move ROBO 6 0; 560 produced; 560 robots')
PRODUCED = re.compile(r'^harness-produced (\S+) cell (-?\d+),(-?\d+) factory (-?\d+),(-?\d+) limbo (\d) mission (-?\d+) '
                      r'factory-mission (-?\d+)$', re.M)
ROBOTS = re.compile(r'^harness-robots centres (-?\d+) deactivated (-?\d+) online(.*)$', re.M)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--seconds', type=int, default=190)
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
    script = os.path.join(a.work, 'robot.script')
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
    tank = [((int(m.group(2)), int(m.group(3))), int(m.group(6)), int(m.group(8)))
            for m in PRODUCED.finditer(text) if m.group(1) == 'ROBO']
    robots = [(int(m.group(1)), int(m.group(2)), m.group(3).strip()) for m in ROBOTS.finditer(text)]
    print('tank (cell, limbo, factory mission): %s' % tank)
    print('robots (centres, deactivated, centres registered): %s' % robots)
    if len(robots) < 4:
        failures.append('only %d of 4 robot reports (no Robot Control Center, or the game did not reach frame 560)'
                        % len(robots))
    else:
        if robots[0][0] < 1 or 'GAROBO:1' not in robots[0][2]:
            failures.append('the powered Robot Control Center is not counted (PoweredUnitCenters %d, %s)'
                            % (robots[0][0], robots[0][2] or 'no GAROBO'))
        if any(r[1] != 0 for r in robots[1:]):
            failures.append('the robot tank is deactivated: %s' % [r[1] for r in robots[1:]])
    if len(tank) < 5:
        failures.append('only %d of 5 reports on the tank' % len(tank))
    else:
        if tank[3][1] or tank[3][0] == tank[0][0]:
            failures.append('the tank did not leave the factory: %s -> %s' % (tank[0][0], tank[3][0]))
        if tank[3][2] == 16:
            failures.append('the war factory is still unloading at frame 420')
        if tank[4][0] == tank[3][0]:
            failures.append('the tank did not move when ordered: still at %s' % (tank[4][0],))
    bad = sorted(set(l for l in text.splitlines() if standins.BAD.search(l)))
    if bad:
        failures.append('traps or faults: %s' % '; '.join(bad[:5]))
    for f in failures:
        print('FAIL: ' + f)
    print('%s: a robot tank with a powered control centre leaves its factory and moves (%s)'
          % ('FAIL' if failures else 'OK', a.work))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
