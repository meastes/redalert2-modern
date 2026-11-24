// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P4, branch ports-on-rebuild): appended to the generated xsurface unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// 0x007746C0 XSurface::Draw_Ellipse (slot 8; DSurface and BSurface inherit it): main's behaviour body
// (native/link/behaviour_m23_surface.cpp, lane N1, checked against the original's instructions
// 0x007746C0-0x00774C83) under the rebuild's name; the rebuild keeps TibSun's text under #if 0 (it is
// not RA2's walk). Reached by BuildingClass::Draw_Radial_Indicator (a selected or placed building's
// range circle). The original walks the outline from both axes: (cx, cy +- ry) first, x stepped from
// the top and bottom by the midpoint rule until 2*b2*x >= 2*a2*y; then (cx +- rx, cy), y stepped from
// the sides until 2*a2*y > 2*b2*x. 32-bit wrapping products, a2/4 and b2/4 toward zero, no
// negative-radius guard (the original has none). Pixels through Put_Pixel inside the clip rectangle.
// ---------------------------------------------------------------------------------------------
int RB_Draw_Ellipse_Count = 0;   // lane P5: calls of the body below, read by the harness (harness_native.inc)
bool XSurface::Draw_Ellipse(Point2D center, int rx, int ry, Rect cliprect, int color)
{
	++RB_Draw_Ellipse_Count;
	auto put = [&](int x, int y) {
		if (x >= cliprect.X && x < cliprect.X + cliprect.Width && y >= cliprect.Y && y < cliprect.Y + cliprect.Height) {
			Point2D p;
			p.X = x;
			p.Y = y;
			Put_Pixel(p, color);
		}
	};
	auto put4 = [&](int dx, int dy) {
		put(center.X + dx, center.Y + dy);
		put(center.X - dx, center.Y - dy);
		put(center.X + dx, center.Y - dy);
		put(center.X - dx, center.Y + dy);
	};
	auto mul = [](unsigned a, unsigned b) { return a * b; };
	unsigned const b2 = mul(ry, ry);
	unsigned const a2 = mul(rx, rx);

	put(center.X, center.Y + ry);
	put(center.X, center.Y - ry);
	{
		int x = 0, y = ry;
		unsigned px = 0, py = 2 * mul(a2, ry);
		unsigned d = (unsigned)((int)a2 / 4) - mul(a2, ry);
		for (;;) {
			d += px + b2;
			if ((int)d >= 0) {
				py -= 2 * a2;
				d -= py;
				--y;
			}
			px += 2 * b2;
			++x;
			if ((int)px >= (int)py) break;
			put4(x, y);
		}
	}
	put(center.X + rx, center.Y);
	put(center.X - rx, center.Y);
	{
		int x = rx, y = 0;
		unsigned px = 0, py = 2 * mul(b2, rx);
		unsigned d = (unsigned)((int)b2 / 4) - mul(b2, rx);
		for (;;) {
			d += px + a2;
			if ((int)d >= 0) {
				py -= 2 * b2;
				d -= py;
				--x;
			}
			px += 2 * a2;
			++y;
			if ((int)px > (int)py) break;
			put4(x, y);
		}
	}
	return true;
}
