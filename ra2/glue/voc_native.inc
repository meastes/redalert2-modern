// SPDX-License-Identifier: GPL-3.0-or-later
// ---------------------------------------------------------------------------------------------
// NATIVE (lane P3, branch ports-on-rebuild): appended to the generated voc unit
// (native/rb/fixups.json "append"). NOT EXACT; never part of an exact build.
//
// Sound_Effect: voc.h keeps TibSun's two declarations ("RA2: no such routine; callers play through
// VocClass::PlayGlobal"), and the parked drafts of UnitClass (weapon report), BuildingClass (the
// tesla charge) and the map (ice cracks) still call them by those names. Behaviour forms: the
// global play and the positional play the original's callers make in their place.
// ---------------------------------------------------------------------------------------------
int Sound_Effect(VocType voc, float volume, int panning)
{
	return VocClass::PlayGlobal(voc, volume, panning) != NULL;
}

int Sound_Effect(VocType voc, Coord const & coord)
{
	return VocClass::PlayAt(voc, coord) != NULL;
}
