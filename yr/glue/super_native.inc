// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y6): SuperClass::Launch (gamemd.exe 0x006CC390, 6,884 B), unmatched in
// ra2-decomp. The RA2 twin's SuperClass::Place (src/recovered/super.cpp, RA2 0x0069D850, exact
// there: the nuke, iron curtain, lightning storm, chronosphere, chrono warp and paradrops) in the YR
// tree's names, with YR's changes and its five new super weapons read from gamemd.exe's code: the
// chrono warp also lets parasites go, empties bunkers and spares units inside a war factory; the
// iron curtain spares immobilized feet; paradrops by side (Allied, Soviet, Yuri); the psychic
// dominator, spy plane, genetic mutator, force shield and psychic reveal call the tree's matched
// helpers. Not compared byte for byte; behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------
#include "airctype.h"
#include "revent.h"
#include "parasite.h"
#include "foot.h"
#include "ilocos.h"
#include "ipiggy.h"
#include "loco.h"
#include "building.h"
#include "builtype.h"
#include "weapon.h"
#include "bullet.h"
#include "voc.h"
#include "combat.h"

namespace Unsorted { extern int MuteSWLaunches; }
void __fastcall FUN_00539EB0(int duration, int deferment, Cell cell, HouseClass * owner);	// LightningStorm::Start
void __fastcall FUN_0053AE50(HouseClass * owner, Cell cell);	// PsyDom::Start
int __fastcall FUN_00753250(char const * name);	// the speech index of a name
int __fastcall Create_Paradrop_Planes(HouseClass * house, int air, int number, MissionType mission, AbstractClass * tarcom, AbstractClass * navcom, int passenger, int passengers);
int __fastcall Create_Spy_Planes(HouseClass * house, int air, int number, MissionType mission, AbstractClass * tarcom, AbstractClass * navcom);
