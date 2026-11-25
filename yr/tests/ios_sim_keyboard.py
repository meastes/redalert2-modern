#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The on-screen keyboard for the games' text entry in the one-game iPhone app (lane P17, regression row C14-kbd,
the user's Duo report "Keyboard does not automatically show when selecting text fields"): on the "RA2 Duo
Validation" Simulator, one start per game (--game=ra2, --game=yr): main menu -> Single Player -> Skirmish, then

  1. a tap on the player name field (control 1696; every dialog Edit of both games is the same NewEdit list box,
     Subclass's Replace_Edit, so this stands for the LAN name, the lobby chat line and the save description too)
     raises the keyboard (`host: on-screen keyboard shown`);
  2. the Keys button puts it away while the field keeps the focus (`hidden`);
  3. a second tap on the field brings it back (`host: tap on the edit control: keyboard back`, `shown`);
  4. a tap away from the field puts it away (`hidden`);
  5. Start Game; in play the message list's edit line, the in-game chat and beacon text line, opened at frame 300
     and closed at frame 420 (the harness's `chat open|close`, as Main_Loop's chat key opens it in a network
     game: glue/mainloop_native.inc and glue/yr_app_hooks.cpp ra2_native_message_edit_open) raises the keyboard
     and puts it away (`shown`, `hidden`).

So each game's keyboard lines are shown, hidden, shown, hidden, shown, hidden. Not a plain ctest: it needs the
Simulator (ctest label one-ios, RA2_ONE_IOS_SIM=ON: one-game-ios-sim-keyboard).

  ios_sim_keyboard.py --app build/ios-simulator/RA2YR.app --data-dir <RA2 + YR folder> --work <dir>
                      [--games ra2,yr] [--play-seconds 75]
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ios_sim_one_game as one  # noqa: E402  (launch, simctl, device, the Simulator's names)

KEYS_BUTTON = -3   # the host's Keys button (platform/sdl/host.cpp kKeysButton), the script's `toolbar -3`


def tap(when, x, y):
    base, ms = when.split('+')
    return ['%s tdown %d %d' % (when, x, y), '%s+%d tup %d %d' % (base, int(ms) + 80, x, y)]


def script(play_w):
    mx = (play_w - 800) // 2
    lines = []
    lines += tap('s1+2000', mx + 720, 220)        # Single Player
    lines += tap('s2+1500', mx + 720, 304)        # Skirmish
    lines += ['s3+1200 tdown @ctl=1696', 's3+1280 tup @ctl=1696']          # 1. the name field
    lines += ['s3+1900 toolbar %d' % KEYS_BUTTON]                          # 2. Keys: put away
    lines += ['s3+2600 tdown @ctl=1696', 's3+2680 tup @ctl=1696']          # 3. the field again
    lines += tap('s3+3300', mx + 600, 300)        # 4. away from it
    lines += tap('s3+4000', mx + 720, 262)        # 5. Start Game
    return lines


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--app', required=True)
    ap.add_argument('--data-dir', required=True)
    ap.add_argument('--work', required=True)
    ap.add_argument('--udid', default=one.VALIDATION)
    ap.add_argument('--games', default='ra2,yr')
    ap.add_argument('--play-seconds', type=int, default=75)
    ap.add_argument('--play-w', type=int, default=872)
    ap.add_argument('--shot-seconds', type=float, default=30)
    ap.add_argument('--enhancements', default='menufill=backdrop,zoomgestures=off')
    ap.add_argument('--keep-booted', action='store_true')
    a = ap.parse_args()
    a.data_dir = os.path.abspath(a.data_dir)
    a.work = os.path.abspath(a.work)
    if not os.path.isdir(a.app) or not os.path.isdir(a.data_dir):
        print('skipped: no Simulator app (%s) or game folder (%s)' % (a.app, a.data_dir))
        return 77
    d = one.device(a.udid)
    if not d or d['name'] != one.VALIDATION_NAME:
        print('refused: %s is not the "%s" Simulator' % (a.udid, one.VALIDATION_NAME))
        return 2
    os.makedirs(a.work, exist_ok=True)
    booted_here = d['state'] != 'Booted'
    if booted_here:
        one.simctl('boot', a.udid)
        one.simctl('bootstatus', a.udid, '-b')
    failures = []
    try:
        one.simctl('install', a.udid, a.app)
        for game in a.games.split(','):
            env = {'RA2_NATIVE_ORDERS': '300 chat open; 420 chat close'}
            text = one.launch(a, 'keyboard-' + game, ['--game=' + game], [script(a.play_w)], a.play_seconds, env)
            if 'unrecovered ' in text or 'crash: signal' in text:
                failures.append('%s: a trap or fault: %s' % (game, [l for l in text.splitlines() if 'unrecovered' in l or 'crash:' in l][:2]))
            keys = re.findall(r'^host: on-screen keyboard (shown|hidden)', text, re.M)
            back = 'host: tap on the edit control: keyboard back' in text
            chat = re.findall(r'^harness-chat (open|close) edit (\d)', text, re.M)
            print('%s: keyboard %s; tap brought it back: %s; chat line %s' % (game, keys, back, chat))
            want = ['shown', 'hidden'] * 3
            if keys[:2] != want[:2]:
                failures.append('%s: a tap on the player name field did not raise the keyboard (%s)' % (game, keys))
            elif keys[:4] != want[:4] or not back:
                failures.append('%s: a tap on the name field did not bring back the keyboard put away with Keys (%s)' % (game, keys))
            if chat != [('open', '1'), ('close', '0')]:
                failures.append('%s: the chat line was not opened and closed in play (%s)' % (game, chat))
            elif keys[4:6] != want[4:6]:
                failures.append('%s: the in-game chat line did not raise the keyboard (%s)' % (game, keys))
    finally:
        if booted_here and not a.keep_booted:
            one.simctl('shutdown', a.udid, check=False)
    for f in failures:
        print('FAIL: ' + f)
    print('%s (%s)' % ('FAIL' if failures else 'OK', a.work))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
