// SPDX-License-Identifier: GPL-3.0-or-later
// Storage for Westwood Online globals the YR units reference (lane P64): since ra2-decomp ac45ee9e the
// lobby, the session, the frame loop and the score code read them (netshare.cpp, session.cpp, queue.cpp,
// stats.cpp, conquer.cpp), but wonline.cpp, which declares most of them, is left out natively
// (yr/fixups.json skip_unit) and their gamemd.exe addresses are not in yr/data/globals.tsv. Without a
// definition the link closure made each a trap function, so a write (the LAN lobby's colour change,
// the reconnect dialog's ping counters) would fault on a code page. Natively Session.Type is never
// GAME_INTERNET, so they keep the zero gamemd.exe starts them with. Sizes cover each use; the C++ types
// do not enter the symbol names. Behaviour code for the native build.
int IsColorChangePending;                // netshare.cpp DecodePubGameopt (0x005E3D10)
int g_GameSKU;
int g_GameStarted;
int g_MaxPlayers;
int g_PingsSent;
int g_PingsReceived;
int g_GameServerPort;
char g_GameServerHost[256];
alignas(8) char g_NickName[64];          // a Wstring in the tree (zero: no name)
alignas(8) char g_LoginPassword[64];     // a Wstring in the tree
int WestwoodOnline_GameID;
int WestwoodOnline_StartTime;
int WestwoodOnline_GameSKU_TS;
int WestwoodOnline_PingTimes[16];
char WestwoodOnline_Clan1_Players[256];
char WestwoodOnline_Clan2_Players[256];
char WestwoodOnline_LoginName[64];
char WestwoodOnline_UserName[64];
