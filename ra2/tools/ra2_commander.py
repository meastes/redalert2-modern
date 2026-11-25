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

OBJ = re.compile(r'^obj (\d+) (\w+) (\S+) (.+?) @(-?\d+),(-?\d+) s(-?\d+)/(\d+) m(-?\d+)(.*)$')


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
        for line in self.cmd('state'):      # the current tick (the scenario line's loop field) and the player's house
            m = re.match(r'^scenario \S+ loop (\d+)', line)
            if m:
                self.tick = int(m.group(1))
            m = re.match(r'^player (.+?) credits', line)
            if m:
                self.player = m.group(1)

    # -- transport -------------------------------------------------------------------------------------------------
    def send(self, *commands):
        if self.over:   # after the end the score screen follows: the server answers no more
            raise GameOver(self.over)
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
            m = re.match(r'^player (.+?) credits (-?\d+) power (-?\d+) drain (-?\d+) input (\w+)', l)
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
        """An attack move (the game's Can_Force_Order: the force-attack and select keys held, Ctrl+Shift, with a left
        click on the ground). The command bar's AttackMove button is not on the bar in every state."""
        self.look(*cell)
        self.send('input keydown Left_Ctrl', 'input keydown Left_Shift')
        self.adv(1)
        self.raw_click('@=%d,%d' % cell)
        self.send('input keyup Left_Shift', 'input keyup Left_Ctrl')
        self.adv(2)

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


# -- mission helpers (lane P22) ----------------------------------------------------------------------------------------
def get(g, oid):
    """The object with that id now, or None."""
    for o in g.objects('all'):
        if o.id == oid:
            return o
    return None


def go_until(g, oid, cell, r=1, step=60, timeout=3000, show=True):
    """Advances until the object is within r cells of cell (or gone, or the timeout)."""
    end = g.tick + timeout
    o = None
    while g.tick < end:
        g.adv(step)
        o = get(g, oid)
        if o is None:
            print('dead', oid, g.tick)
            return None
        if dist(o, cell) <= r:
            break
    if show:
        print(g.tick, o, o.m)
    return o


def kill_list(g, team, ids, step=60, timeout=2500, verbose=True):
    """Team <n> ordered at each target in turn, each waited for until gone (or the timeout)."""
    for tid in ids:
        t = get(g, tid)
        if t is None:
            continue
        g.team(team)
        g.attack(t)
        end = g.tick + timeout
        t2 = t
        while g.tick < end:
            g.adv(step)
            t2 = get(g, tid)
            if t2 is None:
                break
        if verbose:
            print(g.tick, 'target', t, 'gone' if t2 is None else 'left %d' % t2.s)


def exterminate(g, houses, teams, step=120, rounds=200, prefer=None, near=None, skip=()):
    """Each round every team is ordered at the target nearest a point (or the houses' first object), until the houses
    have nothing left on the map. prefer: types taken first; skip: types left alone."""
    for r in range(rounds):
        objs = [o for o in g.objects('all') if o.house in houses and o.type not in skip]
        if not objs:
            print(g.tick, 'none left')
            return True
        ref = near or (objs[0].x, objs[0].y)
        if prefer:
            pref = [o for o in objs if o.type in prefer]
            if pref:
                objs = pref
        tgt = min(objs, key=lambda o: dist(o, ref))
        for t in teams:
            g.team(t)
            g.attack(tgt)
        end = g.tick + 900
        while g.tick < end:
            g.adv(step)
            if get(g, tgt.id) is None:
                break
        if r % 5 == 0:
            print(g.tick, 'round', r, 'left', len(objs), 'target', tgt)
    return False


def produce(g, kind, name, ticks, step=200, keep=5, want=None, rally=None, door=None):
    """For the given ticks keeps the factory of kind ('vehicle', 'infantry', ...) building name: whenever it is idle a
    batch of keep is queued (credits allowing). want: stop once the player has that many of name. rally: new units
    are walked there (units left at a factory's door block its exit: the next one is refunded instead of built)."""
    end = g.tick + ticks
    known = None
    while g.tick < end:
        st = g.state()
        have = [o for o in st['objs'] if o.own and o.type == name]
        if known is None:
            known = {o.id for o in have}
        new = [o for o in have if o.id not in known]
        if rally and door == 'all':
            if new:
                select_all_combat(g)
                g.move(rally)
        elif rally and door:
            idle = [o for o in have if dist(o, door) <= 4]
            if len(idle) >= 3 and band_select(g, idle):
                g.move(rally)
        elif rally and new:
            g.select(new)
            g.move(rally)
        known |= {o.id for o in new}
        if want is not None and len(have) >= want:
            break
        if kind not in st['factory'] and st.get('credits', 0) >= 300:
            g.queue(name, keep, st)
        g.adv(step)
    return len([o for o in g.mine() if o.type == name])


def threats(g, center, radius, objs=None):
    objs = objs if objs is not None else g.enemy()
    return [o for o in objs if dist(o, center) <= radius and o.kind != 'bld' and o.s > 0]


def hold(g, center, radius, team, ticks, step=60, produce_kind=None, produce_name=None, keep=5, want=None,
         rally=None, log=True):
    """For the given ticks: whenever an enemy unit comes within radius of center, team <n> is ordered at the nearest
    one (focus fire); new units of produce_name are added to the team as they appear; the factory kept busy.
    Returns the losses seen (news lines)."""
    end = g.tick + ticks
    known = None
    lost = []
    while g.tick < end:
        st = g.state()
        objs = st['objs']
        mine = [o for o in objs if o.own]
        if produce_name:
            have = [o for o in mine if o.type == produce_name]
            if known is None:
                known = {o.id for o in have}
            new = [o for o in have if o.id not in known]
            if new:
                g.team(team)
                g.select(new, add=True)
                g.with_key('Left_Ctrl', g.key, str(team))
                known |= {o.id for o in new}
            if (want is None or len(have) < want) and produce_kind not in st['factory'] and st.get('credits', 0) >= 300:
                g.queue(produce_name, keep, st)
        en = threats(g, center, radius, [o for o in objs if not o.own and not o.ally])
        if en:
            tgt = min(en, key=lambda o: dist(o, center))
            g.team(team)
            g.attack(tgt)
        elif rally:
            pass
        g.adv(step)
        ls = [l for l in g.take_news() if ' lost ' in l and g.player and (' %s ' % g.player) in l]
        if ls and log:
            print(g.tick, '; '.join(l.split(' ', 3)[3] for l in ls)[:300])
        lost += ls
    return lost


def deploy_mcv(g, mcv_type):
    """The player's MCV selected and deployed where it stands (a click on it, then D)."""
    m = [o for o in g.mine() if o.type == mcv_type]
    if not m:
        return None
    g.select(m[:1])
    g.key('D', wait=30)
    g.adv(120)
    return [o for o in g.mine() if o.kind == 'bld']


def base(g, plan, center, r=6):
    """Buildings bought and placed in order: plan is a list of (type, (x, y) or None); None places near center."""
    out = []
    for name, at in plan:
        o = g.build(name, ring(at or center, 0, r))
        out.append(o)
    return out


def select_all_combat(g, team=None):
    """P (select combatants on the screen), and P again for the whole map, then Ctrl+<team>."""
    g.deselect()
    g.key('P', wait=4)
    g.key('P', wait=4)
    if team is not None:
        g.with_key('Left_Ctrl', g.key, str(team))
    return len(g.selected())


def push(g, cell, ticks=1500, step=150, radius=6, regroup=True, quiet=False):
    """All combat units (or team 1 when regroup is off) attack-moved to cell; waits until no enemy unit or building is
    left within radius of cell and the units are near, or the ticks run out. Returns the player's unit count."""
    if regroup:
        select_all_combat(g, 1)
    else:
        g.team(1)
    g.attack_move(cell)
    end = g.tick + ticks
    while g.tick < end:
        g.adv(step)
        objs = g.objects('all')
        en = [o for o in objs if not o.own and not o.ally and dist(o, cell) <= radius and o.s > 0
              and not o.house.startswith(('Civ', 'Neutral', 'Special'))]
        near = [o for o in objs if o.own and o.kind != 'bld' and dist(o, cell) <= radius]
        if not en and near:
            break
    units = [o for o in g.mine() if o.kind != 'bld']
    if not quiet:
        lost = [l for l in g.take_news() if ' lost ' in l]
        print('push %s: tick %d, units %d, losses since last %d' % (cell, g.tick, len(units), len(lost)))
    return len(units)


def pixel(g, cell):
    """The view-relative pixel of a cell's centre now (probe), or None when it is not in the view."""
    for line in g.cmd('probe %d %d' % tuple(cell)):
        m = re.match(r'^probe rel (-?\d+),(-?\d+) .* rect (-?\d+),(-?\d+) (\d+)x(\d+)', line)
        if m:
            x, y, w, h = int(m.group(1)), int(m.group(2)), int(m.group(5)), int(m.group(6))
            if 0 <= x < w and 0 <= y < h:
                return (x + int(m.group(3)), y + int(m.group(4)))
    return None


def band_select(g, objs, team=None):
    """The player's units among objs selected with one band box over their cells (the view centred on them first);
    returns the selection count. Only the player's own units are picked up by a band."""
    if not objs:
        return 0
    cx = sum(o.x for o in objs) // len(objs)
    cy = sum(o.y for o in objs) // len(objs)
    g.deselect()
    g.look(cx, cy)
    pts = [p for p in (pixel(g, o.cell) for o in objs) if p]
    if not pts:
        return 0
    x0 = max(2, min(p[0] for p in pts) - 30)
    y0 = max(2, min(p[1] for p in pts) - 40)
    x1 = min(850, max(p[0] for p in pts) + 30)
    y1 = min(730, max(p[1] for p in pts) + 20)
    g.send('input move %d %d' % (x0, y0), 'input +2 ldown %d %d' % (x0, y0), 'input +5 move %d %d' % ((x0 + x1) // 2, (y0 + y1) // 2),
           'input +8 move %d %d' % (x1, y1), 'input +11 lup %d %d' % (x1, y1))
    g.adv(15)
    if team is not None:
        g.with_key('Left_Ctrl', g.key, str(team))
    return len(g.selected())
