#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Lane P12: the shell screens of the trial against the played port's, picture by picture.

Both binaries (--src: redalert2-decomp's src-based ra2-native, the port the user has played; --trial: this
repository's) are driven through the same tour of the shell with the same scripted clicks, each in a fresh
settings folder (HOME) and write folder, and the window is written at each settled screen (the input script's
"dump" lines, RA2_NATIVE_WINDOW_DUMP, logged with RA2_NATIVE_WINDOW_DUMP_LOG). The pictures of each screen are
then compared in 8x8 tiles: a tile differs when any pixel's colour channel differs by more than TOLERANCE. A
screen passes when every differing tile lies inside its ALLOWED rectangles (the differences listed below, each
with its reason) and at most SLACK of the remaining tiles differ.

  --size mac      the window at the default size (the 1024x768 game in an 800x600 window)
  --size iphone   iPhone 17 Pro landscape points (874x402) with the touch controls (RA2_NATIVE_TOUCH_UI=1,
                  the toolbar column) and the Mac's widescreen game mode for that window (1672x768)

The tour (dialog numbers as the shell shows them; each click waits for its dialog's slide-in, "sN+ms"):
main menu, Options, Enhancements, Options' Keyboard, Options' Network, Single Player, Load, Skirmish setup.
The campaign briefing and the score screen are not in the tour (they need a game started through movies or a
forced end: the rb-main-zoom-score check covers the score screen's layout).

Exit 0 when every screen passes, 1 when one fails, 77 without the game or the reference binary. The pictures
and a per-screen difference map (diff-<screen>.bmp: differing tiles white, allowed ones grey) stay in --work.

  rb_shell_frames.py --src <ra2-native> --trial <rb-native-trial> --data-dir <game folder> --game-exe <game.exe>
                     --work <dir> --size mac|iphone
"""
import argparse
import os
import re
import shutil
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'tests'))
import game_process  # noqa: E402

SKIP = 77
TOLERANCE = 24          # per channel: the two ports' smooth scaling may round a level apart
SLACK = 0.002           # of a screen's tiles outside the allowed rectangles

# The shell's column buttons at 1024x768 (main's tests: the main menu's Single Player 304, Network 388, Options
# 472; the Options column Keyboard 304, Network 346, Enhancements 388, the bottom slot 639; Single Player's Load 346
# and Skirmish 388). With the widescreen mode the 800x600 panel is centred in 1672 columns: 324 further right.
TOUR = [
    # (screen name or None, the dialog it waits for, the click after it, x, y)
    ('main-menu', 1, 834, 472),
    ('options', 2, 834, 388),
    ('enhancements', 3, 834, 639),
    (None, 4, 834, 304),
    ('keyboard', 5, 834, 639),
    (None, 6, 834, 346),
    ('network', 7, 834, 639),
    (None, 8, 834, 639),
    (None, 9, 834, 304),
    ('single-player', 10, 834, 346),
    ('load', 11, 834, 639),
    (None, 12, 834, 388),
    ('skirmish', 13, 834, 639),
]
SETTLE_MS = 2500        # after the dialog is shown and slid in: the dump, then the click 500 ms later

# Allowed differences, in fractions of the picture (x, y, w, h) so they hold at both sizes, by screen ('*': all).
# Each says why it is allowed; anything else that differs fails the screen.
ALLOWED = {
    '*': [
        # The column's monitor (top right): a looping movie, never at the same frame in two runs.
        ((0.72, 0.10, 0.17, 0.27), 'the column monitor movie plays on a wall clock'),
        # The column buttons' lights pulse on a wall clock.
        ((0.72, 0.36, 0.17, 0.56), 'the column buttons light on a wall clock'),
    ],
    'enhancements': [
        # The dropdowns' rows: the trial follows the original's item heights (Combo_Proc's OD_SUBCLASSED: the field
        # LineHeight + 2, the rows LineHeight + 6); a played port built before main's lane N33 keeps user32's 16.
        ((0.38, 0.22, 0.24, 0.60), 'dropdown heights follow the original (deliberate; main lane N33)'),
    ],
    'options': [
        ((0.42, 0.25, 0.22, 0.08), 'the resolution dropdown follows the original item heights (deliberate)'),
    ],
    'skirmish': [
        ((0.10, 0.10, 0.62, 0.80), 'the setup dropdowns follow the original item heights (deliberate); the map preview'),
    ],
    'keyboard': [
        # List_Proc's OD_SUBCLASSED sets the rows to LineHeight + 2 (LB_SETITEMHEIGHT -1: 0x005F6B34-0x005F6B53); main's
        # behaviour row keeps user32's height, so the commands list's rows sit a little further apart in the trial.
        ((0.36, 0.25, 0.30, 0.27), 'list rows follow the original item height (0x005F6B4D); main keeps user32\'s'),
    ],
    'load': [
        # The same row height; the original fills the selected row (AlternateSurface Fill_Rect after LB_GETSEL,
        # 0x005F6DA6-0x005F6E08) where main's behaviour row outlines it; and the trial draws the date and time
        # columns Fill_List sets (OD_SETCELL at 255 and 315), which main's row does not draw.
        ((0.20, 0.27, 0.44, 0.45), 'the load list: original row height, selection fill, date/time columns'),
    ],
    'network': [
        ((0.10, 0.10, 0.62, 0.40), 'the network dropdowns follow the original item heights (deliberate)'),
    ],
}


def read_bmp(path):
    b = open(path, 'rb').read()
    off = struct.unpack_from('<I', b, 10)[0]
    w, h = struct.unpack_from('<ii', b, 18)
    bpp = struct.unpack_from('<H', b, 28)[0] // 8
    rows = abs(h)
    stride = ((w * bpp + 3) // 4) * 4
    def px(x, y):
        line = (rows - 1 - y) if h > 0 else y
        i = off + line * stride + x * bpp
        return b[i], b[i + 1], b[i + 2]
    return w, rows, px


def write_map(path, w, h, tiles, cols):
    stride = ((w * 3 + 3) // 4) * 4
    data = bytearray(stride * h)
    for (tx, ty), v in tiles.items():
        c = 255 if v == 2 else 110
        for y in range(ty * 8, min(h, ty * 8 + 8)):
            line = (h - 1 - y) * stride
            for x in range(tx * 8, min(w, tx * 8 + 8)):
                data[line + x * 3:line + x * 3 + 3] = bytes((c, c, c))
    head = struct.pack('<2sIHHI', b'BM', 54 + len(data), 0, 0, 54) + struct.pack('<IiiHHIIiiII', 40, w, h, 1, 24, 0,
                                                                                   len(data), 2835, 2835, 0, 0)
    with open(path, 'wb') as f:
        f.write(head + data)


def compare(name, a, b, out_map):
    wa, ha, pa = read_bmp(a)
    wb, hb, pb = read_bmp(b)
    if (wa, ha) != (wb, hb):
        return False, 'sizes differ (%dx%d against %dx%d)' % (wa, ha, wb, hb)
    rects = ALLOWED['*'] + ALLOWED.get(name, [])
    allowed_px = [(int(x * wa), int(y * ha), int((x + w) * wa), int((y + h) * ha)) for (x, y, w, h), _ in rects]
    cols, rows = (wa + 7) // 8, (ha + 7) // 8
    tiles = {}
    bad = 0
    total = 0
    for ty in range(rows):
        for tx in range(cols):
            cx, cy = tx * 8 + 4, ty * 8 + 4
            inside = any(x0 <= cx < x1 and y0 <= cy < y1 for x0, y0, x1, y1 in allowed_px)
            if not inside:
                total += 1
            differ = False
            for y in range(ty * 8, min(ha, ty * 8 + 8), 2):
                for x in range(tx * 8, min(wa, tx * 8 + 8), 2):
                    p, q = pa(x, y), pb(x, y)
                    if abs(p[0] - q[0]) > TOLERANCE or abs(p[1] - q[1]) > TOLERANCE or abs(p[2] - q[2]) > TOLERANCE:
                        differ = True
                        break
                if differ:
                    break
            if differ:
                tiles[(tx, ty)] = 1 if inside else 2
                if not inside:
                    bad += 1
    write_map(out_map, wa, ha, tiles, cols)
    share = bad / max(total, 1)
    return share <= SLACK, '%d of %d tiles differ outside the allowed areas (%.2f%%)' % (bad, total, 100 * share)


def run(a, tag, binary):
    work = os.path.join(a.work, tag)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(os.path.join(work, 'home'))
    os.makedirs(os.path.join(work, 'write'))
    dx = 324 if a.size == 'iphone' else 0
    lines = []
    for name, dialog, x, y in TOUR:
        if name:
            lines.append('s%d+%d dump' % (dialog, SETTLE_MS))
        lines.append('s%d+%d click %d %d' % (dialog, SETTLE_MS + 500, x + dx, y))
    lines.append('s%d+%d quit' % (TOUR[-1][1] + 1, 2000))
    script = os.path.join(work, 'tour.script')
    with open(script, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    env = game_process.game_env()
    for k in ('RA2_NATIVE_ENHANCEMENTS', 'RA2_NATIVE_SETUP', 'RA2_DATA_DIR', 'RA2_NATIVE_TOUCH_UI', 'RA2_NATIVE_WINDOW_SIZE',
              'RA2_NATIVE_WIDESCREEN'):
        env.pop(k, None)
    env.update({'HOME': os.path.join(work, 'home'), 'RA2_GAME_EXE': a.game_exe, 'RA2_NATIVE_LOG_STDERR': '1',
                'RA2_NATIVE_MOVIE_SKIP': '1', 'RA2_NATIVE_IGNORE_FOCUS': '1',
                'RA2_NATIVE_WINDOW_DUMP': os.path.join(work, 'w'), 'RA2_NATIVE_WINDOW_DUMP_MS': '100000000',
                'RA2_NATIVE_WINDOW_DUMP_LOG': '1'})
    if a.size == 'iphone':
        env.update({'RA2_NATIVE_TOUCH_UI': '1', 'RA2_NATIVE_WINDOW_SIZE': '874x402'})
    cmd = [binary, '--app', '--data-dir', a.data_dir, '--input-script=' + script, '--write-dir=' + os.path.join(work, 'write'),
           '--game-args=-noaudio']
    try:
        p = game_process.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace',
                             timeout=a.seconds)
        out = p.stdout
    except subprocess.TimeoutExpired as x:
        out = x.stdout.decode('utf-8', 'replace') if isinstance(x.stdout, bytes) else (x.stdout or '')
        out += '\nrb_shell_frames: time limit\n'
    with open(os.path.join(work, 'run.log'), 'w') as f:
        f.write(out)
    # Dump 0 is the first picture; the scripted dumps follow in the tour's order.
    shots = {}
    names = [n for n, _, _, _ in TOUR if n]
    numbers = [int(m) for m in re.findall(r'^host: window dump (\d+)$', out, re.M)]
    numbers = [n for n in numbers if n > 0]
    for name, n in zip(names, numbers):
        path = os.path.join(work, 'w.%d.bmp' % n)
        if os.path.isfile(path):
            shots[name] = path
    return shots, out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--src', default='')
    ap.add_argument('--trial', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--game-exe', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--size', choices=['mac', 'iphone'], default='mac')
    ap.add_argument('--seconds', type=int, default=240)
    a = ap.parse_args()
    if not a.data_dir or not os.path.isdir(a.data_dir):
        print('rb-shell-frames: no installed game given; skipped')
        return SKIP
    if not a.game_exe:
        a.game_exe = os.path.join(a.data_dir, 'game.exe')
    if not a.src or not os.path.isfile(a.src):
        print('rb-shell-frames: no reference binary (--src); skipped')
        return SKIP
    a.work = os.path.abspath(a.work)
    src, src_log = run(a, 'src', a.src)
    trial, trial_log = run(a, 'trial', a.trial)
    failed = []
    for name in [n for n, _, _, _ in TOUR if n]:
        if name not in src or name not in trial:
            failed.append(name)
            print('rb-shell-frames: %-14s FAILED: not reached (src %s, trial %s)' % (name, name in src, name in trial))
            continue
        ok, what = compare(name, src[name], trial[name], os.path.join(a.work, 'diff-%s.bmp' % name))
        print('rb-shell-frames: %-14s %s: %s' % (name, 'ok' if ok else 'FAILED', what))
        if not ok:
            failed.append(name)
    for tag, log in (('src', src_log), ('trial', trial_log)):
        if re.search(r'signal \d+|SIG(SEGV|BUS|ILL|ABRT)|crash:', log):
            failed.append(tag + ' fault')
            print('rb-shell-frames: a fault in the %s run' % tag)
    print('rb-shell-frames (%s): %s (pictures in %s)' % (a.size, 'ok' if not failed else 'FAILED: ' + ', '.join(failed), a.work))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
