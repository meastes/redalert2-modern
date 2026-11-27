// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFTS (lane Y3), appended to the owner-draw unit. Behaviour code for the
// native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------

namespace OwnerDraw {
BOOL CALLBACK InitCompactDialogControlsProc(HWND window, LPARAM lparam);
BOOL CALLBACK ClassifyLayoutBand(HWND window, LPARAM lparam);
BOOL CALLBACK SendTransitionCompleteToCustomTextChildProc(HWND window, LPARAM lparam);
bool __fastcall TrySetDialogLayoutBand1(HWND dialog);
bool __fastcall TrySetDialogLayoutBand2(HWND dialog);
void __fastcall UpdateControlPosition(HWND dialog, LPARAM lparam);
}
void __fastcall Remove_Modeless_Dialog(HWND window);
WideWstring FUN_00603FC0(HWND window, HWND control, WORD low, WORD high);
WideWstring FUN_00603F60(HWND window, HWND control);
WideWstring FUN_00604060(HWND window, HWND control);
extern int DAT_007F5BEC;
namespace OwnerDraw { extern HWND CurrentDialogHwnd; }
namespace Unsorted { extern int WSDialogCount; }

// UI::StandardWndProc, YR 0x00622B50 (1,482 bytes, unmatched in ra2-decomp): the dialog procedure
// every owner-drawn dialog procedure calls first. From gamemd.exe's code (its RA2 counterpart is
// the default dialog procedure 0x005FFB00, native/drafts/005ffb00.cpp): WM_DESTROY drops the
// modeless record and the dialog count and gives the focus back to the main window; WM_PAINT
// paints through OwnerDraw::Paint unless the record skips drawing (+0xBC), then plays a pending
// campaign-menu transition (+0xBE); WM_NCHITTEST feeds the tooltip control (0x695) the text of
// the child under the cursor (0x4E8/0x4E9 requests, else the control's label); WM_DRAWITEM,
// WM_ERASEBKGND, the WM_CTLCOLOR* brushes, 0x497 and 0x4EC as gamemd.exe handles them;
// WM_INITDIALOG prepares, scales, lays out and centres the dialog. The x86 jump table on
// message - 2 (0x006230DC/0x006230F0) is the switch below.
namespace UI {
}

// OwnerDraw::DefaultHandler, YR 0x00610CA0 (7,265 bytes, unmatched in ra2-decomp): the window
// procedure every subclassed owner-draw control gets (RegisterChildControlProc), which calls the
// control's own handler (SubclassProcs) around the shared work. TibSun's CtrlProc (TibSun archive
// code/ownrdraw.cpp) as the RA2 tree's draft of its RA2 counterpart 0x005EE460 carries it
// (native/drafts/005ee460.cpp, behaviour-checked), with gamemd.exe's code read for the YR
// differences: the IME service first, this unit's tables and statics (ProcessedMessages,
// ActiveWindows), End_Tooltip, the parent's status-bar text through the 0x4E8/0x4E9 requests, the
// cleanup copy skipped while the web browser shows, RunOpenAnimationIfNeeded for the slide-in and
// FUN_00610570's teardown on WM_NCDESTROY. Record fields the tree does not name are reached at
// gamemd.exe's offsets: +0x70/+0xA8 (Dialog.reserved70) and the tail +0x1E8..+0x1FC (reserved).
// NATIVE (lane P64): upstream ac45ee9e names every WinData field (src/ownrdraw.cpp); the drafts'
// raw 32-bit offsets are those fields, here by name (natively the pointers before them are 8 bytes).
static inline void *Yr_WinData_Field(OwnerDraw::WinData *d, int off)
{
	switch (off) {
	case 0x70: return &d->StaticDrawMode;
	case 0x7C: return &d->StaticOwnsShape;
	case 0x80: return &d->StaticTextRevealCount;
	case 0x84: return &d->StaticTextRevealDelay;
	case 0x88: return &d->StaticTextRevealStep;
	case 0x8C: return &d->StaticColorAdjust;
	case 0x90: return &d->StaticSoundIndex;
	case 0x94: return &d->StaticFrameCount;
	case 0x98: return &d->StaticCurrentFrame;
	case 0x9C: return &d->StaticLastFrameTick;
	case 0xA0: return &d->StaticFrameDelayMs;
	case 0xA8: return &d->StaticAnimationRunning;
	case 0xC4: return &d->ButtonTimerActive;
	case 0xC5: return &d->ButtonAlternateFrame;
	case 0xC8: return &d->alpha;
	case 0xCC: return &d->UseItemColorOverrides;
	case 0xCD: return &d->disabled;
	case 0xD0: return &d->ComboBoxMaxVisibleDropItems;
	case 0xD5: return &d->HasTopPanelAnimation;
	case 0xD6: return &d->HasButtonAnimation;
	case 0xD7: return &d->HasMainScreenAnimation;
	case 0x110: return &d->ComboBox.itemColors[0];
	case 0x1E8: return &d->font;
	case 0x1EC: return &d->bkMode;
	case 0x1F0: return &d->bkColor;
	case 0x1F4: return &d->textColor;
	case 0x1FC: return &d->animState;
	}
	fprintf(stderr, "yr-native: WinData offset 0x%X has no field\n", off);
	abort();
}
#define YR_REC_TAIL(type, data, off) (*(type *)Yr_WinData_Field((data), (off)))
#define YR_REC_70(data) ((data)->StaticDrawMode)
#define YR_REC_A8(data) ((data)->StaticAnimationRunning)
extern HWND DAT_00AC48C0;                         // 0x00AC48C0 the open combo drop-down
extern HWND DAT_00AC48C4;                         // 0x00AC48C4 its owner dialog
int __fastcall Is_Web_Browser_Visible(void);
bool __fastcall RunOpenAnimationIfNeeded(HWND dialog);
void __fastcall WideToCharString(char *destination, wchar_t const *source, int length);
void __fastcall FUN_0072AA10(Rect *rect, int x, int y);
BOOL CALLBACK ODAddWindowToList(HWND window, ArrayList<HWND> *list);
LRESULT CALLBACK FUN_0060D540(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
namespace OwnerDraw {
int __fastcall ServiceIMEMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
int __fastcall GetIMEResult(void);
void __fastcall CancelIMEComposition(void);
LRESULT CALLBACK OwnerDrawButtonHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK ListBoxHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

LRESULT CALLBACK DefaultHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	static POINT min_update_rect = {0xFFFFFF, 0xFFFFFF};
	static POINT max_update_rect;
	static int num_rect_updates;
	static bool was_in_focus = true;
	static char in_programmatic_reorder;

	if (message == WM_SETCURSOR) {
		return(1);
	}
	if (ServiceIMEMessage(window, message, wparam, lparam)) {
		return(GetIMEResult());
	}

	WNDPROC specific_proc = NULL;
	SubclassProcs.getValue(window, specific_proc);

	if (message == WM_SYSKEYUP && wparam == VK_TAB) {
		SendMessage(MainWindow, WM_SYSKEYUP, VK_TAB, lparam);
	}

	LRESULT result = 0;
	BOOL show_tooltip = FALSE;
	BOOL allow_proc = TRUE;
	int anim_state = 0;

	CtrlMsgData key;
	key.window = window;
	key.message = message;

	RECT display_rect;
	Get_Display_Rect(window, &display_rect);
	RECT window_rect;
	GetWindowRect(window, &window_rect);
	int offset_x = window_rect.left - display_rect.left;
	int offset_y = window_rect.top - display_rect.top;

	HWND owner = NULL;
	WinData *ownerdata = NULL;
	int state = 0;

	if (ActiveWindows.length() != 0) {
		HWND topwindow = NULL;
		ActiveWindows.getTail(topwindow);
		BOOL allow = FALSE;
		BOOL forward = FALSE;
		if (GetParent(window) == NULL) {
			allow = TRUE;
		}
		if (GetWindowLong(window, GWL_ID) <= 0) {
			allow = TRUE;
		}
		for (HWND parent = window; parent != NULL; parent = GetParent(parent)) {
			if (parent == topwindow) {
				allow = TRUE;
				break;
			}
		}
		if (message < WM_MOUSEMOVE || message > WM_MBUTTONDBLCLK) {
			forward = TRUE;
		}
		if (message >= WM_NCMOUSEMOVE && message <= 0xA9) {
			forward = FALSE;
		}
		if (message >= WM_KEYFIRST && message <= WM_KEYLAST) {
			forward = FALSE;
		}
		if (message == WM_SYSKEYUP || message == WM_SYSKEYDOWN || message == WM_SYSCOMMAND || message == WM_SYSCHAR) {
			forward = TRUE;
		}
		if (message == 0x49B || message == WM_TIMER || message == 0x4AD) {
			forward = FALSE;
		}
		if (!allow && !forward) {
			return(0);
		}
	}

	if (ProcessedMessages.contains(key)) {
		if (message != WM_COMMAND && message != WM_SYSKEYDOWN && message != WM_SYSKEYUP
			&& message != WM_SYSCOMMAND && message != WM_SYSCHAR) {
			return(0);
		}
	}
	bool processing = true;
	ProcessedMessages.remove(key);
	ProcessedMessages.add(key, processing);

	RECT client_rect;
	GetClientRect(window, &client_rect);
	RECT disp_rect;
	Get_Display_Rect(window, &disp_rect);

	bool in_focus = GameInFocus;
	bool is_paint = false;
	if (message == WM_PAINT) {
		is_paint = true;
		if (!in_focus) {
			ValidateRect(window, NULL);
			ProcessedMessages.remove(key);
			return(0);
		}
	}
	if (in_focus == true && !was_in_focus) {
		InvalidateRect(MainWindow, NULL, FALSE);
		in_focus = GameInFocus;
	}
	was_in_focus = in_focus;

	RECT update_rect;
	if (is_paint) {
		++num_rect_updates;
		GetUpdateRect(window, &update_rect, FALSE);
		update_rect.left += disp_rect.left;
		update_rect.right += disp_rect.left;
		update_rect.top += disp_rect.top;
		update_rect.bottom += disp_rect.top;
	}

	WinData *data = NULL;
	Dialogs.getPointer(window, &data);

	if (message == 0x4A0) {
		result = (data != NULL && data->attachedWindow != NULL);
		goto cleanup;
	}
	if (data != NULL) {
		if (message == 0x49F) {
			HDC hdc = (HDC)lparam;
			HGDIOBJ old = SelectObject(hdc, GetStockObject(SYSTEM_FONT));
			YR_REC_TAIL(HGDIOBJ, data, 0x1E8) = old;
			SelectObject(hdc, old);
			YR_REC_TAIL(int, data, 0x1EC) = GetBkMode(hdc);
			YR_REC_TAIL(COLORREF, data, 0x1F0) = GetBkColor(hdc);
			YR_REC_TAIL(COLORREF, data, 0x1F4) = GetTextColor(hdc);
			result = 1;
			goto cleanup;
		}
		if (message == 0x49E) {
			HDC hdc = (HDC)lparam;
			SelectObject(hdc, YR_REC_TAIL(HGDIOBJ, data, 0x1E8));
			SetBkMode(hdc, YR_REC_TAIL(int, data, 0x1EC));
			SetBkColor(hdc, YR_REC_TAIL(COLORREF, data, 0x1F0));
			SetTextColor(hdc, YR_REC_TAIL(COLORREF, data, 0x1F4));
			result = 1;
			goto cleanup;
		}
		if (message == 0x49D) {
			HWND linked = data->attachedWindow;
			result = data->paintDisabled;
			data->paintDisabled = lparam;
			if (linked != NULL) {
				WinData *linkeddata = NULL;
				if (Dialogs.getPointer(linked, &linkeddata) && linkeddata != NULL) {
					linkeddata->paintDisabled = lparam;
				}
			}
			goto call_custom_proc;
		}
	}
	if (message == 0x4A9) {
		HWND topwindow = NULL;
		ActiveWindows.getTail(topwindow);
		result = (LRESULT)topwindow;
		HWND target = window;
		if (wparam) {
			target = (HWND)wparam;
		}
		HWND found = NULL;
		int scan = 0;
		for (int index = 0; index < ActiveWindows.length(); index++) {
			ActiveWindows.get(found, scan);
			if (found == target) {
				ActiveWindows.remove(scan);
			} else {
				scan++;
			}
		}
		if (lparam) {
			ActiveWindows.addTail(target);
			in_programmatic_reorder = 1;
			SetWindowPos(target, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			in_programmatic_reorder = 0;
		}
		goto cleanup;
	}
	if (ActiveWindows.length() != 0) {
		if (message == WM_WINDOWPOSCHANGING) {
			if (in_programmatic_reorder != 1) {
				HWND found = NULL;
				for (int index = 0; index < ActiveWindows.length(); index++) {
					ActiveWindows.get(found, index);
					if (found == window) {
						WINDOWPOS *wp = (WINDOWPOS *)lparam;
						if (index == ActiveWindows.length() - 1 && wp->hwndInsertAfter != NULL && (wp->flags & SWP_NOZORDER) == 0) {
							wp->hwndInsertAfter = NULL;
						} else {
							wp->flags |= SWP_NOOWNERZORDER | SWP_NOZORDER;
						}
						InvalidateRect(window, NULL, FALSE);
						result = 0;
						goto cleanup;
					}
				}
			}
			goto call_custom_proc;
		}
		if (message == WM_DESTROY) {
			HWND found = NULL;
			int scan = 0;
			for (int index = 0; index < ActiveWindows.length(); index++) {
				ActiveWindows.get(found, scan);
				if (found == window) {
					ActiveWindows.remove(scan);
				} else {
					scan++;
				}
			}
		}
	}

	if (window == ODTooltip.window && (message == WM_NCDESTROY || message == WM_SHOWWINDOW || message == WM_KILLFOCUS)) {
		End_Tooltip();
	}

	if (message == WM_ERASEBKGND) {
		result = 1;
		goto cleanup;
	} else if (message == WM_SETFOCUS) {
		if (specific_proc == OwnerDrawButtonHandler || specific_proc == ListBoxHandler) {
			SetFocus((HWND)wparam);
		}
		if (data == NULL) {
			goto after_messages;
		}
		if (!data->hasFocus) {
			CancelIMEComposition();
			data->hasFocus = 1;
		}
	} else if (message == WM_KILLFOCUS) {
		data->hasFocus = 0;
	} else if (message == WM_SHOWWINDOW) {
		if (wparam == 0) {
			if (data == NULL) {
				goto after_messages;
			}
			YR_REC_TAIL(int, data, 0x1FC) = 0;
		}
	}
	if (data != NULL) {
		if (message == 0x49C) {
			result = (LRESULT)data->image;
			data->image = (Surface *)lparam;
			goto cleanup;
		} else if (message == 0x4AA) {
			result = (LRESULT)data->altImage;
			data->altImage = (Surface *)lparam;
			goto cleanup;
		} else if (message == 0x49A) {
			result = data->toolTipsEnabled;
			data->toolTipsEnabled = lparam;
			goto cleanup;
		}
		if (data->toolTipsEnabled && (message == WM_TIMER || (message >= WM_MOUSEMOVE && message <= WM_MBUTTONDBLCLK))) {
			RECT corner;
			GetWindowRect(window, &corner);
			WindowFromPoint(*(POINT *)&corner);
		}
	}
	if (message == WM_GETTEXT) {
		allow_proc = FALSE;
	} else if (message == WM_GETTEXTLENGTH) {
		allow_proc = FALSE;
		result = 0;
	} else if (message == LB_GETTEXT) {
		allow_proc = FALSE;
		result = 0;
	} else if (message == CB_ADDSTRING || message == CB_INSERTSTRING || message == CB_SELECTSTRING || message == CB_FINDSTRINGEXACT
		|| message == CB_GETLBTEXT || message == CB_GETLBTEXTLEN || message == LB_ADDSTRING || message == LB_FINDSTRING
		|| message == LB_FINDSTRINGEXACT || message == LB_SELECTSTRING || message == LB_INSERTSTRING) {
		allow_proc = FALSE;
		result = -1;
	}
	if (data != NULL) {
		if (message == 0x4CE) {
			allow_proc = FALSE;
			result = data->hasText == 0;
			goto after_messages;
		} else if (message == 0x4B5) {
			char *destination = (char *)lparam;
			if (destination != NULL) {
				*destination = '\0';
				if (data->text != NULL) {
					WideToCharString(destination, data->text, wparam - 1);   // NATIVE: gamemd.exe passes wparam and writes one byte past the buffer
					if (wparam > 0) {
						destination[wparam - 1] = '\0';
					}
				}
			}
		} else if (message == 0x4B3) {
			wchar_t *destination = (wchar_t *)lparam;
			if (destination != NULL) {
				*destination = L'\0';
				if (data->text != NULL) {
					wcsncpy(destination, data->text, wparam);
					if ((int)wparam > 0) {
						destination[wparam - 1] = L'\0';
					}
				}
			}
		} else if (message == 0x4EB) {
			data->rejectChars = (wchar_t *)lparam;
		} else if (message == 0x4B4) {
			data->Set_Text((char const *)lparam);
			data->hasText = 1;
		} else if (message == 0x4D1) {
			result = data->asciiOnly;
			data->asciiOnly = wparam;
			allow_proc = FALSE;
			goto after_messages;
		} else if (message == 0x4B2) {
			wchar_t const *text = data->text;
			wchar_t const *new_text = (wchar_t const *)lparam;
			bool both = text != NULL || new_text != NULL;
			bool added = text == NULL && new_text != NULL;
			bool removed = text != NULL && new_text == NULL;
			bool changed = added || removed || (both && wcscmp(text, new_text) != 0);
			data->Set_Text((wchar_t const *)lparam);
			data->hasText = 0;
			if (changed && YR_REC_70(data) == 1 && YR_REC_A8(data)) {
				KillTimer(window, 0);
				YR_REC_A8(data) = 0;
				SendMessage(window, 0x4EE, 0, 0);
			}
		}
	}
	if (message == WM_MOUSEMOVE) {
		HWND parent = GetParent(window);
		HWND status = GetDlgItem(parent, 0x695);
		if (status != NULL) {
			WideWstring text = FUN_00603FC0(parent, window, LOWORD(lparam), HIWORD(lparam));
			if (wcscmp(text.get(), DAT_00887734) == 0) {
				text = FUN_00603F60(parent, window);
				if (wcscmp(text.get(), DAT_00887734) == 0) {
					text = FUN_00604060(parent, window);
				}
			}
			SendMessage(status, 0x4B2, 0, (LPARAM)text.text());
		}
	}

after_messages:
	if (is_paint && data != NULL) {
		if (data->paintDisabled) {
			ValidateRect(window, NULL);
			result = CallWindowProc(specific_proc, window, 0x4A5, wparam, lparam);
		} else {
			{
				RECT tooltip_rect;
				tooltip_rect.left = ODTooltip.bounds.X;
				tooltip_rect.right = ODTooltip.bounds.Width + ODTooltip.bounds.X + 1;
				tooltip_rect.top = ODTooltip.bounds.Y;
				tooltip_rect.bottom = ODTooltip.bounds.Height + ODTooltip.bounds.Y + 1;
				RECT intersect;
				if (num_rect_updates == 1 && IntersectRect(&intersect, &disp_rect, &tooltip_rect)) {
					if (ODTooltip.isActive && ODTooltip.isHidden != 1 && ODTooltip.background != NULL) {
						Rect drect = ODTooltip.bounds;
						Rect srect(0, 0, ODTooltip.bounds.Width, ODTooltip.bounds.Height);
						VisibleSurface->Blit_From(drect, *ODTooltip.background, srect, false, true);
						ODTooltip.isHidden = true;
						show_tooltip = TRUE;
					}
				}
			}
			if (min_update_rect.x >= disp_rect.left) {
				min_update_rect.x = disp_rect.left;
			}
			if (min_update_rect.y >= disp_rect.top) {
				min_update_rect.y = disp_rect.top;
			}
			if (max_update_rect.x <= disp_rect.right) {
				max_update_rect.x = disp_rect.right;
			}
			if (max_update_rect.y <= disp_rect.bottom) {
				max_update_rect.y = disp_rect.bottom;
			}

			owner = window;
			while (owner != NULL) {
				if (GetWindowLong(owner, DWL_DLGPROC) != 0) {
					break;
				}
				owner = GetParent(owner);
			}
			ownerdata = NULL;
			if (owner != NULL) {
				Dialogs.getPointer(owner, &ownerdata);
			}
			if (owner == window) {
				if (YR_REC_TAIL(int, ownerdata, 0x1FC) < 1) {
					state = 1;
					anim_state = 1;
					goto call_proc_paint;
				}
				state = YR_REC_TAIL(int, ownerdata, 0x1FC);
			} else {
				if (ownerdata == NULL) {
					state = anim_state;
					ValidateRect(window, NULL);
					result = 1;
					goto finalize_paint;
				}
				state = YR_REC_TAIL(int, ownerdata, 0x1FC);
			}
			anim_state = state;
			if (state < 1) {
				ValidateRect(window, NULL);
				result = 1;
			} else {
			call_proc_paint:
				result = CallWindowProc(specific_proc, window, message, wparam, lparam);
				if (ownerdata != NULL) {
					YR_REC_TAIL(int, ownerdata, 0x1FC) = state;
				}
				{
					ArrayList<HWND> children;
					EnumChildWindows(window, (WNDENUMPROC)ODAddWindowToList, (LPARAM)&children);
					HWND combo_owner = NULL;
					HWND child = NULL;
					for (int index = 0; index < children.length(); index++) {
						children.get(child, index);
						WNDPROC childproc = NULL;
						DialogProcs.getValue(child, childproc);
						if (childproc != ::FUN_0060D540) {
							InvalidateRect(child, NULL, FALSE);
							UpdateWindow(child);
						} else {
							combo_owner = child;
						}
					}
					if (combo_owner != NULL) {
						InvalidateRect(combo_owner, NULL, FALSE);
						UpdateWindow(combo_owner);
					} else if (DAT_00AC48C0 != NULL && DAT_00AC48C4 == owner) {
						RECT drop_rect;
						Get_Display_Rect(DAT_00AC48C0, &drop_rect);
						RECT intersect;
						if (IntersectRect(&intersect, &disp_rect, &drop_rect)) {
							InvalidateRect(DAT_00AC48C0, NULL, FALSE);
							UpdateWindow(DAT_00AC48C0);
						}
					}
				}
				state = anim_state;
			}

		finalize_paint:
			if (state > 0) {
				HWND sibling = owner;
				while (window == owner || num_rect_updates == 1) {
					if (sibling == NULL) {
						break;
					}
					sibling = GetWindow(sibling, GW_HWNDPREV);
					if (sibling == NULL) {
						break;
					}
					if (GetWindowLong(sibling, DWL_DLGPROC)) {
						RECT sibling_rect;
						Get_Display_Rect(sibling, &sibling_rect);
						RECT intersect;
						if (IntersectRect(&intersect, &disp_rect, &sibling_rect)) {
							InvalidateRect(sibling, NULL, FALSE);
							UpdateWindow(sibling);
							break;
						}
					}
				}
			}
		}
		goto after_proc;
	}
	if (!allow_proc) {
		goto after_proc;
	}

call_custom_proc:
	if (specific_proc != NULL) {
		result = CallWindowProc(specific_proc, window, message, wparam, lparam);
	}

after_proc:
	if (message == WM_NCDESTROY) {
		WinData *destroydata = NULL;
		if (Dialogs.getPointer(window, &destroydata) && destroydata != NULL && destroydata->cachedSurface != NULL) {
			delete destroydata->cachedSurface;
			destroydata->cachedSurface = NULL;
			CachedSurfaceCount--;
		}
		ODRemoveFromDict(window, 0);
	}

cleanup:
	ProcessedMessages.remove(key);
	if (data != NULL && is_paint) {
		if (GetWindowLong(window, DWL_DLGPROC) && num_rect_updates > 1) {
			YR_REC_TAIL(int, data, 0x1FC) = 2;
		}
		if (--num_rect_updates == 0) {
			if (!data->paintDisabled && anim_state >= 1 && !Is_Web_Browser_Visible()) {
				Rect whole(min_update_rect.x, min_update_rect.y, max_update_rect.x - min_update_rect.x, max_update_rect.y - min_update_rect.y);
				Rect source = whole;
				Rect destination(whole.X + offset_x, whole.Y + offset_y, whole.Width, whole.Height);
				if (YR_REC_TAIL(int, data, 0x1FC) == 1) {
					YR_REC_TAIL(int, data, 0x1FC) = 2;
					if (GetWindowLong(window, DWL_DLGPROC) && RunOpenAnimationIfNeeded(window)) {
						YR_REC_TAIL(int, data, 0x1FC) = 3;
						// NATIVE (as the RA2 tree's draft, Scout1209 r15): paint the slide-in's final
						// update now, children included, so the copy below shows the strip with them.
						UpdateWindow(window);
					}
					VisibleSurface->Lock();
					AlternateSurface->Lock();
					VisibleSurface->Blit_From(destination, *AlternateSurface, whole, false, true);
					AlternateSurface->Unlock();
					VisibleSurface->Unlock();
					ArrayList<HWND> children;
					EnumChildWindows(window, (WNDENUMPROC)ODAddWindowToList, (LPARAM)&children);
					HWND child = NULL;
					for (int index = 0; index < children.length(); index++) {
						children.get(child, index);
						SendMessage(child, 0x4AF, 0, 0);
					}
				} else {
					char class_name[128];
					GetClassName(window, class_name, sizeof(class_name));
					if (strcmp(class_name, "ComboBox") == 0) {
						FUN_0072AA10(&source, -1, -1);
						FUN_0072AA10(&destination, -1, -1);
					}
					VisibleSurface->Lock();
					AlternateSurface->Lock();
					VisibleSurface->Blit_From(destination, *AlternateSurface, source, false, true);
					AlternateSurface->Unlock();
					VisibleSurface->Unlock();
				}
			}
			min_update_rect.x = 0xFFFFFF;
			min_update_rect.y = 0xFFFFFF;
			max_update_rect.x = 0;
			max_update_rect.y = 0;
		}
	}
	if (show_tooltip) {
		DrawTooltip(true);
	}
	if (message == WM_INITDIALOG) {
		result = 0;
	}
	return(result);
}
}

// OwnerDraw::OwnerDrawButtonHandler, YR 0x00612B70 (3,168 bytes, unmatched in ra2-decomp; RA2
// counterpart 0x005F0320): the push button. From gamemd.exe's code with the tree's matched sibling
// AutoRadioButtonHandler (0x00616980, the same three-piece PCX art, TibSun's ButtonCtrlProc) for
// its kind-0 paint. The button's kind (+0xB0, Dialog.LayoutBand, which ClassifyLayoutBand sets)
// picks the art: kinds 1-3 draw a frame of the small-button, side-button or close-button shape
// through its drawer (kind 1 frames 2 / 3 lit / 4 pressed, kinds 2-3 frames 0 / 2 lit / 1 pressed);
// kind 0 the image (+0x14, pressed +0x18) or the b?e_li/mi/ri PCX pieces with the click sound.
// A disabled shape button's text takes the side's colour (txtlabel2.cpp's RGBs, through the
// hicolor format). Message 0x4DC flashes the button (+0xC4) by a one-second timer toggling the
// lit flag (+0xC5); a press plays GUIMainButtonSound. The button state is +0xE8.
#define YR_REC_FLASH(data) ((data)->ButtonTimerActive)
#define YR_REC_LIT(data) ((data)->ButtonAlternateFrame)
namespace OwnerDraw {
ConvertClass * GetSmallButtonAnimConvert(void);
ConvertClass * GetSideButtonConvert(void);
extern SHPStruct *SmallButtonAnimShape;
extern SHPStruct *SideButtonShape;
extern SHPStruct *CloseButtonShape;
extern int PrimaryTextColor;
int __fastcall DrawWideText(Surface * surface, wchar_t const * text, LTRBStruct * rect, BitFont * font, unsigned color, int style, int p5, int p6, int p7, int p8);
}
ConvertClass * __fastcall FUN_0072B050(void);
Point2D __fastcall FUN_0072A9C0(Rect const * rect);
void __fastcall FUN_00621B80(Rect * rect, Surface * surface, unsigned short color, int alpha);
extern RGBClass DAT_00B0F9FC;
extern RGBClass DAT_00B0FB14;
extern RGBClass DAT_00B0FB19;
extern RGBClass DAT_00B0FA94;

// OwnerDraw::StaticHandler, YR 0x006153E0 (4,032 bytes, unmatched in ra2-decomp; RA2 counterpart
// 0x005F2B50, whose behaviour rewrite in native/link/behaviour_m14_ownerdraw.cpp follows its
// decompile): the label/picture control. From gamemd.exe's code: the record's kind (+0x70):
// 0 text, 1 typed text (characters shown +0x80, step +0x88, period +0x84, typing sound +0x90),
// 2 a picture (+0x14, centred, magenta transparent), 3 a shape frame, 4 an animated shape (shape
// +0x78 through drawer +0x74, frame +0x98 of +0x94; 0x4D3 starts it at Control_Volume's period,
// 0x4D4 stops it, 0x4D5/0x4D6 set/get the frame, 0x4D7 names the window told of each frame);
// kinds 2-4 wait +0xA0 ms from +0x9C. The first paint saves the background under the control
// (+0x10); 0x4B1 sets a fill colour (+0xB4/+0xB8); 0x498 the text colour (+0xEC). Movies: 0x4E4
// by name, 0x4DF by Movies index, a MovieHandle (+0x58) on the background surface advanced by
// timer 0x65 every 34 ms; 0x4E3 loops it, 0x4E0/0x4E1 pause/resume, 0x4E2 stops, 0x4F0 blits.
// NATIVE: the record's pointer fields in this range (+0x58, +0x60, +0x74, +0x78, +0xA4) are
// pointer-sized, kept by window in a side table as the RA2 tree's rewrite does; the other fields
// are reached at their 32-bit offsets through the record's Dialog view (YR_REC).
#include "movies.h"
#include "shapeset.h"
void FUN_004068E0(char const *format, ...);
#define YR_REC(type, data, off) (*(type *)Yr_WinData_Field((data), (off)))
namespace OwnerDraw { extern int DisabledTextColor; }
int Control_Volume(HWND dialog, HWND control);
MovieHandle * __fastcall OwnerDraw_InitMovieHandle(char const * name, Surface * surface, Rect * bounds);
extern DynamicVectorClass<char const *> Movies;

namespace {
unsigned short YR_Hicolor(unsigned color)
{
	return (unsigned short)DSurface::Build_Hicolor_Pixel(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
}
}

// OwnerDraw::ResetControlDrawModeAndTimerProc, YR 0x0060A5B0 (1,184 bytes, unmatched in
// ra2-decomp; RA2 counterpart 0x005E8130): the WM_INITDIALOG pass over a dialog's controls that
// gives each static its kind (+0x70) and timers. From gamemd.exe's code: a typed-text control
// (FUN_00602490) gets kind 1 with its period, step and tail (FUN_00600CA0, FUN_006015E0,
// FUN_00601D20) and MessageCharTyped as its sound in the 0x103/0xBC7 dialogs' control 0x6F4;
// a special control (Is_Special_Control) kind 2 with no image yet; the animated controls
// (control 0x71C of the listed dialogs, 0x6EA/0x6EB/0x6EC of 0x94, 0x72B of 0x103/0xBC7, 0x7A9
// of 0xC4) kind 4 with their shape and drawer; then the reveal delay (Is_Typed_Control_Int)
// starts the control's timer.
// NATIVE: gamemd.exe takes the drawer from Play_Control_Sound's register result and the shape
// from FUN_00603870 (both declared without a pointer result in the tree); natively the two
// helpers below repeat their dialog/control tables and return the pointers whole.
bool __fastcall FUN_00602490(HWND dialog, HWND control);
int __fastcall FUN_00600CA0(HWND dialog, HWND control);
int __fastcall FUN_006015E0(HWND dialog, HWND control);
int __fastcall FUN_00601D20(HWND dialog, HWND control);
bool __fastcall Is_Special_Control(HWND dialog, HWND control);
int __fastcall Is_Typed_Control_Int(HWND dialog, HWND control);
ConvertClass *YR_FUN_0072E280(void) __asm__("__Z12FUN_0072E280v");
ConvertClass *YR_FUN_0072E2A0(void) __asm__("__Z12FUN_0072E2A0v");
void *YR_LoadWholeFileEx(char const *name, bool &allocated) __asm__("__ZN10FileSystem15LoadWholeFileExEPKcRb");
ConvertClass *YR_FUN_0072DAC0(void) __asm__("__Z12FUN_0072DAC0v");
ConvertClass *YR_FUN_0072DAD0(void) __asm__("__Z12FUN_0072DAD0v");
ConvertClass *YR_FUN_0072D450(void) __asm__("__Z12FUN_0072D450v");
ConvertClass *YR_FUN_0072CC80(void) __asm__("__Z12FUN_0072CC80v");
extern SHPStruct *YR_Shape_B0FB48 __asm__("_DAT_00B0FB48");
extern SHPStruct *YR_Shape_B0FB10 __asm__("_DAT_00B0FB10");
extern SHPStruct *YR_Shape_B0FB30 __asm__("_DAT_00B0FB30");
extern SHPStruct *YR_Shape_B0FAD0 __asm__("_DAT_00B0FAD0");

namespace {
bool YR_Animated_Dialog(int type)
{
	static int const listed[] = {0x94, 0xD8, 0xF5, 0xE2, 0xD5, 0x101, 0x129, 0xD7, 0xBB, 0x100, 0xD6, 0x125, 0x122, 0x112, 0xE7,
		0x116, 0x11D, 0x11C, 0xFE, 0x10F, 0x117, 0x114, 0xE6, 0xF3, 0xF4, 0x2BC, 0x10E, 0x108, 0xBC6, 0xBC7};
	for (int index = 0; index < ARRAY_SIZE(listed); index++) {
		if (listed[index] == type) {
			return true;
		}
	}
	return false;
}

int YR_Dialog_Type(HWND dialog)
{
	OwnerDraw::WinData *data = Find_Data(dialog);
	return (data != NULL) ? data->DialogID : 0;
}

// Play_Control_Sound (0x006035F0): the drawer of the control's animation.
ConvertClass *YR_Control_Drawer(HWND dialog, HWND control)
{
	int type = YR_Dialog_Type(dialog);
	int id = GetDlgCtrlID(control);
	ConvertClass *drawer = YR_FUN_0072E280();
	if (YR_Animated_Dialog(type) && id == 0x71C) {
		drawer = YR_FUN_0072E2A0();
	}
	if (type == 0x94) {
		if (id == 0x6EA) {
			return YR_FUN_0072DAC0();
		}
		if (id == 0x6EC) {
			return YR_FUN_0072DAD0();
		}
	}
	if (type == 0x103 || type == 0xBC7) {
		if (id == 0x72B) {
			drawer = YR_FUN_0072D450();
		}
		return drawer;
	}
	if (type == 0xC4 && id == 0x7A9) {
		drawer = YR_FUN_0072CC80();
	}
	return drawer;
}

// FUN_00603870 (0x00603870): the control's animation shape (allocated says it was loaded for it).
void const *YR_Control_Shape(HWND dialog, HWND control, bool &allocated)
{
	int type = YR_Dialog_Type(dialog);
	int id = GetDlgCtrlID(control);
	if (YR_Animated_Dialog(type) && id == 0x71C) {
		return YR_LoadWholeFileEx("SDWRNANM.SHP", allocated);   // DAT_00833674's name
	}
	if (type == 0x94) {
		if (id == 0x6EA) {
			return YR_Shape_B0FB48;
		}
		if (id == 0x6EC) {
			return YR_Shape_B0FB10;
		}
	}
	if (type == 0x103 || type == 0xBC7) {
		return (id == 0x72B) ? YR_Shape_B0FB30 : NULL;
	}
	if (type == 0xC4 && id == 0x7A9) {
		return YR_Shape_B0FAD0;
	}
	return NULL;
}
}

BOOL CALLBACK OwnerDraw::ResetControlDrawModeAndTimerProc(HWND window, LPARAM lparam)
{
	HWND parent = GetParent(window);
	WinData *data = Find_Data(window);
	if (data == NULL) {
		return TRUE;
	}
	if (FUN_00602490(parent, window)) {
		YR_REC(int, data, 0x70) = 1;
		YR_REC(bool, data, 0xA8) = false;
		YR_REC(int, data, 0x80) = 1;
		YR_REC(int, data, 0x84) = FUN_00600CA0(parent, window);
		YR_REC(int, data, 0x88) = FUN_006015E0(parent, window);
		YR_REC(int, data, 0x8C) = FUN_00601D20(parent, window);
		int type = YR_Dialog_Type(parent);
		int id = GetDlgCtrlID(window);
		if ((type == 0x103 || type == 0xBC7) && id == 0x6F4) {
			YR_REC(int, data, 0x90) = Rule->MessageCharTyped;
		} else {
			YR_REC(int, data, 0x90) = -1;
		}
		return TRUE;
	}

	bool animated = false;
	if (Is_Special_Control(parent, window)) {
		YR_REC(int, data, 0x70) = 2;
		if (data->image == NULL) {
			data->image = NULL;
		}
	} else {
		int type = YR_Dialog_Type(parent);
		int id = GetDlgCtrlID(window);
		if (YR_Animated_Dialog(type) && id == 0x71C) {
			animated = true;
		} else if (type == 0x94) {
			animated = (id == 0x6EA || id == 0x6EC || id == 0x6EB);
		} else if (type == 0x103 || type == 0xBC7) {
			animated = (id == 0x72B);
		} else if (type == 0xC4) {
			animated = (id == 0x7A9);
		}
		if (!animated) {
			goto timer;
		}
		YR_REC(int, data, 0x70) = 4;
		if (data->StaticShape == NULL) {
			bool allocated = false;
			data->StaticShapeDrawer = YR_Control_Drawer(parent, window);
			data->StaticShape = (ShapeSet *)YR_Control_Shape(parent, window, allocated);
			YR_REC(bool, data, 0x7C) = allocated;
		}
	}
	YR_REC(int, data, 0x98) = 0;
	YR_REC(unsigned, data, 0x9C) = GetTickCount();
	YR_REC(int, data, 0xA0) = Is_Typed_Control_Int(parent, window);

timer:
	if (YR_REC(int, data, 0xA0) > 0) {
		SetTimer(window, 0, YR_REC(int, data, 0xA0), NULL);
		if (animated) {
			YR_REC(bool, data, 0xA8) = true;
		}
	} else if (animated) {
		if (data->StaticShape != NULL) {
			YR_REC(int, data, 0x94) = ((ShapeSet const *)data->StaticShape)->Get_Count();
			YR_REC(bool, data, 0xA8) = false;
		}
	}
	return TRUE;
}

// FUN_0060B950, YR 0x0060B950 (1,895 bytes, unmatched in ra2-decomp): the per-dialog nudges of a
// control's position after the layout pass (a pixel or a few in the listed dialogs, the 0x103
// dialog's typed lines by the render height), moving the control only when one applied.
// Transcribed from gamemd.exe's tables.
int __fastcall Is_Owner_Draw_Subset_Message(unsigned int message);
namespace Drawing { extern int RenderWidth; }
extern int DAT_007F5BE4;
extern int DAT_007F5BF0;

namespace {
bool YR_In(int value, int const *list, int count)
{
	for (int index = 0; index < count; index++) {
		if (list[index] == value) {
			return true;
		}
	}
	return false;
}
}
#define YR_IN(value, ...) ([&]() { static int const list_[] = {__VA_ARGS__}; return YR_In((value), list_, ARRAY_SIZE(list_)); }())

void __fastcall FUN_0060B950(HWND window)
{
	HWND parent = GetParent(window);
	RECT parent_rect;
	RECT rect;
	GetWindowRect(parent, &parent_rect);
	GetWindowRect(window, &rect);
	int x = rect.left - parent_rect.left;
	int y = rect.top - parent_rect.top;
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
	int const x0 = x, y0 = y, w0 = w, h0 = h;

	int type = YR_Dialog_Type(parent);
	int id = GetDlgCtrlID(window);
	bool listed = YR_IN(type, 0x10C, 0x94, 0xA3, 0x73, 0xD8, 0xBBB, 0xF5, 0x105, 0xE2, 0xD5, 0x2B5, 0xB7, 0x2B4, 0xBC7, 0x101, 0x129,
		0xBC6, 0xD7, 0x108, 0xBC, 0xBD, 0xBB, 0xB5, 0xBBA, 0xFF, 0x6B, 0x100, 0x102, 0xB8, 0xD6, 0x103);

	if ((listed || Is_Owner_Draw_Subset_Message(type)) && id == 0x694) {
		if (listed) {
			if (Session.FUN_0069BBE0()) {
				return;
			}
			if (YR_IN(type, 0xBC, 0xBD, 0x102, 0xC2, 0xC9, 0xBC6, 0x105, 0x6B, 0x113)) {
				y++;
			} else {
				h++;
				y += 7;
			}
		} else if (YR_IN(type, 0x125, 0xE7, 0x116, 0x11D, 0x11C, 0x114, 0x2BC, 0x109, 0x10F, 0x117, 0xE6, 0xF3, 0xF4, 0x10E, 0x112)) {
			y += 2;
		} else {
			return;
		}
	} else if (type == 0x102) {
		if (id == 0x50C) {
			y--;
		} else if (YR_IN(id, 0x54E, 0x693, 0x696, 0x69A)) {
			x--;
		} else if (id == 0x6A0) {
			x++;
			w++;
		} else {
			return;
		}
	} else if (type == 0xBB) {
		if (id != 0x540) {
			return;
		}
		w--;
	} else if (type == 0xBC || type == 0xBD) {
		if (id == 0x53F) {
			w++;
		} else if (id == 0x54F || id == 0x402) {
			y--;
		} else if (YR_IN(id, 0x54E, 0x3E8, 0x54D, 0x69A)) {
			x--;
		} else {
			return;
		}
	} else if (type == 0x103) {
		if (!YR_IN(id, 0x6F3, 0x6F4, 0x69F, 0x6F6, 0x6EF, 0x6F1, 0x6F0, 0x6F2) || Drawing::RenderWidth < DAT_007F5BE4) {
			return;
		}
		y += 10 - (DAT_007F5BF0 - DAT_007F5BEC) / 2;
	} else if (type == 0x108) {
		if (YR_IN(id, 0x793, 0x794, 0x792, 0x795)) {
			y++;
		}
		if (YR_IN(id, 0x797, 0x796, 0x798)) {
			y += 2;
		}
		if (YR_IN(id, 0x790, 0x791, 0x78F, 0x792, 0x793, 0x794, 0x795, 0x796, 0x797, 0x798)) {
			x++;
		}
	} else if (type == 0x113) {
		if (id == 0x768) {
			x++;
		} else if (id == 0x53F) {
			w--;
		} else {
			return;
		}
	} else if (type == 0x10E) {
		if (id != 0x79F) {
			return;
		}
		x++;
	} else {
		return;
	}
	if (x != x0 || y != y0 || w != w0 || h != h0) {
		MoveWindow(window, x, y, w, h, FALSE);
	}
}

// FUN_0060C0C0, YR 0x0060C0C0 (987 bytes, unmatched in ra2-decomp): UpdateControlPosition's
// per-control callback (lparam: the layout offset). From gamemd.exe's code: each control of the
// current dialog is placed by the first layout rule that claims it (FUN_00608500's explicit
// rectangle, the owner-drawn or plain FUN_00608CD0 / FUN_00609730 groups, the tooltip bar 0x695,
// the 0xE2 dialog's 0x71D, the listed dialogs' FUN_0060B7A0), else kept where it is relative to
// its parent; then FUN_0060B950 applies the dialog's nudges.
bool __fastcall FUN_00608500(HWND parent, HWND child, Rect *out);
void __fastcall FUN_0060AF50(HWND window, RECT *rect);
bool __fastcall FUN_00608CD0(HWND parent, HWND child);
bool FUN_00609730(HWND dialog, HWND control);
void __fastcall FUN_0060B000(HWND window, Point2D const &size);
void __fastcall FUN_0060B1D0(HWND window, Point2D const &size);
void __fastcall FUN_0060B350(HWND window);
void __fastcall FUN_0060B420(HWND window);
void __fastcall FUN_0060B550(HWND window);
void __fastcall FUN_0060B610(HWND window);
void __fastcall FUN_0060B7A0(HWND window);
bool Is_Owner_Draw_Message(unsigned int msg);

// DrawCampaignMenuTransition, YR 0x006071E0 (3,553 bytes, unmatched in ra2-decomp): slides the
// dialog's frame pieces in (reverse) or out, one piece after another, then tells the dialog 0x4EC
// (in) or 0x4ED (out). The RA2 tree's matched counterpart OwnerDraw::unknown_5e5150
// (src/recovered/ownrdraw.cpp, RA2 0x005E5150) with gamemd.exe's code read for the YR globals:
// the drawers FUN_0072E280 / GetSmallButtonAnimConvert / FUN_0072D450, the shapes DAT_00B0F9DC
// (top panel), DAT_00B0FAF8 / DAT_00B0FAC0 (side panel and its slide), SmallButtonAnimShape (the
// button rows), DAT_00B0FB00 / DAT_00B0FB34 (main screen piece), the piece count DAT_00B0FA20,
// the layout rectangles DAT_00B0FC10..DAT_00B0FC28, the record flags +0xD5..+0xD7, the visible
// button counts FUN_0060A180 / FUN_0060A250, and ShellButtonSlideSound.
BOOL CALLBACK FUN_0060A180(HWND window, LPARAM lparam);
BOOL CALLBACK FUN_0060A250(HWND window, LPARAM lparam);
void __fastcall FUN_00406F70(void);
extern int DAT_00AC1CAC;
extern int DAT_00AC4894;
extern int DAT_00B0FA20;
extern Rect *DAT_00B0FC10;
extern Rect *DAT_00B0FC14;
extern Rect *DAT_00B0FC18;
extern Rect *DAT_00B0FC1C;
extern Rect *DAT_00B0FC20;
extern Rect *DAT_00B0FC28;
extern SHPStruct *YR_Shape_B0F9DC __asm__("_DAT_00B0F9DC");
extern SHPStruct *YR_Shape_B0FAF8 __asm__("_DAT_00B0FAF8");
extern SHPStruct *YR_Shape_B0FAC0 __asm__("_DAT_00B0FAC0");
extern SHPStruct *YR_Shape_B0FB00 __asm__("_DAT_00B0FB00");
extern SHPStruct *YR_Shape_B0FB34 __asm__("_DAT_00B0FB34");

namespace {
inline int YR_Slide_Phase(int delta)
{
	if (delta < 0) {
		return -1;
	}
	if (delta >= 6) {
		return -2;
	}
	return delta;
}

inline void YR_Draw_Piece(ConvertClass *drawer, SHPStruct *shape, int frame, Point2D const &point, Rect const &window)
{
	Draw_Shape(*AlternateSurface, *drawer, (ShapeSet const *)shape, frame, point, window, SHAPE_WIN_REL);
}
}

void __fastcall DrawCampaignMenuTransition(HWND dialog, bool reverse)
{
	OwnerDraw::WinData *record = Find_Data(dialog);
	bool has_d7 = record ? YR_REC(bool, record, 0xD7) : false;
	record = Find_Data(dialog);
	bool has_d6 = record ? YR_REC(bool, record, 0xD6) : false;
	record = Find_Data(dialog);
	bool has_d5 = record ? YR_REC(bool, record, 0xD5) : false;

	ConvertClass *drawer = YR_FUN_0072E280();
	ConvertClass *drawer_b = OwnerDraw::GetSmallButtonAnimConvert();
	SHPStruct *shape_button = OwnerDraw::SmallButtonAnimShape;
	SHPStruct *shape_top = YR_Shape_B0F9DC;
	SHPStruct *shape_side = YR_Shape_B0FAF8;
	SHPStruct *shape_side_slide = YR_Shape_B0FAC0;
	ConvertClass *drawer_c = YR_FUN_0072D450();
	SHPStruct *shape_main = YR_Shape_B0FB00;
	int count = DAT_00B0FA20;
	Rect window = AlternateSurface->Get_Rect();
	Rect rect_side_slide = *DAT_00B0FC20;
	Rect rect_top = *DAT_00B0FC14;
	Rect rect_side = *DAT_00B0FC18;
	Rect rect_rows = *DAT_00B0FC10;
	Rect rect_main = *DAT_00B0FC1C;

	RECT client;
	Get_Display_Rect(dialog, &client);
	RECT screen;
	GetWindowRect(dialog, &screen);
	int dx = screen.left - client.left;
	int dy = screen.top - client.top;

	int x = rect_rows.X;
	int y = rect_rows.Y;
	int width = rect_rows.Width;
	int height = rect_rows.Height * count;
	if (has_d5) {
		x = rect_side.X;
		width = rect_side.Width;
		y = rect_side.Y;
		height += rect_rows.Y;
	}
	Rect from(x, y, width, height);
	Rect to = from;
	to.X += dx;
	to.Y += dy;
	Rect from_main = rect_main;
	Rect to_main = from_main;
	to_main.X += dx;
	to_main.Y += dy;

	int dir = reverse ? -1 : 1;
	int base_rows = reverse ? 10 : 5;
	int base_empty = reverse ? 16 : 11;
	int base_top = reverse ? 6 : 1;
	int base_side = reverse ? 5 : 0;
	int base_main = reverse ? 5 : 0;

	DAT_00AC1CAC = 0;
	EnumChildWindows(dialog, FUN_0060A180, 0);
	int buttons = DAT_00AC1CAC;
	int last = count - 1;
	int top = rect_rows.Y;
	if (has_d6) {
		top += rect_rows.Height;
	}
	DAT_00AC4894 = 0;
	EnumChildWindows(dialog, FUN_0060A250, 0);
	bool any = DAT_00AC4894 >= 1;
	if (!any) {
		last++;
	}

	int pieces = count + 3;
	int limit = pieces - 2;
	int *order = new int[pieces];
	int value = 1;
	for (int i = 0; i < limit; i++) {
		order[i] = value++;
	}
	order[pieces - 1] = 0;
	order[limit] = value + 1;
	order[pieces - 3] = 0;
	int steps = 0;
	for (int j = 0; j < pieces; j++) {
		if (steps <= order[j]) {
			steps = order[j];
		}
	}
	steps += 6;

	for (int step = 0; step < steps; step++) {
		if (has_d6) {
			Point2D point = FUN_0072A9C0(&rect_top);
			int phase = YR_Slide_Phase(step - order[pieces - 3]);
			if (phase == -1) {
				YR_Draw_Piece(drawer, shape_top, reverse ? 6 : 0, point, window);
			} else if (phase == -2 && !reverse) {
				YR_Draw_Piece(drawer, shape_top, 6, point, window);
			}
		}
		if (has_d5) {
			Point2D point = FUN_0072A9C0(&rect_side);
			int phase = YR_Slide_Phase(step - order[pieces - 2]);
			if (phase == -1) {
				YR_Draw_Piece(drawer, shape_side, !reverse, point, window);
			} else if (phase == -2) {
				YR_Draw_Piece(drawer, shape_side, reverse != 0, point, window);
			} else {
				point = FUN_0072A9C0(&rect_side_slide);
				YR_Draw_Piece(drawer, shape_side, 1, point, window);
				point = FUN_0072A9C0(&rect_side);
				YR_Draw_Piece(drawer, shape_side_slide, phase * dir + base_side, point, window);
			}
		}
		if (has_d6) {
			Point2D point = FUN_0072A9C0(&rect_top);
			int phase = YR_Slide_Phase(step - order[pieces - 3]);
			if (phase == -2) {
				if (reverse) {
					YR_Draw_Piece(drawer, shape_top, 0, point, window);
				}
			} else {
				YR_Draw_Piece(drawer, shape_top, phase * dir + base_top, point, window);
			}
		}
		int k;
		for (k = 0; k < buttons; k++) {
			Point2D point(rect_rows.X, k * rect_rows.Height + top);
			int phase = YR_Slide_Phase(step - order[k]);
			if (phase == -1) {
				YR_Draw_Piece(drawer_b, shape_button, reverse ? 10 : 1, point, window);
			} else if (phase == -2) {
				YR_Draw_Piece(drawer_b, shape_button, reverse ? 1 : 10, point, window);
			} else {
				YR_Draw_Piece(drawer_b, shape_button, phase * dir + base_rows, point, window);
			}
		}
		for (k = buttons; k < last; k++) {
			Point2D point(rect_rows.X, k * rect_rows.Height + top);
			int phase = YR_Slide_Phase(step - order[k]);
			if (phase == -1) {
				YR_Draw_Piece(drawer_b, shape_button, reverse ? 10 : 0, point, window);
			} else if (phase == -2) {
				YR_Draw_Piece(drawer_b, shape_button, reverse ? 0 : 10, point, window);
			} else {
				YR_Draw_Piece(drawer_b, shape_button, phase * dir + base_empty, point, window);
			}
		}
		if (any) {
			Point2D point(rect_rows.X, DAT_00B0FC28->Y - rect_rows.Height);
			int phase = YR_Slide_Phase(step - order[count - 1]);
			if (phase == -1) {
				YR_Draw_Piece(drawer_b, shape_button, reverse ? 10 : 1, point, window);
			} else if (phase == -2) {
				YR_Draw_Piece(drawer_b, shape_button, reverse ? 1 : 10, point, window);
			} else {
				YR_Draw_Piece(drawer_b, shape_button, phase * dir + base_rows, point, window);
			}
		}
		if (has_d7) {
			Point2D point = FUN_0072A9C0(&rect_main);
			int phase = YR_Slide_Phase(step - order[pieces - 1]);
			if (phase == -1) {
				if (!reverse && Drawing::RenderWidth >= DAT_007F5BE4) {
					point.X += 0x50;
				}
				YR_Draw_Piece(drawer_c, reverse ? YR_Shape_B0FB34 : shape_main, 0, point, window);
			} else if (phase == -2) {
				if (reverse && Drawing::RenderWidth >= DAT_007F5BE4) {
					point.X += 0x50;
				}
				YR_Draw_Piece(drawer_c, reverse ? shape_main : YR_Shape_B0FB34, 0, point, window);
			} else {
				if (Drawing::RenderWidth >= DAT_007F5BE4) {
					point.X += 0x50;
				}
				YR_Draw_Piece(drawer_c, shape_main, phase * dir + base_main, point, window);
			}
		}
		VisibleSurface->Lock();
		AlternateSurface->Lock();
		VisibleSurface->Blit_From(to, *AlternateSurface, from, false, true);
		if (has_d7) {
			VisibleSurface->Blit_From(to_main, *AlternateSurface, from_main, false, true);
		}
		AlternateSurface->Unlock();
		VisibleSurface->Unlock();
		FUN_00406F70();
		Sleep(30);
	}
	delete [] order;
	if (reverse) {
		VocClass::PlayGlobal(Rule->ShellButtonSlideSound, 1.0f, 0x2000);
		while (MouseCursor->Get_Mouse_State()) {
			MouseCursor->Show_Mouse();
		}
		SendMessage(dialog, 0x4EC, 0, 0);
	} else {
		SendMessage(dialog, 0x4ED, 0, 0);
	}
}
