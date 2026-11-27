// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFTS (lane Y3): functions unmatched in the YR tree, the text TibSun's
// (TibSun archive code/house.cpp; SPDX GPL-3.0-or-later, Copyright 2025 Electronic Arts Inc. and
// OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md); native changes are marked
// NATIVE. Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y4): HouseClass::Can_Build, unmatched in ra2-decomp. TibSun's text
// (TibSun archive code/house.cpp; SPDX GPL-3.0-or-later, Copyright 2025 Electronic Arts Inc. and
// OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md) with the RA2 twin's YR-era tests
// (src/recovered/house.cpp, RA2 0x004F7870's shape: override prerequisites, stolen technology,
// required and forbidden houses) in place of TibSun's ownable/construction-yard test, marked YR;
// TibSun's GDI/Nod factory groups become YR's refinery group. Lane Y6 compared the ownership tests
// with gamemd.exe 0x004F7870 and added YR's secret-lab grant and the super-weapons-off test; the
// rest is not compared. Behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y4): HouseClass::Where_To_Place_Building, unmatched in ra2-decomp.
// TibSun's text (TibSun archive code/house.cpp; same licence notice as above); native changes are
// marked NATIVE. Behaviour code for the native build only; gamemd.exe's version is not compared.
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y4): HouseClass::Can_Build_Here, unmatched in ra2-decomp.
// TibSun's text (TibSun archive code/house.cpp; same licence notice as above); native changes are
// marked NATIVE. Behaviour code for the native build only; gamemd.exe's version is not compared.
// ---------------------------------------------------------------------------------------------
bool HouseClass::Can_Build_Here(BuildingTypeClass *building, Cell const & cell)
{
	if (Session.Type == GAME_NORMAL) {
		return(true);
	}

	int spacing = Rule->AIBaseSpacing;
	int width = (2 * spacing) + building->Width();
	int height = (2 * spacing) + building->Height();

	int mask = 1 << HeapID;

	for (int x = cell.X - spacing - 1; x < cell.X + width + 1; x++) {
		for (int y = cell.Y - spacing - 1; y < cell.Y + height + 1; y++) {
			if (Map[Cell(x, y)].OccupiedBy & mask) {
				return(true);
			}
		}
	}

	return(false);
}

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y6): HouseClass::AI_Build_Defense (gamemd.exe 0x00506EF0, 3,216 B),
// unmatched in ra2-decomp. The text is the RA2 twin's (src/recovered/house.cpp, RA2 0x004F1190,
// exact there) in the YR tree's names, with YR's changes read from gamemd.exe's code: the facing
// inlined as DirType(atan2), the candidate weights are the defense values alone (RA2 adds
// 10000 / cost), the cells ranked through BasePlanningCellContainer (IndexClass here). Not compared
// byte for byte; behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y7): HouseClass::Random_Cell_In_Zone, YR 0x00501AC0 (1,596 bytes,
// ra2-decomp's PickRandomCellInZone, unmatched; a link-closure trap reached through Where_To_Go
// when a computer player's unit looked for somewhere to go). TibSun's text (code/house.cpp) with
// gamemd.exe's changes read from its code: the base's centre is the coordinate of the base centre
// cell (FUN_0050DF30: COORD_NONE without one) instead of a Center member, the random numbers are
// the scenario's (Scen->RandomNumber), and the legal-area test is In_Local_Radar(cell, true)
// (MapClass::IsWithinUsableArea 0x00578460). The quadrant rotation is written out on the raw
// facing as gamemd.exe does it (the upper three bits from the previous eighth, the rest kept).
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y14): HouseClass::Fire_PsyDom, gamemd.exe 0x0050A150 (497 bytes, unmatched in
// ra2-decomp; a link-closure trap on the play paths: an AI house firing its Psychic Dominator). Written from
// gamemd.exe's code in the shape of its matched siblings Fire_GenMutator (0x00509F60) and FUN_0050A350: with
// no dominator already active (FUN_0053B400, PsyDom::Active) and an enemy chosen, and no preferred target
// cell, every foot unit out of limbo scores the cells of its spread (CellSpreadTable up to entry
// DAT_007ED3D0[3]) by the foot units there that are not allied, not in the air and can be permanently mind
// controlled; the best cell, if it is in the usable map, is the target of Place_Special_Blast.
bool FUN_0053B400(void);
