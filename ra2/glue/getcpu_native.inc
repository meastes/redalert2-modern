// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P2, branch ports-on-rebuild): appended to the generated getcpu unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// The CPU probes are the library's handwritten assembly (0x0077F14D Detect_MMX_Availability,
// 0x0077F20F Detect_CMOV_Availability); arm64 has neither CPUID nor MMX/CMOV, so the native bodies
// report a CPUID-capable family-6 processor without MMX and CMOV, model 0 (every MMX/CMOV branch
// takes the plain path): main's native/link/behaviour_launch.cpp, carried over.
// Get_CPU_Type() 0x004D2A60 is exact in main (src/recovered/getcpu.cpp) and not yet carried by the
// rebuild; with model 0 main's Get_CPU_Type returns -1 (unknown) before reading the vendor, so the
// behaviour body returns that (item 4 overlay). Get_CPU_Type_Name() 0x004D2B80 is the rebuild's own
// since the re-pin to b873d2b3c6 (lane P5), and its "Unknown" path follows from the -1.
// ---------------------------------------------------------------------------------------------
extern "C" bool __cdecl Detect_MMX_Availability(void)
{
	CPUType = 6;
	return false;
}

extern "C" bool __cdecl Detect_CMOV_Availability(void)
{
	return false;
}

int Get_CPU_Type(void)
{
	return -1;   // family 6, model 0: main's 0x004D2A60 returns -1 when the model byte is zero
}
