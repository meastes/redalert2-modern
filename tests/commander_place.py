#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ra2/tools/ra2_commander.py's place_legal aims by the footprint the step server reports (lane P75; no game needed).

place_legal clicks origin + ((w-1)//2, (h-1)//2), the cell the game turns back into the origin (ZoneOffset), with w x h
from the SIZE table, 3x3 when a type is missing. YR Allied 4's robot control centre (GAROBO, 2x2) was missing, so every
click went one cell off and the finished building blocked the build queue for thousands of ticks; GATECH stood as 2x3
for a 3x2 footprint. yr/glue/yr_stepserver.cpp's buildable now ends "buildable <type> <n> legal size WxH" (the occupy
list's extent) and place_legal uses that. This runs place_legal against a fake step server that places a building only
when the click lands on origin + ZoneOffset for its real footprint, with and without the size line.
"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'ra2', 'tools'))
import ra2_commander as rc  # noqa: E402


class FakeServer:
    def __init__(self, name, size, report):
        self.name, self.size, self.report = name, size, report
        self.tick = 100
        self.placed = None
        self.armed = False

    def one(self, c):
        w = c.split()
        if c == 'state':
            return ['scenario ALL04DMD.MAP loop %d game %d over 0' % (self.tick, self.tick),
                    'player Americans credits 0 power 0 drain 0 input open',
                    'view 0,0 centre @0,0 tactical 632x568 selected 0',
                    'sidebar tab 0: GAPOWR %s' % self.name]
        if w[0] == 'objects':
            return ['obj 7 bld %s Americans @%d,%d s600/600 m5 own seen' % ((self.name,) + self.placed)] if self.placed else []
        if w[0] == 'buildable':
            out = ['place 80 150 ok']
            out.append('buildable %s 1 legal size %dx%d' % (self.name, self.size[0], self.size[1]) if self.report
                       else 'buildable %s 1 legal' % self.name)
            return out
        if w[0] == 'advance':
            self.tick += int(w[1])
            return ['tick %d loop %d game %d' % (self.tick, self.tick, self.tick)]
        if w[0] in ('look', 'save'):
            return []
        if w[0] == 'input':
            if w[1] == '+2' and w[2] == 'ldown':
                if w[3] == '@cameo=%s' % self.name:
                    self.armed = True
                m = re.match(r'@=(\d+),(\d+)$', w[3])
                if m and self.armed:
                    x, y = int(m.group(1)), int(m.group(2))
                    ox, oy = x - (self.size[0] - 1) // 2, y - (self.size[1] - 1) // 2   # the game's ZoneOffset
                    if (ox, oy) == (80, 150):
                        self.placed = (ox, oy)
                    self.armed = False
            return []
        raise AssertionError('unexpected command %r' % c)

    def send(self, work, commands):
        return [self.one(c) for c in commands]


def run(name, size, report):
    rc.FOOTPRINT.clear()
    fake = FakeServer(name, size, report)
    rc.ra2_step.send = fake.send
    g = rc.Game('/nonexistent')
    return rc.place_legal(g, name, (81, 151), r=4, tries=1) is not None


def main():
    bad = []
    for name, size in (('GAROBO', (2, 2)), ('GATECH', (3, 2)), ('GAWEAP', (5, 3)), ('GAPOWR', (2, 2))):
        ok = run(name, size, True)
        print('%s %dx%d with the size line: %s' % (name, size[0], size[1], 'placed' if ok else 'not placed'))
        if not ok:
            bad.append(name)
    if run('GAROBO', (2, 2), False):
        bad.append('GAROBO placed without the size line (the test does not model the fault)')
    for b in bad:
        print('FAIL: ' + b)
    print('commander-place: ' + ('FAIL' if bad else 'ok'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
