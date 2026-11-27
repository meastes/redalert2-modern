#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest yr-native-slave-miner (lane P57; built with -DRA2_NATIVE_YR=ON): a deployed Slave Miner sold with its slaves out
undeploys into the Slave Miner vehicle, which takes over the building's slave manager and slaves, as gamemd.exe does.

gamemd.exe's BuildingClass::Do_MISSION_DECONSTRUCTION (0x00449C30) undeploys any building that Can_Be_Undeployed (any
UndeploysInto type but a construction yard) when it is sold, and hands its SlaveManager to the new unit
(0x0044A03C: SlaveManagerClass::SetOwner), so the slaves name the vehicle as their owner. The draft
(glue/building_native.inc, TibSun's text) undeployed only 1x1 buildings (IsVehicle) and had no SetOwner: the sold YAREFN
was refunded and deleted with its manager.

Through the menus into a skirmish (yr_standins.py's route), harness orders (glue/yr_harness.cpp):
  frame 60    a deployed Slave Miner (YAREFN) spawned for the player;
  frame 100   its manager and live slaves (their IDs) logged (`slaves YAREFN`); 110 it is sold (where the skirmish's
              random start has no ore near, the miner undeploys by itself to look for some: the same deconstruction);
  frame 300   no YAREFN; one SMIN whose manager holds the same slaves (by ID) and owns them; every SLAV names the SMIN
              (left alone, the SMIN redeploys where it finds ore);
  frame 310   the SMIN is destroyed; 500 no slave names a deleted techno.
Without SetOwner the SMIN has a new manager with new slaves (other IDs) and the building's slaves die with its manager.

  yr_slave_miner.py --binary <yr-native> --data-dir <game folder> --work <dir>
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
ORDERS = ('60 spawn YAREFN 6 0; 100 slaves YAREFN; 101 slaves SLAV; 110 sell YAREFN; '
          '300 slaves YAREFN; 301 slaves SMIN; 302 slaves SLAV; 310 kill SMIN; 500 slaves SLAV')
BAD = re.compile(r'unrecovered|\btrap\b|signal \d+|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:', re.I)


def reports(text):
    """{order frame: [lines the order wrote]}: the harness logs `harness-order <frame> ...` after each order's lines."""
    seen, pending = {}, []
    for line in text.splitlines():
        if line.startswith(('harness-slaves ', 'harness-slave ')):
            pending.append(line)
            continue
        d = re.match(r'^harness-order (\d+) slaves ', line)
        if d:
            seen[int(d.group(1))] = pending
            pending = []
    return seen


def managers(lines, kind):
    out = []
    for line in lines:
        m = re.match(r'^harness-slaves %s manager (yes|no) nodes (\d+) live (\d+) owned (\d+) ids (\S+) ' % kind, line)
        if m:
            out.append((m.group(1) == 'yes', int(m.group(2)), int(m.group(3)), int(m.group(4)), m.group(5)))
    return out


def owners(lines):
    return [m.group(1) for m in (re.match(r'^harness-slave SLAV owner (\S+) ', line) for line in lines) if m]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--seconds', type=int, default=130)
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
    script = os.path.join(a.work, 'slaves.script')
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
    seen = reports(text)
    failures = []

    before = managers(seen.get(100, []), 'YAREFN')
    print('frame 100: YAREFN %s; slaves name %s' % (before, owners(seen.get(101, []))))
    if len(before) != 1 or not before[0][0] or before[0][2] < 1 or before[0][3] != before[0][2]:
        failures.append('frame 100: not one YAREFN with a manager and live slaves it owns: %s' % before)
    if not re.search(r'^harness-order 110 sell YAREFN: ', text, re.M):
        failures.append('the sell order did not run')   # -1 when the miner had already undeployed by itself

    gone = seen.get(300, [])
    after = managers(seen.get(301, []), 'SMIN')
    print('frame 300: YAREFN %s; SMIN %s; slaves name %s' % (gone, after, owners(seen.get(302, []))))
    if not any(line.startswith('harness-slaves YAREFN gone') for line in gone):
        failures.append('frame 300: the sold YAREFN is still there (%s)' % gone)
    # the start's own SMIN (if any) have their own managers; the undeployed one has the building's slaves
    took = [m for m in after if m[0] and m[2] >= 1 and m[3] == m[2]]
    if not took:
        failures.append('frame 300: no SMIN took over a manager with live slaves (%s)' % after)
    elif before and not any(m[4] == before[0][4] for m in took):
        failures.append('frame 300: the SMIN\'s slaves are %s, the YAREFN\'s were %s (not handed over)'
                        % ([m[4] for m in took], before[0][4]))
    slaves = owners(seen.get(302, []))
    # (other houses' slave miners' slaves name theirs)
    if 'stale' in slaves or (before and slaves.count('SMIN') < before[0][2]):
        failures.append('frame 300: the slaves name %s, not the SMIN' % slaves)
    if 'manager-owner-mismatch' in text:
        failures.append('a manager\'s Owner is not the techno that holds it')

    if not re.search(r'^harness-kill SMIN result ', text, re.M):
        failures.append('the SMIN kill order did not run')
    late = owners(seen.get(500, []))
    print('frame 500: slaves name %s' % late)
    if 'stale' in late:
        failures.append('frame 500: a slave names a deleted techno')
    if 500 not in seen:
        failures.append('the run did not reach frame 500')

    bad = sorted(set(l for l in text.splitlines() if BAD.search(l) and not l.startswith('input-script')))
    if bad:
        failures.append('traps or faults: %s' % '; '.join(bad[:5]))
    for f in failures:
        print('FAIL: ' + f)
    print('%s: a sold Slave Miner undeploys with its slaves (%s)' % ('FAIL' if failures else 'OK', a.work))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
