// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y12): Show_Who_Was_Responsible, gamemd.exe 0x004C3E30 (2,924 bytes;
// the tree declares it in egos.h and calls it as FUN_004C3E30 from the main menu's Credits, and Do_Win
// calls it after a campaign's last mission: a link-closure trap at both). Written from behaviour: Red
// Alert 2's draft (src/recovered/classes/EgoClass/Show_Who_Was_Responsible.cpp, game.exe 0x004B37C0) in
// the YR tree's names, with gamemd.exe's changes: CREDITSMD.TXT, the tree's EgoClass (Scroll by two
// pixels, Render onto the credits surface DAT_008A0F58), OD_Blend_Color for the fades. Behaviour code for
// the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
#include "stringtable.h"
namespace Game { bool __cdecl CallBack(void); }
class InputManagerClass {
	public:
		void DoSomething(void);
		static InputManagerClass * Instance;
};

static wchar_t Credits_Line[512];      // gamemd.exe 0x008A0F5C: the line with its labels expanded
static int const Credits_Scroll_Ticks = 2;    // gamemd.exe 0x00820770

