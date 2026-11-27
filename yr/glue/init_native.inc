// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y2), appended to the init unit: Allocate_Surfaces, YR 0x00533FD0
// (1,137 bytes, unmatched in ra2-decomp). From the RA2 tree's Allocate_Surfaces
// (src/recovered/init.cpp, 0x00519500; TibSun's) with gamemd.exe's changes read from its code:
// YR's DSurface takes a second flag (its 3D-device surface: composite and tile only), and the
// composite and tile surfaces go to system memory unless [Video] VideoBackBuffer (0x00840A6C).
// Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
extern bool DAT_00840A6C;                         // 0x00840A6C [Video] VideoBackBuffer

// DSurface's video-memory flag (+0x19, protected; the original reads the field).
struct YRSurfaceMemory : DSurface {
	static bool Video_Ram(Surface *surface) { return ((YRSurfaceMemory *)surface)->IsVideoRam; }
};

// NON-EXACT NATIVE DRAFT: FUN_00534E50 (330 bytes, unnamed, unmatched), from gamemd.exe's code:
// the neutral mixfiles reloaded: NEUTRAL.MIX (0x00884E5C) and NTRLMD.MIX (0x00884E58) freed, then
// NTRLMD.MIX and NEUTRAL.MIX made again with the fast key. The original returns whether both
// loaded (the tree declares it void; its callers ignore the result).
extern MixFileClass *DAT_00884E58;
extern MixFileClass *DAT_00884E5C;
