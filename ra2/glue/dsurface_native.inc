// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P4, branch ports-on-rebuild): appended to the generated dsurface unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// 0x004AC650 DSurface::Draw_Depth_Antialiased_Line (slot 15): the laser beam (LaserDrawClass::Draw_It:
// Prism towers, laser weapons) and Draw_Radial_Indicator's spokes. Main's behaviour body
// (native/link/behaviour_m23_surface.cpp, M25 Scout1202, written from the original's code) under the
// rebuild's names; the rebuild keeps TibSun's text under #if 0. Main's buffer names map as: its
// abuf (DepthBuffer, ZBuffer) Area/MaxValue/Width/Wrap_Pointer are the rebuild's Get_Bounds(),
// Get_Scroll(), Get_Buffer_Width(), Wrap_Overflow/Wrap_Underflow; its zbuf is AlphaBuffer (ABuffer).
// Kept from the original (main's notes): the falloff table T[i] = ftol(((i/128 - 1)^3 + 1) * 128)
// built once; the clip rectangle (the surface shrunk by one pixel, intersected with cliprect; points
// relative to its origin, sorted by x, clipped, a clipped end's depth interpolated by length); a 3D
// Bresenham with x-major, y-major and two depth-major bodies, two-pixel antialiasing; per pixel only
// where the line's depth is below the depth buffer's and the alpha buffer's value is not zero, each
// enabled channel ((T[256-w] * back + T[w] * colour) * alpha) >> 15, clamped to 255. Difference: w is
// clamped to 0..256 (the original indexes past the table for an intensity above 1).
// ---------------------------------------------------------------------------------------------
#include "zbuffer.h"
#include "abuffer.h"

int RB_Draw_AA_Line_Count = 0;   // lane P5: calls of the body below, read by the harness (harness_native.inc)

namespace {
int RB_AA_Falloff[257];
bool RB_AA_Falloff_Ready;

inline int RB_AA_Channel(int back, int color, int w, int zvalue)
{
	return ((RB_AA_Falloff[256 - w] * back + RB_AA_Falloff[w] * color) * zvalue) >> 15;
}

inline int RB_AA_Weight(int weight, float intensity)
{
	int w = (int)(weight * (double)intensity);
	return w < 0 ? 0 : (w > 256 ? 256 : w);
}

inline void RB_AA_Unpack(unsigned short pixel, int & r, int & g, int & b)
{
	r = (pixel >> DSurface::RedRight) << DSurface::RedLeft;
	g = (pixel >> DSurface::GreenRight) << DSurface::GreenLeft;
	b = (pixel >> DSurface::BlueRight) << DSurface::BlueLeft;
}

void RB_AA_Blend_Pair(unsigned short * first, unsigned short * second, bool swap, int w1, int w2, int zvalue,
                      RGBClass const & color, bool red, bool green, bool blue)
{
	int r0, g0, b0, rr1, gg1, bb1;
	RB_AA_Unpack(*first, r0, g0, b0);
	RB_AA_Unpack(*second, rr1, gg1, bb1);
	int r1 = swap ? rr1 : r0, g1 = swap ? gg1 : g0, b1 = swap ? bb1 : b0;
	int r2 = swap ? r0 : rr1, g2 = swap ? g0 : gg1, b2 = swap ? b0 : bb1;
	if (red) {
		r1 = RB_AA_Channel(r1, color.Get_Red(), w1, zvalue);
		r2 = RB_AA_Channel(r2, color.Get_Red(), w2, zvalue);
	}
	if (green) {
		g1 = RB_AA_Channel(g1, color.Get_Green(), w1, zvalue);
		g2 = RB_AA_Channel(g2, color.Get_Green(), w2, zvalue);
	}
	if (blue) {
		b1 = RB_AA_Channel(b1, color.Get_Blue(), w1, zvalue);
		b2 = RB_AA_Channel(b2, color.Get_Blue(), w2, zvalue);
	}
	*first = (unsigned short)DSurface::Build_Hicolor_Pixel(r1 > 255 ? 255 : r1, g1 > 255 ? 255 : g1, b1 > 255 ? 255 : b1);
	*second = (unsigned short)DSurface::Build_Hicolor_Pixel(r2 > 255 ? 255 : r2, g2 > 255 ? 255 : g2, b2 > 255 ? 255 : b2);
}

template<class B>
inline void RB_AA_Next(B * buffer, unsigned short *& pointer)
{
	pointer = (unsigned short *)buffer->Wrap_Overflow((uintptr_t)(pointer + 1));
}

template<class B>
inline void RB_AA_Row(B * buffer, unsigned short *& pointer, int step)
{
	pointer += step;
	pointer = (unsigned short *)(step > 0 ? buffer->Wrap_Overflow((uintptr_t)pointer) : buffer->Wrap_Underflow((uintptr_t)pointer));
}
}

bool DSurface::Draw_Depth_Antialiased_Line(Rect const & cliprect, Point2D const & startpoint, Point2D const & endpoint,
                                           RGBClass & color, int start_depth, int end_depth, bool write_depth,
                                           bool blend_red, bool blend_green, bool blend_blue, float intensity)
{
	++RB_Draw_AA_Line_Count;   // lane P5: the combat check's count (harness_native.inc `draws`)
	if (!RB_AA_Falloff_Ready) {
		RB_AA_Falloff_Ready = true;
		for (int i = 0; i <= 256; ++i) {
			double t = i * 0.0078125 - 1.0;
			RB_AA_Falloff[i] = (int)((t * t * t + 1.0) * 128.0);
		}
	}
	Rect surface = Get_Rect();
	Rect shrunk(surface.X + 1, surface.Y + 1, surface.Width - 2, surface.Height - 2);
	Rect clip = Intersect(cliprect, shrunk);
	if (clip.Width == 0 && clip.Height == 0) {
		return false;
	}
	Point2D p0(clip.X + startpoint.X, clip.Y + startpoint.Y);
	Point2D p1(clip.X + endpoint.X, clip.Y + endpoint.Y);
	int z0 = start_depth, z1 = end_depth;
	if (p0.X > p1.X) {
		Point2D t = p0; p0 = p1; p1 = t;
		int tz = z0; z0 = z1; z1 = tz;
	}
	Point2D const p0o = p0, p1o = p1;
	if (!Clip_Line_To_Rect(p0, p1, clip)) {
		return false;
	}
	int zs = z0, ze = z1;
	if (p1.X != p1o.X || p1.Y != p1o.Y) {
		int full = Point2D(p0o.X - p1o.X, p0o.Y - p1o.Y).Length();
		int part = Point2D(p0o.X - p1.X, p0o.Y - p1.Y).Length();
		int d = (int)((double)part / full * (double)(z0 - z1));
		d = d < 0 ? -d : d;
		ze = z0 < z1 ? z0 + d : z0 - d;
	}
	if (p0.X != p0o.X || p0.Y != p0o.Y) {
		int full = Point2D(p0o.X - p1o.X, p0o.Y - p1o.Y).Length();
		int part = Point2D(p0.X - p1o.X, p0.Y - p1o.Y).Length();
		int d = (int)((double)part / full * (double)(z0 - z1));
		d = d < 0 ? -d : d;
		zs = z1 > z0 ? z1 - d : z1 + d;
	}
	unsigned short * row = (unsigned short *)Lock(Point2D(p0.X, p0.Y));
	ZBuffer * dbuf = DepthBuffer;
	ABuffer * abuf = AlphaBuffer;
	if (dbuf == NULL || abuf == NULL) {
		Unlock();
		return false;
	}
	unsigned short * dptr = (unsigned short *)dbuf->Get_Buffer_Offset(Point2D(p0.X, p0.Y - dbuf->Get_Bounds().Y));
	unsigned short * aptr = (unsigned short *)abuf->Get_Buffer_Offset(Point2D(p0.X - abuf->Get_Bounds().X, p0.Y - abuf->Get_Bounds().Y));
	int bufstep = dbuf->Get_Buffer_Width();
	int rowdepth = -1;
	int depth = (unsigned short)(dbuf->Get_Scroll() + dbuf->Get_Bounds().Y) - p0.Y - clip.Y + zs;
	if (row == NULL) {
		Unlock();
		return false;
	}
	int dx = p1.X - p0.X;
	int dy = p1.Y - p0.Y;
	int dz = ze - zs;
	int pitch = Stride();
	if (dy < 0) {
		pitch = -pitch;
		bufstep = -bufstep;
		rowdepth = 1;
	}
	int zsign = dz < 0 ? -1 : 1;
	int ady = dy < 0 ? -dy : dy;
	int adz = dz < 0 ? -dz : dz;
	int rowpixels = pitch / 2;
	if (dx == 0 && ady == 0) {
		Unlock();
		return false;
	}
	int w1 = 256, w2 = 0;
	auto pixel = [&](unsigned short * first, unsigned short * second, bool swap) {
		int zvalue = *aptr;
		if ((unsigned short)depth < *dptr && zvalue != 0) {
			RB_AA_Blend_Pair(first, second, swap, RB_AA_Weight(w1, intensity), RB_AA_Weight(w2, intensity), zvalue, color,
			                 blend_red, blend_green, blend_blue);
			if (write_depth) {
				*dptr = (unsigned short)depth;
			}
		}
	};
	if (adz > dx && adz > ady) {
		if (dx > ady) {
			int step = (int)((double)ady / dx * 256.0);
			for (int i = 0; i < dx; i += 2) {
				pixel(row + i, row + i + rowpixels, true);
				w1 -= step;
				w2 += step;
				if (w2 > 256) {
					w1 += 256;
					w2 -= 256;
					row = (unsigned short *)((char *)row + pitch);
					RB_AA_Row(dbuf, dptr, bufstep);
					RB_AA_Row(abuf, aptr, bufstep);
					depth += rowdepth;
				}
				RB_AA_Next(dbuf, dptr);
				RB_AA_Next(abuf, aptr);
				depth += zsign;
			}
		} else {
			int step = (int)((double)dx / ady * 256.0);
			int column = 0;
			for (int n = ady; n > 0; --n) {
				pixel(row + column, row + column + 1, false);
				w1 -= step;
				w2 += step;
				if (w2 > 256) {
					w1 += 256;
					w2 -= 256;
					++column;
					RB_AA_Next(dbuf, dptr);
					RB_AA_Next(abuf, aptr);
				}
				row = (unsigned short *)((char *)row + pitch);
				RB_AA_Row(dbuf, dptr, bufstep);
				RB_AA_Row(abuf, aptr, bufstep);
				depth += zsign + rowdepth;
			}
		}
	} else if (dx > ady) {
		int step = (int)((double)ady / dx * 256.0);
		int error = 2 * adz - dx;
		for (int i = 0; i < dx; ++i) {
			pixel(row + i, row + i + rowpixels, true);
			w1 -= step;
			w2 += step;
			if (w2 > 256) {
				w1 += 256;
				w2 -= 256;
				row = (unsigned short *)((char *)row + pitch);
				RB_AA_Row(dbuf, dptr, bufstep);
				RB_AA_Row(abuf, aptr, bufstep);
				depth += rowdepth;
			}
			if (error > 0) {
				depth += zsign;
				error -= 2 * dx;
			}
			error += 2 * adz;
			RB_AA_Next(dbuf, dptr);
			RB_AA_Next(abuf, aptr);
		}
	} else {
		int step = (int)((double)dx / ady * 256.0);
		int error = 2 * adz - ady;
		int column = 0;
		for (int n = ady; n > 0; --n) {
			pixel(row + column, row + column + 1, false);
			w1 -= step;
			w2 += step;
			if (w2 > 256) {
				w1 += 256;
				w2 -= 256;
				++column;
				RB_AA_Next(dbuf, dptr);
				RB_AA_Next(abuf, aptr);
			}
			if (error > 0) {
				depth += zsign;
				error -= 2 * ady;
			}
			error += 2 * adz;
			row = (unsigned short *)((char *)row + pitch);
			RB_AA_Row(dbuf, dptr, bufstep);
			RB_AA_Row(abuf, aptr, bufstep);
			depth += rowdepth;
		}
	}
	Unlock();
	return true;
}
