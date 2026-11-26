// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P21): appended to the generated conquer unit after harness_input_native.inc (fixups.json "append");
// called at the end of each Main_Loop seam pass (mainloop_native.inc). Test scaffolding, NOT original code; never part
// of an exact build.
//
// The step server: the game paused by default, advanced only a requested number of loop frames, so a mission can be
// played (by an agent, a script or a person at a prompt) without real-time precision.
//
//   RA2_NATIVE_STEP=<socket path>    live: at the served scenario's first frame the game stops and serves commands on a
//                                    Unix socket (platform/sdl/host.cpp ra2port_step_*: one client at a time, any number in
//                                    turn; ra2/tools/ra2_step.py is the client). POSIX builds (Mac, iOS Simulator).
//   RA2_NATIVE_STEP_MISSION=<n>      the n-th scenario of the run is the one served or replayed (default 1); ticks count
//                                    loop frames from its first (tick 0), so a replay does not depend on the route there
//   RA2_NATIVE_STEP_RECORD=<file>    every command that changes the game appended as "<tick> <command>": the replay
//                                    script (inputs and ticks only; no game data)
//   RA2_NATIVE_STEP_PRELOAD=<file>   live: a replay's commands run first at their ticks, then the server serves (a recorded
//                                    run resumed at any tick)
//   RA2_NATIVE_STEP_REPLAY=<file>    replay: the recorded commands run at their ticks, no server (every platform)
// Determinism (what made a live session and its replay part, found here): the host's event pump runs at most once a
// millisecond, so under RA2_NATIVE_SCRIPT_ONLY=1 the frame's scripted events are pumped as they are played (host.cpp
// frame()); the game's mouse follows the pointer on the mouse thread every 16 ms of wall time, so it is processed once a
// frame here; a one-frame click was read a click late, so a click holds the button four frames; a pointer left at the
// screen's corner edge-scrolled the map, so it is parked in the view's middle at the start and after each click.
//
// A command runs between two loop frames: after the seam's pass for tick T (the frame's game logic, scripted input
// due at T, the harness orders) and before T+1's Main_Loop. Commands (one line each; every reply ends with a line
// holding a single "."):
//   advance <n>              run n loop frames, stopping early when the game ends; reply: "tick <T> loop <F> game
//                            <Frame>" and the news since the last reply (triggers gone or new, technos lost or appeared)
//   input [+<k>] <action>    one line of the port's input script without its frame (platform/sdl/host.cpp: click P,
//                            rclick P, drag P P, move P, ldown/lup/rdown/rup P, key NAME, keydown/keyup NAME, wheel ...),
//                            played from tick T+1+k through the SDL event path a player's mouse and keys take, so the
//                            sidebar, the command bar and the selection code run as in play. P is "X Y" in game pixels
//                            or "@..." (harness_input_native.inc: "@^ID" the object with that ID where it is drawn,
//                            "@=X,Y" a cell's centre, "@ID[:n]" the player's n-th ID, "@!ID[:n]" another house's,
//                            "@tab=N", "@cameo=ID", "@cmd=NAME"); a point not in the view gives no click   [recorded]
//   look <x> <y>             the view centred on cell x,y through the trackpad pan's hook (ra2_native_scroll_map, the
//                            path the host's pan gesture takes; clamped at the map's edges)                [recorded]
//   order <harness order>    a game-event shortcut (harness_native.inc RB_Run_Order: goto, fire, deploy, train,
//                            hunt ...), for what the UI path cannot reach in a script; each use is a shortcut, said
//                            so in the replay                                                              [recorded]
//   save <file>              Save_Game in the write folder, as the options menu's Save                    [recorded]
//   state                    the structured state: scenario, ticks, game over, the mission timer, the player's credits,
//                            power and input lock, the houses, the factories (what each builds, %), each sidebar tab's
//                            build options, the view, every techno (id, kind, type, house, cell, strength/max,
//                            mission, selected, own/ally, on a cell the player has mapped), the live triggers
//   objects [mine|enemy|all] the techno lines alone
//   triggers                 the live triggers (id, name, enabled, tripped events)
//   cell <x> <y> [<r>]       the cells around x,y: water/cliff/land, height, mapped by the player, occupant
//   terrain <x0> <y0> <x1> <y1>  one character a cell (land type, bridge, buildings, the player's and the enemy's)
//   buildable <T> <x> <y> [r] where building type T would place (origins near x,y; r 0: each blocked cell's reason)
//   probe <x> <y>            a cell's pixel in the view and the view's position (diagnostics)
//   screenshot <file>        the last picture as the window shows it (BMP)
//   mapfile <file> [<name>]  the scenario's own file (map, triggers, teams: the user's data, read through the game's
//                            mix files) copied to <file>, for planning (ra2/tools/ra2_mapinfo.py); never into the
//                            repository. With a name, that data file instead (RULES.INI: weapon ranges, speeds)
//   release                  the server stops; the game runs on freely
//   quit                     the process ends
// When the game ends the connected client's remaining commands are answered, then the score screen follows.
// ---------------------------------------------------------------------------------------------
#include "ccfile.h"
#include "_rect.h"
#include "rules.h"

extern "C" int ra2port_step_listen(const char * path);          // platform/sdl/host.cpp (POSIX; 0 elsewhere)
extern "C" int ra2port_step_getline(char * buffer, int size);   // blocks for the next command line; -1 when no server
extern "C" int ra2port_step_getline_ex(char * buffer, int size, int connected_only);   // -2: no client connected
extern "C" void ra2port_step_reply(const char * text);
extern "C" int ra2port_script_add(int frame, const char * text);
extern "C" int ra2port_screenshot(const char * path);
extern "C" int ra2_native_scroll_map(int, int, int dx, int dy);  // mainloop_native.inc
extern "C" void ra2port_progress(const char * milestone, const char * detail);
extern "C" void ra2port_exit_now(int code);                     // platform/sdl/host.cpp
extern "C" int ra2port_host_write_file(const char * path, const void * data, int size, int append);   // the host's paths
extern "C" char * ra2port_host_read_file(const char * path, int * size);
bool Save_Game(const char * file_name, wchar_t const * descr, bool bargraph);
void RB_Process_Mouse_Now(void);          // glue/wwmouse_native.inc
extern bool PlayerWins;
extern bool PlayerLoses;

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
	RB_Step_Out("scenario %s loop %d game %d over %d win %d lose %d", Scen ? (char const *)Scen->ScenarioName : "?", frame, Frame,
		PlayerWins || PlayerLoses ? 1 : 0, PlayerWins ? 1 : 0, PlayerLoses ? 1 : 0);
	if (Scen != NULL && Scen->MissionTimer.Is_Active()) RB_Step_Out("timer %ld frames", (long)Scen->MissionTimer.Value());
	if (PlayerPtr != NULL) {
		RB_Step_Out("player %s credits %ld power %d drain %d input %s", RB_House_Name(PlayerPtr), (long)PlayerPtr->Available_Money(),
			PlayerPtr->Power, PlayerPtr->Drain, IgnoreInput ? "locked" : "open");
	}
	for (int h = 0; h < Houses.Count(); ++h) {
		HouseClass * house = Houses[h];
		if (house == NULL) continue;
		RB_Step_Out("house h%d %s %s%s%s buildings %d", (int)house->HeapID, RB_House_Name(house),
			house == PlayerPtr ? "player" : (PlayerPtr && PlayerPtr->Is_Ally(house)) ? "ally" : "enemy",
			house->IsDefeated ? " defeated" : "", house->IsToWin ? " to-win" : house->IsToLose ? " to-lose" : "", house->Buildings.Count());
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
			n += snprintf(line + n, sizeof(line) - n, " %c%d%c%s", c.Is_Tile_Water() ? 'w' : c.Is_Tile_Cliff() ? 'c' : '.', (int)c.Height,
				c.IsMapped ? 'm' : '?', who ? (who->Owner_HouseClass() == PlayerPtr ? "P" : "o") : "");
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
		RB_Run_Order(rest);
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
		// terrain <x0> <y0> <x1> <y1>: one character a cell: the land type (. clear, r road, ~ water, # rock, W wall,
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
									(int)Map.In_Local_Radar(&cc), (int)cc.Is_Bridge_Here(), (int)cc.Ramp, (unsigned)(cc.Flag.Composite & 0x3F));
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

void RB_Step_Server_Frame(int frame)
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
			char note[128];
			int n = snprintf(note, sizeof(note), "# %s: ticks are loop frames from the scenario's first\n", scenario);
			if (rb_step_record) ra2port_host_write_file(rb_step_record, note, n, 1);
		} else if (scenarios > target) {
			base = -1;
		}
	}
	if (base < 0) return;
	// The game's mouse (WWMouseClass: the pointer the tactical view's cursor and click actions read) follows the pointer on
	// the mouse thread, every 16 ms of wall time: at the uncapped loop's speed a replay ran several frames before it moved,
	// while a live session's pauses let it catch up, and a click landed elsewhere. Moved here, once a frame, after the
	// frame's scripted input reached the game, it is where the input put it whatever the machine's speed.
	RB_Process_Mouse_Now();
	int tick = frame - base;
	rb_step_host_offset = base;
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
		if (RB_Step_Command(tick, line, true)) RB_Step_Record(tick, line);
		if (rb_step_budget == 0) ra2port_step_reply(".");
	}
}
