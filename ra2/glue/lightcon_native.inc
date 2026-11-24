// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P2, branch ports-on-rebuild): appended to the generated lightcon unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// Adjust_Color_565 (0x00797440) and Adjust_Color_555 (0x0079777A) are the library's handwritten
// assembly (winasm.asm's ADJUST_COLOR macro). Main's native bodies (native/link/behaviour_launch.cpp,
// unknown_build_tint_table_00797440 / _0079777a, read from their listings) carried over: entry 0 is
// 0; entry i scales the palette's red, green and blue bytes by the tints when the mask is set, else
// all three by the intensity (16.16 fixed point, saturated to 255), packed 5:6:5 or 5:5:5. The
// original's MMX and CMOV paths are not taken with the port's CPU flags (getcpu_native.inc).
// Adjust_Color_556 and _655 stay traps until a run reaches them.
// ---------------------------------------------------------------------------------------------
static void Adjust_Color_Native(void const * palette, unsigned short * table, int red, int green, int blue, int level,
	bool const * remap, bool six_green)
{
	unsigned char const * rgb = (unsigned char const *)palette;
	table[0] = 0;
	for (int i = 1; i < 256; ++i) {
		unsigned int r_scale = remap[i] ? red : level;
		unsigned int g_scale = remap[i] ? green : level;
		unsigned int b_scale = remap[i] ? blue : level;
		unsigned int r = ((unsigned int)rgb[3 * i] * r_scale) >> 16;
		unsigned int g = ((unsigned int)rgb[3 * i + 1] * g_scale) >> 16;
		unsigned int b = ((unsigned int)rgb[3 * i + 2] * b_scale) >> 16;
		if (r > 255) r = 255;
		if (g > 255) g = 255;
		if (b > 255) b = 255;
		table[i] = six_green ? (unsigned short)((r & 0xF8) << 8 | (g & 0xFC) << 3 | b >> 3)
		                     : (unsigned short)((r & 0xF8) << 7 | (g & 0xF8) << 2 | b >> 3);
	}
}

extern "C" void __cdecl Adjust_Color_565(void * palette, void * translator, int red_tint, int green_tint, int blue_tint, int intensity, bool * tint_mask)
{
	Adjust_Color_Native(palette, (unsigned short *)translator, red_tint, green_tint, blue_tint, intensity, tint_mask, true);
}

extern "C" void __cdecl Adjust_Color_555(void * palette, void * translator, int red_tint, int green_tint, int blue_tint, int intensity, bool * tint_mask)
{
	Adjust_Color_Native(palette, (unsigned short *)translator, red_tint, green_tint, blue_tint, intensity, tint_mask, false);
}
