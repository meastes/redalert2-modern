// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P8, branch ports-on-rebuild): appended to the generated mainopt unit (fixups.json
// "append"). NOT original code; never part of an exact build.
//
// Main_Options_Dialog_Proc (RA2 0x00542590) is the rebuild's exact text (lane P65: this file carried
// a native draft of it until the rebuild matched it; the draft was cut there by lane P28's row and is
// gone). The port's two additions to it are fixups.json rows on that text: the Enhancements button
// (id 0x7F80, appended to the template by the port, port/src/native_dialogs.cpp) opens the
// Enhancements page below (main's native/link/native_enhancements.cpp, lanes C3/C7, with the
// rebuild's names), and the button's label is set when the dialog is subclassed.
// ---------------------------------------------------------------------------------------------
#include <stdio.h>

extern HWND MainWindow;                   // win.h (not included by this unit)
void RB_Run_Enhancements(HWND options);
void RB_Enhancements_Button_Text(HWND options);

// ---------------------------------------------------------------------------------------------
// NATIVE ADDITION (not in the original game): the Enhancements page, main's native/link/
// native_enhancements.cpp (lanes C3, C7) with the rebuild's names. The port gives the page's
// template under the Options dialog's id while it is created (ra2port_native_page), so the game's
// own dialog code draws it with the Options look; the rows and what they change are the host's
// (platform/sdl/enhancements.cpp, ra2_enh_*).
// ---------------------------------------------------------------------------------------------
extern "C" void ra2port_native_page(int on);          // port/src/native_dialogs.cpp
extern "C" int ra2_enh_rows(void);
extern "C" const char *ra2_enh_label(int);
extern "C" const char *ra2_enh_help(int);
extern "C" int ra2_enh_automatic(int);
extern "C" int ra2_enh_value_count(int);
extern "C" const char *ra2_enh_value_name(int, int);
extern "C" int ra2_enh_value_index(int);
extern "C" void ra2_enh_set_value(int, int);
extern "C" const char *ra2_enh_preset(void);
extern "C" void ra2_enh_begin(void);
extern "C" void ra2_enh_set_preset(int);
extern "C" void ra2_enh_accept(void);
namespace OwnerDraw { bool __fastcall WS_Save_Dialog_Values(HWND dialog); }   // RA2 0x005E5FC0: the column slides out

namespace {
const int kEnhRows = 11;            // rows the template has (port/src/native_dialogs.cpp)
const int kEnhLabel = 0x7F20, kEnhDropdown = 0x7F10, kEnhPresetText = 0x7F40;
const int kEnhOriginal = 0x7F00, kEnhEnhanced = 0x7F01, kEnhOk = 0x7F02, kEnhCancel = 0x7F03;
const int kEnhHint = 0x695, kEnhTitle = 0x694, kEnhHeading = 0x7F7E, kEnhRowHelp = 0x7F70;
int enh_hint_row = -1;            // the row the hint line describes (the last one changed; -1: the page)

void Enh_Set_Wide(HWND dialog, int id, wchar_t const *text)
{
	HWND control = GetDlgItem(dialog, id);
	if (control) SendMessage(control, OD_SETTEXTW, 0, (LPARAM)text);
}
void Enh_Set_Text(HWND dialog, int id, char const *text)
{
	wchar_t wide[256];
	int n = 0;
	for (; text && text[n] && n < 255; ++n) wide[n] = (unsigned char)text[n];
	wide[n] = 0;
	Enh_Set_Wide(dialog, id, wide);
}
int Enh_Count() { return ra2_enh_rows() < kEnhRows ? ra2_enh_rows() : kEnhRows; }
void Enh_Fill(HWND dialog)
{
	for (int i = 0; i < Enh_Count(); ++i) {
		HWND combo = GetDlgItem(dialog, kEnhDropdown + i);
		if (!combo) continue;
		for (int k = 0; k < ra2_enh_value_count(i); ++k) SendMessage(combo, OD_ADDSTRING, 0, (LPARAM)ra2_enh_value_name(i, k));
		EnableWindow(combo, !ra2_enh_automatic(i));
	}
}
void Enh_Refresh(HWND dialog)
{
	for (int i = 0; i < kEnhRows; ++i) {
		if (i >= Enh_Count()) {
			ShowWindow(GetDlgItem(dialog, kEnhLabel + i), SW_HIDE);
			ShowWindow(GetDlgItem(dialog, kEnhDropdown + i), SW_HIDE);
			continue;
		}
		Enh_Set_Text(dialog, kEnhLabel + i, ra2_enh_label(i));
		HWND combo = GetDlgItem(dialog, kEnhDropdown + i);
		if (combo) SendMessage(combo, CB_SETCURSEL, ra2_enh_value_index(i), 0);
	}
	Enh_Set_Text(dialog, kEnhPresetText, ra2_enh_preset());
	Enh_Set_Text(dialog, kEnhHint, ra2_enh_help(-1));
	Enh_Set_Text(dialog, kEnhRowHelp, enh_hint_row >= 0 ? ra2_enh_help(enh_hint_row) : "");
}
BOOL CALLBACK RB_Enhancements_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int handled = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (handled) return handled;
	int *result = (int *)ra2port_get_dialog_user(window);
	switch (message) {
		case OD_SUBCLASSED:
			Enh_Set_Text(window, kEnhTitle, "Enhancements");
			Enh_Set_Text(window, kEnhHeading, "Native additions (not in the original game)");
			Enh_Set_Wide(window, kEnhOk, StringTable::LoadString("GUI:Ok", NULL, __FILE__, __LINE__));
			Enh_Set_Wide(window, kEnhCancel, StringTable::LoadString("GUI:Cancel", NULL, __FILE__, __LINE__));
			Enh_Set_Text(window, kEnhOriginal, "Original");
			Enh_Set_Text(window, kEnhEnhanced, "Enhanced");
			Enh_Fill(window);
			Enh_Refresh(window);
			fprintf(stderr, "enhancements: page shown (%d rows)\n", Enh_Count());
			return 0;
		case WM_COMMAND: {
			int id = LOWORD(wparam);
			int code = HIWORD(wparam);
			if (id >= kEnhDropdown && id < kEnhDropdown + Enh_Count()) {
				if (code != CBN_SELCHANGE) return 0;
				enh_hint_row = id - kEnhDropdown;
				ra2_enh_set_value(enh_hint_row, (int)SendMessage(GetDlgItem(window, id), CB_GETCURSEL, 0, 0));
				Enh_Refresh(window);
				return 0;
			}
			if (code != 0) return 0;
			if (id == kEnhOriginal || id == kEnhEnhanced) {
				ra2_enh_set_preset(id == kEnhEnhanced);
				enh_hint_row = -1;
				Enh_Refresh(window);
			} else if (id == kEnhOk) {
				ra2_enh_accept();
				if (result) *result = 1;
			} else if (id == kEnhCancel) {
				if (result) *result = 0;
			}
			return 0;
		}
	}
	return 0;
}
}  // namespace

// The page over the Options dialog, as the Options dialog's runner runs its own pages: the Options
// column slides out (the original's in-place transition, the skirmish map dialog's Random Map button)
// and is hidden, the page runs to Ok or Cancel, the Options dialog is shown again (its next paint
// slides its column back in).
void RB_Run_Enhancements(HWND options)
{
	int result = -1;
	enh_hint_row = -1;
	ra2_enh_begin();
	if (options) {
		bool slid = OwnerDraw::WS_Save_Dialog_Values(options);
		fprintf(stderr, "enhancements: the Options column %s\n", slid ? "slid out" : "did not slide out");
		ShowWindow(options, SW_HIDE);
	}
	ra2port_native_page(1);
	HWND dialog = OwnerDraw::Begin_Dialog(IDD_OPT_MAIN, (DLGPROC)RB_Enhancements_Proc);
	ra2port_native_page(0);
	if (!dialog) {
		if (options) ShowWindow(options, SW_SHOW);
		return;
	}
	ra2port_set_dialog_user(dialog, &result);
	OwnerDraw::Display_Dialog(dialog);
	while (result < 0) {
		if (OwnerDraw::Dialog_Message_Handler() == true) break;
		Title_Screen_Restore();
	}
	OwnerDraw::End_Dialog(dialog);
	if (options) {
		ShowWindow(options, SW_SHOW);
		InvalidateRect(options, NULL, FALSE);
	}
	fprintf(stderr, "enhancements: page closed (%s)\n", result == 1 ? "accept" : "cancel");
}

void RB_Enhancements_Button_Text(HWND options)
{
	Enh_Set_Text(options, 0x7F80, "Enhancements");
}
