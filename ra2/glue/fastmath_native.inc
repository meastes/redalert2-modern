// SPDX-License-Identifier: GPL-3.0-or-later
// Lane P9 (frame identity): fastmath::atan on the unnarrowed double argument, as the original
// (0x004BA5B0: fld qword value, fdiv dword step, call _ftol) and main's port (Fast_Atan with its
// row 382). Appended to fastmath.cpp by native/rb/fixups.json.
static float rb_atan_d(double f)
{
	float result;
	int i = abs(rb_x87_ftol(f / atan_tab[ATAN_TABLE_SIZE + 1]));

	if (i < ATAN_TABLE_SIZE + 1) {
		result = atan_tab[i & 0x3FFFFFFF];
	} else {
		result = M_PI/2;
	}

	if ((float)f < 0) {
		result = -result;
	}

	return(result);
}
