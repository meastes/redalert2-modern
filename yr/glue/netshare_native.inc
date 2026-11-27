// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane YW, LAN), appended to the netshare unit: LBSaveSelections, YR
// 0x005E5F10 (793 bytes, unmatched in ra2-decomp), the counterpart of LBRestoreSelections above:
// before a network list box is refilled, the selected rows' texts go into the dictionary (every
// selected row of a multiple-selection list, else the current row unless the list has none), from
// gamemd.exe's code. Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane YW, LAN): FUN_005EC9F0, YR 0x005EC9F0 (2,168 bytes, unmatched in
// ra2-decomp), the host screen's "put this entry in this slot" (the RA2 tree's Unknown_005cbf20,
// RA2 0x005CBF20, src/recovered/netshare.cpp), from gamemd.exe's code: a computer is refused (its
// slot's combo box set back, inline as gamemd.exe has it) while an observer is seated or the
// observer mode is on, and an observer boots every computer; then the slot changes, a player
// replaced by a fixed entry is rejected and the slot names sent (mode 1 and 2), the side, colour,
// start and team combo boxes follow an observer or a computer arriving or leaving (YR's five AI
// slot arrays; a computer's team is the game mode's default, vt_entry_30), the slot's controls are
// refreshed, and the computer count is kept, with mode 2 sending the lobby state.
// Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
static bool Yr_Slot_Is_Computer(NodeNameType * node)
{
	return node != NULL && (node == DAT_00AC11A4 || node == DAT_00AC11A8 || node == DAT_00AC11AC);
}

static bool Yr_Slot_Is_Fixed(NodeNameType * node)
{
	return node == NULL || node == DAT_00AC119C || node == DAT_00AC11A0 || node == DAT_00AC11B0 || node == DAT_00AC11B4
		|| Yr_Slot_Is_Computer(node);
}

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane YW, LAN): FUN_005EA3E0, YR 0x005EA3E0 (1,277 bytes, unmatched in
// ra2-decomp), the game-setup screens' refresh of the slots' controls from the session (the host's
// and the guest's MPlayer dialogs call it), from gamemd.exe's code. full: the slot names (FUN_005EB060),
// each slot's country picture and country, colour (the player's own first), start and team combo
// boxes, the slot icons (FUN_005EA2A0's loop), the Internet game's extra and the game mode's own
// controls (vt_entry_78). Otherwise the game mode's controls and the icons, then each part only when
// its summary changed since the last call: the slot names (DAT_00AC0D44 against FUN_005E8DC0's new
// text), the countries ("%d," per slot, DAT_00AC0B10), the colours, starts and teams (FUN_005EAB30,
// FUN_005EAC00, FUN_005EACD0 into their buffers). Behaviour code for the native build.
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane YW2, LAN): DecodePubGameopt, YR 0x005E3D10 (8,702 bytes, unmatched in
// ra2-decomp), from gamemd.exe's code; the RA2 tree's twin is Decode_Pub_Gameopt_005c3ff0 (RA2
// 0x005C3FF0, src/recovered/netshare.cpp, a parked draft) and TibSun's netshare.cpp DecodePubGameopt
// the shape. A public options string from another player of the network game, by its first letter:
//   W  the random map's size and start points (Scen's Width .. HouseHomeCells), the setup screen redrawn;
//   O  the observer slot moved: the slot's side, colour and combo boxes follow (DAT_008316A4);
//   A  a player's accept state (Net2SetAccept);
//   K  a joiner's map state (K0 no map, K1 has it, K2 needs a transfer: FUN_005E39C0), host only;
//   G  (Westwood Online's game screen only) the host asks to start;
//   L  the slot names (FUN_005E8EA0);
//   anything else: the host's full options (FUN_005DBB60's encoding): seed, speeds, money, units, the
//   flags, the mode, difficulty, the map (name, players, official, length, file, digest), each player's
//   name, country, colour, start and team, then the five AI slot arrays. A map not present is
//   reported (K0/K2 back to the host), changes clear this player's accept, and the screen is redrawn.
// Returns whether the map changed. Westwood Online's parts are left out, as Westwood Online is not
// built (the Internet game, Session.Type GAME_INTERNET, is never set natively): the NAT state Z, the
// pings P, the co-op campaign C (its game mode's slots take the score blocks as 32-bit words), the
// chat-channel users (g_pChat), the Internet players' addresses in the full options and the
// channel's game screen (dialog 0xC9) in L.
// Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------
extern int DAT_00AC1178;
extern int Frame;
int __fastcall FUN_004E37D0(int side);
int __fastcall FUN_004E41D0(int side);
void __fastcall FUN_004E3A00(HWND dialog, int id);
void __fastcall FUN_004E3B00(HWND dialog, int id);
void __fastcall FUN_004E45A0(HWND dialog, int id);
void __fastcall FUN_004E4770(HWND dialog, int id);
void __fastcall FUN_004E49A0(HWND dialog, int id, int wanted);
void __fastcall FUN_004E4FC0(int count);
void __fastcall FUN_004E5310(HWND dialog);
void __fastcall FUN_004E5D60(HWND dialog);
void __fastcall Net2SetAccept(char * who, int status);
int __fastcall Net2SetHouseAndColor(char * who, int house, int color, int start, int team);
void Net2DisplayUsers(void);
void FUN_005E2340(void);
WideWstring __fastcall FUN_007B83E0(Wstring const & text);

// The options last decoded (gamemd.exe 0x008316EC .. 0x008316F8 and 0x00AC11C3 .. 0x00AC11C7).
static int Yr_Last_Game_Speed = -1;
static int Yr_Last_Credits = -1;
static int Yr_Last_Unit_Count = -1;
static int Yr_Last_Camp_Difficulty = -1;
static bool Yr_Last_Short_Game;
static bool Yr_Last_SW_Allowed;
static bool Yr_Last_Build_Off_Ally;
static bool Yr_Last_MCV_Redeploy;
static bool Yr_Last_Goodies;

// ---------------------------------------------------------------------------------------------
// NATIVE (lane N6), appended to the netshare unit: the Choose Map dialog's procedure, gamemd.exe
// 0x005E6920 (the pinned tree's FUN_005E6920, which it declares and never defines). Text adopted from
// a later ra2-decomp (src/netshare.cpp Scenario_DlgProc and its inline helpers, commit 5a94489,
// GPL-3.0-or-later, THIRD_PARTY.md), with the pinned tree's names: 0x00608070 is its
// WS_Save_Dialog_Values (the later tree's FUN_00608070, the dialog's slide-out), 0x005E74E0 its
// Update_Network_Dialog_Preview, OwnerDraw::Paint its OwnerDraw::Draw_Dialog_Back. Not compared with
// gamemd.exe byte for byte here. Behaviour code for the native build.
//
// It was a link-closure trap: Choose Map in a Yuri's Revenge skirmish (control 0x5AA, skirmish.cpp ->
// Scenario_Dialog) stopped the game as the dialog was created.
// ---------------------------------------------------------------------------------------------
#include "tiprequest.h"

extern bool DAT_00AC11C8;      // the game mode list is being filled (its selection changes are not the user's)
extern bool DAT_00AC11C9;      // the mode's refusal was shown once
extern int DAT_008316FC;       // the game mode row last chosen
bool WS_Save_Dialog_Values(HWND dialog);
bool __fastcall RunOpenAnimationIfNeeded(HWND dialog);
void __fastcall FUN_0060AAA0(HWND dialog);
void __fastcall FUN_005D6130(HWND list, int id);
