#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctests ra2-native-campaign-won-<mission> (lane P21): a campaign mission won by a recorded play, replayed.

The mission is reached through the real menus as ra2/tools/ra2_step.py reaches it for the live step server (Single
Player, New Campaign, the side's emblem; missions before it won by RA2_NATIVE_FORCE_END), and the recorded commands
(ra2/tests/campaign/<mission>.replay: "<tick> input|look|order|save ...", inputs and ticks only, no game data) run at
their ticks (RA2_NATIVE_STEP_REPLAY, ra2/glue/stepserver_native.inc): clicks and keys through the port's input path,
the view moved through the trackpad pan's hook, the periodic saves written. The game runs at game speed 0, uncapped,
with the gate's fixed seed, and only scripted input reaches it, as when it was recorded.

With --asan (lane P35; the binary built with RA2_NATIVE_ASAN=ON, ctest label ra2-asan) AddressSanitizer's reports go
to <work>/asan.<pid> and any report fails the run, as a fault does.

Pass: the mission's own triggers end it in a win (step-replay: game over ... (win), game-over: player wins), at the tick
the recording ended (a different tick means the replay went another way: reported), the mission's score screen shown
(the dialog after the win), the save points written, and no fault, trap or assertion. Skipped (77) without the
installed game.

  ra2_campaign.py --replay ra2/tests/campaign/all01.replay --side allied --mission 1 --binary <ra2-native>
                  --data-dir <game folder> [--game-exe <game.exe>] --work <dir> [--seconds 900] [--keep] [--asan]

--game yr (lane P50; ctests yr-native-campaign-won-<mission>, yr/yr.cmake): Yuri's Revenge's (yr-native, its step server
yr/glue/yr_stepserver.cpp; replays in yr/tests/campaign/), skipped without gamemd.exe in --data-dir. --soak <n> adds the
mission soak's digests every n game frames to the log (RA2_NATIVE_SOAK: where two runs of a replay part).

--survive <tick> (lane P55; ctest yr-native-campaign-crash-all05): a recording that ended in a fault, not a win, replayed
past it: a copy of the replay gets "<tick> order money" (a harness line in the log at that tick) and a quit just after it.
Pass: the game reaches that tick with no fault, trap or assertion, and the mission was not lost; no win is expected.
"""
import argparse
import glob
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'tests'))
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
import game_process  # noqa: E402
import mission_soak  # noqa: E402
import ra2_step  # noqa: E402

SKIP = 77
BAD = re.compile(r'AddressSanitizer|unrecovered|\btrap\b|signal \d+|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:|assert|not written|step-replay: .*not a recorded',
                 re.I)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--replay', required=True)
    ap.add_argument('--side', default='allied', choices=['allied', 'soviet'])
    ap.add_argument('--mission', type=int, default=1)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--game-exe', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--seconds', type=int, default=900)
    ap.add_argument('--keep', action='store_true', help='keep the write folder (saves) after a pass')
    ap.add_argument('--asan', action='store_true', help='an AddressSanitizer build: any report fails (lane P35)')
    ap.add_argument('--game', default='ra2', choices=['ra2', 'yr'], help='the game (lane P50): ra2-native or yr-native')
    ap.add_argument('--soak', type=int, default=0, help='the mission soak\'s digests every n game frames (diagnostics)')
    ap.add_argument('--survive', type=int, default=0, help='pass when this tick is reached with no fault (lane P55)')
    a = ap.parse_args()
    exe = 'gamemd.exe' if a.game == 'yr' else 'game.exe'
    if not a.data_dir or not os.path.isfile(os.path.join(a.data_dir, exe)):
        print('campaign-won: skipped: no installed game (%s) in --data-dir' % exe)
        return SKIP
    with open(a.replay) as f:
        text = f.read()
    m = re.search(r'^(\d+) # game over: won', text, re.M)
    want_tick = int(m.group(1)) if m else -1
    saves = re.findall(r'^\d+ save (\S+)', text, re.M)
    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    replay = os.path.abspath(a.replay)
    if a.survive:
        # the replay's copy runs on to the tick, logs there and quits (the step server queues the quit as input)
        replay = os.path.join(work, 'survive.replay')
        with open(replay, 'w') as f:
            f.write(text + ('' if text.endswith('\n') else '\n'))
            f.write('%d order money\n%d input +10 quit\n' % (a.survive, a.survive))
    write, script = ra2_step.prepare(a, work)
    # The score screen of mission n is dialog 3 + n (the menus are 1-3; each forced win before it shows one): leave
    # two seconds after it is shown.
    with open(script, 'a') as f:
        f.write('d%d+2000 quit\n' % (3 + a.mission))
    env = ra2_step.campaign_env(a, work, write)
    env['RA2_NATIVE_STEP_REPLAY'] = replay
    if re.search(r'^# frame clock', text, re.M):
        env['RA2_NATIVE_FRAME_CLOCK'] = '1'   # recorded with the frame clock (lane P33)
    if a.soak:
        env['RA2_NATIVE_SOAK'] = str(a.soak)
        if os.environ.get('RA2_NATIVE_SOAK_DUMP'):
            env['RA2_NATIVE_SOAK_DUMP'] = os.environ['RA2_NATIVE_SOAK_DUMP']   # every techno at those frames
    if a.asan:
        # halt_on_error=0 with the build's -fsanitize-recover=address: every report is written, the run goes on.
        env['ASAN_OPTIONS'] = 'halt_on_error=0:detect_leaks=0:detect_odr_violation=0:log_path=' + os.path.join(work, 'asan')
    cmd = [a.binary, '--data-dir', a.data_dir, '--run-to=never', '--run-seconds=%d' % a.seconds, '--write-dir=' + write,
           '--input-script=' + script, '--game-args=-noaudio -CD.']
    log = os.path.join(work, 'run.log')
    with open(log, 'w') as out:
        try:
            code = game_process.run(cmd, env=env, stdout=out, stderr=subprocess.STDOUT, timeout=a.seconds + 90).returncode
        except subprocess.TimeoutExpired:
            code = -1
    with open(log, errors='replace') as f:
        out = f.read()
    lines = out.splitlines()
    bad = []
    hits = [l for l in lines if BAD.search(l) and not l.startswith('input-script')]
    if hits:
        bad.append('fault or trap: %s' % hits[0][:200])
    m = re.search(r'^step-replay: (\d+) commands', out, re.M)
    if not m:
        bad.append('the replay was not loaded')
    over = re.search(r'^step-replay: game over at tick (\d+) \((win|lose)\)', out, re.M)
    if a.survive:
        if over and over.group(2) == 'lose':
            bad.append('the mission was lost at tick %s' % over.group(1))
        if not re.search(r'^step-replay %d order money$' % a.survive, out, re.M):
            ticks = re.findall(r'^step-replay (\d+) ', out, re.M)
            bad.append('tick %d was not reached (the last command run at tick %s)' % (a.survive, ticks[-1] if ticks else '-'))
        for b in bad:
            print('campaign-survive: FAIL: %s' % b)
        print('campaign-survive: %s %s: tick %d, exit %d (%s)' % (os.path.basename(a.replay), 'FAIL' if bad else 'OK',
                                                                 a.survive, code, log))
        if not bad and not a.keep:
            shutil.rmtree(write, ignore_errors=True)
        return 1 if bad else 0
    if not over:
        bad.append('the mission did not end (no game over)')
    elif over.group(2) != 'win':
        bad.append('the mission was lost at tick %s' % over.group(1))
    elif want_tick >= 0 and int(over.group(1)) != want_tick:
        bad.append('won at tick %s, the recording at %d: the replay went another way' % (over.group(1), want_tick))
    if not re.search(r'^game-over: player wins', out, re.M):
        bad.append('no game-over: player wins')
    late = [l for l in lines if 'run late' in l]
    if late:
        bad.append('commands run late: %s' % late[0])
    shown = [int(x) for x in re.findall(r'^dialog shown #(\d+)', out, re.M)]
    after = out.split('game-over: player wins', 1)[1] if 'game-over: player wins' in out else ''
    if not re.search(r'^dialog shown #%d ' % (3 + a.mission), after, re.M):
        bad.append('the score screen (dialog %d) was not shown after the win (dialogs %s)' % (3 + a.mission, shown[-3:]))
    if a.asan:
        for rep in sorted(glob.glob(os.path.join(work, 'asan.*'))):
            with open(rep, errors='replace') as f:
                summary = [l for l in f.read().splitlines() if l.startswith('SUMMARY: AddressSanitizer')]
            bad.append('AddressSanitizer: %s (%s)' % (summary[0][27:200] if summary else 'a report', rep))
    for s in saves:
        if not os.path.isfile(os.path.join(write, s)):
            bad.append('save point %s not written' % s)
    for b in bad:
        print('campaign-won: FAIL: %s' % b)
    print('campaign-won: %s %s: %s, exit %d (%s)' % (os.path.basename(a.replay), 'FAIL' if bad else 'OK',
                                                   over.group(0) if over else 'no end', code, log))
    if not bad and not a.keep:
        shutil.rmtree(write, ignore_errors=True)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
