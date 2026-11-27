// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFTS (lane Y3): functions unmatched in the YR tree, the text TibSun's
// (TibSun archive code/display.cpp; SPDX GPL-3.0-or-later, Copyright 2025 Electronic Arts Inc. and
// OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md); native changes are marked
// NATIVE. Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------

void DisplayClass::Set_Cursor_Shape(Cell const * list)
{
	if (CursorSize) {
		Cursor_Mark(ZoneCell+ZoneOffset, false);
	}
	ZoneOffset = CELL_NONE;

	if (list) {
		//int	w,h;
		static Cell _list[120];

		memcpy(_list, list, sizeof(_list));
		CursorSize = _list;
		Cell dim = Get_Occupy_Dimensions (CursorSize);
		dim -= Cell(1,1);
		ZoneOffset = -dim / 2;
		Cursor_Mark(ZoneCell+ZoneOffset, true);
	} else {
		CursorSize = 0;
	}
}

void DisplayClass::All_To_Look(bool units_only, bool is_fog)
{
	for (int index = 0; index < Layer[LAYER_GROUND].Count(); index++) {
		TechnoClass * tech = Dynamic_Cast<TechnoClass *>(Layer[LAYER_GROUND][index]);
		if (tech != NULL) {
			if (tech->RTTI == RTTI_BUILDING && units_only) continue;

			if (tech->House->Is_Player_Control()) {
				if (tech->IsDiscoveredByPlayer) {
					tech->Look(false, is_fog);
				}
			} else {
				if (tech->RTTI == RTTI_BUILDING && Rule->IsAllyReveal && tech->House->Is_Ally(PlayerPtr)) {
					tech->Look(is_fog, false);
				}
			}
		}
	}
}
