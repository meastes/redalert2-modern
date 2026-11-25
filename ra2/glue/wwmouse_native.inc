// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P8, branch ports-on-rebuild): appended to the generated wwmouse unit (fixups.json "append").
// NOT original code. The mouse mutex (RA2 0x00B2AF60, this unit's static) for DSurface's primary-lock guard (lane C23,
// glue/dsurface fixups).
// ---------------------------------------------------------------------------------------------
HANDLE RB_Mouse_Mutex(void)
{
	return MouseMutex;
}

// Lane P21 (the step server, glue/stepserver_native.inc): the mouse processed now from the game's thread (under the
// mouse mutex, as the mouse thread does), so the game's pointer is where the frame's scripted input put it.
void RB_Process_Mouse_Now(void)
{
	if (_MousePtr != NULL) _MousePtr->Process_Mouse();
}
