#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Commander helpers over the step server (lane P22; ra2/tools/ra2_step.py, ra2/glue/stepserver_native.inc).

A mission is played by a short Python script per phase that imports this module, talks to the running game and
prints a compact summary. Every input goes through the step server's recorded commands (look, input), so the
recording (step.replay) stays inputs and ticks only and replays the same.

    import ra2_commander as rc
    g = rc.Game('build/p22/all02')
    g.adv(60)
    me = g.mine()
    g.select([o.id for o in me if o.type == 'E1'], team=1)
    g.move_team(1, (100, 60))

Clicks are written as raw pointer events (move, button down two frames later, up four after that), so a click
costs seven ticks and never overlaps the next one; a pointer is parked in the view's middle only on request.
"""
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ra2_step  # noqa: E402

OBJ = re.compile(r'^obj (\d+) (\w+) (\S+) (\S+) @(-?\d+),(-?\d+) s(-?\d+)/(\d+) m(-?\d+)(.*)$')


class Obj:
    __slots__ = ('id', 'kind', 'type', 'house', 'x', 'y', 's', 'smax', 'm', 'sel', 'own', 'ally', 'seen')

    def __init__(self, m):
        self.id = int(m.group(1))
        self.kind = m.group(2)
        self.type = m.group(3)
        self.house = m.group(4)
        self.x, self.y = int(m.group(5)), int(m.group(6))
        self.s, self.smax, self.m = int(m.group(7)), int(m.group(8)), int(m.group(9))
        rest = m.group(10)
        self.sel = ' sel' in rest
        self.own = ' own' in rest
        self.ally = ' ally' in rest
        self.seen = ' seen' in rest

    @property
    def cell(self):
        return (self.x, self.y)

    def __repr__(self):
        return '%s#%d@%d,%d(%d/%d)' % (self.type, self.id, self.x, self.y, self.s, self.smax)


def dist(a, b):
    ax, ay = a.cell if isinstance(a, Obj) else a
    bx, by = b.cell if isinstance(b, Obj) else b
    return max(abs(ax - bx), abs(ay - by))


class GameOver(Exception):
    pass


class Game:
    def __init__(self, work, verbose=False):
        self.work = os.path.abspath(work)
        self.tick = 0
        self.over = None          # 'win' / 'lose' once ended
        self.news = []
        self.verbose = verbose
        self.lost_mine = []

    # -- transport -------------------------------------------------------------------------------------------------
    def send(self, *commands):
        replies = ra2_step.send(self.work, list(commands))
        for c, lines in zip(commands, replies):
            if c.startswith('advance'):
                self._advance_reply(lines)
        return replies

    def cmd(self, command):
        return self.send(command)[0]

    def _advance_reply(self, lines):
        for line in lines:
            m = re.match(r'^tick (\d+) loop \d+ game \d+(.*)$', line)
            if m:
                self.tick = int(m.group(1))
                if 'over win' in m.group(2):
                    self.over = 'win'
                elif 'over lose' in m.group(2):
                    self.over = 'lose'
            elif line.startswith('news'):
                self.news.append(line)
                if ' lost ' in line and self.player and (' %s ' % self.player) in line:
                    self.lost_mine.append(line)
        if self.verbose:
            for line in lines:
                print(line)

    player = None
    look_wait = 3

    # -- time ------------------------------------------------------------------------------------------------------
    def adv(self, n):
        if n <= 0:
            return self.tick
        self.cmd('advance %d' % n)
        if self.over:
            print('GAME OVER: %s at tick %d' % (self.over, self.tick))
            raise GameOver(self.over)
        return self.tick

    def take_news(self, grep=None):
        out = [n for n in self.news if grep is None or re.search(grep, n)]
        self.news = []
        return out

    # -- reading ---------------------------------------------------------------------------------------------------
    def objects(self, which='all'):
        return [Obj(m) for m in (OBJ.match(l) for l in self.cmd('objects ' + which)) if m]

    def mine(self):
        return self.objects('mine')

    def enemy(self):
        return self.objects('enemy')

    def state(self):
        lines = self.cmd('state')
        st = {'objs': [], 'factory': {}, 'tabs': {}, 'houses': [], 'triggers': [], 'lines': lines}
        for l in lines:
            m = OBJ.match(l)
            if m:
                st['objs'].append(Obj(m))
                continue
            m = re.match(r'^player (\S+) credits (-?\d+) power (-?\d+) drain (-?\d+) input (\w+)', l)
            if m:
                st['player'] = m.group(1)
                self.player = m.group(1)
                st['credits'], st['power'], st['drain'] = int(m.group(2)), int(m.group(3)), int(m.group(4))
                st['input'] = m.group(5)
            m = re.match(r'^factory (\w+) (\S+) (\d+)%( on-hold)?', l)
            if m:
                st['factory'][m.group(1)] = (m.group(2), int(m.group(3)))
            m = re.match(r'^sidebar tab (\d):(.*)$', l)
            if m:
                st['tabs'][int(m.group(1))] = m.group(2).split()
            if l.startswith('house '):
                st['houses'].append(l)
            if l.startswith('trig '):
                st['triggers'].append(l)
            m = re.match(r'^timer (-?\d+) frames', l)
            if m:
                st['timer'] = int(m.group(1))
            m = re.match(r'^view (-?\d+),(-?\d+) centre @(-?\d+),(-?\d+)', l)
            if m:
                st['centre'] = (int(m.group(3)), int(m.group(4)))
        return st

    def summary(self, st=None, enemies_near=None):
        st = st or self.state()
        mine = [o for o in st['objs'] if o.own]
        counts = {}
        for o in mine:
            counts[o.type] = counts.get(o.type, 0) + 1
        print('tick %d credits %s power %s/%s input %s factories %s' % (self.tick, st.get('credits'), st.get('power'),
              st.get('drain'), st.get('input'), st['factory']))
        print('mine: ' + ' '.join('%s:%d' % kv for kv in sorted(counts.items())))
        for t, v in sorted(st['tabs'].items()):
            if v:
                print('tab %d: %s' % (t, ' '.join(v)))
        return st

    # -- input -----------------------------------------------------------------------------------------------------
    def look(self, x, y):
        """The view centred on a cell; one frame runs so that the next click's point is resolved against a picture
        drawn with the new view (objects register where they are drawn)."""
        if getattr(self, '_view', None) == (x, y):
            return
        self.cmd('look %d %d' % (x, y))
        self.adv(self.look_wait)
        self._view = (x, y)

    def raw_click(self, spec, right=False, wait=10):
        """A click at a point spec ('@^id', '@=x,y', '@cameo=..', 'X Y'): pointer there, button down two frames
        later and up four frames after that."""
        b = 'r' if right else 'l'
        self.send('input move %s' % spec, 'input +2 %sdown %s' % (b, spec), 'input +6 %sup %s' % (b, spec))
        self.adv(wait)
        self._view = None

    def park(self):
        self.cmd('input move 470 400')
        self.adv(1)

    def click_cell(self, cell, right=False):
        self.look(*cell)
        self.raw_click('@=%d,%d' % cell, right)

    def click_obj(self, o, right=False):
        self.look(o.x, o.y)
        self.raw_click('@^%d' % o.id, right)

    def key(self, name, wait=2):
        self.cmd('input key %s' % name)
        self.adv(wait)

    def with_key(self, mod, fn, *args):
        self.cmd('input keydown %s' % mod)
        self.adv(1)
        try:
            fn(*args)
        finally:
            self.cmd('input keyup %s' % mod)
            self.adv(2)

    def select(self, objs, team=None, add=False):
        """Selects the objects by clicks (the first plain unless add, the rest with Shift); team: Ctrl+<n> after."""
        objs = [o for o in objs]
        if not objs:
            return
        start = 0
        if not add:
            # a click on another controlled unit while one is selected can be an order (an infantry's move onto a
            # friendly infantry): the selection dropped first (a right click), then a plain click
            self.deselect()
            self.click_obj(objs[0])
            start = 1
        if len(objs) > start:
            def rest():
                for o in objs[start:]:
                    self.click_obj(o)
            self.with_key('Left_Shift', rest)
        if team is not None:
            self.with_key('Left_Ctrl', self.key, str(team))

    def team(self, n):
        self.key(str(n))

    def move(self, cell):
        """The selection ordered to a cell (a left click on the ground)."""
        self.click_cell(cell)

    def attack(self, target):
        self.click_obj(target)

    def force_fire(self, target):
        self.with_key('Left_Ctrl', self.click_obj, target)

    def attack_move(self, cell):
        """The command bar's attack move, then the cell."""
        self.raw_click('@cmd=AttackMove')
        self.click_cell(cell)

    def deselect(self):
        self.raw_click('428 368', right=True, wait=12)

    def selected(self):
        return [o for o in self.objects('all') if o.sel]

    # -- production ------------------------------------------------------------------------------------------------
    TAB_OF = {}

    def tab_of(self, name, st=None):
        st = st or self.state()
        for t, v in st['tabs'].items():
            if name in v:
                return t
        return None

    def queue(self, name, count=1, st=None):
        """Clicks a build icon count times (production or queue). Returns False if not buildable now."""
        t = self.tab_of(name, st)
        if t is None:
            return False
        self.raw_click('@tab=%d' % t)
        for _ in range(count):
            self.raw_click('@cameo=%s' % name)
        return True

    def factory(self, kind):
        return self.state()['factory'].get(kind)

    def place(self, name, cells, kind='building'):
        """A finished building placed at the first cell of cells that takes it (tab, icon, cell; repeated)."""
        before = {o.id for o in self.mine() if o.type == name}
        for c in cells:
            t = self.tab_of(name)
            if t is None:
                return None
            self.raw_click('@tab=%d' % t)
            self.raw_click('@cameo=%s' % name)
            self.click_cell(c)
            self.adv(3)
            now = [o for o in self.mine() if o.type == name and o.id not in before]
            if now:
                return now[0]
        # leave placement mode
        self.raw_click('@=%d,%d' % cells[0], right=True)
        return None

    def build(self, name, cells, timeout=6000, kind='building'):
        """A building bought, waited for and placed. Returns the new object or None."""
        st = self.state()
        f = st['factory'].get(kind)
        if not (f and f[0] == name):
            if not self.queue(name, 1, st):
                print('build: %s not on the sidebar' % name)
                return None
        end = self.tick + timeout
        while self.tick < end:
            f = self.factory(kind)
            if f and f[0] == name and f[1] >= 100:
                break
            if not f:
                break
            self.adv(60)
        o = self.place(name, cells)
        print('build %s -> %s at tick %d' % (name, o, self.tick))
        return o

    def save(self, name):
        self.cmd('save %s' % name)

    def wait_until(self, pred, step=60, timeout=6000):
        end = self.tick + timeout
        while self.tick < end:
            if pred():
                return True
            self.adv(step)
        return False


def ring(center, r0=0, r1=8, step=1):
    """Cells around center, nearest first (placement candidates)."""
    cx, cy = center
    out = []
    for r in range(r0, r1 + 1, step):
        for dx in range(-r, r + 1):
            for dy in range(-r, r + 1):
                if max(abs(dx), abs(dy)) == r:
                    out.append((cx + dx, cy + dy))
    return out
