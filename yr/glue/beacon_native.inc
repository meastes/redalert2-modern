// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE STAND-INS (lane N3, 2026-11-19), appended to the beacon unit: functions unmatched in
// ra2-decomp that were link-closure traps on the command bar's Beacon button (TabClass::AI -> FUN_00731A30:
// beacon mode; the map click -> DisplayClass::Mouse_Left_Release -> PlaceBeacon; every frame after,
// BeaconClass::Draw -> Drawing::Intersect). Behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------

#include "stringtable.h"
#include "vox.h"
#include "rules.h"
#include "audevent.h"

// Drawing::Intersect (gamemd.exe's clip of a draw rectangle to bounds; the callers pass null offsets): the
// tree's own template Intersect (rect.h, TibSun's text) with the same arguments.
Rect __fastcall Drawing::Intersect(Rect const & a, Rect const & b, int * x, int * y)
{
	return(::Intersect(a, b, x, y));
}

// BeaconManagerClass::PlaceBeacon (gamemd.exe 0x00430BA0, 909 bytes): written from gamemd.exe's
// instructions (yr/tools/yr_dis.py) on the shape of Red Alert 2's accepted twin (game.exe 0x0042E370,
// src/recovered/beacon.cpp), with YR's names and the inlined BeaconClass::SetCoordAndHouse and
// VisibleToPlayer called: a beacon of the house in the slot (-1: the first free one; none free: nothing).
// In a network game the player's own placement shows the help lines, plays EVA_BeaconPlaced and the
// placement sound and sends packet 0x20 to the other players; an allied one raises radar event 11 and
// EVA_BeaconDetected.
#pragma pack(push, 1)
struct BeaconPlacePacket {
	int Command;
	char Name[43];
	int X;
	int Y;
	int Z;
	unsigned char ID;
	unsigned char House;
	char Rest[456 - 0x3d];
};
#pragma pack(pop)
