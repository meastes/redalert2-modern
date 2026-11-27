#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ra2/tools/ra2_commander.py's Game.select with moving units (lane P75; no game needed).

In YR Soviet 7 (lane P73) 35 Apocalypses, ordered to move in groups of six, were then selected from the list read
before the moves and given Ctrl+9: one unit joined team 9. gamemd.exe's team create (0x00731060, uicmnds.cpp
FUN_00731060, compared) puts every selected unit of the player's in the group; the fault was the harness's: each click
was aimed (look) where the stale list put the unit, the unit was outside the view by then, and the step server dropped
the click ("no point"). This runs select() against a fake step server whose units move a cell every 20 ticks, whose
clicks resolve only for objects in the view, where Shift-clicks toggle and Ctrl+<n> assigns the selection, and checks
that every unit ends in the team (and that the old aim, at the stale cells, loses most of them).
"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'ra2', 'tools'))
import ra2_commander as rc  # noqa: E402


class FakeServer:
    VIEW = 4   # cells from the view's centre that a click resolves in

    def __init__(self, n):
        self.tick = 1000
        self.centre = (0, 0)
        self.keys = set()
        self.units = {}
        for k in range(n):
            self.units[2000 + k] = {'x': 150 + k % 6, 'y': 150 + k // 6, 'sel': False, 'group': -1, 'moving': True}

    def pos(self, u):
        if not u['moving']:
            return u['x'], u['y']
        return u['x'] - (self.tick - 1000) // 20, u['y'] - (self.tick - 1000) // 20

    def click(self, spec):
        m = re.match(r'@\^(\d+)$', spec)
        u = self.units.get(int(m.group(1))) if m else None
        if u is None:
            return
        x, y = self.pos(u)
        if max(abs(x - self.centre[0]), abs(y - self.centre[1])) > self.VIEW:
            return   # the step server's "no point": no click
        if 'Left_Shift' in self.keys:
            u['sel'] = not u['sel']
        else:
            for v in self.units.values():
                v['sel'] = False
            u['sel'] = True

    def one(self, c):
        w = c.split()
        if c == 'state':
            return ['scenario SOV07TMD.MAP loop %d game %d over 0' % (self.tick, self.tick),
                    'player Player House credits 0 power 0 drain 0 input open',
                    'view 0,0 centre @%d,%d tactical 632x568 selected 0' % self.centre]
        if w[0] == 'objects':
            out = []
            for i, u in self.units.items():
                x, y = self.pos(u)
                out.append('obj %d veh APOC Player House @%d,%d s800/800 m2 own seen%s' % (i, x, y, ' sel' if u['sel'] else ''))
            return out
        if w[0] == 'look':
            self.centre = (int(w[1]), int(w[2]))
            return []
        if w[0] == 'advance':
            self.tick += int(w[1])
            return ['tick %d loop %d game %d' % (self.tick, self.tick, self.tick)]
        if w[0] == 'input':
            if w[1] == 'keydown':
                self.keys.add(w[2])
            elif w[1] == 'keyup':
                self.keys.discard(w[2])
            elif w[1] == 'key' and 'Left_Ctrl' in self.keys:
                n = int(w[2])
                for u in self.units.values():   # gamemd.exe 0x00731060
                    if u['group'] == n - 1:
                        u['group'] = -1
                    if u['sel']:
                        u['group'] = n - 1
            elif w[1] == '+2' and w[2] == 'ldown':
                self.click(w[3])
            elif w[1] == '+2' and w[2] == 'rdown':
                for u in self.units.values():
                    u['sel'] = False
            return []
        raise AssertionError('unexpected command %r' % c)

    def send(self, work, commands):
        return [self.one(c) for c in commands]


def run(n):
    fake = FakeServer(n)
    rc.ra2_step.send = fake.send
    g = rc.Game('/nonexistent')
    stale = g.objects('all')          # read before the units move
    g.adv(600)                         # 30 cells later
    old = FakeServer(n)                # the old aim: at the stale cells
    old.tick = fake.tick
    rc.ra2_step.send = old.send
    g.deselect()
    with_old = 0
    for o in stale:
        g.look(o.x, o.y)
        old.keys.add('Left_Shift')
        old.click('@^%d' % o.id)
        with_old += 1
    old_hit = sum(1 for u in old.units.values() if u['sel'])
    rc.ra2_step.send = fake.send
    g._view = None
    left = g.select(stale, team=9)
    in_team = sum(1 for u in fake.units.values() if u['group'] == 8)
    return old_hit, in_team, left or []


def main():
    bad = []
    for n in (6, 35):
        old_hit, in_team, left = run(n)
        print('%d units: the stale aim selects %d; select() puts %d in team 9, %d left' % (n, old_hit, in_team, len(left)))
        if in_team != n or left:
            bad.append('%d units: %d in team 9, %d left unselected' % (n, in_team, len(left)))
        if old_hit >= n:
            bad.append('%d units: the stale aim selected all of them (the test does not model the fault)' % n)
    # a unit that does not move, selected among moving ones, and IDs as well as Obj
    fake = FakeServer(8)
    fake.units[2003]['moving'] = False
    rc.ra2_step.send = fake.send
    g = rc.Game('/nonexistent')
    left = g.select([2000 + k for k in range(8)] + [9999], team=3)
    if left or sum(1 for u in fake.units.values() if u['group'] == 2) != 8:
        bad.append('IDs: not all eight in team 3')
    for b in bad:
        print('FAIL: ' + b)
    print('commander-select: ' + ('FAIL' if bad else 'ok'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
