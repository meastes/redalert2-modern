// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P3, branch ports-on-rebuild): appended to the generated loaddlg unit
// (native/rb/fixups.json "append"); LoadOptionsClass::Load asks it first. Not original behaviour.
//
// Resume at start (main's lane I, native/link/behaviour_m15.cpp): when the host asked for a saved
// game to be loaded (--load-save, ra2port::request_resume), the port's menus take Single Player and
// Load Saved Game; the load dialog then loads that file at once (Load_File, the dialog's own load
// step) instead of showing the list. A failed load shows the dialog as usual.
// ---------------------------------------------------------------------------------------------
extern "C" const char * ra2port_take_resume(void);   // port/src/port_core.cpp

int RB_Resume_Load(LoadOptionsClass & options)
{
	char const * file = ra2port_take_resume();
	if (file == NULL) return -1;
	if (options.Load_File(file)) {
		fprintf(stderr, "resume: loaded %s\n", file);
		return 1;
	}
	fprintf(stderr, "resume: could not load %s\n", file);
	return -1;
}
