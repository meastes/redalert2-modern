#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Lane P8 (branch ports-on-rebuild): main's src-port tests unchanged against the rebuild trial.

Main's tests (tests/*.py) start their skirmish with the src-based port's skirmish harness
(--skirmish=<map>, --skirmish-order=build with RA2_NATIVE_BUILD_LIST: link/harness_skirmish.cpp,
harness_build.cpp), which the trial does not carry. This wrapper stands in for the binary (--binary
rb_skirmish_wrap.py, with RB_TRIAL naming the trial, default build/mac/rb-native-trial): it keeps
--skirmish (the port takes --skirmish-frames with it; the trial's game side ignores the map) and starts the same kind of game through the real menus instead (Single Player, Skirmish,
Start Game: the setup's defaults, one computer player; rb_routes.py's START), by putting those clicks
in front of the test's input script; the test's own lines keep their loop frames (counted from the
game's first frame in both ports). With Zoom gestures on (RA2_NATIVE_ENHANCEMENTS zoomgestures=on) the
menus are at the scaled screen's centre (1880x1504 for 1024x768), so the clicks move by (428, 368).
--skirmish-order=build: the RA2_NATIVE_BUILD_LIST buildings are placed by the trial's harness (spawn
orders at loop frames 60, 70, ... beside the base). Everything else is passed through.

Differences from main's harness, which a test may notice: the map is the setup's default rather than
'#5' and the computer player's country is the setup's choice; the objects a test names ('@MTNK:1')
are resolved in the game as in main's port (harness_input_native.inc).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TRIAL = os.environ.get('RB_TRIAL') or os.path.join(HERE, '..', '..', 'build', 'mac', 'rb-native-trial')
START = [(1, 1000, 834, 304), (2, 1000, 834, 388), (3, 1500, 834, 346)]


def main():
    args = sys.argv[1:]
    out = []
    skirmish = False
    build = False
    script = None
    for a in args:
        if a.startswith('--skirmish='):
            skirmish = True     # kept: the port takes --skirmish-frames with it (the trial's game side ignores the map)
        if a == '--skirmish-order=build':
            build = True
            continue
        if a.startswith('--input-script='):
            script = a.split('=', 1)[1]
            continue
        out.append(a)
    env = dict(os.environ)
    # Lane P14: the tests were written for Zoom gestures off, every platform's default until lane P12 turned them on
    # for iOS (the menus then run at render scale 2 and the menu clicks miss); a test that wants them says so.
    if 'zoomgestures=' not in env.get('RA2_NATIVE_ENHANCEMENTS', ''):
        env['RA2_NATIVE_ENHANCEMENTS'] = ','.join(x for x in (env.get('RA2_NATIVE_ENHANCEMENTS', ''), 'zoomgestures=off') if x)
    if skirmish:
        # Main's harness keeps RA2.INI's [Options] GameSpeed (3 in the user's install); the setup dialog stores its slider (0).
        env.setdefault('RA2_NATIVE_SKIRMISH_SPEED', '3')
        dx, dy = (428, 368) if 'zoomgestures=on' in env.get('RA2_NATIVE_ENHANCEMENTS', '') else (0, 0)
        lines = ['s%d+%d click %d %d' % (k, ms, x + dx, y + dy) for k, ms, x, y in START]
        if script and os.path.isfile(script):
            with open(script) as f:
                text = f.read()
            # The test's dialog-relative lines ("dN+ms", "sN+ms") count the dialogs from the first one shown; the
            # menus add three before the game's.
            body = []
            for line in text.splitlines():
                t = line.split(None, 1)
                if t and t[0][:1] in 'ds' and '+' in t[0] and t[0][1:t[0].index('+')].isdigit():
                    k = int(t[0][1:t[0].index('+')]) + 3
                    line = '%s%d%s %s' % (t[0][0], k, t[0][t[0].index('+'):], t[1] if len(t) > 1 else '')
                body.append(line)
            lines += body
        merged = os.path.join(os.path.dirname(script) if script else env.get('TMPDIR', '/tmp'), 'rb-skirmish.script')
        with open(merged, 'w') as f:
            f.write('\n'.join(lines) + '\n')
        out.append('--input-script=' + merged)
    elif script:
        out.append('--input-script=' + script)
    if build:
        orders = []
        # the third (the HD air-base check's Patriot, NASAM) straight below the yard on screen (x - y unchanged), where the
        # view keeps it at either map edge; (-5, 0) put it 11 cells left of the yard, off the view at a western start (lane P9)
        offsets = [(0, 6), (4, 0), (3, 3), (2, 6), (-2, 6), (4, 3)]
        for k, name in enumerate(x for x in env.get('RA2_NATIVE_BUILD_LIST', '').split(',') if x):
            dx, dy = offsets[k % len(offsets)]
            orders.append('%d spawn %s %d %d' % (60 + 10 * k, name, dx, dy))
        if orders:
            env['RA2_NATIVE_ORDERS'] = '; '.join(filter(None, [env.get('RA2_NATIVE_ORDERS', '')] + orders))
    os.execve(TRIAL, [TRIAL] + out, env)


if __name__ == '__main__':
    main()
