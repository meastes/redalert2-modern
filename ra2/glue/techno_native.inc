// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P2, branch ports-on-rebuild): appended to the generated techno unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// TechnoClass::Is_Weapon_Valid (techno.h, 0x006D8640) and this unit's Is_Weapon_Present are one
// function: the callers pass the weapon with this in ecx, which the __stdcall body never reads. The
// member forwards to it (rb_aliases.py binds only same-shape names; this one adds this).
// ---------------------------------------------------------------------------------------------
bool TechnoClass::Is_Weapon_Valid(WeaponDataStruct const * weapon) const
{
	return Is_Weapon_Present(weapon) != 0;
}

// 0x006C3B20 (TechnoClass vtable slot 247, +0x3DC): the old health bar slot, an empty body in RA2 (ret 8;
// main: TechnoClass::unknown_slot247 in src/recovered/techno.cpp, exact). The rebuild defines it at that
// address as Draw_Health_Bar_Old and declares the virtual apart as Unknown_Virtual_3DC (lane P4); the
// Draw_Post_Render draft calls the virtual when a selected unit's health bar is drawn.
void TechnoClass::Unknown_Virtual_3DC(unsigned, unsigned)
{
}
