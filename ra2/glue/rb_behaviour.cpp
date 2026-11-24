// SPDX-License-Identifier: GPL-3.0-or-later
// Behaviour-written bodies for functions the trial game reaches that the rebuild carries neither
// exact nor as a compiling parked draft (lane P2, branch ports-on-rebuild). Each is main's native
// body (native/link/, native/drafts/) carried over under the rebuild's name and declaration; the
// source row is named with it. NOT EXACT and never part of an exact build. Item 3 of
// docs/REBUILD_HANDOVER_PLAN.md section 2 replaces this file with the renamed link/ and drafts/.
#include <stdint.h>

extern "C" unsigned int _controlfp(unsigned int value, unsigned int mask);   // native/port/src/crt.cpp

// 0x0077F0F4 set_control_word (misc.h; Westwood math, TibSun code/cmathstuff.asm): stores the x87
// control word (fstcw) into FPU_FLAG, which only the x87 ftol replacement reads. arm64 has no x87;
// the port keeps the control word as a value (_controlfp). Main: native/link/behaviour_winmain.cpp.
static unsigned int fpu_flag_native;
extern "C" void set_control_word(void)
{
    fpu_flag_native = _controlfp(0, 0) & 0xFFFF;
}

// 0x0077F274 Get_CPU_Clock (mpu.h, extern "C"; detproc.asm: rdtsc, the high half through the
// reference, the low half returned). Natively the time-stamp counter is a virtual 1 GHz counter, the
// port's game clock in nanoseconds (it stands still while the host pause holds the game). Main:
// native/link/behaviour_asm_lib.cpp, carried over.
extern "C" uint64_t ra2port_game_ns(void);   // native/port/src/win32_time.cpp
extern "C" unsigned int Get_CPU_Clock(unsigned int &high)
{
    uint64_t t = ra2port_game_ns();
    high = (unsigned int)(t >> 32);
    return (unsigned int)t;
}

// 0x0073C370 Is_Current_Channel_Tournament and 0x0073C3A0 Is_Current_Channel_Flag_8 (wonline.cpp, netshare.h's
// declarations): tests of the current Westwood Online channel, false unless the session is an Internet game
// (Session.Type 4; main's src/recovered/wonline.cpp Unknown_0073c370/0073c3a0 through 0x0073C340/0x0073C3C0).
// The LAN host dialog and the game-mode tests call them in every network game. The trial carries no Westwood
// Online (wonline left out), so no Internet game exists and both answer false. Lane P9, found by main's
// lan_pair.py (the LAN host aborted at the link-closure trap).
bool Is_Current_Channel_Tournament(void)
{
    return false;
}
bool Is_Current_Channel_Flag_8(void)
{
    return false;
}

// 0x004068F0 DebugString (debug.h): empty in the original. Natively, as main's Debug_Printf (link/behaviour_startup.cpp),
// it prints the game's debug lines to stderr under RA2_NATIVE_DEBUG_LOG=1 (test runs only), through the port's VC6
// formatter so %ls takes the game's 16-bit wide strings. Lane P9 (main's lan_pair.py reads "Starting scnenario").
extern "C" char *getenv(char const *);
extern "C" int dprintf(int, char const *, ...);
extern "C" int ra2__vsnprintf(char *dst, unsigned long cap, char const *fmt, __builtin_va_list ap);   // native/port/src/crt.cpp
void __cdecl DebugString(char const *format, ...)
{
    static int enabled = -1;
    if (enabled < 0) {
        char const *value = getenv("RA2_NATIVE_DEBUG_LOG");
        enabled = value != 0 && value[0] == '1';
    }
    if (enabled && format != 0) {
        char line[2048];
        __builtin_va_list args;
        __builtin_va_start(args, format);
        int n = ra2__vsnprintf(line, sizeof(line) - 1, format, args);
        __builtin_va_end(args);
        line[n < 0 ? sizeof(line) - 1 : n] = 0;
        dprintf(2, "%s", line);
    }
}

// RA2_NATIVE_CRC_LOG=1: each multiplayer frame's game CRC as "crc <frame> <value>" through the debug log (main's
// link/native_event_data.cpp Native_Log_Frame_CRC). Lane P9.
void RB_Log_Frame_CRC(int frame, unsigned int crc)
{
    static int enabled = -1;
    if (enabled < 0) enabled = getenv("RA2_NATIVE_CRC_LOG") != 0;
    if (enabled) DebugString("crc %d %08x\n", frame, crc);
}
