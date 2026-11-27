// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE STAND-INS (lane N3, 2026-11-19), appended to the planmgr unit: planning mode's functions
// unmatched in ra2-decomp. They were link-closure traps: the command bar's PlanningMode button with units
// selected stopped the game at once (FUN_0063B550, the first frame's draw), and a planned order or a commit
// would have reached the rest. Each is written from gamemd.exe's instructions (yr/tools/yr_dis.py)
// in the tree's names, with the matched helpers it calls or inlines, on the shape of Red Alert 2's drafts of
// the twins (src/recovered/planmgr.cpp: Planning_Execute 0x00614BA0, Planning_Add_Order 0x00614EC0,
// Planning_Draw_Techno 0x006182F0, Planning_Draw_Node 0x00619420 / 0x00619870) and of the tree's matched
// filled twin Draw_Filled_Unit_Waypoints (0x0063BE60). Behaviour code for the native build only.
// ---------------------------------------------------------------------------------------------

// The planning node at a coordinate (any id when id is -1; any house when house is -1), as FUN_00639210
// does but tested with Lead_At, as gamemd.exe's inlined loops in 0x00637E00 and 0x00638120 test it.
static PlanningNodeClass * Planning_Node_At(int house, int id, Coord const & coord)
{
	int count = PlanningNodeClass::Unknown1.Count();
	for (int i = 0; i < count; i++) {
		PlanningNodeClass * node = PlanningNodeClass::Unknown1[i];
		if (node->Lead_At(coord) && (id == -1 || id == node->Id)) {
			int owner = FUN_006339B0(node->Member_Owner(0));
			if (house == -1 || house == owner) {
				return node;
			}
		}
	}
	return NULL;
}

// A techno's planning token, made and listed when it has none (inlined in 0x00637E00 and 0x00638120).
static PlanningTokenClass * Planning_Token_Of(TechnoClass * techno)
{
	PlanningTokenClass * token = techno->FUN_00705D20();
	if (token == NULL) {
		token = new PlanningTokenClass(techno);
		PlanningTokenClass::Array.Add(token);
		techno->FUN_00705D10(token);
	}
	return token;
}
