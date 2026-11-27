// SPDX-License-Identifier: GPL-3.0-or-later
#include "slavemgr.h"	// YR (lane P57): SlaveManagerClass (Exit_Object's Deploy2, the deconstruction's FUN_006B0DB0 and SetOwner)
#include "airctype.h"	// YR (P61): AircraftTypeClass (Exit_Object's IsAirportBound)

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y4), appended to the building unit: BuildingClass::Exit_Object
// (TechnoClass *, Cell), unmatched in ra2-decomp. TibSun's text of Exit_Object(TechnoClass *)
// (TibSun archive code/building.cpp; SPDX GPL-3.0-or-later, Copyright 2025 Electronic Arts Inc.
// and OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md); native changes are marked
// NATIVE. YR's second parameter (a preferred cell) goes to Find_Exit_Cell for vehicles only (lane Y6,
// compared with gamemd.exe 0x00443C60: infantry and the other factory get CELL_NONE). Behaviour code
// for the native build only; otherwise gamemd.exe's version is not compared.
// NATIVE (lane N3): TibSun's Assign_Mission(m) is written QueueMission(m, false), what gamemd.exe calls there
// (vtable slot 0x1E8, MissionClass::QueueMission 0x005B35E0, e.g. 0x004445F0 for the war factory's unload): the
// YR tree's ObjectClass::Assign_Mission is an empty virtual, so the war factory never unloaded and every
// mission given in these drafts was dropped. (The same in all of this file's drafts.)
// ---------------------------------------------------------------------------------------------
int BuildingClass::Exit_Object(TechnoClass * base, Cell preferred)	// NATIVE: YR's preferred cell passed on to Find_Exit_Cell for vehicles
{
	if (!base) return(0);

	/*
	**	A unit exiting a building is always considered to be "locked". That means, it
	**	will be considered as to have legally entered the visible map domain.
	*/
	base->IsInPlayfield /* NATIVE: YR's name of IsLocked */ = true;

	/*
	**	Find a good cell to unload the object to. The object, probably a vehicle
	**	will drive/walk to the adjacent free cell.
	*/
	Cell cell;

	switch (base->Fetch_RTTI()) {

		case RTTI_AIRCRAFT:

			House->Update_Production_Mode(RTTI_AIRCRAFT);
			House->BuildAircraft = AIRCRAFT_NONE;

			/*
			**	YR (P61, gamemd.exe 0x00443CB4-0x00444091): an aircraft goes onto a free pad (RadioClass::HasFreeLink:
			**	any empty link, so an Airforce Command's four pads each take one), tethered to it (RADIO_TETHER to the
			**	aircraft itself), its position set to the pad's docking point and the pad noted (DockNowHeadingTo); an
			**	airport-bound aircraft (IsAirportBound) is not sent to the rally point and, with no free pad, is not
			**	made at all (return 0, the factory waits). TibSun's text took only the first link (In_Radio_Contact),
			**	so a second Harrier flew in from the map edge, and sent TETHER to the first link. gamemd.exe's ion storm
			**	test (FUN_0053A130) is always false, so its storm placement is never reached.
			*/
			if (HasFreeLink((TechnoClass *)base)) {
				AircraftClass * air = (AircraftClass *)base;

				air->Set_Height(0);
				ScenarioInit++;

				if (air->Unlimbo(Docking_Coord(air), air->Pose_Dir())) {
					Transmit_Message(RADIO_HELLO, air);
					Transmit_Message(RADIO_TETHER, air);
					air->Set_Coord(Docking_Coord(air));
					air->DockNowHeadingTo = this;

					if (ArchiveTarget != NULL && !air->Class->IsAirportBound) {
						air->Assign_Destination(ArchiveTarget);
						air->QueueMission(MISSION_MOVE, false);
					}
					ScenarioInit--;
					return(2);
				}
				ScenarioInit--;
				return(0);

			} else if (((AircraftClass *)base)->Class->IsAirportBound) {
				return(0);

			} else {

				/*
				 * The aircraft has a pad to land on, so spawn it at the edge of the
				 * visible map and let it fly in.
				 */
				int x = Map.LocalRect.X;
				cell = Cell(x, -x) + Cell(1, Map.PlayRect.Width) + Cell(Map.LocalRect.Y, Map.LocalRect.Y);

				Cell diff;
				diff = Center_Coord().As_Cell() - cell;

				if (diff.X - diff.Y > Map.LocalRect.Width) {
					cell = Cell(cell.X - 1, cell.Y) + Cell(Map.LocalRect.Width, -Map.LocalRect.Width);
				} else {
					cell = Cell(cell.X - 1, cell.Y);
				}

				Cell spawncell = cell;
				int random = Random_Pick(0, Map.LocalRect.Height);
				spawncell = spawncell + Cell(random, random);

				ScenarioInit++;
				if (base->Unlimbo(Coord(spawncell, 0), DIR_N)) {
					if (ArchiveTarget != NULL) {
						base->Assign_Destination(ArchiveTarget);
						base->QueueMission(MISSION_MOVE, false);
					} else {
						cell = base->Nearby_Location(this);
						if (cell != CELL_NONE) {
							base->Assign_Destination(&Map[cell]);
							base->QueueMission(MISSION_MOVE, false);
						} else {
							base->Assign_Destination(NULL);
							base->QueueMission(MISSION_MOVE, false);
						}
					}
					ScenarioInit--;
					return(2);
				}
				ScenarioInit--;
			}
			break;

		case RTTI_INFANTRY:
		case RTTI_UNIT:

			if (!Class->IsHospital) {

				/*
				 * If a unit is already exiting and this building doesn't allow more than
				 * one exit at a time, then refuse the exit attempt for now.
				 * YR (P61, gamemd.exe 0x004440BC): refused only when every radio link is taken (HasFreeLink).
				 */
				if (!Class->IsArmory && !Class->IsWeaponsFactory && !HasFreeLink()) {
					return(1);
				}
			}

			if (!Class->IsHospital && !Class->IsArmory) {
				House->Update_Production_Mode(base->Fetch_RTTI());

				if (base->Fetch_RTTI() == RTTI_UNIT) {
					House->BuildUnit = UNIT_NONE;
				}
				if (base->Fetch_RTTI() == RTTI_INFANTRY) {
					House->BuildInfantry = INFANTRY_NONE;
				}
			}

			if (!Class->IsRefinery && !Class->IsWeeder) {

				if (Class->IsWeaponsFactory && Class->Naval) {

					/*
					**	YR (P61, gamemd.exe 0x00444167-0x0044448F): a naval yard puts the boat straight onto the water:
					**	from the yard's centre cell towards the rally point (ArchiveTarget) to the first cell outside the
					**	yard; that cell when it is open water inside the visible map, else the nearest place the boat can
					**	go (MapClass::Nearby_Location for its SpeedType); the boat faces east, is sent to the rally
					**	point, and its position is set to the cell's coordinate. No radio contact; the yard plays its
					**	unload mission while no link is held. TibSun's text took the war factory's path (the exit
					**	coordinate, a tethered unload).
					*/
					if (!HasAnyLink()) {
						QueueMission(MISSION_UNLOAD, false);
					}
					Cell exitcell = Center_Coord().As_Cell();
					if (ArchiveTarget != NULL) {
						Cell rally = ArchiveTarget->Center_Coord().As_Cell();
						FacingType facing = ::Direction(Get_Cell(), rally).As_Dir8();
						while (Map[exitcell].Cell_Building() == this) {
							exitcell = Adjacent_Cell(exitcell, facing);
						}
					}
					if (ArchiveTarget == NULL || Map[exitcell].Land_Type() != LAND_WATER || Map[exitcell].Cell_Techno() != NULL
							|| !Map.In_Local_Radar(exitcell, true)) {
						Cell center = Center_Coord().As_Cell();
						exitcell = Map.Nearby_Location(center, base->Techno_Type_Class()->Speed);
					}
					if (!base->Unlimbo(Map[exitcell].Center_Coord(), DIR_E)) {
						return(0);
					}
					if (ArchiveTarget != NULL) {
						base->Assign_Destination(ArchiveTarget);
						base->QueueMission(MISSION_MOVE, false);
					}
					base->Mark(MARK_UP);
					base->Set_Coord(Map[exitcell].Cell_Coord());
					base->Mark(MARK_DOWN);
					return(2);

				} else if (Class->IsWeaponsFactory) {

					base->ArchiveTarget = ArchiveTarget;

					if (Mission == MISSION_UNLOAD) {
						for (int index = 0; index < Buildings.Count(); index++) {
							BuildingClass * bldg = Buildings[index];
							if (bldg->House == House && bldg->Class == Class && bldg != this && bldg->Mission == MISSION_GUARD && !bldg->Factory) {
								FactoryClass * temp = Factory;
								bldg->Factory = Factory;
								Factory = NULL;
								int retval = (bldg->Exit_Object(base, CELL_NONE));	// gamemd.exe 0x00444542: no preferred cell for the other factory
								bldg->Factory = NULL;
								Factory = temp;
								return(retval);
							}
						}
						return(1);	// fail while we're still unloading previous
					}
					ScenarioInit++;
					if (base->Unlimbo(Exit_Coord(0), DIR_E)) {
						base->Mark(MARK_UP);
						base->Set_Coord(Exit_Coord(0));
						base->Mark(MARK_DOWN);
						Transmit_Message(RADIO_HELLO, base);
						Transmit_Message(RADIO_TETHER);
						QueueMission(MISSION_UNLOAD, false);
						ScenarioInit--;
						return(2);
					}

				} else if (Class->ToBuild != RTTI_INFANTRYTYPE && !Class->IsHospital && !Class->IsArmory && !Class->Cloning) {	// YR (P61): not a cloning vat (0x0044462A)

					Cell exitcell = Find_Exit_Cell(base, preferred);
					if (exitcell == CELL_NONE) {
						return(0);
					}

					Dir256 dir = ::Direction(Center_Coord(), Coord(exitcell)).As_Dir256();

					Cell to = exitcell;
					cell = Get_Cell();
					int x = cell.X;
					if (to.X >= x + Class->Width()) {
						to.X -= 1;
					} else if (to.X < cell.X) {
						to.X += 1;
					}
					int y = cell.Y;
					if (to.Y >= y + Class->Height()) {
						to.Y -= 1;
					} else if (to.Y < cell.Y) {
						to.Y += 1;
					}

					/*
					 * The barracks types have a special exit door location that the
					 * exiting object should be placed at.
					 */
					Coord exitcoord = Coord(to, 0);
					if (Class->IsGDIBarracks) {
						if (exitcell.X == x + 1 && exitcell.Y == y + 2) {
							exitcoord = exitcoord + Class->ExitCoordinate;
						}
					}
					if (Class->IsNODBarracks && exitcell.X == x + 2 && exitcell.Y == y + 2) {
						exitcoord = exitcoord + Class->ExitCoordinate;
					}
					// YR (P61, gamemd.exe 0x00444857, 0x00444C2C): Yuri's barracks' door is at + 2,1
					if (Class->IsYuriBarracks && exitcell.X == x + 2 && exitcell.Y == y + 1) {
						exitcoord = exitcoord + Class->ExitCoordinate;
					}

					ScenarioInit++;
					if (base->Unlimbo(exitcoord, dir)) {

						base->QueueMission(MISSION_MOVE, false);
						base->Assign_Destination(&Map[exitcell]);

						/*
						 * When the computer produces a unit, it has the unit guard an
						 * area around the spot it should head to.
						 */
						if (!House->Is_Human_Player()) {
							base->QueueMission(MISSION_GUARD_AREA, false);

							cell = House->Where_To_Go((FootClass *)base);
							base->Assign_Archive_Target((cell != CELL_NONE && Class->ToBuild != RTTI_NONE) ? &Map[cell] : NULL);
						}

						ScenarioInit--;
						return(2);
					}

				} else {

					base->ArchiveTarget = ArchiveTarget;
					Coord exitcoord;

					Cell exitcell = Find_Exit_Cell(base, CELL_NONE);	// gamemd.exe 0x004449AF: infantry ignore the preferred cell
					if (exitcell == CELL_NONE) {
						return(0);
					}

					/*
					**	YR (P61, gamemd.exe 0x004449D9-0x00444A51): a soldier leaving a barracks (not a cloning vat)
					**	is copied by each of the house's Cloning Vats: a new one of its type for the vat's house, put
					**	out of the vat by its own Exit_Object. TibSun's text had no cloning: the vats never cloned.
					*/
					if (Class->ToBuild == RTTI_INFANTRYTYPE && !Class->Cloning) {
						TechnoTypeClass const * type = base->Techno_Type_Class();
						for (int index = 0; index < House->CloningVats.Count(); index++) {
							BuildingClass * vat = House->CloningVats[index];
							vat->Exit_Object((TechnoClass *)type->Create_One_Of(vat->House), CELL_NONE);
						}
					}

					Coord exitcellcoord(exitcell);
					Dir256 dir = ::Direction(Center_Coord(), exitcellcoord).As_Dir256();

					Cell to = exitcell;
					cell = Get_Cell();
					int x = cell.X;
					if (to.X >= x + Class->Width()) {
						to.X -= 1;
					} else if (to.X < cell.X) {
						to.X += 1;
					}
					int y = cell.Y;
					if (to.Y >= y + Class->Height()) {
						to.Y -= 1;
					} else if (to.Y < cell.Y) {
						to.Y += 1;
					}

					/*
					 * The barracks types have a special exit door location that the
					 * exiting object should be placed at.
					 */
					exitcoord = Coord(to, 0);
					if (Class->IsGDIBarracks) {
						if (exitcell.X == x + 1 && exitcell.Y == y + 2) {
							exitcoord = exitcoord + Class->ExitCoordinate;
						}
					}
					if (Class->IsNODBarracks && exitcell.X == x + 2 && exitcell.Y == y + 2) {
						exitcoord = exitcoord + Class->ExitCoordinate;
					}
					// YR (P61, gamemd.exe 0x00444857, 0x00444C2C): Yuri's barracks' door is at + 2,1
					if (Class->IsYuriBarracks && exitcell.X == x + 2 && exitcell.Y == y + 1) {
						exitcoord = exitcoord + Class->ExitCoordinate;
					}

					ScenarioInit++;
					if (base->Unlimbo(exitcoord, dir)) {

						if (((FootClass *)base)->NavCom != NULL) {
							base->ArchiveTarget = ((FootClass *)base)->NavCom;
						}

						base->QueueMission(MISSION_MOVE, false);
						base->Assign_Destination(&Map[exitcell]);

						/*
						 * When the computer produces infantry, it has the infantry guard
						 * an area around the spot it should head to.
						 */
						if (!House->Is_Human_Player()) {
							base->QueueMission(MISSION_GUARD_AREA, false);

							cell = House->Where_To_Go((FootClass *)base);
							if (cell != CELL_NONE && Class->ToBuild != RTTI_NONE) {
								base->ArchiveTarget = &Map[cell];
								((FootClass *)base)->Queue_Navigation_List(&Map[cell]);
							} else {
								base->Assign_Archive_Target(NULL);
							}
						}

						/*
						**	Establish radio contact so unload coordination can occur. This
						**	radio contact should always succeed.
						*/
						if (Transmit_Message(RADIO_HELLO, base) == RADIO_ROGER) {
							Transmit_Message(RADIO_UNLOAD);
						}
						ScenarioInit--;
						return(2);
					}
				}

			} else {

				/*
				 * A unit exits a refinery to the south west corner and heads off to
				 * harvest. Infantry just scatters out.
				 */
				if (base->Fetch_RTTI() == RTTI_UNIT) {

					cell = Adjacent_Cell(Center_Coord().As_Cell(), FACING_SW);

					ScenarioInit++;
					if (base->Unlimbo(Coord(Adjacent_Cell(cell, FACING_S), 0), DIR_SW)) {
						base->PrimaryFacing.Set(DirType(DIR_S));
						base->QueueMission(MISSION_HARVEST, false);
					}

				} else {
					base->Scatter(COORD_NONE, true, false);
					return(0);
				}
			}
			ScenarioInit--;
			break;

		case RTTI_BUILDING:

			if (!House->Is_Human_Player()) {

				House->Update_Production_Mode(RTTI_BUILDING);
				House->BuildStructure = STRUCT_NONE;

				/*
				**	Find the next available spot to place this newly created building. If the
				**	building could be placed at the desired location, fine. If not, then this
				**	routine will return failure. The calling routine will probably abandon this
				**	building in preference to building another.
				*/
				BuildingClass * building = (BuildingClass *)base;
				BaseNodeClass * node = House->Base.Next_Buildable(building->Class->HeapID);
				Coord placementcoord(0, 0, 0);
				BuildingTypeClass * buildingtype = building->Class;

				if (node != NULL && node->CellID != Cell(0, 0)) {

					if (buildingtype->PowersUpBuilding[0] != '\0' || House->Can_Build_Here(buildingtype, node->CellID)) {
						placementcoord = Coord(node->CellID);
					} else {

						/*
						**	Find a suitable new spot to place.
						*/
						placementcoord = Coord(House->Where_To_Place_Building(buildingtype, HouseClass::Base_Cell_Weight_By_Distance, -1));
						if (placementcoord == COORD_NONE) {
							return(0);
						}
						node->CellID = placementcoord.As_Cell();
					}

				} else {

					if (buildingtype->PowersUpBuilding[0] == '\0') {
						placementcoord = Coord(House->Where_To_Place_Building(buildingtype, HouseClass::Base_Cell_Weight_By_Distance, -1));
					} else {
						cell = House->Where_To_Place_Upgrade(buildingtype);
						if (cell != CELL_NONE) {
							placementcoord = Coord(cell);
						}
					}

					if (node != NULL && placementcoord != COORD_NONE) {
						node->CellID = placementcoord.As_Cell();
					}
				}

				if (placementcoord != COORD_NONE) {

					Cell cellnum = placementcoord.As_Cell();
					switch (((BuildingTypeClass *)base->Class_Of())->Flush_For_Placement(cellnum, House)) {

						default:
							return(0);

						case 1:
							/*
							**	YR (P57): gamemd.exe 0x00445237: the node's failures are counted
							**	(BaseClass::FailedToPlaceNode); in a multiplayer game a node that has
							**	failed more than Rule->MaximumBuildingPlacementFailures times is dropped.
							*/
							{
								int failures = House->Base.FailedToPlaceNode(node);
								if (Session.Type != GAME_NORMAL && failures > Rule->MaximumBuildingPlacementFailures && node != NULL) {
									int index = House->Base.Nodes.ID(node);
									if (index < House->Base.Nodes.Count()) {
										House->Base.Nodes.Delete_Index(index);
									}
								}
							}
							return(1);

						case 0:
							if (base->Unlimbo(placementcoord)) {
								// YR (P57): 0x004452F0: a placed slave miner sets its slaves to wait (SlaveManagerClass::Deploy2).
								if (building->SlaveManager != NULL) {
									building->SlaveManager->Deploy2();
								}
								if (node != NULL) {
									if (building->Class->HeapID == House->BuildStructure) {
										House->BuildStructure = STRUCT_NONE;
									}
								}

								if (base->CurrentMission == MISSION_NONE && base->MissionQueue == MISSION_CONSTRUCTION) {
									base->Commence();
								}

								/*
								**	YR (P57): 0x00445345: a placed wall piece becomes the wall overlay (a firestorm
								**	wall or a wall with its ToOverlay), filling the gaps to the house's walls
								**	along each side (MapClass::Place_Firestorm_Wall 0x00588570, Place_Wall 0x00588750).
								*/
								if (base->Fetch_RTTI() == RTTI_BUILDING) {
									if (building->Class->IsFirestormWall) {
										Map.Place_Firestorm_Wall(cellnum, House, building->Class);
									} else if (building->Class->ToOverlay != NULL && building->Class->ToOverlay->IsWall) {
										Map.Place_Wall(cellnum, House, building->Class);
									}
								}

								/*
								 * When a wall tower is placed, the next base defense node is
								 * moved to the spot the tower actually appeared at.
								 */
								if (building->Class == Rule->WallTower) {
									int index = House->Base.Nodes.ID(node) + 1;
									int count = House->Base.Nodes.Count();
									for (; index < count; index++) {
										if (House->Base.Nodes[index].Type >= STRUCT_FIRST && BuildingTypes[House->Base.Nodes[index].Type]->IsBaseDefense) {
											House->Base.Nodes[index].CellID = base->Get_Coord().As_Cell();
											break;
										}
									}
								}
								return(2);
							}
							// fall through

						case 2:
							if (node == NULL) {
								return(0);
							} else {
								int index = House->Base.Nodes.ID(node);

								BuildingTypeClass * nodetype = BuildingTypes[node->Type];
								if (!nodetype->IsWall && !nodetype->IsGate) {
									Cell placecell = placementcoord.As_Cell();
									for (int i = 0; i < House->Base.Nodes.Count(); i++) {
										if (House->Base.Nodes[i].CellID == placecell) {
											House->Base.Nodes[i].CellID = Cell(0, 0);
										}
									}
									return(0);
								}
								House->Base.Nodes.Delete_Index(index);
							}
							break;
					}

				} else {

					if (buildingtype->PowersUpBuilding[0] != '\0') {
						if (House->Base.Next_Buildable(STRUCT_NONE) == node) {
							House->Base.Nodes.Delete_Index(House->Base.Next_Buildable_Index(STRUCT_NONE));
						}
					}
				}
			}
			break;

		default:
			break;
	}

	/*
	**	Failure to exit the object results in a false return value.
	*/
	return(0);
}

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
