// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P8, branch ports-on-rebuild): appended to the generated ownrdraw unit (fixups.json "append").
// NOT original code; never part of an exact build.
//
// The touch app's on-screen keyboard (main's native/link/native_text_entry.cpp, lane C14): the host raises the keyboard
// while text can be typed: a Win32 edit control has the focus (port/src/user32.cpp), or Westwood's "NewEdit" control
// (New_Edit_Proc, RA2 0x005F22A0: the skirmish and lobby player-name fields), asked here by its window class.
// ---------------------------------------------------------------------------------------------
extern "C" int ra2_native_new_edit_focused(void)
{
	HWND focus = GetFocus();
	if (!focus || !IsWindowVisible(focus)) return 0;
	char name[32];
	if (!GetClassNameA(focus, name, sizeof(name))) return 0;
	return strcmp(name, "NewEdit") == 0;
}

// ---------------------------------------------------------------------------------------------
// Mac widescreen (main's native/link/native_widescreen.cpp, lane Y8, user report 2026-11-16; not original behaviour): the
// shell's idle step (Title_Screen_Restore, every menu loop's; fixups.json) first asks the host whether the window's aspect
// wants another game mode (host.cpp ra2_host_widescreen_target; off unless the host turns widescreen on: the Mac app,
// RA2_NATIVE_WIDESCREEN=1 in tests) and changes it as the Options dialog does (Change_Display_Mode). The statics' movies
// (the main menu's animated backdrop) draw on the surfaces the change makes again, so they are stopped first and started
// again after (main's Native_Static_Movies_Stop/Restart): each static's movie name is recorded when it starts (the static's
// WM_USER+228, fixups.json) and dropped when it stops or the static is destroyed (the port has no IsWindow).
// ---------------------------------------------------------------------------------------------
#include "options.h"
extern "C" int ra2_host_widescreen_target(int width, int height, int * w, int * h);
bool Change_Display_Mode(int width, int height);   // mainopt.cpp

namespace {
struct RB_Static_Movie { HWND window; char name[64]; };
RB_Static_Movie rb_static_movies[16];
}

void RB_Static_Movie_Name(HWND window, char const * name)
{
	int free_slot = -1;
	for (int i = 0; i < 16; i++) {
		if (rb_static_movies[i].window == window) {
			if (name == NULL) { rb_static_movies[i].window = NULL; return; }
			snprintf(rb_static_movies[i].name, sizeof(rb_static_movies[i].name), "%s", name);
			return;
		}
		if (free_slot < 0 && rb_static_movies[i].window == NULL) free_slot = i;
	}
	if (name == NULL || free_slot < 0) return;
	rb_static_movies[free_slot].window = window;
	snprintf(rb_static_movies[free_slot].name, sizeof(rb_static_movies[free_slot].name), "%s", name);
}

void RB_Widescreen_Check(void)
{
	int w = 0, h = 0;
	if (!ra2_host_widescreen_target(VideoModeWidth, VideoModeHeight, &w, &h)) return;
	fprintf(stderr, "widescreen: %dx%d -> %dx%d for the window\n", VideoModeWidth, VideoModeHeight, w, h);
	RB_Static_Movie playing[16];
	int count = 0;
	for (int i = 0; i < 16; i++) {
		if (rb_static_movies[i].window != NULL) playing[count++] = rb_static_movies[i];
	}
	for (int k = 0; k < count; k++) SendMessage(playing[k].window, WM_USER + 226, 0, 0);
	bool changed = Change_Display_Mode(w, h);
	for (int j = 0; j < count; j++) SendMessage(playing[j].window, WM_USER + 228, 0, (LPARAM)playing[j].name);
	if (!changed) {
		fprintf(stderr, "widescreen: Change_Display_Mode %dx%d failed\n", w, h);
		return;
	}
	LogicalSurface = HiddenSurface;
	Options.ScreenWidth = w;
	Options.ScreenHeight = h;
	if ((w != 640 && w != 800 && w != 1024) || h > 768) Options.AllowHiResModes = true;
}
