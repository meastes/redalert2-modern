// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P2, branch ports-on-rebuild): appended to the generated getcpu unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// The CPU probes are the library's handwritten assembly (0x0077F14D Detect_MMX_Availability,
// 0x0077F20F Detect_CMOV_Availability); arm64 has neither CPUID nor MMX/CMOV, so the native bodies
// report a CPUID-capable family-6 processor without MMX and CMOV, model 0 (every MMX/CMOV branch
// takes the plain path): main's native/link/behaviour_launch.cpp, carried over.
// Get_CPU_Type() 0x004D2A60 and Get_CPU_Type_Name() 0x004D2B80 are the rebuild's own (Get_CPU_Type since
// the re-pin to 66bccbbfdd, lane P28; its behaviour body here is gone): with CPUModel 0 it returns
// CPU_UNKNOWN before reading the vendor, and the name's "Unknown" path follows.
// ---------------------------------------------------------------------------------------------
extern char CPUModel;   // RA2 0x008015A4 (the rebuild's Get_CPU_Type reads it)

extern "C" bool __cdecl Detect_MMX_Availability(void)
{
	CPUType = 6;
	CPUModel = 0;   // model 0, whatever the game.exe's initial byte: Get_CPU_Type answers CPU_UNKNOWN, as the old body did
	return false;
}

extern "C" bool __cdecl Detect_CMOV_Availability(void)
{
	return false;
}
