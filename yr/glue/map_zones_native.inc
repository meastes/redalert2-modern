// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFTS (lane Y3): functions unmatched in the YR tree, the text TibSun's
// (TibSun archive code/map.cpp; SPDX GPL-3.0-or-later, Copyright 2025 Electronic Arts Inc. and
// OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md); native changes are marked
// NATIVE. Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------

// TibSun map.cpp's file-level helpers the functions below use.
int LastAdjacentZone;

FacingType BridgeSideFacings[BRIDGE_COUNT] = {
	FACING_N,
	FACING_N,
	FACING_NONE,
	FACING_E,
	FACING_E,
	FACING_NONE,
	FACING_N,
	FACING_N,
	FACING_N,
	FACING_N,
	FACING_N,
	FACING_E,
	FACING_E,
	FACING_E,
	FACING_E,
	FACING_E
};

bool MapClass::Is_Same_Cell_Zone(Cell const & from, Cell const & to, MZoneType mzone, bool from_bridge, bool to_bridge, bool leavemap)
{
	if (mzone == MZONE_NONE) {
		return(true);
	}

	bool flocal = Map.In_Local_Radar(from);
	bool finradar = Map.In_Radar(from);

	if (!flocal && finradar) {
		return(true);
	}

	bool tlocal = Map.In_Local_Radar(to);
	bool tinradar = Map.In_Radar(to);

	if (leavemap && flocal && !tlocal && tinradar) {
		return(true);
	}

	return(Get_Cell_Zone(&from, mzone, from_bridge) == Get_Cell_Zone(&to, mzone, to_bridge));   // NATIVE: the tree's Get_Cell_Zone takes the cell's address
}

void MapClass::Update_Cell_Zone(Cell const & cell)
{
	CellZoneStruct * cell_data = &CellZones[Get_Cell_Zone_Index(cell)];

	/*
	 * Calculate the offsets for accessing neighbors per facing in the flattened map array.
	 */
	int stride = Map.PlayRect.Width + Map.PlayRect.Height + 1;
	int offsets[FACING_COUNT] = {
		-(stride + 0),
		-(stride - 1),
		1,
		stride + 1,
		stride + 0,
		stride - 1,
		-1,
		-(stride + 1),
	};

	/*
	 * Don't assign a zone to cells outside the map.
	 */
	if (cell_data->Passability != PASSABLE_OUTSIDE) {

		/*
		 * Try to find a neighboring cell that's land passable (as a baseline).
		 */
		CellZoneStruct * land_neighbor = NULL;
		bool has_land_neighbor = false;
		for (FacingType i = FACING_FIRST; i < FACING_COUNT; i++) {
			land_neighbor = &cell_data[offsets[i]];
			if (land_neighbor->Passability == PASSABLE_LAND) {
				has_land_neighbor = true;
				break;
			}
		}

		/*
		 * If the cell neighbors another passable cell, we need to check the
		 * topology is not too complicated.
		 */
		if (has_land_neighbor && land_neighbor != NULL) {
			int transitions = 0;
			int prev_zone = 0;

			for (FacingType j = FACING_FIRST; j < FACING_COUNT; j++) {
				CellZoneStruct * neighbor = &cell_data[offsets[j]];
				if (Zones[MZONE_NORMAL][neighbor->ZoneID] != Zones[MZONE_NORMAL][prev_zone] && neighbor->Passability != PASSABLE_OUTSIDE) {
					prev_zone = neighbor->ZoneID;
					transitions++;
				}
			}

			/*
			 * If there are only a few transitions, join this cell to its passable neighbor's zone.
			 */
			if (transitions <= 3 && cell_data->Passability == PASSABLE_LAND) {
				cell_data->ZoneID = land_neighbor->ZoneID;
				return;
			}
		}

		/*
		 * If we've failed, recalculate the map's zones completely.
		 */
		Map.Zone_Reset();
	}
}

void MapClass::Place_Down(Cell const & cell, ObjectClass * object)
{
	if (!object) return;
	if (cell.X < 0 || cell.Y < 0 || cell.X >= MAP_CELL_W || cell.Y >= MAP_CELL_H) {	// NATIVE (lane Y12): a cell off the cell array (the original's Is_Valid reads past it) is logged and left alone (lane Y12's LTNK in Allied 2 and Soviet 7 was YR's turn tracks 67-71 missing from the drive locomotor's table: fixed by lane Y16, hdr/drive.h's fixup)
		Coord where = object->Center_Coord();
		static int logged = 0;
		if (logged++ < 5) fprintf(stderr, "place-down: %s at cell %d,%d (coord %d,%d,%d) is off the map\n", object->Class_Of()->Name(), cell.X, cell.Y, where.X, where.Y, where.Z);
		return;
	}

	if (object->Class_Of()->IsFootprint && object->In_Which_Layer() == LAYER_GROUND)  {
		Cell xlist[32];
		List_Copy(object->Occupy_List(), ARRAY_SIZE(xlist), xlist);
		Cell const * list = xlist;
		while (*list != REFRESH_EOL) {
			Cell newcell = cell + *list++;
			if (Is_Valid(newcell)) {
				(*this)[newcell].Occupy_Down(object, object->IsOnBridge);
				(*this)[newcell].Recalc_Attributes();
			}
		}
	}
}

void MapClass::Pick_Up(Cell const & cell, ObjectClass * object)
{
	if (!object) return;
	if (cell.X < 0 || cell.Y < 0 || cell.X >= MAP_CELL_W || cell.Y >= MAP_CELL_H) {	// NATIVE (lane Y12): as Place_Down
		Coord where = object->Center_Coord();
		static int logged = 0;
		if (logged++ < 5) fprintf(stderr, "pick-up: %s at cell %d,%d (coord %d,%d,%d) is off the map\n", object->Class_Of()->Name(), cell.X, cell.Y, where.X, where.Y, where.Z);
		return;
	}

	if (object->Class_Of()->IsFootprint && object->In_Which_Layer() == LAYER_GROUND)  {
		Cell xlist[32];
		List_Copy(object->Occupy_List(), ARRAY_SIZE(xlist), xlist);
		Cell const * list = xlist;
		while (*list != REFRESH_EOL) {
			Cell newcell = cell + *list++;
			if (Is_Valid(newcell)) {
				(*this)[newcell].Occupy_Up(object, object->IsOnBridge);
				(*this)[newcell].Recalc_Attributes();
			}
		}
	}
}

int MapClass::Cell_Threat(Cell const & cell, HouseClass const & house)
{
	int threat = house.Regions[Map.Cell_Region(Map[cell].Fetch_CellID())].Threat_Value();

#if NEVER
	if (!threat && Map[cell].IsVisible) {
		threat = 1;
	}
#endif
	return(threat);
}

#define LB2_CELL(n) ((OverlayType)(204 + (n)))	// map.cpp's LB2: the second low-bridge family's overlays

// NATIVE: YR's MapClass::Is_Fogged (0x005865E0, 5 bytes: `xor al, al; ret 4`) always answers false;
// TibSun's fog test is not used.
bool MapClass::Is_Fogged(Coord const &)
{
	return(false);
}
