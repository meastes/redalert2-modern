#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest yr-native-skirmish-layout (lane P15; built with -DRA2_NATIVE_YR=ON): the Skirmish Game screen of Yuri's
Revenge laid out as gamemd.exe lays it out, at the original's 800x600, on the Duo (a 1024x720 game in a
phone-shaped window with the phone's settings) and at a Mac size (a 1024x768 game in a 1440x900 window).

User report 2026-11-24 (the Duo, the combined app; the earlier port had it too): the map labels on the right
("Battle", "The Alamo (2)") sat too low, the map name across a panel border, and the Start column's "???"
wrapped onto two lines. The labels were where the dialog template puts them (0x6EC at 432,103 DLU, 0x5A8 at
432,116); the column's art under them was wrong. gamemd.exe draws the skirmish column with the map preview's
frame (record flag 0xD5, FUN_0072E8C0) and a dark label box under it (0xD6, FUN_0072E860; the button slots
start one slot lower), and the port's record kept those flags 0x20 bytes away from where OwnerDraw::Paint
reads them (yr/fixups.json, lane P15), so the plain column (the main menu's monitor ring and an empty grey
slot) was drawn behind the labels. The Start column's text was drawn in a box ending at the arrow (15 pixels
for its 37-pixel field) where gamemd.exe draws it to the field's right edge (0x00617BC6; the draft in
yr/glue/ownrdraw_combo_native.inc).

The reference is the original's layout (the RA2 port has drawn the same column right all along; the played
YR port had this bug, so it is not compared with). Checks in each run, in the 800x600 menu's coordinates
(mapped through the host's layout line when the menu is scaled):
  - the label box: below the map name, inside the box (650..780, 208..224), the column is dark (the box's
    black), not the grey button slot the plain column puts there;
  - the labels' text lies inside the box (its yellow rows between 162 and 228);
  - the Start column's "???" (combos 0x6A3, 0x6A4) is one line: its yellow pixels span at most 12 rows
    and at least 13 columns (wrapped: about 20 rows, at most 11 columns);
  - no trap or fault.

  yr_skirmish_layout.py --binary <yr-native> --data-dir <game folder> --work <dir>
Skipped (77) without the installed game (gamemd.exe in --data-dir).
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tests'))
import game_process  # noqa: E402  (tests: the game's process group, lane C19)
from one_game_toggle import read_bmp  # noqa: E402  (the dumps' reader)

# Main menu -> Single Player (the column's first slot) -> Skirmish (its second), then a dump once the setup
# has slid in. The default computer opponent gives the second row (0x50E, 0x6A4).
SCRIPT = 's1+1000 click 720 220\ns2+1000 click 720 304\ns3+3000 dump\ns3+3500 quit\n'
SETUP = ('[Intro]\nPlay=no\n\n[Skirmish]\nGameMode=1\nScenIndex=0\n'
         + ''.join('Slot%02d=%d,-2,-2\n' % (i, 6 if i == 1 else 1) for i in range(1, 8)))
RUNS = [
    # name, [Video], host settings, window
    ('800x600', '[Video]\nScreenWidth=800\nScreenHeight=600\n', '', None),
    ('duo', '[Video]\nScreenWidth=1024\nScreenHeight=720\nAllowHiResModes=yes\n',
     'menufill=fit,uisize=1.25,zoomgestures=on,vsync=on,scaling=smooth', '1136x800'),
    ('mac', '[Video]\nScreenWidth=1024\nScreenHeight=768\nAllowHiResModes=yes\n', 'menufill=fit', '1440x900'),
]
LAYOUT = re.compile(r'^host: layout: menu (\d+)x(\d+) at the scale of \S+: picture (-?[\d.]+),(-?[\d.]+) ([\d.]+)x([\d.]+) in '
                    r'([\d.]+)x([\d.]+)', re.M)
BAD = re.compile(r'trap reached|stopped: (signal|trap|fault)|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:', re.I)

BOX_INSIDE = (650, 208, 780, 224)       # inside the label box, below the map name's line
BOX_ROWS = (162, 228)                   # the box's inner rows (the labels' text lies between)
LABELS = (645, 160, 785, 232)           # where the two labels draw (0x6EC 648,167 135x16; 0x5A8 648,188 135x33)
START = [(486, 58, 37, 20), (486, 84, 37, 20)]   # the Start column's first two combos (0x6A3, 0x6A4)


def run(a, name, video, enhancements, window):
    work = os.path.join(a.work, name)
    shutil.rmtree(work, ignore_errors=True)
    writes = os.path.join(work, 'w')
    os.makedirs(writes)
    os.makedirs(os.path.join(work, 'shots'))
    with open(os.path.join(writes, 'RA2MD.INI'), 'w') as f:
        f.write(SETUP + '\n' + video)
    script = os.path.join(work, 'ui.script')
    with open(script, 'w') as f:
        f.write(SCRIPT)
    env = dict(os.environ, RA2_NATIVE_MOVIE_SKIP='1', RA2_NATIVE_IGNORE_FOCUS='1', RA2_NATIVE_MSG_LOG='0',
               RA2_NATIVE_SETTINGS=os.path.join(work, 'settings'), RA2_NATIVE_WINDOW_DUMP=os.path.join(work, 'shots', 'w'),
               RA2_NATIVE_WINDOW_DUMP_MS='100000000', RA2_NATIVE_WINDOW_DUMP_LOG='1')
    for k in ('RA2_NATIVE_ENHANCEMENTS', 'RA2_NATIVE_WINDOW_SIZE', 'RA2_NATIVE_TOUCH_UI', 'RA2_NATIVE_FORCE_END'):
        env.pop(k, None)
    if enhancements:
        env['RA2_NATIVE_ENHANCEMENTS'] = enhancements
    if window:
        env['RA2_NATIVE_WINDOW_SIZE'] = window
    cmd = [a.binary, '--data-dir', a.data_dir, '--run-to=never', '--run-seconds=%d' % a.seconds,
           '--write-dir=' + writes, '--input-script=' + script, '--game-args=-noaudio -CD.']
    with open(os.path.join(work, 'run.log'), 'w') as log:
        game_process.run(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=a.seconds + 120)
    text = open(os.path.join(work, 'run.log'), errors='replace').read()
    numbers = [int(n) for n in re.findall(r'^host: window dump (\d+)$', text, re.M) if int(n) > 0]
    shot = os.path.join(work, 'shots', 'w.%d.bmp' % numbers[-1]) if numbers else None
    return text, shot if shot and os.path.isfile(shot) else None


class Picture:
    """A dump read in the 800x600 menu's coordinates: the menu's place in the window from the host's layout
    line (the window's points scaled to the dump's pixels), else the whole dump."""

    def __init__(self, path, text, window):
        self.width, self.height, self.rows = read_bmp(path)
        self.x0, self.y0, self.scale = 0.0, 0.0, self.width / 800.0
        lines = LAYOUT.findall(text)
        if lines and window:
            pw, ph, x, y, w, h = (float(v) for v in lines[-1][:6])
            k = self.width / float(window.split('x')[0])
            self.scale = w / pw * k
            self.x0, self.y0 = x * k + (pw - 800) / 2 * self.scale, y * k + (ph - 600) / 2 * self.scale

    def pixels(self, box):
        """(x, y in menu coordinates, (r, g, b)) of every dump pixel inside a menu box (x0, y0, x1, y1)."""
        x0, y0, x1, y1 = box
        px0, py0 = int(self.x0 + x0 * self.scale + 0.5), int(self.y0 + y0 * self.scale + 0.5)
        px1, py1 = int(self.x0 + x1 * self.scale + 0.5), int(self.y0 + y1 * self.scale + 0.5)
        for py in range(max(0, py0), min(self.height, py1)):
            row = self.rows[py]
            for px in range(max(0, px0), min(self.width, px1)):
                yield (px - self.x0) / self.scale, (py - self.y0) / self.scale, row[px]


def yellow(p):
    r, g, b = p
    return r >= 150 and g >= 150 and b <= 110


def check(name, pic, failures):
    inside = list(pic.pixels(BOX_INSIDE))
    dark = sum(1 for _, _, p in inside if max(p) < 48) / max(1, len(inside))
    rows = [y for _, y, p in pic.pixels(LABELS) if yellow(p)]
    top, bottom = (min(rows), max(rows)) if rows else (None, None)
    print('%s: menu at %.0f,%.0f scale %.3f; label box dark %.2f; label text rows %s..%s' % (
        name, pic.x0, pic.y0, pic.scale, dark, '%.0f' % top if rows else '-', '%.0f' % bottom if rows else '-'))
    if dark < 0.8:
        failures.append('%s: no dark label box under the map labels (%.2f dark at %s; the plain column\'s grey slot?)'
                        % (name, dark, BOX_INSIDE))
    if not rows:
        failures.append('%s: no label text found' % name)
    elif top < BOX_ROWS[0] or bottom > BOX_ROWS[1]:
        failures.append('%s: the labels\' text (rows %.0f..%.0f) is not inside the label box (%d..%d)'
                        % (name, top, bottom, BOX_ROWS[0], BOX_ROWS[1]))
    for x, y, w, h in START:
        # The text part of the field: from its left to the arrow (the arrow's own art has no yellow).
        hits = [(px, py) for px, py, p in pic.pixels((x + 1, y + 1, x + w - 18, y + h - 1)) if yellow(p)]
        if not hits:
            failures.append('%s: no text in the Start combo at %d,%d' % (name, x, y))
            continue
        span = max(py for _, py in hits) - min(py for _, py in hits) + 1 / pic.scale
        across = max(px for px, _ in hits) - min(px for px, _ in hits) + 1 / pic.scale
        print('%s: Start combo at %d,%d: text %.1f rows x %.1f columns' % (name, x, y, span, across))
        if span > 12 or across < 13:
            failures.append('%s: the Start combo at %d,%d does not show "???" on one line (%.1f rows, %.1f columns)'
                            % (name, x, y, span, across))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--seconds', type=int, default=30)
    a = ap.parse_args()
    if not a.data_dir or not os.path.exists(os.path.join(a.data_dir, 'gamemd.exe')):
        print('skipped: no Yuri\'s Revenge install (gamemd.exe) in --data-dir')
        return 77
    a.work = os.path.abspath(a.work)
    failures = []
    for name, video, enhancements, window in RUNS:
        text, shot = run(a, name, video, enhancements, window)
        bad = sorted(set(l for l in text.splitlines() if BAD.search(l)))
        if bad:
            failures.append('%s: %s' % (name, bad[0]))
        if not shot:
            failures.append('%s: the skirmish setup was not dumped' % name)
            continue
        check(name, Picture(shot, text, window), failures)
    for f in failures:
        print('FAIL: ' + f)
    print('%s (%s)' % ('FAIL' if failures else 'OK', a.work))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
