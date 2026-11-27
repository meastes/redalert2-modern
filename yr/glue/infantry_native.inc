// SPDX-License-Identifier: GPL-3.0-or-later
#include "capmgr.h"   // CaptureManagerClass (Per_Cell_Process's boarding, lane P18)
#include "revent.h"   // RadarEventClass (Per_Cell_Process's bridge repair, lane P61)

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y4), appended to the infantry unit: InfantryClass::Assign_Destination
// (AbstractClass *, bool), YR 0x0051AA40 (1,953 bytes, unmatched in ra2-decomp). TibSun's text
// (TibSun archive code/infantry.cpp; SPDX GPL-3.0-or-later, Copyright 2025 Electronic Arts Inc. and
// OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md) with gamemd.exe's changes, read
// from its code and marked YR: the deploy-sequence and same-cell early returns, the rocker link
// release, the transport queue (QueueUpToEnter) and the docking reply, the jump jet's fly test that
// also honours the type's HoverAttack flag (+0x390). Behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------
void InfantryClass::Assign_Destination(AbstractClass * target, bool immediate)
{
	/*
	**	YR: a human player's soldier in one of the deploy sequences (DoType 27-30) keeps it.
	*/
	if (Owner_HouseClass()->Is_Human_Player()) {
		if (Doing == (DoType)27 || Doing == (DoType)28 || Doing == (DoType)29 || Doing == (DoType)30) {
			return;
		}
	}

	/*
	**	YR: a jump jet soldier told to go to the cell it is in ignores the order.
	*/
	if (Class->JumpJet && target != NULL) {
		Cell here = Center_Coord().As_Cell();
		Cell there = target->Center_Coord().As_Cell();
		if (here == there) {
			return;
		}
	}

	/*
	**	YR: break the rocker link with the unit this one was paired with.
	*/
	if (DirectRockerLinkedUnit != NULL) {
		DirectRockerLinkedUnit->DirectRockerLinkedUnit = NULL;
		DirectRockerLinkedUnit = NULL;
	}

	/*
	**	Special flag so that infantry will start heading in the right direction immediately.
	*/
	if (Locomotion->Is_Moving() && target != NULL && Map[Center_Coord()].Is_Clear_To_Move(Class->Speed, true, false)) {
		if (Class->JumpJet || CurrentMission != MISSION_ATTACK || NavCom != target) {
			Stop_Driver();
		}
	}

	/*
	**	When telling an infantry soldier to move to a location twice, then this
	**	means that movement is more important than safety. Get up and run!
	*/
	if (House->Is_Human_Player() && target != NULL && NavCom == target && IsProne && !Class->IsFraidyCat && !Class->IsCyborg) {
		Do_Action(DO_GET_UP);
	}

	/*
	**	Handle entry logic here.
	*/
	if (Get_Mission() == MISSION_ENTER || MissionQueue == MISSION_ENTER) {

		/*
		**	If not already in radio contact (presumed with the transport), then
		**	either try to establish contact if allowed, or just move close and
		**	wait until radio contact can be established.
		*/
		if (!HasAnyLink()) {
			TechnoClass * techno = Dynamic_Cast<TechnoClass *>(target);
			if (techno != NULL) {

				/*
				**	Determine if the transport is already in radio contact. If so, then just move
				**	toward the transport and try to establish contact at a later time.
				*/
				if (techno->HasAnyLink()) {
					if (techno->Fetch_RTTI() == RTTI_UNIT || techno->Fetch_RTTI() == RTTI_BUILDING) {
						/*
						**	YR: queue up to enter the busy transport or building instead of walking to it.
						*/
						QueueUpToEnter = techno;
						ArchiveTarget = NULL;
						target = NULL;
						BASECLASS::Assign_Destination(target, immediate);
						return;
					}
					ArchiveTarget = target;
				} else {
					if (Transmit_Message(RADIO_HELLO, techno) == RADIO_ROGER) {
						RadioMessageType reply = Transmit_Message(RADIO_DOCKING);
						if (reply == RADIO_DOCKING) {
							return;		// YR
						}
						if (reply != RADIO_ROGER) {
							Transmit_Message(RADIO_OVER_OUT);
						}
					}
				}
			}
		} else {
			Path[0] = FACING_NONE;
		}
	} else {
		Path[0] = FACING_NONE;
	}

	if (target != NULL && Class->JumpJet && Locomotion->Is_Moving()) {
		IPersistPtr persist(Locomotion);
		CLSID clsid;
		persist->GetClassID(&clsid);
		if (clsid == CLSID_WalkLocomotion) {
			NavQueue.Add_Head(target);
			target = Get_Target_Cell_Ptr();
			if (target != NULL && ((CellClass *)target)->IsUnderBridge) {
				target = NULL;
			}
		}
	}

	if (Class->JumpJet && target != NULL && !Locomotion->Is_Moving()) {
		bool should_fly = Should_JumpJet_Fly(Destination_Coord().As_Cell(), target->Center_Coord().As_Cell());
		bool jumpjet = Is_JumpJet();
		if (Class->HoverAttack) {
			should_fly = true;		// YR: the type's flag at +0x390 always flies
		}
		if (jumpjet) {
			if (!should_fly) {
				IPiggybackPtr piggy(Locomotion);
				if (piggy != NULL) {
					if (piggy->Is_Piggybacking() && piggy->Is_Ok_To_End()) {
						piggy->End_Piggyback(&Locomotion);
					}
				}
				ILocomotionPtr walk(CLSID_WalkLocomotion);
				walk->Link_To_Object(this);
				piggy = IPiggybackPtr(walk);
				if (piggy != NULL) {
					piggy->Begin_Piggyback(Locomotion);
					Locomotion = walk;
				}
			}
		} else {
			if (should_fly) {
				IPiggybackPtr piggy(Locomotion);
				if (piggy != NULL) {
					if (piggy->Is_Piggybacking() && piggy->Is_Ok_To_End()) {
						piggy->End_Piggyback(&Locomotion);
					}
				}
			}
		}
	}

	BASECLASS::Assign_Destination(target, immediate);
}

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y4): InfantryClass::Per_Cell_Process (PCPType), unmatched in
// ra2-decomp. TibSun's text (TibSun archive code/infantry.cpp; same licence notice as above);
// native changes are marked NATIVE. Behaviour code for the native build only; gamemd.exe's
// version is not compared yet.
// ---------------------------------------------------------------------------------------------
void InfantryClass::Per_Cell_Process(PCPType why)
{
	BStart(BENCH_PCP);
	CellClass * cellptr = &Map[Get_Coord()];

	if (why == PCP_END) {

		/*
		**	YR (lane N4, from gamemd.exe 0x00519675-0x00519739): an occupier (or an assaulter) that reaches
		**	the cell of the building it was sent to capture garrisons it (FUN_00522910: into the building's
		**	Occupants; an assaulter clears it instead); a building that can no longer take it sends it away.
		**	TibSun's text had no garrisons: the soldier fell through to the capture code below, which
		**	deletes a unit that is neither an engineer nor a spy, so a GI sent into a building vanished.
		*/
		if (Get_Mission() == MISSION_CAPTURE && (Class->Occupier || Class->Assaulter)
				&& NavCom != NULL && NavCom->Fetch_RTTI() == RTTI_BUILDING
				&& cellptr->Cell_Building() == (BuildingClass *)NavCom) {
			BuildingClass * building = (BuildingClass *)NavCom;
			if (building->CanBeOccupiedBy(this)) {
				if (building->Tag) {
					building->Tag->Spring(TEVENT_PLAYER_ENTERED, this);
				}
				FUN_00522910(building);
			} else {
				Assign_Destination(NULL, true);
				Scatter(building->Center_Coord(), true, true);
			}
			BEnd(BENCH_PCP);
			return;
		}

		/*
		**	YR (lane P61, gamemd.exe 0x0051973C-0x00519945): a soldier sent into a grinder (MISSION_EATEN) that
		**	reaches the building's cell is ground up: its die voice (the player's only) and die sound, the
		**	building's house refunded the soldier's Refund_Amount, its tag sprung, the grinder's sound and
		**	grinding animation, and the soldier deleted. TibSun's text had no grinder: the soldier was never
		**	ground. UnitClass::Per_Cell_Process's matched text has the same steps.
		*/
		if (Get_Mission() == MISSION_EATEN) {
			BuildingClass * building = cellptr->Cell_Building();
			if (building != NULL && (building == NavCom || building == TarCom)) {
				if (building->Tag) {
					building->Tag->Spring(TEVENT_PLAYER_ENTERED, this);
				}
				if (Techno_Type_Class()->VoiceDie.Count() > 0 && House->Is_Player_Control()) {
					Coord coord = PositionCoord;
					TypeList<int> const & sounds = Techno_Type_Class()->VoiceDie;
					unsigned int rnd = NonCriticalRandomNumber();
					VocClass::PlayAt(sounds[rnd % (unsigned int)sounds.Count()], coord, NULL);
				}
				if (Techno_Type_Class()->DieSound.Count() > 0) {
					Coord coord = PositionCoord;
					TypeList<int> const & sounds = Techno_Type_Class()->DieSound;
					unsigned int rnd = NonCriticalRandomNumber();
					VocClass::PlayAt(sounds[rnd % (unsigned int)sounds.Count()], coord, NULL);
				}
				building->House->Refund_Money(Refund_Amount());
				if (Tag) {
					Tag->Spring(TEVENT_DESTROYED_ANY, this);
				}
				if (building->Class->Grinding) {
					VocClass::PlayAt(Rule->EnterGrinderSound, Center_Coord(), NULL);
					if (building->Anims[BANIM_ACTIVE_ONE] != NULL) {
						building->DestroyNthAnim(BANIM_ACTIVE_ONE);
						building->PlayNthAnim(BANIM_SPECIAL_ONE, building->Get_Health_Ratio() <= Rule->ConditionYellow, false);
					}
				}
				Delete_Me();
				BEnd(BENCH_PCP);
				return;
			}
		}

		/*
		**	If the infantry unit is entering a cell that contains the building it is trying to
		**	capture, then capture it.
		*/
		if (Mission == MISSION_CAPTURE || Mission == MISSION_GUARD_AREA || Mission == MISSION_PATROL) {
			// YR (lane N4, gamemd.exe 0x00519979-0x005199BE): a building that counts as a vehicle (IsStrange, TibSun's
			// Considered_Vehicle; the YR tree's Considered_Vehicle is another slot, true for every building, so an engineer
			// sent into a bridge repair hut "captured" it as a vehicle and vanished without repairing the bridge)
			// A vehicle (not a building) still takes this branch as before: a vehicle thief's hijack (lane P1's test) is
			// done here in the draft; gamemd.exe's own path for it is not compared yet.
			if (NavCom != NULL && NavCom->Is_Techno() && (NavCom->Fetch_RTTI() == RTTI_BUILDING ? ((TechnoClass*)NavCom)->IsStrange() : ((TechnoClass*)NavCom)->Considered_Vehicle())) {
				TechnoClass * tech = (TechnoClass *)NavCom;
				if (tech != NULL) {
					if (PositionCell == tech->PositionCell) {
						if (tech->Tag) {
							tech->Tag->Spring(TEVENT_PLAYER_ENTERED, this);
						}
						tech->Detach_All(false);
						tech->Captured(House);
						tech->EnteredByInfType = Class->HeapID;
						tech->Scatter_Incoming_Infantry();
						if (Tag && Tag->Is_To_Inherit()) {
							tech->Attach_Tag(Tag);
						}
						if (Tag) {
							Tag->Spring(TEVENT_DESTROYED_ANY, this);
						}
						Delete_Me();
						BEnd(BENCH_PCP);
						return;
					} else {
						Coord destination = Locomotion->Destination();
						if (destination == COORD_NONE || PositionCell == Cell(destination)) {
							Assign_Destination(NavCom);
							BEnd(BENCH_PCP);
							return;
						}
					}
				}
			}

			TechnoClass * tech = cellptr->Cell_Building();

			if (tech != NULL && (tech == NavCom || tech == TarCom)) {
				if (tech->Tag) {
					tech->Tag->Spring(TEVENT_PLAYER_ENTERED, this);
				}

				if (Class->IsEngineer) {
					// are we trying to repair a bridge?
					if (tech->RTTI == RTTI_BUILDING && ((BuildingClass*)tech)->Class->IsBridgeRepairHut) {
						/*
						**	YR (lane P61, gamemd.exe 0x00519B90-0x00519CD9): the player's repair is a radar event
						**	(RADAREVENT_BRIDGE_REPAIRED), announced only when the event is made (a repeat close by is
						**	suppressed); the rules' RepairBridgeSound plays at the hut; a low (wood) bridge overlay
						**	(74-101) near the hut counts as the train bridge tiles do.
						*/
						if (House->Is_Player_Control() && RadarEventClass::Create(RADAREVENT_BRIDGE_REPAIRED, Get_Cell())) {
							VoxClass::Play("EVA_BridgeRepaired");
						}
						if (Rule->RepairBridgeSound != -1) {
							Coord hut = tech->PositionCoord;
							VocClass::PlayAt(Rule->RepairBridgeSound, hut, NULL);
						}

						bool train = false;
						for (int y = -2; y < 3; y++) {
							for (int x = -2; x < 3; x++) {
								Cell cell = (Cell)Cell(PositionCell + Cell(x, y));
								IsometricTileType ittype = Map[cell].ITType;
								int overlay = Map[cell].Overlay;
								if ((ittype >= IsometricTileTypeClass::TrainBridgeSet && ittype < IsometricTileTypeClass::TrainBridgeSet + TRAIN_BRIDGE_COUNT)
										|| (overlay >= 74 && overlay <= 101)) {
									train = true;
								}
							}
						}
						if (train) {
							Map.Repair_Train_Bridge(PositionCell);
						} else {
							Map.Repair_Bridge(PositionCell);
						}
						for (int i = Infantry.Count() - 1; i >= 0; i--) {
							Infantry[i]->Detach(tech, false);
						}
						tech->Scatter_Incoming_Infantry();
					}

					/*
					**	An engineer will either mega-repair a friendly or allied
					**	building or it will damage/capture an enemy building. Whether
					**	it damages or captures depends on how badly damaged the
					**	enemy building is.
					*/
					/*
					**	YR (lane P61, gamemd.exe 0x00519D47-0x00519D7C, 0x00519FA2-0x00519FF6): a neutral house's
					**	garrisonable building (IsMultiplayPassive, CanBeOccupied) is repaired as an ally's is, and a
					**	building at full strength sends the engineer away (not used up).
					*/
					else if (House->Is_Ally(tech) || (tech->House->Class->IsMultiplayPassive && ((BuildingClass *)tech)->Class->CanBeOccupied)) {
						if (tech->Strength == tech->Techno_Type_Class()->MaxStrength) {
							Assign_Destination(NULL, true);
							Scatter(tech->Center_Coord(), true, true);
							BEnd(BENCH_PCP);
							return;
						}
						tech->Renovate();
					} else {
						bool iscapturable = false;
						if (tech->RTTI == RTTI_BUILDING) {
							iscapturable = ((BuildingClass *)tech)->Class->IsCaptureable;
						}

						if (Session.Type != GAME_NORMAL && Session.Options.CrapEngineers && tech->HealthRatio > Rule->ConditionRed) {
							int maxdamage = tech->Strength - int(tech->TClass->MaxStrength * Rule->ConditionRed / 2);
							int damage = MIN((tech->TClass->MaxStrength) * ((1 - Rule->ConditionRed / 2) / 2), maxdamage);
							tech->Take_Damage(damage, 0, Rule->C4Warhead, this, true);
						} else if (iscapturable) {
							/*
							**	YR (lane P61, gamemd.exe 0x00519EB2-0x00519F2C): a building being sold, or warped
							**	out by a chrono weapon, is not captured: the engineer is sent away.
							*/
							if (tech->Get_Mission() == MISSION_DECONSTRUCTION || tech->IsBeingWarpedOut()) {
								Assign_Destination(NULL, true);
								Scatter(tech->Center_Coord(), true, true);
								BEnd(BENCH_PCP);
								return;
							}
							if (tech->Tag) {
								tech->Tag->Spring(TEVENT_PLAYER_ENTERED, this);
							}
							tech->House->IsThieved = true;
							if (Tag && Tag->Is_To_Inherit()) {
								tech->Attach_Tag(Tag);
							}
							tech->Captured(House);
							tech->EnteredByInfType = Class->HeapID;
							tech->Scatter_Incoming_Infantry();
						}
					}
				} else {
					/*
					**	YR (lane P61, gamemd.exe 0x00519FF8-0x0051A06E): a spy infiltrates (BuildingClass::Infiltrate
					**	speaks its own EVA lines); any other soldier is sent away, not deleted as in TibSun's text.
					*/
					if (Class->IsAgent) {
						((BuildingClass *)tech)->Spied_By(House);
					} else {
						Assign_Destination(NULL, true);
						Scatter(tech->Center_Coord(), true, true);
						BEnd(BENCH_PCP);
						return;
					}
				}
				if (Tag) {
					Tag->Spring(TEVENT_DESTROYED_ANY, this);
				}
				Delete_Me();
				BEnd(BENCH_PCP);
				return;

			} else {
				if (NavCom == NULL) {
					if (CurrentMission != MISSION_GUARD_AREA) {
						Enter_Idle_Mode();
					}
					if (Map[Get_Coord()].Cell_Building()) {
						Scatter(COORD_NONE, true);
					}
				}
			}
		}

		/*
		**	Infantry entering a transport vehicle will break radio contact
		**	at attach itself to the transporter.
		*/
		TechnoClass * techno;
		if (Class->IsVehicleThief) {
			techno = Get_Cell_Ptr()->Cell_Unit(IsOnBridge);
			if (techno == NULL) techno = Get_Cell_Ptr()->Cell_Aircraft(IsOnBridge);
			if (techno == NULL) techno = Get_Cell_Ptr()->Cell_Building();
		} else {
			techno = Get_Cell_Ptr()->Cell_Building();
			if (techno == NULL) techno = Get_Cell_Ptr()->Cell_Aircraft(IsOnBridge);
			if (techno == NULL) techno = Get_Cell_Ptr()->Cell_Unit(IsOnBridge);
		}

		TechnoClass * on_target_cell = NULL;
		if (NavCom != NULL && NavCom->RTTI == RTTI_CELL) {
			CellClass * cellptr = &Map[NavCom->Center_Coord()];
			if (Class->IsVehicleThief) {
				on_target_cell = cellptr->Cell_Unit(true);
				if (on_target_cell == NULL) on_target_cell = cellptr->Cell_Aircraft(true);
				if (on_target_cell == NULL) on_target_cell = cellptr->Cell_Building();
			} else {
				on_target_cell = cellptr->Cell_Building();
				if (on_target_cell == NULL) on_target_cell = cellptr->Cell_Aircraft(true);
				if (on_target_cell == NULL) on_target_cell = cellptr->Cell_Unit(true);
			}
		}

		if (Mission == MISSION_ENTER && techno != NULL && (NavCom == techno || TarCom == techno || on_target_cell != NULL && techno == on_target_cell)) {
			if (techno->Tag) {
				techno->Tag->Spring(TEVENT_PLAYER_ENTERED, this);
			}
			if (techno->RTTI == RTTI_BUILDING) {
				if (techno == Get_Cell_Ptr()->Cell_Building() && Transmit_Message(RADIO_IM_IN) == RADIO_ROGER) {
					/*
					**	YR (lane P61, gamemd.exe 0x0051A2AD-0x0051A39D): the soldier drops its weapon state and any
					**	mind control; a bio reactor (InfantryAbsorb) plays EnterBioReactorSound and marks it
					**	Absorbed; a soldier counted as its house's infantry (CountedAsOwnedSpecial) stops counting
					**	(the building's sale counts it again: Do_MISSION_DECONSTRUCTION, lane P57); the bio reactor
					**	takes no radio HELLO but has its house's power recalculated when it gives ExtraPower; the
					**	soldier stops moving (AbortMotion). TibSun's text sent HELLO to every building and cleared NavCom.
					*/
					BuildingClass * building = (BuildingClass *)techno;
					MissionAccumulateTime = 0;
					sub_70DE00(0);
					SetCurrentWeaponStage(0);
					if (MindControlledBy != NULL && MindControlledBy->CaptureManager != NULL) {
						MindControlledBy->CaptureManager->FreeUnit(this);
					}
					if (building->Class->InfantryAbsorb) {
						VocClass::PlayAt(Rule->EnterBioReactorSound, Center_Coord(), NULL);
						Absorbed = true;
					}
					Limbo();
					if (building->IsAbsorbAllowed() && CountedAsOwnedSpecial) {
						House->CurInfantry--;
						CountedAsOwnedSpecial = false;
					}
					if (!building->Class->InfantryAbsorb) {
						Transmit_Message(RADIO_HELLO, techno);
					} else if (building->Class->ExtraPower > 0) {
						building->House->RecalcPower = true;
					}
					techno->Cargo.Attach(this);
					AbortMotion();
					BEnd(BENCH_PCP);
					return;
				}
			} else {
				if (Get_Cell() == techno->Get_Cell() && techno == NavCom) {
					if (Transmit_Message(RADIO_CAN_LOAD, techno) == RADIO_ROGER) {
						/*
						**	YR (lane P18, from gamemd.exe 0x0051A3FC-0x0051A47E, the steps of UnitClass::Per_Cell_Process's
						**	matched text): the soldier drops its weapon state and any mind control, and boards through the
						**	transport's AddPassenger (virtual), so a gunner transport (the IFV) takes its weapon and turret
						**	from the passenger's IFVMode (FootClass::AddPassenger -> UnitClass::ReceiveGunner). TibSun's
						**	text attached it to the cargo directly: an engineer in an IFV kept the default missile turret.
						*/
						Assign_Archive_Target(NULL);
						IsOnBridge = false;
						MissionAccumulateTime = 0;
						sub_70DE00(0);
						SetCurrentWeaponStage(0);
						if (MindControlledBy != NULL && MindControlledBy->CaptureManager != NULL) {
							MindControlledBy->CaptureManager->FreeUnit(this);
						}
						Limbo();
						if (techno->Techno_Type_Class()->OpenTopped) {
							techno->EnteredOpenTopped(this);
						}
						Transporter = techno;
						techno->AddPassenger(this);
						Hidden();
					} else {
						Assign_Destination(NULL);
						Assign_Mission(MISSION_GUARD);
						Scatter(COORD_NONE, true, true);
					}
					BEnd(BENCH_PCP);
					return;
				}
			}
		}

		/*
		**	If the infantry unit is entering a cell that contains the building it is trying to
		**	sabotage, then sabotage it.
		*/
		/*
		**	YR (lane P61, gamemd.exe 0x0051A4BF-0x0051A664): only a bomber (IsBomber) sabotages; an iron-curtained
		**	building takes no charge; a charge already set sends the soldier away; the soldier stops (AbortMotion,
		**	where TibSun's text cleared NavCom).
		*/
		if (Get_Mission() == MISSION_SABOTAGE && Class->IsBomber) {
			BuildingClass * building = cellptr->Cell_Building();
			if (building != NULL && building == NavCom) {
				if (building->Tag) {
					building->Tag->Spring(TEVENT_PLAYER_ENTERED, this);
				}
				if (building->Get_Mission() != MISSION_DECONSTRUCTION && !building->IsIronCurtained()) {
					if (building->IsGoingToBlow) {
						Assign_Destination(NULL, true);
						Arm = Rearm_Delay(true);
						Scatter(building->Center_Coord(), true, true);
						BEnd(BENCH_PCP);
						return;
					}
					building->IsGoingToBlow = true;
					building->Clicked_As_Target((Rule->C4Delay * TICKS_PER_MINUTE) / 2);
					building->CountDown = Rule->C4Delay * TICKS_PER_MINUTE;
					building->WhomToRepay = this;
				}
				AbortMotion();
				Do_Uncloak();
				Arm = Rearm_Delay(true);
				Scatter(building->Center_Coord(), true, true);	// RUN AWAY!
				BEnd(BENCH_PCP);
				return;
			} else {
				if (&Map[Center_Coord()] == NavCom) {
					Explosion_Damage(PositionCoord, Rule->BridgeStrength, this, Rule->C4Warhead);

					Stop_Driver();
					Scatter(Adjacent_Cell(PositionCoord, (FacingType)PrimaryFacing.Current().As_Dir8()), true, true);
					Assign_Mission(MISSION_MOVE);

					if (NavCom == NULL || Map[NavCom->Center_Coord()].Land_Type() == LAND_WATER) {
						Mark(MARK_DOWN);		// Needed only so that Tanya will get destroyed by the explosion.
					}
					Explosion_Damage(PositionCoord, Rule->BridgeStrength, NULL, Rule->C4Warhead);
					Explosion_Damage(PositionCoord, Rule->BridgeStrength, NULL, Rule->C4Warhead);
					if (!IsActive) {
						BEnd(BENCH_PCP);
						return;
					}

					Mark(MARK_DOWN);
				}
			}
		}

		/*
		**	If this unit is on a teather, then cut it at this time so that
		**	the "parent" unit is free to proceed. Note that the parent
		**	unit might actually be a building.
		*/
		if (IsTethered) {
			Transmit_Message(RADIO_UNLOADED);

			/*
			**	Special voice play.
			*/
			if (Class->VoiceComment.Count() > 1) {
				VocClass::PlayAt(Class->VoiceComment[1], PositionCoord);	// NATIVE (lane Y11b): YR's sound call for TibSun's Sound_Effect
			}

			/*
			**	If the cell is now full of infantry, tell them all to scatter
			**	in order to make room for more.
			*/
			if (IsOnBridge) {
				if ((cellptr->BridgeFlag.Composite & 0x01C) == 0x01C) {
					cellptr->Incoming(COORD_NONE, true, true, true);
				}
			} else {
				if ((cellptr->Flag.Composite & 0x01C) == 0x01C) {
					cellptr->Incoming(COORD_NONE, true, true, false);
				}
			}
		}

		/*
		**	When the infantry reaches the center of the cell, it may begin a new mission.
		*/
		if (MissionQueue == MISSION_NONE && NavCom == NULL && TarCom == NULL && !HasAnyLink()) {	// YR: any link (gamemd.exe 0x0051A8AA)
			Enter_Idle_Mode();
		}
		Commence();

		/*
		**	YR (lane P61, gamemd.exe 0x0051A8CB-0x0051A925, as UnitClass::Per_Cell_Process's matched text): the
		**	sight moves by vt_entry_48C (the last sight taken back) and UpdateSight, and the cells around are
		**	revealed by RevealArea3 out to the sight range + 3. TibSun's text called Look (RevealArea1).
		*/
		vt_entry_48C(false, false, false, NULL);
		UpdateSight(false, false, false, NULL, 0);
		Map.RevealArea3(PositionCoord, 0, LastSightRange + 3, 0);

		/*
		**	If after all is said and done, the unit finishes its move on an impassable cell, then
		**	it must presume that it is in the case of a unit driving onto a bridge that blows up
		**	before the unit completes it's move. In such a case the unit should have been destroyed
		**	anyway, so blow it up now.
		*/
		CellClass * cellptr = &Map[Get_Coord()];
		LandType land = cellptr->Land_Type();
		if (!Locomotion->Is_Moving() && !Class->IsBomber && !Has_Ability(ABILITY_C4) && (land == LAND_ROCK || land == LAND_WATER) && (!IsOnBridge || !cellptr->IsUnderBridge)) {
			int damage = Strength;
			Take_Damage(damage, 0, Rule->C4Warhead, NULL, true);
			BEnd(BENCH_PCP);
			return;
		}

		// YR (lane P61): no Tiberium damage here (gamemd.exe goes from the cell's land test to FootClass::Per_Cell_Process,
		// 0x0051A9EB); TibSun's text hurt a soldier standing on ore and could make a visceroid of it.
	}

	if (IsActive) {
		BASECLASS::Per_Cell_Process(why);
	}
	BEnd(BENCH_PCP);
}
