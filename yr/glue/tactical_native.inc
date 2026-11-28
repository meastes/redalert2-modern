// SPDX-License-Identifier: GPL-3.0-or-later

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFTS (lane Y3): functions unmatched in the YR tree, the text TibSun's
// (TibSun archive code/tactical.cpp; SPDX GPL-3.0-or-later, Copyright 2025 Electronic Arts Inc. and
// OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md); native changes are marked
// NATIVE. Behaviour code for the native build; never part of an exact build.
// ---------------------------------------------------------------------------------------------

#include "beacon.h"
#include "ebolt.h"
#include "linetrail.h"
#include "radbeam.h"
#include "capmgr.h"
#include "super.h"
#include "suprtype.h"
#include "stringtable.h"
#include "airstrike.h"
#include "foot.h"
#include "fastmath.h"

bool __fastcall FUN_00637AA0(void);   // planning.cpp: planning mode is on
void __fastcall FUN_0063B0A0(void);
void __fastcall FUN_0063B150(void);
void __fastcall FUN_0063B2F0(void);   // planmgr.cpp: planning mode's nodes (lane P74)
bool __fastcall FUN_0043B150(TechnoClass * techno);   // building.cpp: a Psychic Sensor shows this techno's intent (lane P74)
extern int DAT_00842900;   // the airstrike laser's phase shift (7; gamemd.exe's data, lane P74)
// NATIVE (lane P74): how often the foreground pass drew an airstrike designator's laser and a Psychic Sensor's intent
// line, for the harness's `rendercounts` (yr/tests/yr_render_steps.py); counting only.
static int Yr_Render_Laser_Draws;
static int Yr_Render_Intent_Draws;
int Yr_Render_Draw_Count(int which) { return(which ? Yr_Render_Intent_Draws : Yr_Render_Laser_Draws); }
extern BeaconManagerClass BeaconManager;
extern DynamicVectorClass<SuperClass *> ShowTimers;

void Tactical::Render(Surface & surface, bool fullredraw, int drawpass)
{
	/*
	 * Set true when the view scrolled further than one screen, so the still-valid region can no
	 * longer be blitted and a full redraw is forced. It persists across the passes of a frame.
	 */
	static bool scrolled_off_screen = false;

	/*
	 * The composite surface to draw the foreground objects onto. This tracks the front buffer
	 * across the surface swap performed during the pan pass.
	 */
	Surface * composite = &surface;

	SelectableCount = 0;
	bool has_selectable_buildings = Contains_Selectable_Buildings(TacticalRect);

	/*
	 * Work out how far the view has panned since the last frame. A pan smaller than the view can be
	 * satisfied by blitting the still-valid region and only redrawing the strips that scrolled in;
	 * a larger pan forces a full redraw.
	 */
	int xoffset = LastTacPixelX - TacPixelX;
	int yoffset = LastTacPixelY - TacPixelY;

	if (drawpass == DRAW_PASS_PAN) {
		if (abs(xoffset) < TacticalRect.Width && abs(yoffset) < TacticalRect.Height) {
			scrolled_off_screen = false;
		} else {
			fullredraw = true;
			scrolled_off_screen = true;
		}
	} else {
		if (scrolled_off_screen) {
			fullredraw = true;
		}
	}

	/*
	 * Either the whole view, or just the strips the pan exposed.
	 */
	if (fullredraw) {
		xoffset = 0;
		yoffset = 0;
	}

	/*
	 * Decide whether the cached layers actually need to be rebuilt this pass. A pending redraw,
	 * selectable buildings, dirty areas or a pixel pan all force a rebuild; but once the first
	 * frame has been rendered, a pan is only honoured if the world coordinate also changed. The
	 * foreground pass never rebuilds the cached layers.
	 */
	bool render;
	/*
	 * Either the whole view, or just the strips the pan exposed.
	 */
	if (fullredraw) {
		render = true;
	} else if (IsFirstRender) {
		if (has_selectable_buildings) {
			render = true;
		} else {
			render = IsToRedraw || DirtyAreas.Count() != 0 || TacPixelX != LastTacPixelX || TacPixelY != LastTacPixelY;
		}
	} else if (has_selectable_buildings) {
		render = true;
	} else if (DirtyAreas.Count() == 0 && IsToRedraw) {
		render = true;
	} else if (DirtyAreas.Count() == 0 && TacticalCoord.X == LastTacticalCoord.X && TacticalCoord.Y == LastTacticalCoord.Y) {
		render = false;
	} else {
		render = IsToRedraw || DirtyAreas.Count() != 0 || TacPixelX != LastTacPixelX || TacPixelY != LastTacPixelY;
	}

	if (drawpass != DRAW_PASS_FOREGROUND && render) {

		/*
		 * The magnitude of the pan, and the source/destination rectangles for shifting the
		 * still-valid portion of the cached surface across by that amount.
		 */
		unsigned panwidth = abs(xoffset);
		unsigned panheight = abs(yoffset);
		int ystart = (yoffset >= 0) ? 0 : -yoffset;
		int xstart = (xoffset >= 0) ? 0 : -xoffset;

		Rect sourcerect(TacticalDimensions.X + xstart, TacticalDimensions.Y + ystart,
					TacticalDimensions.Width - panwidth, TacticalDimensions.Height - panheight);
		Rect destrect(TacticalDimensions.X + ((xoffset <= 0) ? 0 : xoffset),
					TacticalDimensions.Y + ((yoffset <= 0) ? 0 : yoffset),
					TacticalDimensions.Width - panwidth, TacticalDimensions.Height - panheight);

		if (xoffset != 0 || yoffset != 0) {
			if (drawpass == DRAW_PASS_PAN || drawpass == DRAW_PASS_FULL) {
				CompositeSurface->Blit_From(destrect, *TileSurface, sourcerect);
			}
			if (drawpass == DRAW_PASS_PAN || drawpass == DRAW_PASS_FULL) {
				DepthBuffer->Pan(-xoffset, -yoffset, 0xFFFFu);
			}
			if (drawpass == DRAW_PASS_PAN || drawpass == DRAW_PASS_FULL) {
				AlphaBuffer->Pan(-xoffset, -yoffset, 0x7Fu);
			}
			if (drawpass == DRAW_PASS_PAN || drawpass == DRAW_PASS_FULL) {

				/*
				 * Swap the composite and tile surfaces so the freshly-panned image becomes the
				 * render target.
				 */
				Surface * oldcomp = CompositeSurface;
				CompositeSurface = TileSurface;
				TileSurface = oldcomp;
				composite = CompositeSurface;
				LogicalSurface = CompositeSurface;
			}
		} else {
			if (fullredraw && (drawpass == DRAW_PASS_PAN || drawpass == DRAW_PASS_FULL)) {
				DepthBuffer->Fill(0xFFFFu);
				AlphaBuffer->Fill(127u);
			}
		}

		if (drawpass == DRAW_PASS_PAN) {
			return;
		}

		if (!Debug_Map) {
			Register_Dirty_Buildings(Rect(TacticalRect.X - 64, TacticalRect.Y - 64, TacticalRect.Width + 128, TacticalRect.Height + 128));
		}

		Surface * prevlogical = LogicalSurface;
		LogicalSurface = TileSurface;
		LogicalSurface->Lock();

		if (drawpass == DRAW_PASS_BACKGROUND || drawpass == DRAW_PASS_FULL) {

			/*
			 * Build the two redraw strips that the pan exposed. 'xpanrect' is the full-height strip
			 * revealed by horizontal scrolling (left edge if scrolled right, right edge if scrolled
			 * left), 'ypanrect' the strip revealed by vertical scrolling. 'bounds' is the region
			 * that the blit above preserved, used to clip the depth wipe and shroud.
			 */
			bool scrolled_left = LastTacPixelX > TacPixelX;
			bool scrolled_right = LastTacPixelX < TacPixelX;
			bool scrolled_up = LastTacPixelY > TacPixelY;
			bool scrolled_down = LastTacPixelY < TacPixelY;

			Rect leftstrip(TacticalRect.X, TacticalRect.Y, panwidth, TacticalRect.Height);
			Rect rightstrip(TacticalRect.X + TacticalRect.Width - panwidth, TacticalRect.Y, panwidth, TacticalRect.Height);
			Rect leftclip = Intersect(leftstrip, TacticalRect);
			Rect rightclip = Intersect(rightstrip, TacticalRect);

			Rect topstrip;
			Rect bottomstrip;
			if (scrolled_left || scrolled_right) {
				int stripx = scrolled_left ? TacticalRect.X + panwidth : TacticalRect.X;
				topstrip = Rect(stripx, TacticalRect.Y, TacticalRect.Width - panwidth, panheight);
				bottomstrip = Rect(stripx, TacticalRect.Y + TacticalRect.Height - panheight, TacticalRect.Width - panwidth, panheight);
			} else {
				topstrip = Rect(TacticalRect.X, TacticalRect.Y, TacticalRect.Width, panheight);
				bottomstrip = Rect(TacticalRect.X, TacticalRect.Y + TacticalRect.Height - panheight, TacticalRect.Width, panheight);
			}
			Rect topclip = Intersect(topstrip, TacticalRect);
			Rect bottomclip = Intersect(bottomstrip, TacticalRect);

			Rect bounds(TacticalRect.X + (scrolled_left ? panwidth : 0),
					TacticalRect.Y + (scrolled_up ? panheight : 0),
					TacticalRect.Width - panwidth, TacticalRect.Height - panheight);

			Rect xpanrect(0, 0, 0, 0);
			Rect ypanrect(0, 0, 0, 0);
			if (scrolled_left) {
				xpanrect = leftclip;
			} else if (scrolled_right) {
				xpanrect = rightclip;
			}
			if (scrolled_up) {
				ypanrect = topclip;
			} else if (scrolled_down) {
				ypanrect = bottomclip;
			}

			Wipe_Depth(fullredraw, xoffset, yoffset, bounds);
			Render_Shroud(xpanrect, ypanrect, bounds, fullredraw);
			Render_Tiles(xpanrect, ypanrect, fullredraw);
			Render_Fogged_Objects(xpanrect, ypanrect, fullredraw);
			Render_Overlays(xpanrect, ypanrect, fullredraw);
			Render_Terrain(xpanrect, ypanrect, fullredraw);
			Render_Tile_Shadows(xpanrect, ypanrect, fullredraw);
			Render_Buildings(xpanrect, ypanrect, fullredraw);

			for (int index = DirtyAreas.Count() - 1; index >= 0; index--) {
				DirtyAreas.Delete_Index(index);
			}

			LogicalSurface->Unlock();
			LogicalSurface = prevlogical;
			CellRedrawCount = 0;
		}
	}

	/*
	 * Remember this frame's position for the next pan calculation.
	 */
	LastTacticalCoord.X = TacticalCoord.X;
	LastTacticalCoord.Y = TacticalCoord.Y;
	LastTacPixelX = TacPixelX;
	LastTacPixelY = TacPixelY;

	if (drawpass == DRAW_PASS_BACKGROUND || drawpass == DRAW_PASS_FULL) {
		LogicalSurface->Blit_From(TacticalRect, *TileSurface, TacticalRect);
	}

	if (drawpass == DRAW_PASS_FOREGROUND || drawpass == DRAW_PASS_FULL) {
		/*
		**	NATIVE: YR's foreground pass, read from gamemd.exe (0x006D45F2..0x006D4B37) on TibSun's
		**	shape. YR (P74): the steps the draft left out, all matched upstream since the ac45ee9e pin:
		**	Draw_Radial_Indicators after the beams (0x006D467F); for a techno not the player's, the
		**	Psychic Sensor's line of its intent (FUN_0043B150, then FootClass::FUN_004DC340 for a
		**	foot object on the map, 0x006D4764), drawn in planning mode too; the airstrike
		**	designator's laser onto the building it paints (0x006D47FB, TechnoClass::FUN_00705860);
		**	in planning mode the planned nodes (FUN_0063B2F0, 0x006D491A); Draw_Sparkles over the
		**	view (0x006D492B).
		*/
		Add_Buildings_To_Selectable(TacticalRect);
		composite->Lock();
		Surface * savedlogical = LogicalSurface;
		LogicalSurface = composite;
		Draw_Waypoints(false);
		Draw_Rally_Points(false);
		Draw_Placement(false);

		IonBlastClass::Draw_All();
		Draw_Objects(true);
		SpotLightClass::Draw_All();
		LaserDrawClass::Draw_All();
		EBolt::FUN_004C2830();
		LineTrail::Process_All();
		RadBeam::Update_All();
		Draw_Radial_Indicators();	// YR (P74, 0x006D467F)
		BeaconManager.Draw(LogicalSurface, TacticalRect);

		Draw_Rubber_Band();
		Draw_Waypoints(true);
		Draw_Rally_Points(true);
		Draw_Placement(true);

		bool planning = FUN_00637AA0();
		if (planning) {
			FUN_0063B0A0();
			FUN_0063B150();
		}

		for (int i = 0; i < TechnoClass::Array.Count(); i++) {
			TechnoClass * techno = TechnoClass::Array[i];
			if (techno->House->Is_Player_Control()) {
				if (planning) {
					continue;
				}
				if (techno->IsSelected && TechnoClass::ActionLines) {
					techno->Draw_Action_Line(false, 0);
				}
			} else {
				/*
				**	YR (P74, gamemd.exe 0x006D4764): a Psychic Sensor of the player's in range shows the
				**	enemy's intent (FUN_0043B150), drawn by FootClass::FUN_004DC340 for a foot object
				**	on the map.
				*/
				if (FUN_0043B150(techno) && techno->Is_Foot() && !techno->IsInLimbo) {
					((FootClass *)techno)->FUN_004DC340();
					Yr_Render_Intent_Draws++;
				}
				if (planning) {
					continue;
				}
			}
			if (techno->Strength) {
				if (techno->CaptureManager != NULL && techno->CaptureManager->NeedsToDrawLinks()) {
					techno->CaptureManager->DrawLinks();
				}
				if (techno->Strength && techno->MindControlledBy != NULL && techno->MindControlledBy->CaptureManager != NULL
					&& techno->MindControlledBy->CaptureManager->NeedsToDrawLinks()) {
					techno->MindControlledBy->CaptureManager->DrawLinks();
				}
			}

			/*
			**	YR (P74, gamemd.exe 0x006D47FB-0x006D48F1): an airstrike designator painting a building
			**	draws its laser from its fire coordinate to a point that circles the building's centre
			**	(timeGetTime() >> DAT_00842900; 15 leptons: x by the cosine of 0.37 of the phase, z by
			**	its sine).
			*/
			AirstrikeClass * airstrike = techno->Airstrike;
			if (airstrike != NULL && airstrike->Owner == techno && airstrike->Target != NULL
					&& airstrike->Target->Fetch_RTTI() == RTTI_BUILDING) {
				double phase = (double)(unsigned __int64)(timeGetTime() >> DAT_00842900);
				Coord center = airstrike->Target->Center_Coord();
				int z = (int)(fastmath::sin(phase) * 15.0 + center.Z);
				int x = (int)(fastmath::cos(phase * 0.37) * 15.0 + center.X);
				Coord end(x, center.Y, z);
				Coord start = techno->Fire_Coord(0, Point3D(0, 0, 0));
				techno->FUN_00705860(start, end);
				Yr_Render_Laser_Draws++;
			}
		}

		if (planning) {
			FUN_0063B2F0();	// YR (P74, 0x006D491A): the planned nodes
		}
		Draw_Sparkles(TacticalRect, TacticalRect);	// YR (P74, 0x006D492B)

		LogicalSurface = savedlogical;
		composite->Unlock();

		/*
		**	The countdown timers: the mission timer, the super weapons on show, the houses'
		**	power blackouts (gamemd.exe shows the remaining frames / 15).
		*/
		int index = 0;
		if (Scen->MissionTimer.Started != -1) {
			DrawTimer(index++, ColorSchemes[PlayerPtr->Scheme], Scen->MissionTimer.Value() / 15, Scen->MissionTimerTextCSF, NULL, NULL);
		}
		for (int timer = 0; timer < ShowTimers.Count(); timer++) {
			SuperClass * super = ShowTimers[timer];
			int remaining = super->Control.Value();
			if (super->IsSuspended && Session.Type == GAME_NORMAL && remaining == super->GetRechargeTime()) {
				continue;
			}
			DrawTimer(index++, ColorSchemes[super->House->Scheme], remaining / 15, super->Class->UIName, (unsigned __int64 *)&super->BlinkTimer, &super->BlinkState);
		}
		for (int h = 0; h < Houses.Count(); h++) {
			HouseClass * house = Houses[h];
			int remaining = house->PowerBlackoutTimer.Value();
			if (remaining > 0) {
				DrawTimer(index++, ColorSchemes[house->Scheme], remaining / 15, StringTable::LoadString("MSG:BlackoutTimer", NULL, "D:\\ra2mdpost\\Tactical.CPP", 0x1C4), NULL, NULL);
			}
		}

		Draw_Screen_Text(ScreenText);

		IsFirstRender = false;
		IsToRedraw = false;
	}
}

// ---------------------------------------------------------------------------------------------
// NON-EXACT NATIVE DRAFT (lane Y4): Tactical::Draw_Tiles(Cell const &, Rect const &), unmatched in
// ra2-decomp. TibSun's text (TibSun archive code/tactical.cpp; SPDX GPL-3.0-or-later, Copyright 2025
// Electronic Arts Inc. and OpenTS contributors, EA's GPL section 7 terms; THIRD_PARTY.md).
// Behaviour code for the native build only. Compared with gamemd.exe 0x006D7F20 (824 bytes) by lane P74:
// the same steps but the elevation test marked "YR (P74)".
// ---------------------------------------------------------------------------------------------
void Tactical::Draw_Tiles(Cell const & cell, Rect const & cliprect)
{
	CellClass * basecell = &Map[cell];

	Coord coord = Coord_Whole(Coord(cell));
	Point2D screen = Coord_To_Pixel_Absolute(coord);
	if (basecell->Height < 2) {
		Point2D pixel = screen - Point2D(TacPixelX, TacPixelY);
		pixel.X -= ISO_TILE_PIXEL_W / 2;
		basecell->Draw_It(pixel, TacticalRect, 0);
	}

	int x = cell.X;
	int y = cell.Y;
	for (int level = 0; level < 24; level += 2) {
		for (int col = 0; col < 3; col++) {
			int cx = x + ((col + 1) >> 1);
			int cy = y - (col >> 1) + 1;
			if (Map.In_Radar(Cell(cx, cy))) {
				CellClass * cellptr = &Map[Cell(cx, cy)];
				bool draw = false;
				int height = cellptr->Height;
				bool overlap = false;

				if (col != 1) {
					if (height == level + 1) {
						draw = true;
						overlap = true;
					}
				} else if (height > level && height < level + 4) {
					draw = true;
					overlap = true;
				}

				/*
				**	YR (P74, gamemd.exe 0x006D8078-0x006D80C9): a raised cell below this level is drawn when its
				**	elevation reaches it: the side columns compare height - level, the middle one height - level + 1.
				**	TibSun's text compared one less in both (height - level - 1, height - level).
				*/
				if (cellptr->Elevation > 0 && !draw) {
					int diff = height - level;
					if (col != 1) {
						if (diff < 0 && cellptr->Elevation + diff >= 0) {
							draw = true;
						} else {
							draw = overlap;
						}
					} else if (diff + 1 > 0) {
						draw = overlap;
					} else {
						draw = true;
						if (cellptr->Elevation + diff + 1 <= 0) {
							draw = overlap;
						}
					}
				}

				if (height == level) {
					if (!draw) {

						/*
						 * Same height and not already flagged for redraw: the tile only needs
						 * redrawing if its iso tile carries extra (overhanging) image data.
						 */
						int subtile = cellptr->SubTile;
						int ittype = cellptr->ITType;
						if (ittype == ISOTILE_NONE || ittype == ISOTILE_NONE_LEGACY) {
							continue;
						}
						IsometricTileTypeClass * type = IsometricTileTypes[ittype];
						if (type == NULL) {
							continue;
						}
						IsoTileSet const * set = (IsoTileSet const *)type->Get_Image_Data();
						if (set == NULL) {
							continue;
						}
						if (subtile >= set->Tile_Count() - 1) {
							subtile = set->Tile_Count() - 1;
						}
						IsoTileRecord const * record = set->Fetch_Record_Pointer_Unsafe(subtile);
						if (record == NULL || !record->IsHasExtraData) {
							continue;
						}
					}
				} else if (!draw) {
					continue;
				}

				Cell above(cx, cy);
				coord = Coord_Whole(Coord(above));
				Point2D pixel;
				Coord_To_Pixel(coord, pixel);
				pixel.X -= ISO_TILE_PIXEL_W / 2;
				Map[above].Draw_It(pixel, TacticalRect, 0);
			}
		}
		x++;
		y++;
	}
}
