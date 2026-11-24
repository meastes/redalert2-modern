// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P8, branch ports-on-rebuild): appended to the generated display unit (fixups.json "append").
// NOT original code; never part of an exact build. Main's native/link/native_map_zoom.cpp (lanes C4, C16, N31) and
// the interface-size half of native_enhancements.cpp (lanes C3, C7) under the rebuild's names; the hooks are
// fixups.json rows at the same sites as main's port/fixups.json rows (each row names main's site).
//
// Map zoom: the game's screen keeps its size; only the tactical view is made smaller. DisplayClass::Set_View_Dimensions
// keeps the view rectangle RB_Map_View answers (the tactical rectangle divided by the zoom, at the same corner); the host
// (platform/sdl/host.cpp, the zoom layout) shows it over the whole tactical area. The sites that take the view's right
// edge as the sidebar's x ask RB_Sidebar_X (the tactical rectangle's while zoomed). Zoom gestures: the wheel, a trackpad
// or two-finger pinch change the zoom in play (ra2_native_zoom_by); with the setting on the game's screen is made twice
// the window's in its map column (RB_Render_Size), so the map can be shown down to 0.5x, and the in-play dialogs are laid
// out at the window's size (RB_Dialog_Screen). Game interface size: the sidebar surface may be shorter than the screen so
// the host can show the sidebar column larger (RB_Sidebar_Height). At 1x / off (the defaults, the Original preset) every
// answer is the original's.
// ---------------------------------------------------------------------------------------------
#include <stdio.h>

extern "C" double ra2_enh_map_zoom(void);
extern "C" void ra2_enh_map_view(int, int, int, int, int, int, int, int);
extern "C" void ra2_enh_in_play(int);
extern "C" void ra2_enh_map_show(double, double, double, double);
extern "C" int ra2_enh_zoom_gestures(void);
extern "C" void ra2_enh_set_render_scale(int);
extern "C" void ra2_enh_set_dialog_screen(int);
extern "C" int ra2_enh_sidebar_height(int screen_height);
bool Allocate_Surfaces(const Rect & hidden_rect, const Rect & composite_rect, const Rect & tile_rect, const Rect & sidebar_rect, bool hidden_first);   // init.h
extern bool InterfaceArtLoaded;                                  // txtlabel2.cpp, RA2 0x00AC119C
extern bool SidebarArtLoaded;                                    // txtlabel2.cpp, RA2 0x00AC11B0
void __fastcall Free_Sidebar_Rects(void);                        // txtlabel2.cpp, RA2 0x006F5400
void __fastcall Layout_Sidebar_Art(int width, int height);       // txtlabel2.cpp, RA2 0x006F4C90
void RB_Hd_Vehicles_Start(void);                                // glue/unit/hd_vehicles.cpp: HD vehicles taken with the zoom

namespace {
double game_zoom = 1.0;      // the zoom of the game running (taken at its start or load)
double view_zoom = 1.0;      // the zoom the view rectangle was last made with
bool zoomed = false;         // the view is smaller than the tactical rectangle
int full_right = 0;          // the tactical rectangle's right edge (the sidebar's x)
int full_bottom = 0;         // the tactical rectangle's bottom edge (the command bar's top)
const double kZoomMin = 0.5, kZoomMax = 3.0;
int render_scale = 1;
double zoom_min() { return kZoomMin * render_scale < 1.0 ? 1.0 : kZoomMin * render_scale; }
double show_ox = 0, show_oy = 0, show_w = 0, show_h = 0;
double pending_factor = 1.0, pending_ax = 0, pending_ay = 0;
bool pending = false;
int round_up(double v, int step) { int n = (int)v; if (n < v) ++n; return (n + step - 1) / step * step; }
void report_show(Rect const & view)
{
	ra2_enh_map_show(view.X + show_ox, view.Y + show_oy, show_w, show_h);
}
}

// Set_View_Dimensions' rectangle: the tactical rectangle divided by the game's zoom, rounded up to whole cells with a pixel to
// spare (never larger than the tactical rectangle), at the same corner. Unchanged at 1x.
Rect RB_Map_View(Rect const & rect)
{
	Rect view = rect;
	show_w = rect.Width;
	show_h = rect.Height;
	if (game_zoom > 1.0 && rect.Width > 0 && rect.Height > 0) {
		show_w = rect.Width / game_zoom;
		show_h = rect.Height / game_zoom;
		int w = round_up(show_w + 1, 60), h = round_up(show_h + 1, 30);
		view.Width = w < rect.Width ? w : rect.Width;
		view.Height = h < rect.Height ? h : rect.Height;
	}
	if (show_ox + show_w > view.Width) show_ox = view.Width - show_w;
	if (show_oy + show_h > view.Height) show_oy = view.Height - show_h;
	if (show_ox < 0) show_ox = 0;
	if (show_oy < 0) show_oy = 0;
	view_zoom = game_zoom;
	zoomed = view.Width != rect.Width || view.Height != rect.Height;
	full_right = rect.X + rect.Width;
	full_bottom = rect.Y + rect.Height;
	ra2_enh_map_view(view.X, view.Y, view.Width, view.Height, rect.X, rect.Y, rect.Width, rect.Height);
	report_show(view);
	return view;
}

namespace {
// The view's origin (TacPixelX/Y, the world pixel at its top left) read fresh, and set to a given origin through the position.
Point2D view_origin(Tactical * t)
{
	t->Set_Tactical_Position(t->Get_Tactical_Position());
	return Point2D(t->TacPixelX, t->TacPixelY);
}
Point2D set_view_origin(Tactical * t, int x, int y)
{
	Point2D now = view_origin(t), centre = t->Get_Tactical_Position();
	t->Set_Tactical_Position(Point2D(x + centre.X - now.X, y + centre.Y - now.Y));
	return Point2D(t->TacPixelX, t->TacPixelY);
}
void apply_zoom()
{
	pending = false;
	Tactical * tactical = TacticalMap;
	double z0 = game_zoom, z1 = z0 * pending_factor;
	if (z1 < zoom_min()) z1 = zoom_min();
	if (z1 > kZoomMax * render_scale) z1 = kZoomMax * render_scale;
	if (!tactical || z1 == z0) return;
	Rect full = Get_Title_Screen_Area();
	double ax = pending_ax < 0 ? 0 : pending_ax > full.Width ? full.Width : pending_ax;
	double ay = pending_ay < 0 ? 0 : pending_ay > full.Height ? full.Height : pending_ay;
	Point2D origin0 = view_origin(tactical);
	Rect const view0 = TacticalRect;
	double wx = origin0.X + show_ox + ax / z0;
	double wy = origin0.Y + show_oy + ay / z0;
	game_zoom = z1;
	view_zoom = z1;
	Rect view1 = RB_Map_View(full);
	if (view1.Width != view0.Width || view1.Height != view0.Height) {
		Map.Set_View_Dimensions(full);
	}
	Rect const & view = TacticalRect;
	double ox = wx - ax / z1, oy = wy - ay / z1;
	int tx = (int)ox, ty = (int)oy;
	if (tx > ox) --tx;
	if (ty > oy) --ty;
	Point2D placed = set_view_origin(tactical, tx, ty);
	show_ox = ox - placed.X;
	show_oy = oy - placed.Y;
	if (show_ox < 0) show_ox = 0;
	if (show_oy < 0) show_oy = 0;
	if (show_ox + show_w > view.Width) show_ox = view.Width - show_w;
	if (show_oy + show_h > view.Height) show_oy = view.Height - show_h;
	if (z1 == 1.0) show_ox = show_oy = 0;
	report_show(view);
	fprintf(stderr, "zoom: %.3f -> %.3f at %.0f,%.0f: view %dx%d, shown %.2fx%.2f from %.2f,%.2f, origin %d,%d\n", z0, z1, ax, ay,
		view.Width, view.Height, show_w, show_h, show_ox, show_oy, placed.X, placed.Y);
	Map.Flag_To_Redraw(GS_REDRAW_TACTICAL);
}
}

// The host's zoom (the wheel, a trackpad pinch, a two-finger pinch): multiplied by factor about (ax, ay), game pixels across
// the tactical rectangle from its corner; taken at the frame's end. Not while the game ignores the player's input (a mission's
// locked intro: N31).
extern "C" void ra2_native_zoom_by(double factor, double ax, double ay)
{
	if (IgnoreInput) {
		static int last = -1000;
		if (Frame - last > 30 || Frame < last) fprintf(stderr, "zoom: input locked (frame %d): the pinch is ignored\n", (int)Frame);
		last = Frame;
		return;
	}
	pending_factor = pending ? pending_factor * factor : factor;
	pending_ax = ax;
	pending_ay = ay;
	pending = true;
}
extern "C" double ra2_native_zoom(void) { return game_zoom; }

// The sidebar's x where the original takes the view's right edge: the tactical rectangle's right edge while zoomed.
int RB_Sidebar_X(int view_right)
{
	return zoomed ? full_right : view_right;
}

// Game interface size (main's Native_Sidebar_Height, lane C3): the sidebar surface's rows for a screen this tall.
int RB_Sidebar_Height(int screen_height)
{
	return ra2_enh_sidebar_height(screen_height);
}

// SidebarClass::Recalc_Layout takes the view's bottom edge as the command bar's top: the tactical rectangle's while zoomed,
// less the rows the sidebar surface is shorter than the screen (Game interface size; at 1.5x the scroll buttons had sat
// below the surface: main's codex4 R703/R705/R706 heap damage).
int RB_Sidebar_Bottom(int view_bottom)
{
	int bottom = zoomed ? full_bottom : view_bottom;
	int screen = VisibleRect.Height;
	return bottom - (screen - RB_Sidebar_Height(screen));
}

// The command bar's top row in the sidebar art's layout (lane N31): the screen's bottom, whatever height the layout is given.
int RB_Command_Bar_Top(void)
{
	return VideoModeHeight - 32;
}

// CCToolTip::Update, after it placed a tip: a tip for the map side outside the zoomed view is moved into it at the same place
// on screen.
void RB_Tooltip_Zoom(bool map_side, int * x, int * y, int width, int height)
{
	if (!zoomed || !map_side) return;
	Rect const & view = TacticalRect;
	if (*x >= view.X && *y >= view.Y && *x + width <= view.X + view.Width && *y + height <= view.Y + view.Height) return;
	int nx = view.X + (int)((*x - view.X) / view_zoom);
	int ny = view.Y + (int)((*y - view.Y) / view_zoom);
	if (nx > view.X + view.Width - width) nx = view.X + view.Width - width;
	if (ny > view.Y + view.Height - height) ny = view.Y + view.Height - height;
	if (nx < view.X) nx = view.X;
	if (ny < view.Y) ny = view.Y;
	*x = nx;
	*y = ny;
}

// A game is loaded (Get_All, before its Set_View_Dimensions): it takes the zoom set now.
void RB_Map_Zoom_Load(void)
{
	ra2_enh_in_play(0);
	game_zoom = ra2_enh_map_zoom() * render_scale;
	RB_Hd_Vehicles_Start();
}

static bool view_asked = false;
static Point2D view_asked_at;
void RB_View_Asked(int x, int y)
{
	view_asked = true;
	view_asked_at = Point2D(x, y);
}

// A new game in the same process (main's Native_Sidebar_New_Game, lane C7): when the interface size changed since the
// sidebar surface was made, the surfaces are made again as Get_All makes them, before the scenario is read.
static void RB_Sidebar_New_Game(void)
{
	if (SidebarSurface == NULL) return;
	int height = RB_Sidebar_Height(VisibleRect.Height);
	if (SidebarSurface->Get_Height() == height) return;
	fprintf(stderr, "enhancements: new game: sidebar surface %d rows, was %d\n", height, SidebarSurface->Get_Height());
	Rect temp = Get_Title_Screen_Area();
	Rect sidebar(0, 0, SidebarWidth, height);
	Rect tile(0, 0, temp.Width, VisibleRect.Height);
	Rect composite(0, 0, temp.Width, VisibleRect.Height);
	Allocate_Surfaces(VisibleRect, composite, tile, sidebar, false);
	LogicalSurface = HiddenSurface;
}

// A scenario starts (Start_Scenario's top): the loading screen and movies are shown whole until the first frame.
void RB_Map_Zoom_Leave(void)
{
	view_asked = false;
	ra2_enh_in_play(0);
	RB_Sidebar_New_Game();
}

// The scenario has started (after Start_Scenario's display mode toggle): the zoom set now, the view made for it.
void RB_Map_Zoom_Start(void)
{
	game_zoom = ra2_enh_map_zoom() * render_scale;
	RB_Hd_Vehicles_Start();
	if (game_zoom != view_zoom) {
		Rect tactical = Get_Title_Screen_Area();
		Tactical * t = TacticalMap;
		bool asked = view_asked;
		Point2D asked_at = view_asked_at;
		double cx = 0, cy = 0;
		if (t) {
			Point2D o = view_origin(t);
			cx = asked ? asked_at.X : o.X + TacticalRect.Width / 2.0;
			cy = asked ? asked_at.Y : o.Y + TacticalRect.Height / 2.0;
		}
		Map.Set_View_Dimensions(tactical);
		if (t) {
			show_ox = show_oy = 0;
			set_view_origin(t, (int)(cx - show_w / 2), (int)(cy - show_h / 2));
			report_show(TacticalRect);
		}
		Map.Flag_To_Redraw(GS_REDRAW_ALL);
	}
}

// Each Main_Loop frame (the seam): the game is in play.
void RB_Map_Zoom_Frame(void)
{
	ra2_enh_in_play(1);
	if (pending) apply_zoom();
}

// The game is over: the score screen, the next movies and the menus are shown whole; a zoomed game's last frame is cleared.
void RB_Map_Zoom_End(void)
{
	ra2_enh_in_play(0);
	if (view_zoom != 1.0) {
		if (VisibleSurface) VisibleSurface->Fill(0);
		if (HiddenSurface) HiddenSurface->Fill(0);
		if (CompositeSurface) CompositeSurface->Fill(0);
	}
}

// The screen size (WinMain after reading RA2.INI, Change_Display_Mode at its top): with Zoom gestures on, a game screen of
// 800x600 or more is made render scale (2) times larger in its map column and above the command bar.
static int nominal_width = 0;
void RB_Render_Size(int * w, int * h)
{
	int scale = ra2_enh_zoom_gestures() && *w >= 800 && *h >= 600 ? 2 : 1;
	render_scale = scale;
	ra2_enh_set_render_scale(scale);
	nominal_width = *w;
	if (scale == 1) return;
	int nw = *w, nh = *h;
	*w = (nw - 168) * scale + 168;
	*h = (nh - 32) * scale + 32;
	fprintf(stderr, "zoom: render scale %d: the game's screen %dx%d for %dx%d\n", scale, *w, *h, nw, nh);
}

// Layout_Sidebar_Art's command bar (lane N31): the x of the bar's right end, the shown part's at render scale 2.
int RB_Command_Bar_Right(int width, int column_x)
{
	if (render_scale <= 1 || nominal_width <= 168) return column_x;
	if (width != (nominal_width - 168) * render_scale + 168) return column_x;
	return column_x - (width - nominal_width);
}

// The in-play dialogs at the window's size (Special_Dialog around its dialog loop).
static void relayout_art(int width, int height, int sidebar_height)
{
	if (InterfaceArtLoaded) Relayout_Interface_Art(width, height);
	if (SidebarArtLoaded) {
		Free_Sidebar_Rects();
		Layout_Sidebar_Art(width, sidebar_height);
	}
}
void RB_Dialog_Screen(bool on)
{
	static int saved_w = 0, saved_h = 0;
	static bool set = false, scaled = false;
	if (on && !set) {
		saved_w = VideoModeWidth;
		saved_h = VideoModeHeight;
		scaled = render_scale > 1;
		if (!scaled && RB_Sidebar_Height(saved_h) == saved_h) return;
		set = true;
		if (scaled) {
			VideoModeWidth = (saved_w - 168) / render_scale + 168;
			VideoModeHeight = (saved_h - 32) / render_scale + 32;
		}
		relayout_art(VideoModeWidth, VideoModeHeight, VideoModeHeight);
		if (scaled) ra2_enh_set_dialog_screen(1);
		fprintf(stderr, "zoom: in-play dialogs at %dx%d (the screen %dx%d)\n", VideoModeWidth, VideoModeHeight, saved_w, saved_h);
	} else if (!on && set) {
		VideoModeWidth = saved_w;
		VideoModeHeight = saved_h;
		set = false;
		relayout_art(saved_w, saved_h, RB_Sidebar_Height(saved_h));
		if (scaled) ra2_enh_set_dialog_screen(0);
	}
}
