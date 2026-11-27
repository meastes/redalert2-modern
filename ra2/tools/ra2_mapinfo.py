#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""A scenario's own rules in plain lines, for planning a mission (lane P21).

Reads a map file the step server copied from the user's installation at run time (ra2_step.py send "mapfile <file>",
read through the game's own mix files) and prints its triggers (house, events, actions, linked trigger, difficulty
flags), tags, team types, scripts and waypoints, naming the event and action types. The map stays in the run folder
under build/; nothing of it enters the repository.

  ra2_mapinfo.py <map file> [--section triggers|teams|scripts|waypoints|basic|all] [--grep TEXT]

Event and action numbers are those of the two executables' dispatch tables (lane P73 read game.exe's and gamemd.exe's
jump tables in TEventClass::operator(), Event_Needs and TActionClass::operator(); events 53-61 and actions 106-145 were
misnamed before, off by one or more). Names follow the handlers (vendor/ra2-decomp's taction.cpp/tevent.cpp); a number
without a name is printed as eventN/actionN.
"""
import argparse
import re
import sys

EVENTS = {
    0: 'none', 1: 'entered-by', 2: 'spied-by', 3: 'thieved-by', 4: 'discovered-by-player', 5: 'house-discovered',
    6: 'attacked-by-any', 7: 'destroyed-by-any', 8: 'any-event', 9: 'destroyed-units-all', 10: 'destroyed-buildings-all',
    11: 'destroyed-all', 12: 'credits-exceed', 13: 'elapsed-time', 14: 'mission-timer-expired', 15: 'destroyed-buildings-n',
    16: 'destroyed-units-n', 17: 'no-factories-left', 18: 'civilians-evacuated', 19: 'build-building-type',
    20: 'build-unit-type', 21: 'build-infantry-type', 22: 'build-aircraft-type', 23: 'team-leaves-map', 24: 'zone-entry-by',
    25: 'crosses-horizontal-line', 26: 'crosses-vertical-line', 27: 'global-set', 28: 'global-clear',
    29: 'destroyed-by-anything-not-infiltrate', 30: 'low-power', 31: 'bridge-destroyed', 32: 'building-exists',
    33: 'selected-by-player', 34: 'comes-near-waypoint', 35: 'enemy-in-spotlight', 36: 'local-set', 37: 'local-clear',
    38: 'first-damaged-combat', 39: 'half-health-combat', 40: 'quarter-health-combat', 41: 'first-damaged-any',
    42: 'half-health-any', 43: 'quarter-health-any', 44: 'attacked-by-house', 45: 'ambient-light-below',
    46: 'ambient-light-above', 47: 'elapsed-scenario-time', 48: 'destroyed-by-anything', 49: 'pickup-crate',
    50: 'pickup-crate-any', 51: 'random-delay', 52: 'credits-below', 53: 'spy-as-house-entering',
    54: 'spy-as-infantry-entering', 55: 'destroyed-naval-all', 56: 'destroyed-land-all', 57: 'building-does-not-exist',
    # Yuri's Revenge only (game.exe's tables stop at 57):
    58: 'power-full', 59: 'entered-or-overflown-by', 60: 'tech-type-exists-n', 61: 'tech-type-does-not-exist',
}
# The last event and action each executable knows (game.exe 1.006: Event_Needs 0x006E6970 and TEventClass::operator()
# 0x006E6050 stop at 57, TActionClass::operator() 0x006ADAE0 at 117; gamemd.exe 1.001: 0x0071F5B0 and 0x0071E940 at 61,
# 0x006DD8B0 at 145). Read from the jump tables by lane P73; both games number the shared entries alike.
LAST = {'ra2': (57, 117), 'yr': (61, 145)}
ACTIONS = {
    0: 'none', 1: 'winner-is', 2: 'loser-is', 3: 'production-begins', 4: 'create-team', 5: 'destroy-team', 6: 'all-to-hunt',
    7: 'reinforcement-team', 8: 'drop-zone-flare', 9: 'fire-sale', 10: 'play-movie', 11: 'text-trigger', 12: 'destroy-trigger',
    13: 'autocreate-begins', 14: 'change-house', 15: 'allow-win-noop', 16: 'reveal-all-map', 17: 'reveal-around-waypoint',
    18: 'reveal-zone-of-waypoint', 19: 'play-sound', 20: 'play-music', 21: 'play-speech', 22: 'force-trigger',
    23: 'timer-start', 24: 'timer-stop', 25: 'timer-extend', 26: 'timer-shorten', 27: 'timer-set', 28: 'global-set',
    29: 'global-clear', 30: 'auto-base-building', 31: 'grow-shroud', 32: 'destroy-attached-object', 33: 'add-one-time-super',
    34: 'add-repeating-super', 35: 'preferred-target', 36: 'all-change-house', 37: 'make-ally', 38: 'make-enemy',
    39: 'change-zoom-noop', 40: 'resize-player-view', 41: 'play-anim-at', 42: 'do-explosion-at', 43: 'create-voxel-anim',
    44: 'ion-storm-start', 45: 'ion-storm-stop', 46: 'lock-input', 47: 'unlock-input', 48: 'move-camera-to-waypoint',
    49: 'zoom-in', 50: 'zoom-out', 51: 'reshroud-map', 52: 'change-light-behaviour', 53: 'enable-trigger',
    54: 'disable-trigger', 55: 'create-radar-event', 56: 'local-set', 57: 'local-clear', 58: 'meteor-shower',
    59: 'reduce-tiberium', 60: 'sell-building', 61: 'turn-off-building', 62: 'turn-on-building', 63: 'apply-100-damage',
    64: 'light-flash-small', 65: 'light-flash-medium', 66: 'light-flash-large', 67: 'announce-win', 68: 'announce-lose',
    69: 'force-end', 70: 'destroy-tag', 71: 'set-ambient-step', 72: 'set-ambient-rate', 73: 'set-ambient-light',
    74: 'ai-triggers-begin', 75: 'ai-triggers-stop', 76: 'ratio-ai-teams', 77: 'ratio-aircraft', 78: 'ratio-infantry',
    79: 'ratio-units', 80: 'reinforcement-at-waypoint', 81: 'wakeup-self', 82: 'wakeup-all-sleepers',
    83: 'wakeup-all-harmless', 84: 'wakeup-group', 85: 'vein-growth', 86: 'tiberium-growth', 87: 'ice-growth',
    88: 'particle-anim', 89: 'remove-particle-anim', 90: 'lightning-strike', 91: 'go-berzerk', 92: 'activate-firestorm-noop',
    93: 'deactivate-firestorm-noop', 94: 'ion-cannon-strike', 95: 'nuke-strike', 96: 'chem-missile-strike',
    97: 'toggle-train-cargo', 98: 'play-sound-random', 99: 'play-sound-at', 100: 'play-ingame-movie',
    101: 'reshroud-map-at', 102: 'lightning-storm-strike', 103: 'timer-text', 104: 'flash-team', 105: 'talk-bubble',
    106: 'set-tech-level', 107: 'chrono-warp-team-to', 108: 'create-crate', 109: 'iron-curtain-at', 110: 'pause-game',
    111: 'evict-occupiers', 112: 'center-camera-at', 113: 'make-house-cheer', 114: 'set-tab-to', 115: 'flash-cameo',
    116: 'stop-sounds-at', 117: 'play-ingame-movie-pause',
    # Yuri's Revenge only (game.exe's table stops at 117):
    118: 'clear-smudges', 119: 'destroy-all-of', 120: 'destroy-all-buildings-of', 121: 'destroy-all-land-units-of',
    122: 'destroy-all-naval-units-of', 123: 'mind-control-base-of', 124: 'restore-mind-controlled-base-of',
    125: 'create-building-at', 126: 'restore-starting-units-of', 127: 'start-chrono-screen-effect', 128: 'teleport-all-to',
    129: 'set-super-charge', 130: 'restore-starting-buildings-of', 131: 'flash-buildings-of-type',
    132: 'set-super-recharge-time', 133: 'reset-super-recharge-time', 134: 'reset-super', 135: 'set-preferred-target-cell',
    136: 'clear-preferred-target-cell', 137: 'set-base-center', 138: 'clear-base-center', 139: 'blackout-radar',
    140: 'set-defense-cell', 141: 'clear-defense-cell', 142: 'retint-red', 143: 'retint-green', 144: 'retint-blue',
    145: 'jump-camera-to-base',
}
# Actions whose waypoint is the last parameter (the editors' letters; TActionClass's location) and those whose waypoint is
# the data parameter (a number), from the handlers in vendor/ra2-decomp/src/taction.cpp.
WAYPOINT_ACTIONS = {8, 41, 42, 43, 48, 55, 58, 80, 88, 89, 90, 94, 95, 96, 99, 102, 107, 108, 109, 112, 116, 125, 128,
                    135, 137, 140}
WAYPOINT_DATA_ACTIONS = {17, 18, 59, 63, 64, 65, 66, 98, 101}


def read_ini(path):
    sections, current = {}, None
    with open(path, errors='replace') as f:
        for line in f:
            line = line.rstrip('\r\n')
            if line.startswith(';'):
                continue
            m = re.match(r'\s*\[(.+?)\]', line)
            if m:
                current = sections.setdefault(m.group(1), {})
                continue
            if current is not None and '=' in line:
                k, v = line.split('=', 1)
                current[k.strip()] = v.split(';')[0].strip() if not v.strip().startswith('"') else v.strip()
    return sections


def waypoint_cell(value):
    v = int(value)
    return '%d,%d' % (v % 1000, v // 1000)


def waypoint_name(index):
    """The editors' letter names (A..Z, AA..) for waypoint numbers in action parameters."""
    return str(index)


def waypoint_from_letters(text):
    n = 0
    for c in text:
        if not c.isalpha():
            return None
        n = n * 26 + (ord(c.upper()) - 64)
    return n - 1


def name_of(table, t, game, which):
    """The name for an event or action number, or eventN/actionN past the game's last one (game None: any)."""
    if game and t > LAST[game][which]:
        return '%s%d' % (('event', 'action')[which], t)
    return table.get(t, '%s%d' % (('event', 'action')[which], t))


def events(value, game=None):
    """Each event is its type, a parameter code and a value; code 2 (TEventClass::Read_INI: Event_Needs's number and
    techno type, events 60 and 61) adds a type name."""
    f = value.split(',')
    count = int(f[0])
    out, i = [], 1
    for _ in range(count):
        t = int(f[i])
        width = 4 if f[i + 1].strip() == '2' else 3
        params = f[i + 1:i + width]
        out.append('%s(%s)' % (name_of(EVENTS, t, game, 0), ','.join(params)))
        i += width
    return out


def actions(value, waypoints, game=None):
    f = value.split(',')
    count = int(f[0])
    out, i = [], 1
    for _ in range(count):
        t = int(f[i])
        params = f[i + 1:i + 8]
        text = '%s(%s)' % (name_of(ACTIONS, t, game, 1), ','.join(params))
        wp = None
        if t in WAYPOINT_ACTIONS and params:
            wp = waypoint_from_letters(params[-1])
        elif t in WAYPOINT_DATA_ACTIONS and len(params) > 1 and params[1].strip().lstrip('-').isdigit():
            wp = int(params[1])
        if wp is not None:
            if str(wp) in waypoints:
                text += '[wp%d@%s]' % (wp, waypoint_cell(waypoints[str(wp)]))
        out.append(text)
        i += 8
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('--section', default='all')
    ap.add_argument('--grep', default='')
    ap.add_argument('--game', choices=('ra2', 'yr'), default=None,
                    help="name numbers past the game's last event/action as eventN/actionN")
    a = ap.parse_args()
    ini = read_ini(a.map)
    want = a.section
    lines = []
    waypoints = ini.get('Waypoints', {})
    if want in ('basic', 'all'):
        for k, v in ini.get('Basic', {}).items():
            lines.append('basic %s=%s' % (k, v))
    if want in ('waypoints', 'all'):
        for k, v in sorted(waypoints.items(), key=lambda kv: int(kv[0])):
            lines.append('waypoint %s @%s' % (k, waypoint_cell(v)))
    if want in ('triggers', 'all'):
        tags = {}
        for k, v in ini.get('Tags', {}).items():
            f = v.split(',')
            if len(f) >= 3:
                tags.setdefault(f[2], []).append('%s(%s,persist %s)' % (k, f[1], f[0]))
        for k, v in ini.get('Triggers', {}).items():
            f = v.split(',')
            house, linked, name = f[0], f[1], f[2]
            disabled = f[3] if len(f) > 3 else '0'
            diff = ''.join(c for c, x in zip('emh', f[4:7]) if x == '1')
            ev = events(ini.get('Events', {}).get(k, '0'), a.game)
            ac = actions(ini.get('Actions', {}).get(k, '0'), waypoints, a.game)
            lines.append('trigger %s "%s" house %s%s%s diff %s tags %s\n    when %s\n    then %s' % (
                k, name, house, ' linked ' + linked if linked != '<none>' else '', ' DISABLED' if disabled == '1' else '',
                diff, ' '.join(tags.get(k, [])) or '-', ' + '.join(ev), ' ; '.join(ac)))
    if want in ('teams', 'all'):
        for k, v in ini.get('TeamTypes', {}).items():
            t = ini.get(v, {})
            members = ','.join(t.get(str(i), '') for i in range(1, 6) if t.get(str(i)))
            members = ' '.join(x for x in [t.get('TaskForce', '')] if x)
            tf = ini.get(t.get('TaskForce', ''), {})
            units = ' '.join(tf.get(str(i), '') for i in range(0, 8) if tf.get(str(i)))
            lines.append('team %s "%s" house %s script %s taskforce %s [%s] waypoint %s' % (
                v, t.get('Name', ''), t.get('House', ''), t.get('Script', ''), t.get('TaskForce', ''), units, t.get('Waypoint', '')))
    if want in ('scripts', 'all'):
        for k, v in ini.get('ScriptTypes', {}).items():
            s = ini.get(v, {})
            steps = ' '.join(s.get(str(i), '') for i in range(0, 50) if s.get(str(i)))
            lines.append('script %s "%s": %s' % (v, s.get('Name', ''), steps))
    for line in lines:
        if a.grep and a.grep.lower() not in line.lower():
            continue
        print(line)
    return 0


if __name__ == '__main__':
    sys.exit(main())
