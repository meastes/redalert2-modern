// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE STAND-INS (lane N8): Yuri's Revenge's random map generator. The skirmish
// Choose Map dialog's Create Random Map (and its Preview) runs MapGeneratorClass::Generate_Random_Map,
// which the YR tree (pinned ra2-decomp, and its latest) declares and never defines, with the
// generator functions only it reaches. Written from gamemd.exe's instructions (yr_dis.py) in the
// shape of Red Alert 2's twins: each YR body was compared with RA2's (game.exe) and has the same
// size and the same calls except where noted. The text is the rebuild tree's mapgen.cpp
// (TibSun archive / OpenTS / ra2-decomp text, GPL-3.0-or-later, EA's GPL section 7
// terms; THIRD_PARTY.md) and main's src/recovered/mapgen.cpp, under the YR tree's names.
//   gamemd.exe   bytes  RA2 twin (game.exe)            function
//   0x0058C800   1678   0x0056E100 (same size)         MapRegionClass::Create_Region_From_Cell
//   0x0058D620   4012   0x0056EF20 (same size)         MapRegionClass::Split_Region
//   0x00591D80   1724   0x00573680 (same size)         MapRegionClass::Build_North_Ramp
//   0x00594B50    998   0x00576420 (925)               Generate_Starting_Points (YR: the zone regions inline, ends in FUN_0058B820)
//   0x005981F0   1889   0x00579960 (1897)              MapGeneratorClass::Load_Settings (YR reads RMGMD.INI)
//   0x00598960   3018   0x0057A0D0 (3086)              MapGeneratorClass::Generate_Random_Map
//   0x0059E740   6472   0x0057FF20 (same size)         MapGeneratorClass::Place_Waterfall
//   0x005A08D0   2230   0x005820B0 (same size)         MapGeneratorClass::Grow_Water_Region
//   0x005A23A0   1300   0x00583B80 (same size)         MapGeneratorClass::Create_Tiberium
//   0x005A89D0   1321   0x0058A1B0 (same size)         MapGeneratorClass::Generate_Rural_Roads
// The "RA2:" comments are the rebuild's notes on Red Alert 2's form, which these YR bodies share.
// Behaviour code for the native build only; it earns no exact-match credit.
// ---------------------------------------------------------------------------------------------
#include "findmake.h"


// gamemd.exe 0x0058D620 (4,012 bytes, the size of RA2's 0x0056EF20). Main's draft of the RA2 twin
// (src/recovered/mapgen.cpp Split_Region_0056ef20, behaviour-checked) under the YR tree's names, with
// gamemd.exe's calls: the region's own Get_Angle_Score (0x0058C6F0), RMGGaussian scaled by 22.5
// degrees, Build_Border_Cell_List2 (0x0058E5D0), From_ID inline, the inline height change, and on the
// inland and mountainous maps (MapType 3, 4: 0x0058E2F9) no new height 0.
bool MapRegionClass::Split_Region(void)
{
	int i, j;

	if (Unsplittable) {
		return(false);
	}

	int heap_size = (2 * CellCount) + 10;
	CellNode * nodes = new CellNode[heap_size];
	PriorityQueueClass<CellNode> * queue = new PriorityQueueClass<CellNode>((2 * CellCount) + 10);
	queue->Clear();

	CellData::Reset_Spreads(false);

	int spread_count = 0;
	for (i = Cells.Count() - 1; i >= 0; i--) {
		MapRegionClass::Get_Cell_Data(Cells[i]).RegionID = -2;
	}

	unsigned int target_count = Pick_Random_UInt(CellCount / 8, CellCount / 3);
	unsigned int seed_index = Pick_Random_UInt(0, Cells.Count() - 1);

	int node_count = 1;
	Cell seed = Cells[seed_index];
	nodes[0].Element = seed;
	nodes[0].Score = 0.0f;
	MapRegionClass::Get_Cell_Data(seed).SpreadID = -3;
	queue->Insert(nodes[0]);

	CellNode * node = queue->Extract_Min();
	double dist = (unsigned int)RMGRandom() * (6.283185307179586 / 4294967295.0);

	while (node != NULL) {
		if (spread_count >= (int)target_count) {
			break;
		}

		CellData::Set_Region(node->Element, -3);

		int cell_index = node->Element.X + MapCellStride * node->Element.Y;
		CellNode * newnode = &nodes[node_count];

		for (int dir = FACING_N; dir < FACING_COUNT; dir += 2) {
			Cell ncell = Adjacent_Cell(node->Element, (FacingType)dir);
			if (My_In_Radar(ncell)) {
				if (CellData::Get_Region(ncell) == -2 && CellData::Get_Spread(ncell) != -3 && Map[ncell].Is_Tile_Clear()) {
					newnode->Element = ncell;
					newnode->Score = (float)Get_Angle_Score(seed, ncell, dist);
					MapRegionClass::Get_Cell_Data(cell_index + AStarFacingToOffset[dir]).SpreadID = -3;
					node_count++;
					queue->Insert(*newnode++);
				}
			}
		}

		spread_count++;
		dist += RMGGaussian() * 0.39269908169872414;
		node = queue->Extract_Min();
	}

	CellData::Reset_Spreads(false);

	CellNode * remaining = queue->Extract_Min();
	DynamicVectorClass<Cell> ncells;

	while (remaining != NULL) {
		Cell c = remaining->Element;
		MapRegionClass::Get_Cell_Data(c).RegionID = -3;
		remaining = queue->Extract_Min();
	}

	delete[] nodes;
	delete queue;

	DynamicVectorClass<MapRegionClass *> new_regions;
	for (i = Cells.Count() - 1; i >= 0; i--) {
		Cell cell = Cells[i];
		int cell_index = cell.X + MapCellStride * cell.Y;
		if (MapRegionClass::Get_Cell_Data(cell_index).RegionID < -1) {
			int old_id = MapRegionClass::Get_Cell_Data(cell_index).RegionID;

			MapRegionClass * region = new MapRegionClass(cell);
			region->Cells.Add(cell);
			region->CellCount++;
			new_regions.Add(region);
			MapRegionClass::Get_Cell_Data(cell_index).RegionID = region->ID;

			DynamicVectorClass<Cell> open;
			open.Set_Growth_Step(100000);
			open.Add(cell);

			while (open.Count() != 0) {
				int index = open.Count() - 1;
				Cell c = open[index];
				open.Delete_Index(index);

				for (int m = 0; m < FACING_COUNT; m++) {
					Cell ncell = Adjacent_Cell(c, (FacingType)m);
					if (My_In_Radar(ncell)) {
						CellData & ndata = MapRegionClass::Get_Cell_Data(ncell);
						if (ndata.RegionID == old_id) {
							region->Cells.Add(ncell);
							region->CellCount++;
							ndata.RegionID = region->ID;
							open.Add(ncell);
						}
					}
				}
			}
		}
	}

	for (i = new_regions.Count() - 1; i >= 0; i--) {
		MapRegionClass * region = new_regions[i];
		if (region->Discardable) {
			continue;
		}

		DynamicVectorClass<Cell> * cells = region->Build_Border_Cell_List2();
		bool * adjacent = new bool[MapRegionClass::TotalCount];
		for (j = 0; j < MapRegionClass::TotalCount; j++) {
			adjacent[j] = false;
		}

		for (j = cells->Count() - 1; j >= 0; j--) {
			Cell c = (*cells)[j];
			for (int m = 0; m < FACING_COUNT; m++) {
				Cell ncell = Adjacent_Cell(c, (FacingType)m);
				if (My_In_Radar(ncell)) {
					int id = MapRegionClass::Get_Cell_Data(ncell).RegionID;
					if (id >= 0 && id != region->ID) {
						adjacent[id] = true;
					}
				}
			}
		}

		delete cells;

		MapRegionClass * merge_target = NULL;
		int min_height = -1;
		int max_height = -1;
		int merge_height = -1;
		int max_count = -1;

		for (j = 0; j < MapRegionClass::TotalCount; j++) {
			if (adjacent[j]) {
				MapRegionClass * other = MapRegionClass::From_ID(j);
				if (other == NULL) continue;	// NATIVE: gamemd.exe reads through the missing region

				if (!other->Discardable) {
					if (other->CellCount > max_count && !other->ContainsWater) {
						max_count = other->CellCount;
						merge_height = other->CellHeight;
						merge_target = other;
					}

					if (min_height == -1) {
						min_height = other->CellHeight;
						max_height = other->CellHeight;
					} else {
						if (other->CellHeight > max_height) {
							max_height = other->CellHeight;
						}
						if (other->CellHeight < min_height) {
							min_height = other->CellHeight;
						}
					}
				}
			}
		}

		DynamicVectorClass<int> heights;
		int diff = max_height - min_height;
		if (diff != 0) {
			if (diff != 4) {
				if (diff == 8) {
					heights.Add((max_height + min_height) / 2);
				}
			} else {
				heights.Add(max_height);
				heights.Add(min_height);
			}
		} else {
			if (min_height >= 4) {
				heights.Add(min_height - 4);
			}
			if (max_height <= 7) {
				heights.Add(max_height + 4);
			}
		}

		if (RandomMapGen.SeedData.MapType == 3 || RandomMapGen.SeedData.MapType == 4) {
			for (int h = heights.Count() - 1; h >= 0; h--) {
				if (heights[h] == 0) {
					heights.Delete_Index(h);
				}
			}
		}

		delete[] adjacent;

		if (region->CellCount > 100) {
			/*
			** NATIVE: with no height left to choose gamemd.exe picks past the list's end; the
			** region keeps its height here.
			*/
			if (heights.Count() > 0) {
				int old_height = region->CellHeight;
				unsigned int max_index = (unsigned int)heights.Count() - 1;
				unsigned int index = Pick_Random_UInt(0, max_index);

				region->CellHeight = heights[index];
				region->Increase_Height((char)(region->CellHeight - old_height));
			}
		} else {
			if (merge_target != NULL) {
				int old_height = region->CellHeight;
				region->CellHeight = merge_height;
				region->Increase_Height((char)(region->CellHeight - old_height));

				for (int n = region->Cells.Count() - 1; n >= 0; n--) {
					Cell c = region->Cells[n];
					merge_target->Cells.Add(c);
					merge_target->CellCount++;
					MapRegionClass::Get_Cell_Data(c).RegionID = merge_target->ID;
				}

				region->Discardable = true;
			}
		}
	}

	for (i = MapRegionClass::MapRegions.Count() - 1; i >= 0; i--) {
		MapRegionClass * region = MapRegionClass::MapRegions[i];
		if (region != this && region->Discardable) {
			delete region;
		}
	}

	SplitCount++;
	return(true);
}




// gamemd.exe 0x005981F0 (1,889 bytes; RA2's 0x00579960 has 1,897): the rebuild's text of the RA2 twin; YR reads
// RMGMD.INI, the same keys in the same order.
void MapGeneratorClass::Load_Settings(void)
{
	static char const * section = "General"; // RA2: the section name is a static pointer (0x7E0BE4), read for every key
	CCFileClass file("RMGMD.INI");
	CCINIClass * ini = new CCINIClass;
	ini->Load(file, false);

	RMGLevelLightSettings.Clear();
	RMGVegetationMinimums.Clear();
	RMGVegetationMaximums.Clear();
	TemperateOrePatchLamps.Clear();
	SnowOrePatchLamps.Clear();
	TemperateAmbientLight.Clear();
	SnowAmbientLight.Clear();

	RMGMinimumTiberium = ini->Get_Int(section, "RMGMinimumTiberium", RMGMinimumTiberium);
	RMGMaximumTiberium = ini->Get_Int(section, "RMGMaximumTiberium", RMGMaximumTiberium);
	RMGLevelLightSettings = ini->Get_IntList(section, "RMGLevelLightSettings", RMGLevelLightSettings);
	RMGVegetationMinimums = ini->Get_IntList(section, "RMGVegetationMinimums", RMGVegetationMinimums);
	RMGVegetationMaximums = ini->Get_IntList(section, "RMGVegetationMaximums", RMGVegetationMaximums);
	TemperateAmbientLight = ini->Get_IntList(section, "TemperateAmbientLight", TemperateAmbientLight);
	SnowAmbientLight = ini->Get_IntList(section, "SnowAmbientLight", SnowAmbientLight);
	TemperateAmbientRed = ini->Get_IntList(section, "TemperateAmbientRed", TemperateAmbientRed);
	TemperateAmbientGreen = ini->Get_IntList(section, "TemperateAmbientGreen", TemperateAmbientGreen);
	TemperateAmbientBlue = ini->Get_IntList(section, "TemperateAmbientBlue", TemperateAmbientBlue);
	SnowAmbientRed = ini->Get_IntList(section, "SnowAmbientRed", SnowAmbientRed);
	SnowAmbientGreen = ini->Get_IntList(section, "SnowAmbientGreen", SnowAmbientGreen);
	SnowAmbientBlue = ini->Get_IntList(section, "SnowAmbientBlue", SnowAmbientBlue);
	MaxTrees = ini->Get_Int(section, "MaxTrees", MaxTrees);

	TemperateOrePatchLamps = TGet_TypeList<BuildingTypeClass>(*ini, section, "TemperateOrePatchLamps", TemperateOrePatchLamps); // RA2: the lamp lists are read with the generic type-list reader (a 128 byte buffer inside), not by hand
	SnowOrePatchLamps = TGet_TypeList<BuildingTypeClass>(*ini, section, "SnowOrePatchLamps", SnowOrePatchLamps);

	delete ini;
	file.Close();
}


/*
** A new preview between the generator's stages (gamemd.exe repeats this block inline): drawn, the
** dialog repainted, and in a Westwood Online tour game sent to the guests.
*/
static void RMG_Refresh_Preview(bool full_init, HWND dialog)
{
	if (full_init) {
		RandomMapGen.MapPreview->Create_Preview();
		SendMessage(dialog, WM_PAINT, 0, 0);
		if (Session.Type == GAME_INTERNET && Session.WOLWelcomeChoice == 3) {
			Send_Preview_To_Guests(RandomMapGen.MapPreview);
		}
	}
}

static void RMG_Recalc_Cells(void)
{
	DebugString("RMG: Recalculating cell attributes\n");
	Map.Reset_Iterator();
	for (CellClass * cptr = Map.Iterate(); cptr != NULL; cptr = Map.Iterate()) {
		cptr->Recalc_Attributes(-1);
	}
}

static void RMG_Delete_Regions(void)
{
	for (int index = MapRegionClass::MapRegions.Count() - 1; index >= 0; index--) {
		MapRegionClass * region = MapRegionClass::MapRegions[index];
		if (region != NULL) {
			delete region;
		}
	}
	MapRegionClass::TotalCount = 0;
}

// gamemd.exe 0x00598960 (3,018 bytes; RA2's 0x0057A0D0 has 3,086). The rebuild's text of the RA2 twin
// in gamemd.exe's order: Init_Regions inline (the tree's Init_Regions, 0x005A1C70, is the same code),
// Load_Settings (RMGMD.INI) after the first preview, Fixup_Cliff_Edges after the cliffs, the start
// points (0x00594B50), Create_Tiberium, and on the snow maps the rough and sand chances of
// Seed_Arctic_Vegetation before Generate_Arctic_Vegetation. The tour check is Session +0x0C == 3.
void MapGeneratorClass::Generate_Random_Map(bool full_init, HWND dialog)
{
	if (RMGCallback != NULL) RMGCallback();

	RMGRandom = Random2Class(SeedData.Seed);
	RMGGaussian = NormalDistribution(Random_Fraction);

	if (RMGCallback != NULL) RMGCallback();

	bool show_progress = !Scen->IsReadingScenario;
	if (show_progress) {
		Progress.Initialize(100, 1, dialog);
		Progress.Set_Graphic_Data("PROGBAR2.SHP", NULL, 0);
		Progress.Set_Layout(Point2D(-1, -1), NULL, false, true);
		Progress.Display_Progress();
		HWND control = GetDlgItem(dialog, 0x638);
		if (control != NULL) {
			ShowWindow(control, SW_SHOW);
		}
	}

	if (RMGCallback != NULL) RMGCallback();

	DebugString("RMG: Init random map\n");

	Init_Map(full_init);

	if (RMGCallback != NULL) RMGCallback();

	ScenarioInit++;

	RMG_Refresh_Preview(full_init, dialog);

	if (RMGCallback != NULL) RMGCallback();

	Load_Settings();

	DebugString("RMG: Seeding water\n");

	if (SeedData.MapType == 3 || SeedData.MapType == 4) {
		if (SeedData.WaterAmount != 0) {
			Seed_Water();
		}
	} else {
		Seed_Land();
	}

	Vary_Water_Tiles();

	Update_Progress(55);

	if (RMGCallback != NULL) RMGCallback();

	RMG_Refresh_Preview(full_init, dialog);

	if (RMGCallback != NULL) RMGCallback();

	Update_Progress(60);

	if (RMGCallback != NULL) RMGCallback();

	RMG_Refresh_Preview(full_init, dialog);

	if (RMGCallback != NULL) RMGCallback();

	DebugString("RMG: Init regions\n");

	Init_Regions();

	Update_Progress(65);

	if (RMGCallback != NULL) RMGCallback();

	DebugString("RMG: Making regions\n");

	if (SeedData.MapType == 4 || SeedData.MapType == 3) {
		MapRegionClass::Make_Ice_Regions();
		MapRegionClass::Make_Regions();
		Fixup_Cliff_Corners();
		Map.Build_All_Cliffs(0, -1);
		Fixup_Cliff_Edges();
	}

	Update_Progress(70);

	if (RMGCallback != NULL) RMGCallback();

	RMG_Refresh_Preview(full_init, dialog);

	if (RMGCallback != NULL) RMGCallback();

	Scatter_Green_Tiles();

	RMG_Recalc_Cells();

	if (RMGCallback != NULL) RMGCallback();

	Update_Progress(75);

	DebugString("RMG: Creating starting points\n");

	do {
		while (!Generate_Starting_Points()) {}
	} while (!Init_Start_Points());

	DebugString("RMG: Adding tech buildings\n");
	if (SeedData.MapType != 0) {
		Generate_Neutral_Buildings();
	}

	if (RMGCallback != NULL) RMGCallback();

	DebugString("RMG: Creating tiberium\n");
	Create_Tiberium();

	if (RMGCallback != NULL) RMGCallback();

	if (RMGCellData != NULL) {
		int size = MapCellStride * MapCellStride;
		for (int index = 0; index < size; index++) {
			RMGCellData[index].RegionID = -1;
			RMGCellData[index].SpreadID = -1;
		}
	}

	RMG_Delete_Regions();

	RMG_Recalc_Cells();

	Update_Progress(80);

	if (RMGCallback != NULL) RMGCallback();

	RMG_Refresh_Preview(full_init, dialog);

	if (RMGCallback != NULL) RMGCallback();

	Update_Progress(85);

	if (RMGCallback != NULL) RMGCallback();

	RMG_Refresh_Preview(full_init, dialog);

	if (RMGCallback != NULL) RMGCallback();

	RMG_Recalc_Cells();

	if (RMGCallback != NULL) RMGCallback();

	DebugString("RMG: Creating hills\n");
	Generate_Hills();

	Update_Progress(90);

	if (RMGCallback != NULL) RMGCallback();

	RMG_Refresh_Preview(full_init, dialog);

	if (RMGCallback != NULL) RMGCallback();

	DebugString("RMG: Creating LATs, rocks etc\n");

	if (SeedData.Biome == 0) {
		Seed_Vegetation();
		Generate_Vegetation();
	} else {
		Seed_Arctic_Vegetation();
		Generate_Arctic_Vegetation();
	}

	if (RMGCallback != NULL) RMGCallback();

	Update_Progress(95);

	if (RMGCallback != NULL) RMGCallback();

	RMG_Refresh_Preview(full_init, dialog);

	ScenarioInit--;

	if (RMGCallback != NULL) RMGCallback();

	RMG_Recalc_Cells();

	if (RMGCallback != NULL) RMGCallback();

	TiberiumClass::Init_Tiberium_Growth_System();
	TiberiumClass::Init_Tiberium_Spread_System();

	DebugString("RMG: Cleanup\n");

	SeededWaterAmount = 0;
	WorkingRegionID = 0;

	if (RMGCellData != NULL) {
		delete [] RMGCellData;
		RMGCellData = NULL;
	}

	RMG_Delete_Regions();

	Map.Overpass();
	DebugString("RMG: Compute Radar Image\n");
	Map.Set_Local_Dimensions(Map.LocalRect);
	Map.Compute_Radar_Image();

	Update_Progress(100);

	DebugString("RMG: Done\n");

	if (show_progress) {
		HWND control = GetDlgItem(dialog, 0x638);
		if (control != NULL) {
			ShowWindow(control, SW_HIDE);
		}
		Update_Progress(0);
		Progress.Clear_Dialog();
	}
}






