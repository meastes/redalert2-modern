// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y4): BuildingClass::Do_MISSION_DECONSTRUCTION, unmatched in
// ra2-decomp. TibSun's text (TibSun archive code/building.cpp; same licence notice as above);
// native changes are marked NATIVE. Behaviour code for the native build only. The survivors part is
// compared with gamemd.exe 0x00449C30 (lane Y6: who leaves survivors, absorbed passengers and
// occupants let out), and the tank bunker's release (lane P55: 0x0044AAA4). Lane P57 compared the
// whole function (0x00449C30-0x0044ABC5) step by step and restored YR's steps (marked "YR (P57)"):
// the slave manager's redeploy to ore (0x0044AA3D) and its hand-over to the undeployed unit
// (0x0044A03C), the deploy and pack-up sounds, the radio links (SendToEachLink, not TibSun's
// contact only), TibSun's repair-bay sale and AuxSound2 removed (not in gamemd.exe), the passengers'
// exit point, the undeployed unit's health, voice and selection, and the building's removal.
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y11b): BuildingClass::Drop_Debris, gamemd.exe 0x00442D90 (1,651 bytes),
// unmatched in the YR tree (a link-closure trap whenever a building is destroyed). TibSun's text
// (TibSun archive code/building.cpp, BuildingClass::Drop_Debris; attribution as above) with Yuri's
// Revenge's changes read from gamemd.exe's code: a building that absorbs units or infantry (UnitAbsorb,
// InfantryAbsorb: the bio reactor, the tank bunker) first lets out what it holds at its own position,
// facing the building's way (each one steps the foundation list on), scattering and a computer house's set to Hunt (each
// counted as the house's infantry again); a survivor that cannot be placed is deleted; the survivors
// then follow TibSun's rules (a technician from a building with buildup data, an attack on the
// destroyer unless the house can overpower it, else Move for a human house or Hunt); the scorch or crater mark is placed within half a cell of each clear cell's centre.
// Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y11b): BuildingClass::FUN_00457DE0 (gamemd.exe 0x00457DE0, 1,029 bytes,
// unmatched in the YR tree): a garrisoned building lets its occupants out (it is destroyed, red, sold
// or abandoned). Written from gamemd.exe's code: the first cell along the outside of the foundation
// that the first occupant can enter (the right edge upwards from the bottom right corner, the bottom
// edge leftwards, the top edge rightwards from the top left corner, the left edge downwards); with
// none, the occupants die unless the caller forces them out at the foundation's bottom right cell.
// Each is placed there, its garrison orders dropped for a human house, its target cleared, scattered
// away from the building, and (hunt) taken from its team and set to Hunt; one that cannot be placed is
// deleted. Then the list is emptied and the cell's threat updated.
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y12): BuildingClass::Captured, gamemd.exe 0x00448260 (4,518 bytes,
// unmatched in the YR tree; a campaign map's opening ownership changes reached it as a link-closure
// trap). Written from behaviour: Red Alert 2's exact text (src/recovered/building.cpp
// BuildingClass::Captured, game.exe 0x00444660, which names gamemd.exe 0x00448260 as its twin) and
// TibSun's (building.cpp:4212) in the YR tree's names, with gamemd.exe's YR changes: the attached bomb
// disarmed unless the building can be occupied; the multiplayer-passive cash bonus; the EVA lines
// (EVA_TechBuildingLost, the type's CaptureEvaEvent, EVA_BuildingCaptured after the radar event 10); a
// building that needs an engineer gets its engineer flag and its light source; the house's YR lists
// (construction yards, repair bays, grinders, absorbers, bunkers, occupiables, cloning vats, secret
// labs, psychic detectors, factory plants with the cost multipliers, buildings) and counters (ore
// purifiers, docks, self heal) move from the old owner to the new, Grand_Opening(true) rebuilding the
// new owner's counters as it does for every captured building. NATIVE: the light source and the
// type's animations are not re-enabled here (Grand_Opening redraws them).
// Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y12): BuildingClass::Fire_Direction, gamemd.exe 0x0044D7D0 (161 bytes,
// BuildingClass vtable 0x007E3EBC slot 194; unmatched in the YR tree, a link-closure trap the first Soviet
// mission reached). TibSun's text (TibSun archive code/building.cpp:6151; same licence notice as the drafts
// above), which gamemd.exe follows: no target, the body's facing rounded to 256 directions; a turret not
// drawn as a voxel animation, its facing rounded to 32; otherwise the aim at the target (Aim_Direction).
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y15), appended to the building unit: BuildingClass::Do_MISSION_UNLOAD,
// gamemd.exe 0x0044D880 (2,848 bytes), unmatched in ra2-decomp. Written from gamemd.exe's code read
// instruction by instruction, on the shape of the RA2 twin (game.exe 0x00449290, src/recovered/building.cpp,
// matched there): YR adds the absorbers' unload in front (a Bio Reactor or a unit absorber lets its
// first passenger out to an adjacent cell), sends a unit whose locomotor is not the drive one 4,1
// cells away (RA2: 5,1) and plays the factory's door animation by name. Behaviour code for the native
// build only; not compared with a VC6 compile.
// ---------------------------------------------------------------------------------------------
void __cdecl FUN_004068E0(char const * fmt, ...);	// the debug log (audevent.h)

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE STAND-IN (lane N3, 2026-11-19): BuildingClass::ForceCreate (gamemd.exe 0x00458A80,
// 809 bytes; BuildingClass's vtable slot 0x490), unmatched in ra2-decomp. Written from gamemd.exe's
// instructions (yr/tools/yr_dis.py) in the shape of the tree's matched FootClass::ForceCreate
// (0x004DF510), which uses the same virtual calls on one cell: over every cell of the building's
// foundation an overlay is cleared and the units there are lifted off the map (any building or
// non-techno there refuses the placement and puts them back); the building is placed; the units come
// down on the nearest free cells. Reached by the trigger action "create building at"
// (TActionClass::TAction_CreateBuildingAt) and HouseClass::RespawnStartingForces. Behaviour code for
// the native build only.
// ---------------------------------------------------------------------------------------------
bool BuildingClass::ForceCreate(Coord & coord, unsigned unk)
{
	Cell const * offset = Occupy_List(false);
	Cell origin = coord.As_Cell();
	DynamicVectorClass<TechnoClass *> objs;
	int index;
	while (*offset != REFRESH_EOL) {
		CellClass & cell = Map[origin + *offset++];
		if (cell.Overlay != OVERLAY_NONE) {
			cell.Overlay = OVERLAY_NONE;
			cell.OverlayData = 0;
			cell.Rubble = NULL;
		}
		for (ObjectClass * occupier = cell.Cell_Occupier(); occupier != NULL; occupier = cell.Cell_Occupier()) {
			if (!(occupier->AbstractFlags & 1) || occupier->RTTI == RTTI_BUILDING) {
				for (index = objs.Count() - 1; index >= 0; index--) {
					TechnoClass * object = objs[index];
					Coord position = object->Position;
					if (!object->Unlimbo(position, DIR_N)) {
						object->Delete_Me();
					}
				}
				objs.Clear();
				return(false);
			}
			objs.Add((TechnoClass *)occupier);
			occupier->Limbo();
		}
	}
	if (!Unlimbo(coord, (Dir256)unk)) {
		Delete_Me();
		for (index = objs.Count() - 1; index >= 0; index--) {
			TechnoClass * object = objs[index];
			Coord position = object->Position;
			if (!object->Unlimbo(position, DIR_N)) {
				object->Delete_Me();
			}
		}
		objs.Clear();
		return(false);
	}
	for (index = objs.Count() - 1; index >= 0; index--) {
		TechnoClass * object = objs[index];
		Cell current = object->Get_Cell();
		Cell nearby = Map.Nearby_Location(current, object->TClass->Speed);
		if (!object->Unlimbo(Map[nearby].Center_Coord(), DIR_N)) {
			object->Delete_Me();
		}
	}
	return(true);
}
