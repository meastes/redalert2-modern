// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P8, branch ports-on-rebuild): appended to the generated conquer unit after harness_native.inc
// (fixups.json "append"); called each loop frame from the Main_Loop seam. Test scaffolding, NOT original code;
// never part of an exact build.
//
// 1. Scripted points (main's native/link/harness_input.cpp, harness_sidebar.cpp, team_button_hook.cpp under the
//    rebuild's names): the port's input script resolves "@..." points through ra2port_set_point_resolver:
//      "@ID[:n][+dx,dy]"  the player's n-th object of type ID (0 the first; then other houses under the player's control),
//                         moved by dx, dy cells: its centre's screen pixel (Tactical::Coord_To_Pixel)
//      "@!ID[:n]"         the n-th newest object of type ID of a house the player does not control
//      "@=X,Y"            the centre of cell X,Y
//      "@tab=N"           the sidebar's tab button N (0 buildings, 1 defences, 2 infantry, 3 vehicles)
//      "@cameo=ID"        the build icon of type ID in the strip shown now
//      "@cmd=NAME"        the command bar's button for a command (AttackMove, Beacon, Cheer, Deploy, Guard,
//                         PlanningMode, Stop, TypeSelect, Team01..Team03)
//      "@thumb"           the command bar's thumb shown now (the close thumb while the bar is open, else the open one)
// 2. The state lines main's tests read (RA2_NATIVE_INPUT_LOG=<n>: every n loop frames, and at --skirmish-frames):
//      harness-view <frame> @x,y msgs 0
//      harness-state <frame> selected <n>; <ID>[*]:m<mission>@<cellx>,<celly> ... inf=<n> veh=<n>
//    (main's format; '*' selected; inf/veh the objects in the player's infantry and vehicle factories).
//    Orders (RA2_NATIVE_ORDERS, harness_native.inc's list): `thumb` (main's lane N31 line: harness-thumb
//    open|closed close X,Y WxH[ listed] open X,Y WxH[ listed] bar X,Y WxH end X,Y) and `view` (harness-view <x>,<y>
//    locked <0|1> zoom <z>).
// 3. The mission soak's digest (main's native/link/harness_soak.cpp, lane N25), for frame identity with the src-based port:
//      RA2_NATIVE_SOAK=<period>       soak-mission/-trigger/-timer/-house lines and every <period> game frames
//                                     soak-digest <frame> <hash> technos <n> player <n> credits <n> rng <hash> view <x>,<y>
//                                     (the same hash over the same fields in the same order as main's)
//      RA2_NATIVE_SOAK_SEED=<n>       CustomSeed for Init_Random (set at Select_Game's top: fixups.json)
//      RA2_NATIVE_SOAK_SPEED=<0-6>    GameControlsShown and Options.GameSpeed (a single-player game at that speed)
//      RA2_NATIVE_SOAK_END=<frames>[@<n>]  the n-th scenario of the run ends the run at <frames> or when won or lost
//                                     (milestone soak-end)
//      RA2_NATIVE_SOAK_DUMP=<from>[-<to>]  every techno at those frames (soak-techno lines)
// ---------------------------------------------------------------------------------------------
#include "cell.h"
#include "spawnmanager.h"
#include "factory.h"
extern "C" void ra2port_progress(const char * milestone, const char * detail);
extern Rect * TabBarRect;                                  // txtlabel2.cpp
extern "C" void ra2port_set_point_resolver(int (*resolver)(const char * spec, int * x, int * y));
extern "C" int ra2port_skirmish_frames(void);
extern "C" double ra2_native_zoom(void);                   // glue/zoom_native.inc

namespace {

bool RB_Same_Name(char const * a, char const * b, int length)
{
	for (int i = 0; i < length; ++i, ++a, ++b) {
		char x = *a >= 'a' && *a <= 'z' ? *a - 32 : *a;
		char y = *b >= 'a' && *b <= 'z' ? *b - 32 : *b;
		if (x != y) return false;
		if (x == 0) return true;
	}
	return *a == 0;
}
int RB_Read_Int(char const * & p)
{
	int sign = 1, value = 0;
	if (*p == '-') { sign = -1; ++p; } else if (*p == '+') ++p;
	while (*p >= '0' && *p <= '9') value = value * 10 + (*p++ - '0');
	return sign * value;
}
bool RB_Prefix(char const * text, char const * prefix, char const * & rest)
{
	while (*prefix && *text == *prefix) { ++text; ++prefix; }
	if (*prefix) return false;
	rest = text;
	return true;
}
void RB_Centre(GadgetClass const & gadget, int * x, int * y)
{
	*x = gadget.X + gadget.Width / 2;
	*y = gadget.Y + gadget.Height / 2;
}
bool RB_In_Gadget_List(GadgetClass const * gadget)
{
	for (GadgetClass * g = GScreenClass::Buttons; g != NULL; g = (GadgetClass *)g->Get_Next()) {
		if (g == gadget) return true;
	}
	return false;
}

int RB_Sidebar_Point(char const * spec, int * x, int * y)
{
	char const * rest;
	if (RB_Prefix(spec, "tab=", rest)) {
		int tab = *rest - '0';
		if (tab < 0 || tab > 3) return 0;
		RB_Centre(SidebarClass::TabButton[tab], x, y);
		return 1;
	}
	if (!RB_Prefix(spec, "cameo=", rest)) return 0;
	int shown = Map.Get_Doubled_Row_Count();
	for (int strip = 0; strip < 4; ++strip) {
		SidebarClass::StripClass & column = Map.Column[strip];
		if (!column.AllowedToDraw) continue;
		for (int i = 0; i < shown && i < 60; ++i) {
			int index = column.TopIndex * 2 + i;
			if (index >= column.BuildableCount) break;
			TechnoTypeClass const * type = Fetch_Techno_Type(column.Buildables[index].BuildableType, column.Buildables[index].BuildableID);
			if (type && RB_Same_Name(type->Name(), rest, 64)) {
				RB_Centre(SidebarClass::StripClass::SelectButton[strip * 60 + i], x, y);
				return 1;
			}
		}
	}
	return 0;
}

int RB_Bar_Point(char const * spec, int * x, int * y)
{
	static struct { char const * name; int const * slot; } const commands[] = {
		{"AttackMove", &TabClass::AttackMoveCommand}, {"Beacon", &TabClass::BeaconCommand},
		{"Cheer", &TabClass::CheerCommand}, {"Deploy", &TabClass::DeployCommand}, {"Guard", &TabClass::GuardCommand},
		{"PlanningMode", &TabClass::PlanningModeCommand}, {"Stop", &TabClass::StopCommand},
		{"TypeSelect", &TabClass::TypeSelectCommand}, {"Team01", &TabClass::Team01Command},
		{"Team02", &TabClass::Team02Command}, {"Team03", &TabClass::Team03Command}};
	int slot = -1;
	for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
		if (RB_Same_Name(spec + 4, commands[i].name, 64)) slot = *commands[i].slot;
	}
	ShapeButtonClass * button = Map.IsThumbActive && slot >= 0 ? TabClass::Get_Button(slot) : NULL;
	if (button == NULL || !button->Is_Enabled()) {
		fprintf(stderr, "input-script: no command bar button for %s (slot %d)\n", spec, slot);
		return 0;
	}
	*x = button->X + button->Width / 2;
	*y = button->Y + button->Height / 2;
	return 1;
}

int RB_Thumb_Point(int * x, int * y)
{
	ShapeButtonClass & button = Map.IsThumbActive ? TabClass::ThumbOpenButton : TabClass::ThumbClosedButton;
	if (!RB_In_Gadget_List(&button)) {
		fprintf(stderr, "input-script: the bar's %s thumb is not a button now\n", Map.IsThumbActive ? "close" : "open");
		return 0;
	}
	*x = button.X + button.Width / 2;
	*y = button.Y + button.Height / 2;
	return 1;
}

int RB_Resolve(char const * spec, int * x, int * y)
{
	HouseClass * player = PlayerPtr;
	if (player == NULL || TacticalMap == NULL) return 0;
	if (RB_Sidebar_Point(spec, x, y)) return 1;
	if (spec[0] == 'c' && spec[1] == 'm' && spec[2] == 'd' && spec[3] == '=') return RB_Bar_Point(spec, x, y);
	if (!strcmp(spec, "thumb")) return RB_Thumb_Point(x, y);
	if (spec[0] == '=') {
		char const * p = spec + 1;
		Cell cell;
		cell.X = (short)RB_Read_Int(p);
		if (*p == ',') ++p;
		cell.Y = (short)RB_Read_Int(p);
		Coord coord = Map[cell].Center_Coord();
		Point2D pixel;
		if (!TacticalMap->Coord_To_Pixel(coord, pixel)) return 0;
		*x = pixel.X;
		*y = pixel.Y;
		return 1;
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
			if (techno == NULL || !RB_Same_Name(RB_Type_Name(techno), spec, length)) continue;
			if (pass == 2 ? techno->House == player || techno->House->Is_Player_Control()
				: pass == 0 ? techno->House != player : techno->House == player || !techno->House->Is_Player_Control()) continue;
			if (index-- > 0) continue;
			Coord coord = techno->Center_Coord();
			coord.X += dx * 256;
			coord.Y += dy * 256;
			Point2D pixel;
			TacticalMap->Coord_To_Pixel(coord, pixel);
			*x = pixel.X;
			*y = pixel.Y;
			return 1;
		}
	}
	return 0;
}

int RB_In_Factory(RTTIType kind)
{
	HouseClass * player = PlayerPtr;
	FactoryClass * factory = player ? player->Fetch_Factory(kind, false, 0) : NULL;
	if (factory == NULL) return 0;
	int n = 0;
	if (kind == RTTI_INFANTRY || kind == RTTI_INFANTRYTYPE) {
		for (int t = 0; t < InfantryTypes.Count(); ++t) n += factory->Total(InfantryTypes[t]);
	} else {
		for (int u = 0; u < UnitTypes.Count(); ++u) n += factory->Total(UnitTypes[u]);
	}
	return n;
}

bool RB_Is_Selected(ObjectClass * object)
{
	for (int i = 0; i < CurrentObject.Count(); ++i) {
		if (CurrentObject[i] == object) return true;
	}
	return false;
}

void RB_Harness_Input(int frame)
{
	static int every = -1;
	if (every < 0) {
		char const * text = getenv("RA2_NATIVE_INPUT_LOG");
		every = text ? atoi(text) : 0;
	}
	bool last = frame == ra2port_skirmish_frames();
	if ((!last && (every <= 0 || frame % every != 0)) || PlayerPtr == NULL) return;
	Point2D view = TacticalMap ? TacticalMap->Get_Tactical_Position() : Point2D(0, 0);
	fprintf(stderr, "harness-view %d @%d,%d msgs %d\n", frame, view.X, view.Y, 0);
	static char detail[4096];
	int n = snprintf(detail, sizeof(detail), "selected %d", CurrentObject.Count());
	for (int i = 0; i < CurrentObject.Count() && n < (int)sizeof(detail) - 80; ++i) {
		ObjectClass * object = CurrentObject[i];
		HouseClass * house = object ? object->Owner_HouseClass() : NULL;
		if (object == NULL || house == PlayerPtr) continue;
		ObjectTypeClass const * type = object->Class_Of();
		n += snprintf(detail + n, sizeof(detail) - n, " other=%s/%s", type ? type->Name() : "?", house ? house->Class->Name() : "-");
	}
	n += snprintf(detail + n, sizeof(detail) - n, ";");
	for (int k = 0; k < Technos.Count() && n < (int)sizeof(detail) - 80; ++k) {
		TechnoClass * techno = Technos[k];
		if (techno == NULL || techno->House != PlayerPtr) continue;
		Cell cell = techno->Center_Coord().As_Cell();
		n += snprintf(detail + n, sizeof(detail) - n, " %s%s:m%d@%d,%d", RB_Type_Name(techno), RB_Is_Selected(techno) ? "*" : "",
			(int)techno->CurrentMission, cell.X, cell.Y);   // the field, as main's harness_input.cpp
		int pending = techno->GetMegaMission();   // main's ":p<order>" (FootClass slot 272: 0x1D an attack-move queued)
		if (pending != -1) n += snprintf(detail + n, sizeof(detail) - n, ":p%d", pending);
	}
	n += snprintf(detail + n, sizeof(detail) - n, " inf=%d veh=%d", RB_In_Factory(RTTI_INFANTRY), RB_In_Factory(RTTI_UNIT));
	fprintf(stderr, "harness-state %d %s\n", frame, detail);
	// The milestones main's tests read (harness-input-<frame> for the first 64 logged frames, harness-input at the last).
	static char names[64][24];
	static int named = 0;
	if (last) {
		ra2port_progress("harness-input", detail);
	} else if (named < 64) {
		snprintf(names[named], sizeof(names[named]), "harness-input-%d", frame);
		ra2port_progress(names[named++], detail);
	}
}

// --- the mission soak (main's harness_soak.cpp) ---
unsigned RB_Mix(unsigned hash, unsigned value)
{
	for (int i = 0; i < 4; ++i) {
		hash ^= (value >> (8 * i)) & 0xFF;
		hash *= 16777619u;
	}
	return hash;
}
unsigned RB_Mix_Text(unsigned hash, char const * text)
{
	for (; text && *text; ++text) hash = RB_Mix(hash, (unsigned char)*text);
	return hash;
}

void RB_Digest(int frame)
{
	unsigned hash = 2166136261u;
	int player = 0;
	for (int i = 0; i < Technos.Count(); ++i) {
		TechnoClass * techno = Technos[i];
		if (techno == NULL) continue;
		TechnoTypeClass const * type = techno->Techno_Type_Class();
		hash = RB_Mix_Text(hash, type ? type->Name() : "?");
		hash = RB_Mix(hash, techno->House ? (int)techno->House->HeapID : -1);
		Coord coord = techno->Get_Coord();
		hash = RB_Mix(hash, coord.X);
		hash = RB_Mix(hash, coord.Y);
		hash = RB_Mix(hash, coord.Z);
		hash = RB_Mix(hash, techno->Strength);
		hash = RB_Mix(hash, (int)techno->CurrentMission);   // the field, as main's Digest (Get_Mission answers MissionQueue while it is MISSION_NONE)
		if (techno->House == PlayerPtr) ++player;
	}
	for (int h = 0; h < Houses.Count(); ++h) {
		if (Houses[h]) hash = RB_Mix(hash, Houses[h]->Credits);
	}
	unsigned rng = 2166136261u;
	unsigned char const * bytes = (unsigned char const *)&Scen->RandomNumber;
	for (unsigned b = 0; b < sizeof(Scen->RandomNumber); ++b) rng = RB_Mix(rng, bytes[b]);
	Point2D view = TacticalMap ? TacticalMap->Get_Relative_Tactical_Position() : Point2D(0, 0);
	fprintf(stderr, "soak-digest %d %08x technos %d player %d credits %d rng %08x view %d,%d\n", frame, hash, Technos.Count(),
		player, PlayerPtr ? PlayerPtr->Credits : -1, rng, view.X, view.Y);
}

// MissionClass::Timer is private (mission.h): read through a member pointer taken by an explicit instantiation, as
// harness_native.inc reads TriggerClass::IsTripped.
template<CDTimerClass<FrameTimerClass> MissionClass::* M> struct RB_Mission_Timer_Access { friend CDTimerClass<FrameTimerClass> MissionClass::* RB_Mission_Timer(void) { return(M); } };
CDTimerClass<FrameTimerClass> MissionClass::* RB_Mission_Timer(void);
template struct RB_Mission_Timer_Access<&MissionClass::Timer>;

void RB_Soak_Dump(int frame)
{
	for (int i = 0; i < Technos.Count(); ++i) {
		TechnoClass * techno = Technos[i];
		if (techno == NULL) continue;
		Coord coord = techno->Get_Coord();
		fprintf(stderr, "soak-techno %d #%d %s h%d %d,%d,%d s%d m%d k%d q%d st%d t%ld\n", frame, i, RB_Type_Name(techno),
			techno->House ? (int)techno->House->HeapID : -1, coord.X, coord.Y, coord.Z, techno->Strength, (int)techno->CurrentMission,
			(int)techno->Fetch_RTTI(), (int)techno->MissionQueue, techno->Status, (long)(techno->*RB_Mission_Timer()).Value());
		if (SpawnManagerClass * spawn = techno->SpawnManager) {   // main's soak-spawn line
			char nodes[256] = "";
			int n = 0;
			for (int k = 0; k < spawn->SpawnedNodes.Count() && n < 200; ++k) {
				SpawnControl * node = spawn->SpawnedNodes[k];
				n += snprintf(nodes + n, sizeof(nodes) - n, " %d/%ld", node->Status, (long)node->Timer.Value());
			}
			fprintf(stderr, "soak-spawn %d #%d status %d update %ld spawn %ld dest %d target %d nodes%s\n", frame, i,
				spawn->Status, (long)spawn->UpdateTimer.Value(), (long)spawn->SpawnTimer.Value(), spawn->Destination != NULL,
				spawn->Target != NULL, nodes);
		}
	}
}

char rb_soak_scenario[0x104] = "";
int rb_soak_last_frame = -1;
bool rb_soak_timer = false;
bool rb_soak_house_done[64];

}

// CustomSeed from RA2_NATIVE_SOAK_SEED (Select_Game's top, before its Init_Random; main's Select_Game seam).
void RB_Soak_Seed(void)
{
	char const * env = getenv("RA2_NATIVE_SOAK_SEED");
	if (env && *env) CustomSeed = (unsigned)atoi(env);
}

void RB_Harness_Soak(int loop_frame)
{
	static int period = -2;
	if (period == -2) {
		char const * env = getenv("RA2_NATIVE_SOAK");
		period = env ? atoi(env) : -1;
		if (period == 0) period = 500;
	}
	static int dump_from = -1, dump_to = -2;
	static bool dump_read = false;
	if (!dump_read) {
		dump_read = true;
		if (char const * env = getenv("RA2_NATIVE_SOAK_DUMP")) {
			dump_from = dump_to = atoi(env);
			for (char const * p = env; *p; ++p) if (*p == '-') dump_to = atoi(p + 1);
		}
	}
	static int speed = -2;
	if (speed == -2) {
		char const * env = getenv("RA2_NATIVE_SOAK_SPEED");
		speed = env && *env ? atoi(env) : -1;
	}
	if (speed >= 0) {
		GameControlsShown = true;
		Options.GameSpeed = speed;
	}
	if (period < 0 || Scen == NULL) return;
	int frame = Frame;
	bool renamed = false;
	for (int k = 0; k < (int)sizeof(rb_soak_scenario) - 1; ++k) {
		if (rb_soak_scenario[k] != Scen->ScenarioName[k]) renamed = true;
		rb_soak_scenario[k] = Scen->ScenarioName[k];
		if (Scen->ScenarioName[k] == 0) break;
	}
	static int end_frames = -2, end_mission = 0, missions = 0;
	if (end_frames == -2) {
		char const * env = getenv("RA2_NATIVE_SOAK_END");
		end_frames = env ? atoi(env) : -1;
		end_mission = 1;
		for (char const * p = env; p && *p; ++p) if (*p == '@') end_mission = atoi(p + 1);
	}
	if (frame < rb_soak_last_frame || renamed) {
		++missions;
		fprintf(stderr, "soak-mission %s loop %d triggers %d types %d\n", rb_soak_scenario, loop_frame, Triggers.Count(), TriggerTypes.Count());
		rb_soak_timer = false;
		for (int k = 0; k < 64; ++k) rb_soak_house_done[k] = false;
	}
	if (frame != rb_soak_last_frame) {
		bool active = Scen->MissionTimer.Is_Active();
		if (active != rb_soak_timer) {
			fprintf(stderr, "soak-timer %d %s %ld\n", frame, active ? "started" : "stopped", (long)Scen->MissionTimer.Value());
			rb_soak_timer = active;
		}
		for (int i = 0; i < Houses.Count() && i < 64; ++i) {
			HouseClass * house = Houses[i];
			if (house == NULL || rb_soak_house_done[i]) continue;
			if (house->IsDefeated) {
				rb_soak_house_done[i] = true;
				fprintf(stderr, "soak-house %d %d:%s defeated\n", frame, i, (char const *)house->IniName);   // lane P14: the name main's soak prints (the player's, not the house type's)
			}
		}
		if (frame % period == 0) RB_Digest(frame);
		if (frame >= dump_from && frame <= dump_to) RB_Soak_Dump(frame);
	}
	rb_soak_last_frame = frame;
	if (end_frames >= 0 && missions == end_mission) {
		char const * how = NULL;
		if (PlayerWins) how = "won";
		else if (PlayerLoses || (PlayerPtr && PlayerPtr->IsDefeated)) how = "lost";
		else if (frame >= end_frames) how = "played";
		if (how) {
			char detail[0x140];
			snprintf(detail, sizeof(detail), "%s %s at frame %d", rb_soak_scenario, how, frame);
			fprintf(stderr, "soak-end %s\n", detail);
			ra2port_progress("soak-end", detail);
			end_frames = -1;
		}
	}
}

// The thumb and view orders (harness_native.inc's RB_Run_Order asks).
bool RB_Run_Input_Order(char const * word)
{
	if (!strcmp(word, "thumb")) {
		ShapeButtonClass const & a = TabClass::ThumbOpenButton;
		ShapeButtonClass const & b = TabClass::ThumbClosedButton;
		Rect bar = TabBarRect ? *TabBarRect : Rect(0, 0, 0, 0);
		Rect end = TabEndRect ? *TabEndRect : Rect(0, 0, 0, 0);
		fprintf(stderr, "harness-thumb %s close %d,%d %dx%d%s open %d,%d %dx%d%s bar %d,%d %dx%d end %d,%d\n", Map.IsThumbActive ? "open" : "closed",
			a.X, a.Y, a.Width, a.Height, RB_In_Gadget_List(&a) ? " listed" : "",
			b.X, b.Y, b.Width, b.Height, RB_In_Gadget_List(&b) ? " listed" : "", bar.X, bar.Y, bar.Width, bar.Height, end.X, end.Y);
		return true;
	}
	if (!strcmp(word, "view")) {
		Point2D view = TacticalMap ? TacticalMap->Get_Tactical_Position() : Point2D(0, 0);
		fprintf(stderr, "harness-view %d,%d locked %d zoom %.3f frame %d\n", view.X, view.Y, IgnoreInput ? 1 : 0, ra2_native_zoom(), Frame);
		return true;
	}
	return false;
}

void RB_Harness_Input_Frame(int frame)
{
	static bool registered = false;
	if (!registered) {
		registered = true;
		ra2port_set_point_resolver(RB_Resolve);
	}
	// RA2_NATIVE_SKIRMISH_SPEED=<0-6> (main's name, harness_skirmish.cpp): Options.GameSpeed at the game's first frame. Main's
	// skirmish harness starts the game without the setup dialog, so its game runs at RA2.INI's [Options] GameSpeed (3 in the
	// user's install, about 20 frames a second), where the dialog's Start stores its slider (0 by default: the 62 a second
	// frame floor); tools/rb_skirmish_wrap.py sets 3 so main's tests' wall-clock rules (a double click's 500 ms) see the same
	// frame rate.
	if (frame == 1) {
		if (char const * speed = getenv("RA2_NATIVE_SKIRMISH_SPEED")) {
			Options.GameSpeed = atoi(speed);
			fprintf(stderr, "harness-speed %d\n", Options.GameSpeed);
		}
	}
	RB_Harness_Input(frame);
	RB_Harness_Soak(frame);
}
