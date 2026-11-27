#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctest ra2-native-window-resize / yr-native-window-resize (lane P48): the map after the window's size changes in play.

codex1's Windows recheck (r3570): maximise a skirmish, restore it, and the map went black (the sidebar and the
Escape menu still worked); minimise and restore did the same in Yuri's Revenge, and a save made afterwards loaded with
the black map. The cause was the pointer, not the drawing: crossing the title bar to the window's controls (or to the
taskbar) it leaves the window, and the game's cursor stayed where its last motion put it, on the picture's edge, so
ScrollClass::Scroll_Edge scrolled the map into its unrevealed corner every frame until the pointer came back (the
view saved with it). platform/sdl/host.cpp hold_cursor_off_edges now holds the cursor one pixel inside the edges when
the pointer leaves or the window is minimised.

Each run plays the player's way through SDL (the input script's maximize / restore / minimize / window W H, and leave:
SDL's mouse-leave) with the game's cursor put on an edge first (where the pointer last was, crossing to the window's
controls). Checks, per step: the map area of the window is not black (its lit pixels at least half the first
picture's) and, in Red Alert 2 (the step server's state), the view did not move (more than a scroll step).

  ra2: Allied 1 through the step server (ra2/tools/ra2_step.py), past its locked opening; maximise with the cursor on
       the top right edge, restore; the cursor on the bottom edge, minimise, restore; a smaller window and back; then a
       save, loaded in a fresh run, whose map must show too.
  yr:  a skirmish through the menus (yr/tests/yr_skirmish_smoke.py's clicks), the same steps by a timed input script.

Skipped (77) without the installed game. POSIX (the step server's socket, for ra2).

  window_resize.py --game ra2|yr --binary <ra2-native|yr-native> --data-dir <game folder> --work <dir>
"""
import argparse
import glob
import os
import re
import shutil
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
STEP = os.path.join(ROOT, 'ra2', 'tools', 'ra2_step.py')
sys.path.insert(0, HERE)
import game_process  # noqa: E402

SKIP = 77


def lit_fraction(path):
    """The share of lit pixels in the map area of a window dump (32-bit BMP): its left 70% and top 85%, clear of the
    sidebar column and the command bar at any window size."""
    with open(path, 'rb') as f:
        d = f.read()
    if d[:2] != b'BM':
        raise SystemExit('window_resize: %s is not a BMP' % path)
    offset = struct.unpack('<I', d[10:14])[0]
    w, h, _, bpp = struct.unpack('<iiHH', d[18:30])
    if bpp != 32:
        raise SystemExit('window_resize: %s has %d bits a pixel' % (path, bpp))
    top_down = h < 0
    h = abs(h)
    lit = total = 0
    for y in range(0, int(h * 0.85), 4):
        row = y if top_down else h - 1 - y
        base = offset + row * w * 4
        for x in range(0, int(w * 0.7), 4):
            p = d[base + x * 4:base + x * 4 + 3]
            total += 1
            if max(p) > 24:
                lit += 1
    return lit / total if total else 0.0


def check_pictures(shots, bad):
    """shots: [(label, path)], the first the reference."""
    ref = lit_fraction(shots[0][1])
    print('window_resize: %s: %.2f of the map area lit' % (shots[0][0], ref))
    if ref < 0.2:
        bad.append('the first picture (%s) has too little map to compare (%.2f lit)' % (shots[0][0], ref))
        return
    for label, path in shots[1:]:
        f = lit_fraction(path)
        print('window_resize: %s: %.2f of the map area lit' % (label, f))
        if f < ref * 0.5:
            bad.append('%s: the map area is black (%.2f lit, %.2f before; %s)' % (label, f, ref, path))


def faults(log, bad):
    with open(log, errors='replace') as f:
        text = f.read()
    hit = re.search(r'unrecovered|\btrap\b|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:|assert', text, re.I)
    if hit:
        bad.append('fault or trap: %s' % text[hit.start():hit.start() + 160].splitlines()[0])
    return text


def run_ra2(a, work, bad):
    py = sys.executable
    start = [py, '-B', STEP, 'start', '--binary', a.binary, '--data-dir', a.data_dir, '--work', work, '--side', 'allied',
             '--mission', '1', '--seconds', '900']
    if a.game_exe:
        start += ['--game-exe', a.game_exe]
    r = subprocess.run(start, capture_output=True, text=True, timeout=400)
    print(r.stdout.strip())
    if r.returncode != 0:
        bad.append('the step server did not start (%s)' % os.path.join(work, 'game.log'))
        return

    def send(*commands):
        out = subprocess.run([py, '-B', STEP, 'send', '--work', work] + list(commands), capture_output=True, text=True,
                             timeout=600).stdout
        return out

    def view():
        m = re.search(r'^view (-?\d+),(-?\d+) ', send('state'), re.M)
        return (int(m.group(1)), int(m.group(2))) if m else None

    shots = []

    def shot(label):
        path = os.path.join(work, '%d-%s.bmp' % (len(shots), label))
        send('advance 2', 'screenshot ' + path)
        if os.path.isfile(path):
            shots.append((label, path))
        else:
            bad.append('%s: no screenshot' % label)

    try:
        # Allied 1's opening locks the player's input (no edge scroll then): played on past it.
        send('input +0 move 500 400', 'advance 2100')
        if 'input open' not in send('state'):
            bad.append('Allied 1 still locks input at tick 2100')
            return
        v0 = view()
        shot('before')
        steps = [
            ('maximised, restored from the top right', ['input +0 maximize', 'advance 10', 'input +0 move 1023 0',
                                                        'input +1 leave', 'advance 5', 'input +0 restore', 'advance 300']),
            ('minimised, restored from the bottom', ['input +0 move 512 767', 'input +1 leave', 'advance 3',
                                                    'input +0 minimize', 'advance 100', 'input +0 restore', 'advance 300']),
            ('a smaller window and back from the left', ['input +0 move 0 400', 'input +1 leave', 'advance 3',
                                                         'input +0 window 700 520', 'advance 100',
                                                         'input +0 window 1024 768', 'advance 300']),
        ]
        for label, commands in steps:
            out = send(*commands)
            if 'error' in out:
                bad.append('%s: %s' % (label, [l for l in out.splitlines() if 'error' in l][:2]))
            v = view()
            # (the cursor rests on the edge for the frame before the pointer leaves: a step of the scroll at most;
            # unheld, 300 frames scrolled the view about 1,000 pixels)
            if not v or not v0 or abs(v[0] - v0[0]) > 32 or abs(v[1] - v0[1]) > 32:
                bad.append('%s: the view moved from %s to %s (edge scroll while the pointer was away)' % (label, v0, v))
            shot(label)
        out = send('input +0 move 500 400', 'advance 5', 'save P48RESIZ.SAV')
        if 'saved' not in out:
            bad.append('the save after the steps failed: %s' % out.strip()[-200:])
    finally:
        subprocess.run([py, '-B', STEP, 'stop', '--work', work], capture_output=True, timeout=60)
    faults(os.path.join(work, 'game.log'), bad)
    check_pictures(shots, bad)

    # The save in a fresh run (Load Saved Game through the menus, --load-save): its map shows.
    write = os.path.join(work, 'w')
    if not os.path.isfile(os.path.join(write, 'P48RESIZ.SAV')):
        bad.append('no P48RESIZ.SAV in %s' % write)
        return
    script = os.path.join(work, 'load.script')
    with open(script, 'w') as f:
        f.write('1 move 500 400\n')     # the pointer over the map, as a player's (it starts at the top left corner)
    dumps = os.path.join(work, 'load')
    env = dict(os.environ, RA2_NATIVE_WINDOW_DUMP=dumps, RA2_NATIVE_WINDOW_DUMP_MS='2000', RA2_NATIVE_MOVIE_SKIP='1',
               RA2_NATIVE_IGNORE_FOCUS='1', RA2_GAME_EXE=a.game_exe or os.path.join(a.data_dir, 'game.exe'))
    cmd = [a.binary, '--data-dir', a.data_dir, '--run-to=never', '--run-seconds=45', '--write-dir=' + write,
           '--input-script=' + script, '--load-save=P48RESIZ.SAV', '--game-args=-noaudio -CD.']
    log = os.path.join(work, 'load.log')
    with open(log, 'w') as f:
        game_process.run(cmd, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=200)
    text = faults(log, bad)
    if 'Loading' not in text and 'load' not in text.lower():
        bad.append('the load run shows no load (%s)' % log)
    last = sorted(glob.glob(dumps + '.*.bmp'), key=lambda p: int(p.rsplit('.', 2)[1]))
    if not last:
        bad.append('the load run presented nothing (%s)' % log)
        return
    f = lit_fraction(last[-1])
    print('window_resize: the save loaded: %.2f of the map area lit (%s)' % (f, last[-1]))
    if f < 0.2:
        bad.append('the save made after the steps loads with a black map (%.2f lit; %s)' % (f, last[-1]))


# Yuri's Revenge: yr_skirmish_smoke.py's menu clicks (800x600), then the steps by time from the setup's settling.
YR_SCRIPT = """\
s1+1000 click 720 220
s2+1000 click 720 304
s3+2000 click 720 262
s3+3000 move 316 300
s3+14000 dump
s3+15000 maximize
s3+16000 move 799 0
s3+16100 leave
s3+16500 restore
s3+21000 dump
s3+22000 move 400 599
s3+22100 leave
s3+22500 minimize
s3+24000 restore
s3+29000 dump
s3+30000 move 0 300
s3+30100 leave
s3+30500 window 700 520
s3+32000 window 800 600
s3+37000 dump
"""
YR_LABELS = ['before', 'maximised, restored from the top right', 'minimised, restored from the bottom',
             'a smaller window and back from the left']


def run_yr(a, work, bad):
    os.makedirs(os.path.join(work, 'w'))
    script = os.path.join(work, 'resize.script')
    with open(script, 'w') as f:
        f.write(YR_SCRIPT)
    dumps = os.path.join(work, 'win')
    env = dict(os.environ, RA2_NATIVE_WINDOW_DUMP=dumps, RA2_NATIVE_WINDOW_DUMP_MS='100000000', RA2_NATIVE_MOVIE_SKIP='1',
               RA2_NATIVE_IGNORE_FOCUS='1', RA2_NATIVE_DIALOG_LOG='1')
    cmd = [a.binary, '--data-dir', a.data_dir, '--run-to=never', '--run-seconds=75', '--write-dir=' + os.path.join(work, 'w'),
           '--input-script=' + script, '--game-args=-noaudio -CD.']
    log = os.path.join(work, 'run.log')
    with open(log, 'w') as f:
        game_process.run(cmd, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=260)
    text = faults(log, bad)
    numbers = [int(n) for n in re.findall(r'^host: window dump (\d+)$', text, re.M)]
    if len(numbers) != len(YR_LABELS):
        bad.append('%d scripted pictures, %d expected (%s)' % (len(numbers), len(YR_LABELS), log))
        return
    check_pictures([(label, '%s.%d.bmp' % (dumps, n)) for label, n in zip(YR_LABELS, numbers)], bad)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--game', choices=('ra2', 'yr'), required=True)
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--game-exe', default='')
    ap.add_argument('--work', required=True)
    a = ap.parse_args()
    exe = 'game.exe' if a.game == 'ra2' else 'gamemd.exe'
    if not a.data_dir or not os.path.isfile(os.path.join(a.data_dir, exe)):
        print('window_resize: skipped: no installed game (%s) in --data-dir' % exe)
        return SKIP
    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    bad = []
    (run_ra2 if a.game == 'ra2' else run_yr)(a, work, bad)
    for b in bad:
        print('window_resize: FAIL: %s' % b)
    print('window_resize: %s (%s)' % ('FAIL' if bad else 'OK', work))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
