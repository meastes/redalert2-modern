#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest yr-native-grinder (lane P61; built with -DRA2_NATIVE_YR=ON): a soldier sent into the player's Grinder is ground up
and refunded, and one sent into the player's Bio Reactor is taken in and powers it, as gamemd.exe does.

gamemd.exe's InfantryClass::Per_Cell_Process (0x00519630) grinds a soldier whose mission is MISSION_EATEN when it reaches
the building's cell (0x0051973C: the building's house refunded Refund_Amount, the soldier deleted), and a soldier entering
a bio reactor (InfantryAbsorb) stops counting as its house's infantry and has the house's power recalculated
(0x0051A2AD-0x0051A39D). The draft (glue/infantry_native.inc, TibSun's text) had no MISSION_EATEN step: the soldier
walked to the grinder and stood there, no refund.

Through the menus into a skirmish (yr_standins.py's route), harness orders (glue/yr_harness.cpp):
  frame 60    a Grinder (YAGRND) and a Bio Reactor (YAPOWR) spawned for the player, a GI (E1) beside each;
  frame 70    the player's money and the reactor's state (`absorb YAPOWR`: passengers, power, drain, infantry count);
  frame 75    the player's first GI is sent into the grinder as its click sends it (`enter E1 YAGRND eaten`);
  frame 400   money: more by the GI's refund; that GI gone;
  frame 410   the next GI is sent into the reactor (`enter E1 YAPOWR own`); 700 the reactor has a passenger and more power.

  yr_grinder.py --binary <yr-native> --data-dir <game folder> --work <dir>
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
ORDERS = ('60 spawn YAGRND 6 0; 61 spawn YAPOWR 0 6; 62 spawn E1 near YAGRND 3 0; 63 spawn E1 near YAPOWR 3 0; '
          '70 money; 71 absorb YAPOWR; 72 owner E1; 75 enter E1 YAGRND eaten; '
          '400 money; 401 owner E1; 410 enter E1 YAPOWR own; 700 absorb YAPOWR; 701 owner E1')
BAD = re.compile(r'unrecovered|\btrap\b|signal \d+|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:', re.I)


def block(text, frame, order):
    """The lines an order wrote: those after the previous `harness-order` line, up to its own."""
    head = text.split('harness-order %d %s' % (frame, order))[0]
    return head.rsplit('harness-order', 1)[-1] if 'harness-order %d %s' % (frame, order) in text else ''


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--seconds', type=int, default=100)
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
    script = os.path.join(a.work, 'grinder.script')
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

    def money(frame):
        m = re.search(r'^harness-money (-?\d+)', block(text, frame, 'money'), re.M)
        return int(m.group(1)) if m else None

    def gis(frame):
        return re.findall(r'^harness-owner E1 h\d+ p( limbo)? &(\S+)', block(text, frame, 'owner E1'), re.M)

    def absorb(frame):
        m = re.search(r'^harness-absorb YAPOWR cargo (\d+) power (-?\d+) drain (-?\d+) infantry (-?\d+)',
                      block(text, frame, 'absorb YAPOWR'), re.M)
        return tuple(int(x) for x in m.groups()) if m else None

    sent = re.search(r'^harness-enter E1 &(\S+) -> YAGRND ', text, re.M)
    before, after = money(70), money(400)
    print('grinder: money %s -> %s; GIs %d -> %d; the GI sent %s' % (before, after, len(gis(72)), len(gis(401)),
                                                                   sent.group(1) if sent else None))
    if not sent:
        failures.append('the grinder order did not run')
    if before is None or after is None or after <= before:
        failures.append('no refund for the ground GI (money %s -> %s)' % (before, after))
    if sent and any(addr == sent.group(1) for _, addr in gis(401)):
        failures.append('the GI sent into the grinder is still there at frame 400')

    r0, r1 = absorb(71), absorb(700)
    print('bio reactor: (passengers, power, drain, infantry) %s -> %s' % (r0, r1))
    if not re.search(r'^harness-enter E1 &\S+ -> YAPOWR ', text, re.M):
        failures.append('the bio reactor order did not run')
    elif r0 is None or r1 is None or r1[0] != r0[0] + 1 or r1[1] <= r0[1]:
        failures.append('the bio reactor did not take the GI in and power up: %s -> %s' % (r0, r1))

    bad = [line for line in text.splitlines() if BAD.search(line)]
    if bad:
        failures.append('trap/crash: %s' % bad[:3])
    for f in failures:
        print('FAIL: ' + f)
    print('yr-native-grinder: %s' % ('FAILED' if failures else 'ok'))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
