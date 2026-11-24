// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P2, branch ports-on-rebuild): appended to the generated beacon unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// beacon.h declares the compiled copy of trect.h's Intersect at 0x0041FDF0 (the one VC6 left out of
// line for its callers) as a plain function; the rebuild carries no row for it, so the copy is the
// template itself, instantiated for int.
// ---------------------------------------------------------------------------------------------
Rect const __fastcall Intersect(Rect const & bounding_rect, Rect const & draw_rect, int * x, int * y)
{
	return Intersect<int>(bounding_rect, draw_rect, x, y);
}
