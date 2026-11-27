// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFTS (lane Y3), appended to the owner-draw unit after ownrdraw_native.inc:
// the combo box and its drop list. Behaviour code for the native build; never part of an exact
// build.
//
// OwnerDraw::ComboBoxHandler, YR 0x00617250 (6,895 bytes, unmatched in ra2-decomp) and
// FUN_0060D540, YR 0x0060D540 (7,636 bytes, the "ComboDropWin" class procedure; unmatched).
// The combo is the RA2 tree's behaviour rewrite of RA2 0x005F4BA0 (native/link/
// behaviour_m14_ownerdraw.cpp, which runs RA2's skirmish and LAN set-up), its messages checked
// against gamemd.exe's (the same narrow/wide add, find and select cases, the arrow click, the
// drop list, 0x497/0x498 item colours at +0x110, the colour-box flag +0xCC, the alternate art
// +0xCD, +0xD0); the drop list is the RA2 tree's matched counterpart unknown_combo_drop_proc_5ead20
// (src/recovered/ownrdraw.cpp) with gamemd.exe's globals: the combo owner DAT_00AC48D0, the open
// drop list DAT_00AC48C0 / its dialog DAT_00AC48C4, DefaultBorderColor, AltBorderColor,
// ListSelectionFillColor, AltSelectionFillColor, AltComboTextColor, ControlInsetPx.
// NATIVE: the items keep their wide text in a record set as the item data (gamemd.exe keeps them in
// the record's nodes), and the open drop list is kept by combo in a side table, as the RA2 tree's
// rewrite does; the port's USER32 combo keeps the narrow text, the selection and the count.
namespace OwnerDraw {
extern int DefaultBorderColor;
extern int AltBorderColor;
extern int ListSelectionFillColor;
extern int AltSelectionFillColor;
extern int AltComboTextColor;
extern int ControlInsetPx;
extern int AltDisabledTextColor;
void __fastcall CopyDimmedBackground(Rect const & rect, HWND hWnd, int alpha);
void DrawBeveledBorder(Surface & surface, Rect const & rect, int offset, int color);
}
void ODDrawArrowBitmap(Surface * surface, Rect const & rect, BOOL upward, BOOL pressed, bool dark);
int ODColorToHiColor(COLORREF color);
void __fastcall FUN_0072A9E0(Rect *rect, int amount);
extern HWND DAT_00AC48D0;                         // 0x00AC48D0 the combo whose list is down
LRESULT CALLBACK FUN_0060D540(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

namespace {
LRESULT YR_Original(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	WNDPROC proc = NULL;
	OwnerDraw::DialogProcs.getValue(window, proc);
	return CallWindowProc(proc, window, message, wparam, lparam);
}

struct YRComboItem {
	int Reserved;
	int Data;
	wchar_t *Text;
	YRComboItem *Next;
};
YRComboItem *YRComboItems = NULL;
struct { HWND Combo; HWND Drop; } YRComboDrops[16];

YRComboItem *YR_Combo_Item(HWND window, int index)
{
	YRComboItem *item = (YRComboItem *)YR_Original(window, CB_GETITEMDATA, index, 0);
	for (YRComboItem *known = YRComboItems; known != NULL; known = known->Next) {
		if (known == item) {
			return item;
		}
	}
	return NULL;
}

void YR_Combo_Free(HWND window, int index)
{
	YRComboItem *item = YR_Combo_Item(window, index);
	if (item == NULL) {
		return;
	}
	for (YRComboItem **link = &YRComboItems; *link != NULL; link = &(*link)->Next) {
		if (*link == item) {
			*link = item->Next;
			break;
		}
	}
	delete [] item->Text;
	delete item;
}

HWND &YR_Combo_Drop(HWND combo, bool make)
{
	static HWND none;
	for (int index = 0; index < 16; index++) {
		if (YRComboDrops[index].Combo == combo && YRComboDrops[index].Drop != NULL) {
			return YRComboDrops[index].Drop;
		}
	}
	if (make) {
		for (int index = 0; index < 16; index++) {
			if (YRComboDrops[index].Drop == NULL) {
				YRComboDrops[index].Combo = combo;
				return YRComboDrops[index].Drop;
			}
		}
	}
	none = NULL;
	return none;
}

// An item's wide text (the record's, else the narrow text of the port's list widened).
bool YR_Combo_Text(HWND window, int index, wchar_t *out, int size)
{
	out[0] = L'\0';
	YRComboItem *item = YR_Combo_Item(window, index);
	if (item != NULL && item->Text != NULL) {
		int n = 0;
		for (; item->Text[n] && n < size - 1; n++) {
			out[n] = item->Text[n];
		}
		out[n] = L'\0';
		return true;
	}
	char narrow[256];
	LRESULT length = YR_Original(window, CB_GETLBTEXTLEN, index, 0);
	if (length < 0 || length >= (LRESULT)sizeof(narrow) || YR_Original(window, CB_GETLBTEXT, index, (LPARAM)narrow) < 0) {
		return false;
	}
	int n = 0;
	for (; narrow[n] && n < size - 1; n++) {
		out[n] = (unsigned char)narrow[n];
	}
	out[n] = L'\0';
	return true;
}
}

LRESULT CALLBACK OwnerDraw::ComboBoxHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	WinData *data = Find_Data(window);
	if (data != NULL) {
		switch (message) {
		case 0x4BB:
		case 0x4BC:
		case 0x4C1:
		case 0x4C2: {
			bool wide = message >= 0x4C1;
			bool add = message == 0x4BC || message == 0x4C2;
			int length = 0;
			char narrow[512];
			if (lparam) {
				if (wide) {
					while (((wchar_t const *)lparam)[length]) {
						length++;
					}
				} else {
					while (((char const *)lparam)[length]) {
						length++;
					}
				}
			}
			if (wide) {
				WideToCharString(narrow, (wchar_t const *)lparam, sizeof(narrow) - 1);
			} else {
				int n = length < (int)sizeof(narrow) - 1 ? length : (int)sizeof(narrow) - 1;
				memcpy(narrow, (char const *)lparam, n);
				narrow[n] = '\0';
			}
			LRESULT index = YR_Original(window, add ? CB_ADDSTRING : CB_INSERTSTRING, add ? 0 : wparam, (LPARAM)narrow);
			if (index < 0) {
				return index;
			}
			YRComboItem *item = new YRComboItem;
			item->Reserved = 0;
			item->Data = 0;
			item->Text = new wchar_t[length + 1];
			for (int i = 0; i < length; i++) {
				item->Text[i] = wide ? ((wchar_t const *)lparam)[i] : (wchar_t)(unsigned char)((char const *)lparam)[i];
			}
			item->Text[length] = L'\0';
			item->Next = YRComboItems;
			YRComboItems = item;
			YR_Original(window, CB_SETITEMDATA, index, (LPARAM)item);
			return index;
		}
		case 0x4B8:
		case 0x4B9:
		case 0x4BA:
		case 0x4BE:
		case 0x4BF:
		case 0x4C0: {
			bool wide = message >= 0x4BE;
			bool exact = message == 0x4B9 || message == 0x4BF;
			bool select = message == 0x4BA || message == 0x4C0;
			wchar_t want[256];
			int wlen = 0;
			if (lparam) {
				for (; wlen < 255; wlen++) {
					wchar_t c = wide ? ((wchar_t const *)lparam)[wlen] : (wchar_t)(unsigned char)((char const *)lparam)[wlen];
					if (c == 0) {
						break;
					}
					want[wlen] = c;
				}
			}
			want[wlen] = L'\0';
			int count = (int)YR_Original(window, CB_GETCOUNT, 0, 0);
			if (count <= 0) {
				return -1;
			}
			int start = ((int)wparam + 1 < 0 || (int)wparam + 1 >= count) ? 0 : (int)wparam + 1;
			for (int n = 0; n < count; n++) {
				int i = (start + n) % count;
				wchar_t text[256];
				if (!YR_Combo_Text(window, i, text, 256)) {
					continue;
				}
				bool match = exact ? _wcsicmp(text, want) == 0 : _wcsnicmp(text, want, wlen) == 0;
				if (!match) {
					continue;
				}
				if (select) {
					YR_Original(window, CB_SETCURSEL, i, 0);
					InvalidateRect(window, NULL, TRUE);
				}
				return i;
			}
			return -1;
		}
		case 0x4B3:
		case 0x4B5: {
			if (!lparam || (int)wparam <= 0) {
				return 0;
			}
			int selected = (int)YR_Original(window, CB_GETCURSEL, 0, 0);
			wchar_t text[256];
			text[0] = L'\0';
			if (selected >= 0) {
				YR_Combo_Text(window, selected, text, 256);
			}
			int n = 0;
			if (message == 0x4B3) {
				wchar_t *out = (wchar_t *)lparam;
				for (; text[n] && n < (int)wparam - 1; n++) {
					out[n] = text[n];
				}
				out[n] = L'\0';
			} else {
				char *out = (char *)lparam;
				for (; text[n] && n < (int)wparam - 1; n++) {
					out[n] = (char)text[n];
				}
				out[n] = '\0';
			}
			return n;
		}
		case CB_GETITEMDATA:
		case LB_GETITEMDATA: {
			YRComboItem *item = YR_Combo_Item(window, (int)wparam);
			if (item != NULL) {
				return item->Data;
			}
			return YR_Original(window, CB_GETITEMDATA, wparam, lparam);
		}
		case CB_SETITEMDATA:
		case LB_SETITEMDATA: {
			YRComboItem *item = YR_Combo_Item(window, (int)wparam);
			if (item != NULL) {
				item->Data = (int)lparam;
				return 0;
			}
			return YR_Original(window, CB_SETITEMDATA, wparam, lparam);
		}
		case CB_DELETESTRING:
			YR_Combo_Free(window, (int)wparam);
			break;
		case CB_RESETCONTENT: {
			int count = (int)YR_Original(window, CB_GETCOUNT, 0, 0);
			for (int i = 0; i < count; i++) {
				YR_Combo_Free(window, i);
			}
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK: {
			VocClass::PlayGlobal(Rule->GUIComboOpenSound, 1.0f, 0x2000);
			RECT client;
			GetClientRect(window, &client);
			if ((int)(lparam & 0xFFFF) > client.right - 20) {
				LRESULT dropped = SendMessage(window, CB_GETDROPPEDSTATE, 0, 0);
				PostMessage(window, CB_SHOWDROPDOWN, dropped != 1, 0);
			}
			return 0;
		}
		case CB_GETDROPPEDSTATE:
			return YR_Combo_Drop(window, false) != NULL ? 1 : 0;
		case CB_SHOWDROPDOWN: {
			HWND parent = GetParent(window);
			HWND &slot = YR_Combo_Drop(window, false);
			if (!wparam) {
				if (slot == NULL) {
					return 1;
				}
				HWND drop = slot;
				slot = NULL;
				ReleaseCapture();
				SendMessage(parent, 0x4A9, (WPARAM)drop, 0);
				DestroyWindow(drop);
				ODRemoveFromDict(drop, 0);
				YR_Original(window, CB_SHOWDROPDOWN, 0, 0);
				InvalidateRect(parent, NULL, FALSE);
				return 1;
			}
			if (slot != NULL) {
				return 1;
			}
			SetFocus(window);
			RECT client;
			RECT display;
			RECT parent_display;
			GetClientRect(window, &client);
			Get_Display_Rect(window, &display);
			Get_Display_Rect(parent, &parent_display);
			int count = (int)YR_Original(window, CB_GETCOUNT, 0, 0);
			int item_height = (int)YR_Original(window, CB_GETITEMHEIGHT, 0, 0);
			if (item_height < 1) {
				item_height = 1;
			}
			int height = count * item_height + 4;
			if (display.bottom + height > parent_display.bottom - 2 * item_height) {
				height = parent_display.bottom - (2 * item_height + 4) - display.bottom;
				if (height < item_height) {
					height = parent_display.bottom - display.bottom;
					height -= height % item_height;
				}
			}
			HWND drop = CreateWindowEx(0, "ComboDropWin", NULL, WS_CHILD, display.left - parent_display.left,
				client.bottom + display.top - parent_display.top + 2, client.right, height, parent, NULL, ProgramInstance, window);
			if (drop == NULL) {
				return 0;
			}
			OwnerDraw::InitializeCtrl(drop, 0);
			YR_Combo_Drop(window, true) = drop;
			YR_Original(window, CB_SHOWDROPDOWN, 1, 0);
			SendMessage(drop, 0x7E8, 0, 0);
			SendMessage(parent, 0x4A9, (WPARAM)drop, 1);
			SetCapture(drop);
			ShowWindow(drop, SW_SHOWNORMAL);
			InvalidateRect(drop, NULL, FALSE);
			InvalidateRect(parent, NULL, FALSE);
			return 1;
		}
		case 0x497:
			memset(&YR_REC(int, data, 0x110), 0xFF, 50 * sizeof(int));
			break;
		case 0x498:
			if (wparam < 50) {
				(&YR_REC(int, data, 0x110))[wparam] = (int)lparam;
			}
			break;
		case WM_DESTROY:
			SendMessage(window, CB_SHOWDROPDOWN, 0, 0);
			break;
		case 0x4DD:
			YR_REC(bool, data, 0xCC) = lparam == 1;
			break;
		case 0x4DE:
			YR_REC(int, data, 0xD0) = (int)lparam;
			break;
		case 0x4F1:
			YR_REC(bool, data, 0xCD) = lparam == 1;
			break;
		case 0x4DA:
		case 0x4DB:
			return SendMessage(GetParent(window), message, wparam, lparam);
		}
	}
	if (data == NULL || message != WM_PAINT) {
		if (message == WM_ERASEBKGND) {
			return 1;
		}
		LRESULT result = YR_Original(window, message, wparam, lparam);
		if (data != NULL && (message == CB_SETCURSEL || message == CB_RESETCONTENT)) {
			InvalidateRect(window, NULL, TRUE);
		}
		return result;
	}
	if (!data->skipDraw) {
		RECT where;
		Get_Display_Rect(window, &where);
		Rect area(where.left, where.top, where.right - where.left, where.bottom - where.top);
		Surface *surface = AlternateSurface;
		LONG style = GetWindowLong(window, GWL_STYLE);
		bool disabled = (style & WS_DISABLED) != 0;
		bool dropped = YR_Combo_Drop(window, false) != NULL;
		CopyDimmedBackground(area, window, YR_REC(int, data, 0xC8));
		FUN_00621B80(&area, surface, 0, YR_REC(int, data, 0xC8));
		DrawBeveledBorder(*surface, area, 1, -1);
		Rect arrow(area.X + area.Width - 20, area.Y + 1, 19, area.Height - 2);
		ODDrawArrowBitmap(surface, arrow, dropped, dropped, YR_REC(bool, data, 0xCD));
		if (disabled) {
			FUN_00621B80(&area, surface, 0, YR_REC(int, data, 0xC8));
		}
		if ((style & 3) == CBS_DROPDOWNLIST) {
			int selected = (int)YR_Original(window, CB_GETCURSEL, 0, 0);
			wchar_t text[256];
			text[0] = L'\0';
			if (selected >= 0) {
				YR_Combo_Text(window, selected, text, 256);
			}
			unsigned color = disabled ? AltDisabledTextColor : PrimaryTextColor;
			int *colors = &YR_REC(int, data, 0x110);
			if (YR_REC(bool, data, 0xCC) && selected >= 0 && selected < 50 && colors[selected] >= 0) {
				unsigned fill = colors[selected];
				Rect swatch(area.X, area.Y, area.Width - 20, area.Height);
				FUN_0072A9E0(&swatch, 2);
				surface->Fill_Rect(swatch, DSurface::Build_Hicolor_Pixel(fill & 0xFF, (fill >> 8) & 0xFF, (fill >> 16) & 0xFF));
				color = fill;
			}
			// gamemd.exe 0x00617B42-0x00617C04 (lane P15): the text is cut a character at a time, "..." put after the
			// cut, while it is wider than the client width less the arrow's 20 (0x00617B4E), and drawn in the
			// display rectangle from its left + 2 to its right edge (0x00617BC6), the arrow's width included; a
			// box ending at the arrow wrapped the Start column's "???" onto two lines.
			RECT box = {area.X + 2, area.Y, area.X + area.Width, area.Y + area.Height};
			RECT client;
			GetClientRect(window, &client);
			int room = client.right - 20;
			BitFont *font = data->Font;
			int length = wcslen(text);
			if (font != NULL) {
				int width = font->FUN_00433ED0(text, 0);
				while (length > 0 && width > room) {
					text[--length] = L'\0';
					if (length + 3 < 256) {
						wcscat(text, L"...");
					}
					width = font->FUN_00433ED0(text, 0);
				}
			}
			if (text[0]) {
				DrawWideText(surface, text, (LTRBStruct *)&box, font, color, 4, 0xC, 0, 0, 0);
			}
		}
	}
	ValidateRect(window, NULL);
	return 0;
}

// OwnerDraw::NewEditHandler, YR 0x00614B30 (2,215 bytes, unmatched in ra2-decomp; RA2 counterpart
// 0x005F22A0): the "NewEdit" control (a list box standing in for an edit). The RA2 tree's
// behaviour rewrite (native/link/behaviour_m14_ownerdraw.cpp) with the YR record names: the text
// is Wide.editText, its limit Wide.textLimit, the caret Wide.caretIndex, the scroll
// Wide.scrollStart, the blink state Wide.caretBlink (published in DAT_00AC48C8 for the drawer
// FUN_00623880, as gamemd.exe does); Return and Tab notify the dialog (0x4DA, 0x4DB; a chat line's
// Return also sends notification 0x501, lane YW2).
void __fastcall FUN_00623880(Surface *surface, Rect *rect, wchar_t *text, int caret, BitFont *font, unsigned color,
	int *scroll, int focus, int password, int p10, int p11);
extern int DAT_00AC48C8;
