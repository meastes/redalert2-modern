// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P3, branch ports-on-rebuild): appended to the generated savever unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// SaveVersionInfo's constructor: at this pin the unit's head defines the constructor and
// Set_Version/Get_Version under saveload.h's SavegameInformation (the same layout: the two
// declarations of one class, a finding for the rebuild), so SaveVersionInfo's own constructor,
// which Save_Game, Load_Game and the load dialog's Read_File make, has no definition. The same body
// under SaveVersionInfo's member names.
// ---------------------------------------------------------------------------------------------
SaveVersionInfo::SaveVersionInfo(void)
{
	InternalVersion = 0;
	Version = 0;
	CampaignNumber = -1;
	ScenarioNumber = 0;
	GameType = 0;
	ScenarioDescription[0] = 0;
	PlayerHouse[0] = 0;
	UnknownString[0] = 0;
	PlayerName[0] = 0;
	ExecutableName[0] = 0;
	StartTime.dwLowDateTime = 0;
	StartTime.dwHighDateTime = 0;
	PlayTime.dwLowDateTime = 0;
	PlayTime.dwHighDateTime = 0;
	LastSaveTime.dwLowDateTime = 0;
	LastSaveTime.dwHighDateTime = 0;
}
