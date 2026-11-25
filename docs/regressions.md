# The user's own bugs, checked on this port

Every bug the user found by hand in the old port (Mac, iPhone Duo, LAN), from redalert2-decomp's
`docs/PORTS_FACTS.md`, `docs/PORTS_REMAINING.md`, `docs/CAMPAIGN_PLAY_LIST.md`, `docs/HANDOFF.md`, `native/README.md`
(lanes N1-N33, C, Y, Scout and R rows) and lane P12's Duo report on this port; lane P14, 2026-11-24. The id after each
bug is the one those notes use.

Status: **scripted, passing** a check on this port runs it and passed (the ctest named; the P12 full run of 60 rows
at e084c67 plus lane P14's new rows); **fixed** the old port's fix is carried (a `fixups.json` row or glue) but no
script reaches it; **same code** the code is the shared host's, byte for byte the old port's; **needs his check**
nothing scripted reaches it: worth a look while playing.

Counts (RA2 rows, 66): scripted and passing 30 (2 of them, the Movies menu, failed here first and lane P14 fixed
them), fixed 10, same code 5, needs his check 19, waiting for other lanes 2 (the both-games toggle, Windows). The 25
Yuri's Revenge-only rows wait for the YR port (lane P13): N32, N13, Y15-diff, N3-mac-defeat, Y13-anim, N4-garrison,
N18-internet, N18-slide, N18-rows, Y11-backdrop, Y11-focus, N21-banner, N27, Y14-bar, N3-typesel, N3-savedlg,
N5-origload, N3-netcrash, N3-yrkbd, N21-abort, Y14-noise, N4-mirage, N12-mirage1, N4-derrick, Y14-skip.

| Bug | Where found | Status on the new port | Check |
| --- | --- | --- | --- |
| Map could be panned (and pinch-zoomed) during a mission intro whose input is locked (Allied 'Hail to the... (N31-3) | iPhone Duo, campaign | scripted, passing | ra2-main-locked-input |
| With one unit selected, a tap on another of the player's units moved the selected one ('not able to select... (C18-cross) | iPhone Duo, campaign | scripted, passing | ra2-main-touch-cross-classic, -cross-modern, -cross-mouse; ra2-ios-sim-touch-cross-classic |
| 'Pink flag' seen by the user; unexplained (user will report again if seen) (U-pink) | iPhone Duo, campaign | needs his check | never explained; watch for it |
| Allied 1 crash around Tanya's C4 (uncertain whether user-reported; the commit names only 'Allied 1 Tanya... (r20) | Mac, campaign | scripted, passing | ra2-native-chain-allied; the gate: Allied 1 identical to the old port for 6,000 frames, with Tanya clicked through (allied-01-orders) |
| A skirmish started from the menus ran ~95 frames/s ('way too fast') at the slider's default Fastest (M20-speed) | Mac, skirmish | scripted, passing | ra2-native-skirmish (3,747 frames in 75 s at the 16 ms floor); P12 row keeps the floor to game speed 0 |
| Tank shells did no damage (in a LAN game tanks never damaged each other; GIs did); Sight_From also... (C22-tank) | Mac + iPhone LAN (affects every mode), combat | scripted, passing | ra2-native-tankdamage (new, lane P14): a Grizzly shells an enemy Rhino, strength 400 to 264 to 60, dead at frame 350 |
| Ctrl+click force fire did not work (the user's force-attack bug) (S1209-force) | Mac, combat | needs his check | Ctrl+click force fire: the gate's order scripts use Ctrl+click on Allied 1 (identical), no damage check |
| Units would not select, move or deploy with the trackpad (default skirmish, windowed, tap-to-click) (S1209-trackpad) | Mac (trackpad), combat | needs his check | host code identical to the old port |
| Crash when the mouse moved over the map (U-mousecrash) | Mac, combat | scripted, passing | every gate run and ra2-main-controls-* move the mouse over the map |
| Menu buttons drawn with the wrong art (main menu and in-game menus need the red/blue sidebar button shapes) (M20-buttons) | Mac, menus/shell | scripted, passing | ra2-shell-frames-mac / -iphone (menu frames against the old port) |
| Menu buttons invisible after the window lost and regained focus (S1209-invis) | Mac, menus/shell | fixed by lane P14, scripted, passing | ra2-main-menu-movie (new): failed here first, the Movies list was empty (the movie tables were zero storage); passes after the fix |
| Main menu flashed (user video); labels missing right after the slide-in (S1209-flash) | Mac, menus/shell | needs his check | no script |
| Window went unresponsive (beachball) in long busy stretches such as loads (S1209-beachball) | Mac, menus/shell | needs his check | host code identical |
| Loads from a game folder on a network share slow or failing (directory-listing cache, negative opens;... (S1209-share) | Mac, menus/shell | needs his check | host code identical |
| A movie from the Movies menu was cut at the sidebar edge (its right part drawn lower and larger); the main... (C11-movie) | iPhone Duo, menus/shell | fixed by lane P14, scripted, passing | ra2-main-menu-movie (as S1209-invis) |
| Main menu art and buttons apart with noise around them at the Duo's 944x634 (sizes between 800x600 and... (Scout1274-menu) | iPhone Duo, menus/shell | scripted, passing | ra2-main-widescreen; P12 carried the menu panel rows |
| Exit Game -> OK left a black screen instead of quitting the app (r31-exit) | iPhone Duo, menus/shell | needs his check | iOS host code identical; Simulator only |
| Internet button clickable (showed a message box); the user wanted it disabled (C20-internet) | Mac / all, menus/shell | fixed | P12 row (Internet disabled at the main menu); lan_lobby.py not yet run on the new port |
| One-game toggle read as help text, then flickered over the menu movie (C23-toggle) | iPhone Duo, menus/shell | YR port pending | the one-game toggle comes with lane P13 |
| With Menu fill = Scale to fit and Pinch zoom on, RA2's menus were letterboxed on all four sides (YR's... (N28-fit) | iPhone Duo, menus/shell | needs his check | zoom rows carried; one-game-menu-backdrop is a both-games check (lane P13) |
| Enhancements page talked about gestures on the Mac and Windows (Y8-labels) | Mac / Windows, menus/shell | same code | platform/sdl/enhancements.cpp is the old port's |
| Enhancements page did not look like the game's own dialogs (the user's restyle request) (C7-restyle) | all, menus/shell | scripted, passing | ra2-shell-frames (Enhancements page); P12 fixed the Cancel button |
| A double tap in the LAN lobby read as a right click (probably user-reported) (r29-lobbytap) | iPhone Duo, menus/shell | needs his check | host code identical |
| Building placement preview did not follow the mouse (M20-place) | Mac, sidebar/command bar | needs his check | placement preview: the gate's skirmish order script places a power plant by clicks (both ports), the preview is not checked |
| Sidebar flickered (cursor-erased sidebar every frame while a cameo's clock ran) (M20-flicker) | Mac, sidebar/command bar | scripted, passing | cursor_stress.py passed on this port (lane P9) |
| With Game interface size above 1x the command bar ran across the Escape menu and the column ended above... (C16-dlg) | iPhone Duo, sidebar/command bar | scripted, passing | ra2-main-zoom-menu, ra2-main-zoom-menu-ui |
| A command bar closed with its close thumb could not be opened again with Zoom gestures on (open thumb off... (N31-1) | iPhone Duo, sidebar/command bar | scripted, passing | ra2-main-command-bar-thumb |
| Command bar flashed higher up the map when its icons were tapped (after any in-play dialog) (N31-2) | iPhone Duo, sidebar/command bar | scripted, passing | ra2-main-command-bar-thumb |
| No way to set a command-bar team on touch (the toolbar's Fire/Ctrl did nothing on team buttons) (probably... (Y15-team) | iPhone Duo, sidebar/command bar | scripted, passing | ra2-main-touch-team-hold, ra2-ios-sim-touch-team-hold |
| Game crashed after a few minutes with two machines (reused control-group orders) (r21-lancrash) | Mac + Mac (two machines), multiplayer/LAN | scripted, passing | ra2-main-lan-pair (18,000 frames, no CRC difference) |
| Opening Escape > Game Controls > Sound in a network game crashed (r22-sound) | Mac LAN, multiplayer/LAN | scripted, passing | ra2-main-lan-pair |
| Edge scrolling 'way too fast' in a network game (r19-scroll) | Mac LAN, multiplayer/LAN | scripted, passing | ra2-main-lan-pair |
| First LAN lobby issues: name entry, name focus and caret across activation, lobby list background, slot... (r17-lobby) | Mac LAN, multiplayer/LAN | needs his check | lan_lobby.py not yet run on the new port |
| Return in the lobby's chat line sent nothing (C20-chat) | Mac + iPhone, multiplayer/LAN | needs his check | lan_lobby.py not yet run on the new port |
| The iOS app saw the other players but no hosted game, in either direction (C20-ios-host) | iPhone + Mac, multiplayer/LAN | needs his check | lan_lobby.py not yet run on the new port |
| Two games with the same PATH got the same session UniqueID about half the time and ignored each other's... (C20-uniqueid) | iPhone / Mac, multiplayer/LAN | fixed | P12 carried main's C20 install id row |
| iPhone in a LAN game: a notification, Control Center or the background held the game and stalled or... (C22-bg) | iPhone + Mac, multiplayer/LAN | needs his check | lan_pair.py --background not yet run on the new port |
| 'Reconnection Error!' with Stop (once): out of sync between an old Mac build and a newer phone build, not... (C22-reconn) | Mac + iPhone, multiplayer/LAN | needs his check | a LAN game between the old port and the new one was not tried |
| Game froze after the glass toolbar closed (host pause never lifted: a background/focus pause without SDL's... (r27-freeze) | iPhone Duo, iPhone touch | same code | iOS host identical |
| The '...' button stayed pressed (grey) without opening the toolbar, and from then on taps did nothing (r28-stuck) | iPhone Duo, iPhone touch | same code | iOS host identical |
| Two-finger tap (right click: deselect) often came out as a one-finger tap that moved the units (r28-2finger) | iPhone Duo, iPhone touch | scripted, passing | ra2-main-touch-two-finger, ra2-ios-sim-touch-two-finger |
| Two-finger tap kept panning; a single tap once began a selection box at a skirmish's start (UIKit reused... (r31-fingerid) | iPhone Duo, iPhone touch | scripted, passing | ra2-main-touch-two-finger (partly) |
| Two-finger tap did not deselect; the toolbar's A-Move tap gave no order (a selection band instead)... (C8-race) | iPhone Duo, iPhone touch | scripted, passing | ra2-main-touch-two-finger, ra2-main-touch-attack-move, ra2-ios-sim-touch-attack-move |
| The user's choice: Fire/Move/Add/A-Move as one either-or group, A-Move a toggle (it was one-shot) (C10-modes) | iPhone Duo, iPhone touch | same code | touch modes are host code; ra2-main-touch-modern |
| Keys button misplaced right of the menu panel (C12); black safe-area bars in play though the Duo has no cutout (C13-keys) | iPhone Duo, iPhone touch | same code | host code |
| On-screen keyboard did not come up for the player-name fields or the chat line (probably a user request;... (C14-kbd) | iPhone Duo, iPhone touch | needs his check | the game-side hook is carried (glue/ownrdraw_native.inc); no route types into a field |
| Abort Mission screen laid out wrong at render scale 2 (Zoom gestures) (C16-abort) | iPhone Duo, display/zoom/HD | scripted, passing | ra2-main-zoom-menu |
| After Abort Mission the zoomed screen was shown whole: tactical part in the top-left corner, bottom half black (C16-abort2) | iPhone Duo, display/zoom/HD | scripted, passing | ra2-main-zoom-abort |
| The Mac app did not lay the game out at the window's aspect (the iOS app did) (Y8-wide) | Mac, display/zoom/HD | scripted, passing | ra2-main-widescreen |
| Black bars around the 800x600 menus (the user's 'no black bars'; a request) (C12-fill) | iPhone Duo / all, display/zoom/HD | needs his check | enhancements_frames.py not run on the new port |
| Crash when a vehicle took the veteran crate (Allied 'Hail to the Chief'): SIGSEGV in Audio_List_Remove... (N30-crate) | iPhone Duo, audio/movies | needs his check | the sacrifice event offset (N30): the new port's EventClass is typed (lane P8); no crate route |
| Music never played in the menus or in game (the theme restarted every frame) (M20-music) | Mac, audio/movies | scripted, passing | ra2-native-audio |
| Movies did not play properly (Bink tables) (M20-movies) | Mac, audio/movies | scripted, passing | ra2-native-audio (Bink intro with sound) |
| No Windows-specific bug reported by the user: RA2 LAN Mac + PC 'works great', the installed game loaded... (W-none) | Windows PC, Windows | pending | Windows targets come with lane P13 |
| Enhancements page: Cancel drawn as a grey push button in the page body (P12-cancel) | iPhone Duo (this port, lane P12), menus/shell | scripted, passing | ra2-shell-frames |
| Dropdown lists: no frame, rows left on screen after closing, the arrow never pressed (P12-combo) | iPhone Duo (this port, lane P12), menus/shell | scripted, passing | ra2-shell-frames |
| Dropdown click played the generic click instead of the combo open sound (P12-combosound) | iPhone Duo (this port, lane P12), audio/movies | fixed | P12 row |
| Fullscreen 'Automatic' row drawn dark red on touch hosts (P12-auto) | iPhone Duo (this port, lane P12), menus/shell | fixed | P12 row (owner ruling: the played look) |
| Internet button live on the main menu (P12-internet) | iPhone Duo (this port, lane P12), menus/shell | fixed | P12 row |
| Every list box empty (Load/Save/Delete, Keyboard commands); the column lists drew no text (P12-lists) | iPhone Duo (this port, lane P12), saves | scripted, passing | ra2-shell-frames (Load list); the Movies list was still empty: fixed by lane P14 |
| After a load error nothing was left to click (P12-loaderr) | iPhone Duo (this port, lane P12), saves | fixed | P12 row |
| LAN UniqueID could collide between devices (P12-uniqueid) | iPhone Duo (this port, lane P12), multiplayer/LAN | fixed | P12 row |
| 16 ms frame floor applied at every game speed (P12-floor) | iPhone Duo (this port, lane P12), skirmish | fixed | P12 row |
| Pinch zoom off on iOS by default (no 0.5x map zoom); the old app's On was the user's saved choice (P12-pinch) | iPhone Duo (this port, lane P12), display/zoom/HD | fixed | P12 (iOS default on) |
| Play resolution differs from the old app (976x720 vs 1024x720 at 944x634 points, interface 1.25x); not... (P12-res) | iPhone Duo (this port, lane P12), display/zoom/HD | needs his check | 976x720 on the Duo against 1024x720 here; the app log line settles it |
| Home-screen name was 'RA2 Rebuild Trial' (P12-name) | iPhone Duo (this port, lane P12), iPhone touch | fixed | P12 |

## Yuri's Revenge rows (lane P13, 2026-11-24)

The Yuri's Revenge port here is the old port's (vendor/ra2-decomp at the same pin, yr/fixups.json, yr/glue and the
shared host copied from redalert2-decomp's native/yr at 887b1a2e11; lane P15's skirmish-screen fix on top), so every row
is **same code** as the old port; the check column says what ran on this repository. The runs: lane P13's full Mac
ctest at 23ebd62/efc9665 (every yr-native row passed except member-aliases and save, two test-setup faults fixed in
79472b1 and passing on the recheck), the recheck after the P15/P16/P14 merges, and the "RA2 Duo Validation" Simulator
rows of the combined app. The mapping from id to bug follows the old port's notes (docs/yr-port-notes.md lanes Y11 to
N32); where the notes do not say which check covers a row, it is marked needs his check.

Counts (25): scripted and passing 20, same code (no script) 2, needs his check 3.

| Bug | Status on the new port | Check |
| --- | --- | --- |
| Touch pan and zoom gestures during a mission intro's input lock (N32) | scripted, passing | yr-native-locked-input |
| Allied 1's Guardian GIs did not deploy; deployers on command (N13) | scripted, passing | yr-native-deployers |
| Campaign dialog's Difficulty label read MISSING (Y15-diff) | same code | yr-native-campaign-allied/-soviet pass through the dialog; the label text is not checked |
| Immediate defeat in a Yuri's Revenge skirmish (Mac report; a stale build) (N3-mac-defeat) | scripted, passing | yr-native-skirmish-long (ten minutes against three hard AI), -smoke, -score |
| Unit walk and idle animation glitches (Y13-anim) | needs his check | no script compares the animation frames |
| A GI could not garrison a civilian house (N4-garrison) | scripted, passing | yr-native-garrison |
| Internet button live in Yuri's Revenge's main menu (N18-internet) | needs his check | no row clicks it |
| Main menu slide-in drawn wrong (N18-slide) | same code | one-game-ios-sim-menus walks the menus; the slide-in frames are not compared |
| Menu list rows (N18-rows) | needs his check | no row compares them |
| Shell dialogs' backdrop black (Options) or noise (Enhancements) (Y11-backdrop) | scripted, passing | one-game-menu-backdrop, one-game-ios-sim-menus |
| The toggle and the menu's focus after a swap (Y11-focus) | scripted, passing | one-game-toggle (Mac), one-game-toggle-wine (Windows), one-game-ios-sim |
| End-of-game banner dark (N21-banner) | scripted, passing | yr-native-end-banner |
| Command bar thumbs with Zoom gestures on (N27) | scripted, passing | yr-native-command-bar-thumb |
| Bottom command bar missing (Y14-bar) | scripted, passing | yr-native-graphics |
| Command bar's TypeSelect did nothing (N3-typesel) | scripted, passing | yr-native-type-select |
| Save through the Save dialog failed (N3-savedlg) | scripted, passing | yr-native-save-dialog |
| The user's own original Yuri's Revenge saves (N5-origload) | scripted, passing (partly) | yr-native-save (the installation here has no YR saves: that part is skipped) |
| Crash on the Network screen (N3-netcrash) | scripted, passing | yr-native-lan-lobby, yr-native-lan-pair |
| No on-screen keyboard in Yuri's Revenge's name field (N3-yrkbd) | scripted, passing | one-game-ios-sim-menus (both games' name fields; Red Alert 2's fixed by lane P13) |
| Abort Mission when scaled showed the zoomed map small (N21-abort) | scripted, passing | yr-native-zoom-abort |
| Noise drawn while the MCV deploys and a Grizzly first moves (Y14-noise) | scripted, passing | yr-native-graphics |
| Mirage tank camouflage in the wrong colours (N4-mirage) | scripted, passing | yr-native-mirage |
| Mirage tank's first camouflage animation corrupted (N12-mirage1) | scripted, passing | yr-native-mirage (first animation against the second) |
| Oil derrick and power plant animations missing (N4-derrick) | scripted, passing | yr-native-building-anims |
| Skip button left on the start-up loading screen (Y14-skip) | scripted, passing | one-game-ios-sim-menus (failed once in the full run, passed on the recheck: the check reads the host's touch-control states) |
| Windows: a Yuri's Revenge save made by the save menu failed its load in a fresh process, 3,333 unmatched fields and signal 8 (codex1's PC check of 3182afe, P19-yrsave) | scripted, passing | yr-native-save-wine (fails on 3182afe's yr-native.exe as on the PC), yr-native-original-layouts on build/windows-one |
| Windows: Yuri's Revenge stopped ("Red Alert 2 stopped", details "printed to the terminal") in early play after the MCV was selected, speed 2, after a swap from Red Alert 2 (codex1's PC check of 3182afe, P19-yrcrash) | scripted, passing | one-game-yr-play-wine: under Wine the click on the selected MCV trapped in TechnoClass::VoiceDeploy (no return: mingw's clang put int3 there; -fno-strict-return now), and a later fault in fastmath::atan2f (yr/fixups.json); the crash dialog of an engine started by a swap now names the log |

Rows that change with the combined app (the RA2 table above): C23-toggle is **scripted, passing** (one-game-toggle,
one-game-toggle-wine, one-game-ios-sim), C14-kbd is **scripted, passing** in the Simulator (one-game-ios-sim-menus), and
W-none has the Windows builds (ctest label ra2-wine and the YR and one-game rows on build/windows, build/windows-one).

Lane P17 (the user's Duo reports of 2026-11-24): C14-kbd ("Keyboard does not automatically show when selecting text
fields"). Every text field of both games is covered by the two hooks the host asks: the dialogs' fields are one
mechanism in each game (Red Alert 2's Replace_Edit and Yuri's Revenge's FUN_0060F320 hide each dialog Edit behind a
NewEdit list box: the skirmish and LAN player names, the lobby chat line, the save description), answered by
`ra2_native_new_edit_focused` (Red Alert 2's took the rebuild's list box only from lane P13's fix, after the build the
user had); the in-game chat and beacon line by `ra2_native_message_edit_open` (both games). The host now also takes
the keyboard's own hide key (SDL stops text input behind its back) as a dismissal, so a tap on the field brings the
keyboard back; before, it thought the keyboard still shown and never asked again while that focus lasted.
**scripted** in the Simulator: one-game-ios-sim-keyboard (both games: the name field raises it, Keys puts it away, a
second tap brings it back, a tap away puts it away; in play the chat line opened and closed by the harness's
`chat open|close` raises it and puts it away). The toggle flash ("the Play Yuri's Revenge button flashes between menu
changes, after buttons slide out and before they slide in"): the toggle was not among the controls
Enable_Animated_Control rests through the sidebar slide, so a repaint asked during the slide-out drew it alone over
the emptied column (two presents in Red Alert 2, four in Yuri's Revenge, main menu -> Options); fixed in both games
(ra2/fixups.json, yr/fixups.json), **scripted, passing**: one-game-toggle-slide (Mac; fails without the rows).

## Lane P21: missions played to a real win through the step server (2026-11-25)

A pilot: the game paused by default and advanced by a requested number of ticks (ra2/glue/stepserver_native.inc, client
ra2/tools/ra2_step.py), input through the port's SDL input path (clicks on the map, the sidebar's tabs and cameos, keys),
state as text, plans from each mission's own map file read from the installation at run time (ra2/tools/ra2_mapinfo.py)
and from text walkthroughs. Each win is a replay of inputs and ticks (ra2/tests/campaign/*.replay), checked by
`ra2-native-campaign-won-*` (label ra2-campaign-won): the mission's own triggers win it at the recorded tick and its score
screen shows. One path per mission; it does not stand for the variety of real play, and the user's own play stays the
final check.

| Mission | Result | Check |
| --- | --- | --- |
| Allied 1 (ALL01T): Tanya's C4 on the four Dreadnoughts, the fort taken, a barracks, an engineer into the bridge hut, the three bonus Grizzlies through the Soviet base | won at tick 11,007 (about 12 minutes of game time), 50 inputs | ra2-native-campaign-won-all01 (about a minute) |
| Soviet 4 (SOV04S): base built and placed through the sidebar, flak, subs against the fleet, Rhinos massed, the Korean base's construction yard first | won at tick 47,420, about 760 inputs (most of them placement attempts) | ra2-native-campaign-won-sov04 |
| Allied 2 (ALL02S, lane P22): Tanya on the flak cannons, the Academy taken, rocketeers and Grizzlies through the Soviet base | won at tick 30,335 | ra2-native-campaign-won-all02 |
| Soviet 1 (SOV01T, lane P22): the bridge repaired, conscripts massed by attack move, the Pentagon's four parts | won at tick 49,448 | ra2-native-campaign-won-sov01 |
| Soviet 3 (SOV03T, lane P22): derricks, Rhinos, an engineer into the battle lab, the beacon held | won at tick 54,669 | ra2-native-campaign-won-sov03 |
| Allied 3 (ALL03U, lanes P22 and P23): GIs in the bridge houses, Grizzlies massed at home, walked round the Soviet base in short legs, the beacon force-fired by its cell | won at tick 39,041 | ra2-native-campaign-won-all03 |
| Allied 4 (ALL04U, lane P23): rocketeers on the grand cannons, the MCV landed and guarded, ten destroyers up the river, the amplifier force-fired | won at tick 32,994 | ra2-native-campaign-won-all04 |
| Allied 5 (ALL05S, lane P23): Tanya and spies, coils by barrels and C4 during spy power outages, the battle lab spied, both silos C4'd | won at tick 22,647 | ra2-native-campaign-won-all05 |
| Allied 8 (ALL08U, lane P24): Tanya and the GIs along the elevated highway, the compound's barrels, Tanya's C4 on its reactors and the beacon, a base by the arch, prism tanks by short sieges | won at tick 106,268 | ra2-native-campaign-won-all08 |

Parked by lane P22 for a retry (no crash, hang or assertion in any): Soviet 2, Soviet 5 (Allied 3 won by lane P23). Deferred by lane P23, no port fault: Allied 6, Allied 7, Soviet 7 (docs/ra2-port-notes.md, lane P23). Lane P22's missions, wins and shelved ones, with lane-hours: docs/ra2-port-notes.md, "Campaign missions played to wins".

No crash, hang, trap or assertion in either mission or their replays. What the pilot found, all in the scripted-input
path rather than the game (fixed for the step server's runs, RA2_NATIVE_SCRIPT_ONLY=1; the older --input-script tests keep
their behaviour):

| Found | Effect | Status |
| --- | --- | --- |
| The host's event pump runs at most once a millisecond (port/src/user32.cpp PeekMessageA) | at the uncapped loop's speed a scripted event reached the game a frame or more later than written, depending on the machine: a recorded run did not replay the same (likely also why "clicks timed by loop frame are not repeatable" for the gate's skirmish rows, ra2/ra2.cmake) | fixed for script-only runs: the frame's events pumped as they play (platform/sdl/host.cpp frame()) |
| The game's mouse (WWMouseClass) follows the pointer on the mouse thread every 16 ms of wall time | the cursor action a click used could lag the pointer by frames; live (paused) and replayed runs differed | fixed for the step server: the mouse processed once a frame on the game thread (glue/wwmouse_native.inc RB_Process_Mouse_Now) |
| A one-frame click (the input script's `click`) was read one click late | the selection or order happened at the next click | the step server's click holds the button four frames, as a real click does |
| The game's pointer starts at 0,0 until the first mouse message (port/src/user32.cpp `cursor`) | a scripted run that had not moved the mouse edge-scrolled the map to the top left | the step server parks the pointer in the view's middle at the start and after each click |
| `#` starts a comment in input-script lines | an object point written `@#id` became the screen's corner | the object point is `@^id` |
| A building's listed click point is a corner of its picture; a left click on a cell the selection cannot use deselects (the original's behaviour) | clicks on buildings missed and dropped the selection | `@^id` for a building is its foundation's middle |

Other platforms (lane P21): the replays are Mac rows only for now. One try of all01.replay on the Windows build under Wine
11.18 (ra2/tools/ra2_wine.sh as the binary) did not get past start-up in 1,200 s: the fresh prefix's `wineboot -i`
hung for seven minutes and, once ended, the game wrote nothing past MoltenVK's start lines, so whether the replay is
deterministic there is not known yet. The iOS Simulator was not tried (the replay needs no socket, so both rows are a
matter of running `ra2/tests/ra2_campaign.py` with ra2_wine.sh or ra2_ios_sim.sh as the binary once those start).
