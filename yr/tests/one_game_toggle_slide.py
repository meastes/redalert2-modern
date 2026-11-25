#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest one-game-toggle-slide (lane P17; the user's Duo report: "the Play Yuri's Revenge button flashes between menu
changes, after buttons slide out and before they slide in"): the combined app's main-menu toggle (control 0x7F5E, a
sidebar button in the slot above Exit Game) shows only while the column's buttons do. In each game (--game=ra2, then
--game=yr) the menus go main menu -> Options -> Main Menu -> Single Player -> Main Menu (then quit), with a window dump at every
present (RA2_NATIVE_WINDOW_DUMP_MS=0). In each dump the toggle's rectangle and the slot below it (Exit Game in the main
menu; the dialogs' own bottom button elsewhere) are measured (the share of the sidebar buttons' red art). A dump with
the toggle drawn while the slot below is empty is the flash: the toggle drawn alone over the emptied column (before
the fix, two presents between the main menu's slide-out and the next dialog's slide-in: Enable_Animated_Control's
rest did not take the toggle). Also: the toggle is seen in the main menu at least twice (the first menu and the
return), and the dialogs were all shown.
Skipped (77) without the installed game (RA2 with Yuri's Revenge in --data-dir).

  one_game_toggle_slide.py --binary <ra2yr-native> --data-dir <game folder> --work <dir> [--games ra2,yr]
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tests'))
import game_process  # noqa: E402  (tests: the game's process group, lane C19)

INI = '[Video]\nScreenWidth=800\nScreenHeight=600\n'
# Options (the main menu's fifth button), its Main Menu (bottom), Single Player (first), its Main Menu (bottom): each
# click waits for its dialog to settle ("s<k>", host.cpp; k counts that engine's dialogs).
SCRIPT = 's1+2500 click 720 388\ns2+2500 click 720 556\ns3+2500 click 720 220\ns4+2500 click 720 556\ns5+2500 quit\n'
LIT = 0.15        # a button's red art over its rectangle (the toggle and Exit Game: about 0.3)
EMPTY = 0.05      # an empty slot


class Bmp:
    """Pixels of an uncompressed 24/32-bit BMP (SDL_SaveBMP) read where asked, without decoding the whole picture."""

    def __init__(self, path):
        self.data = open(path, 'rb').read()
        d = self.data
        self.offset = int.from_bytes(d[10:14], 'little')
        self.width = int.from_bytes(d[18:22], 'little', signed=True)
        h = int.from_bytes(d[22:26], 'little', signed=True)
        self.bottom_up, self.height = h > 0, abs(h)
        self.step = int.from_bytes(d[28:30], 'little') // 8
        self.stride = (self.width * self.step + 3) & ~3

    def pixel(self, x, y):
        row = self.height - 1 - y if self.bottom_up else y
        i = self.offset + row * self.stride + x * self.step
        return self.data[i + 2], self.data[i + 1], self.data[i]

    def red_share(self, x, y, w, h):
        """The share of red-art pixels in the 800x600 game rectangle (the picture scaled to the window)."""
        sx, sy = self.width / 800.0, self.height / 600.0
        n = red = 0
        for j in range(0, h, 2):
            py = int((y + j + 0.5) * sy)
            if not 0 <= py < self.height:
                continue
            for i in range(0, w, 2):
                px = int((x + i + 0.5) * sx)
                if not 0 <= px < self.width:
                    continue
                r, g, b = self.pixel(px, py)
                n += 1
                red += r >= 80 and r > 1.5 * g and r > 1.5 * b
        return red / max(1, n)


def run(a, game):
    work = os.path.join(a.work, game)
    shots = os.path.join(work, 'shots')
    writes = os.path.join(work, 'w')
    os.makedirs(shots)
    os.makedirs(writes)
    with open(os.path.join(writes, 'RA2.INI'), 'w') as f:
        f.write(INI)
    script = os.path.join(work, 'e0.script')
    with open(script, 'w') as f:
        f.write(SCRIPT)
    env = dict(os.environ, RA2_NATIVE_SETTINGS=os.path.join(work, 'settings'),
               RA2_NATIVE_WINDOW_DUMP=os.path.join(shots, 'w'), RA2_NATIVE_WINDOW_DUMP_MS='0',
               RA2_NATIVE_IGNORE_FOCUS='1', RA2_NATIVE_DIALOG_LOG='1', RA2_NATIVE_MOVIE_SKIP='1')
    for k in ('RA2_GAMEMD_EXE', 'RA2_NATIVE_INPUT_SCRIPT'):
        env.pop(k, None)
    cmd = [a.binary, '--game=' + game, '--data-dir', a.data_dir, '--run-to=never', '--run-seconds=%d' % a.seconds,
           '--write-dir=' + writes, '--game-args=-noaudio', '--engine-scripts=' + script]
    log_path = os.path.join(work, 'run.log')
    with open(log_path, 'w') as log:
        game_process.run(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=a.seconds + 120)
    return open(log_path, errors='replace').read(), shots


def check(game, text, shots, failures):
    if 'trap reached' in text or re.search(r'stopped: (signal|trap|fault)', text):
        failures.append('%s: a trap or fault: %s' % (game, [l for l in text.splitlines() if 'stopped:' in l]))
    placed = re.findall(r'^port: game toggle "[^"]+" at (-?\d+),(-?\d+) (\d+)x(\d+) in the main menu', text, re.M)
    shown = len(re.findall(r'^dialog shown #\d+ ', text, re.M))
    if len(placed) < 2 or shown < 5:
        failures.append('%s: the route did not run: toggle placed %d times, %d dialogs shown (wanted 2 and 5)'
                        % (game, len(placed), shown))
        return
    x, y, w, h = (int(v) for v in placed[0])
    dumps = sorted(int(m.group(1)) for m in (re.search(r'\.(\d+)\.bmp$', f) for f in os.listdir(shots)) if m)
    seen = alone = 0
    first_alone = None
    for n in dumps:
        path = os.path.join(shots, 'w-e0.%d.bmp' % n)
        if not os.path.exists(path):
            path = os.path.join(shots, 'w.%d.bmp' % n)
        bmp = Bmp(path)
        toggle = bmp.red_share(x, y, w, h)
        below = bmp.red_share(x, y + h, w, h)
        if toggle >= LIT:
            seen += 1
            if below < EMPTY:
                alone += 1
                if first_alone is None:
                    first_alone = (n, toggle, below)
    print('%s: toggle at %d,%d %dx%d: %d presents dumped, drawn in %d, drawn over an empty column in %d%s'
          % (game, x, y, w, h, len(dumps), seen, alone,
             ' (first: dump %d, %.0f%% red, the slot below %.0f%%)' % (first_alone[0], first_alone[1] * 100, first_alone[2] * 100)
             if first_alone else ''))
    if seen < 20:
        failures.append('%s: the toggle was drawn in only %d presents' % (game, seen))
    if alone:
        failures.append('%s: the toggle flashed: drawn over the emptied column in %d presents (dump %d first)'
                        % (game, alone, first_alone[0]))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--games', default='ra2,yr')
    ap.add_argument('--seconds', type=int, default=150)
    a = ap.parse_args()
    if not a.data_dir or not os.path.exists(os.path.join(a.data_dir, 'gamemd.exe')):
        print('skipped: no Yuri\'s Revenge install (gamemd.exe) in --data-dir')
        return 77
    shutil.rmtree(a.work, ignore_errors=True)
    failures = []
    for game in a.games.split(','):
        text, shots = run(a, game)
        check(game, text, shots, failures)
    for f in failures:
        print('FAIL: ' + f)
    print('%s: the toggle through the menu slides (%s)' % ('FAIL' if failures else 'OK', a.work))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
