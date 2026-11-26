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

Lane P32 (2026-11-26): lane P28's full ctest on a clean main (a360d48, busy Mac at night: other lanes' games running,
tests in parallel, the display possibly asleep) reported one-game-toggle and one-game-saves aborting in Metal ("texture
height zero") when Yuri's Revenge started after the swap from Red Alert 2 (P28-swap0). **Not reproduced**: on a clean
build of a360d48 both passed alone (193 s, 198 s), and with every one-game row and the four yr-native-skirmish rows at
`ctest -j6` beside lane P31's running game; P28's worktree and logs were gone. So no commit is named as the cause (the
combined app's host, yr/combined, last changed in e27a880, lane P13; the host's release for the next engine not since
the import). The only path to that abort the host can
reach is drawing to a window whose drawable has no size (Metal's next drawable for a zero-height layer): the renderer
the swapped engine makes on the kept window, or a window with no screen to draw on. Guarded: `present_now`
(platform/sdl/host.cpp) draws nothing while the render output size is zero, keeps the picture in its texture, logs
`host: the window's drawable is WxH: drawing waits for its size`, and redraws on the window's pixel-size event.
**fixed, unreproduced**: the one-game rows pass with the guard; if the log line ever shows in a run, that run is the
reproduction.

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
| Allied 1 (ALL01T): Tanya's C4 on the four Dreadnoughts, the fort taken, a barracks, an engineer into the bridge hut, the three bonus Grizzlies on the V3s and sentry guns, the GIs in support, Tanya's C4 on the Soviet base | won at tick 13,871 (re-recorded by lane P35 with the frame clock after the MissionControl[MISSION_NONE] fix below; lane P21's recording won at tick 11,007 and goes another way under the fix) | ra2-native-campaign-won-all01 (about a minute); under ASan ra2-native-asan-all01 |
| Soviet 4 (SOV04S): base built and placed through the sidebar, flak, subs against the fleet, Rhinos massed, the Korean base's construction yard first | won at tick 47,420, about 760 inputs (most of them placement attempts) | ra2-native-campaign-won-sov04 |
| Allied 2 (ALL02S, lane P22): Tanya on the flak cannons, the Academy taken, rocketeers and Grizzlies through the Soviet base | won at tick 30,335 | ra2-native-campaign-won-all02 |
| Soviet 1 (SOV01T, lane P22): the bridge repaired, conscripts massed by attack move, the Pentagon's four parts | won at tick 49,448 | ra2-native-campaign-won-sov01 |
| Soviet 3 (SOV03T, lane P22): derricks, Rhinos, an engineer into the battle lab, the beacon held | won at tick 54,669 | ra2-native-campaign-won-sov03 |
| Allied 3 (ALL03U, lanes P22 and P23): GIs in the bridge houses, Grizzlies massed at home, walked round the Soviet base in short legs, the beacon force-fired by its cell | won at tick 39,041 | ra2-native-campaign-won-all03 |
| Allied 4 (ALL04U, lane P23): rocketeers on the grand cannons, the MCV landed and guarded, ten destroyers up the river, the amplifier force-fired | won at tick 32,994 | ra2-native-campaign-won-all04 |
| Allied 5 (ALL05S, lane P23): Tanya and spies, coils by barrels and C4 during spy power outages, the battle lab spied, both silos C4'd | won at tick 22,647 | ra2-native-campaign-won-all05 |
| Allied 8 (ALL08U, lane P24): Tanya and the GIs along the elevated highway, the compound's barrels, Tanya's C4 on its reactors and the beacon, a base by the arch, prism tanks by short sieges | won at tick 106,268 | ra2-native-campaign-won-all08 |
| Soviet 6 (SOV06T, lane P24): base with two tesla coils against the Grizzly landings, submarines and scorpions, the Korean fleet sunk off the base, twenty submarines into Pearl Harbor | won at tick 30,200 | ra2-native-campaign-won-sov06 |
| Allied 10 (ALL10S, lane P25): the MCV by Einstein's lab, refineries, a battle lab and a second war factory on legal origins, prism tanks by short sieges on the three Soviet bases in turn | won at tick 42,660 | ra2-native-campaign-won-all10 |
| Soviet 9 (SOV09U, lane P25): psi troopers on a GI, the engineers (barracks and battle lab), the sniper and the Grizzly in the Light Blue compound; the sniper on the west gate's seals and dogs, a spy into a power plant, the psi trooper on the President | won at tick 27,285 | ra2-native-campaign-won-sov09 |
| Soviet 11 (SOV11S, lane P25): the two starting Kirovs round the map's south-west and north-west edges onto the Kremlin | won at tick 35,900 | ra2-native-campaign-won-sov11 |
| Soviet 12 (SOV12S, lane P26): two refineries and tesla coils against the scripted chrono attacks (Rhinos on the seals' landing craft), a battle lab, eight Kirovs round the south coast to a stage out of the patriots' reach, the Iron Curtain on them eleven cells out, the chronosphere bombed | won at tick 64,411 | ra2-native-campaign-won-sov12 |
| Soviet 2 (SOV02T, lanes P22 and P27): the German refineries by their barrels, a base with two sentry guns by the ore, both derricks taken (the alligators shot first), the Allied base razed by Rhinos (its fire sale ends it), the land mopped up, a naval yard as bait: the last four destroyers came into the bay and died to submarines and Rhinos on the shore | won at tick 46,156 | ra2-native-campaign-won-sov02 |
| Soviet 5 (SOV05U, lanes P22 and P27): conscripts and the starting tesla troopers along the west road, the barrel clusters by the Allied posts shot from four cells, the derrick taken, tesla troopers bought (each becomes Other3's, a house the player commands) and walked round the GI nests, Ivans' bombs on the tower's pillbox, a home guard of conscripts, the tesla troopers into the Paris Tower, which then kills every Allied unit | won at tick 51,032 | ra2-native-campaign-won-sov05 |
| Soviet 7 (SOV07S, lanes P23 and P27): refinery, radar, coils, a war factory and Apocalypse tanks; the six prism tanks met before they reach the construction yard, flak round the lab, sixteen tanks parked round the lab against engineers and spies through the last wave, then a hunt | won at tick 42,844 | ra2-native-campaign-won-sov07 |
| Allied 7 (ALL07T, lanes P23 and P31): rocketeers queued at the start (a second barracks), a Harrier scouting and striking BadGuy1's first two buildings (a sentry gun and a hut by Niihau), the rocketeers on the Soviet MCV as it lands and on the construction yard as it deploys, then on each building left (BadGuy1's buildings all gone wins) | won at tick 8,436 | ra2-native-campaign-won-all07 |
| Soviet 10 (SOV10T, lanes P33, P34 and P36): power, two refineries, flak cannons and flak troopers, a naval yard (rebuilt after each dolphin raid; submarines answer dolphins only by attack moves), ten submarines and four dreadnoughts; the subs clear the beach waters, the dreadnoughts the beach's prism towers and pillboxes and the compound's east tower, three Rhinos landed to map the west tower, the refineries sold for the nuke silo, the nuke beside the west tower and an Apocalypse on it before the repairs, the engineer into the battle lab, the second nuke on the weather control device | won at tick 114,676 | ra2-native-campaign-won-sov10 |
| Soviet 8 (SOV08T, lane P37): the starting army only (two Apocalypses, a Rhino, four desolators, eight conscripts) up the west road, the sentry gun (78,46) on the way, the fence at (82,40-41) shot open, four of Vlad2's power plants shelled from (91,41) until its power (1,050) is below its drain (1,085), so the tesla coils stop; then through the south gate to (99-101,21) and the White House (CAWASH01 at 98,14) shelled | won at tick 5,480 | ra2-native-campaign-won-sov08 |
| Allied 9 (ALL09T, lanes P24, P27, P36, P37 and P38): the village freed by the seals, the five Hack engineers carried in the Night Hawk past the dogs, the flak tracks and the guards north of the base killed, then dropped at (89,98): one into the north reactor (91,106, the player's by its tag), one into Balabos's prism pyramid (87,107, the player's two seconds later); powered by the reactor, the pyramid (MayanPrism, 1,000 a shot) force-fired at each building in reach; a second reactor (86,114) taken puts BadGuy1 below its drain (coils, flak and sentry guns stop); the second pyramid (106,126) and a reactor (108,115) taken by the Grizzlies' escort, the engineer it brings into the construction yard; a barracks, engineers into the depot, radar and war factory, the barrel chain at (85-91,139-144) set off by Rhinos (both south reactors gone, BadGuy1's power 0), V3s and tesla troopers on the rest | won at tick 35,100 | ra2-native-campaign-won-all09 |
| Allied 6 (ALL06U, lanes P23, P27, P40, P41 and P43; the plan from a user-supplied video guide): the MCV deployed outside the Pentagon wall (92,157), refineries and prism towers at the bridgehead against the V3s, the construction yard repaired against the west raids; BadGuy2: eight Harriers on the coil by the yard, 23 Grizzlies on the coil, yard, barracks and war factory in that order (the firesale); BadGuy1: the monument coil, barracks and war factory, then 36 Grizzlies on the compound's two south coils and its construction yard (the firesale); the guard shack (CAGARD01, 34,82) force-fired | won at tick 116,548 | ra2-native-campaign-won-all06 |

Parked by lane P22 for a retry (no crash, hang or assertion in any): Soviet 2, Soviet 5 (Allied 3 won by lane P23, Soviets 2 and 5 by lane P27). Deferred by lane P23, no port fault: Allied 6, Allied 7, Soviet 7 (docs/ra2-port-notes.md, lane P23; Soviet 7 won by lane P27). Deferred by lane P24, no port fault: Allied 9, Soviet 8 (docs/ra2-port-notes.md, lane P24). Deferred by lane P25, no port fault: Allied 11 (docs/ra2-port-notes.md, lane P25). Deferred by lane P26, no port fault: Allied 12, Soviet 10 (docs/ra2-port-notes.md, lane P26). Allied 7 won by lane P31. Soviet 8 won by lane P37. Allied 9 won by lane P38. Allied 12 deferred again by lane P39 (the win also needs BadGuy3's coil, flak cannons and sentry guns gone; docs/ra2-port-notes.md, lane P39). Shelved by lane P31: Allied 11 (a replay of its reveal-time Harrier strike went another way in four of six reruns; docs/ra2-port-notes.md, lane P31). Shelved by lane P40 at its 3 lane-hours: Allied 12 (no chronosphere lived to its first shot on the second pair of Apocalypses; docs/ra2-port-notes.md, lane P40). Allied 6 deferred again by lane P40 (base rebuilt to tick 14,685; docs/ra2-port-notes.md, lane P40). Shelved by lane P41 at 2.15 lane-hours, at a real blocker: Allied 6 (no scripted shortcut; the win needs both enemy houses gone, and a 30-Grizzly strike killed BadGuy2's coil, war factory and barracks but not its construction yard, which rebuilt them; no chrono legionnaires on the sidebar; docs/ra2-port-notes.md, lane P41). Allied 6 won by lane P43. Lane P22's missions, wins and shelved ones, with lane-hours: docs/ra2-port-notes.md, "Campaign missions played to wins".

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
| A wall's name is also its overlay type's (lane P29) | the step server's `buildable NAWALL` said "no building type" | the name is looked up among the building types when the generic lookup finds the overlay |
| A wall overlay keeps the cell's land type clear (lane P30; the original's behaviour: OverlayTypeClass::Read_INI, exact, reads Land= and RULES.INI's walls have none) | the step server's `terrain` and `cell` never showed a placed wall, so lane P29 read a wall placed by a legal click as lost | not a port fault: a live click traced with lldb (EventClass::Execute PLACE, HouseClass::Place_Object, BuildingClass::Unlimbo's wall branch, OverlayClass::Mark, MapClass::Place_Wall) puts the NAWALL overlay on the cell, and the line between two walls fills as in the original. `terrain` now draws a wall overlay as W and `cell` names a cell's overlay (+NAME); **scripted, passing**: ra2-native-walls (two walls by clicks in Soviet 7 and the two cells between them, checked through the step server; it fails on the old output) |
| The step server's socket wait ended on EINTR (lane P30) | a debugger attaching interrupted accept() or read() and the server released the game, which ran on freely (and lost the mission) | retried on EINTR (platform/sdl/host.cpp) |
| Allied 11's recording (lane P31, build/p31/all11/try6.replay, kept outside the repository) preloaded to tick 22,140 left BadGuy2 at power 900 in two reruns and 3,050 in four (the Harrier strike on the barrels never landed) | a recorded win that depends on that strike would not replay | **fixed, scripted, passing** (lane P33): not the click or the camera. The game's clock (timeGetTime, QueryPerformanceCounter: SystemTimerClass, the audio clock, the Bink movies) ran on the wall clock while the step server counts loop frames, uncapped. The reveal's EVA speech, the stream fade and the radar's Bink movie (RadarClass::Update_Transmission, Play_Movie, which holds the scenario while the movie plays) are paced by that clock, so on a busier machine they span fewer loop frames: in two preloads of try6 the hold began at game frame 19,338 and at 19,259 (tick about 19,340 and 19,260) and ended 358 ticks later, and every input after it landed at another game frame (lane P31's two "identical" dumps were taken at tick 19,456, where its preloads ended, before the hold). The same holds for a trigger's Pause for N seconds (ScenarioClass::PauseTimer). Now the step server's runs use a frame clock (RA2_NATIVE_FRAME_CLOCK=1, port/src/port_core.cpp game_clock_ns): from the served scenario's first frame to its end the game's clock advances 16 ms (one SystemTimerClass tick) a loop frame and stands still between frames and while the server waits for a command. New recordings say `# frame clock` and replay with it (ra2_step.py start, ra2_campaign.py); the earlier recordings, made on the wall clock, replay as before. With it, try6 preloaded twice, once with every frame slowed by 3 ms (RA2_NATIVE_STEP_SLOW), gave the same game frame at every 20th tick to 21,256. ra2-native-frame-clock: the clock advances 600 over 600 ticks in a normal and a 5 ms-slowed run (the wall clock gave 55) |
| A transport holding more size than it has pips (Soviet 10, lane P34: an amphibious transport, Passengers=12, with two Apocalypses of Size=6 and an engineer) | the game stopped with EXC_BREAKPOINT in macOS malloc at the next allocation after the transport was drawn (SDL_SaveBMP, AStarClass::Ban_Neighborhood_Subzone_Edges, a Metal command buffer; three runs, ticks 68,248 to 71,651) | **fixed** (lane P34): TechnoClass::Draw_Pips (exact, 0x006D4920) colours the pips in new int[Max_Pips()] from slot GetTotalSize()-1, clamped one past the end, down by each passenger's size, so it writes past the end and below the start; the original's heap took the stray ints, macOS malloc's free lists did not. A fixups.json row puts the array inside a block with room for every write (the pips drawn are the original's). AddressSanitizer on the crashing recording (build/p34/saves/sov10/crash2.replay, outside the repository) found the overflow and the write below the block, and none of them with the row; the six global-buffer-overflow reads ASan also lists (CCINIClass::Get_PipEnum, FootClass::Do_MISSION_ENTER, RadarClass::Init_For_House, two RLE blitters) are left open (lane P35 fixed them: the rows below; FootClass::Do_MISSION_ENTER's was most likely the MissionControl[MISSION_NONE] read: its Normal_Delay reads Rate 16 bytes before the table, in ASan's redzone there) |
| The frame clock slipped after a long wait for a command (lane P34) | a live session whose client paused more than five seconds before a command that reads the game's clock (state's clock line) put the wall time of the wait into the frame clock ("port: frame clock: a frame ran over 5 s"); its replay, which never waits, kept the clock: the two parted | **fixed** (lane P34): the step server marks the wait's end (ra2port_frame_clock(3)) before it runs each command as well as after |
| AddressSanitizer sweep (lane P35): every won replay (21, with sov08, sov10 and all09 recorded after the sweep began) and the ra2-quick, ra2-main, ra2-campaign and ra2-resaves groups on an ASan build of the Mac target (RA2_NATIVE_ASAN=ON) | what ASan reports are original behaviour that the original's Windows layout absorbed: reads or writes outside an object that land on the port's neighbours | the six rows below; with them no replay reports anything and every replay wins at its recorded tick under ASan as in the release build. **Scripted**: ra2-native-asan-all01, ra2-native-asan-sov06 (label ra2-asan, only in an RA2_NATIVE_ASAN build) |
| Falling and just-landed paratroopers, and any object whose mission is MISSION_NONE, behaved by whatever the linker put before the mission table (lane P35) | Allied 1 under ASan: the GIs never shot the Soviet paratroopers in the air (frame 2,548) and the replay went another way. MissionClass::Current_Mission_Control (exact) reads MissionControl[-1] for MISSION_NONE, the 32 bytes before the table: IsNoThreat, IsScatter, IsRetaliate and the mission delays of those objects came from another global (the Mac release build: an empty vector, so IsNoThreat false by luck and IsScatter/IsRetaliate false; ASan's layout: a pointer's high bytes, IsNoThreat true). ASan cannot report it (the bytes belong to another global). The iPhone and Windows builds lay out their globals differently, so this is a plausible cause of the user's unexplained "random failure" in Allied 1 around the paratroopers (Tanya alive, the Soviet paratroopers just dropped), never reproduced on the Mac | **fixed** (lane P35): a fixups.json row (src/mission.cpp) returns the original's record for MISSION_NONE, the bytes before 0x00A403B0 in the original: IsNoThreat, IsZombie, IsRecruitable, IsParalyzed false (COORD_NONE's Z and zero padding), IsRetaliate and IsScatter true (the Aircraft vector's vtable bytes), Normal_Delay and AA_Delay 0 (subnormal doubles); the same on every platform. Owner's ruling: the original's values, so all01.replay was re-recorded (won at tick 13,871; the old recording won at 11,007); the other 20 recorded wins (sov08, sov10 and all09 among them) hold at their ticks |
| CCINIClass::Get_PipEnum read its default name one entry past the pip table (lane P35, ASan, every mission start) | an infantry type made blue by the rules reads _pips[5]: in the original the next table's first name, "Ammo" (0x007D1998), which matches no pip, so it turns green; in the port, whatever followed | **fixed**: a fixups.json row names "Ammo" for that index (src/ccini.cpp) |
| RadarClass::Init_For_House read the 3-byte TooltipColor as 4 bytes (lane P35, ASan) | one byte read past the colour; the fourth byte was never used | **fixed**: a fixups.json row copies the three bytes (src/radar.cpp) |
| RLE_Blit's flat z-shape buffer (_temp_buf, 256 bytes) read past its end (lane P35, ASan: RLEBlitTransDarkenZRead/ReadWrite, building shadows) | a shadow line wider than 256 pixels took its depths from the next .bss bytes (in the original the Bombs vector and constants, never meant as depths) | **fixed**: a fixups.json row makes the buffer 64 KB of zeros (src/blit.cpp); lines up to 256 pixels read what they always read |
| Rally point lines read their dash pattern past its end (lane P35, ASan, Soviet 4) | Tactical::Draw_Rally_Points' _pattern[8] (0x007F6954) is stepped through 16 entries by DSurface::Draw_Masked_Dashed_Line; in the original the next 8 bytes are the same pattern again, in the port whatever followed | **fixed**: a fixups.json row makes the array the original's 16 bytes (src/tactical.cpp) |
| A unit's voxel shadow sampled its compositing surface past the buffer (lane P35, ASan, Soviet 12: heap-buffer-overflow read in _voxel_draw_shadow, row 263 of the 256x256 eight-bit surface) | the original read the Windows heap after the block: values that cannot be recovered and only mask shadow pixels | **fixed**: a fixups.json row reads 0 (the surface's empty fill) outside the surface (src/voxlib.cpp); visual only |

Other platforms (lane P21): the replays are Mac rows only for now. One try of all01.replay on the Windows build under Wine
11.18 (ra2/tools/ra2_wine.sh as the binary) did not get past start-up in 1,200 s: the fresh prefix's `wineboot -i`
hung for seven minutes and, once ended, the game wrote nothing past MoltenVK's start lines, so whether the replay is
deterministic there is not known yet. The iOS Simulator was not tried (the replay needs no socket, so both rows are a
matter of running `ra2/tests/ra2_campaign.py` with ra2_wine.sh or ra2_ios_sim.sh as the binary once those start).
