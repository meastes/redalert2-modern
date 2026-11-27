// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFTS (lane Y3 partial, written out by lane N23): planning mode's mouse
// handling, unmatched in the YR tree. Written from gamemd.exe's instructions (yr_dis.py) in the
// shape of Red Alert 2's twins (src/recovered/planmgr.cpp: Planning_Mouse_Move_00617900 and
// Planning_Left_Down_00617340). gamemd.exe inlines the hovered node's hit test
// (PlanningNodeClass::Hit_Test, the lead's 8x8 box grown by 4) and the node search
// (Find_Node_At over PlanningNodeClass::Unknown1), and shows each refusal once
// (DAT_00AC4C08) with the scold sound, as the tree's other planning refusals.
// Behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------
