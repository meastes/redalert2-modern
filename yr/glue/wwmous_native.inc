// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// Native glue (lane Y2), appended to the wwmous unit: WinMain (yr/glue/startup_winmain.inc)
// makes the mouse here, where gamemd.exe's WWMouseClass (wwmous.h) is the one in scope.
// 0x006BDEF9: new WWMouseClass(VisibleSurface, MainWindow), then its Hide_Mouse (slot 3).
// ---------------------------------------------------------------------------------------------
struct VirtualObject;
