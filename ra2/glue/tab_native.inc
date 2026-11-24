// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P8, branch ports-on-rebuild): appended to the generated tab unit (fixups.json "append").
// NOT original code; never part of an exact build.
//
// The touch host's question (platform/sdl/host.cpp ra2_native_team_button_at; main's native/link/team_button_hook.cpp,
// lane Y15): which command-bar team button (1-3) is under a game pixel, else 0. A hold there sets the team
// (platform/sdl/touch.cpp): the bar's own buttons, TabClass::Get_Button for the Team01..Team03 commands, while the bar is
// open (IsThumbActive).
// ---------------------------------------------------------------------------------------------
extern "C" int ra2_native_team_button_at(int x, int y)
{
	if (!Map.IsThumbActive) return 0;
	int const commands[3] = { TabClass::Team01Command, TabClass::Team02Command, TabClass::Team03Command };
	for (int i = 0; i < 3; i++) {
		if (commands[i] < 0) continue;
		ShapeButtonClass * button = TabClass::Get_Button(commands[i]);
		if (button && button->Is_Enabled() && x >= button->X && x < button->X + button->Width &&
			y >= button->Y && y < button->Y + button->Height) {
			return TabClass::Team_Number(commands[i]);
		}
	}
	return 0;
}
