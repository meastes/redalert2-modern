// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P8, branch ports-on-rebuild): appended to the generated display unit (fixups.json "append").
// NOT original code; never part of an exact build. The control enhancements under the rebuild's names: main's
// native/link/native_queue_five.cpp and native_modern_controls.cpp (lanes C16, C18; the YR port's glue/yr_enhancements.cpp
// is the same rules). Each is off by default and in the Original preset, and off answers every question with the
// original's value.
//
// Queue five with Shift: a sidebar build icon's production click (PRODUCE) or right-click abandon (ABANDON) adds its event
// five times, as five clicks would, while Shift is held (SidebarClass::StripClass::SelectClass::Action 0x006809F0).
//
// Controls: Modern: on the map outside the placement, repair, sell, power, super weapon, waypoint and beacon modes a left
// click selects (never an order), on open ground it clears the selection (Shift keeps it), Shift toggles a unit, a click on
// a unit of a group makes it the selection, a double click selects the player's units of its type; a right click gives the
// order the original's left click would give there and never selects; the right drag still scrolls; Ctrl or Alt keeps the
// original's left click; on the radar the buttons swap. Classic keeps the touch tap across player-controlled houses (lane
// C18: Allied mission 2's Tanya).
// ---------------------------------------------------------------------------------------------
#include <stdio.h>

extern "C" long long ra2port_now_ms(void);
extern "C" int ra2_enh_queue_five(void);
extern "C" int ra2_enh_modern_controls(void);
extern "C" int ra2_host_touch_click(void);
ObjectClass * Best_Selected_Object(Cell const * cell, ObjectClass const * object);   // init.cpp, RA2 0x0051A690

namespace {
const unsigned short kShift = 0x10, kCtrl = 0x11, kAlt = 0x12;   // VK_SHIFT, VK_CONTROL, VK_MENU
const int kQueueFive = 5;
const long long kDoubleClickMs = 500;

bool Held(unsigned short key) { return Keyboard != NULL && Keyboard->Down((KeyNumType)key); }

bool In_Mode()
{
	ObjectClass * pending = Map.PendingObjectPtr;
	return (pending != NULL && pending->Is_Techno()) || Map.PendingObject != NULL || Map.IsRepairMode || Map.IsSellMode
		|| Map.IsPowerMode || Map.IsTargettingMode != SUPER_NONE || Map.IsWaypointMode || Map.IsBeaconMode;
}

bool Selectable(ObjectClass * object) { return object != NULL && object->CanBeSelected(); }

ObjectClass * last_object = NULL;
long long last_ms = 0;
ObjectClass * type_select = NULL;

bool Type_Selectable(ObjectClass * object)
{
	if (!Selectable(object) || !object->Is_Techno() || object->Fetch_RTTI() == RTTI_BUILDING) return false;
	HouseClass * house = object->Owner_HouseClass();
	return house != NULL && house->Is_Player_Control();
}

// What the band's select (Bandbox_Selection_Callback) takes.
bool Band_Takes(ObjectClass * object)
{
	HouseClass * house = object->Owner_HouseClass();
	if (house == NULL || !house->Is_Player_Control() || !object->CanBeSelected() || object->IsInLimbo) return false;
	if (object->Fetch_RTTI() == RTTI_BUILDING) {
		BuildingTypeClass * type = ((BuildingClass *)object)->Class;
		if (type->UndeploysInto == NULL) return false;
		for (int index = 0; index < Rule->BuildConst.Count(); index++) {
			if (Rule->BuildConst[index] == type) return false;
		}
	}
	return true;
}
int band_objects = 0, band_unselected = 0;
void Count_Band(ObjectClass * object)
{
	if (!Band_Takes(object)) return;
	++band_objects;
	if (!object->IsSelected) ++band_unselected;
}
void Unselect_Band(ObjectClass * object)
{
	if (Band_Takes(object) && object->IsSelected) object->Unselect();
}

// Classic's touch rule (lane C18): a plain tap on a selectable unit of a player-controlled house other than the selected
// object's selects it (the original answers with a move there, a trap for touch where no move cursor shows first).
bool Tap_Selects_Across_Houses(ObjectClass * object, ActionType action)
{
	if ((action != ACTION_MOVE && action != ACTION_NOMOVE) || !ra2_host_touch_click() || In_Mode() || Held(kCtrl) || Held(kAlt) || Held(kShift)) {
		return false;
	}
	if (!Selectable(object) || object->IsSelected || !object->Is_Techno() || CurrentObject.Count() == 0) return false;
	HouseClass * house = object->Owner_HouseClass();
	if (house == NULL || !house->Is_Player_Control()) return false;
	ObjectClass * selected = Best_Selected_Object(NULL, object);
	return selected != NULL && selected->Owner_HouseClass() != house;
}
}

// The sidebar's PRODUCE and ABANDON events: five copies with the setting on and Shift held. Answers OutList.Add's result
// for the first copy.
bool RB_Queue_Five_Add(EventClass const & event)
{
	bool added = OutList.Add(event);
	if (!added || !ra2_enh_queue_five() || !Held(kShift)) return added;
	int copies = 1;
	while (copies < kQueueFive && OutList.Add(event)) ++copies;
	fprintf(stderr, "queue-five: event %d added %d times\n", (int)event.Type, copies);
	return added;
}

// The left button released over the map (ScrollClass::Message_Handler, WM_LBUTTONUP; band: the selection band is being dragged, DisplayClass::IsRubberBand): the action Mouse_Left_Release
// performs for the original's action there. May clear the selection or the object.
ActionType RB_Modern_Left_Up(ObjectClass * & object, ActionType action, bool band)
{
	type_select = NULL;
	if (!ra2_enh_modern_controls() && Tap_Selects_Across_Houses(object, action)) {
		fprintf(stderr, "controls: tap on another player-controlled house's unit: selected (the original's move there)\n");
		return ACTION_SELECT;
	}
	if (!ra2_enh_modern_controls() || In_Mode() || Held(kCtrl) || Held(kAlt)) return action;
	bool shift = Held(kShift);
	if (band) {
		last_object = NULL;
		return Selectable(object) && action == ACTION_TOGGLE_SELECT ? ACTION_TOGGLE_SELECT : ACTION_NONE;
	}
	if (!Selectable(object)) {
		last_object = NULL;
		if (!shift) Unselect_All();
		object = NULL;
		return ACTION_NONE;
	}
	long long now = ra2port_now_ms();
	bool twice = object == last_object && now - last_ms <= kDoubleClickMs;
	last_object = twice ? NULL : object;
	last_ms = now;
	if (twice && Type_Selectable(object)) {
		if (!shift) Unselect_All();
		type_select = object;
		object = NULL;
		return ACTION_NONE;
	}
	if (shift) return ACTION_TOGGLE_SELECT;
	if (object->IsSelected && CurrentObject.Count() > 1) return ACTION_SELECT;
	return ACTION_NONE;
}

// After Mouse_Left_Release: a double click selects the player's units of the object's type on the screen (Select_Type, the
// type-select key's command 0x006F71E0).
void RB_Modern_Left_Done(void)
{
	ObjectClass * object = type_select;
	type_select = NULL;
	if (object == NULL) return;
	Select_Type(object->Class_Of()->Name());
	fprintf(stderr, "modern-controls: double click: %d selected\n", CurrentObject.Count());
}

// The right button released over the map (WM_RBUTTONUP): true when Modern takes it as an order (no scroll ran, no mode on).
bool RB_Modern_Right_Order(bool scrolled)
{
	return ra2_enh_modern_controls() && !scrolled && !In_Mode();
}

// The action a Modern right click performs for the original's left-click action there: none when that would only select.
ActionType RB_Modern_Right_Action(ActionType action)
{
	last_object = NULL;
	if (action == ACTION_NONE || action == ACTION_SELECT || action == ACTION_TOGGLE_SELECT) return ACTION_NONE;
	fprintf(stderr, "modern-controls: right click: action %d\n", (int)action);
	return action;
}

// The band's walk (Mouse_Left_Release): with Shift over objects all selected already, they are unselected instead.
typedef void (*RB_Band_Select)(ObjectClass *);
RB_Band_Select RB_Modern_Band_Walk(RB_Band_Select select)
{
	if (!ra2_enh_modern_controls() || !Held(kShift) || TacticalMap == NULL) return select;
	Point2D start = TacticalMap->RubberBandStart;
	band_objects = band_unselected = 0;
	TacticalMap->Select_Rubber_Band(Count_Band);
	TacticalMap->RubberBandStart = start;
	if (band_objects > 0 && band_unselected == 0) {
		fprintf(stderr, "modern-controls: shift band: %d selected objects unselected\n", band_objects);
		return Unselect_Band;
	}
	return select;
}

// The radar's mouse flags (RTacticalClass::Action): Modern swaps the buttons (LEFTPRESS 0x01, LEFTRELEASE 0x04,
// RIGHTPRESS 0x10, RIGHTRELEASE 0x40).
unsigned RB_Modern_Radar_Flags(unsigned flags)
{
	if (!ra2_enh_modern_controls()) return flags;
	unsigned out = flags & ~0x55u;
	if (flags & 0x01) out |= 0x10;
	if (flags & 0x04) out |= 0x40;
	if (flags & 0x10) out |= 0x04;
	if (flags & 0x40) out |= 0x01;
	return out;
}
