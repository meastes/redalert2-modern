// SPDX-License-Identifier: GPL-3.0-or-later
// NATIVE (lane P1, native/rb/fixups.json): RDTSC and Get_RDTSC_CPU_Speed for the rebuild's mpu.cpp.
// The x86 time-stamp counter is natively the port's game clock in nanoseconds (a virtual 1 GHz
// counter, the clock the port's QueryPerformanceCounter reads), so the measured rate is 1000 MHz;
// the RA2 native tree (native/link/behaviour_asm_lib.cpp) and the YR port (yr/glue/yr_asm.cpp) do
// the same. Appended to the unit so it stores the unit's own TSC_Low/TSC_High.
extern "C" uint64_t ra2port_game_ns(void);
void RDTSC(void)
{
	uint64_t t = ra2port_game_ns();
	TSC_Low = (unsigned int)t;
	TSC_High = (unsigned int)(t >> 32);
}
int Get_RDTSC_CPU_Speed(void) { return 1000; }
