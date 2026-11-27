// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P50): the step server for yr-native, Red Alert 2's (ra2/glue/stepserver_native.inc, lane P21 and the
// lanes after it) written against the YR tree's classes (vendor/ra2-decomp's names). Test scaffolding, NOT original
// code. Called once a loop frame from yr_native_main_loop (glue/yr_app_hooks.cpp), after the harness's orders.
//
// The same environment and commands as Red Alert 2's (see that file's comment): RA2_NATIVE_STEP=<socket>,
// RA2_NATIVE_STEP_MISSION, _RECORD, _PRELOAD, _REPLAY, RA2_NATIVE_FRAME_CLOCK=1 ("# frame clock" in the recording),
// RA2_NATIVE_STEP_SLOW; advance, input (Escape refused), look, order (glue/yr_harness.cpp's orders), save, state,
// objects, triggers, cell, terrain, buildable, probe, screenshot, mapfile, release, quit. The client is
// ra2/tools/ra2_step.py --game yr. Differences from Red Alert 2's:
//   - the "@..." points of the input script (Red Alert 2's harness_input_native.inc: "@^ID", "@=X,Y", "@ID[:n][+dx,dy]",
//     "@!ID[:n]", "@tab=N", "@cameo=ID") are answered here (yr_step_resolve_point, asked by glue/yr_app_hooks.cpp's
//     resolver after its own "@cmd=", "@unit=", "@node=", "@thumb"); also "@repair" and "@sell", the sidebar's repair
//     and sell buttons (SidebarClass::Repair, ::Power);
//   - the mouse is processed once a frame as the mouse thread processes it (WWMouseClass::Process_Mouse);
//   - the game's end is logged here ("game-over: player wins|player loses at frame <n>"), as Red Alert 2's Main_Loop seam
//     logs it (yr/tests/yr_campaign.py reads it).
// ---------------------------------------------------------------------------------------------
#include "always.h"
#include "globals.h"
#include "techno.h"
#include "techtype.h"
#include "house.h"
#include "houstype.h"
#include "scenario.h"
#include "trigger.h"
#include "trigtype.h"
#include "rtti.hh"
#include "_tactica.h"
#include "tactical.h"
#include "_map.h"
#include "display.h"
#include "mouse.h"
#include "sidebar.h"
#include "factory.h"
#include "cell.h"
#include "ccfile.h"
#include "builtype.h"
#include "overtype.h"
#include "objtype.h"
#include "super.h"
#include "suprtype.h"
#include "saveload.h"
#include "gadget.h"
#include "gscreen.h"
#include "shapebtn.h"
#include "conquer.h"
#include "wwmous.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

extern Rect TacticalRect;
extern "C" int ra2port_step_listen(const char * path);          // platform/sdl/host.cpp (POSIX; 0 elsewhere)
extern "C" int ra2port_step_getline(char * buffer, int size);   // blocks for the next command line; -1 when no server
extern "C" int ra2port_step_getline_ex(char * buffer, int size, int connected_only);   // -2: no client connected
extern "C" void ra2port_step_reply(const char * text);
extern "C" int ra2port_script_add(int frame, const char * text);
extern "C" int ra2port_screenshot(const char * path);
extern "C" int ra2_native_scroll_map(int, int, int dx, int dy);  // glue/yr_app_hooks.cpp
extern "C" void ra2port_progress(const char * milestone, const char * detail);
extern "C" void ra2port_exit_now(int code);                     // platform/sdl/host.cpp
extern "C" void ra2port_frame_clock(int what);                   // port/src/port_core.cpp (lane P33)
extern "C" uint64_t ra2port_monotonic_ns(void);                  // port/src/win32_time.cpp
extern "C" uint64_t ra2port_game_ns(void);                       // the game's clock (port/src/win32_time.cpp)
extern "C" void ra2port_sleep_until_ns(uint64_t deadline);
extern "C" int ra2port_host_write_file(const char * path, const void * data, int size, int append);   // the host's paths
extern "C" char * ra2port_host_read_file(const char * path, int * size);
extern "C" int yr_harness_run_order(int frame, char const * text);   // glue/yr_harness.cpp: one RA2_NATIVE_ORDERS order
extern WWMouseClass * DAT_00B78164;   // wwmous.cpp: the game's WWMouseClass (the mouse thread's)

// TriggerClass::IsTripped is private (trigger.h): read through a member pointer taken by an explicit instantiation
// (access checks do not apply to its arguments), as Red Alert 2's harness_native.inc reads it.
template<int TriggerClass::* M> struct Yr_Step_Tripped_Access { friend int TriggerClass::* RB_Tripped_Member(void) { return(M); } };
int TriggerClass::* RB_Tripped_Member(void);
template struct Yr_Step_Tripped_Access<&TriggerClass::IsTripped>;

namespace {

char const * RB_Type_Name(TechnoClass * techno)
{
	TechnoTypeClass const * type = techno->Techno_Type_Class();
	return(type != NULL ? type->Name() : "?");
}
char const * RB_Trigger_Id(TriggerTypeClass * type) { return(type != NULL ? type->Name() : "?"); }
char const * RB_Trigger_Name(TriggerTypeClass * type) { return(type != NULL ? (char const *)type->GivenName : "?"); }

bool RB_Is_Selected(ObjectClass * object)
{
	for (int i = 0; i < CurrentObject.Count(); ++i) {
		if (CurrentObject[i] == object) return(true);
	}
	return(false);
}

// A sidebar entry's name: a techno type's, or a super weapon type's.
char const * RB_Buildable_Name(RTTIType rtti, int id)
{
	if (rtti == RTTI_SPECIAL || rtti == RTTI_SUPERWEAPONTYPE) {
		return((unsigned)id < (unsigned)SuperWeaponTypes.Count() && SuperWeaponTypes[id] ? SuperWeaponTypes[id]->Name() : NULL);
	}
	TechnoTypeClass const * type = Fetch_Techno_Type(rtti, id);
	return(type ? type->Name() : NULL);
}

bool RB_Same_Name(char const * a, char const * b, int length)
{
	for (int i = 0; i < length; ++i, ++a, ++b) {
		char x = *a >= 'a' && *a <= 'z' ? *a - 32 : *a;
		char y = *b >= 'a' && *b <= 'z' ? *b - 32 : *b;
		if (x != y) return(false);
		if (x == 0) return(true);
	}
	return(*a == 0);
}

int RB_Read_Int(char const * & p)
{
	int sign = 1, value = 0;
	if (*p == '-') { sign = -1; ++p; } else if (*p == '+') ++p;
	while (*p >= '0' && *p <= '9') value = value * 10 + (*p++ - '0');
	return(sign * value);
}

void RB_Centre(GadgetClass const & gadget, int * x, int * y)
{
	*x = gadget.X + gadget.Width / 2;
	*y = gadget.Y + gadget.Height / 2;
}

}

// "@..." points (see the file comment), in the screen's game pixels; 0: no point (no click).
extern "C" int yr_step_resolve_point(char const * spec, int * x, int * y)
{
	HouseClass * player = PlayerPtr;
	if (player == NULL || TacticalMap == NULL) return(0);
	if (!strncmp(spec, "tab=", 4)) {
		int tab = spec[4] - '0';
		if (tab < 0 || tab > 3) return(0);
		RB_Centre(SidebarClass::TabButtons[tab], x, y);
		return(1);
	}
	if (!strcmp(spec, "repair")) { RB_Centre(SidebarClass::Repair, x, y); return(1); }
	if (!strcmp(spec, "sell")) { RB_Centre(SidebarClass::Power, x, y); return(1); }
	if (!strncmp(spec, "cameo=", 6)) {
		char const * rest = spec + 6;
		int shown = Map.GetVisibleCameoCount();
		for (int strip = 0; strip < SidebarClass::STRIPS; ++strip) {
			SidebarClass::StripClass & column = Map.Column[strip];
			if (!column.AllowedToDraw) continue;
			for (int i = 0; i < shown && i < 60; ++i) {
				int index = column.TopIndex * 2 + i;
				if (index >= column.BuildableCount) break;
				char const * name = RB_Buildable_Name(column.Buildables[index].BuildableType, column.Buildables[index].BuildableID);
				if (name && RB_Same_Name(name, rest, 64)) {
					RB_Centre(SidebarClass::StripClass::SelectButton[strip * 60 + i], x, y);
					return(1);
				}
			}
		}
		return(0);
	}
	if (spec[0] == '=') {
		char const * p = spec + 1;
		Cell cell;
		cell.X = (short)RB_Read_Int(p);
		if (*p == ',') ++p;
		cell.Y = (short)RB_Read_Int(p);
		Coord coord = Map[cell].Center_Coord();
		Point2D pixel;
		if (!TacticalMap->Coord_To_Pixel(coord, pixel)) return(0);
		*x = pixel.X + TacticalRect.X;
		*y = pixel.Y + TacticalRect.Y;
		return(1);
	}
	if (spec[0] == '^') {   // the object with that ID (the obj lines), where it was drawn
		int id = atoi(spec + 1);
		for (int b = 0; b < Technos.Count(); ++b) {   // a building: the middle of its foundation
			TechnoClass * techno = Technos[b];
			if (techno == NULL || techno->ID != id || techno->IsInLimbo || techno->Fetch_RTTI() != RTTI_BUILDING) continue;
			Point2D pixel;
			TacticalMap->Coord_To_Pixel(techno->Center_Coord(), pixel);
			if (pixel.X < 0 || pixel.Y < 0 || pixel.X >= TacticalRect.Width || pixel.Y >= TacticalRect.Height) return(0);
			*x = pixel.X + TacticalRect.X;
			*y = pixel.Y + TacticalRect.Y;
			return(1);
		}
		for (int k = 0; k < TacticalMap->SelectableCount; ++k) {
			ObjectClass * object = Tactical::SelectableObjects[k].Object;
			if (object == NULL || object->ID != id || !object->IsActive) continue;
			Point2D pixel = Tactical::SelectableObjects[k].Position - Point2D(TacticalMap->TacPixelX, TacticalMap->TacPixelY);
			if (pixel.X < 0 || pixel.Y < 0 || pixel.X >= TacticalRect.Width || pixel.Y >= TacticalRect.Height) return(0);
			*x = pixel.X + TacticalRect.X;
			*y = pixel.Y + TacticalRect.Y;
			return(1);
		}
		return(0);
	}
	bool other = spec[0] == '!';
	if (other) ++spec;
	int length = 0;
	while (spec[length] && spec[length] != ':' && spec[length] != '+' && spec[length] != '-') ++length;
	char const * p = spec + length;
	int index = 0, dx = 0, dy = 0;
	if (*p == ':') { ++p; index = RB_Read_Int(p); }
	if (*p == '+' || *p == '-') { dx = RB_Read_Int(p); if (*p == ',') { ++p; dy = RB_Read_Int(p); } }
	for (int pass = other ? 2 : 0; pass < (other ? 3 : 2); ++pass) {
		for (int k = 0; k < Technos.Count(); ++k) {
			TechnoClass * techno = Technos[other ? Technos.Count() - 1 - k : k];
			if (techno == NULL || techno->House == NULL || !RB_Same_Name(RB_Type_Name(techno), spec, length)) continue;
			if (pass == 2 ? techno->House == player || techno->House->Is_Player_Control()
				: pass == 0 ? techno->House != player : techno->House == player || !techno->House->Is_Player_Control()) continue;
			if (index-- > 0) continue;
			Coord coord = techno->Center_Coord();
			coord.X += dx * 256;
			coord.Y += dy * 256;
			Point2D pixel;
			TacticalMap->Coord_To_Pixel(coord, pixel);
			*x = pixel.X + TacticalRect.X;
			*y = pixel.Y + TacticalRect.Y;
			return(1);
		}
	}
	return(0);
}

namespace {

enum { RB_STEP_OFF, RB_STEP_LIVE, RB_STEP_REPLAY };
int rb_step_mode = -1;
int rb_step_budget = 0;          // loop frames left of the last advance
bool rb_step_reply_due = false;  // an advance's reply waits for its frames
char const * rb_step_record = NULL;   // RA2_NATIVE_STEP_RECORD's path
struct RB_Step_Line { int frame; char text[200]; };
RB_Step_Line * rb_step_replay = NULL;   // the replay's lines (RA2_NATIVE_STEP_REPLAY, or live: RA2_NATIVE_STEP_PRELOAD)
int rb_step_replay_count = 0, rb_step_replay_next = 0;
int rb_step_host_offset = 0;     // the served scenario's first loop frame: tick + offset is the host's loop frame
char rb_step_news[16384];
int rb_step_news_length = 0;

void RB_Step_Out(char const * format, ...)
{
	char line[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(line, sizeof(line), format, args);
	va_end(args);
	ra2port_step_reply(line);
}

void RB_Step_News(char const * format, ...)
{
	if (rb_step_news_length > (int)sizeof(rb_step_news) - 300) return;
	va_list args;
	va_start(args, format);
	rb_step_news_length += vsnprintf(rb_step_news + rb_step_news_length, sizeof(rb_step_news) - rb_step_news_length, format, args);
	va_end(args);
	rb_step_news[rb_step_news_length++] = '\n';
	rb_step_news[rb_step_news_length] = 0;
}

char const * RB_House_Name(HouseClass const * house)
{
	return(house != NULL ? (char const *)house->IniName : "-");
}

bool RB_Seen_By_Player(ObjectClass * object)
{
	if (object == NULL) return(false);
	Cell cell = object->Center_Coord().As_Cell();
	return(Map[cell].IsMapped != 0);
}

/*
 * The news: what changed since the last reply (the trigger watch's diff, kept here as lines for the client, and
 * technos lost or gained by each side).
 */
struct RB_Step_Trigger { TriggerClass * trigger; TriggerTypeClass * type; int tripped; };
RB_Step_Trigger rb_step_triggers[512];
int rb_step_trigger_count = -1;
struct RB_Step_Techno { TechnoClass * techno; char type[24]; char house[24]; Cell cell; };
RB_Step_Techno rb_step_technos[2048];
int rb_step_techno_count = -1;

void RB_Step_Watch(int frame)
{
	static RB_Step_Trigger now[512];
	int count = 0;
	for (int i = 0; i < Triggers.Count() && count < 512; ++i) {
		TriggerClass * trigger = Triggers[i];
		if (trigger == NULL) continue;
		now[count].trigger = trigger;
		now[count].type = trigger->Class;
		now[count].tripped = trigger->*RB_Tripped_Member();
		++count;
	}
	if (rb_step_trigger_count >= 0) {
		for (int i = 0; i < rb_step_trigger_count; ++i) {
			int k = 0;
			while (k < count && !(now[k].trigger == rb_step_triggers[i].trigger && now[k].type == rb_step_triggers[i].type)) ++k;
			if (k == count) RB_Step_News("news %d trigger-gone %s %s", frame, RB_Trigger_Id(rb_step_triggers[i].type), RB_Trigger_Name(rb_step_triggers[i].type));
		}
		for (int k = 0; k < count; ++k) {
			int i = 0;
			while (i < rb_step_trigger_count && !(now[k].trigger == rb_step_triggers[i].trigger && now[k].type == rb_step_triggers[i].type)) ++i;
			if (i == rb_step_trigger_count) RB_Step_News("news %d trigger-new %s %s", frame, RB_Trigger_Id(now[k].type), RB_Trigger_Name(now[k].type));
		}
	}
	memcpy(rb_step_triggers, now, sizeof(RB_Step_Trigger) * count);
	rb_step_trigger_count = count;

	static RB_Step_Techno seen[2048];
	int n = 0;
	for (int t = 0; t < Technos.Count() && n < 2048; ++t) {
		TechnoClass * techno = Technos[t];
		if (techno == NULL || techno->IsInLimbo) continue;
		seen[n].techno = techno;
		snprintf(seen[n].type, sizeof(seen[n].type), "%s", RB_Type_Name(techno));
		snprintf(seen[n].house, sizeof(seen[n].house), "%s", RB_House_Name(techno->House));
		seen[n].cell = techno->Center_Coord().As_Cell();
		++n;
	}
	if (rb_step_techno_count >= 0) {
		for (int i = 0; i < rb_step_techno_count; ++i) {
			int k = 0;
			while (k < n && seen[k].techno != rb_step_technos[i].techno) ++k;
			if (k == n) RB_Step_News("news %d lost %s %s @%d,%d", frame, rb_step_technos[i].house, rb_step_technos[i].type,
				(int)rb_step_technos[i].cell.X, (int)rb_step_technos[i].cell.Y);
		}
		for (int k = 0; k < n; ++k) {
			int i = 0;
			while (i < rb_step_techno_count && rb_step_technos[i].techno != seen[k].techno) ++i;
			if (i == rb_step_techno_count) RB_Step_News("news %d appeared %s %s @%d,%d", frame, seen[k].house, seen[k].type,
				(int)seen[k].cell.X, (int)seen[k].cell.Y);
		}
	}
	memcpy(rb_step_technos, seen, sizeof(RB_Step_Techno) * n);
	rb_step_techno_count = n;
}

/*
 * The state report.
 */
void RB_Step_Objects(char const * which)
{
	bool mine = !strcmp(which, "mine"), enemy = !strcmp(which, "enemy");
	for (int i = 0; i < Technos.Count(); ++i) {
		TechnoClass * techno = Technos[i];
		if (techno == NULL || techno->IsInLimbo) continue;
		bool own = techno->House == PlayerPtr;
		bool allied = own || (PlayerPtr && techno->House && PlayerPtr->Is_Ally(techno->House));
		if (mine && !own) continue;
		if (enemy && allied) continue;
		TechnoTypeClass const * type = techno->Techno_Type_Class();
		Cell cell = techno->Center_Coord().As_Cell();
		char const * kind = "?";
		switch (techno->Fetch_RTTI()) {
			case RTTI_BUILDING: kind = "bld"; break;
			case RTTI_UNIT: kind = "veh"; break;
			case RTTI_INFANTRY: kind = "inf"; break;
			case RTTI_AIRCRAFT: kind = "air"; break;
			default: break;
		}
		RB_Step_Out("obj %d %s %s %s @%d,%d s%d/%d m%d%s%s%s", techno->ID, kind, RB_Type_Name(techno), RB_House_Name(techno->House),
			(int)cell.X, (int)cell.Y, techno->Strength, type ? type->MaxStrength : 0, (int)techno->CurrentMission,
			RB_Is_Selected(techno) ? " sel" : "", own ? " own" : allied ? " ally" : "", RB_Seen_By_Player(techno) ? " seen" : "");
	}
}

void RB_Step_Triggers(void)
{
	for (int i = 0; i < Triggers.Count(); ++i) {
		TriggerClass * trigger = Triggers[i];
		if (trigger == NULL) continue;
		RB_Step_Out("trig %s \"%s\" %s tripped %x", RB_Trigger_Id(trigger->Class), RB_Trigger_Name(trigger->Class),
			trigger->Is_Enabled() ? "on" : "off", (unsigned)(trigger->*RB_Tripped_Member()));
	}
}

void RB_Step_Factory(char const * label, RTTIType kind, bool naval = false, int category = 0)
{
	FactoryClass * factory = PlayerPtr ? PlayerPtr->Fetch_Factory(kind, naval, category) : NULL;
	if (factory == NULL) return;
	TechnoClass * object = factory->Get_Object();
	RB_Step_Out("factory %s %s %d%%%s", label, object ? RB_Type_Name(object) : "-", factory->Completion() * 100 / 54,
		factory->Is_Suspended() ? " on-hold" : "");
}

void RB_Step_State(int frame)
{
	// clock: the game's clock in SystemTimerClass ticks (16 ms; under the frame clock one a loop frame, lane P33)
	RB_Step_Out("scenario %s loop %d game %d over %d win %d lose %d clock %u", Scen ? (char const *)Scen->ScenarioName : "?", frame,
		Frame, PlayerWins || PlayerLoses ? 1 : 0, PlayerWins ? 1 : 0, PlayerLoses ? 1 : 0, (unsigned)(ra2port_game_ns() / 16000000u));
	if (Scen != NULL && Scen->MissionTimer.Is_Active()) RB_Step_Out("timer %ld frames", (long)Scen->MissionTimer.Value());
	if (PlayerPtr != NULL) {
		RB_Step_Out("player %s credits %ld power %d drain %d input %s", RB_House_Name(PlayerPtr), (long)PlayerPtr->Available_Money(),
			PlayerPtr->Power, PlayerPtr->Drain, IgnoreInput ? "locked" : "open");
	}
	for (int h = 0; h < Houses.Count(); ++h) {
		HouseClass * house = Houses[h];
		if (house == NULL) continue;
		// lane P31: each house's power, drain and credits (Allied 11: whether the enemy base is on low power, can repair)
		RB_Step_Out("house h%d %s %s%s%s buildings %d power %d drain %d credits %ld", (int)house->HeapID, RB_House_Name(house),
			house == PlayerPtr ? "player" : (PlayerPtr && PlayerPtr->Is_Ally(house)) ? "ally" : "enemy",
			house->IsDefeated ? " defeated" : "", house->IsToWin ? " to-win" : house->IsToLose ? " to-lose" : "", house->Buildings.Count(),
			house->Power, house->Drain, (long)house->Available_Money());
	}
	RB_Step_Factory("building", RTTI_BUILDING);
	RB_Step_Factory("infantry", RTTI_INFANTRY);
	RB_Step_Factory("vehicle", RTTI_UNIT);
	RB_Step_Factory("aircraft", RTTI_AIRCRAFT);
	RB_Step_Factory("defense", RTTI_BUILDING, false, 5); // lane P26: the defense queue apart (BuildCat=Combat)
	RB_Step_Factory("ship", RTTI_UNIT, true, 0);           // lane P26: the naval yard's queue apart
	for (int strip = 0; strip < 4; ++strip) {
		SidebarClass::StripClass & column = Map.Column[strip];
		char line[1024];
		int n = snprintf(line, sizeof(line), "sidebar tab %d:", strip);
		for (int i = 0; i < column.BuildableCount && n < (int)sizeof(line) - 40; ++i) {
			char const * name = RB_Buildable_Name(column.Buildables[i].BuildableType, column.Buildables[i].BuildableID);
			if (name) n += snprintf(line + n, sizeof(line) - n, " %s", name);
		}
		RB_Step_Out("%s", line);
	}
	// lane P36: every house's super weapons (Soviet 10's nuke: whether it is charged, the frames left on its timer; Allied
	// 12: when the enemy's nuke will be ready); the player's lines are "super <name> ...", another house's "super h<n> <name> ..."
	for (int h = 0; h < Houses.Count(); ++h) {
		HouseClass * house = Houses[h];
		if (house == NULL) continue;
		for (int i = 0; i < house->SuperWeapon.Count(); ++i) {
			SuperClass * sw = house->SuperWeapon[i];
			if (sw == NULL || sw->Class == NULL || !sw->IsPresent) continue;
			char who[16] = "";
			if (house != PlayerPtr) snprintf(who, sizeof(who), "h%d ", (int)house->HeapID);
			RB_Step_Out("super %s%s ready %d charging %d left %d", who, sw->Class->Name(), sw->IsReady ? 1 : 0,
				!sw->IsReady && !sw->IsSuspended ? 1 : 0   /* SuperClass::Is_Charging is not ported in the YR tree */, sw->Control.Value());
		}
	}
	Point2D view = TacticalMap ? TacticalMap->Get_Tactical_Position() : Point2D(0, 0);
	Cell centre(0, 0);
	if (TacticalMap) centre = TacticalMap->Pixel_To_Cell(Point2D(TacticalRect.Width / 2, TacticalRect.Height / 2));
	RB_Step_Out("view %d,%d centre @%d,%d tactical %dx%d selected %d", view.X, view.Y, (int)centre.X, (int)centre.Y, TacticalRect.Width,
		TacticalRect.Height, CurrentObject.Count());
	RB_Step_Objects("all");
	RB_Step_Triggers();
}

void RB_Step_Cells(int x, int y, int r)
{
	for (int cy = y - r; cy <= y + r; ++cy) {
		char line[1024];
		int n = snprintf(line, sizeof(line), "row %d:", cy);
		for (int cx = x - r; cx <= x + r && n < (int)sizeof(line) - 40; ++cx) {
			Cell cell((short)cx, (short)cy);
			if (!Map.In_Radar(cell)) { n += snprintf(line + n, sizeof(line) - n, " ----"); continue; }
			CellClass & c = Map[cell];
			ObjectClass * who = c.Cell_Occupier();
			// lane P30: an overlay by name after a '+' (a wall's land type stays clear, as in the original: OverlayTypeClass
			// reads no Land= for walls, so only the overlay shows one)
			char ov[40] = "";
			if (c.Overlay != OVERLAY_NONE && (unsigned)c.Overlay < (unsigned)OverlayTypes.Count() && OverlayTypes[c.Overlay]) {
				snprintf(ov, sizeof(ov), "+%s", OverlayTypes[c.Overlay]->Name());
			}
			n += snprintf(line + n, sizeof(line) - n, " %c%d%c%s%s", c.Is_Tile_Water() ? 'w' : c.Is_Tile_Cliff() ? 'c' : '.', (int)c.Height,
				c.IsMapped ? 'm' : '?', who ? (who->Owner_HouseClass() == PlayerPtr ? "P" : "o") : "", ov);
		}
		RB_Step_Out("%s", line);
	}
}

void RB_Step_Look(int x, int y)
{
	// TacticalCoord is the view's centre in absolute pixels (tactical.h): the pan that brings the cell's centre there.
	if (TacticalMap == NULL) return;
	Point2D want = TacticalMap->Coord_To_Pixel_Absolute(Map[Cell((short)x, (short)y)].Center_Coord());
	Point2D now = TacticalMap->Get_Tactical_Position();
	ra2_native_scroll_map(0, 0, want.X - now.X, want.Y - now.Y);
}

void RB_Step_Mapfile(char const * rest)
{
	if (Scen == NULL) { RB_Step_Out("error no scenario"); return; }
	// lane P26: "mapfile <out> [<name>]": another of the game's data files by name (RULES.INI: ranges, speeds, timers
	// for planning), read the same way; without a name, the scenario's own file.
	char out[512];
	char name[64];
	name[0] = '\0';
	if (sscanf(rest, "%511s %63s", out, name) < 1) { RB_Step_Out("error mapfile <out> [<name>]"); return; }
	char const * want = name[0] ? name : (char const *)Scen->ScenarioName;
	CCFileClass file(want);
	if (!file.Is_Available()) { RB_Step_Out("error %s not found", want); return; }
	int size = file.Size();
	char * data = new char[size > 0 ? size : 1];
	file.Open(1);
	int got = file.Read(data, size);
	file.Close();
	bool wrote = ra2port_host_write_file(out, data, got, 0) != 0;
	delete [] data;
	RB_Step_Out("mapfile %s %d bytes -> %s", want, got, wrote ? out : "(not written)");
}

void RB_Step_Record(int frame, char const * text)
{
	if (rb_step_record == NULL) return;
	char line[400];
	int n = snprintf(line, sizeof(line), "%d %s\n", frame, text);
	ra2port_host_write_file(rb_step_record, line, n, 1);
}

// The pointer parked in the middle of the tactical view after a click, as a player's hand leaves the map's edge: a
// pointer left at the screen's edge edge-scrolls the map every frame.
int RB_Step_Park(int at)
{
	char line[64];
	snprintf(line, sizeof(line), "move %d %d", TacticalRect.X + TacticalRect.Width / 2, TacticalRect.Y + TacticalRect.Height / 2);
	return(ra2port_script_add(at, line));
}

// "x y" or "x,y" (and an optional third number): the numbers read, advancing rest past them.
int RB_Step_Numbers(char const * & rest, int * out, int most)
{
	int n = 0;
	while (n < most) {
		while (*rest == ' ' || *rest == ',') ++rest;
		if (!((*rest >= '0' && *rest <= '9') || *rest == '-')) break;
		out[n++] = atoi(rest);
		if (*rest == '-') ++rest;
		while (*rest >= '0' && *rest <= '9') ++rest;
	}
	return(n);
}

// One command; true when it changes the game (recorded). Replies go to the client (live) or the log (replay).
bool RB_Step_Command(int frame, char const * text, bool live)
{
	while (*text == ' ') ++text;
	char const * rest = text;
	while (*rest && *rest != ' ') ++rest;
	int length = (int)(rest - text);
	while (*rest == ' ') ++rest;
	#define RB_IS(word) (length == (int)strlen(word) && !strncmp(text, word, length))
	if (RB_IS("advance")) {
		rb_step_budget = atoi(rest) > 0 ? atoi(rest) : 1;
		rb_step_reply_due = live;
		return(false);
	}
	if (RB_IS("input")) {
		int delay = 0;
		if (*rest == '+') { delay = atoi(rest + 1); while (*rest && *rest != ' ') ++rest; while (*rest == ' ') ++rest; }
		// lane P22: Escape opens the options dialog, whose modal loop runs no Main_Loop frame, so the server (which
		// answers between loop frames) would never answer again: refused, not queued or recorded.
		if (!strncmp(rest, "key", 3)) {
			char const * name = rest;
			while (*name && *name != ' ') ++name;
			while (*name == ' ') ++name;
			if (!strncasecmp(name, "Escape", 6)) {
				if (live) RB_Step_Out("error key Escape refused: it opens a modal dialog the step server cannot serve through");
				return(false);
			}
		}
		// click / rclick P: the pointer moved there, the button pressed two frames later and released four after that
		// (a real click holds the button for several frames; the script's one-frame click was read a click late here).
		int queued = 0;
		int at = rb_step_host_offset + frame + 1 + delay;
		bool left = !strncmp(rest, "click ", 6), right = !strncmp(rest, "rclick ", 7);
		if (left || right) {
			char const * point = rest + (left ? 6 : 7);
			char line[240];
			snprintf(line, sizeof(line), "move %s", point);
			queued += ra2port_script_add(at, line);
			snprintf(line, sizeof(line), "%s %s", left ? "ldown" : "rdown", point);
			queued += ra2port_script_add(at + 2, line);
			snprintf(line, sizeof(line), "%s %s", left ? "lup" : "rup", point);
			queued += ra2port_script_add(at + 6, line);
			queued += RB_Step_Park(at + 20);
		} else if (!strncmp(rest, "drag ", 5)) {
			// drag P0 P1: a band box held the same way (the pointer moves to P1 while the button is down)
			char a[100] = "", b[100] = "";
			char const * p = rest + 5;
			int n = 0;
			for (int k = 0; k < 2; ++k) {     // "X Y" (two fields) or "@spec" (one)
				char * out = k ? b : a;
				while (*p == ' ') ++p;
				int fields = *p == '@' ? 1 : 2;
				n = 0;
				for (int f = 0; f < fields; ++f) {
					while (*p == ' ') ++p;
					if (f) out[n++] = ' ';
					while (*p && *p != ' ' && n < 98) out[n++] = *p++;
				}
				out[n] = 0;
			}
			char line[240];
			snprintf(line, sizeof(line), "move %s", a);
			queued += ra2port_script_add(at, line);
			snprintf(line, sizeof(line), "ldown %s", a);
			queued += ra2port_script_add(at + 2, line);
			snprintf(line, sizeof(line), "move %s", b);
			queued += ra2port_script_add(at + 5, line);
			snprintf(line, sizeof(line), "lup %s", b);
			queued += ra2port_script_add(at + 9, line);
			queued += RB_Step_Park(at + 24);
		} else {
			queued = ra2port_script_add(at, rest);
		}
		if (live) RB_Step_Out("input %d events from tick %d", queued, frame + 1 + delay);
		if (!live) fprintf(stderr, "step-replay %d input %s (%d events)\n", frame, rest, queued);
		return(queued > 0);
	}
	if (RB_IS("look")) {
		int xy[2] = {0, 0};
		if (RB_Step_Numbers(rest, xy, 2) < 2) { if (live) RB_Step_Out("error look <x> <y>"); return(false); }
		RB_Step_Look(xy[0], xy[1]);
		if (live && TacticalMap) {
			Point2D at = TacticalMap->Get_Tactical_Position();
			RB_Step_Out("look @%d,%d: view %d,%d (clamped at the map's edges; points resolve from the next frame)", xy[0], xy[1], at.X, at.Y);
		}
		return(true);
	}
	if (RB_IS("order")) {
		yr_harness_run_order(frame, rest);
		if (live) RB_Step_Out("order run (its harness- line is in the game's log)");
		if (!live) fprintf(stderr, "step-replay %d order %s\n", frame, rest);
		return(true);
	}
	if (RB_IS("save")) {
		bool saved = Save_Game(rest, L"Step server save", false);
		char detail[256];
		snprintf(detail, sizeof(detail), "frame %d: %s %s", frame, saved ? "saved" : "save failed", rest);
		ra2port_progress("step-save", detail);
		fprintf(stderr, "step-save %s\n", detail);
		if (live) RB_Step_Out("save %s", detail);
		return(true);
	}
	if (!live) {
		fprintf(stderr, "step-replay %d: not a recorded command: %s\n", frame, text);
		return(false);
	}
	if (RB_IS("state")) {
		RB_Step_State(frame);
	} else if (RB_IS("objects")) {
		RB_Step_Objects(*rest ? rest : "all");
	} else if (RB_IS("triggers")) {
		RB_Step_Triggers();
	} else if (RB_IS("cell")) {
		int v[3] = {0, 0, 3};
		RB_Step_Numbers(rest, v, 3);
		RB_Step_Cells(v[0], v[1], v[2] > 12 ? 12 : v[2]);
	} else if (RB_IS("terrain")) {
		// terrain <x0> <y0> <x1> <y1>: one character a cell: the land type (. clear, r road, ~ water, # rock, W wall (a wall overlay too),
		// t ore, b beach, , rough, i ice, = rail, T tunnel, v weeds), a bridge deck '%', a building 'B' (the player's 'P'
		// for any of its technos, 'e' an enemy's, 'a' another's), unmapped cells in lower case shadow ' '.
		int v[4] = {0, 0, 0, 0};
		RB_Step_Numbers(rest, v, 4);
		for (int cy = v[1]; cy <= v[3]; ++cy) {
			char row[300];
			int n = snprintf(row, sizeof(row), "t%3d ", cy);
			for (int cx = v[0]; cx <= v[2] && n < (int)sizeof(row) - 2; ++cx) {
				Cell cell((short)cx, (short)cy);
				char ch = ' ';
				if (Map.In_Radar(cell)) {
					CellClass & c = Map[cell];
					int land = (int)c.Land_Type();
					ch = land >= 0 && land < 12 ? ".r~#Wtb,i=Tv"[land] : '?';
					// lane P30: a wall overlay keeps the clear land type (as in the original), so it is shown by its overlay
					if (c.Overlay != OVERLAY_NONE && (unsigned)c.Overlay < (unsigned)OverlayTypes.Count() && OverlayTypes[c.Overlay] && OverlayTypes[c.Overlay]->IsWall) ch = 'W';
					if (c.IsBridgeDeck) ch = '%';
					ObjectClass * who = c.Cell_Occupier();
					if (who != NULL) {
						HouseClass * house = who->Owner_HouseClass();
						if (house == PlayerPtr) ch = 'P';
						else if (house && PlayerPtr && !PlayerPtr->Is_Ally(house)) ch = 'e';
						else if (who->Fetch_RTTI() == RTTI_BUILDING) ch = 'B';
					}
				}
				row[n++] = ch;
			}
			row[n] = 0;
			RB_Step_Out("%s", row);
		}
	} else if (RB_IS("buildable")) {
		// buildable <TYPE> <x> <y> [<r>] (lane P25): for each origin cell within r of x,y, whether the building would place
		// there as the placement cursor judges it (the proximity check, the shroud check, every footprint cell clear to
		// build); "place X Y ok" lines for the legal ones, and with r 0 the reason for each footprint cell that is not.
		char name[32] = {0};
		int v[3] = {0, 0, 0};
		char const * p = rest;
		int k = 0;
		while (*p == ' ') ++p;
		while (*p && *p != ' ' && k < 31) name[k++] = *p++;
		RB_Step_Numbers(p, v, 3);
		ObjectTypeClass const * ot = ObjectTypeClass::From_Name(name);
		if (ot != NULL && ot->RTTI != RTTI_BUILDINGTYPE) {
			// a wall's name is also its overlay's (NAWALL, GAWALL), which the generic lookup finds first (lane P29)
			ot = NULL;
			for (int i = 0; i < BuildingTypes.Count(); ++i) {
				if (BuildingTypes[i] && !stricmp(BuildingTypes[i]->Name(), name)) { ot = BuildingTypes[i]; break; }
			}
		}
		if (ot == NULL || ot->RTTI != RTTI_BUILDINGTYPE || PlayerPtr == NULL) {
			RB_Step_Out("error no building type %s", name);
		} else {
			BuildingTypeClass * bt = (BuildingTypeClass *)ot;
			Cell const * list = bt->Occupy_List(true);
			int r = v[2] > 12 ? 12 : v[2];
			int legal = 0;
			for (int cy = v[1] - r; cy <= v[1] + r; ++cy) {
				for (int cx = v[0] - r; cx <= v[0] + r; ++cx) {
					Cell at((short)cx, (short)cy);
					bool prox = Map.Passes_Proximity_Check(bt, PlayerPtr->HeapID, list, at);
					bool shroud = Map.Passes_Shroud_Check(bt, PlayerPtr->HeapID, list, at);
					bool clear = true;
					for (Cell const * l = list; *l != REFRESH_EOL; ++l) {
						Cell c = at + *l;
						if (!Map.In_Radar(c)) { clear = false; if (r == 0) RB_Step_Out("cell %d,%d off the map", c.X, c.Y); continue; }
						CellClass & cc = Map[c];
						if (!cc.Is_Clear_To_Build(bt->Speed, bt, PlayerPtr)) {
							clear = false;
							if (r == 0) {
								ObjectClass * o = cc.Cell_Object();
								RB_Step_Out("cell %d,%d blocked: object %s overlay %d land %d build %d local %d bridge %d ramp %d flags %x", c.X, c.Y,
									(o && o->Class_Of()) ? o->Class_Of()->Name() : "-", (int)cc.Overlay, (int)cc.Land_Type(), (int)::Ground[cc.Land_Type()].Build,
									(int)Map.In_Local_Radar(c), (int)cc.Is_Bridge_Here(), (int)cc.Ramp, (unsigned)(cc.Flag.Composite & 0x3F));
							}
						}
					}
					if (r == 0) RB_Step_Out("place %d %d proximity %d shroud %d clear %d", cx, cy, (int)prox, (int)shroud, (int)clear);
					else if (prox && shroud && clear) { RB_Step_Out("place %d %d ok", cx, cy); ++legal; }
				}
			}
			if (r > 0) RB_Step_Out("buildable %s %d legal", name, legal);
		}
	} else if (RB_IS("probe")) {
		int xy[2] = {0, 0};
		RB_Step_Numbers(rest, xy, 2);
		Coord c = Map[Cell((short)xy[0], (short)xy[1])].Center_Coord();
		Point2D rel, abs = TacticalMap->Coord_To_Pixel_Absolute(c);
		bool in = TacticalMap->Coord_To_Pixel(c, rel);
		Point2D at = TacticalMap->Get_Tactical_Position();
		RB_Step_Out("probe rel %d,%d%s abs %d,%d view %d,%d tacpixel %d,%d rect %d,%d %dx%d z %d", rel.X, rel.Y, in ? "" : " (off)", abs.X, abs.Y,
			at.X, at.Y, TacticalMap->TacPixelX, TacticalMap->TacPixelY, TacticalRect.X, TacticalRect.Y, TacticalRect.Width, TacticalRect.Height, c.Z);
	} else if (RB_IS("screenshot")) {
		RB_Step_Out("screenshot %s %s", rest, ra2port_screenshot(rest) ? "written" : "not written");
	} else if (RB_IS("mapfile")) {
		RB_Step_Mapfile(rest);
	} else if (RB_IS("release")) {
		rb_step_mode = RB_STEP_OFF;
		RB_Step_Out("released at frame %d", frame);
	} else if (RB_IS("quit")) {
		RB_Step_Out("quit at frame %d", frame);
		ra2port_step_reply(".");
		fprintf(stderr, "step: quit at frame %d\n", frame);
		ra2port_exit_now(0);
	} else {
		RB_Step_Out("error unknown command: %s", text);
	}
	#undef RB_IS
	return(false);
}

void RB_Step_Load_Replay(char const * path)
{
	int size = 0;
	char * all = ra2port_host_read_file(path, &size);
	if (all == NULL) {
		fprintf(stderr, "step-replay: cannot open %s\n", path);
		return;
	}
	int capacity = 256;
	rb_step_replay = (RB_Step_Line *)malloc(sizeof(RB_Step_Line) * capacity);
	for (char * line = all; line && *line;) {
		char * end = strchr(line, '\n');
		if (end) *end = 0;
		char * hash = strchr(line, '#');
		if (hash) *hash = 0;
		char * p = line;
		line = end ? end + 1 : NULL;
		while (*p == ' ' || *p == '\t') ++p;
		if (*p < '0' || *p > '9') continue;
		if (rb_step_replay_count == capacity) {
			capacity *= 2;
			rb_step_replay = (RB_Step_Line *)realloc(rb_step_replay, sizeof(RB_Step_Line) * capacity);
		}
		RB_Step_Line & l = rb_step_replay[rb_step_replay_count++];
		l.frame = atoi(p);
		while (*p >= '0' && *p <= '9') ++p;
		while (*p == ' ') ++p;
		int n = 0;
		while (*p && *p != '\r' && n < (int)sizeof(l.text) - 1) l.text[n++] = *p++;
		while (n > 0 && l.text[n - 1] == ' ') --n;
		l.text[n] = 0;
	}
	free(all);
	fprintf(stderr, "step-replay: %d commands from %s\n", rb_step_replay_count, path);
}
}

extern "C" void yr_step_server_frame(int frame)
{
	static int target = 1, scenarios = 0, base = -1, last_game_frame = -1;
	static char scenario[64] = "";
	if (rb_step_mode < 0) {
		rb_step_mode = RB_STEP_OFF;
		if (char const * replay = getenv("RA2_NATIVE_STEP_REPLAY")) {
			if (*replay) {
				RB_Step_Load_Replay(replay);
				rb_step_mode = RB_STEP_REPLAY;
			}
		} else if (char const * path = getenv("RA2_NATIVE_STEP")) {
			if (*path && ra2port_step_listen(path)) {
				rb_step_mode = RB_STEP_LIVE;
				fprintf(stderr, "step: serving on %s\n", path);
			} else {
				fprintf(stderr, "step: no server (%s)\n", path);
			}
		}
		if (char const * record = getenv("RA2_NATIVE_STEP_RECORD")) {
			if (*record) rb_step_record = record;
		}
		if (char const * mission = getenv("RA2_NATIVE_STEP_MISSION")) target = atoi(mission) > 0 ? atoi(mission) : 1;
		// RA2_NATIVE_STEP_PRELOAD=<replay file> (live): its commands run first, at their ticks (and are recorded again), then
		// the server serves from where they end: a recorded run resumed at any point.
		if (rb_step_mode == RB_STEP_LIVE) {
			if (char const * preload = getenv("RA2_NATIVE_STEP_PRELOAD")) {
				if (*preload) RB_Step_Load_Replay(preload);
			}
		}
	}
	if (rb_step_mode == RB_STEP_OFF || Scen == NULL) return;
	// The n-th scenario of the run (RA2_NATIVE_STEP_MISSION, default the first) is the one served or replayed; ticks count
	// loop frames from its first (tick 0), so a replay does not depend on how the mission was reached.
	bool fresh = Frame < last_game_frame;
	for (int k = 0; k < (int)sizeof(scenario) - 1; ++k) {
		if (scenario[k] != Scen->ScenarioName[k]) fresh = true;
		scenario[k] = Scen->ScenarioName[k];
		if (scenario[k] == 0) break;
	}
	last_game_frame = Frame;
	if (fresh) {
		++scenarios;
		if (scenarios == target) {
			base = frame;
			rb_step_trigger_count = rb_step_techno_count = -1;
			RB_Step_Park(frame + 1);   // the pointer starts in the view's middle, not at the screen's corner
			fprintf(stderr, "step: %s from loop frame %d (scenario %d of the run)\n", scenario, frame, scenarios);
			ra2port_progress("step-mission", scenario);
			char note[160];
			int n = snprintf(note, sizeof(note), "# %s: ticks are loop frames from the scenario's first\n", scenario);
			if (rb_step_record) ra2port_host_write_file(rb_step_record, note, n, 1);
			char const * clock = getenv("RA2_NATIVE_FRAME_CLOCK");
			if (clock && *clock == '1') {
				ra2port_frame_clock(1);
				n = snprintf(note, sizeof(note), "# frame clock: the game's clock advances 16 ms a loop frame\n");
				if (rb_step_record) ra2port_host_write_file(rb_step_record, note, n, 1);
			}
		} else if (scenarios > target) {
			base = -1;
			ra2port_frame_clock(0);
		}
	}
	if (base < 0) return;
	// The game's end, as Red Alert 2's Main_Loop seam logs it (mainloop_native.inc RB_Note_Game_Over; lane P50).
	static bool ended = false;
	if ((PlayerWins || PlayerLoses) && !ended) {
		ended = true;
		char why[96];
		snprintf(why, sizeof(why), "%s at frame %d", PlayerWins ? "player wins" : "player loses", frame);
		ra2port_progress("game-over", why);
		fprintf(stderr, "game-over: %s\n", why);
	}
	if (!(PlayerWins || PlayerLoses)) ra2port_frame_clock(2);   // one loop frame of the frame clock (when it runs)
	else ra2port_frame_clock(0);                                // the score screen runs on the wall clock
	// RA2_NATIVE_STEP_SLOW=<ms> (lane P33, ra2/tests/ra2_frame_clock.py): each loop frame of the served scenario takes that
	// much longer, as on a slower or busier machine; a run's ticks must not depend on it.
	static int slow_ms = -1;
	if (slow_ms < 0) slow_ms = getenv("RA2_NATIVE_STEP_SLOW") ? atoi(getenv("RA2_NATIVE_STEP_SLOW")) : 0;
	if (slow_ms > 0) ra2port_sleep_until_ns(ra2port_monotonic_ns() + (uint64_t)slow_ms * 1000000u);
	// The game's mouse (WWMouseClass: the pointer the tactical view's cursor and click actions read) follows the pointer on
	// the mouse thread, every 16 ms of wall time: at the uncapped loop's speed a replay ran several frames before it moved,
	// while a live session's pauses let it catch up, and a click landed elsewhere. Moved here, once a frame, after the
	// frame's scripted input reached the game, it is where the input put it whatever the machine's speed.
	// (wwmous.cpp FUN_007B84D0 does this only while no mouse thread runs: called directly, under the mouse mutex as the
	// thread's call. Without it a click's press read a pointer the thread had not yet moved: a band box from the parked
	// pointer to the clicked unit selected two tanks in some runs of one replay.)
	if (DAT_00B78164 != NULL) DAT_00B78164->Process_Mouse();
	int tick = frame - base;
	rb_step_host_offset = base;
	// RA2_NATIVE_STEP_TRACE=1 (diagnostics, lane P50): the selection each tick, to find where two runs of a replay part.
	static int trace = -1;
	if (trace < 0) trace = getenv("RA2_NATIVE_STEP_TRACE") ? 1 : 0;
	if (trace) {
		char line[512];
		int n = snprintf(line, sizeof(line), "step-trace %d sel %d:", tick, CurrentObject.Count());
		for (int i = 0; i < CurrentObject.Count() && n < (int)sizeof(line) - 16; ++i) n += snprintf(line + n, sizeof(line) - n, " %d", CurrentObject[i]->ID);
		fprintf(stderr, "%s\n", line);
	}
	RB_Step_Watch(tick);
	if (rb_step_mode == RB_STEP_REPLAY) {
		while (rb_step_replay_next < rb_step_replay_count && rb_step_replay[rb_step_replay_next].frame <= tick) {
			RB_Step_Line & l = rb_step_replay[rb_step_replay_next++];
			if (l.frame < tick) fprintf(stderr, "step-replay: line for tick %d run late at %d\n", l.frame, tick);
			RB_Step_Command(tick, l.text, false);
		}
		rb_step_news_length = 0;
		rb_step_news[0] = 0;
		if (PlayerWins || PlayerLoses) {
			static bool told = false;
			if (!told) fprintf(stderr, "step-replay: game over at tick %d (%s)\n", tick, PlayerWins ? "win" : "lose");
			told = true;
		}
		return;
	}
	bool over = PlayerWins || PlayerLoses;
	if (rb_step_budget > 0) {
		--rb_step_budget;
		if (rb_step_budget > 0 && !over) return;
		rb_step_budget = 0;
	}
	if (rb_step_reply_due) {
		rb_step_reply_due = false;
		RB_Step_Out("tick %d loop %d game %d%s", tick, frame, Frame, over ? (PlayerWins ? " over win" : " over lose") : "");
		if (rb_step_news_length) ra2port_step_reply(rb_step_news);
		rb_step_news_length = 0;
		rb_step_news[0] = 0;
		ra2port_step_reply(".");
	}
	if (over) {
		// The game ends at this frame (Main_Loop's end): the score screen follows outside the loop and runs on.
		static bool told = false;
		if (!told) {
			RB_Step_Record(tick, PlayerWins ? "# game over: won" : "# game over: lost");
			fprintf(stderr, "step: game over at tick %d (%s)\n", tick, PlayerWins ? "win" : "lose");
			// The connected client's remaining commands (state, save, screenshot) are answered; an advance answers at once.
			char line[320];
			for (;;) {
				int got = ra2port_step_getline_ex(line, sizeof(line), 1);
				if (got < 0) break;
				if (got == 0) continue;
				if (!strncmp(line, "advance", 7)) {
					RB_Step_Out("tick %d loop %d game %d over %s", tick, frame, Frame, PlayerWins ? "win" : "lose");
				} else {
					RB_Step_Command(tick, line, true);
				}
				rb_step_budget = 0;
				rb_step_reply_due = false;
				ra2port_step_reply(".");
			}
		}
		told = true;
		return;
	}
	if (rb_step_replay_next < rb_step_replay_count) {
		while (rb_step_replay_next < rb_step_replay_count && rb_step_replay[rb_step_replay_next].frame <= tick) {
			RB_Step_Line & l = rb_step_replay[rb_step_replay_next++];
			if (RB_Step_Command(tick, l.text, false)) RB_Step_Record(tick, l.text);
		}
		if (rb_step_replay_next < rb_step_replay_count) {
			rb_step_budget = rb_step_replay[rb_step_replay_next].frame - tick;
			return;
		}
		fprintf(stderr, "step: preload done at tick %d\n", tick);
	}
	char line[320];
	while (rb_step_mode == RB_STEP_LIVE && rb_step_budget == 0) {
		int got = ra2port_step_getline(line, sizeof(line));
		if (got < 0) { rb_step_mode = RB_STEP_OFF; break; }
		if (got == 0) continue;
		// The wait just ended is not a frame's work either, and the command may read the game's clock (state's clock
		// line): without this, a wait of over five seconds before it let the frame clock slip by the rest (lane P34).
		ra2port_frame_clock(3);
		if (RB_Step_Command(tick, line, true)) RB_Step_Record(tick, line);
		if (rb_step_budget == 0) ra2port_step_reply(".");
		ra2port_frame_clock(3);   // the wait for the client's command was not a frame's work
	}
}
