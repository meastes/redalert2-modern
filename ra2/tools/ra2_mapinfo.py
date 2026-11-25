#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""A scenario's own rules in plain lines, for planning a mission (lane P21).

Reads a map file the step server copied from the user's installation at run time (ra2_step.py send "mapfile <file>",
read through the game's own mix files) and prints its triggers (house, events, actions, linked trigger, difficulty
flags), tags, team types, scripts and waypoints, naming the event and action types. The map stays in the run folder
under build/; nothing of it enters the repository.

  ra2_mapinfo.py <map file> [--section triggers|teams|scripts|waypoints|basic|all] [--grep TEXT]

Event and action names follow the scenario editors' lists for Red Alert 2 (Tiberian Sun's trigger system); a number
without a name is printed as it is.
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
    29: 'destroyed-by-anything-fake', 30: 'low-power', 31: 'bridge-destroyed', 32: 'building-exists', 33: 'selected-by-player',
    34: 'comes-near-waypoint', 35: 'enemy-in-spotlight', 36: 'local-set', 37: 'local-clear', 38: 'first-damaged-combat',
    39: 'half-health-combat', 40: 'quarter-health-combat', 41: 'first-damaged-any', 42: 'half-health-any',
    43: 'quarter-health-any', 44: 'attacked-by-house', 45: 'ambient-light-below', 46: 'ambient-light-above',
    47: 'elapsed-scenario-time', 48: 'destroyed-by-anything', 49: 'pickup-crate', 50: 'pickup-crate-any', 51: 'random-delay',
    52: 'credits-below', 53: 'player-spied', 54: 'destroyed-by-anything-incl-infiltrate', 55: 'destroyed-building-type',
    56: 'destroyed-unit-type', 57: 'tech-type-exists', 58: 'tech-type-does-not-exist', 59: 'power-full',
    60: 'entered-or-overflown-by', 61: 'tech-type-exists-n',
}
ACTIONS = {
    0: 'none', 1: 'winner-is', 2: 'loser-is', 3: 'production-begins', 4: 'create-team', 5: 'destroy-team', 6: 'all-to-hunt',
    7: 'reinforcement-team', 8: 'drop-zone-flare', 9: 'fire-sale', 10: 'play-movie', 11: 'text-trigger', 12: 'destroy-trigger',
    13: 'autocreate-begins', 14: 'change-house', 15: 'allow-win', 16: 'reveal-all-map', 17: 'reveal-around-waypoint',
    18: 'reveal-zone-of-waypoint', 19: 'play-sound', 20: 'play-music', 21: 'play-speech', 22: 'force-trigger',
    23: 'timer-start', 24: 'timer-stop', 25: 'timer-extend', 26: 'timer-shorten', 27: 'timer-set', 28: 'global-set',
    29: 'global-clear', 30: 'auto-base-building', 31: 'grow-shroud', 32: 'destroy-attached-object', 33: 'add-one-time-super',
    34: 'add-repeating-super', 35: 'preferred-target', 36: 'all-change-house', 37: 'make-ally', 38: 'make-enemy',
    39: 'change-zoom', 40: 'resize-player-view', 41: 'play-anim-at', 42: 'do-explosion-at', 43: 'create-voxel-anim',
    44: 'ion-storm-start', 45: 'ion-storm-stop', 46: 'lock-input', 47: 'unlock-input', 48: 'move-camera-to-waypoint',
    49: 'zoom-in', 50: 'zoom-out', 51: 'reshroud-map', 52: 'change-light-behaviour', 53: 'enable-trigger',
    54: 'disable-trigger', 55: 'create-radar-event', 56: 'local-set', 57: 'local-clear', 58: 'meteor-shower',
    59: 'reduce-tiberium', 60: 'sell-building', 61: 'turn-off-building', 62: 'turn-on-building', 63: 'apply-100-damage',
    64: 'light-flash-small', 65: 'light-flash-medium', 66: 'light-flash-large', 67: 'announce-win', 68: 'announce-lose',
    69: 'force-end', 70: 'destroy-tag', 71: 'set-ambient-step', 72: 'set-ambient-rate', 73: 'set-ambient-light',
    74: 'ai-triggers-begin', 75: 'ai-triggers-stop', 76: 'ratio-ai-teams', 77: 'ratio-aircraft', 78: 'ratio-infantry',
    79: 'ratio-units', 80: 'reinforcement-at-waypoint', 81: 'wakeup-self', 82: 'wakeup-all-sleepers',
    83: 'wakeup-all-harmless', 84: 'wakeup-group', 85: 'vein-growth', 86: 'tiberium-growth', 87: 'ice-growth',
    88: 'particle-anim', 89: 'remove-particle-anim', 90: 'lightning-strike', 91: 'go-berzerk', 92: 'activate-firestorm',
    93: 'deactivate-firestorm', 94: 'ion-cannon-strike', 95: 'nuke-strike', 96: 'chem-missile-strike',
    97: 'toggle-train-cargo', 98: 'play-sound-random', 99: 'play-sound-at', 100: 'play-ingame-movie',
    101: 'reshroud-map-at', 102: 'lightning-storm-strike', 103: 'timer-text', 104: 'flash-team', 105: 'talk-bubble',
    106: 'set-objective-text', 107: 'set-global', 108: 'add-unit-to-unit', 109: 'sabotage-building', 110: 'set-tech-level',
    111: 'reinforce-team-at', 112: 'attack-team-at-waypoint', 113: 'create-crate', 114: 'iron-curtain-at',
    115: 'pause-game', 116: 'evict-occupiers', 117: 'center-camera', 118: 'make-house-cheer', 119: 'set-tab-to',
    120: 'flash-cameo', 121: 'stop-sounds-at', 122: 'play-ingame-movie-pause', 123: 'clear-smudges',
    124: 'destroy-all-of', 125: 'destroy-all-buildings-of', 126: 'destroy-all-land-units-of', 127: 'destroy-all-units-of',
    128: 'destroy-all-of-house', 129: 'make-ally-oneway', 130: 'make-enemy-oneway', 131: 'change-tech-level',
    132: 'reveal-map-around-team', 133: 'create-team-and-reveal', 134: 'set-spotlight-behaviour',
    135: 'set-team-mission', 136: 'set-tech-type-tag', 137: 'superweapon-strike-at', 138: 'set-ambient-sound',
    139: 'set-house-lighting', 140: 'clear-team-lighting', 141: 'reshroud-team', 142: 'set-camera-zoom', 143: 'save-game',
    144: 'goto-next-mission', 145: 'preview-waypoint', 146: 'reset-objectives',
}
WAYPOINT_ACTIONS = {7, 8, 17, 18, 41, 42, 43, 48, 63, 64, 65, 66, 80, 88, 90, 94, 95, 96, 99, 101, 102, 112, 113, 114,
                    117, 121, 145}


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


def events(value):
    f = value.split(',')
    count = int(f[0])
    out, i = [], 1
    for _ in range(count):
        t = int(f[i])
        width = 4 if t in (60, 61) else 3
        params = f[i + 1:i + width]
        out.append('%s(%s)' % (EVENTS.get(t, 'event%d' % t), ','.join(params)))
        i += width
    return out


def actions(value, waypoints):
    f = value.split(',')
    count = int(f[0])
    out, i = [], 1
    for _ in range(count):
        t = int(f[i])
        params = f[i + 1:i + 8]
        text = '%s(%s)' % (ACTIONS.get(t, 'action%d' % t), ','.join(params))
        if t in WAYPOINT_ACTIONS and params:
            wp = waypoint_from_letters(params[-1])
            if wp is not None and str(wp) in waypoints:
                text += '[wp%d@%s]' % (wp, waypoint_cell(waypoints[str(wp)]))
        out.append(text)
        i += 8
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('--section', default='all')
    ap.add_argument('--grep', default='')
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
            ev = events(ini.get('Events', {}).get(k, '0'))
            ac = actions(ini.get('Actions', {}).get(k, '0'), waypoints)
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
