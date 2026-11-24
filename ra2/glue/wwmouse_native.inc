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
