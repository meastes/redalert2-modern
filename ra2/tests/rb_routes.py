#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ctests rb-native-skirmish, rb-native-soak, rb-native-exit-route (lane P3, branch ports-on-rebuild):
the rebuild-based trial (native/rb, rb-native-trial) through the real menus with scripted input,
reading the user's installed game at run time, each run in a fresh write folder under --work.
Modelled on yr/tests/yr_skirmish_smoke.py and tests/exit_route.py.

  skirmish  main menu -> Single Player -> Skirmish -> Start Game (the setup's defaults) -> about 75 s
            of the game. Checks: the time limit ends the run; the game loop ran (frame milestone,
            at least 3,000 frames at the native frame floor's 62 a second).
  soak      the same start, then 420 s of an idle player: the computer wins, Do_Lose shows the
            skirmish score screen. Checks: the time limit ends the run; at least 6,000 frames (the count stops at the score screen, whose time varies: 12,339 to 16,918 seen);
            the score screen (its 50-control dialog) shown. Lane P4: a band selection of the player's units
            at frame 300 (their brackets and health bars drawn), and after the score screen its Continue,
            the setup's Back and Single Player's Back: the main menu shown again (menu-return).
  exit      the same start, then at frame 600 Escape -> Abort Mission -> Quit -> the skirmish setup's
            Back -> Single Player's Back -> Exit Game -> OK. Checks: the main menu shown again
            (menu-return), WinMain returning 0 and the process exiting 0.
  audio     the skirmish route without -noaudio (lane P3: the sound system up, the movies played).
            Checks as skirmish, and from the port's traces (RA2_PORT_TRACE, RA2_NATIVE_AUDIO_LOG):
            the start-up movie opened with its sound track (Bink, read through the user's
            BINKW32.DLL tables), the mixer's output not silent before the menu, and in the game at
            least three sound buffers playing with a nonzero peak.

  save      the skirmish route with a harness save at loop frame 600 (RA2_NATIVE_SAVE_AT, the Main_Loop
            seam: Save_Game, as the options menu's Save), then a new process that loads it at start
            (--load-save: the port's menus take Single Player and Load Saved Game, the load dialog
            loads the file) and plays on. Checks: the save written; the load reported; the game
            loop reached again; the time limit ends the second run; at least 900 frames (the save's game speed). Since lane P4
            the save is written in the original's format (a compound file, the field maps of
            rb_original_layouts.py) and read back through them; the port's container is not used.
  original  one of the user's saves from the original game (--save, default ALL01T.sav, Allied 1), read in
            place from --data-dir at run time and never copied or written (main's
            tests/original_save_load.py route): --load-save, then play on. Checks: opened as an original
            (compound file); loaded; objects converted by the field maps; no type read without a map;
            the game loop reached; the time limit ends the run; at least 900 frames.
  resave    the original route's save loaded, a harness save at loop frame 300 (written in the original's
            format), then a new process loads that save and plays on (main's original_save_write.py, one save).
            Checks as save.

  combat    (lane P5) the skirmish start with harness orders (COMBAT_ORDERS, glue/harness_native.inc): power
            plants, Prism towers and a selected Gap Generator for the player, a Prism tank sent at an enemy
            Rhino. Checks: each placed; the Gap Generator selected; the radial indicator drawn
            (XSurface::Draw_Ellipse) with its sweep (DSurface::Draw_Depth_Antialiased_Line); laser beams seen;
            the Rhino killed; at least 1,920 frames.
  campaign  (lane P6) one of the user's Allied campaign saves (--save ALL02S.sav or ALL03U.sav; CAMPAIGN below), read
            in place, loaded and played on with the trigger watch (RA2_NATIVE_TRIGGERS, glue/harness_native.inc) and
            harness orders (the player's units hunt at frames 1,200 and 2,400). Checks: the save loaded as an original;
            each of the mission's scripted triggers named in CAMPAIGN fired (left the trigger list: a reinforcement
            arriving, a house change, a hint, an enemy attack team sent); the hunt order given; the game loop and the
            time limit; no trap or fault. ALL03U's route found the zero-storage VoxelAnims (glue/vanim_native.inc).
  production (lane P6) the skirmish start with harness orders (PRODUCTION_ORDERS): power plants, a barracks and a war
            factory placed for the player; a GI and a Grizzly ordered through the factories (PRODUCE events, the cameo
            clicks); the nuclear missile (the player's SuperWeapon 0, MultiSpecial) granted, charged and launched at the
            strongest enemy house's first building. Checks: each placed; one more GI and one more Grizzly at frame 1,500
            than at 290 and their price taken (money 9,100 from 10,000); the missile launched and its target damaged
            (a harness-watch strength below the one it had); at least 1,920 frames.
  combatsave (lane P5) the combat route saved at frame 260 (attack under way), then loaded and played on;
            checks as save. `resave --save <file>` takes any of the user's saves.

  allied1   (lane P16, the user's Duo report 2026-11-24: "a random failure in the middle" of Allied 1 with Tanya alive) the
            user's ALL01T.sav (Allied 1's start), Tanya walked north (harness `goto TANY 62 80`) and sent at the four
            Dreadnoughts (`fire TANY DRED`, her C4) with the trigger watch. Checks: Objective 1 Complete fired (every BadGuy2
            Dreadnought gone); no loss in the run (no Mission Failed, no lose flag, no game-over) past Dreadnought Retreat.
  allied1idle (lane P16) the same walk north, then Tanya left idle: the map's own failure, as the src-based port and the
            original play it. Two minutes (1,800 frames) after the Statue movie, Enable Retreat sends the Dreadnoughts out
            (Dreadnought Retreat), they cross Enable Failure 3's cell line, are deleted at the map edge (Enable Failure 3b:
            BadGuy2 all destroyed with Objective 1 Complete disabled) and Mission Failed makes the player lose. Checks: the
            loss comes from that chain in order, after Dreadnought Retreat, and from nothing else.
  forcewin, forcelose  (lane P7) the skirmish start, then RA2_NATIVE_FORCE_END=300[,lose] (glue/mainloop_native.inc,
            main's harness_save.cpp: PlayerWins or PlayerLoses set at loop frame 300, as the house AI sets them), the
            score screen's Continue, the setup's Back and Single Player's Back. Checks: the force-end line; the skirmish
            score screen (its 50-control dialog) shown; the main menu shown again (menu-return).
  chain     (lane P7, main's tests/campaign_chain.py over the rebuild, --side allied|soviet) a new campaign started
            through the menus (Single Player, New Campaign, the side's emblem), won twelve times by RA2_NATIVE_FORCE_END
            (an entry every 200 loop frames), each score screen left by its Continue: every mission of the side
            starts and ends, the last with the campaign's ending, back at the main menu. Checks: the twelve missions
            won in order (ALL01..ALL12 or SOV01..SOV12, from the force-end lines), the main menu again, no trap or fault.

Every route also fails on a trap (a link-closure function reached), a fault, a crash trace or a
blit outside its surface. Skipped (77) without the installed game (game.exe in --data-dir) or the
pinned target game.exe (--game-exe, the build the rebuild's addresses name).

  rb_routes.py --route skirmish|soak|exit|audio|save|resave|original|combat|combatsave|campaign|production|allied1|allied1idle|forcewin|forcelose|chain
               [--side allied|soviet] --binary <rb-native-trial> --data-dir <game folder>
               --game-exe <game.exe> --work <dir> [--seconds N]
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tests'))
import game_process  # noqa: E402  (native/tests: the game's process group)

SKIP = 77
BAD = re.compile(r'unrecovered|trap|signal \d+|SIG(SEGV|BUS|ILL|ABRT|FPE)|crash:|swizzle: unmatched|reaches outside'
                 r'|original-save: no layout|resume: could not load|not saved:', re.I)
# At 1024x768: Single Player (the main menu's first button), Skirmish (Single Player's third),
# Start Game (the setup's first). Each menu click waits for its dialog to settle ("s<k>", host.cpp).
START = """\
s1+1000 click 834 304
s2+1000 click 834 388
s3+1500 click 834 346
"""
# In the game (frame 600) Escape opens the options; Abort Mission (877,327 125x25); the question's
# Quit (877,227); then the bottom slot (756,619 156x42) is the setup's Back, Single Player's Back and
# the main menu's Exit Game; the box's OK (597,472 126x26). The fallback quit fires if the route stalls.
EXIT = """\
600 key Escape
s4+1000 click 939 339
s5+1000 click 939 239
s6+1000 click 834 640
s7+1000 click 834 640
s8+1000 click 834 640
s9+1000 click 660 485
s9+30000 quit
"""
# Lane P4: at frame 300 a band selection over the tactical view (0..856 wide at 1024x768) selects the
# player's units, so their selection brackets and health bars draw (TechnoClass::Draw_Post_Render's
# old health bar slot 0x3DC). After the computer wins, the score screen's Continue (the bottom slot,
# as the setup's Back) -> the skirmish setup's Back -> Single Player's Back -> the main menu.
SELECT = """\
300 move 20 40
302 ldown 20 40
306 move 400 400
310 move 840 740
316 lup 840 740
"""
SCORE_CONTINUE = """\
s4+3000 click 834 640
s5+2000 click 834 640
s6+2000 click 834 640
"""
# Lane P5: the combat route's harness orders (glue/harness_native.inc, loop frames at 62 a second): three
# power plants, two Prism towers and a Gap Generator for the player beside its MCV; the Gap Generator
# selected (its radial indicator: Draw_Radial_Ellipse, XSurface::Draw_Ellipse and, for the sweep spokes,
# DSurface::Draw_Depth_Antialiased_Line); a Prism tank; two enemy Rhinos; the Prism tank sent at one
# (its laser and the towers': LaserDrawClass::Draw_House_Color; every laser weapon RA2 uses is
# house-coloured, so the anti-aliased line is the indicator's sweep spokes').
COMBAT_ORDERS = ('60 spawn GAPOWR 0 6; 61 spawn GAPOWR 2 6; 62 spawn GAPOWR -2 6; 70 spawn ATESLA 4 0; '
                 '72 spawn ATESLA 4 3; 80 spawn GAGAP -4 -4; 90 select GAGAP; 100 spawn SREF 2 -2; '
                 '200 spawn HTNK enemy 9 1; 201 spawn HTNK enemy 9 4; 202 fire SREF HTNK; 300 draws; '
                 '1200 health; 1200 draws; 2000 draws')
# Lane P6: the campaign route's saves (one of the user's, read in place), its seconds and the scripted triggers that must
# fire (`harness-trigger <frame> gone <ID> <name>`; frames seen at the save's game speed, about 30 a second). ALL02S
# (Allied 2): the Rocketeer movie, the Rocketeers' arrival and their change to the player's house, the first
# attack wave, the flak cannon hint, the patrol shot once the player's units hunt (frames 76 to 2,675). ALL03U (Allied 3):
# the garrisons, the IFV hints, input unlocked, the objective stated, the attack party, the civilian attacks and the
# Shock Trooper introduction (frames 46 to 3,751).
CAMPAIGN_ORDERS = '1 state; 1200 hunt; 2400 hunt; 3600 state'
CAMPAIGN = {
    'ALL02S.sav': (120, ['Destroy Airplane Team', 'Play Rocketeer Movie', 'Rocketeers Move', 'Rocketeers Have Arrived',
                         'Rocketeers Become USA', 'Attack Here - first time', 'Txt Flak Cannons', 'txt Flak Reminder',
                         '1st Patrol has been shot']),
    'ALL03U.sav': (150, ['Garrison1', 'Hint-IFV -- pt1', 'Garrison3', 'UnlockInput', 'StateObjective', 'AttackParty',
                         'ShockTrooperHint-pt2', 'CivAttack1', 'CivAttack2', 'ShockTroopIntro']),
}
# Lane P6: the production route's orders (glue/harness_native.inc: train, count, super).
PRODUCTION_ORDERS = ('60 spawn GAPOWR 0 6; 61 spawn GAPOWR 2 6; 70 spawn GAPILE 4 0; 80 spawn GAWEAP -5 0; 290 count E1; '
                     '290 count MTNK; 300 train E1; 301 train MTNK; 400 super 0 enemy; 1500 count E1; 1500 count MTNK; 1500 state')
# Lane P16: Allied 1's walk (glue/harness_native.inc goto, fire, where; loop frames at the save's speed). Tanya starts at 62,108
# and walks north past the Initial Paratroopers cell line (row 84); the Dreadnoughts lie at rows 90-91. ALLIED1_SINK sends her at
# them (the nearest left each time) until all four are gone.
ALLIED1_WALK = '1 state; 30 goto TANY 62 80; 300 where TANY; 600 where TANY'
ALLIED1_SINK = ALLIED1_WALK + ''.join('; %d fire TANY DRED' % f for f in range(650, 2650, 200)) + '; 2700 where TANY; 3300 state'
ALLIED1_IDLE = ALLIED1_WALK + '; 2700 where TANY; 3000 state'
ALLIED1_ESCAPE = ['Dreadnought Retreat', 'Enable Failure 3', 'Enable Failure 3b', 'Mission Failed']
ROUTES = {
    # route: (script, seconds, run-to, game args)
    'skirmish': (START, 75, 'never', '-noaudio -CD.'),
    'soak': (START + SELECT + SCORE_CONTINUE, 420, 'never', '-noaudio -CD.'),
    'exit': (START + EXIT, 120, 'never', '-noaudio -CD.'),
    'audio': (START, 75, 'never', '-CD.'),
    'save': ('', 60, 'never', '-noaudio -CD.'),
    'original': ('', 75, 'never', '-noaudio -CD.'),
    'resave': ('', 75, 'never', '-noaudio -CD.'),
    'combat': (START, 60, 'never', '-noaudio -CD.'),
    'combatsave': ('', 60, 'never', '-noaudio -CD.'),
    'campaign': ('', 110, 'never', '-noaudio -CD.'),
    'production': (START, 60, 'never', '-noaudio -CD.'),
    'allied1': ('', 130, 'never', '-noaudio -CD.'),
    'allied1idle': ('', 130, 'never', '-noaudio -CD.'),
    'forcewin': (START + SCORE_CONTINUE, 50, 'never', '-noaudio -CD.'),
    'forcelose': (START + SCORE_CONTINUE, 50, 'never', '-noaudio -CD.'),
    'chain': ('', 1200, 'seconds', '-noaudio -CD.'),
}
# Lane P7: main's tests/campaign_chain.py. Game pixels at 1024x768: Single Player and New Campaign in the menus' right
# column, the side's emblem in the campaign dialog, the score screen's Continue. Dialogs 1-3 are the menus; each score
# screen follows (4 .. 15); 16 is the main menu.
CHAIN_MISSIONS = 12
CHAIN_EMBLEM = {'allied': (433, 179), 'soviet': (429, 448)}
CHAIN_PREFIX = {'allied': 'ALL', 'soviet': 'SOV'}


def chain_script(side):
    lines = ['s1+1500 click 829 302', 's2+1500 click 831 305', 's3+2000 click %d %d' % CHAIN_EMBLEM[side]]
    for k in range(CHAIN_MISSIONS):
        lines.append('d%d+3000 click 831 640' % (4 + k))
    lines.append('d%d+3000 quit' % (4 + CHAIN_MISSIONS))
    return '\n'.join(lines) + '\n'
SAVE_FILE = 'RBROUTE.SAV'
COMPOUND = b'\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1'   # an OLE compound file: the original's save format
# The audio route's lines (RA2_PORT_TRACE, RA2_NATIVE_AUDIO_LOG): the start-up movie with sound, the
# mixer's two-second reports.
MOVIE_SOUND = re.compile(r'BinkOpen: .* [1-9]\d* audio tracks .*sound')
MIXED = re.compile(r'^audio: mixed \d+ frames, (\d+) of \d+ buffers playing, peak (\d+)')


def run(cmd, env, seconds, log):
    try:
        p = game_process.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                             errors='replace', timeout=seconds + 90)
        out, code = p.stdout, p.returncode
    except subprocess.TimeoutExpired as x:
        out = x.stdout.decode('utf-8', 'replace') if isinstance(x.stdout, bytes) else (x.stdout or '')
        code = -1
    with open(log, 'w') as f:
        f.write(out)
    return out, code


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--route', required=True, choices=sorted(ROUTES))
    ap.add_argument('--binary', required=True)
    ap.add_argument('--data-dir', default='')
    ap.add_argument('--game-exe', default='')
    ap.add_argument('--work', required=True)
    ap.add_argument('--seconds', type=int, default=0)
    ap.add_argument('--save', default='ALL01T.sav', help='original route: one of the user\'s saves in --data-dir')
    ap.add_argument('--side', default='allied', choices=sorted(CHAIN_EMBLEM), help='chain route: the campaign side')
    a = ap.parse_args()
    if not a.data_dir or not os.path.isfile(os.path.join(a.data_dir, 'game.exe')):
        print('%s: skipped: no installed game (game.exe) in --data-dir' % a.route)
        return SKIP
    if not a.game_exe or not os.path.isfile(a.game_exe):
        print('%s: skipped: no pinned target game.exe (--game-exe)' % a.route)
        return SKIP
    script, seconds, run_to, game_args = ROUTES[a.route]
    if a.route == 'chain':
        script = chain_script(a.side)
    seconds = a.seconds or seconds
    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)
    write = os.path.join(work, 'write')     # fresh per run: the game writes its RA2.INI here
    os.makedirs(write)
    script_path = os.path.join(work, a.route + '.script')
    with open(script_path, 'w') as f:
        f.write(script)
    env = dict(os.environ, RA2_GAME_EXE=a.game_exe, RA2_NATIVE_CRASH_TRACE='1', RA2_NATIVE_IGNORE_FOCUS='1')
    if a.route != 'audio':
        env['RA2_NATIVE_MOVIE_SKIP'] = '1'
    else:
        env.update(RA2_PORT_TRACE='1', RA2_NATIVE_AUDIO_LOG='1')
        env.pop('RA2_NATIVE_MOVIE_SKIP', None)
    for k in ('RA2_NATIVE_ORDERS', 'RA2_NATIVE_TRAP_CONTINUE', 'RA2_NATIVE_SAVE_AT', 'RA2_NATIVE_SAVE_LEAVE', 'RA2_NATIVE_ENHANCEMENTS',
              'RA2_NATIVE_TEST_PORT_SAVE'):
        env.pop(k, None)
    common = [a.binary, '--data-dir', a.data_dir, '--write-dir=' + write, '--game-args=' + game_args]
    bad = []
    if a.route in ('save', 'resave', 'combatsave'):
        if a.route == 'resave' and not os.path.isfile(os.path.join(a.data_dir, a.save)):
            print('resave: skipped: no %s in --data-dir' % a.save)
            return SKIP
        make = os.path.join(work, 'make.script')
        with open(make, 'w') as f:
            f.write(START if a.route != 'resave' else '')
        first = ['--input-script=' + make] if a.route != 'resave' else ['--load-save=' + a.save]
        # combatsave: the combat route's orders, saved at frame 260: the Prism tank's attack under way (the Rhino dies at about 350)
        at = {'save': '600', 'resave': '300', 'combatsave': '260'}[a.route]
        make_env = dict(env, RA2_NATIVE_SAVE_AT=at + ',' + SAVE_FILE)
        if a.route == 'combatsave':
            make_env['RA2_NATIVE_ORDERS'] = COMBAT_ORDERS
        out, code = run(common + ['--run-to=never', '--run-seconds=50'] + first, make_env, 50, os.path.join(work, 'make.log'))
        if a.route == 'combatsave' and not re.search(r'^harness-fire SREF -> HTNK', out, re.M):
            print('combatsave: FAIL: the attack was not under way before the save (%s)' % os.path.join(work, 'make.log'))
            return 1
        made = [l for l in out.splitlines() if 'reached harness-save' in l or 'harness-save' in l]
        if not os.path.isfile(os.path.join(write, SAVE_FILE)) or not any('saved' in l for l in made):
            print('%s: FAIL: the harness save was not made (%s)' % (a.route, os.path.join(work, 'make.log')))
            return 1
        with open(os.path.join(write, SAVE_FILE), 'rb') as f:
            if f.read(8) != COMPOUND:
                print('%s: FAIL: %s is not in the original\'s format (no compound file header)' % (a.route, SAVE_FILE))
                return 1
        cmd = common + ['--run-to=' + run_to, '--run-seconds=%d' % seconds, '--load-save=' + SAVE_FILE]
        env.update(RA2_NATIVE_STG_LOG='1')
    elif a.route == 'campaign':
        if a.save not in CAMPAIGN:
            a.save = 'ALL02S.sav' if a.save == 'ALL01T.sav' else a.save
        if a.save not in CAMPAIGN:
            print('campaign: FAIL: no trigger list for %s (CAMPAIGN)' % a.save)
            return 1
        if not os.path.isfile(os.path.join(a.data_dir, a.save)):
            print('campaign: skipped: no %s in --data-dir' % a.save)
            return SKIP
        seconds = a.seconds or CAMPAIGN[a.save][0]
        cmd = common + ['--run-to=' + run_to, '--run-seconds=%d' % seconds, '--load-save=' + a.save]
        env.update(RA2_NATIVE_STG_LOG='1', RA2_NATIVE_TRIGGERS='1', RA2_NATIVE_ORDERS=CAMPAIGN_ORDERS)
    elif a.route in ('allied1', 'allied1idle'):
        if not os.path.isfile(os.path.join(a.data_dir, a.save)):
            print('%s: skipped: no %s in --data-dir' % (a.route, a.save))
            return SKIP
        cmd = common + ['--run-to=' + run_to, '--run-seconds=%d' % seconds, '--load-save=' + a.save]
        env.update(RA2_NATIVE_STG_LOG='1', RA2_NATIVE_TRIGGERS='1', RA2_NATIVE_ORDERS=ALLIED1_SINK if a.route == 'allied1' else ALLIED1_IDLE)
        env.pop('RA2_NATIVE_FORCE_END', None)
    elif a.route == 'original':
        if not os.path.isfile(os.path.join(a.data_dir, a.save)):
            print('original: skipped: no %s in --data-dir' % a.save)
            return SKIP
        cmd = common + ['--run-to=' + run_to, '--run-seconds=%d' % seconds, '--load-save=' + a.save]
        env.update(RA2_NATIVE_STG_LOG='1', RA2_NATIVE_ORIGINAL_LOG='1')
    else:
        cmd = common + ['--run-to=' + run_to, '--run-seconds=%d' % seconds, '--input-script=' + script_path]
        if a.route == 'combat':
            env['RA2_NATIVE_ORDERS'] = COMBAT_ORDERS
        if a.route == 'production':
            env['RA2_NATIVE_ORDERS'] = PRODUCTION_ORDERS
        if a.route in ('forcewin', 'forcelose'):
            env['RA2_NATIVE_FORCE_END'] = '300' + (',lose' if a.route == 'forcelose' else '')
        if a.route == 'chain':
            env['RA2_NATIVE_FORCE_END'] = ','.join(str(100 + 200 * k) for k in range(CHAIN_MISSIONS))
            env['RA2_NATIVE_DIALOG_LOG'] = '1'
    log = os.path.join(work, a.route + '.log')
    out, code = run(cmd, env, seconds, log)
    lines = out.splitlines()
    hits = [l for l in lines if BAD.search(l) and not l.startswith('input-script')]
    if hits:
        bad.append('fault or trap: %s' % hits[0][:200])
    stop = [l for l in lines if 'stopped:' in l]
    stop = stop[-1] if stop else ''
    frames = 0
    m = [re.search(r'frames run (\d+)', l) for l in lines]
    m = [x for x in m if x]
    if m:
        frames = int(m[-1].group(1))
    shown = ''.join(l for l in lines if 'dialogs shown:' in l)
    if a.route in ('save', 'resave', 'combatsave') and 'resume: loaded ' + SAVE_FILE not in out:
        bad.append('the save was not loaded')
    if a.route == 'campaign':
        if 'resume: loaded ' + a.save not in out:
            bad.append('%s was not loaded' % a.save)
        fired = set(m.group(1).strip() for m in re.finditer(r'^harness-trigger \d+ gone \S+ (.*)$', out, re.M))
        missing = [t for t in CAMPAIGN[a.save][1] if t not in fired]
        if missing:
            bad.append('scripted triggers not fired: %s' % ', '.join(missing))
        if not re.search(r'^harness-triggers \S+ [1-9]', out, re.M):
            bad.append('no trigger list watched')
        if not re.search(r'^harness-hunt [1-9]', out, re.M):
            bad.append('the hunt order found none of the player\'s units')
    if a.route in ('allied1', 'allied1idle'):
        if 'resume: loaded ' + a.save not in out:
            bad.append('%s was not loaded' % a.save)
        if not re.search(r'^harness-goto TANY @62,80 1$', out, re.M):
            bad.append('Tanya was not sent north')
        gone = [(int(m.group(1)), m.group(2).strip()) for m in re.finditer(r'^harness-trigger (\d+) gone \S+ (.*)$', out, re.M)]
        names = [n for _, n in gone]
        lost = re.search(r'^game-over: player loses at frame (\d+)', out, re.M)
        if a.route == 'allied1':
            if 'Objective 1 Complete' not in names:
                bad.append('Objective 1 Complete did not fire (the Dreadnoughts not all sunk: %d harness-killed)'
                           % len(re.findall(r'^harness-killed DRED ', out, re.M)))
            for n in ('Enable Failure 3', 'Enable Failure 3b', 'Mission Failed'):
                if n in names:
                    bad.append('%s fired' % n)
            if lost or re.search(r'^harness-house \d+ lose ', out, re.M):
                bad.append('the player lost (%s)' % (lost.group(0) if lost else 'lose flag set'))
            if 'Dreadnought Retreat' not in names:
                bad.append('the run did not reach Dreadnought Retreat (the escape the sinking forestalls)')
        else:
            order = [n for n in names if n in ALLIED1_ESCAPE]
            if order != ALLIED1_ESCAPE:
                bad.append('the escape chain was %s, not %s' % (order, ALLIED1_ESCAPE))
            if not lost:
                bad.append('no loss: the escaped Dreadnoughts must fail the mission')
            else:
                retreat = [f for f, n in gone if n == 'Dreadnought Retreat']
                if not retreat or int(lost.group(1)) <= retreat[0]:
                    bad.append('lost before Dreadnought Retreat (%s)' % lost.group(0))
            for n in ('Enable Failure', 'Enable Failure b', 'Enable Failure 2'):
                if n in names:
                    bad.append('%s fired (another failure than the escape)' % n)
    if a.route in ('save', 'resave', 'combatsave', 'original', 'campaign', 'allied1', 'allied1idle'):
        if not any('original save (compound file)' in l for l in lines):
            bad.append('the save was not opened as an original (compound file)')
    if a.route == 'original':
        if 'resume: loaded ' + a.save not in out:
            bad.append('%s was not loaded' % a.save)
        converted = sum(1 for l in lines if l.startswith('original-save: ') and ' i686 bytes to ' in l)
        if not converted:
            bad.append('no object converted by the field maps')
    if a.route == 'combat':
        for want in ('GAPOWR', 'ATESLA', 'GAGAP', 'SREF', 'HTNK'):
            if not re.search(r'^harness-spawn %s ' % want, out, re.M):
                bad.append('%s was not placed' % want)
        if not re.search(r'^harness-select GAGAP selected', out, re.M):
            bad.append('the Gap Generator was not selected')
        if not re.search(r'^harness-fire SREF -> HTNK', out, re.M):
            bad.append('the Prism tank was not sent at a Rhino')
        draws = re.findall(r'^harness-draws ellipse (\d+) aaline (\d+) lasers (\d+)', out, re.M)
        ellipse, aaline, lasers = (int(draws[-1][0]), int(draws[-1][1]), int(draws[-1][2])) if draws else (0, 0, 0)
        if not lasers:
            bad.append('no laser beam drawn (Prism tank or towers: LaserDrawClass::Draw_House_Color)')
        if not ellipse:
            bad.append('XSurface::Draw_Ellipse never ran (no radial indicator drawn)')
        if not aaline:
            bad.append('DSurface::Draw_Depth_Antialiased_Line never ran (no laser or sweep drawn)')
        if not re.search(r'^harness-killed HTNK ', out, re.M):
            bad.append('the Rhino the Prism tank was sent at did not die')
    if a.route == 'production':
        for want in ('GAPOWR', 'GAPILE', 'GAWEAP'):
            if not re.search(r'^harness-spawn %s ' % want, out, re.M):
                bad.append('%s was not placed' % want)
        for want in ('E1', 'MTNK'):
            counts = [int(x) for x in re.findall(r'^harness-count %s (\d+)' % want, out, re.M)]
            if not re.search(r'^harness-train %s$' % want, out, re.M):
                bad.append('%s was not ordered' % want)
            elif len(counts) < 2 or counts[-1] <= counts[0]:
                bad.append('no %s produced (counts %s)' % (want, counts))
        if not re.search(r'^harness-state \S+ money 9100 ', out, re.M):
            bad.append('the two prices were not taken (money 9,100 expected at frame 1,500)')
        m = re.search(r'^harness-super 0 MultiSpecial @\d+,\d+ -> \S+ s(\d+)', out, re.M)
        if not m:
            bad.append('the nuclear missile was not launched')
        else:
            seen = [int(x) for x in re.findall(r'^harness-watch s(\d+)', out, re.M)]
            if not seen or min(seen) >= int(m.group(1)):
                bad.append('the missile\'s target was not damaged (strengths %s)' % seen[:6])
    if a.route in ('skirmish', 'soak', 'audio', 'save', 'resave', 'combatsave', 'original', 'combat', 'campaign', 'production',
                   'allied1', 'allied1idle', 'forcewin', 'forcelose'):
        if 'time limit' not in stop:
            bad.append('the run stopped early: %s' % (stop or 'no stop line'))
        if not any('reached frame' in l for l in lines):
            bad.append('the game loop was not reached')
        # A loaded game runs at its save's game speed (Sync_Delay waits; about 20 frames a second seen),
        # not the skirmish slider's default 0 the frame floor paces at 62.
        floor = {'forcewin': 300, 'forcelose': 300, 'soak': 6000, 'save': 900, 'resave': 900, 'combatsave': 900, 'original': 900, 'campaign': 2400, 'allied1': 3000, 'allied1idle': 3000}.get(a.route, int(3000 * seconds / 75))
        if frames < floor:
            bad.append('%d frames run (at least %d expected)' % (frames, floor))
    if a.route in ('forcewin', 'forcelose'):
        want = 'force-end: %s at frame 300 ' % ('loss' if a.route == 'forcelose' else 'win')
        if not any(l.startswith(want) for l in lines):
            bad.append('no forced end at frame 300 (%s)' % want.strip())
        if '(50 controls)' not in shown:
            bad.append('the skirmish score screen was not shown')
        elif not any('reached menu-return' in l for l in lines):
            bad.append('the score screen\'s Continue did not lead back to the main menu')
    if a.route == 'chain':
        won = [m.group(1) for m in (re.match(r'force-end: win at frame \d+ in (\S+)', l) for l in lines) if m]
        numbers = []
        for scenario in won:
            mm = re.match(CHAIN_PREFIX[a.side] + r'(\d\d)', scenario, re.I)
            numbers.append(int(mm.group(1)) if mm else -1)
        dialogs = [int(mm.group(1)) for mm in (re.match(r'dialog shown #(\d+)', l) for l in lines) if mm]
        print('chain: %s missions won %s' % (a.side, ' '.join(won) or 'none'))
        if numbers != list(range(1, CHAIN_MISSIONS + 1)):
            bad.append('missions won %s, not %s01-%02d in order' % (numbers, CHAIN_PREFIX[a.side], CHAIN_MISSIONS))
        if not (any(l.startswith('run-to: reached menu-return') for l in lines) and max(dialogs or [0]) >= 4 + CHAIN_MISSIONS):
            bad.append('the main menu did not come back after the last mission (dialogs shown: %d)' % len(dialogs))
    if a.route == 'soak' and '(50 controls)' not in shown:
        bad.append('the skirmish score screen was not shown')
    elif a.route == 'soak' and not any('reached menu-return' in l for l in lines):
        bad.append('the score screen\'s Continue did not lead back to the main menu')
    if a.route == 'exit':
        if code != 0:
            bad.append('exit %d' % code)
        if 'WinMain returned 0' not in stop:
            bad.append('WinMain did not return 0: %s' % (stop or 'no stop line'))
        if not any('reached menu-return' in l for l in lines):
            bad.append('the main menu was not shown again')
    if a.route == 'audio':
        if not any(MOVIE_SOUND.search(l) for l in lines):
            bad.append('the start-up movie was not opened with sound')
        in_game = False
        menu_peak = game_peak = game_playing = 0
        for l in lines:
            if 'reached frame' in l:
                in_game = True
            mm = MIXED.match(l)
            if not mm:
                continue
            if in_game:
                game_playing = max(game_playing, int(mm.group(1)))
                game_peak = max(game_peak, int(mm.group(2)))
            else:
                menu_peak = max(menu_peak, int(mm.group(2)))
        if menu_peak == 0:
            bad.append('the mixer was silent before the game')
        if game_playing < 3 or game_peak == 0:
            bad.append('in the game: at most %d buffers playing, peak %d' % (game_playing, game_peak))
    for b in bad:
        print('%s: FAIL: %s' % (a.route, b))
    print('%s: %s: %d frames, exit %d, %s (%s)' % (a.route, 'FAIL' if bad else 'OK', frames, code, stop[:120], log))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
