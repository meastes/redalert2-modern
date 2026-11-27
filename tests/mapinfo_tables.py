#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ra2/tools/ra2_mapinfo.py's trigger event and action numbers (lane P73; no game needed).

Lane P73 read the two executables' trigger dispatch tables (game.exe 1.006: Event_Needs 0x006E6970,
TEventClass::operator() 0x006E6050, TActionClass::operator() 0x006ADAE0; gamemd.exe 1.001: 0x0071F5B0, 0x0071E940,
0x006DD8B0) and found the tool's names for events 53-61 and actions 106-145 shifted from the real handlers: a map's
"building N does not exist" (57) printed as tech-type-exists, "entered by" (59) as power-full. This keeps the numbers
the tables give, and the event parser's width (code 2 adds a type name, as TEventClass::Read_INI reads it).
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'ra2', 'tools'))
import ra2_mapinfo as m  # noqa: E402

EXPECT_EVENTS = {1: 'entered-by', 13: 'elapsed-time', 32: 'building-exists', 52: 'credits-below',
                 53: 'spy-as-house-entering', 54: 'spy-as-infantry-entering', 55: 'destroyed-naval-all',
                 56: 'destroyed-land-all', 57: 'building-does-not-exist', 58: 'power-full',
                 59: 'entered-or-overflown-by', 60: 'tech-type-exists-n', 61: 'tech-type-does-not-exist'}
EXPECT_ACTIONS = {1: 'winner-is', 2: 'loser-is', 9: 'fire-sale', 53: 'enable-trigger', 67: 'announce-win',
                  105: 'talk-bubble', 106: 'set-tech-level', 108: 'create-crate', 109: 'iron-curtain-at',
                  112: 'center-camera-at', 117: 'play-ingame-movie-pause', 119: 'destroy-all-of',
                  125: 'create-building-at', 145: 'jump-camera-to-base'}


def main():
    bad = []
    for table, expect, what in ((m.EVENTS, EXPECT_EVENTS, 'event'), (m.ACTIONS, EXPECT_ACTIONS, 'action')):
        for k, v in expect.items():
            if table.get(k) != v:
                bad.append('%s %d is %r, want %r' % (what, k, table.get(k), v))
    if max(m.EVENTS) != m.LAST['yr'][0] or max(m.ACTIONS) != m.LAST['yr'][1]:
        bad.append('tables end at %d/%d, gamemd.exe at %d/%d' % (max(m.EVENTS), max(m.ACTIONS), *m.LAST['yr']))
    # Two events: a TechType count (code 2: number, then the type) and a building that does not exist (code 0).
    got = m.events('2,60,2,3,HTNK,57,0,14')
    if got != ['tech-type-exists-n(2,3,HTNK)', 'building-does-not-exist(0,14)']:
        bad.append('events parsed as %r' % got)
    if m.events('1,58,0,8', 'ra2') != ['event58(0,8)']:
        bad.append('game.exe has no event 58: %r' % m.events('1,58,0,8', 'ra2'))
    if m.actions('1,118,0,0,0,0,0,0,A', {}, 'ra2') != ['action118(0,0,0,0,0,0,A)']:
        bad.append('game.exe has no action 118')
    got = m.actions('2,108,0,5,0,0,0,0,C;17,0,2,0,0,0,0,A'.replace(';', ','), {'2': '50040'})
    if got != ['create-crate(0,5,0,0,0,0,C)[wp2@40,50]', 'reveal-around-waypoint(0,2,0,0,0,0,A)[wp2@40,50]']:
        bad.append('actions parsed as %r' % got)
    for line in bad:
        print('mapinfo_tables: ' + line)
    print('mapinfo_tables: %s' % ('FAIL' if bad else 'ok'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
