// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y4), appended to the foot unit: FootClass::Active_Click_With
// (ActionType, ObjectClass *, bool), YR 0x004D74E0 (2,155 bytes, unmatched in ra2-decomp).
// TibSun's text (TibSun archive code/foot.cpp; SPDX GPL-3.0-or-later, Copyright 2025 Electronic
// Arts Inc. and OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md) with gamemd.exe's
// cases and changes, read from its code (the same case table as the RA2 twin, src/recovered/foot.cpp
// 0x004C61A0) and marked YR: the berserk and rocker-link refusals the cell variant also makes, the
// unload, detonate and attack-move actions, the move order's original cell. Behaviour code for the
// native build only.
// ---------------------------------------------------------------------------------------------
#include "bomb.h"

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y10), YR 0x004E02C0 (13 bytes, unmatched in ra2-decomp): the
// ILocomotion smart pointer's _Release, which FootClass::Load calls on its Locomotion before the
// saved locomotor is loaded: the interface's Release when it holds one (gamemd.exe: mov eax,[ecx];
// test; call [vtable+8]). Behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------
void __fastcall FUN_004E02C0(void * loco)
{
	IUnknown * pointer = *(IUnknown **)loco;
	if (pointer != NULL) {
		pointer->Release();
	}
}
