// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P8, branch ports-on-rebuild): appended to the generated mainopt unit (fixups.json
// "append"). NOT original code; never part of an exact build.
//
// Main_Options_Dialog_Proc, the shell Options dialog's procedure (template 0xD5, RA2 0x00542590).
// The rebuild keeps its text under `#if 0` (unmatched), so the main menu's Options aborted on the
// link closure's trap. This is that text compiled natively, checked against main's behaviour row
// (native/link/behaviour_m22_options.cpp Main_Options_Proc, written from the disassembly): the list
// box's strings go in through the owner-draw OD_ADDSTRING (0x4BC, main's disassembly) rather than
// windowsx's CB_ADDSTRING, and the mode list is EnumDisplayModes' `new char[]` block (width/height
// pairs ending in 0), read as ints, as the rebuild's own loop reads it. NATIVE: the Enhancements
// button (id 0x7F80, appended to the template by the port, port/src/native_dialogs.cpp) opens the
// Enhancements page below (main's native/link/native_enhancements.cpp, lanes C3/C7, with the
// rebuild's names).
// ---------------------------------------------------------------------------------------------
#include <stdio.h>

extern HWND MainWindow;                   // win.h (not included by this unit)
void RB_Run_Enhancements(HWND options);
void RB_Enhancements_Button_Text(HWND options);

static int * ResolutionModes = NULL;     // RA2 0x00A6ED10: EnumDisplayModes' width/height pairs

static void RB_Free_Modes(void)
{
	delete [] (char *)ResolutionModes;
	ResolutionModes = NULL;
}

BOOL CALLBACK Main_Options_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (rc != 0) {
		return(rc);
	}

	int * result = (int *)ra2port_get_dialog_user(window);
	HWND control = (HWND)lparam;
	switch (message) {
		case WM_COMMAND:
			switch (LOWORD(wparam)) {
				case IDC_OPTMAIN_NETWORK:
				case IDC_OPTMAIN_KEYBOARD:
					RB_Free_Modes();
					*result = LOWORD(wparam);
					break;

				case IDC_OPTMAIN_OK:
					if (HIWORD(wparam) == 0) {
						RB_Free_Modes();
						*result = IDC_OPTMAIN_MAINMENU;
					}
					break;

				case 0x7F80:	// NATIVE: ENHANCEMENTS
					if (HIWORD(wparam) == 0) {
						RB_Run_Enhancements(window);
					}
					break;

				case IDC_OPTMAIN_RESOLUTION:
					if (HIWORD(wparam) == CBN_SELCHANGE) {
						Center_Window_Within_Window(window, MainWindow);
						HWND combo = GetDlgItem(window, IDC_OPTMAIN_RESOLUTION);
						if (combo == NULL || ResolutionModes == NULL) {
							return(0);
						}
						int item = (int)SendMessage(combo, CB_GETCURSEL, 0, 0);
						int mode = (int)SendMessage(combo, CB_GETITEMDATA, item, 0);
						TempOptions.ScreenWidth = ResolutionModes[mode * 2];
						TempOptions.ScreenHeight = ResolutionModes[mode * 2 + 1];
						if (TempOptions.ScreenWidth == Options.ScreenWidth && TempOptions.ScreenHeight == Options.ScreenHeight) {
							return(0);
						}
						if (Mode_Prompt_Dialog()) {
							if (Debug_IngameModeChange) {
								Options.ScreenWidth = TempOptions.ScreenWidth;
								Options.ScreenHeight = TempOptions.ScreenHeight;
								return(0);
							}
							if (Test_Display_Mode_Dialog(TempOptions.ScreenWidth, TempOptions.ScreenHeight)) {
								Options.ScreenWidth = TempOptions.ScreenWidth;
								Options.ScreenHeight = TempOptions.ScreenHeight;
								Relayout_Interface_Art(TempOptions.ScreenWidth, TempOptions.ScreenHeight);
								Darken_Surface(NULL, true);
								return(0);
							}
						}
						SendMessage(combo, CB_SETCURSEL, InitialResolution, 0);
						return(0);
					}
					break;
			}
			return(0);

		case WM_DESTROY:
			if (ResolutionModes) {
				RB_Free_Modes();
			}
			return(0);

		case WM_HSCROLL: {
			if (LOWORD(wparam) != TB_THUMBTRACK) {
				return(0);
			}
			int position = HIWORD(wparam);
			char const * name;
			HWND text;
			if (control == GetDlgItem(window, IDC_DETAIL_LEVEL_SLIDER)) {
				name = DetailLabels[position ? 2 : 0];
				text = GetDlgItem(window, IDC_DETAIL_LEVEL_LABEL);
			} else if (control == GetDlgItem(window, IDC_DIFFICULTY_SLIDER)) {
				name = DifficultyLabels[position];
				text = GetDlgItem(window, IDC_DIFFICULTY_LABEL);
			} else if (control == GetDlgItem(window, IDC_SCROLL_SPEED_SLIDER)) {
				name = ScrollLabels[position];
				text = GetDlgItem(window, IDC_SCROLL_SPEED_LABEL);
			} else {
				if (!OptionsReady) {
					return(0);
				}
				if (control == GetDlgItem(window, IDC_MUSIC_VOLUME)) {
					Options.Set_Score_Volume((float)(SendMessage(control, TBM_GETPOS, 0, 0) * 0.1), true);
				} else if (control == GetDlgItem(window, IDC_SOUND_VOLUME)) {
					Options.Set_Sound_Volume((float)(SendMessage(control, TBM_GETPOS, 0, 0) * 0.1), true);
				} else if (control == GetDlgItem(window, IDC_VOICE_VOLUME)) {
					Options.Set_Voice_Volume((float)(SendMessage(control, TBM_GETPOS, 0, 0) * 0.1), true);
				}
				return(0);
			}
			if (text) {
				SendMessage(text, OD_SETTEXTW, 0, (LPARAM)StringTable::LoadString(name, NULL, "D:\\Ra2\\MainOpt.cpp", 550));
			}
			return(0);
		}

		case OD_SUBCLASSED: {
			RB_Enhancements_Button_Text(window);	// NATIVE: the ENHANCEMENTS button's label
			OptionsReady = false;
			HWND slider = GetDlgItem(window, IDC_DETAIL_LEVEL_SLIDER);
			if (slider) {
				SendMessage(slider, OD_TRACKNUMBERS, 0, 0);
				SendMessage(slider, TBM_SETRANGE, TRUE, MAKELONG(0, 1));
				SendMessage(slider, TBM_SETPOS, TRUE, Options.DetailLevel != 0);
			}

			HWND combo = GetDlgItem(window, IDC_OPTMAIN_RESOLUTION);
			if (Options.AllowHiResModes) {
				ResolutionModes = EnumDisplayModes(640, 480, 4096, 4096, 16);
			} else {
				ResolutionModes = EnumDisplayModes(640, 480, 1024, 768, 16);
			}
			int selected = -1;
			int count = 0;
			for (int index = 0; ResolutionModes != NULL && ResolutionModes[index * 2] != 0; index++) {
				int width = ResolutionModes[index * 2];
				int height = ResolutionModes[index * 2 + 1];
				if (Options.AllowHiResModes || width == 640 || width == 800 || width == 1024) {
					if (width == TempOptions.ScreenWidth && height == TempOptions.ScreenHeight) {
						selected = count;
					}
					char buffer[64];
					sprintf(buffer, "%d x %d x 16", width, height);
					int item = (int)SendMessage(combo, OD_ADDSTRING, 0, (LPARAM)buffer);
					SendMessage(combo, CB_SETITEMDATA, item, index);
					count++;
				}
			}
			SendMessage(combo, CB_SETCURSEL, selected, 0);
			InitialResolution = selected;

			slider = GetDlgItem(window, IDC_DIFFICULTY_SLIDER);
			if (slider) {
				SendMessage(slider, OD_TRACKNUMBERS, 0, 0);
				SendMessage(slider, TBM_SETRANGE, TRUE, MAKELONG(0, 2));
				SendMessage(slider, TBM_SETPOS, TRUE, Options.Difficulty);
			}

			HWND box = GetDlgItem(window, IDC_TARGET_LINES);
			if (box) {
				SendMessage(box, BM_SETCHECK, Options.ActionLines != 0, 0);
			}
			box = GetDlgItem(window, IDC_SHOW_HIDDEN);
			if (box) {
				SendMessage(box, BM_SETCHECK, Options.IsShowHidden != 0, 0);
			}
			box = GetDlgItem(window, IDC_TOOLTIPS);
			if (box) {
				SendMessage(box, BM_SETCHECK, Options.ToolTips != 0, 0);
			}

			slider = GetDlgItem(window, IDC_SCROLL_SPEED_SLIDER);
			if (slider) {
				SendMessage(slider, OD_TRACKNUMBERS, 0, 0);
				SendMessage(slider, TBM_SETRANGE, TRUE, MAKELONG(0, 6));
				SendMessage(slider, TBM_SETPOS, TRUE, 6 - Options.ScrollRate);
			}

			int sound = AudioDeviceAvailable();
			static int const volume_ids[3] = { IDC_MUSIC_VOLUME, IDC_SOUND_VOLUME, IDC_VOICE_VOLUME };
			float const * volumes[3] = { &Options.ScoreVolume, &Options.SoundVolume, &Options.VoiceVolume };
			for (int i = 0; i < 3; i++) {
				slider = GetDlgItem(window, volume_ids[i]);
				if (slider) {
					SendMessage(slider, OD_TRACKSILENT, 0, 0);
					SendMessage(slider, TBM_SETRANGE, TRUE, MAKELONG(0, 10));
					SendMessage(slider, TBM_SETPOS, TRUE, (int)(*volumes[i] * 10.0 + 0.5));
					EnableWindow(slider, sound);
				}
			}
			OptionsReady = true;
			fprintf(stderr, "options: the Options dialog is set up (%d modes)\n", count);
			return(0);
		}
	}
	return(0);
}

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
