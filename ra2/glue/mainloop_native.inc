// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P2, branch ports-on-rebuild): appended to the generated mainloop unit
// (native/rb/fixups.json "append"); Main_Game (conquer.cpp) calls Main_Loop through this seam.
// NOT original code; never part of an exact build.
//
// Frame pacing, as main's port (native/link/harness_skirmish.cpp Native_Frame_Floor, user report
// 2026-11-09, M20/Scout1191): the skirmish speed slider's default is GameSpeed 0 ("fastest"), for which
// Main_Loop sets the frame timer to 0 and Sync_Delay does not wait; the original ran as fast as the
// machine drew. Natively that is hundreds of frames a second (this port's AI won a skirmish in a minute
// of wall time), so in single player and skirmish a frame whose timer gave no wait keeps a 16 ms
// deadline (one SystemTimerClass tick, about 62 frames a second); network games pace themselves.
// RA2_NATIVE_UNCAPPED=1 restores the uncapped loop. Each frame is also reported to the port's run
// progress ("frame", the count), which counts frames for --run-to=frame/frames and scripted input.
// ---------------------------------------------------------------------------------------------
extern "C" uint64_t ra2port_monotonic_ns(void);
extern "C" void ra2port_sleep_until_ns(uint64_t deadline);
extern "C" void ra2port_progress(const char * milestone, const char * detail);
extern "C" void ra2port_frame(int frame);          // scripted input due at the next frame; the frame count
extern "C" int ra2port_skirmish_frames(void);      // --frames=N

static void RB_Frame_Floor(void)
{
	static int uncapped = -1;
	if (uncapped < 0) uncapped = getenv("RA2_NATIVE_UNCAPPED") ? 1 : 0;
	static uint64_t deadline = 0;
	uint64_t const period = 16000000u;
	// Only at game speed 0 (the fastest, where the original's frame timer gives no wait), as main's
	// Native_Frame_Floor (native/link/harness_skirmish.cpp: Options.GameSpeed, 0x00A40B18, > 0 skips it);
	// the slower speeds keep the original's own pacing alone (lane P12, the parity audit).
	if (uncapped || Options.GameSpeed > 0 || (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH)) {
		deadline = 0;
		return;
	}
	uint64_t now = ra2port_monotonic_ns();
	if (deadline && now < deadline) {
		ra2port_sleep_until_ns(deadline);
		deadline += period;
	} else {
		deadline = now + period;
	}
}

// Harness save (lane P3, main's native/link/harness_save.cpp Native_Harness_Save; test scaffolding, not
// original behaviour): RA2_NATIVE_SAVE_AT=<frame>[,<file>] saves the game at that loop frame as the
// options menu's Save ends in (Save_Game), under <file> (default SAVE0001.SAV) in the write folder;
// RA2_NATIVE_SAVE_LEAVE=1 then aborts the game (PlayerAborts) as the options menu's Abort does.
bool Save_Game(const char * file_name, wchar_t const * descr, bool bargraph);
extern bool PlayerAborts;

static void RB_Harness_Save(int frame)
{
	static int at = -2;
	static char file[64] = "SAVE0001.SAV";
	if (at == -2) {
		at = -1;
		if (char const * env = getenv("RA2_NATIVE_SAVE_AT")) {
			at = atoi(env);
			if (char const * comma = strchr(env, ',')) snprintf(file, sizeof(file), "%s", comma + 1);
		}
	}
	if (at < 0 || frame != at) return;
	bool saved = Save_Game(file, L"Native harness save", false);
	char detail[128];
	snprintf(detail, sizeof(detail), "frame %d: %s %s", frame, saved ? "saved" : "save failed", file);
	ra2port_progress("harness-save", detail);
	char const * leave = getenv("RA2_NATIVE_SAVE_LEAVE");
	if (saved && leave && *leave == '1') {
		PlayerAborts = true;
	}
}

// Forced end (lane P7, main's native/link/harness_save.cpp; test scaffolding, not original behaviour):
// RA2_NATIVE_FORCE_END=<f1>[,<f2>,...][,lose] sets PlayerWins (or PlayerLoses), the flag Main_Loop_End reads
// as the house AI sets it on a win or a defeat, at the first loop frame at or past each entry (counted by the
// seam from the first game frame, across games), so the score screen follows and a campaign goes on mission
// by mission. Each entry logs `force-end: win|loss at frame <f> in <scenario>` (main's line).
extern bool PlayerWins;
extern bool PlayerLoses;

static void RB_Harness_Force_End(int frame)
{
	static int end_at[32];
	static int end_count = -1;
	static int end_done = 0;
	static bool end_lose = false;
	if (end_count < 0) {
		end_count = 0;
		for (char const * p = getenv("RA2_NATIVE_FORCE_END"); p && *p;) {
			if (*p == 'l') end_lose = true;
			else if (*p >= '0' && *p <= '9' && end_count < 32) end_at[end_count++] = atoi(p);
			while (*p && *p != ',') ++p;
			if (*p == ',') ++p;
		}
	}
	if (end_done < end_count && frame >= end_at[end_done]) {
		++end_done;
		if (end_lose) PlayerLoses = true; else PlayerWins = true;
		fprintf(stderr, "force-end: %s at frame %d in %s\n", end_lose ? "loss" : "win", frame, Scen ? Scen->ScenarioName : "?");
	}
}

void RB_Harness_Frame(int frame);   // harness_native.inc (lane P5): RA2_NATIVE_ORDERS
void RB_Harness_Soak(int frame);    // harness_input_native.inc (lane P8): RA2_NATIVE_SOAK, main's harness_soak.cpp
void RB_Harness_Input_Frame(int frame);   // harness_input_native.inc (lane P8): scripted points, RA2_NATIVE_INPUT_LOG

// The host's game-side hooks (lane P7, the feature audit; main's native/link/app_hooks.cpp, native_touch_cursor.cpp,
// harness_skirmish.cpp and harness_save.cpp, the YR port's yr_app_hooks.cpp under the rebuild's names). Not original code.
// Without them the host used its weak defaults (no pan, no cursor hiding, no checkpoint) and the port's redraw question
// after a focus change in the menus reached a link-closure trap.
#include "queue.h"
#include "options.h"
extern "C" void ra2_enh_in_play(int on);          // platform/sdl/enhancements.cpp: the split layout, touch play mode, pinch
extern "C" int ra2port_take_checkpoint(void);     // port/src/port_core.cpp (Scout1248)
extern "C" void ra2port_checkpoint_saved(void);

extern "C" int ra2port_game_redraw_pending(void)  // port/src/user32.cpp: is the screen redraw mark still set?
{
	return SurfacesRestored ? 1 : 0;
}

extern "C" int ra2_native_network_game(void)      // host.cpp: a network game keeps running in the background
{
	return Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH;
}

extern "C" int ra2_native_leave_network_game(void)   // the iOS host, background time running out: Abort Mission's EXIT event
{
	if (!ra2_native_network_game()) return 0;
	return Queue_Exit() ? 1 : 0;
}

extern "C" void ra2_native_save_settings(void)    // Cmd+Q: RA2.INI written as the options dialog writes it
{
	Options.Save_Settings();
}

extern "C" int ra2_native_hide_game_cursor(void)  // touch: one extra Hide_Mouse keeps the game cursor hidden (Scout1249)
{
	if (!MouseCursor) return 0;
	if (!MouseCursor->Is_Hidden()) MouseCursor->Hide_Mouse();
	return 1;
}

extern "C" int ra2_native_scroll_map(int, int, int dx, int dy)   // trackpad and touch pan; locked missions ignore it (N31)
{
	Tactical * tactical = TacticalMap;
	if (!tactical) return 0;
	if (IgnoreInput) {
		static int last = -1000;     // main's app_hooks.cpp line (lane N31), once a swipe
		if (Frame - last > 30 || Frame < last) fprintf(stderr, "scroll: input locked (frame %d): the pan is ignored\n", Frame);
		last = Frame;
		return 1;
	}
	Point2D view = tactical->Get_Tactical_Position();
	view.X += dx;
	view.Y += dy;
	tactical->Set_Tactical_Position(view);
	return 1;
}

extern "C" int ra2_native_message_edit_open(void)  // touch keyboard (lane C14): the chat/beacon edit line is open
{
	return Session.Messages.Is_Edit() ? 1 : 0;
}

// LAN pacing (main's native_event_data.cpp Native_Wait_Pass_Due, Scout1223/Scout1231; not original code): in a
// network game Sync_Delay and Wait_For_Players run input, the tactical AI (edge scrolling) and a render on every pass of
// their wait loops. On Windows a render took most of a frame; natively the loop spins many times a frame and the view
// edge-scrolled several times as fast as in a skirmish. A pass runs only once the frame has stalled (100 ms since the
// seam's last frame), then at most every 33 ms (fixups.json, mainloop.cpp and queue.cpp).
static uint64_t RB_Last_Frame_Ns = 0;
bool RB_Wait_Pass_Due(void)
{
	static uint64_t last = 0;
	uint64_t now = ra2port_monotonic_ns();
	if (now - RB_Last_Frame_Ns < 100000000u) return false;
	if (now - last < 33000000u) return false;
	last = now;
	return true;
}

// Checkpoint saves (Scout1248, main's Native_Checkpoint): when the host asks (the iOS app going to the background, and
// periodically) the game is saved as CHECKPT.SAV, never in a network game.
static void RB_Checkpoint(void)
{
	if (!ra2port_take_checkpoint()) return;
	if (ra2_native_network_game()) {
		fprintf(stderr, "checkpoint: skipped (network game)\n");
		return;
	}
	bool saved = Save_Game("CHECKPT.SAV", L"Checkpoint", false);
	if (saved) ra2port_checkpoint_saved();
	fprintf(stderr, "checkpoint: %s\n", saved ? "saved CHECKPT.SAV" : "save failed");
	ra2port_progress("checkpoint", saved ? "saved CHECKPT.SAV" : "save failed");
}

static int rb_loop_frames = 0;      // the seam's loop frames (the run's frame count)

// Test scaffolding (lane P8): with the port's --skirmish (main's tests through tools/ra2_skirmish_wrap.py, which start the game
// through the menus) a run plays one game, as main's skirmish harness does (its Select_Game seam answers false the second
// time: WinMain returns 0 after the game's score screen). Select_Game asks at its top (fixups.json); without --skirmish never.
extern "C" const char * ra2port_skirmish_map(void);
bool RB_Skirmish_Played(void)
{
	return ra2port_skirmish_map() != NULL && rb_loop_frames > 0;
}

// The game's end as the src-based port's seam reports it (main's harness_skirmish.cpp: milestone game-over "player wins|player
// loses|restart|abort at frame <n>"; tests read it). Called at Process_Game_End's top (fixups.json): the in-game menu's Abort sets
// its flag in Ingame_Menu_Dialog, after the seam, so the seam alone would not see it.
void RB_Note_Game_Over(void)
{
	static int ended = 0;
	if (!(PlayerWins || PlayerLoses || PlayerRestarts || PlayerAborts)) return;
	char why[96];
	snprintf(why, sizeof(why), "%s at frame %d", PlayerWins ? "player wins" : PlayerLoses ? "player loses" : PlayerRestarts ? "restart" : "abort",
		rb_loop_frames);
	ra2port_progress(++ended == 1 ? "game-over" : "game-over-2", why);
	fprintf(stderr, "game-over: %s\n", why);
}

void RB_Map_Zoom_Frame(void);   // glue/zoom_native.inc: in play; a pending zoom applied
void RB_Map_Zoom_End(void);     // the game is over: the whole picture again
bool RB_Main_Loop_Seam(void)
{
	RB_Map_Zoom_Frame();
	bool over = Main_Loop();
	if (over) RB_Map_Zoom_End();
	RB_Checkpoint();
	RB_Frame_Floor();
	RB_Last_Frame_Ns = ra2port_monotonic_ns();
	int frames = ++rb_loop_frames;
	char detail[16];
	snprintf(detail, sizeof(detail), "%d", frames);
	ra2port_progress("frame", detail);
	ra2port_frame(frames);
	RB_Harness_Save(frames);
	RB_Harness_Force_End(frames);
	RB_Harness_Soak(frames);
	RB_Harness_Frame(frames);
	RB_Harness_Input_Frame(frames);
	if (frames >= ra2port_skirmish_frames()) {
		ra2port_progress("frames", detail);
	}
	return over;
}
