#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest one-game-yr-play-wine (lane P19; built with -DRA2_NATIVE_COMBINED=ON, Windows): the route of codex1's
Windows crash of 3182afe ("Red Alert 2 stopped" in early Yuri's Revenge play after the MCV was selected): the
combined app's console host (ra2yr-native.exe, under Wine in a fresh prefix) starts Red Alert 2, its main-menu
toggle swaps to Yuri's Revenge, and a skirmish starts from YR's menus with Game interface size 1.25x, Map zoom
1.5x and game speed 2 (as on the PC). The harness selects the MCV and the script clicks on it (the deploy click),
then the scripted mouse moves, clicks and right-clicks over the map for the rest of the run.

Under Wine that route found two faults of 3182afe's Windows build: the deploy click stopped at once in
TechnoClass::VoiceDeploy (0x00708E00), which falls off its end without a return, where mingw's clang put a trap
(the Mac's clang returns; -fno-strict-return on Windows now); and after 8,190 frames fastmath::atan2f (0x004CB3D0)
read 8 GB before atan_tab, as a ratio past _ftol's range came out INT_MIN on x86_64 (yr/fixups.json).

Checks: both engines start in that order; the click on the selected MCV deploys it (the player has a Construction
Yard at frame 1200); the skirmish runs at least --min-frames frames to the time limit; no trap, fault or signal.
Skipped (77) without Wine or the installed game (RA2 with Yuri's Revenge in --data-dir, game.exe given).

  one_game_yr_play.py --binary <ra2yr-native.exe> --data-dir <game folder> --game-exe <game.exe> --work <dir>
                      --wine-prefix <dir> [--wine <wine>] [--seconds 420] [--min-frames 9000]
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tests'))
import game_process  # noqa: E402  (tests: the game's process group, lane C19)
import yr_wine  # noqa: E402  (the Windows build under Wine, lanes YW, Y16)

TOGGLE_ID = 0x7F5E      # the main menu's "Play Yuri's Revenge" (one_game_toggle.py)
SETTINGS = ('[Video]\nScreenWidth=800\nScreenHeight=600\n\n[Intro]\nPlay=no\n\n[Skirmish]\nGameMode=1\nScenIndex=0\n'
            'GameSpeed=2\nCredits=10000\nUnitCount=10\nShortGame=yes\nSuperWeaponsAllowed=yes\nBuildOffAlly=yes\n'
            'MCVRepacks=yes\nCratesAppear=yes\nSlot01=6,-2,-2\n' + ''.join('Slot%02d=1,-2,-2\n' % i for i in range(2, 8)))
ORDERS = '100 select AMCV 0; 110 where AMCV; 1200 list'


def play_script():
    """YR's menus (Single Player, Skirmish, Start Game), the click on the selected MCV, then mouse moves, clicks and
    right-clicks over the map."""
    lines = ['s1+1500 click 720 220', 's2+1000 click 720 304', 's3+2500 click 720 262', 's3+3500 move 316 300',
             '120 move @unit=AMCV', '125 click @unit=AMCV']
    frame = 1300
    for i in range(60):
        x, y = 150 + (i * 29) % 330, 100 + (i * 41) % 380
        lines += ['%d move %d %d' % (frame, x, y), '%d click %d %d' % (frame + 6, x, y),
                  '%d rclick %d %d' % (frame + 12, 200 + (i * 37) % 300, 150 + (i * 53) % 300)]
        frame += 21
    return '\n'.join(lines) + '\n'


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--game-exe', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--seconds', type=int, default=420)
    ap.add_argument('--min-frames', type=int, default=9000)
    yr_wine.add_arguments(ap)
    a = ap.parse_args()
    if not a.data_dir or not os.path.exists(os.path.join(a.data_dir, 'gamemd.exe')):
        print('skipped: no Yuri\'s Revenge install (gamemd.exe) in --data-dir')
        return 77
    if not a.game_exe or not os.path.exists(a.game_exe):
        print('skipped: no game.exe (--game-exe)')
        return 77
    shutil.rmtree(a.work, ignore_errors=True)
    writes = os.path.join(a.work, 'w')
    os.makedirs(writes)
    with open(os.path.join(writes, 'RA2.INI'), 'w') as f:
        f.write('[Video]\nScreenWidth=800\nScreenHeight=600\n')
    with open(os.path.join(writes, 'RA2MD.INI'), 'w') as f:
        f.write(SETTINGS)
    scripts = [os.path.join(a.work, 'e0.script'), os.path.join(a.work, 'e1.script')]
    with open(scripts[0], 'w') as f:
        f.write('s1+2300 ldown @ctl=%d\ns1+2900 lup @ctl=%d\n' % (TOGGLE_ID, TOGGLE_ID))
    with open(scripts[1], 'w') as f:
        f.write(play_script())
    env = dict(os.environ, RA2_NATIVE_SETTINGS=os.path.join(a.work, 'settings'), RA2_GAME_EXE=a.game_exe,
               RA2_NATIVE_IGNORE_FOCUS='1', RA2_NATIVE_MOVIE_SKIP='1', RA2_NATIVE_ENHANCEMENTS='uisize=1.25,mapzoom=1.5',
               RA2_NATIVE_ORDERS=ORDERS)
    for k in ('RA2_GAMEMD_EXE', 'RA2_NATIVE_INPUT_SCRIPT'):
        env.pop(k, None)
    cmd = [a.binary, '--game=ra2', '--data-dir', a.data_dir, '--run-to=never', '--run-seconds=%d' % a.seconds,
           '--write-dir=' + writes, '--game-args=-noaudio', '--engine-scripts=' + ','.join(scripts)]
    prefix = yr_wine.open_prefix(a)
    if prefix == yr_wine.SKIP:
        return 77
    cmd, env = yr_wine.wrap(prefix, cmd, env)
    log_path = os.path.join(a.work, 'run.log')
    try:
        with open(log_path, 'w') as log:
            game_process.run(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=a.seconds + 240)
    finally:
        yr_wine.close(prefix)
    text = open(log_path, errors='replace').read()
    failures = []
    engines = re.findall(r'^one-game: engine \d+: (Red Alert 2|Yuri\'s Revenge)', text, re.M)
    if engines[:2] != ['Red Alert 2', "Yuri's Revenge"]:
        failures.append('the engines started were %s (wanted Red Alert 2, then Yuri\'s Revenge)' % engines)
    if 'enhancements: sidebar 480 of 600 screen rows (interface 1.25x)' not in text or 'map zoom 1.5x' not in text:
        failures.append('Game interface size 1.25x and Map zoom 1.5x were not in effect')
    if not re.search(r'^harness-list h\d+p GACNST ', text, re.M):
        failures.append('the click on the selected MCV did not deploy it (no Construction Yard of the player\'s at frame 1200)')
    stop = re.search(r'^run-to: target never; stopped: (.*)$', text, re.M)
    if not stop or not stop.group(1).startswith('time limit'):
        failures.append('the run did not reach its time limit: %s' % (stop.group(1) if stop else 'no stop line'))
    traps = sorted(set(re.findall(r'unrecovered (\S+) reached', text)))
    if traps or re.search(r'crash: signal|stopped: (signal|trap|fault)', text):
        failures.append('a trap or fault: %s' % (traps or [l for l in text.splitlines() if 'signal' in l][:3]))
    frames = re.search(r'^run-to:\s+frames run (\d+)', text, re.M)
    if not frames or int(frames.group(1)) < a.min_frames:
        failures.append('%s game frames run (wanted at least %d)' % (frames.group(1) if frames else 'no', a.min_frames))
    for f in failures:
        print('FAIL: ' + f)
    print('%s: Red Alert 2 -> Yuri\'s Revenge, a skirmish at interface 1.25x, map zoom 1.5x, speed 2: %s frames (%s)' % (
        'FAIL' if failures else 'OK', frames.group(1) if frames else '?', a.work))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
