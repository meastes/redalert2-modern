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

Yuri's Revenge (lane P50, ra2_step.py start --game yr): the same helpers; the game's 800x600 screen is read from the
step server's view line (Game.yr), so the view's middle, the band box's bounds and the sidebar's sell and repair
buttons ("@sell", "@repair", yr/glue/yr_stepserver.cpp) follow it. YR's types: DEFENCES and DEFENSE_TYPES include
Yuri's (YAGGUN, NATBNK, YAPSYT, NABNKR ...).

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
            m = re.match(r'^view .* tactical (\d+)x(\d+)', line)
            if m:
                self.tactical = (int(m.group(1)), int(m.group(2)))
                self.yr = self.tactical[0] < 700   # Yuri's Revenge's 800x600 screen (lane P50)
        if self.yr:
            w, h = self.tactical
            self.mid = '%d %d' % (w * 2 // 3, h * 2 // 3)     # a free spot in the view (deselect's right click)
            self.park_at = '%d %d' % (w // 2 + 40, h // 2 + 30)
            self.sell_spec, self.repair_spec = '@sell', '@repair'

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
    yr = False
    tactical = (856, 768)
    mid = '428 368'          # RA2's 1024x768 screen
    park_at = '470 400'
    sell_spec = '958 180'
    repair_spec = '914 180'

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
        self.cmd('input move %s' % self.park_at)
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

    def select(self, objs, team=None, add=False, tries=3):
        """Selects the objects by clicks (the first plain unless add, the rest with Shift); team: Ctrl+<n> after.

        Lane P75: each click is aimed where the object is now, not where the caller's list last saw it, and the
        selection is checked afterwards: objects left unselected are clicked again (with Shift, from fresh positions),
        up to `tries` rounds. A click on an object outside the view after its look is no click at all (the step
        server's "no point"), so a list read some hundred ticks earlier, with the units moving, selected one of 35
        Apocalypses in YR Soviet 7 (lane P73), and Ctrl+9 (gamemd.exe 0x00731060: every selected unit of the player's
        joins the group) made a team of that one. Objects are Obj or IDs; IDs no longer on the map are skipped.
        Returns the listed objects still unselected (empty when all are selected)."""
        want = [o.id if isinstance(o, Obj) else int(o) for o in objs]
        if not want:
            return []

        def where(i):
            return {o.id: o for o in self.objects('all')}.get(i)

        left = []
        for round_ in range(max(1, tries)):
            now = {o.id: o for o in self.objects('all')}
            todo = [i for i in want if i in now and not (round_ and now[i].sel)]
            if round_ == 0 and not add and todo:
                # a click on another controlled unit while one is selected can be an order (an infantry's move onto a
                # friendly infantry): the selection dropped first (a right click), then a plain click
                self.deselect()
                first = where(todo[0])
                if first is not None:
                    self.click_obj(first)
                todo = todo[1:]
            if todo:
                def rest():
                    for i in todo:
                        cur = where(i)
                        # a Shift-click on a selected object takes it out of the selection
                        if cur is not None and not cur.sel:
                            self.click_obj(cur)
                self.with_key('Left_Shift', rest)
            now = {o.id: o for o in self.objects('all')}
            left = [now[i] for i in want if i in now and not now[i].sel]
            if not left:
                break
        if team is not None:
            self.with_key('Left_Ctrl', self.key, str(team))
        return left

    def team(self, n):
        self.key(str(n))

    def move(self, cell):
        """The selection ordered to a cell (a left click on the ground)."""
        self.click_cell(cell)

    def attack(self, target):
        self.click_obj(target)

    def force_fire(self, target):
        self.with_key('Left_Ctrl', self.click_obj, target)

    def force_cell(self, cell):
        """Force fire (Ctrl with a left click) at a cell: a big building whose own click is taken as a move (Allied 3's
        Psychic Beacon) is attacked through one of its cells (lane P23)."""
        self.with_key('Left_Ctrl', self.click_cell, cell)

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
        self.raw_click(self.mid, right=True, wait=12)

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
    x1 = min(850 if not g.yr else g.tactical[0] - 2, max(p[0] for p in pts) + 30)
    y1 = min(730 if not g.yr else g.tactical[1] - 2, max(p[1] for p in pts) + 20)
    g.send('input move %d %d' % (x0, y0), 'input +2 ldown %d %d' % (x0, y0), 'input +5 move %d %d' % ((x0 + x1) // 2, (y0 + y1) // 2),
           'input +8 move %d %d' % (x1, y1), 'input +11 lup %d %d' % (x1, y1))
    g.adv(15)
    if team is not None:
        g.with_key('Left_Ctrl', g.key, str(team))
    return len(g.selected())


SELL_BUTTON = '958 180'   # the sidebar's sell ($) button at the 1024x768 screen the runs use


def sell(g, o, wait=200):
    """A building sold through the sidebar's sell button; returns the credits after."""
    g.look(o.x, o.y)
    g.raw_click(g.sell_spec)
    g.adv(5)
    g.raw_click('@^%d' % o.id)
    g.adv(wait)
    g.raw_click(g.mid, right=True)
    return g.state()['credits']


def war(g, houses, home, home_r, def_team, off_team, near=None, skip=(), ticks=20000, step=120, def_name=None,
        def_kind='vehicle', def_want=0, off_name=None, off_kind='vehicle', log_every=10):
    """Defence and offence interleaved: each round the defence team (def_team) is sent at the enemy unit nearest home
    within home_r (and kept topped up to def_want units of def_name), and the offence team at the houses' object
    nearest near (or home). Ends when the houses have nothing left (True) or after ticks (False)."""
    end = g.tick + ticks
    known = None
    current = None
    since = 0
    banned = {}
    r = 0
    while g.tick < end:
        st = g.state()
        objs = st['objs']
        mine = [o for o in objs if o.own]
        if def_name:
            have = [o for o in mine if o.type == def_name]
            if known is None:
                known = {o.id for o in have}
            new = [o for o in have if o.id not in known]
            if new:
                g.select(new)
                g.with_key('Left_Ctrl', g.key, str(def_team))
                known |= {o.id for o in new}
            if len(have) < def_want and def_kind not in st['factory'] and st.get('credits', 0) >= 300:
                g.queue(def_name, 1, st)
        en = [o for o in objs if not o.own and not o.ally and dist(o, home) <= home_r and o.kind != 'bld' and o.s > 0
              and not o.house.startswith(('Civ', 'Neutral', 'Special'))]
        if en:
            g.team(def_team)
            g.attack(min(en, key=lambda o: dist(o, home)))
        targets = [o for o in objs if o.house in houses and o.type not in skip and o.s > 0]
        if not targets:
            print(g.tick, 'none left')
            return True
        ref = near or home
        pool = [o for o in targets if banned.get(o.id, 0) < g.tick] or targets
        tgt = min(pool, key=lambda o: dist(o, ref))
        since = since + 1 if current == tgt.id else 0
        if current != tgt.id or since % 5 == 4:     # the order renewed now and then (a team can drop it)
            g.team(off_team)
            g.attack(tgt)
            current = tgt.id
        if since >= 15:                             # out of reach for now: another target first
            banned[tgt.id] = g.tick + 3000
            since = 0
        g.adv(step)
        r += 1
        if r % log_every == 0:
            print(g.tick, 'left', len(targets), 'target', tgt, 'home threats', len(en))
    return False


DEFENCES = ('NALASR', 'NATSLA', 'NAFLAK', 'GAPILL', 'ATESLA', 'GAPRIS', 'NABNKR', 'GTGCAN', 'NAPSIS',
            'YAGGUN', 'YAPSYT', 'NATBNK', 'GAFWLL', 'YACOMD')   # Yuri's Revenge (lane P50): gattling and psychic towers


def fight_along(g, team, waypoints, radius=8, step=60, ticks_per_leg=3000, ground_defences=('NALASR', 'NATSLA',
                'GAPILL', 'ATESLA', 'GAPRIS', 'GTGCAN'), types=None, quiet=False, houses=()):
    """Team <n> walked along waypoints in short legs (lane P23): at each check the team's living members are counted
    (types: the unit types that make up the team), and the enemy unit or ground defence nearest the team's centre
    within radius is focus-fired; when none is near, the team is moved on to the leg's waypoint. A leg ends once most
    of the team stands within three cells of its waypoint with no threat near. houses: other houses whose units the
    player commands (PlayerControl=yes in the map, as Allied 5's Tanya). Returns the team's members left."""
    types = types or ('MTNK',)
    ids = None
    for wp in waypoints:
        end = g.tick + ticks_per_leg
        moving = False
        while g.tick < end:
            objs = g.objects('all')
            mem = [o for o in objs if (o.own or o.house in houses) and o.type in types and o.kind != 'bld']
            if ids is None:
                ids = {o.id for o in mem}
            mem = [o for o in mem if o.id in ids]
            if not mem:
                return []
            cx = sorted(o.x for o in mem)[len(mem) // 2]
            cy = sorted(o.y for o in mem)[len(mem) // 2]
            en = [o for o in objs if not o.own and not o.ally and o.s > 0 and dist(o, (cx, cy)) <= radius
                  and not o.house.startswith(('Civ', 'Neutral', 'Special'))
                  and (o.kind != 'bld' or o.type in ground_defences) and o.type not in ('HARV', 'CMIN')]
            if en:
                tgt = min(en, key=lambda o: (o.kind == 'bld', dist(o, (cx, cy))))
                g.team(team)
                g.attack(tgt)
                moving = False
            else:
                near = [o for o in mem if dist(o, wp) <= 3]
                if len(near) * 3 >= len(mem) * 2:
                    break
                if not moving:
                    g.team(team)
                    g.move(wp)
                    moving = True
            g.adv(step)
        if not quiet:
            print('fight_along %s: tick %d, team %d' % (wp, g.tick, len(mem)))
    return mem


def defend_step(g, center, radius, team, objs=None):
    """One home-defence check (lane P23): team <n> ordered at the enemy unit nearest center within radius (infantry
    first: an engineer can capture a construction yard). Returns the target or None."""
    objs = objs if objs is not None else g.objects('all')
    en = [o for o in objs if not o.own and not o.ally and o.kind != 'bld' and o.s > 0 and dist(o, center) <= radius
          and not o.house.startswith(('Civ', 'Neutral', 'Special'))]
    if not en:
        return None
    tgt = min(en, key=lambda o: (o.type != 'ENGINEER', dist(o, center)))
    g.team(team)
    g.attack(tgt)
    return tgt


def base_defended(g, plan, center, team, radius=10, r=6, step=60, timeout=6000):
    """base() with a home defence (lane P23): while each building is bought, team <n> answers threats near center
    (defend_step); then it is placed near its cell. plan: (type, (x, y) or None)."""
    out = []
    for name, at in plan:
        st = g.state()
        f = st['factory'].get('building')
        if not (f and f[0] == name):
            if not g.queue(name, 1, st):
                print('base_defended: %s not on the sidebar' % name)
                out.append(None)
                continue
        end = g.tick + timeout
        while g.tick < end:
            st = g.state()
            f = st['factory'].get('building')
            if not f or (f[0] == name and f[1] >= 100):
                break
            defend_step(g, center, radius, team, st['objs'])
            g.adv(step)
        o = g.place(name, ring(at or center, 0, r))
        print('build %s -> %s at tick %d' % (name, o, g.tick))
        out.append(o)
    return out


def siege(g, team, wp, houses, reach=8, types=('DEST',), step=90, ticks=4000, quiet=False, prefer=()):
    """Team <n> (of types) at waypoint wp shells the houses' buildings within reach of the team's centre, nearest
    first (prefer: types taken before the rest), by force fire on each building's cell (a building the player cannot see is not clicked; lane P23); enemy
    units within reach are fired at first. Ends when nothing of the houses is in reach or after ticks."""
    end = g.tick + ticks
    g.team(team)
    g.move(wp)
    g.adv(step)
    cur = None
    while g.tick < end:
        objs = g.objects('all')
        mem = [o for o in objs if o.own and o.type in types and dist(o, wp) <= reach + 6]
        if not mem:     # still on the way (only the members near the waypoint count, not those left at home)
            g.adv(step)
            continue
        cx = sorted(o.x for o in mem)[len(mem) // 2]
        cy = sorted(o.y for o in mem)[len(mem) // 2]
        units = [o for o in objs if o.house in houses and o.kind != 'bld' and o.s > 0 and dist(o, (cx, cy)) <= reach]
        blds = [o for o in objs if o.house in houses and o.kind == 'bld' and o.s > 0 and dist(o, (cx, cy)) <= reach]
        if units:
            t = min(units, key=lambda o: dist(o, (cx, cy)))
            if cur != t.id:
                g.team(team)
                g.attack(t)
                cur = t.id
        elif blds:
            t = min(blds, key=lambda o: (o.type not in prefer, dist(o, (cx, cy))))
            if cur != t.id:
                g.team(team)
                g.force_cell(t.cell)
                cur = t.id
        elif sum(1 for o in mem if dist(o, wp) <= 3) * 2 >= len(mem):
            break
        else:
            g.team(team)
            g.move(wp)
            cur = None
        g.adv(step)
    if not quiet:
        print('siege %s: tick %d, team %d' % (wp, g.tick, len([o for o in g.mine() if o.type in types])))
    return [o for o in g.mine() if o.type in types]


# -- lane P24 helpers -------------------------------------------------------------------------------------------------
def toward(a,b,n):
    dx=b[0]-a[0]; dy=b[1]-a[1]; d=max(abs(dx),abs(dy))
    if d<=n: return b
    return (a[0]+dx*n//d, a[1]+dy*n//d)
def escort(g, legs, houses, teams, radius=5, leg=3, step=40, prio=('DOG',), timeout=4000, skip=('CTECH',), vip='TANY'):
    """teams: list of (team number, types). Short legs; every check each team fires at the nearest threat (prio types
    first) to its own centre; the whole party stops while any threat is in radius."""
    end=g.tick+timeout
    for wp in legs:
        while g.tick<end:
            objs=g.objects('all')
            party=[o for o in objs if (o.own or o.house in houses) and o.kind!='bld']
            if not any(o.type==vip for o in party) and vip: return False
            en=[o for o in objs if not o.own and not o.ally and o.s>0 and o.kind!='bld' and o.type not in skip and o.seen
                and not o.house.startswith(('Civ','Neutral','Special'))]
            fired=False
            for tm in teams:
                tn,types=tm[0],tm[1]
                if len(tm)>2 and tm[2]=='nofire': continue
                mem=[o for o in party if o.type in types]
                if not mem: continue
                cx=sorted(o.x for o in mem)[len(mem)//2]; cy=sorted(o.y for o in mem)[len(mem)//2]
                near=[o for o in en if dist(o,(cx,cy))<=radius]
                if near:
                    t=min(near,key=lambda o:(o.type not in prio, dist(o,(cx,cy))))
                    g.team(tn); g.attack(t); fired=True
                    if getattr(g,"dbg",0): print(g.tick,"fire",tn,t,t.house,flush=True)
            if not fired:
                mem=[o for o in party if o.type in teams[0][1]]
                cx=sorted(o.x for o in mem)[len(mem)//2]; cy=sorted(o.y for o in mem)[len(mem)//2]
                if dist((cx,cy),wp)<=2: break
                nxt=toward((cx,cy),wp,leg)
                for tm in teams:
                    g.team(tm[0]); g.move(nxt)
            g.adv(step)
        print('escort',wp,g.tick,' '.join(repr(o) for o in g.objects('all') if (o.own or o.house in houses) and o.kind!='bld'),flush=True)
    return True
def cellgrid(g, cx, cy, r):
    """{(x,y): token} from the step server's cell listing."""
    out={}
    for line in g.cmd('cell %d %d %d'%(cx,cy,r)):
        m=re.match(r'^row (-?\d+): (.*)$',line)
        if not m: continue
        y=int(m.group(1)); toks=m.group(2).split()
        for i,t in enumerate(toks): out[(cx-r+i,y)]=t
    return out
def free_spots(g, cx, cy, r, w=4, h=4, pad=1):
    """Cells whose w x h block (plus pad) is clear flat land with no occupant (no 'o', no letters after the m)."""
    gr=cellgrid(g,cx,cy,r)
    def ok(t, h0):
        return t is not None and t[0] in '.r' and t[1:2]==h0 and 'o' not in t[2:] and not any(ch.isupper() for ch in t[2:])
    res=[]
    for (x,y),t in gr.items():
        h0=t[1:2]
        if all(ok(gr.get((x+dx,y+dy)),h0) for dx in range(-pad-1,w-1+pad) for dy in range(-pad-1,h-1+pad)):
            res.append((x,y))
    res.sort(key=lambda p:max(abs(p[0]-cx),abs(p[1]-cy)))
    return res
def vip_kill(g, team, vid, targets, guard=6, step=30, timeout=1500, avoid=('CTECH','HARV','CIV1','CIV2','CIV3')):
    """A fragile unit (Tanya) sent at each target in turn; every step it first shoots enemy infantry (dogs first)
    within guard cells. Returns False if it dies."""
    for tid in targets:
        end=g.tick+timeout; cur=None
        while g.tick<end:
            objs=g.objects('all'); byid={o.id:o for o in objs}
            v=byid.get(vid)
            if not v: print('vip dead',g.tick); return False
            if tid not in byid: break
            en=[o for o in objs if not o.own and not o.ally and o.kind=='inf' and o.s>0 and dist(o,v)<=guard and o.type not in avoid]
            if en:
                t=min(en,key=lambda o:(o.type!='DOG',dist(o,v)))
                if cur!=t.id: g.team(team); g.attack(t); cur=t.id
            elif cur!=tid:
                g.team(team)
                if byid[tid].type.startswith('CAMISC'):     # barrels: force fire
                    g.force_fire(byid[tid])
                else:
                    g.attack(byid[tid])
                cur=tid
            g.adv(step)
        print('vip target',tid,'gone' if tid not in {o.id for o in g.objects('all')} else 'left',g.tick,flush=True)
    return True
SIZE={'GAPOWR':(2,2),'GAREFN':(4,3),'GAPILE':(2,2),'GAWEAP':(5,3),'GATECH':(2,3),'GAAIRC':(3,3),'GAPILL':(1,1),'ATESLA':(1,1),
      'GAPRIS':(1,1),'GADEPT':(3,3),'GAYARD':(3,3),'NAPOWR':(2,2),'NAREFN':(4,3),'NAHAND':(2,2),'NAWEAP':(5,3),'NARADR':(2,2),
      'NATECH':(2,3),'NALASR':(1,1),'TESLA':(1,1),'NAFLAK':(1,1),'NAYARD':(3,3),'NANRCT':(4,4),'GASPYS':(2,2),'NADEPT':(3,3),'GAOREP':(2,2),'NAINDP':(2,2),'GACSPH':(4,4)}
def place_smart(g, name, near, reach=2, tries=10):
    """A finished building placed on a clear spot near 'near' within reach cells of an own building."""
    w,h=SIZE.get(name,(3,3))
    own=[o for o in g.mine() if o.kind=='bld']
    spots=[p for p in free_spots(g,near[0],near[1],9,w,h,pad=0) if any(dist(p,o)<=reach+max(w,h) for o in own)]
    if not spots: print('place_smart: no spot for',name); return None
    return g.place(name, spots[:tries])
def build_smart(g, name, near, team=None, center=None, radius=12, timeout=4000, step=60, kind='building'):
    st=g.state(); f=st['factory'].get(kind)
    if not (f and f[0]==name):
        if f: print('build_smart: factory busy with',f); return None
        if not g.queue(name,1,st): print('build_smart: %s not on the sidebar'%name); return None
    end=g.tick+timeout
    while g.tick<end:
        st=g.state(); f=st['factory'].get(kind)
        if not f or f[1]>=100: break
        if team: defend_step(g, center, radius, team, st['objs'])
        g.adv(step)
    o=place_smart(g,name,near)
    print('build',name,o,g.tick,flush=True)
    return o
def build_def(g, name, near, step=60, timeout=3000, queued=False):
    """A defence (its factory is not in the state listing): queued, waited for until the credits stop falling, placed."""
    if not queued and not g.queue(name,1): print('build_def: not on sidebar',name); return None
    end=g.tick+timeout; last=None; still=0
    while g.tick<end:
        g.adv(step); c=g.state()['credits']
        still = still+1 if c==last else 0
        last=c
        if still>=2: break
    o=place_smart(g,name,near)
    print('build_def',name,o,g.tick,flush=True)
    return o

def guard(g, teams, ticks, houses=(), radius=7, step=40, prio=('DOG','SHK','ENGINEER'), skip=('CTECH','HARV'), vip=None, quiet=False,
          danger=('TESLA','NALASR'), danger_r=7, center=None):
    """For the given ticks each team fires at the nearest enemy unit (prio types first) within radius of its own centre;
    no moves. teams: (team, types[, 'nofire'])."""
    end=g.tick+ticks
    while g.tick<end:
        objs=g.objects('all')
        party=[o for o in objs if (o.own or o.house in houses) and o.kind!='bld']
        if vip and not any(o.type==vip for o in party): return False
        en=[o for o in objs if not o.own and not o.ally and o.s>0 and o.kind!='bld' and o.type not in skip and o.seen
            and not o.house.startswith(('Civ','Neutral','Special'))]
        dz=[o for o in objs if o.kind=='bld' and o.type in danger and not o.own and not o.ally and o.s>0]
        en=[o for o in en if not any(dist(o,d)<=danger_r for d in dz)]
        for tm in teams:
            if len(tm)>2: continue
            mem=[o for o in party if o.type in tm[1]]
            if not mem: continue
            cx=sorted(o.x for o in mem)[len(mem)//2]; cy=sorted(o.y for o in mem)[len(mem)//2]
            ref=center or (cx,cy)
            near=[o for o in en if dist(o,ref)<=radius]
            if near:
                t=min(near,key=lambda o:(o.type not in prio, dist(o,(cx,cy))))
                g.team(tm[0]); g.attack(t)
                if not quiet: print(g.tick,'guard',tm[0],t,flush=True)
        g.adv(step)
    return True


# -- lane P25 helpers -------------------------------------------------------------------------------------------------
def legal_spots(g, name, near, r=8):
    out=[]
    for l in g.cmd('buildable %s %d %d %d'%(name,near[0],near[1],r)):
        m=re.match(r'^place (-?\d+) (-?\d+) ok',l)
        if m: out.append((int(m.group(1)),int(m.group(2))))
        # lane P75: YR's step server reports the footprint ("buildable X n legal size WxH"); it replaces the SIZE guess
        m=re.match(r'^buildable \S+ \d+ legal size (\d+)x(\d+)',l)
        if m and int(m.group(1)) > 0: FOOTPRINT[name]=(int(m.group(1)),int(m.group(2)))
    return out
FOOTPRINT={}   # lane P75: footprints the step server reported (legal_spots)
def place_legal(g, name, near, r=8, avoid=None, tries=3, pick=None):
    """A finished building placed on a legal origin (the step server's buildable) nearest near: the selection dropped,
    tab, icon, one click on the cell that puts the footprint's origin there. avoid(origin)->True skips an origin."""
    sp=legal_spots(g,name,near,r)
    w,h=FOOTPRINT.get(name) or SIZE.get(name,(3,3))
    if avoid: sp=[s for s in sp if not avoid(s)]
    if not sp: print('place_legal: no legal spot for',name,'near',near,flush=True); return None
    sp.sort(key=pick or (lambda s: max(abs(s[0]+w//2-near[0]),abs(s[1]+h//2-near[1]))))
    before={o.id for o in g.mine() if o.type==name}
    for o0 in sp[:tries]:
        t=g.tab_of(name)
        if t is None: print('place_legal: not on sidebar',name); return None
        # the click cell: the game places a building's origin at the release cell plus its ZoneOffset, which
        # DisplayClass::Set_Cursor_Shape sets to -(dim - Cell(1,1)) / 2 (TPoint2D's division truncates; dim the occupy
        # list's extent, Get_Occupy_Dimensions), so the click is origin + ((w-1)//2, (h-1)//2): 1x1 and 2x2 +0, 3x3 and
        # 4x4 +1 (lane P47; P36's +dim/2 was one cell long for the 4x4 chronosphere). origin + dim//2 is kept as a
        # fallback for an elevated cell whose centre pixel the click resolves to a neighbour.
        cells=[(o0[0]+(w-1)//2, o0[1]+(h-1)//2)]
        if (w//2,h//2)!=((w-1)//2,(h-1)//2): cells.append((o0[0]+w//2, o0[1]+h//2))
        for c in cells:
            g.raw_click(g.mid, right=True)
            g.raw_click('@tab=%d'%t); g.raw_click('@cameo=%s'%name)
            g.look(*c); g.raw_click('@=%d,%d'%c); g.adv(2)
            now=[o for o in g.mine() if o.type==name and o.id not in before]
            if now: break
        now=[o for o in g.mine() if o.type==name and o.id not in before]
        if now: print('placed',name,o0,now[0],g.tick,flush=True); return now[0]
    g.raw_click(g.mid, right=True)
    print('place_legal: failed',name,sp[:tries],g.tick,flush=True)
    return None
def wait_built(g, kind='building', timeout=4000, step=30, during=None):
    end=g.tick+timeout
    while g.tick<end:
        st=g.state(); f=st['factory'].get(kind)
        if not f or f[1]>=100: return f
        if during: during(st)
        g.adv(step)
    return None
def build_legal(g, name, near, r=8, timeout=4000, during=None, **kw):
    st=g.state(); f=st['factory'].get('building')
    if not (f and f[0]==name):
        if f: print('build_legal: factory busy with',f); return None
        if not g.queue(name,1,st): print('build_legal: %s not on the sidebar'%name); return None
    wait_built(g,'building',timeout,during=during)
    return place_legal(g,name,near,r,**kw)


# -- lane P26/P27 helpers (folded in from the lanes' build folders) -----------------------------------------------------
DEFENSE_TYPES = {'TESLA', 'NAFLAK', 'NALASR', 'NAWALL', 'NAIRON', 'NAMISL', 'NAPSIS',   # BuildCat=Combat: the defense queue
                 'ATESLA', 'GAPILL', 'NASAM', 'GAWALL', 'GACSPH', 'GAWEAT',
                 'GAFWLL', 'NATBNK', 'YAGGUN', 'YAPSYT', 'GAWALL', 'YAWALL', 'NAGATE', 'GAGATE', 'YAGATE'}   # Yuri's Revenge (lane P50)


class Mgr:
    """A base run one step a call (lane P26): the building plan (type, near[, r]) bought and placed on legal origins
    (defense types through the defense queue), the infantry, vehicle, aircraft and ship factories kept on their wants
    ([(type, count)], first short one first), new units of the types in rally walked to their cell, and with defend a
    home defence (all combatants at the threat nearest home within home_r, engineers and spies first)."""

    def __init__(self, g, plan, home, home_r=12, inf=None, veh=None, defend=True, air=None, navy=None):
        self.g = g
        self.plan = [i for i in plan if i[0] not in DEFENSE_TYPES]
        self.dplan = [i for i in plan if i[0] in DEFENSE_TYPES]
        self.home, self.home_r = home, home_r
        self.inf, self.veh, self.air, self.navy = inf or [], veh or [], air or [], navy or []
        self.defend = defend
        self.fails = 0
        self.last_def = -999
        self.rally = {}
        self.known = None
        self.def_types = None

    @staticmethod
    def want_next(wants, mine):
        for t, n in wants:
            if sum(1 for o in mine if o.type == t) < n:
                return t, n
        return None

    def step(self, st=None):
        g = self.g
        st = st or g.state()
        mine = [o for o in st['objs'] if o.own]
        for q, fk in ((self.plan, 'building'), (self.dplan, 'defense')):
            if not q:
                continue
            f = st['factory'].get(fk)
            item = q[0]
            name, near = item[0], item[1]
            r = item[2] if len(item) > 2 else 8
            if f and f[0] == name and f[1] >= 100:
                o = place_legal(g, name, near, r)
                if o is None:
                    o = place_legal(g, name, near, r + 6)
                if o is not None or self.fails > 3:
                    q.pop(0)
                    self.fails = 0
                else:
                    self.fails += 1
                st = g.state()
            elif f and f[1] >= 100 and f[0] != name:
                # a finished building that is not the plan's head (left from an earlier plan): placed near home
                if place_legal(g, f[0], self.home, 10) is None:
                    place_legal(g, f[0], self.home, 16)
                st = g.state()
            elif not f:
                if g.queue(name, 1, st):
                    print(g.tick, 'queued', name, 'credits', st.get('credits'), flush=True)
                elif len(q) > 1:
                    # not on the sidebar now (a prerequisite missing or lost: Soviet 7's flak cannon after the
                    # barracks fell): the next item first, this one later
                    q.append(q.pop(0))
        for kind, wants in (('infantry', self.inf), ('vehicle', self.veh), ('aircraft', self.air), ('ship', self.navy)):
            if not wants or kind in st['factory']:
                continue
            # the first short want that is on the sidebar now (an Allied harvester is CMIN, not HARV; a type whose
            # factory or prerequisite is gone waits)
            w = self.want_next([tw for tw in wants if g.tab_of(tw[0], st) is not None], mine)
            if w and st.get('credits', 0) >= 200:
                g.queue(w[0], 1, st)
        if self.rally:
            have = [o for o in mine if o.type in self.rally]
            if self.known is None:
                self.known = {o.id for o in have}
            for o in have:
                if o.id not in self.known:
                    self.known.add(o.id)
                    g.select([o])
                    g.move(self.rally[o.type])
                    g.deselect()
        if self.defend:
            en = [o for o in st['objs'] if not o.own and not o.ally and o.kind != 'bld' and o.s > 0
                  and dist(o, self.home) <= self.home_r and not o.house.startswith(('Civ', 'Neutral', 'Special'))
                  and (not self.def_types or o.type in self.def_types)]
            if en and g.tick - self.last_def >= 90:
                tgt = min(en, key=lambda o: (o.type not in ('ENGINEER', 'SENGINEER', 'GHOST', 'JUMPJET', 'SPY'),
                                             dist(o, self.home)))
                if select_all_combat(g):
                    g.attack(tgt)
                g.deselect()
                self.last_def = g.tick
        return st

    def run(self, ticks, step=30, every=None):
        end = self.g.tick + ticks
        while self.g.tick < end:
            st = self.step()
            if every:
                every(st)
            self.g.adv(step)
            ls = [l for l in self.g.take_news() if ' lost ' in l and (' %s ' % self.g.player) in l]
            if ls:
                print(self.g.tick, 'LOST', ' ; '.join(l.split(' ', 3)[3] for l in ls)[:300], flush=True)


def capture(g, eng_type, targets, quiet=False):
    """Each idle engineer of eng_type sent into the nearest target building (ids) not yet the player's; returns the
    orders given (lane P26). Neutral animals (Allied 9's and Soviet 2's alligators) kill engineers on the way: clear
    them first."""
    objs = g.objects('all')
    engs = [o for o in objs if o.own and o.type == eng_type]
    tg = [o for o in objs if o.id in targets and not o.own]
    n = 0
    for e in engs:
        if not tg:
            break
        t = min(tg, key=lambda o: dist(o, e))
        tg.remove(t)
        g.select([e])
        g.attack(t)
        g.deselect()
        n += 1
        if not quiet:
            print(g.tick, 'capture', e, '->', t, flush=True)
    return n


REPAIR_BUTTON = '914 180'   # the sidebar's repair (wrench) button at the 1024x768 screen the runs use


def repair_all(g, frac=0.75, types=None, done=None):
    """Own buildings below frac of their strength put under repair (the wrench, then a click on each), each once until
    healed (lane P26). done: the set of ids already under repair (kept on g by default)."""
    if done is None:
        done = g.__dict__.setdefault('_repairing', set())
    mine = g.mine()
    for o in mine:
        if o.kind == 'bld' and o.s < o.smax * frac and o.type != 'CAOILD' and (types is None or o.type in types) \
                and o.id not in done:
            g.look(o.x, o.y)
            g.raw_click(g.repair_spec)
            g.adv(2)
            g.raw_click('@^%d' % o.id)
            g.adv(2)
            g.raw_click(g.mid, right=True)
            done.add(o.id)
    for o in mine:
        if o.kind == 'bld' and o.s >= o.smax:
            done.discard(o.id)


def fight_legs(g, team, waypoints, radius=8, step=60, ticks_per_leg=3000, ground_defences=DEFENCES + ('TESLA',),
               types=('HTNK',), quiet=False, houses=(), every=None, ids=None, recruit=None, skip=('HARV', 'CMIN'),
               extra=None, buildings=(), prefer=()):
    """fight_along with more hooks (lane P27): every(g) is called at each check (a Mgr step, a home defence), ids fixes
    the team's members (default: the units of types now), recruit(objs) returns units to add to the team, extra(objs,
    cx, cy) returns targets force-fired by cell first (barrels beside enemy buildings), and buildings names houses whose
    every building is a target (after units and ground defences; prefer types first; one out of sight is force-fired
    by its cell). Returns the team's members left."""
    mem = []
    for wp in waypoints:
        end = g.tick + ticks_per_leg
        moving = False
        cur = None
        while g.tick < end:
            if every:
                every(g)
            objs = g.objects('all')
            mem = [o for o in objs if (o.own or o.house in houses) and o.type in types and o.kind != 'bld']
            if ids is None:
                ids = {o.id for o in mem}
            if recruit:
                new = [o for o in recruit(objs) if o.id not in ids]
                if new:
                    g.team(team)
                    g.select(new, add=True)
                    g.with_key('Left_Ctrl', g.key, str(team))
                    ids |= {o.id for o in new}
                    moving = False
            mem = [o for o in mem if o.id in ids]
            if not mem:
                return []
            cx = sorted(o.x for o in mem)[len(mem) // 2]
            cy = sorted(o.y for o in mem)[len(mem) // 2]
            ex = extra(objs, cx, cy) if extra else []
            en = [o for o in objs if not o.own and not o.ally and o.s > 0 and dist(o, (cx, cy)) <= radius
                  and not o.house.startswith(('Civ', 'Neutral', 'Special'))
                  and (o.kind != 'bld' or o.type in ground_defences or o.house in buildings) and o.type not in skip]
            if ex:
                t = ex[0]
                if cur != ('x', t.id):
                    g.team(team)
                    g.force_cell(t.cell)
                    cur = ('x', t.id)
                moving = False
            elif en:
                t = min(en, key=lambda o: (o.kind == 'bld' and o.type not in ground_defences,
                                           o.kind == 'bld' and o.type not in prefer, dist(o, (cx, cy))))
                if cur != t.id:
                    g.team(team)
                    if t.kind == 'bld' and not t.seen:
                        g.force_cell(t.cell)
                    else:
                        g.attack(t)
                    cur = t.id
                moving = False
            else:
                cur = None
                near = [o for o in mem if dist(o, wp) <= 3]
                if len(near) * 3 >= len(mem) * 2:
                    break
                if not moving:
                    g.team(team)
                    g.move(wp)
                    moving = True
            g.adv(step)
        if not quiet:
            print('fight_legs %s: tick %d, team %d' % (wp, g.tick, len(mem)), flush=True)
    return mem


def party_legs(g, teams, waypoints, radius=7, step=50, ticks_per_leg=2500, houses=(), every=None,
               defences=DEFENCES + ('TESLA',), skip=('HARV', 'CMIN'), quiet=False, lead=0, zone=None):
    """A mixed party walked along waypoints in short legs (lane P27): teams is [(n, types, prio_types,
    hits_buildings)]; at each check every team fires at the threat (an enemy unit, or a ground defence when
    hits_buildings) nearest the lead team's centre within radius, its prio types first; with no threat the teams move on
    to the waypoint. zone(obj) -> False leaves a threat alone (a party chasing a target across a street runs into the
    next nest: Soviet 5). houses: other houses the player commands (PlayerControl=yes). Returns {team: members}."""
    ids = {}
    out = {}
    for wp in waypoints:
        end = g.tick + ticks_per_leg
        moving = False
        cur = {}
        while g.tick < end:
            if every:
                every(g)
            objs = g.objects('all')
            mems = {}
            for tm in teams:
                n, types = tm[0], tm[1]
                m = [o for o in objs if (o.own or o.house in houses) and o.type in types and o.kind != 'bld']
                if n not in ids:
                    ids[n] = {o.id for o in m}
                mems[n] = [o for o in m if o.id in ids[n]]
            allm = [o for v in mems.values() for o in v]
            if not allm:
                return mems
            ld = mems.get(teams[lead][0]) or allm
            cx = sorted(o.x for o in ld)[len(ld) // 2]
            cy = sorted(o.y for o in ld)[len(ld) // 2]
            en = [o for o in objs if not o.own and not o.ally and o.s > 0 and dist(o, (cx, cy)) <= radius
                  and not o.house.startswith(('Civ', 'Neutral', 'Special', 'Other')) and o.type not in skip
                  and (o.kind != 'bld' or o.type in defences) and (zone is None or zone(o))]
            fired = False
            for n, types, prio, hits_buildings in teams:
                if not mems[n] or prio is None:     # prio None: a team that only follows (engineers)
                    continue
                pool = [o for o in en if o.kind != 'bld' or hits_buildings]
                if not pool:
                    continue
                t = min(pool, key=lambda o: (o.type not in prio, o.kind == 'bld', dist(o, (cx, cy))))
                if cur.get(n) != t.id:
                    g.team(n)
                    if t.kind == 'bld' and not t.seen:
                        g.force_cell(t.cell)
                    else:
                        g.attack(t)
                    cur[n] = t.id
                fired = True
            if fired:
                moving = False
            else:
                cur = {}
                near = [o for o in ld if dist(o, wp) <= 3]
                if len(near) * 3 >= len(ld) * 2:
                    break
                if not moving:
                    for tm in teams:
                        if mems[tm[0]]:
                            g.team(tm[0])
                            g.move(wp)
                    moving = True
            g.adv(step)
        out = mems
        if not quiet:
            print('party %s: tick %d, %s' % (wp, g.tick, ' '.join('%d:%d' % (k, len(v)) for k, v in mems.items())),
                  flush=True)
    return out


def walk_route(g, oid, route, step=40, timeout=4000):
    """One unit walked along a route cell by cell (lane P27: the pathfinder's shortest way can run through an enemy
    nest; Soviet 5's tesla troopers died at 70,64 until walked round it). Returns the unit or None."""
    end = g.tick + timeout
    for wp in route:
        get_o = get(g, oid)
        if get_o is None:
            return None
        g.select([get_o])
        g.move(wp)
        g.deselect()
        while g.tick < end:
            g.adv(step)
            o = get(g, oid)
            if o is None:
                return None
            if dist(o, wp) <= 1:
                break
    return get(g, oid)


def park_ring(g, x0, y0, x1, y1, types=('APOC', 'HTNK'), objs=None):
    """The player's tanks of types moved one to each cell of the ring round a building's footprint (x0..x1, y0..y1),
    nearest first (lane P27: Soviet 7's lab, so that engineers and spies cannot reach it). Returns {unit id: cell};
    re-issue the moves for strays now and then."""
    ring = [(x, y) for x in range(x0 - 1, x1 + 2) for y in range(y0 - 1, y1 + 2)
            if x in (x0 - 1, x1 + 1) or y in (y0 - 1, y1 + 1)]
    pool = [o for o in (objs if objs is not None else g.mine()) if o.own and o.type in types]
    park = {}
    for c in ring:
        if not pool:
            break
        t = min(pool, key=lambda o: dist(o, c))
        pool.remove(t)
        park[t.id] = c
        g.select([t])
        g.move(c)
        g.deselect()
    return park
