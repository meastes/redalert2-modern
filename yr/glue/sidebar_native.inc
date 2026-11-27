// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y3), appended to the sidebar unit: SidebarClass::Reposition_Sidebar,
// YR 0x006ABD30 (1,235 bytes, vtable slot 34, unmatched in ra2-decomp). From the RA2 tree's twin
// (src/recovered/sidebar.cpp, 0x006819A0) with gamemd.exe's changes read from its code: the
// sidebar's x and the button anchors come from FUN_006A5130(false) (Init_IO's layout helper)
// instead of being computed here; the background shapes are SIDEGDI1-3 loaded through
// FileSystem::LoadFile; the button anchors are this unit's DAT_00B0B4DC..DAT_00B0B510; the
// select-button help regions 1000..1059 are all removed before the visible ones are added again;
// the sell help region is the Power button's (ID 102), the 0xC8/0xC9 regions are Waypoint's and
// Upgrade's. Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
extern int DAT_00886F90;                          // 0x00886F90 SidebarRect.X
extern int DAT_00B0B4F4;                          // the select buttons' column x
extern int DAT_00B0B508;                          // Upgrade's position
extern int DAT_00B0B50C;
extern int DAT_00B0B510;                          // Waypoint's offset from Upgrade

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFTS (lane Y3): SidebarClass::AI (YR 0x006A7780, vtable slot AI) and
// SidebarClass::StripClass::AI (YR 0x006A8B30), both unmatched in ra2-decomp. Written from
// gamemd.exe's code with the RA2 tree's SidebarClass::AI (src/recovered/sidebar.cpp, 0x0067DC90)
// and TibSun's StripClass::AI (TibSun archive code/sidebar.cpp; GPL-3.0-or-later, EA's GPL
// section 7 terms; THIRD_PARTY.md) as the shape. The tab switch is this unit's SetTab, the
// scroll buttons are StripClass::Page (gamemd.exe inlines both); the observer's player-list
// refresh at the end of StripClass::AI (PlayerPtr == DAT_00AC1198, YR 0x006A9038..0x006A9251)
// is not written. Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
extern int DAT_00B0B514;     // the strip's scroll step in pixels per frame (TibSun SCROLL_RATE)
void __fastcall FUN_00734250(BuildingClass * building);   // uicmnds.cpp: a finished building's placement

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y3): SidebarClass::StripClass::Draw_It (YR 0x006A9540, 4,280
// bytes) is unmatched in ra2-decomp. The text is the RA2 tree's exact counterpart
// (src/recovered/sidebar.cpp, SidebarClass::StripClass::Unknown_0067F800, RA2 0x0067F800) with
// the YR tree's names and gamemd.exe's differences: the cameos' palette is FileSystem::CAMEO_PAL
// (0x0087F6B0), the clock GCLOCK2.SHP, the hover frame and the texts take the tooltip colour
// (DAT_00B0FA1C), the queue text is L"%d" through Fancy_Text_Print_Wide. The observer's player
// list (PlayerPtr == DAT_00AC1198, YR 0x006AA05B..0x006AA59B) is not written. Behaviour code for
// the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
extern ConvertClass * DAT_0087F6B0;        // FileSystem::CAMEO_PAL, the cameos' drawer
extern RGBClass DAT_00B0FA1C;              // the tooltip/tab text colour (txtlabel2.cpp)
void __fastcall FUN_006AC480(wchar_t const * text, Point2D const & point, Rect const & window, int width);
void __fastcall FUN_00621B80(Rect * rect, Surface * surface, unsigned short color, int alpha);

// ---------------------------------------------------------------------------------------------
// SidebarClass::StripClass::SelectClass::Action, YR 0x006AAD00 (3,207 bytes, the build icons' click:
// start, hold, cancel or place production, fire a super weapon). Lane Y7: adopted from ra2-decomp
// (gibbo101/ra2-decomp, GPL-3.0-or-later, commit 0bafad686c6ac1f7cf29777cb291d6002ebbcfd7, src/sidebar.cpp,
// pull request #298, where it matches gamemd.exe exactly), newer than the pinned tree, which lacks it
// (a link-closure trap: the first tap on a sidebar icon ended the game). Behaviour code for the
// native build; the names it uses that the pinned tree spells differently are noted where changed.
// ---------------------------------------------------------------------------------------------
bool Yr_Queue_Five_Add(EventClass const &event);	// NATIVE (lane Y8): Queue five with Shift, glue/yr_enhancements.cpp
// (the two PRODUCE adds below go through it; with the setting off it is OutList.Add)

