// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFTS (lane Y3): functions unmatched in the YR tree, the text TibSun's
// (TibSun archive code/unit.cpp; SPDX GPL-3.0-or-later, Copyright 2025 Electronic Arts Inc. and
// OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md); native changes are marked
// NATIVE. Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y6): UnitClass::Do_MISSION_HARVEST (gamemd.exe 0x0073E5E0, 2,528 B),
// unmatched in ra2-decomp. TibSun's Mission_Harvest text as the RA2 twin has it (src/recovered/
// unit.cpp, RA2 0x007025A0, exact there: the chrono miner's teleport, the too-far distances, the
// dock search over the type's list), with YR's changes read from gamemd.exe's code: a resource
// gatherer that keeps slaves hands the mission to its slave manager; an idle useless harvester
// goes for repair when a repair bay is reachable (RA2: when the house owns one). Not compared byte
// for byte; behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y15): UnitClass::Is_Route_Broken, unmatched in ra2-decomp (called by
// the route planning above). TibSun's text (TibSun archive code/unit.cpp; SPDX GPL-3.0-or-later,
// Copyright 2025 Electronic Arts Inc. and OpenTS contributors, EA's GPL section 7 terms;
// THIRD_PARTY.md); gamemd.exe's version is not compared. Behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------
bool UnitClass::Is_Route_Broken(Cell const & from, Cell const & to) const
{
	if (HeightAGL >= 0 && Map.Is_Same_Cell_Zone(from, to, MZONE_NORMAL, 0, 0, 0)) {
		if (from == to) {
			return(false);
		}
		int dist = MAX(abs(to.X - from.X), abs(to.Y - from.Y));
		if (dist == 1) {
			return(false);
		}
		if (dist >= 12 || !Map.In_Local_Radar(from) || !Map.In_Local_Radar(to)) {
			return(true);
		}
		return(Search.Test_Cell_Walk(from, to, this, false, Map[to].IsUnderBridge, MZONE_NORMAL) > 15);
	}
	return(true);
}
