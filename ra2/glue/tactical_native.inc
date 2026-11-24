// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P2, branch ports-on-rebuild): appended to the generated tactical unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// Building_Player_Detector_Covers_Target (0x004386F0): parked in main (src/recovered/building.cpp,
// best 82.58), absent from the rebuild. Main's draft with the rebuild's names (main's slot 195 is
// BuildingClass::IsPowerOnline 0x00450360, its unknown_475d60 CellClass::Cell_Coord 0x00475D60, by
// the rebuild catalog): whether one of the player's powered psychic detectors covers an unallied
// object's target or the last entry of its navigation queue (the enemy action lines are drawn then).
// The distance is the double square root of the offset's squares (main's draft takes the float
// vector length in three of its four tests; the radius tests are in whole leptons either way).
// ---------------------------------------------------------------------------------------------
static bool Detector_Covers(BuildingClass * detector, Coord const & other)
{
	Coord position = detector->Center_Coord();
	double x = position.X - other.X;
	double y = position.Y - other.Y;
	double z = position.Z - other.Z;
	return (int)sqrt(x * x + y * y + z * z) <= detector->Class->PsychicDetectionRadius * 256;
}

bool __fastcall Building_Player_Detector_Covers_Target(FootClass * object)
{
	if (PlayerPtr->Is_Ally(object->House)) {
		return false;
	}
	int count = PlayerPtr->PsychicDetectionBuildings.Count();
	for (int index = 0; index < count; ++index) {
		BuildingClass * detector = PlayerPtr->PsychicDetectionBuildings[index];
		if (!detector->IsPowerOnline()) {
			continue;
		}
		AbstractClass * target = object->TarCom;
		if (target != NULL) {
			if (target->Is_Techno()) {
				if (Detector_Covers(detector, target->Center_Coord())) return true;
			} else if (target->Fetch_RTTI() == RTTI_CELL) {
				if (Detector_Covers(detector, ((CellClass *)target)->Cell_Coord())) return true;
			}
		}
		if (object->Is_Foot()) {
			AbstractClass * nav = object->NavQueue.Count() == 0 ? object->NavCom : object->NavQueue[object->NavQueue.Count() - 1];
			if (nav != NULL) {
				if (nav->Fetch_RTTI() == RTTI_CELL) {
					if (Detector_Covers(detector, ((CellClass *)nav)->Cell_Coord())) return true;
				} else if (nav->Is_Techno()) {
					if (Detector_Covers(detector, nav->Center_Coord())) return true;
				}
			}
		}
	}
	return false;
}
