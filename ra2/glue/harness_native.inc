// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P5, branch ports-on-rebuild): appended to the generated conquer unit
// (native/rb/fixups.json "append"); called each loop frame from the Main_Loop seam
// (mainloop_native.inc). Test scaffolding, NOT original code; never part of an exact build.
//
// The order harness of this port, a small subset of the YR port's (native/yr/glue/yr_harness.cpp)
// and main's (native/link/harness_orders.cpp) under the rebuild's names, for the combat check:
// RA2_NATIVE_ORDERS="<frame> <order>; ..." (loop frames counted by the seam from the first game frame):
//   spawn <ID> [enemy] [<dx> <dy>]  one of type ID for the player (or the strongest enemy house), at the
//                                   offset in cells from the player's first building (else first unit),
//                                   else in rings around it: `harness-spawn <ID> h<house> @x,y`
//   select <ID>                     the player's first ID selected (ObjectClass::Select): its radial
//                                   indicator draws while it is selected: `harness-select <ID>`
//   fire <ID> <TARGET>              the player's first ID attacks the nearest TARGET of an enemy house
//                                   (Queue_Mission MISSION_ATTACK, the force-fire click's order); the
//                                   target is watched: `harness-fire`, then every 50 frames
//                                   `harness-watch s<strength>`, `harness-killed <TARGET> at frame <f>`
//   health                          `harness-health <ID> s<strength>` for each of the player's technos
//   draws                           `harness-draws ellipse <n> aaline <n> lasers <n>`: the calls so far of the
//                                   carried XSurface::Draw_Ellipse and DSurface::Draw_Depth_Antialiased_Line
//                                   bodies, and the laser beams seen live (LaserDrawClass::LaserDraws, counted
//                                   as each appears). In RA2 every laser weapon in use is house-coloured
//                                   (PrismShot, PrismSupport: LaserDrawClass::Draw_House_Color, which draws
//                                   with Draw_RGB_Multiplying_Line and Draw_Depth_Shaded_Line); the anti-
//                                   aliased line is reached by a radial indicator's sweep spokes
// Lane P6 (the campaign route):
//   hunt                            every one of the player's units and infantry (not the MCV) gets the hunt
//                                   mission (the AI attack teams' order): `harness-hunt <n>`
//   train <ID>                      the barracks' or war factory's cameo click: PRODUCE of the InfantryTypeClass or
//                                   UnitTypeClass ID for the player (main's harness_orders.cpp `train`): `harness-train <ID>`
//   count <ID>                      `harness-count <ID> <n>`: the player's live technos of type ID
//   super <index> enemy             the player's SuperWeapon[index] granted once and charged (Enable, Forced_Charge), then
//                                   placed (SPECIAL_PLACE, the armed cameo's map click) on the strongest enemy house's first
//                                   building: `harness-super <index> <type> @x,y -> <building> s<strength>`, the building then
//                                   watched as fire's target (main's `super <index> enemy`)
//   supers                          `harness-supers <index>:<type> ...` (the player's list)
//   state                           `harness-state <scenario> money <n> timer <frames|-> technos <n> win <0|1> lose <0|1>`
// Lane P16 (Allied 1's walk):
//   goto <ID|*> <x> <y>             main's order: the player's infantry and vehicles of type ID ('*' all) move to cell x,y
//                                   (MISSION_MOVE): `harness-goto <ID> @x,y <n>`
//   where <ID>                      `harness-where <ID> @x,y s<strength> m<mission>` (the player's first ID) or `... gone`
// Lane P17 (C14-kbd, the touch keyboard for the chat line):
//   chat open|close                 the message list's edit line (Session.Messages) opened as Main_Loop's chat key opens it
//                                   (Add_Edit) or closed (Remove_Edit), so a skirmish can show what a network game's chat does
//                                   to the on-screen keyboard: `harness-chat <open|close> edit <0|1>`
// RA2_NATIVE_TRIGGERS=1 watches the scenario's triggers each frame (main's harness_soak.cpp): `harness-triggers
// <scenario> <n>` at the first frame, then `harness-trigger <frame> gone <ID> <name>` (a live trigger left Triggers:
// a volatile trigger sprung and deleted), `... new <ID> <name>` (one made after the start) and `... tripped <ID>
// <bits>` (its tripped-events bits changed); `harness-house <frame> win|lose|defeated h<n>` as those flags change.
// ---------------------------------------------------------------------------------------------
#include "building.h"
#include "house.h"
#include "laser.h"
#include "houstype.h"
#include "objtype.h"
#include "queue.h"
#include "scenario.h"
#include "trigger.h"
#include "trigtype.h"
#include "event.h"
#include "super.h"
#include "suprtype.h"
#include "infantry.h"
#include "unit.h"
#include "unittype.h"
#include "infatype.h"
#include "target.h"
#include "techtype.h"
#include "session.h"

bool RB_Run_Input_Order(char const * word);    // harness_input_native.inc (lane P8): thumb, view
extern int RB_Draw_Ellipse_Count;              // xsurface_native.inc
extern int RB_Draw_AA_Line_Count;              // dsurface_native.inc

namespace {

struct RB_Order { int frame; int relative; int every; int due; char text[64]; };   // relative: "+<n>[/<p>]" (lane P8)
RB_Order rb_orders[64];
int rb_order_count = -1;
TechnoClass * rb_watched = NULL;
char rb_watched_type[32] = "";
bool rb_killed = false;
int rb_lasers_seen = 0;
int rb_lasers_last = 0;

void RB_Read_Orders(void)
{
	rb_order_count = 0;
	for (char const * p = getenv("RA2_NATIVE_ORDERS"); p && *p && rb_order_count < 64;) {
		while (*p == ' ' || *p == ';') ++p;
		if (!*p) break;
		RB_Order & o = rb_orders[rb_order_count++];
		o.relative = *p == '+';     // lane P8: main's "+<n>[/<p>] <order>", n game frames (Frame) into each scenario, then every p
		if (o.relative) ++p;
		o.frame = atoi(p);
		o.every = 0;
		o.due = -1;
		while (*p >= '0' && *p <= '9') ++p;
		if (o.relative && *p == '/') { ++p; o.every = atoi(p); while (*p >= '0' && *p <= '9') ++p; }
		while (*p == ' ') ++p;
		int n = 0;
		while (*p && *p != ';') { if (n < 63) o.text[n++] = *p; ++p; }
		while (n > 0 && o.text[n - 1] == ' ') --n;
		o.text[n] = 0;
	}
}

char const * RB_Type_Name(TechnoClass * techno)
{
	TechnoTypeClass const * type = techno->Techno_Type_Class();
	return(type != NULL ? type->Name() : "?");
}

bool RB_Is_Live(TechnoClass * techno)
{
	for (int i = 0; i < Technos.Count(); ++i) {
		if (Technos[i] == techno) return(true);
	}
	return(false);
}

HouseClass * RB_Enemy_House(void)
{
	HouseClass * best = NULL;
	for (int i = 0; i < Houses.Count(); ++i) {
		HouseClass * house = Houses[i];
		if (house == NULL || house == PlayerPtr || house->IsDefeated || (PlayerPtr && PlayerPtr->Is_Ally(house))) continue;
		if (house->Class && house->Class->IsMultiplayPassive) continue;
		if (best == NULL || house->Buildings.Count() > best->Buildings.Count()) best = house;
	}
	return(best);
}

TechnoClass * RB_First_Of(char const * id, HouseClass * house)
{
	for (int i = 0; i < Technos.Count(); ++i) {
		TechnoClass * techno = Technos[i];
		if (techno == NULL || techno->IsInLimbo || strcmp(RB_Type_Name(techno), id)) continue;
		if (house != NULL && techno->House != house) continue;
		return(techno);
	}
	return(NULL);
}

int RB_Spawn(char const * id, bool enemy, int dx, int dy, bool offset)
{
	ObjectTypeClass const * type = ObjectTypeClass::From_Name(id);
	HouseClass * house = enemy ? RB_Enemy_House() : PlayerPtr;
	if (type == NULL || house == NULL || PlayerPtr == NULL) {
		fprintf(stderr, "harness-spawn: no type %s or no house\n", id);
		return(-1);
	}
	Coord home;
	if (PlayerPtr->Buildings.Count() > 0) {
		home = PlayerPtr->Buildings[0]->Center_Coord();
	} else {
		TechnoClass * any = NULL;
		for (int i = 0; i < Technos.Count() && any == NULL; ++i) {
			if (Technos[i] && Technos[i]->House == PlayerPtr && !Technos[i]->IsInLimbo) any = Technos[i];
		}
		if (any == NULL) return(-1);
		home = any->Center_Coord();
	}
	ObjectClass * object = type->Create_One_Of(house);
	if (object == NULL) return(-1);
	for (int ring = offset ? 0 : 2; ring <= 12; ++ring) {
		for (int k = 0; k < 8 * (ring ? ring : 1); ++k) {
			int x = ring ? (k % (2 * ring)) - ring : 0, y = ring ? (k < 4 * ring ? -ring : ring) : 0;
			if (ring && (k / (2 * ring)) % 2) { int t = x; x = (k < 4 * ring ? ring : -ring); y = t; }
			int cx = home.X / 256 + (offset ? dx : 0) + x, cy = home.Y / 256 + (offset ? dy : 0) + y;
			Coord at(cx * 256 + 128, cy * 256 + 128, 0);
			at.Z = Map.Get_Height_GL(at);
			if (object->Unlimbo(at, DIR_N)) {
				fprintf(stderr, "harness-spawn %s h%d @%d,%d\n", id, (int)house->HeapID, cx, cy);
				return(1);
			}
		}
	}
	fprintf(stderr, "harness-spawn: no room for %s\n", id);
	delete object;
	return(-1);
}

int RB_Hunt(void)
{
	int n = 0;
	for (int i = 0; i < Technos.Count(); ++i) {
		TechnoClass * techno = Technos[i];
		if (techno == NULL || techno->House != PlayerPtr || techno->IsInLimbo || techno->Strength <= 0) continue;
		RTTIType rtti = techno->Fetch_RTTI();
		if (rtti != RTTI_UNIT && rtti != RTTI_INFANTRY) continue;
		// Lane P8: main's rule (harness_orders.cpp `hunt`): not Tanya, not an engineer, not a type that deploys (the MCV).
		TechnoTypeClass const * type = techno->Techno_Type_Class();
		if (!strcmp(RB_Type_Name(techno), "TANY") || !strcmp(RB_Type_Name(techno), "ENGINEER")) continue;
		if (type != NULL && type->DeploysInto != NULL) continue;
		TargetClass none;
		TargetClass none2;
		if (Queue_Mission(TargetClass(techno), MISSION_HUNT, none, none2)) ++n;
	}
	fprintf(stderr, "harness-hunt %d\n", n);
	return(n);
}

/*
 * The trigger watch (RA2_NATIVE_TRIGGERS=1; main's native/link/harness_soak.cpp under the rebuild's names).
 */
// TriggerClass::IsTripped is private (trigger.h); the harness reads it through a member pointer taken by an
// explicit instantiation (access checks do not apply to explicit instantiation arguments).
template<int TriggerClass::* M> struct RB_Tripped_Access { friend int TriggerClass::* RB_Tripped_Member(void) { return(M); } };
int TriggerClass::* RB_Tripped_Member(void);
template struct RB_Tripped_Access<&TriggerClass::IsTripped>;

struct RB_Seen { TriggerClass * trigger; TriggerTypeClass * type; int tripped; };
RB_Seen rb_seen[512];
int rb_seen_count = -1;
bool rb_house_win, rb_house_lose, rb_house_defeated;

char const * RB_Trigger_Id(TriggerTypeClass * type) { return(type != NULL ? type->Name() : "?"); }
char const * RB_Trigger_Name(TriggerTypeClass * type) { return(type != NULL ? (char const *)type->GivenName : "?"); }

void RB_Watch_Triggers(int frame)
{
	static RB_Seen now[512];
	int count = 0;
	for (int i = 0; i < Triggers.Count() && count < 512; ++i) {
		TriggerClass * trigger = Triggers[i];
		if (trigger == NULL) continue;
		now[count].trigger = trigger;
		now[count].type = trigger->Class;
		now[count].tripped = trigger->*RB_Tripped_Member();
		++count;
	}
	if (rb_seen_count < 0) {
		fprintf(stderr, "harness-triggers %s %d\n", Scen ? Scen->ScenarioName : "?", count);
	} else {
		for (int i = 0; i < rb_seen_count; ++i) {
			int k = 0;
			while (k < count && !(now[k].trigger == rb_seen[i].trigger && now[k].type == rb_seen[i].type)) ++k;
			if (k == count) {
				fprintf(stderr, "harness-trigger %d gone %s %s\n", frame, RB_Trigger_Id(rb_seen[i].type), RB_Trigger_Name(rb_seen[i].type));
			} else if (now[k].tripped != rb_seen[i].tripped) {
				fprintf(stderr, "harness-trigger %d tripped %s %x\n", frame, RB_Trigger_Id(now[k].type), (unsigned)now[k].tripped);
			}
		}
		for (int k = 0; k < count; ++k) {
			int i = 0;
			while (i < rb_seen_count && !(now[k].trigger == rb_seen[i].trigger && now[k].type == rb_seen[i].type)) ++i;
			if (i == rb_seen_count) fprintf(stderr, "harness-trigger %d new %s %s\n", frame, RB_Trigger_Id(now[k].type), RB_Trigger_Name(now[k].type));
		}
	}
	memcpy(rb_seen, now, sizeof(RB_Seen) * count);
	rb_seen_count = count;
	if (PlayerPtr != NULL) {
		if (PlayerPtr->IsToWin != rb_house_win) fprintf(stderr, "harness-house %d win h%d\n", frame, (int)PlayerPtr->HeapID);
		if (PlayerPtr->IsToLose != rb_house_lose) fprintf(stderr, "harness-house %d lose h%d\n", frame, (int)PlayerPtr->HeapID);
		if (PlayerPtr->IsDefeated != rb_house_defeated) fprintf(stderr, "harness-house %d defeated h%d\n", frame, (int)PlayerPtr->HeapID);
		rb_house_win = PlayerPtr->IsToWin;
		rb_house_lose = PlayerPtr->IsToLose;
		rb_house_defeated = PlayerPtr->IsDefeated;
	}
}

void RB_Run_Order(char const * text)
{
	char word[6][32];
	int words = 0;
	for (char const * p = text; *p && words < 6;) {
		while (*p == ' ') ++p;
		if (!*p) break;
		int n = 0;
		while (*p && *p != ' ') { if (n < 31) word[words][n++] = *p; ++p; }
		word[words++][n] = 0;
	}
	if (words < 1) return;
	if (!strcmp(word[0], "spawn") && words >= 2) {
		bool enemy = words >= 3 && !strcmp(word[2], "enemy");
		int first = enemy ? 3 : 2;
		bool offset = words >= first + 2;
		RB_Spawn(word[1], enemy, offset ? atoi(word[first]) : 0, offset ? atoi(word[first + 1]) : 0, offset);
	} else if (!strcmp(word[0], "select") && words >= 2) {
		TechnoClass * techno = RB_First_Of(word[1], PlayerPtr);
		bool selected = techno != NULL && techno->Select();
		fprintf(stderr, "harness-select %s %s\n", word[1], selected ? "selected" : "not selected");
	} else if (!strcmp(word[0], "fire") && words >= 3) {
		TechnoClass * unit = RB_First_Of(word[1], PlayerPtr);
		TechnoClass * target = NULL;
		int best = 0;
		for (int i = 0; i < Technos.Count() && unit != NULL; ++i) {
			TechnoClass * techno = Technos[i];
			if (techno == NULL || techno->IsInLimbo || techno->House == PlayerPtr || strcmp(RB_Type_Name(techno), word[2])) continue;
			Coord a = techno->Center_Coord(), b = unit->Center_Coord();
			int d = abs(a.X - b.X) + abs(a.Y - b.Y);
			if (target == NULL || d < best) target = techno, best = d;
		}
		if (unit == NULL || target == NULL) {
			fprintf(stderr, "harness-fire: no %s or no target %s\n", word[1], word[2]);
			return;
		}
		TargetClass tarcom(target);
		TargetClass none;
		Queue_Mission(TargetClass(unit), MISSION_ATTACK, tarcom, none);
		rb_watched = target;
		rb_killed = false;
		snprintf(rb_watched_type, sizeof(rb_watched_type), "%s", word[2]);
		fprintf(stderr, "harness-fire %s -> %s s%d\n", word[1], word[2], target->Strength);
	} else if (!strcmp(word[0], "health")) {
		for (int i = 0; i < Technos.Count(); ++i) {
			TechnoClass * techno = Technos[i];
			if (techno && techno->House == PlayerPtr) fprintf(stderr, "harness-health %s s%d\n", RB_Type_Name(techno), techno->Strength);
		}
	} else if (!strcmp(word[0], "draws")) {
		fprintf(stderr, "harness-draws ellipse %d aaline %d lasers %d\n", RB_Draw_Ellipse_Count, RB_Draw_AA_Line_Count, rb_lasers_seen);
	} else if (!strcmp(word[0], "train") && words >= 2) {
		bool queued = false;
		for (int i = 0; PlayerPtr != NULL && !queued && i < InfantryTypes.Count(); ++i) {
			if (!strcmp(InfantryTypes[i]->Name(), word[1])) {
				OutList.Add(EventClass(PlayerPtr->HeapID, EventClass::PRODUCE, RTTI_INFANTRYTYPE, i, FALSE));   // lane P8: the naval flag given (the 4-argument form leaves it uninitialised)
				queued = true;
			}
		}
		for (int k = 0; PlayerPtr != NULL && !queued && k < UnitTypes.Count(); ++k) {   // k: the units are compiled with VC6's for scope
			if (!strcmp(UnitTypes[k]->Name(), word[1])) {
				OutList.Add(EventClass(PlayerPtr->HeapID, EventClass::PRODUCE, RTTI_UNITTYPE, k, FALSE));
				queued = true;
			}
		}
		fprintf(stderr, queued ? "harness-train %s\n" : "harness-train: no type %s\n", word[1]);
	} else if (!strcmp(word[0], "count") && words >= 2) {
		int n = 0;
		for (int i = 0; i < Technos.Count(); ++i) {
			TechnoClass * techno = Technos[i];
			if (techno && techno->House == PlayerPtr && !techno->IsInLimbo && !strcmp(RB_Type_Name(techno), word[1])) ++n;
		}
		fprintf(stderr, "harness-count %s %d\n", word[1], n);
	} else if (!strcmp(word[0], "supers")) {
		fprintf(stderr, "harness-supers");
		for (int i = 0; PlayerPtr != NULL && i < PlayerPtr->SuperWeapon.Count(); ++i) {
			SuperClass * super = PlayerPtr->SuperWeapon[i];
			fprintf(stderr, " %d:%s", i, super && super->Class ? super->Class->Name() : "?");
		}
		fprintf(stderr, "\n");
	} else if (!strcmp(word[0], "super") && words >= 3 && !strcmp(word[2], "enemy")) {
		int index = atoi(word[1]);
		HouseClass * enemy = RB_Enemy_House();
		if (PlayerPtr == NULL || enemy == NULL || index < 0 || index >= PlayerPtr->SuperWeapon.Count() || enemy->Buildings.Count() == 0) {
			fprintf(stderr, "harness-super: no super weapon %d or no enemy building\n", index);
			return;
		}
		SuperClass * super = PlayerPtr->SuperWeapon[index];
		super->Enable(true, false, false);
		super->Forced_Charge(true);
		Cell at = enemy->Buildings[0]->Center_Coord().As_Cell();
		OutList.Add(EventClass(PlayerPtr->HeapID, EventClass::SPECIAL_PLACE, index, at));
		rb_watched = enemy->Buildings[0];
		rb_killed = false;
		snprintf(rb_watched_type, sizeof(rb_watched_type), "%s", RB_Type_Name(rb_watched));
		fprintf(stderr, "harness-super %d %s @%d,%d -> %s s%d\n", index, super->Class ? super->Class->Name() : "?", (int)at.X, (int)at.Y,
			rb_watched_type, rb_watched->Strength);
	} else if (!strcmp(word[0], "super") && words >= 3 && !strcmp(word[2], "home")) {
		// Lane P8: main's `super <index> home [<dx> <dy>]` (lane C17, the HD air-base check's paradrops): the player's
		// SuperWeapon[index] granted, charged and placed at the player's first techno's cell moved by dx, dy.
		// Lane P14: as main's, the player's first building, else first vehicle (the differential gate runs both ports on
		// one script, so the cell must be the same).
		int index = atoi(word[1]);
		TechnoClass * home = NULL;
		for (int i = 0; i < Technos.Count() && home == NULL; ++i) {
			if (Technos[i] && Technos[i]->House == PlayerPtr && Technos[i]->Fetch_RTTI() == RTTI_BUILDING) home = Technos[i];
		}
		for (int k = 0; k < Technos.Count() && home == NULL; ++k) {   // k: the units are compiled with VC6's for scope
			if (Technos[k] && Technos[k]->House == PlayerPtr && Technos[k]->Fetch_RTTI() == RTTI_UNIT) home = Technos[k];
		}
		if (PlayerPtr == NULL || home == NULL || index < 0 || index >= PlayerPtr->SuperWeapon.Count()) {
			fprintf(stderr, "harness-super: no super weapon %d or no home\n", index);
			return;
		}
		SuperClass * super = PlayerPtr->SuperWeapon[index];
		super->Enable(true, false, false);
		super->Forced_Charge(true);
		Cell at = home->Center_Coord().As_Cell();
		if (words >= 5) {
			at.X += atoi(word[3]);
			at.Y += atoi(word[4]);
		}
		OutList.Add(EventClass(PlayerPtr->HeapID, EventClass::SPECIAL_PLACE, index, at));
		fprintf(stderr, "harness-super %d %s @%d,%d home\n", index, super->Class ? super->Class->Name() : "?", (int)at.X, (int)at.Y);
	} else if (!strcmp(word[0], "super") && words >= 4 && word[2][0] >= '0' && word[2][0] <= '9') {
		// Lane P14: main's `super <index> <x> <y>` (Scout1170): the player's SuperWeapon[index] granted once, charged and
		// placed at cell x,y (SPECIAL_PLACE, the armed cameo's map click).
		int index = atoi(word[1]);
		if (PlayerPtr == NULL || index < 0 || index >= PlayerPtr->SuperWeapon.Count()) {
			fprintf(stderr, "harness-super: no super weapon %d\n", index);
			return;
		}
		SuperClass * super = PlayerPtr->SuperWeapon[index];
		super->Enable(true, false, false);
		super->Forced_Charge(true);
		Cell at;
		at.X = (short)atoi(word[2]);
		at.Y = (short)atoi(word[3]);
		OutList.Add(EventClass(PlayerPtr->HeapID, EventClass::SPECIAL_PLACE, index, at));
		fprintf(stderr, "harness-super %d %s @%d,%d\n", index, super->Class ? super->Class->Name() : "?", (int)at.X, (int)at.Y);
	} else if (!strcmp(word[0], "deploy") && words >= 2) {
		// Lane P8: main's `deploy <ID|*>` (lane N25): the player's vehicles of type ID ('*' every one) whose type deploys into
		// a building (the MCV) unload there (MISSION_UNLOAD, the D key's order).
		int count = 0;
		for (int i = 0; i < Technos.Count(); ++i) {
			TechnoClass * techno = Technos[i];
			if (techno == NULL || techno->House != PlayerPtr || techno->Fetch_RTTI() != RTTI_UNIT) continue;
			TechnoTypeClass const * type = techno->Techno_Type_Class();
			if (type == NULL || type->DeploysInto == NULL || (strcmp(word[1], "*") && strcmp(type->Name(), word[1]))) continue;
			TargetClass none;
			TargetClass none2;
			Queue_Mission(TargetClass(techno), MISSION_UNLOAD, none, none2);
			++count;
		}
		fprintf(stderr, "harness-deploy %s %d\n", word[1], count);
	} else if (!strcmp(word[0], "goto") && words >= 4) {
		// Lane P16: main's `goto <ID|*> <x> <y>` (native/link/harness_orders.cpp, Scout1153): the player's infantry and vehicles
		// of type ID ('*' every one) sent to cell x,y (MISSION_MOVE with the cell as the navigation target, the move click's
		// order): `harness-goto <ID> @x,y <n>`.
		Cell to((short)atoi(word[2]), (short)atoi(word[3]));
		int count = 0;
		for (int i = 0; i < Technos.Count(); ++i) {
			TechnoClass * techno = Technos[i];
			if (techno == NULL || techno->House != PlayerPtr || techno->IsInLimbo) continue;
			RTTIType rtti = techno->Fetch_RTTI();
			if (rtti != RTTI_UNIT && rtti != RTTI_INFANTRY) continue;
			if (strcmp(word[1], "*") && strcmp(RB_Type_Name(techno), word[1])) continue;
			TargetClass none;
			TargetClass navcom(to);
			Queue_Mission(TargetClass(techno), MISSION_MOVE, none, navcom);
			++count;
		}
		fprintf(stderr, "harness-goto %s @%d,%d %d\n", word[1], (int)to.X, (int)to.Y, count);
	} else if (!strcmp(word[0], "where") && words >= 2) {
		// Lane P16: `where <ID>`: `harness-where <ID> @x,y s<strength> m<mission>` (the player's first ID), or `gone`.
		TechnoClass * unit = RB_First_Of(word[1], PlayerPtr);
		if (unit == NULL) {
			fprintf(stderr, "harness-where %s gone\n", word[1]);
			return;
		}
		Cell at = unit->Center_Coord().As_Cell();
		fprintf(stderr, "harness-where %s @%d,%d s%d m%d\n", word[1], (int)at.X, (int)at.Y, unit->Strength, (int)unit->CurrentMission);
	} else if (!strcmp(word[0], "hunt")) {
		RB_Hunt();
	} else if (!strcmp(word[0], "state")) {
		int technos = 0;
		for (int i = 0; i < Technos.Count(); ++i) if (Technos[i] && Technos[i]->House == PlayerPtr && !Technos[i]->IsInLimbo) ++technos;
		char timer[16] = "-";
		if (Scen != NULL && Scen->MissionTimer.Is_Active()) snprintf(timer, sizeof(timer), "%d", Scen->MissionTimer.Value());
		fprintf(stderr, "harness-state %s money %ld timer %s technos %d win %d lose %d\n", Scen ? Scen->ScenarioName : "?",
			PlayerPtr ? (long)PlayerPtr->Available_Money() : 0L, timer, technos, PlayerPtr ? (int)PlayerPtr->IsToWin : 0,
			PlayerPtr ? (int)PlayerPtr->IsToLose : 0);
	} else if (!strcmp(word[0], "chat") && words >= 2) {
		static wchar_t to[1] = {0};
		if (!strcmp(word[1], "open")) {
			Session.Messages.Add_Edit(Session.PrefColor, TextPrintType(TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW), to, 0, -1);
		} else {
			Session.Messages.Remove_Edit();
		}
		fprintf(stderr, "harness-chat %s edit %d\n", word[1], Session.Messages.Is_Edit() ? 1 : 0);
	} else if (RB_Run_Input_Order(word[0])) {
	} else {
		fprintf(stderr, "harness: unknown order '%s'\n", text);
	}
}

}

void RB_Harness_Frame(int frame)
{
	if (rb_order_count < 0) RB_Read_Orders();
	static int watch = -1;
	if (watch < 0) watch = getenv("RA2_NATIVE_TRIGGERS") && atoi(getenv("RA2_NATIVE_TRIGGERS")) ? 1 : 0;
	if (watch) RB_Watch_Triggers(frame);
	int lasers = LaserDrawClass::LaserDraws.Count();
	if (lasers > rb_lasers_last) rb_lasers_seen += lasers - rb_lasers_last;
	rb_lasers_last = lasers;
	static int last_game_frame = -1;
	static char scenario[64] = "";
	bool fresh = false;
	if (Scen != NULL) {
		fresh = Frame < last_game_frame;
		for (int k = 0; k < (int)sizeof(scenario) - 1; ++k) {
			if (scenario[k] != Scen->ScenarioName[k]) fresh = true;
			scenario[k] = Scen->ScenarioName[k];
			if (scenario[k] == 0) break;
		}
		last_game_frame = Frame;
	}
	for (int i = 0; i < rb_order_count; ++i) {
		RB_Order & o = rb_orders[i];
		if (o.relative) {
			if (fresh) o.due = o.frame;
			if (o.due < 0 || Frame < o.due) continue;
			o.due = o.every > 0 ? Frame + o.every : -1;
			RB_Run_Order(o.text);
		} else if (o.frame == frame) {
			RB_Run_Order(o.text);
		}
	}
	if (rb_watched != NULL && !rb_killed && frame % 50 == 0) {
		if (!RB_Is_Live(rb_watched) || rb_watched->IsInLimbo || rb_watched->Strength <= 0) {
			rb_killed = true;
			fprintf(stderr, "harness-killed %s at frame %d\n", rb_watched_type, frame);
		} else {
			fprintf(stderr, "harness-watch s%d at frame %d\n", rb_watched->Strength, frame);
		}
	}
}
