// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y3), appended to the map unit: MapClass::Set_Map_Dimensions, YR
// 0x00565C10 (5,374 bytes, unmatched in ra2-decomp; RA2 counterpart 0x005484E0, whose draft in
// src/recovered/map.cpp is this same text). The text is TibSun's (TibSun archive code/map.cpp;
// SPDX GPL-3.0-or-later, Copyright 2025 Electronic Arts Inc. and OpenTS contributors, EA's GPL
// section 7 terms; THIRD_PARTY.md); native name changes are marked NATIVE. Behaviour code for the
// native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
void MapClass::Set_Map_Dimensions(Rect const & rect, bool reset_cells, int cell_height, bool refresh_map)
{
	int i = 0;

	int w = PlayRect.Width;
	int h = 2 * Map.PlayRect.Height + 8;
	CellClass *cptr;
	CellClass *cells = NULL;
	CellClass *cells_base = NULL;

	Cell cell(CELL_NONE);

	for (i = 0; i < Objects.Count(); i++) {
		Objects[i]->Mark(MARK_UP);
	}

	for (i = 0; i < Array.Length(); i++) {
		if (!In_Radar(Cell(i % MAP_CELL_W, i / MAP_CELL_H))) {
			CellClass *c = Array[i];
			if (c != NULL) {
				c->Height = cell_height;
			}
		}
	}

	int cellcount = 0;

	if (!reset_cells) {
		cells = (CellClass *) new char[sizeof(CellClass) * w * h];//new CellClass[w * h];
		cells_base = cells;
		Reset_Iterator();
		cptr = Iterate();
		cell = cptr->Fetch_CellID();
		while (cptr != NULL) {
			*cells = *cptr;
			cells->BridgeDeckCell = NULL;
			cells->UnusedCell = NULL;
			cptr = Iterate();
			cellcount++;
			cells++;
		}
	}

	PlayRect = rect;
	PlayRect.X = 0;
	PlayRect.Y = 0;

	if (PlayRect.Width > 63 || PlayRect.Height > 63) {
		Map.Set_Radar_Scale(2);
	}

	MapRegionClass::MapStartDiagonal = Map.PlayRect.Width;
	MapRegionClass::MapEndDiagonal = Map.PlayRect.Width + 2 * Map.PlayRect.Height;

	MapRect.X = 1;
	MapRect.Y = 1;
	int msize = rect.Height + rect.Width - 1;
	MapRect.Width = msize;
	MapRect.Height = msize;

	for (int y = 0; y < 2 * MapRect.Height + 2; y++) {
		for (int x = 0; x < MapRect.Width + 2; x++) {
			Cell cell(x, y);
			if (In_Radar(cell)) {
				int idx = (MAP_CELL_H * y) + x;
				CellClass *c = Array[idx];
				if (c == NULL) {
					CellClass *cc = new CellClass;
					cc->Set_CellID(cell);
					Array[idx] = cc;
				} else {
					c = new(c) CellClass;
					c->Set_CellID(cell);
				}
				Array[idx]->Height = cell_height;
			}
		}
	}

	if (!reset_cells) {
		Reset_Iterator();
		Cell icell = Iterate()->CellID;
		Cell cellpos = Cell(rect.Y + rect.X, rect.Y - rect.X);
		cell = (icell - cell) - cellpos;

		cells = cells_base;
		while (cellcount > 0) {
			cptr = cells;
			Cell scell = cptr->CellID;
			Cell shifted = scell + cell;

			CellClass *c = &(*this)[shifted];

			Cell dcell = c->CellID;
			if (c != NULL) {
				*c = *cptr;
				c->Set_CellID(dcell);
			}

			cellcount--;
			cells++;
		}

		Reset_Iterator();
		cptr = Iterate();
		while (cptr != NULL) {

			Cell c = cptr->Fetch_CellID();

			if (c.X + c.Y < PlayRect.Width - 2 * rect.Y + 1 ||
				c.Y - c.X > PlayRect.Width + 2 * rect.X - 1 ||
				c.X + c.Y > PlayRect.Width + 2 * (h + PlayRect.Height - rect.Y - Map.PlayRect.Height) ||
				c.X - c.Y > PlayRect.Width + 2 * (w - rect.X - Map.PlayRect.Width) - 1) {

				int cellindex = c.X + c.Y * MAP_CELL_W;

				delete Array[cellindex];
				Array[cellindex] = new CellClass;
				Array[cellindex]->Set_CellID(c);
				Array[cellindex]->ITType = ISOTILE_NONE;
				Array[cellindex]->SubTile = 0;
				Array[cellindex]->Ramp = 0;
				Array[cellindex]->Overlay = OVERLAY_NONE;
				Array[cellindex]->Height = cell_height;
			}
			cptr = Iterate();
		}

		for (i = 0; i < WAYPT_COUNT; i++) {
			if (Scen->Is_Valid_Waypoint(i)) {
				Scen->Set_Waypoint(i, Scen->Get_Waypoint_Cell(i) + cell);
			}
		}

		for (i = 0; i < Tubes.Count(); i++) {
			Tubes[i]->Enter += cell;
			Tubes[i]->Exit += cell;
		}

		Coord coord = Coord(cell, 0) - Coord(CELL_LEPTON_W / 2, CELL_LEPTON_H / 2, 0);

		for (i = 0; i < Objects.Count(); i++) {
			ObjectClass *obj = Objects[i];
			if (obj->RTTI != RTTI_ANIM) {
				obj->Set_Coord(coord + obj->Get_Coord());
			}
		}

		for (i = 0; i < Anims.Count(); i++) {
			AnimClass *anim = Anims[i];
			if (anim->OwnerObject == NULL) {   // NATIVE: TibSun xObject is the tree's OwnerObject
				anim->Set_Coord(coord + anim->Get_Coord());
			}
		}

		ScenarioInit++;

		for (i = 0; i < Objects.Count(); i++) {
			if (Objects[i]->RTTI != RTTI_PARTICLESYSTEM || ((ParticleSystemClass *)Objects[i])->Class->Behaves_Like() != PSYS_BEHAVIOR_GAS) {
				Cell cell = Objects[i]->PositionCell;
				const Cell *list = Objects[i]->Occupy_List();
				while (*list != REFRESH_EOL) {
					Cell ncell = cell + *list;
					if (!In_Radar(ncell)) {
						Objects[i]->Limbo();
						delete Objects[i];
						i--;
						break;
					}
					list++;
				}
			}
		}

		ScenarioInit--;

		for (i = 0; i < Array.Length(); i++) {
			CellClass *c1 = Array[i];
			CellClass **c2 = &Array[i];
			if (c1 != NULL) {
				if (!In_Radar(Cell(i % MAP_CELL_W, i / MAP_CELL_H))) {
					*c2 = NULL;
					delete c1;
				}
			}
		}

		Reset_Iterator();
		cptr = Iterate();
		while (cptr != NULL) {
			if (cptr->IsBridgeDeck && cptr->BridgeDeckCell == NULL) {
				FacingType facing = cptr->IsBridgeEastWest ? FACING_N : FACING_E;
				if (cptr->Overlay == OVERLAY_BRIDGE1 || cptr->Overlay == OVERLAY_BRIDGE2) {
					cptr->Set_Under_Bridge(facing);
				} else {
					cptr->Set_Under_Rail_Bridge(facing);
				}
			}
			cptr = Iterate();
		}

		bool freshcalled;
		if (refresh_map) {
			Fresh_Map();
			freshcalled = true;
		} else {
			freshcalled = false;
		}

		for (i = 0; i < Objects.Count(); i++) {
			Objects[i]->Mark(MARK_DOWN);
		}

		Map.Flag_To_Redraw(GS_REDRAW_ALL);

		if (freshcalled) {
			refresh_map = false;
		}
	}

	if (refresh_map) {
		Fresh_Map();
	}

	new(&BlubCell) CellClass;
	delete (void *)cells_base; /// operator delete is called directly here.
}

// -------------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y3): MapClass::AddContentAt (gamemd 0x005683C0) is unmatched in the
// YR tree; written from gamemd.exe's code as the mirror of the tree's RemoveContentAt (0x005687F0):
// the object occupies each footprint cell, and a building that hides things raises the cells its
// height covers (and its RemoveOccupy/AddOccupy cells). An out-of-range cell is the map's BlubCell
// through MapClass::operator[], as the original. Behaviour code for the native build only.
// -------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y12): MapClass::Reveal, gamemd.exe 0x00577D90 (404 bytes, unmatched in the
// YR tree; a Soviet campaign mission reached it as a link-closure trap; the spy satellite's
// HouseClass::Recalc_Spy_Satellite and the reveal crate call it). Written from gamemd.exe's code, as the
// tree's Reveal_The_Map and Reshroud (its neighbours) are written: the house's map is clear; for the
// player, unless already visionary, every cell (but the three edge cells a multiplayer mode keeps hidden)
// is mapped, visible and out of the fog with no shroud counter and no gap covering it, then the shroud's
// edges are rebuilt and the whole screen redrawn.
// ---------------------------------------------------------------------------------------------


// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE STAND-IN (lane N8): MapClass::Pick_Random_Tile_Variant, gamemd.exe 0x0057B440
// (848 bytes, the size of Red Alert 2's 0x0055D640), unmatched in the YR tree; the random map
// generator's shores and clear ground (Place_Shore, the cliff fill) place the pending tile with it.
// Main's exact text of the RA2 twin (src/recovered/map.cpp) under the YR tree's names; gamemd.exe's
// shore group and facing tables are 0x0082A7F4 and 0x0082A89C (42 entries each, read from the
// player's gamemd.exe like the tree's other DAT_ globals).
// ---------------------------------------------------------------------------------------------
extern int DAT_0082A7F4[42];	// the shore pieces' groups
extern int DAT_0082A89C[42];	// the shore pieces' facings
