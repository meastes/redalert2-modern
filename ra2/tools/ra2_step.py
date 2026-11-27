#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The step server's launcher and client (lane P21; ra2/glue/stepserver_native.inc).

  start   the Mac build in the background, a campaign mission reached through the real menus (Single Player, New
          Campaign, the side's emblem; missions before it won by RA2_NATIVE_FORCE_END, as tests/mission_soak.py), the
          game paused at the mission's first frame and served on <work>/step.sock; every command that changes the game
          recorded in <work>/step.replay ("<tick> <command>": inputs and ticks only)
            ra2_step.py start --binary build/mac/ra2-native --data-dir <game> --work <dir> --side allied --mission 1
          --game yr (lane P50): Yuri's Revenge's (yr-native, its step server yr/glue/yr_stepserver.cpp; the 800x600 menus,
          RA2MD.INI, seven missions a side)
  send    commands to the running game, each reply printed (one connection; commands in order)
            ra2_step.py send --work <dir> "advance 300" "state"
  stop    quit the game (the step server's quit), else end the process started

The game runs at game speed 0, uncapped, with the gate's fixed seed (RA2_NATIVE_SOAK_SEED, _SPEED), and only scripted
input reaches it (RA2_NATIVE_SCRIPT_ONLY), so a recorded run replays the same (ra2/tests/ra2_campaign.py). The game's
clock runs one 16 ms tick a loop frame (RA2_NATIVE_FRAME_CLOCK, lane P33; the recording says "# frame clock"), so what
the game paces by its clock (speech, the radar's movies, a trigger's pause) lasts the same number of ticks in every run;
--clock wall gives the wall clock of recordings made before it (a preload without the line gets it by default).
"""
import argparse
import os
import shutil
import signal
import socket
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'tests'))
import mission_soak  # noqa: E402  (the menus' points, the seed)


def uses_frame_clock(path):
    """A recording made with the frame clock (the step server's "# frame clock" line)."""
    with open(path, errors='replace') as f:
        return any(l.startswith('# frame clock') for l in f)


def game_of(a):
    """'ra2' or 'yr' (lane P50: --game)."""
    return getattr(a, 'game', None) or 'ra2'


def campaign_env(a, work, write):
    env = dict(os.environ, RA2_NATIVE_MOVIE_SKIP='1', RA2_NATIVE_IGNORE_FOCUS='1', RA2_NATIVE_MSG_LOG='0',
               RA2_NATIVE_CRASH_TRACE='1', RA2_NATIVE_UNCAPPED='1', RA2_NATIVE_SOAK_SEED=str(mission_soak.SEED),
               RA2_NATIVE_SOAK_SPEED='0', RA2_NATIVE_SCRIPT_ONLY='1', RA2_NATIVE_DIALOG_LOG='1',
               RA2_NATIVE_STEP_MISSION=str(a.mission), RA2_NATIVE_ENHANCEMENTS='zoomgestures=off',
               )
    if game_of(a) == 'ra2':
        env['RA2_GAME_EXE'] = a.game_exe or os.path.join(a.data_dir, 'game.exe')
    for k in ('RA2_NATIVE_ORDERS', 'RA2_NATIVE_TRAP_CONTINUE', 'RA2_NATIVE_FORCE_END', 'RA2_NATIVE_SOAK_DUMP', 'RA2_NATIVE_SOAK',
              'RA2_NATIVE_SOAK_END', 'RA2_NATIVE_STEP', 'RA2_NATIVE_STEP_REPLAY', 'RA2_NATIVE_STEP_RECORD', 'RA2_NATIVE_SAVE_AT',
              'RA2_NATIVE_FRAME_CLOCK'):
        env.pop(k, None)
    if a.mission > 1:
        env['RA2_NATIVE_FORCE_END'] = ','.join(str(100 + 200 * k) for k in range(a.mission - 1))
    return env


def prepare(a, work):
    """A fresh write folder with the installation's RA2.INI (its 1024x768 screen, which the menu points assume) and the
    menu script; returns (write folder, script path)."""
    write = os.path.join(work, 'w')
    shutil.rmtree(write, ignore_errors=True)
    os.makedirs(write)
    game = game_of(a)
    with open(os.path.join(write, mission_soak.GAMES[game]['settings']), 'w') as f:
        f.write(mission_soak.settings(argparse.Namespace(game=game, data_dir=a.data_dir)))
    script = os.path.join(work, 'menus.script')
    with open(script, 'w') as f:
        f.write(mission_soak.script(game, a.side, a.mission))
    return write, script


def cmd_start(a):
    work = os.path.abspath(a.work)
    os.makedirs(work, exist_ok=True)
    write, script = prepare(a, work)
    sock = os.path.join(work, 'step.sock')
    env = campaign_env(a, work, write)
    env['RA2_NATIVE_STEP'] = sock
    frame_clock = a.clock == 'frame' or (a.clock == 'auto' and (not a.preload or uses_frame_clock(a.preload)))
    if frame_clock:
        env['RA2_NATIVE_FRAME_CLOCK'] = '1'
    env['RA2_NATIVE_STEP_RECORD'] = os.path.join(work, 'step.replay')
    if a.preload:
        # The recorded run resumed: its commands rerun first (and are recorded again in a fresh step.replay).
        pre = os.path.join(work, 'preload.replay')
        with open(a.preload) as f:
            lines = [l for l in f if l.strip() and (a.until < 0 or l.startswith('#') or int(l.split()[0]) <= a.until)]
        with open(pre, 'w') as f:
            f.writelines(lines)
        env['RA2_NATIVE_STEP_PRELOAD'] = pre
    if os.path.exists(env['RA2_NATIVE_STEP_RECORD']):
        os.replace(env['RA2_NATIVE_STEP_RECORD'], env['RA2_NATIVE_STEP_RECORD'] + '.prev')
    cmd = [a.binary, '--data-dir', a.data_dir, '--run-to=never', '--run-seconds=%d' % a.seconds, '--write-dir=' + write,
           '--input-script=' + script, '--game-args=-noaudio -CD.']
    log = open(os.path.join(work, 'game.log'), 'w')
    p = subprocess.Popen(cmd, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    with open(os.path.join(work, 'game.pid'), 'w') as f:
        f.write(str(p.pid))
    deadline = time.time() + 240
    while time.time() < deadline:
        if p.poll() is not None:
            print('start: the game ended (%d) before serving: %s' % (p.returncode, os.path.join(work, 'game.log')))
            return 1
        with open(os.path.join(work, 'game.log'), errors='replace') as f:
            if 'step: ' in f.read() and os.path.exists(sock):
                try:
                    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    s.connect(sock)
                    s.close()
                    print('start: pid %d serving %s' % (p.pid, sock))
                    return 0
                except OSError:
                    pass
        time.sleep(1)
    print('start: no server within 240 s (%s)' % os.path.join(work, 'game.log'))
    return 1


def send(work, commands, timeout=3600):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect(os.path.join(work, 'step.sock'))
    f = s.makefile('rw', encoding='utf-8', errors='replace', newline='\n')
    replies = []
    for c in commands:
        f.write(c + '\n')
        f.flush()
        lines = []
        while True:
            line = f.readline()
            if not line:
                lines.append('(connection closed)')
                break
            line = line.rstrip('\n')
            if line == '.':
                break
            lines.append(line)
        replies.append(lines)
    s.close()
    return replies


def cmd_send(a):
    work = os.path.abspath(a.work)
    for c, lines in zip(a.commands, send(work, a.commands)):
        print('> ' + c)
        for line in lines:
            print(line)
    return 0


def cmd_stop(a):
    work = os.path.abspath(a.work)
    try:
        send(work, ['quit'], timeout=20)
    except OSError:
        pass
    pid_file = os.path.join(work, 'game.pid')
    if os.path.exists(pid_file):
        pid = int(open(pid_file).read().strip())
        time.sleep(1)
        try:
            os.kill(pid, signal.SIGTERM)
        except OSError:
            pass
        os.remove(pid_file)
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='what', required=True)
    s = sub.add_parser('start')
    s.add_argument('--game', default='ra2', choices=['ra2', 'yr'], help="the game: ra2 (ra2-native) or yr (yr-native)")
    s.add_argument('--binary', required=True)
    s.add_argument('--data-dir', required=True)
    s.add_argument('--game-exe', default='')
    s.add_argument('--work', required=True)
    s.add_argument('--side', default='allied', choices=['allied', 'soviet'])
    s.add_argument('--mission', type=int, default=1)
    s.add_argument('--seconds', type=int, default=86400)
    s.add_argument('--preload', default='', help='a replay file whose commands run first; the server serves from its end')
    s.add_argument('--until', type=int, default=-1, help='with --preload: only its lines up to this tick')
    s.add_argument('--clock', default='auto', choices=['auto', 'frame', 'wall'],
                   help="the game's clock: frame (16 ms a loop frame), wall; auto: frame unless the preload was recorded "
                        "without it")
    c = sub.add_parser('send')
    c.add_argument('--work', required=True)
    c.add_argument('commands', nargs='+')
    t = sub.add_parser('stop')
    t.add_argument('--work', required=True)
    a = ap.parse_args()
    return {'start': cmd_start, 'send': cmd_send, 'stop': cmd_stop}[a.what](a)


if __name__ == '__main__':
    sys.exit(main())
