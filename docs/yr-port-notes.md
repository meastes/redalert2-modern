# yr-native: the native Yuri's Revenge game (lane Y1)

User decision 2026-11-15 (AGENTS.md); plan 1 of `docs/YR_PORT_SIZING.md`. The game is built from
the pinned ra2-decomp tree in `yr/` (never edited) with native/'s platform layer. RA2's `src/`,
its exact build and the `ra2-native` targets are unchanged; `yr-native` is built only with
`-DRA2_NATIVE_YR=ON` (Mac).

    cmake -DRA2_NATIVE_YR=ON -DRA2_NATIVE_YR_MINGW=<llvm-mingw>/generic-w64-mingw32/include <native build dir>
    cmake --build <native build dir> --target yr-native

## Layout

| Path | What |
| --- | --- |
| `vendor/ra2-decomp`, `vendor/tibsun` | pinned copies (commits, licences: `yr/README.md`, `THIRD_PARTY.md`) |
| `yr/tools/yr_units.py` | one native unit per `src/**/*.cpp` and the merged header directory (TibSun headers, ra2-decomp's `include/` over them, as their build), with `fixups.json` and the RA2 tree's port rewrites (`native_units.port`: long, literal suffixes, ra2_ CRT names, window data, ...) |
| `yr/tools/yr_sdk.py` | the Win32/DirectX headers: the local llvm-mingw toolchain's mingw-w64 headers rewritten for the VC6 widths (long -> 32 bits) into the build directory, with the few clashes with the host's C library resolved (BSD socket types renamed, real `__uuidof`, `DLGPROC` returning BOOL, Win32 window-data names kept); never copied into the repository |
| `yr/sdk` | the YR prelude (host C types first), the VC6 CRT declarations as the port supplies them, and `comdef.h`/`io.h`/`mbstring.h` wrappers |
| `yr/fixups.json` | text fixups where clang refuses VC6 dialect, cut functions (assembly) and left-out units (Westwood Online), each with its reason |
| `yr/glue` | native code for what is cut or not matched: the RA2 native tree's C for the TibSun assembly (LCW_Comp, the time-stamp counter), the VC6 CRT names, the host's RA2 hooks, and YR functions written from RA2 twins (`yr_twins.cpp`) |
| `yr/yr.cmake` | the `yr_units_objs`, `yr_host_objs` and `yr-native` targets and the link closure |

## Stage 1 result (2026-11-15)

- All 435 YR units compile for arm64 at -O2 (3 Westwood Online units left out: Internet play).
  33 text fixups, 4 cut functions with native glue.
- `yr-native` links (11 MB). Link closure: **2,534** entries (functions 811: 611 C++, 197 C,
  3 Win32 [VariantInit/VariantClear/VariantChangeType]; data 1,723: 1,294 address-named
  `DAT_`/`PTR_` globals, 284 C++ and 139 C globals, 6 vtables). All but 10 are referenced
  by the YR units (9 by the shared host, 1 by the port: RA2 hooks, supplied by
  `glue/yr_host_glue.cpp`). Of the 811 functions, 269 carry ra2-decomp's address names
  (`FUN_xxxxxxxx`, their unmatched and library code); WinMain itself (0x006BB9A0, 10,261
  bytes) is unmatched there and has an RA2 twin (RA2 0x0068E280).
- Run: the process starts and faults in the static initialisers, before main: MapClass's
  constructor chain (TabClass) compares a sidebar name from a `DAT_` global that is still
  zero. The YR globals have no values yet: the original keeps them in gamemd.exe's data.

## The Win32 headers (ruling 2026-11-15)

The Mac YR build needs the mingw-w64 Win32/DirectX headers of the local llvm-mingw toolchain
(`-DRA2_NATIVE_YR_MINGW`, `private/toolchains/llvm-mingw-*`), the same toolchain the Windows
native build uses. `tools/yr_sdk.py` rewrites them into the build directory at configure time;
nothing from them is copied into the repository.

## Stage 2 (lane Y2, 2026-11-15): from the data load into Init_Game

* gamemd.exe 1.001 is recognised by its code sections (the user's copy with its 4 KB overlay;
  the 2-byte no-CD patch at 0x0052F949 is undone for the .text check); `data/globals.tsv`
  (from ra2-decomp's pins, `tools/yr_pins.py`) and the address names give each global its
  address. `tools/yr_data.py` makes storage for every global no unit defines and the table the
  port fills from gamemd.exe (546 globals) before the YR static initialisers, which
  `tools/defer_initializers.py` holds back until then. Class-typed globals no unit defines
  (the INI objects, the random generator, BlubCell) are constructed in `glue/yr_globals.cpp`.
* Address names bound to the definitions at those addresses (`tools/yr_aliases.py`), and the
  few functions the tree defines under one name and calls under another
  (`data/name_aliases.tsv`, `glue/uicmnds2_native.inc`, `glue/ccini_native.inc`).
* Window procedures take full-width message parameters (ruling: WPARAM/LPARAM as the RA2
  tree's host passes them). teleport.cpp's bridge test is `true`: gamemd.exe's 0x00718260 makes
  no call to 0x00483610.
* Native drafts of unmatched functions, appended to their units (`fixups.json` "append"; each
  file is labelled NON-EXACT): WinMain (`glue/startup_winmain.inc`), Allocate_Surfaces,
  ConvertClass::CreateFromFile and FUN_00534E50 (`glue/init_native.inc`), Init_Game
  (`glue/init_game_native.inc`); the import thunks the tree calls by address
  (`glue/yr_import_thunks.cpp`); the CPU probes (`glue/yr_asm.cpp`).
* Fixed on the way: Darwin defines BIG_ENDIAN, so base64.cpp's packet union took the
  big-endian layout and the encryption key decoded to nothing (LANGUAGE.MIX then failed
  intermittently); GUID globals (CLSIDs, IIDs) are now loaded.

Run: `yr-native --data-dir <RA2+YR install> --run-to=menu` creates the "Yuri's Revenge" window
(800x600, 16 bpp), its surfaces and mouse, loads the bootstrap and language mixfiles and the
palettes, and stops in Init_Game at FUN_00754CB0 (3,290 bytes, TS Init_Voxel_Matrices; RA2
twin `src/recovered/voxel.cpp` Unknown_00718060).

## Stage 2, lane Y3 (2026-11-15): to the main menu

* The pin moved to ra2-decomp `8719494` (`yr/README.md`, `THIRD_PARTY.md`): 17 of our stops had been
  matched upstream since `f8d8d53` (BuildingTypeClass::Read_INI, Main_Loop, Message_Input,
  VoxelLibrary::Read_File, ...).
* `tools/yr_aliases.py` also binds calls made by a matched function's `functions.csv` name
  (Game::AudioGetTime, INIClass::GetSection, ...; 22 bindings, listed in the generated
  `aliases.tsv`); `data/name_aliases.tsv` gained the ones read from gamemd.exe's calls
  (Toggle_Cameo_Text, ObjectTypeClass::One_Time, IonStormClass::Is_Ion_Storm_Active,
  Vox_From_Name, Fill_Surface_Rect, OwnerDraw::Dialog_Message_Handler, FUN_0053D330).
* Native drafts (NON-EXACT, `fixups.json` "append"): FUN_00754CB0 (voxel matrices, RA2 twin),
  SidebarClass::Reposition_Sidebar, IonBlastClass::One_Time, the TechnoTypeClass constructor
  (gamemd.exe's stores mapped to field offsets), Read_PCX_File (TibSun text), the Adjust_Color
  tables (`yr_asm.cpp`), and the owner-draw layer the menu needs (`glue/ownrdraw_native.inc`):
  UI::StandardWndProc, OwnerDraw::DefaultHandler (TibSun CtrlProc via the RA2 draft),
  OwnerDrawButtonHandler, StaticHandler, ResetControlDrawModeAndTimerProc, FUN_0060C0C0,
  FUN_0060B950, DrawCampaignMenuTransition (the RA2 tree's matched twin). Bindings:
  Game::CallBack, Game::PlayMovie / FUN_005BF260, the game-mode initialisers' Allocate.
* Pointer-width fixes: the audio memory pool and stream casts, ToolTip::Text (intptr_t), and the
  owner-draw record (OwnerDraw::WinData): the unit reaches fields both by name and at raw 32-bit
  offsets; natively the raw ones go through the union's pointer-free Dialog view, the record
  constructor's font by name, and the statics' pointer fields (+0x58, +0x60, +0x74, +0x78, +0xA4)
  sit in a side table by window.
* Research helpers: `tools/yr_dis.py` (gamemd.exe disassembly with the tree's names),
  `tools/yr_layout.py` (32-bit field offsets of a YR class), `tools/yr_ctor_stores.py`
  (a constructor's stores by field), `tools/yr_run.sh` (a run with window dumps).

Run: the "Yuri's Revenge" main menu shows (Single Player, Internet, Network, Movies & Credits,
Options, Exit Game, Version 1.001) with its TV movie playing, and Main_Menu's loop runs. Link closure: 343 entries.

## Stage 2, lane Y3 (2026-11-15): first playable, a skirmish on the Mac

Main menu -> Single Player -> Skirmish setup -> Start Game -> the skirmish loads and runs: the
tactical view (tiles, shroud, overlays, terrain, buildings, infantry, voxel vehicles), the
sidebar, the radar frame and the credits; the units move, take selection, the H (base) and D
(deploy) keys; Escape opens Game Options, Abort Mission and its question return to the skirmish
setup. `ctest -R yr-native-skirmish-smoke` (yr/tests/yr_skirmish_smoke.py; a -DRA2_NATIVE_YR=ON
build, RA2_NATIVE_YR_DATA_DIR set to the RA2 + YR folder) runs that route with scripted input.

Fixes of note on the way (all NATIVE, yr only): yr_data.py loads enumerations, the theater
table and VoxelRenderOrientations from gamemd.exe and sizes arrays bounded by enumerators (the
RadiusTable overran MainWindow); tile sets' record slots (ra2port::IsoTileRef), the ABuffer/ZBuffer
ring positions and RLE_Blit's buffer positions at pointer width; the radar animation shape and the
command-bar button walk no longer go through ints; the positional-sound pool sized from the native
SoundNode; the voxel span steps through ra2port::x87_ftol. Drafts (NON-EXACT, from the RA2 twins,
TibSun's text or gamemd.exe's code): Tactical::Render (YR's foreground pass), Draw_Tile, the
sidebar and strip AI/Draw_It/Recalc, Techno_Draw_Object, the voxel render, the zone and subzone
spans, Execute_DoList, the EVA speech service, the display cursor and Mouse_Left_Up, and others in
yr/glue/*.inc; many traps were gamemd names of functions the tree has under another name
(data/name_aliases.tsv).

Known gaps: a skirmish abort returns to the setup without a score screen (YR does that; lane Y4
reaches the score screen with a lost game). Partial drafts: Wait_For_Players_To_Load (the network wait),
FUN_0063AB60/FUN_0063A5A0 (planning mode), LoadProgressManager::Draw (campaign screen),
StripClass::Draw_It (the observer list), the strip's production-abandon events in Recalc.
(Can_Deploy_Now and the GeneratePalette fault: fixed by lane Y4, below.)

## Stage 2, lane Y4 (2026-11-15): a lost skirmish and the score screen

Main menu -> Skirmish (Short Game) -> a band selection and D (the MCV deploys into a construction
yard) -> H -> Sell -> the construction yard sold -> the game is lost -> the Skirmish Score screen ->
Continue -> setup -> Back -> Single Player -> Back -> main menu. `ctest -R yr-native-skirmish-score`
(yr/tests/yr_skirmish_score.py) runs that route (same build and data conditions as the smoke test).

Fixes of note (all NATIVE, yr only):
* No unit ran its AI: the TechnoTypeClass constructor draft stored gamemd.exe's +0x234 into
  IsHasRadialIndicator; gamemd.exe's ObjectClass::Unlimbo reads IsSentient there, so units never
  joined the logic list and queued missions never started. The same draft left PaletteFile unset:
  the intermittent ColorScheme::GeneratePalette fault during a load (garbage palette names whose
  negative scheme hash indexed outside the bucket table).
* The view scrolled north-west for good: Blit_Sidebar called the mouse through a stand-in class
  whose MSVC Draw_Mouse slot is Set_Mouse_Point in the Itanium layout (two destructor slots).
* The drive tracks were empty (RawTracks holds pointers yr_data.py cannot load): TibSun's tables.
* Address names of members of unit-defined globals (DAT_00886F94 is SidebarRect.Y) had storage of
  their own: fixups map them to the members; yr_data.py lets an unbounded scalar array run over
  the address names of its later elements (DAT_007ED3CC / MapClass::RadiusCount).
* The score screen sorted its pointer table with a 4-byte element size.
* Vtable-slot and second-name bindings read from gamemd.exe's vtables and calls
  (data/name_aliases.tsv): TechnoClass slots 0x3D0 and 0x3E0-0x3EC, GetFireErrorWithoutRange,
  ObjectClass::Direction, SidebarClass::Add, UnselectAll, the area damage's three names.

NON-EXACT drafts (each labelled in its file): from gamemd.exe's code TechnoClass::Can_Deploy_Now
(YR's 891-byte version, replacing TibSun's text), InfantryClass::Assign_Destination (TibSun's text
with YR's changes), FootClass::Active_Click_With (TibSun's text with YR's cases); from TibSun's
text WalkLocomotionClass::Movement_AI, DriveLocomotionClass::While_Moving, AStarClass::
Find_Path_Regular, InfantryClass::Per_Cell_Process, BuildingClass::Exit_Object and
Do_MISSION_DECONSTRUCTION, HouseClass::Can_Build (with the RA2 twin's YR-era tests),
Where_To_Place_Building and Can_Build_Here, CellClass::Reduce_Tiberium, the area damage
(Explosion_Damage), XSurface::Plot_Line, BounceClass::AI, Tactical::Draw_Tiles(Cell).

Known gaps: the selection brackets are not drawn; TibSun-text drafts are not compared with
gamemd.exe yet (YR's sell survivors, YR's area damage details); the AI opponent's further play
will reach more unmatched functions.

## Stage 2, lane Y6 (2026-11-16): longer skirmishes

`ctest -R yr-native-skirmish-long` (yr/tests/yr_skirmish_long.py): ten minutes of a skirmish against
three hard computer opponents without a trap or a fault (same build and data conditions as the
smoke test). The skirmish setup comes from the write folder's RA2MD.INI, which the test writes:
`[Skirmish] SlotNN=<type>,-2,-2` with type 4 hard, 5 normal, 6 easy and 1 closed (the setup's
own encoding, skirmish.cpp Skirmish_Slot_Type); `--opponents` and `--difficulty` choose them.
`RA2_NATIVE_TRAP_CONTINUE=1` lets a research run go on past traps and lists them all.

The trap in HouseClass::AI_Build_Defense about 15 s into a game was not caused by RA2MD.INI: the
function was simply unmatched, and which unmatched function an AI reaches first changes from game
to game (the start positions and the random seed differ per run); the runs that kept a write
folder happened to reach it first, runs without one reached UnitClass::Do_MISSION_HARVEST first.

NON-EXACT drafts (each labelled in its file), in the order long games reached them:
* from the RA2 twins' text in the YR tree's names, with YR's changes read from gamemd.exe:
  HouseClass::AI_Build_Defense (YR weighs the candidates by their defense value alone),
  UnitClass::Do_MISSION_HARVEST (a slave miner hands the mission to its slave manager; an idle
  harvester goes for repair when a repair bay is reachable), ParticleSystemClass::Smoke_AI and
  Spark_AI, SuperClass::Launch (the RA2 twin's Place plus YR's psychic dominator, spy plane,
  genetic mutator, force shield and psychic reveal, and YR's chrono warp, iron curtain and paradrop
  changes), the area damage (Explosion_Damage: now the RA2 twin's warhead spread instead of
  TibSun's fixed 1.5 cells; TibSun's deforming, cliff collapse and fire spread left out);
* from gamemd.exe's code alone (YR-only): AircraftTrackerClass::FillCurrentVector, the slave
  miner's two state machines (SlaveManagerClass FUN_006AF6C0 and FUN_006AFD60);
* bindings: BuildingClass::Fetch_Super_Weapon(2) are the tree's FirstActiveSWIdx and
  SecondActiveSWIdx (data/name_aliases.tsv).

Fixes: the radio's message parameter is pointer-sized natively (fixups.json; it carries an object
pointer for RADIO_MOVE_HERE, which faulted when a harvester docked at a refinery); LParam is
defined in glue/yr_globals.cpp.

Selection: YR has no TibSun-style brackets for units; a selected unit shows the pip board
(pipbrd.shp) under its health pips, a selected building its pip bar along the foundation's top
edge inside the white corner brackets. Lane Y3's TibSun text drew from ObjectTypeClass::SelectShapes
and PipShapes, which YR never loads, and placed the building bar at a zero length.
TechnoClass::Draw_Health_Bar is now written from gamemd.exe (FileSystem::PIPS_SHP, PIPBRD_SHP,
PixelSelectionBracketDelta, YR's pip frames 16-18 and its pip rules).

TibSun-text drafts compared with gamemd.exe (task list of lane Y4):
* BuildingClass::Exit_Object: YR passes the preferred cell to Find_Exit_Cell for vehicles only;
  infantry and the other war factory get no preferred cell. Fixed.
* The sell survivors (Do_MISSION_DECONSTRUCTION): YR leaves survivors unless the building both has
  an archive target and undeploys (TibSun: unless either), so a sold construction yard leaves
  survivors; a building that absorbs units or infantry lets them out, and a garrisoned one its
  occupants. Fixed.
* The area damage: TibSun's one cell and neighbours vs YR's warhead spread. Redone (above).
* HouseClass::Can_Build: YR also allows what a secret lab of the house gives, and refuses a
  building whose super weapon can be switched off from the setup when super weapons are off
  (unless it is one of the rules' BuildTech). Added; the rest (prerequisites, the computer's
  shortcut, build limits) matches.

## yr-native on iOS on its own (lane Y7, 2026-11-16)

`native/build-ios.command [simulator|device] --yr` builds Yuri's Revenge's own iPhone/iPad app,
`build/ios-yr-<platform>/YR.app` (bundle id `io.github.ra2native.YR`, home-screen name "Yuri's
Revenge", the placeholder icon on purple: `make_icon.py --yr`), from the same tree as the Mac
`yr-native` (`-DRA2_NATIVE_YR=ON`; llvm-mingw's headers from `RA2_NATIVE_YR_MINGW`, found under
`private/toolchains` by default). RA2.app is unchanged: the iOS bundle block of
`native/CMakeLists.txt` is the function `ra2_ios_app(target exe id name [icon args])`, called for
`ra2-native` and (in `yr.cmake`) for `yr-native`. Signing and the bundle audit are RA2.app's
(`ios_sign.py` takes the bundle id from the app's Info.plist; `audit_ios_bundle.py --executable YR
--bundle-id io.github.ra2native.YR`, with `RA2_GAMEMD_EXE` sampling gamemd.exe). Device:

    RA2_IOS_TEAM=<team id> native/build-ios.command device --yr
    xcrun devicectl device install app --device <udid> build/ios-yr-device/YR.app

What it took:

* **The system's exports.** `yr_data.py` and the YR `link_closure.py` call now get the iOS SDK
  (`--sdk`): without it they read the Mac SDK, UIKit's `NSFontAttributeName` and
  `UIFontWeightSemibold` became trap stubs, and SDL's launch screen crashed before `main`.
* **Asserts.** The iOS build has no build type: the YR units get `NDEBUG`, as gamemd.exe and the
  Mac Release build (else `Assert_Failure` was one more trap).
* **Chained fixups.** Nothing to do: `__DATA,__yr_init_func` holds rebased pointers on iOS 17
  (281 initialisers run after the data load on the Simulator).
* **Import.** `app_mode.hpp` builds per game: YR's app copies gamemd.exe (not game.exe),
  `ra2md/langmd/multimd/thememd.mix`, `expandmd*`, `mapsmd*`, `movmd*`, `ecachemd*`, `elocalmd*`
  beside RA2's MIX files (28 files, 1.88 GB from the user's installation), requires the eight base
  MIX files and names Yuri's Revenge in its messages; the screen's resolution goes into RA2MD.INI;
  writes go to `Documents/Game/YR`.
* **Sound.** `DAT_00816514` (the sound system master's pointer, passed to AudioLoadSystem) is bound
  to drv_dsnd.cpp's `AudioSystemDSD` (`data/name_aliases.tsv`): gamemd.exe's copy held 32-bit
  addresses, so YR crashed in Init_Game whenever sound was on (every Mac test passes `-noaudio`;
  the Mac app had the same crash). YR now plays with sound.
* **Touch.** The RA2 touch layer is the host's and applies as it is; YR needed the game-side
  hooks RA2 has in native/link: `glue/yr_app_hooks.cpp` gives the map pan
  (`ra2_native_scroll_map`, Tactical::Set_Tactical_Position) and the settings' save, and the
  game loop (conquer.cpp, fixups.json) calls `yr_native_main_loop`, which marks play
  (`ra2_enh_in_play`: the toolbar shows in play only) and runs the host's frame (`ra2port_frame`:
  held buttons, pans, frame-numbered script lines, the stats line). Classic/Modern controls are
  the Enhancements page's (RA2's native/link): not wired into YR yet (YR plays Classic; lane Y8
  wired them).
* **Player input traps.** A tap on a sidebar icon reached `SelectClass::Action` (a trap: the Mac
  runs never built anything by hand): adopted from a newer ra2-decomp commit where it matches
  exactly (`glue/sidebar_native.inc`, with `SuperClass::Impatient_Click`). An aircraft flying to a
  destination reached `FlyLocomotionClass::Nearing_Target`: a draft from the RA2 twin and
  gamemd.exe (`glue/fly_native.inc`, non-exact). Computer players' units reached
  `HouseClass::Random_Cell_In_Zone` (Where_To_Go; TibSun's text with gamemd.exe's changes,
  `glue/house_native.inc`), deployed slave miners `TechnoClass::ScanForTiberium` (bound to the
  tree's `TechnoClass::FUN_0070F8F0`, gamemd.exe's BuildingClass vtable entry) and their slaves
  `SlaveManagerClass::FUN_006B0880` (from gamemd.exe, `glue/slavemanager_native.inc`).
* **Map's members named as globals.** The tree declares the pending placement and the interface
  modes (0x0088098C..0x008809A0: Map 0x0087F7E8 + 0x11A4 PendingObjectPtr ... +0x11B8
  IsTargettingMode) and the placement copy (0x00880978..0x00880980) as separate `int` globals;
  natively those are other storage than Map's members, so a building picked up for placement never
  reached the placement code (and its pointers were cut to 32 bits). fixups.json points
  Manual_Place, TAction_CLEAR_MODES, the coop setup, super.cpp and Tactical at the members. Others of
  the kind remain (the radar's 0x008809F4 LastDrawRect in sidebar.cpp, movie.cpp's 0x008809D8): not
  on the play path checked here.

Simulator route (`yr/tests/ios_sim_skirmish.py`, not a ctest: "RA2 Duo Validation" only, booted
and shut down by the script): the first start imports the files (`RA2_NATIVE_IMPORT`), then taps
through the menus to a skirmish and plays with the touch gestures (swipe pan, hold-and-drag box,
the toolbar's Deploy, Guard and A-Move, taps on sidebar icons and the map); `--score` adds a
second start that loses a short game and goes through the Skirmish Score screen back to the main
menu. Window dumps are the pictures (`simctl io screenshot` shows black for the game's Metal layer).
Results (2026-11-16, Duo Simulator, 872x600, sound on, about 62 frames a second, 101-106 MB): the
import, the menus, the skirmish, the score route back to the main menu; 300-360 s of play without a
trap after the fixes above, with the MCV deployed and power plants built and placed by taps. Open:
one SIGBUS in CellClass::Draw_Shroud_Or_Fog_Shape (glue/cell_native.inc, the shape copy) at a
game's start in one run of ten, not seen again (found and fixed by lane Y8); computer players reach more unrecovered functions
the longer a game runs (each a trap, as on the Mac).

## yr-native on iOS, second stage (lane Y8, 2026-11-16)

* **The SIGBUS at a game's start** (1 run in 10 on the Simulator, CellClass::Draw_Shroud_Or_Fog_Shape):
  `FileSystem::LoadFile` returns an `SHPReference` for a `.SHP` name (gamemd.exe 0x005B40B0), not the
  file; lane Y3's draft (TibSun's text) read that object as a ShapeSet, so its frame rectangles came
  from the reference's own pointers and the heap past it, and its pixel pointer was the object plus a
  garbage offset. Rewritten from gamemd.exe 0x0047EFE0 (`glue/cell_native.inc`, NON-EXACT):
  `SHPStruct::GetFrameBounds` and `GetPixels`, which load the file behind the reference, and the two
  loops as gamemd.exe has them. The shroud's soft edges are now drawn (they never were).
* **Controls: Classic / Modern and Queue five with Shift** (`glue/yr_enhancements.cpp`, native
  additions): RA2's rules (native/link/native_modern_controls.cpp, native_queue_five.cpp) on the YR
  tree's classes, hooked by fixups in ScrollClass::Message_Handler 0x006930A0 (left and right
  release), DisplayClass::Mouse_Left_Release 0x004AB9B0 (the band's walk), RadarClass::RTacticalClass::
  Action 0x006539D0 (the radar's buttons) and the sidebar's two PRODUCE adds (`glue/sidebar_native.inc`).
  Yuri's Revenge's own Shift right click on a build icon already abandons every queued copy
  (ABANDON_ALL), so Queue five changes only the left click. The touch layer needs nothing new: a tap
  is a left click, a two-finger tap or a held release a right click, and the toolbar's Fire, Move,
  Add and A-Move hold the keys the game reads. RA2's tap across player-controlled houses (lane C18,
  a campaign case) is not carried over.
* **The Options dialog and the Enhancements page.** The main menu's Options was a link-closure trap
  (FUN_0055FDB0, the dialog procedure of template 0xD5): drafted from gamemd.exe with the RA2 twin's
  behaviour row as the text (`glue/mainopt_native.inc`, NON-EXACT; YR sets the chosen resolution
  without RA2's test dialog). The port already appends the ENHANCEMENTS button to 0xD5 (gamemd.exe's
  template is read the same way); two fixups in ownrdraw.cpp put it and the page's Ok, Original,
  Enhanced and Cancel in the right-hand column. The page (`Yr_Run_Enhancements`) runs RA2's page
  logic on the YR dialog code and lists the rows Yuri's Revenge uses: Fullscreen, Vertical sync,
  Picture scaling, Menu fill (all the host's), Queue five with Shift and Controls (lane N18: Map zoom and
  the zoom row too; lane N19: Game interface size). The settings are
  the app's own file, so on iOS YR.app keeps its own choices.
* **Not shared, and why** (updated by lane N18, 2026-11-19). Map zoom and the zoom row (Trackpad and
  wheel zoom, Wheel zoom or Pinch zoom by platform) are now Yuri's Revenge's too: `glue/yr_map_zoom.cpp`
  is RA2's link/native_map_zoom.cpp on the YR tree's classes, hooked by 18 fixups (yr/fixups.json, lane
  N18): DisplayClass::Set_View_Dimensions 0x004A8960 keeps the zoomed view; the sites that take the
  view's width or right edge as the sidebar's x (SidebarClass 0x006A5130 and its five blits, RadarClass::AI
  0x00653850, its Reposition_Sidebar and mouse test, the command bar's area, the mouse on the sidebar
  surface, CCToolTip's Update, Reset and Draw) ask Yr_Sidebar_X; Start_Scenario 0x00683AB0, Get_All
  0x0067E730 and Main_Loop's Do_Win/Do_Lose take or drop the zoom as RA2's do; the gestures' steps are
  applied each loop frame (yr_app_hooks.cpp). RA2's render scale (zooming out below 1x draws a game screen
  twice the window's, with its own in-play dialog layout) is Yuri's Revenge's too since lane N20 (below), so
  the gestures zoom between 0.5x and 3x. ctest `yr-native-map-zoom`.
  **Game interface size** is Yuri's Revenge's too (lane N19, 2026-11-19): `glue/yr_interface_size.cpp`
  is RA2's link/native_enhancements.cpp (C3, C7) on the YR tree's classes, hooked by 5 fixups and two glue
  lines. The sidebar surface is made Yr_Sidebar_Height rows tall (the host's ra2_enh_sidebar_height: the
  screen's height over the size, at least 360) at its three allocations (WinMain 0x006BB9A0's
  startup_winmain.inc, Change_Display_Mode 0x00560BF0, Get_All 0x0067E730); SidebarClass 0x006A5130's two
  layouts size the sidebar from Yr_Sidebar_Bottom (N18's, now less the rows the surface is shorter), so the
  cameo strips and scroll buttons lie inside it; the host's split layout (shared) shows the column larger.
  Where YR differs from RA2: its interface art layout FUN_0072FC60 places the in-play command bar
  (TabClass's DAT_00B0FC58..6C) and the in-play dialogs' column (ownrdraw.cpp, FUN_0072F540) from the
  screen's height, while the sidebar in play is laid out from SidebarRect; so that layout keeps the
  screen's height: the command bar stays at the bottom, the in-play dialogs (shown whole while up) draw
  their column to the bottom, and no full-height relayout around them is needed (RA2's
  Native_Dialog_Screen exists for its render scale and its shortened art layout). A new game at an
  unchanged screen size (the menus' 800x600 = the game's) changes no mode, so Start_Scenario 0x00683AB0's
  and Toggle_Display_Mode 0x006842F0's mode checks also change the mode (to the same size) when the
  sidebar surface's rows differ from what the size asks now (Yr_Sidebar_Remake): Change_Display_Mode
  makes the surfaces, the view, the sidebar layout and its buttons again (RA2's Native_Sidebar_New_Game).
  ctest `yr-native-interface-size` (yr/tests/yr_interface_size.py): from a fresh settings folder, Options ->
  Enhancements -> Game interface size 1.5x -> Ok -> a skirmish: the new game's sidebar surface 400 of 600
  rows, play split with a 252-point column in an 884-wide window, the mapping check, the column drawn to
  the bottom and the command bar along the map's bottom, the in-game menu shown whole; a run with the
  defaults unchanged.
  **HD vehicles** is Yuri's Revenge's too (lane N19): `glue/yr_hd_vehicles.cpp` is RA2's
  link/native_hd_vehicles.cpp (C16, C17, C23) on the YR tree's classes, hooked by 10 fixups and two glue
  lines. The YR tree's renderer is TibSun's, the same shape as RA2's: VoxelDrawSystem's _Render (0x007542F0,
  0x00754510) runs the sub-pixel passes after its 1x draw (Yr_Hd_Voxel_Replay is its loops), VoxelLibrary::
  Render_Object 0x00756590 and Render_Shadow 0x00756860 take the passes' start offsets (after the tree's
  x87_ftol rewrite), Bit_Blit 0x004373B0 is renamed Bit_Blit_Core_Native and wrapped (YR's Bit_Blit takes a
  tint: the Nx rows go through the blitter's Blit_Copy_Tinted then), DSurface::Blit_From 0x004BB0D0's Blt
  carries the Nx pixels, the voxel and shadow caches are off while it is on (Techno_Draw_Voxel 0x00706640,
  Techno_Draw_Voxel_Shadow 0x00706BD0), the renderers name their owner (techno_native.inc's
  Techno_Render_Voxel_Object, Techno_Render_Voxel_Shadow 0x00707280), and the setting is taken where N18 takes
  the zoom (Start_Scenario, Get_All); 3x falls back to 2x in a network game (Session.Type). One difference in
  the code: YR's depth and alpha buffers hand the blitters addresses (Get_Buffer_Offset, Wrap_Overflow), so
  an Nx row's depth and alpha values are laid from the row's own place in the buffer and put back after the
  row (RA2's used the buffer's head). The host's overlay is shared. ctest `yr-native-hd-vehicles`
  (yr/tests/yr_hd_vehicles.py): a skirmish at 2x with Map zoom 1.5x and one at 3x, the view scrolled: every
  count has renders and Nx blits, vehicles shown at Nx (about 8,000 screen pixels), mismatched 0. The YR
  page now shows every row. Map zoom below 1x: lane N20 (its section). Mod is the host's row and is shown in both games; widescreen on the Mac has no
  row and is RA2's (YR's shell runs in its own 800x600 mode).
* **Long games had one opponent.** The skirmish setup's map 0 (The Alamo) is a two-player map, and
  the setup closes every slot past the first opponent: lane Y6's ten-minute Mac runs and lane Y7's
  Simulator runs played one computer player. `yr_skirmish_long.py` and `ios_sim_skirmish.py
  --opponents N` now pick map 30 (a four-player map) for more than one opponent; `--trap-continue`
  lists every trap of a research run, `--enhancements` sets the settings for one run.

* **wpaudio's device, EVA and music (sound on).** Every Mac test passes `-noaudio`; with sound on,
  Exit Game crashed in AudioCloseDown (and so did the one-game app's swap away from Yuri's
  Revenge). drv_dsnd.cpp's header (auddsnd.h) spelled the members of AudioSystem, AudioDevice,
  AudioChannel, AudioSample, AudioServiceInfo and AudioSearch it does not use as 32-bit gaps, so
  natively its load wrote numUnits = 1 over the low half of AudioSystem::master and the real
  numUnits stayed 0: wpaudio's device never opened natively, so the EVA voice (VoxClass's stream)
  and the music (ThemeClass's stream) were silent, and AudioUnloadAllSystems called through
  0x1_00000001. Fixed (fixups.json, yr/data/name_aliases.tsv), each found by running with sound
  on (lldb, guard malloc):
  * auddsnd.h's views and audplay.cpp's own AudioChannel view (byte pads) spelled as the real
    members at native widths, and audchan.h naming the three members the driver keeps in its
    padding (a probe compiled both views: equal offsets);
  * theme.cpp and vox.cpp declared the streamers' attribute setters and the attribute globals as
    int (the pointers were cut to 32 bits: AudioAttribsApply faulted);
  * Is_Speaking (Select_Game waits on it at Exit Game) is the tree's FUN_007529E0 (bound);
  * the speech queue's VoxNode was allocated with its 32-bit size 0x20 (vox.cpp);
  * ShapeSet's Get_Data, Get_Rect, Get_Color, Is_Transparent and Is_RLE_Compressed are SHPStruct's
    GetPixels, GetFrameBounds, GetColor, HasTransparency and HasCompression (gamemd.exe calls those;
    the tree's inlines and lane Y3's Get_Color draft read an SHPReference's own bytes as frame
    records: BuildingTypeClass::Get_Draw_Rect and the overlays' radar colours read past it). The
    same cause as the shroud's SIGBUS above.
  With sound on and guard malloc, the smoke route (a skirmish, Abort Mission, back to the setup)
  runs clean, and Exit Game returns from WinMain. Simulator (RA2YR.app, `--game=yr`, sound on):
  600 s against three hard computer players, no trap, 62 frames a second, 122 MB; the toggle
  from YR's main menu to Red Alert 2 and a skirmish there, with sound.

ctest `yr-native-controls` (yr/tests/yr_controls.py): a skirmish with Modern and Queue five (a double
click on a tank selects the tanks on the screen, a right click on open ground gives a move, Shift
with a click on the build icon queues five), the same with the defaults (none of those), and the
Options dialog -> Enhancements -> Enhanced -> Ok -> Main Menu.

Simulator ("RA2 Duo Validation", 872x600, sound on): 600 s against three hard computer players on
map 30 without a trap or a fault (`--trap-continue`: none reached), about 62 frames a second,
107-113 MB; 300 s with Modern and Queue five (the two-finger tap's move logged); two earlier 600 s
runs (one opponent, map 0) also clean. Mac: `yr-native-skirmish-long` with three real opponents,
600 s, no trap.

## One game (lane Y5, 2026-11-16): Red Alert 2 and Yuri's Revenge in one app

User design (2026-11-15): one game. A Red Alert 2 / Yuri's Revenge toggle at the bottom left of the
main menu, away from its buttons; the deeper menus are the selected game's; both games in one app
with one engine active at a time; the toggle swaps engines in the process and lands on the other
game's main menu without its intro; the intro plays once at start-up, for the game chosen last,
which the settings keep. Built with `-DRA2_NATIVE_COMBINED=ON` (with `-DRA2_NATIVE_YR=ON` and
`RA2_NATIVE_LINK_CLOSURE`; Mac); off by default, and `ra2-native`, `yr-native` and their tests
are unchanged.

    cmake -DRA2_NATIVE_YR=ON -DRA2_NATIVE_COMBINED=ON <native build dir>
    cmake --build <native build dir> --target ra2yr-native     # with ra2-engine.so, yr-engine.so

### How the two engines live in one process

* **Symbols.** Each game is an engine bundle (`MH_BUNDLE`, `ra2-engine.so`, `yr-engine.so`): the
  same objects as its executable (`ra2-native`: src/recovered's units, native data, the port;
  `yr-native`: the YR units, `yr_data`, the port), with its own copy of the platform layer
  (`ra2_port`, the SDL host). The host is compiled once more per engine with
  `RA2_NATIVE_ENGINE`: `main.cpp`'s `main` becomes `ra2_engine_main` (`platform/sdl/engine_api.h`).
  ld64's `-exported_symbol _ra2_engine_main` makes every other symbol private to its bundle, so
  the names both games define (`WinMain`, `MapClass`, the C++ templates and vtables, every port
  function) never meet, and weak definitions are never coalesced across the games. No source is
  renamed; neither tree knows about the other.
* **Globals.** All of a game's globals are its bundle's. Each engine start maps a fresh copy of
  its bundle (copied to a temporary file and unlinked once mapped: dyld maps a file once per
  process), so the game starts from its static initialisers with the original's data loaded,
  exactly as a process start would. Images that used to depend on being image 0 now find their
  own image (the YR deferred initialisers' section, `original_save.cpp`'s in-image test).
* **Shared.** SDL (Homebrew's dylib on the Mac: one copy) and the window belong to the thin host
  (`combined/one_game.cpp`, `ra2yr-native`). An engine takes the window it is given
  (`ra2_sdl_host_use_window`; the game's `CreateWindowExA` takes it over) and makes its own
  renderer, textures and audio stream on it. The settings file is shared: the thin host owns its
  `game=ra2|yr` line; YR's own keys are separate (`yr_game_exe`), and in the app YR writes its
  RA2MD.INI and saves under `Game/YR`, so neither game's Load menu lists the other's saves. Each
  game reads its own executable variable (`RA2_GAME_EXE`, `RA2_GAMEMD_EXE`).
* **A swap.** The toggle (below) asks the port to end the game the way a player does: the port
  presses Exit Game in the main menu and OK in its question as those dialogs open; the host
  freezes the picture first, so the window keeps the menu's last frame meanwhile. The game's
  own shutdown runs (`Main_Game` returns, `WinMain` frees its art, surfaces, mixfiles and network
  and returns). The engine then gives the window back and destroys what it made with SDL (renderer
  with its textures, audio stream, event watch: `ra2_sdl_host_release`). The thin host runs the
  ended engine's exit handlers at once (`__cxa_finalize_ranges` over its image: `Prog_End`, the C++
  static destructors, the app settings' save), as `dlclose` or `exit` would, and starts the other
  engine with `skip_intro`: the port opens no Bink movie before the first dialog (the logos and
  the intro), so the other game's main menu comes up in about 5 s. The ended image stays mapped
  and is never called again (a thread of it may still be parked there; unmapping would pull the
  code from under it). Cost: its memory (code and what the game did not free) per swap.
* **The time limit of a test run** is the run's: each engine gets what is left; an ended engine's
  watchdog stands down (`ra2port_run_retire`).

### The toggle

`ra2port::set_game_toggle` (port/src/user32.cpp): the main menu (the dialog with Exit Game,
0x3EE "GUI:ExitGame", the same in both games) gets a clone of its Exit Game button (id 0x7F5E),
labelled "Play Yuri's Revenge" / "Play Red Alert 2" and placed, once the game has laid the menu out,
just above the left end of the menu's bottom bar (the tool-tip line 0x695), away from the
right-hand column. Lane C23 (user requests 2026-11-16: the version-line clone of lane Y5 read as
help text; then: it should look like the dialogs' push buttons, Keyboard Options' Assign and Reset
All, and not flicker) made it such a push button: the game's own owner-draw button code paints it
as those buttons (kind 3, WinData +0xB0: the 126x25 push button shape, red with yellow text, its
lit and pressed frames, the click sound), because the id is given that kind beside the listed
dialogs' ids: RA2 in port/fixups.json (unknown_5e7eb0 0x005E7EB0), YR in yr/fixups.json
(ClassifyLayoutBand, its twin); it takes that shape's size. The menu's movie (the TV screen,
632x570 at the top left) covers everything above the bar on the left, so the toggle stands over
it: the movie's timer draws the toggle on each new frame before the frame is shown (both games'
static procedures; the first version drew it after the frame, and a picture without it could be
shown: the flicker the user saw on the Duo). YR's button paint also takes the button's BST_PUSHED
as pressed, as RA2's does (the port sends no WM_DRAWITEM), so both games' buttons show their press. The deeper menus are simply the running game's. The selection is the settings' `game=` line (the thin host
writes it at every start and swap); the next start runs that game with its start-up movies.

ctest `one-game-toggle` (yr/tests/one_game_toggle.py, needs RA2_NATIVE_YR_DATA_DIR and
RA2_NATIVE_CENSUS_GAME_EXE): RA2 menu -> toggle -> YR menu (no start-up movie) -> toggle -> RA2
menu -> a skirmish in RA2; the game chosen last (RA2) -> YR -> a skirmish in YR; then YR, chosen
last, starts with its movies. Checks the engine order, the skipped movies, the toggle in each
menu, game frames in both skirmishes, the settings and that no process of the run is left. The
toggle is clicked by its id (input script `@ctl=32606`), and window dumps check it: six pictures
120 ms apart over the running movie must each show it red (at least 30% of its rectangle: 70-74%
measured; the menu-button art of the first version 27%, the version-line clone 0%) and the same
(no flicker), and the picture while it is held must differ (the pressed frame); and with
`RA2_NATIVE_TEST_TOGGLE_WATCH=1` the port checks the toggle's rectangle at every present of the screen
until the press: at most 2 changes (measured 2 in 310 presents; the after-the-frame version 78-80)
(lane C23).

Fixed on the way: YR's exit freed the menu drawers with the vector deleting destructor
(gamemd.exe 0x0072B230, harmless at exit on Windows, an abort natively; fixups.json).

### Lane Y13 (2026-11-17): Yuri's Revenge for Windows, data the x86-64 traps found, the rocket

* **The Windows build** (llvm-mingw cross-build on the Mac, as Red Alert 2's: `cmake -S native -B build/yr-win -G Ninja
  -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-x86_64.cmake -DRA2_NATIVE_SDL_SOURCE=<SDL 3.4.2> -DRA2_NATIVE_SUBSYSTEM=all
  -DRA2_NATIVE_LINK_CLOSURE=ON -DRA2_NATIVE_YR=ON -DRA2_NATIVE_YR_MINGW=<llvm-mingw>/generic-w64-mingw32/include`, then
  `ninja -C build/yr-win YRNative`): `YRNative.exe` (GUI subsystem, for players) and `yr-native.exe` (console, the tests),
  self-contained (system DLLs and the UCRT only). What differs from the Mac build (yr.cmake `if(WIN32)` branches):
  - headers: the rewritten Win32 headers are searched ahead of the toolchain's (`-isystem`), leaving out every header
    the macOS SDK has (`yr_sdk.py --host-headers`), so the C library is the toolchain's as the Mac's is its SDK's;
    `long` stays `long` (32 bits on Win64, VC6's width, the type clang's MS intrinsics use): `sdk/yr_win_first.h` reads
    `_mingw.h` before `dialect_win.hpp` defines `__LP64__`, and `yr_prelude.h` spells the units' `RA2_LONG` as `long`;
  - COFF spellings of the units' and glue's assembler labels (`tools/yr_coff.py`, also the multi-literal `.globl`
    aliases), `yr_aliases.py`/`yr_data.py --target windows`, the link closure's `-alternatename` directives;
  - the units' and glue's static initialisers move from `.ctors` to `.yrini$m` (`defer_initializers.py`, COFF) and
    `yr_host_glue.cpp` runs them between its `.yrini$a`/`.yrini$z` markers after the original data is loaded;
  - Red Alert 2's link options (`ra2_windows_game_link`: select-any, 8 MiB stack, icon).
  Link closure 179 on Windows (the Mac's 184 less a few Win32 calls the system supplies; plus `XMP_Init` and two
  `UnVQ` tables, mangled with `unsigned long`: Westwood Online and VQA only). ctest `yr-native-skirmish-smoke` runs
  `yr-native.exe` under Wine (the Mac route: menus, a skirmish, Abort, the setup again): passes. Not tried on a real PC.
* **Data the Windows build's divide traps found** (x86-64 faults on an integer division by zero; arm64 quietly gives 0,
  so each was a silent fault on the Mac and iPhone too):
  - `ZGradients` (Bit_Blit's depth steps), `MouseClass::MouseControl` (every cursor's frames) and
    `InfantryClass::MasterDoControls` (the infantry actions' rates and flags) were never loaded from gamemd.exe (their
    structures were not on yr_data.py's list of pointer-free structures); now they are;
  - `DAT_00ABDE88` in the map unit is that unit's copy of geometry.h's `LEVEL_LEPTON_H` (gamemd.exe's initialiser
    0x005617E0); nothing set it, so `Is_Shrouded` and the cell-height reads used 0 (fixups.json);
  - the TechnoTypeClass constructor draft stored 1 in `IdleRate`; gamemd.exe stores it in `WalkRate` (+0x294). With
    `WalkRate` 0 the walk frames never advanced and with `IdleRate` 1 every unit ran its idle animation each frame:
    a likely cause of the user's unit animation glitches (to be checked by the user).
  The Mac window dumps' black tactical view (lane Y12) is not seen in the fixed build's smoke run.
* **NON-EXACT** `RocketLocomotionClass::Process` (gamemd.exe 0x006622C0; glue/rocket_native.inc): Red Alert 2's text
  with gamemd.exe's cruise missile (CMisl), its launch state 6, the layer re-submit and the legal-cell move.
  Link closure (Mac) 186 -> 184.
* **Long game**: 45 minutes against three hard opponents (Russia, Russia, Yuri; `yr_skirmish_long.py --countries 8,8,9`,
  new) on map 30: no trap, no fault.
* **LAN, first look**: the main menu's Network button reaches `IPXManagerClass::Service` (a trap) and then a fault in
  `Select_Game`; YR's LAN also needs the `MultiplayerGameMode` entries the closure lists (about 40). Not started.

### Lane YW (2026-11-17): the Windows tests and build command, LAN to the join request

* **Windows**: `native/build.sh yr-windows` (build/release-yr-win: `YRNative.exe`, and `yr-native.exe` for the
  tests). ctests on the Windows build, each under Wine in a fresh prefix deleted after the run
  (`tests/yr_wine.py`): `yr-native-skirmish-smoke` (lane Y13), `yr-native-skirmish-score`, `yr-native-combat`:
  all pass.
* **LAN** (Network): Mac to Mac and Mac to Windows-under-Wine, the host screen and the guest's lobby listing the
  host's game, its selection and the Join request. ctest `yr-native-lan-lobby` (`tests/yr_lan_lobby.py`, two
  Mac instances; `--binary-b <yr-native.exe> --wine-prefix-b <dir>` runs the guest under Wine). On the way
  (all NATIVE/NON-EXACT, yr only):
  - drafts: `IPXManagerClass::Service` (0x00541820, from the RA2 tree's twin; the Internet game's NAT and
    router branches left out, Westwood Online is not built), `UI::RegisterWindow` (0x00622820, the network
    dialogs' preparation), `LBSaveSelections` (0x005E5F10), `FUN_005EC9F0` (0x005EC9F0, a slot's entry on the
    host screen; the RA2 tree's Unknown_005cbf20 with YR's start/team arrays), `FUN_005EA3E0` (0x005EA3E0, the
    setup screens' slot controls refresh);
  - `WinsockInterfaceClass::Build_Packet_CRC` / `Passes_CRC_Check`: members calling the matched free functions
    (wsproto.cpp); MultiplayerGameMode's 24 base virtuals from gamemd.exe's vtable 0x007EED60;
  - `OriginalCheats` (0x00825C28) read from gamemd.exe after the data load (Cheat_Disable faulted on the
    Network button); Westwood Online's tournament tests `FUN_0077D940`/`FUN_0077D970` false (glue/yr_lan.cpp);
  - name aliases: `Build_Number` = FUN_00777B50, `OwnerDraw::Draw_Dialog_Back` = OwnerDraw::Paint, wstring.cpp's
    `Bit_Stuff`/`Bit_Unstuff_String` = bfiofile2.cpp's.
  Link closure (Mac) 183 -> 151.
* **LAN, next**: the host does not seat the joining guest yet (the guest's Join shows its waiting box; the
  host's slot stays Open): follow the join packet through `Process_Global_Packet` (netdlg2.cpp) on the host
  and the guest's `MPlayer_Guest_Dialog_Proc`; then Start Game, the game in step (RA2's `native-lan-pair`
  checks: per-frame CRCs, RA2_NATIVE_CRC_LOG) and the Mac + Windows pair. Closure entries likely on that path:
  `DecodePubGameopt`, `FUN_005E6920`, `Send_Statistics_Packet`, `Draw_Sync_Bars`, `FUN_00637E00`.
### Lane Y14 (2026-11-17): the user's graphics reports, the command bar, the Skip button, stand-ins

* **Vehicle noise on a new picture** (the user's iPhone reports: a deploying MCV and a Grizzly's first move drawn as
  pink, green and white noise, gone once the vehicle had moved): a vehicle composites its voxels on the eight-bit
  surface (UnitClass::Unit_Draw_Voxel); a picture not yet in the voxel cache is rendered directly by
  TechnoClass::Techno_Render_Voxel_Object (glue/techno_native.inc, NON-EXACT, gamemd.exe 0x00706ED0), whose draft drew
  it through the house's 16-bit converter: 16-bit pixels written into an 8-bit surface. It now picks EightBitDrawer
  while compositing, as gamemd.exe (0x0070717D) and the tree's Techno_Blit_Voxel do; Techno_Draw_Object's TibSun text
  had lost the same choice. ctest `yr-native-graphics` (yr/tests/yr_graphics.py): the harness watches the composed
  tactical view for noise-coloured pixels for 900 frames while the MCV deploys and a Grizzly first moves (0-24 pixels a
  frame now; 75-210 with the old draft).
* **The command bar** (the user's "missing bottom bar"): TabClass::Draw_It draws it on the composite surface and sets
  Map's TabClass::unknown_byte_5546 (0x0087F7E8 + 0x5546 = gamemd.exe's DAT_00884D2E); Update_Visible_Surface copies
  the bar's 32 rows only while that flag is set. Natively DAT_00884D2E was a separate global that stayed false, so the
  bar never reached the screen. fixups.json: gscreen.cpp reads the member. `yr-native-graphics` checks the layout (the
  tactical view ends where the bar starts) and the bar's rows on the screen; `ios_sim_menus.py` does the same on
  "RA2 Duo Validation" (interface size 1.25x, HD vehicles off: tactical 656x568, bar 656x32 at 568, 29% drawn).
  Likely also the cause of lane Y10's "a loaded game redraws its command bar only once the mouse passes over it".
* **The Skip button on the loading screen** (the user's screenshot): Yuri's Revenge keeps its intro movie open through
  the start-up load, and the touch controls counted any open full movie as playing. port/src/bink.cpp: a full movie
  counts only while the game polls it (BinkWait/BinkDoFrame within the last second of the game clock); host.cpp also
  updates the touch controls when presenting (the load presents without pumping events). `ios_sim_menus.py` checks the
  Skip button gives way before the main menu in Yuri's Revenge.
* **Stand-ins** (link closure 184 -> 175):
  - NON-EXACT HouseClass::Fire_PsyDom (glue/house_native.inc, gamemd.exe 0x0050A150, in the shape of its matched
    siblings);
  - the team script's ion storm actions call the matched FUN_00539EB0 / FUN_0053A090 (LightningStorm::Start /
    RequestStop) with their real types (fixups.json; team.cpp had (int center, bool flag) where gamemd.exe pushes a
    cell and a null owner);
  - bindings (data/name_aliases.tsv): TechnoClass::GotHijacked is the slot gamemd.exe fills with the tree's empty
    Scatter_Incoming_Infantry (0x0070D980); BuildingClass::Spied_By is Infiltrate (0x004571E0, what gamemd.exe's
    Per_Cell_Process calls); BuildingClass::Power_Off is DisableStuff (0x00452480, what Turn_Off calls);
  - BuildingClass::Captured's draft asks IsPowerOnline (vtable slot 0x350, as gamemd.exe 0x00448C01), not the
    undefined Is_Powered_On;
  - NON-EXACT MapClass::Repair_Train_Bridge and Repair_Bridge (TibSun's text, glue/map_native.inc; not compared with
    gamemd.exe's RepairWoodBridgeAt 0x00570050 / RepairConcreteBridgeAt 0x00573540; no test reaches them yet).
  Still open on play paths: BuildingClass::Do_MISSION_UNLOAD (0x0044D880, 2,848 bytes; RA2 twin 0x00449290 in
  src/recovered/building.cpp), ParticleClass::Gas_Behavior_AI, TechnoClass::Draw_A_Link_To (needs
  DSurface::Draw_Depth_Antialiased_Line, also open), RadBeam FUN_00659AC0/FUN_00659FF0, WaveClass::Draw_Laser /
  Draw_Sonic / FUN_007602E0, MechLocomotionClass::Movement_AI, UnitClass::Is_Route_Broken.
* **Harness** (glue/yr_harness.cpp): `deploy <ID>`, `move <ID> <dx> <dy>`, `where <ID>`, `rects`, `noise <frames>`.
* **Open from Y12**: the campaign dialog's Difficulty label `MISSING:''` (StringTable::LoadString given an empty
  label; the dialog's code was not found in the time box) and the LTNK position (not investigated).

### Lane YW2 (2026-11-17): a LAN game in step

* **The game**: two Macs (or two games on one Mac) play a Network game: the host seats the guest, chat works
  both ways in the setup screens, the guest's Accept and the host's Start Game load the map on both, and the
  frames run in step: 3,000 frames with every frame's game CRC equal. ctest `yr-native-lan-pair`
  (`tests/yr_lan_pair.py`; RA2_NATIVE_CRC_LOG gives each frame's "crc <frame> <value>", fixups.json's
  queue.cpp row). `yr-native-lan-pair-wine`: the guest is the Windows build under Wine
  (`-DRA2_NATIVE_YR_WINDOWS_EXE=<build/release-yr-win/yr-native.exe>`; skipped without it; Wine from
  RA2_NATIVE_WINE, PATH or the main checkout's private/tools bundle): passes, 3,000 frames, CRCs equal.
* **Why the guest was never seated**: the host refuses a player whose serial it already has
  (TXT_SERIAL_DUP), and every native copy read the same empty serial from the registry the installer would
  have written. Without one, the installation keeps its own 22-digit serial, made once at random, in its
  settings folder (`YRSERIAL.TXT`; glue/yr_lan.cpp `Yr_Installation_Serial`, called from Decrypt_Serial by a
  fixup).
* **Globals that are Session's fields**: gamemd.exe names some of Session's fields as globals
  (`GameModeOptionsClass::Instance` = Session.Options.MPModeIndex, `Game::PlayerColor` = PrefColor,
  `Game::PlayerCount` = NumPlayers, `Game::ChatMask` = PlayerChatEnabled,
  `Game::Network::PreCalcFrameRate` = PrecalcDesiredFrameRate); yr_data.py gave each its own storage, so the
  host's game mode never reached the options it sent (mode 0: the guest faulted in Read_Scenario_INI). Fixups
  name the fields in netshare, netdlg2, netdlg, session, house and scenario. Still separate storage, not on the
  LAN path: `Unsorted::SWAllowed` (Options.SWAllowed, house.cpp), `Unsorted::MuteSWLaunches` (0x00A8B538,
  Session+0x300, many units), `Game::RecordingFlag` (0x00A8D5F8, init/sidebar): a general fix belongs in
  yr_data.py (an address inside a defined object becomes that object's field).
* **NON-EXACT drafts** (yr only): `DecodePubGameopt` (0x005E3D10; the RA2 twin's draft and TibSun's
  shape; Westwood Online's Z/P/C options and chat-channel parts left out), `FUN_006475F0` Queue_AI_Multiplayer
  and `Wait_For_Players` (0x00648710) from the RA2 tree's exact text (gamemd.exe repeats it) with YR's names,
  `FUN_0064BDD0` Extract_Compressed_Events (the RA2 draft's text and fixes; same 414 instructions),
  `Draw_Sync_Bars` (0x0064A7B0), `FUN_00650A90` Game::LogFrameCRC (PARTIAL: the -MPDEBUG sync record keeps only
  the CRC, random number and frame). OwnerDraw::NewEditHandler (Y? draft): a chat line's Return now also sends
  the 0x501 notification the setup screens answer by sending the line (the RA2 rewrite's C20 fix).
* Link closure (Mac) 151 -> 137 (with Y14's landing).
* The Windows build with lane Y14's landing (harness orders, drafts, bink.cpp, host.cpp) cross-builds and links
  under llvm-mingw unchanged.
* **Next**: the -MPDEBUG sync record's object lists; longer games with orders (RA2's native-lan-pair gives orders
  and control groups); the both-games app on Windows (engines as DLLs).

### Lane Y15 (2026-11-17): globals that are members, team buttons on touch, stand-ins

* **Globals inside a defined object are its members** (tools/yr_member_aliases.py, data/member_aliases.tsv): gamemd.exe
  names some fields of its global objects as globals (Game::PlayerColor is Session.PrefColor, DAT_00884D2E is Map's
  TabClass::unknown_byte_5546, DAT_00B054D4 is ColorSchemes' item array). The tool finds every global yr_data.py would
  give storage whose address lies inside a unit-defined object (owners from globals.tsv, template types included),
  maps the offset to the member by the owner unit's clang layouts (i686-msvc against arm64 and x86-64 Windows,
  original_layouts.py --game yr), and yr_units.py appends to the owner's unit one assembler alias per name (owner +
  the member's native offset; `#if __x86_64__` for Windows). 48 names, 15 owners: Session (Unsorted::SWAllowed =
  Options.SWAllowed, Unsorted::MuteSWLaunches, Game::RecordingFlag = the Record bit-field's byte, Game::PlayerColor /
  PlayerCount / ChatMask / PreCalcFrameRate, GameModeOptionsClass::Instance, ...), Map (20: the pending placement,
  the radar, the command bar's flags), the vectors' raw counts and arrays (Houses, ColorSchemes, CurrentObject,
  IngameVQ, VoxArray), TargetTracker, CDList, WinVersion, RandomMapGen, RadarStatics' two buttons, DAT_00ABFEC8. Not
  merged (the tree declares an int where the member is a pointer): DAT_0088097C, DAT_0088098C, DAT_00880990 keep their
  lane Y7 fixups (narrowed). The per-name fixups of lanes YW2, Y14 and Y7 that the rule covers are gone. Found on the
  way: the superweapon timer's flash read a null DAT_00B054D4 (yr-native-skirmish-long's fault in
  Tactical::DrawTimer once a timer reached zero), and Delete_All_Objects sorted TargetTracker's table with the 8-byte
  entry size (fixups: sizeof, Archive declared a pointer). ctest `yr-native-member-aliases`: the table is what the
  build gives (rerun the tool after a header change, then build again).
* **Pointer tables declared without a bound** (fixups.json): DAT_00822774 (TXT_EASY/NORMAL/HARD), DAT_0081B538,
  DAT_00823FD4 hold their pointers followed by the strings; with the extent to the next global the strings were loaded
  as pointer slots and the pointers into them reached converted storage: the campaign dialog's Difficulty label read
  `MISSING:''`. Their bounds are the tables' own.
* **Team buttons on touch** (both games; platform/sdl/touch.cpp, host.cpp, link/team_button_hook.cpp,
  glue/yr_app_hooks.cpp): the command bar (TabClass::AI) gives an empty team the selection on a click, selects a team
  with members, and disbands a team on a right click; Ctrl is not read there, which is why the toolbar's Fire toggle
  did nothing. A hold (400 ms) on a team button now sends the right click and, 150 ms later, the click: the team
  becomes the selection. A tap is unchanged; a mouse is unchanged. Each game answers `ra2_native_team_button_at`
  from its bar's buttons; input scripts take `@team=N`. ctests `native-touch-team-hold` (RA2) and
  `yr-native-touch-team`; `ios_sim_skirmish.py --team-hold` on "RA2 Duo Validation" passed.
* **Stand-ins** (NON-EXACT, yr only): BuildingClass::Do_MISSION_UNLOAD (gamemd.exe 0x0044D880, read
  instruction by instruction on the RA2 twin's shape: the Bio Reactor's and unit absorbers' unload, the war factory,
  the gap generator); RadBeam FUN_00659AC0/FUN_00659FF0 (RA2's Compute_Pulse_Step/Compute_Swing_Step, fields checked
  against gamemd.exe); from Red Alert 2's text (mischa85/RedAlert2, attributed): MechLocomotionClass::Movement_AI,
  WaveClass::Draw_Sonic and Draw_Laser, ParticleClass::Gas_Behavior_AI, DSurface::Draw_Depth_Antialiased_Line;
  UnitClass::Is_Route_Broken (TibSun's text); TechnoClass::Draw_A_Link_To (gamemd.exe 0x00704E40, the mind control
  link, read instruction by instruction). Still open: WaveClass::FUN_007602E0 (wave type 3, 1,777 bytes). Link
  closure (Mac) 137 -> 127.
* **Open**: the LTNK off-map place-down/pick-up in the campaign chains (Allied 2, Soviet 7) is deterministic (the
  same garbage coordinates every run, e.g. -1447125004,-1459498755), so it is not a pointer's low half (those vary
  with ASLR); not found in the time box.

### Lane Y16 (2026-11-18): the LTNK off the map, the Magnetron's beam, stand-in tests, a longer LAN game, one game on Windows

* **The LTNK driven off the map** (Allied 2, Soviet 7; lane Y12's `place-down:`/`pick-up:` lines with the same garbage
  coordinates every run): gamemd.exe's drive locomotor has 72 turn tracks and 15 raw tracks; the tree has TibSun's 67
  and 13. YR's turn tracks 67-71 (raw tracks 14, 16 steps, and 15, 17 steps) are what Force_Track gives a unit leaving
  a factory diagonally; with the 67-entry table TrackControl[70] read past it, its raw track was RawTracks[-1] and
  Smooth_Turn added garbage offsets. The tables are pointer-free, so `TrackControl[72]`, `Track14_007E78A8[16]` and
  `Track15_007E7968[17]` are declared only (hdr/drive.h's fixup) and loaded from gamemd.exe (yr_data.py:
  `TurnTrackType`/`TrackType` are plain structures); `RawTracks[15]` (glue/drive_native.inc) keeps its pointers. On the
  way, yr_data.py reads a static member's bound from its own class's declaration (`Class::member` index), not from a
  same-named member of another class (ship.h's `TrackControl[67]` had given the drive's its bound; CDFileClass::First
  and LastCDDrive now get their own types too). `yr-native-campaign-allied/-soviet` fail on any off-map line.
* **NON-EXACT WaveClass::FUN_007602E0** (gamemd.exe 0x007602E0, 1,777 bytes; Draw_It's wave type 3, the Magnetron's
  beam): gamemd.exe's routine is Draw_Sonic with FUN_00760190 (matched) called where Draw_Sonic calls
  Set_Sonic_Pixel; the two disassemblies differ in those four call targets only (yr_dis.py; ra2-decomp's attempts.csv
  says the same). glue/wave_native.inc: the Draw_Sonic draft with the call swapped. Link closure (Mac) 127 -> 126.
* **ctest `yr-native-standins`** (yr/tests/yr_standins.py; harness orders `spawn <ID> [enemy|dx dy]`, `fire`, `enter`,
  `owner`, `waves`, `wave <type>`, `super <name> enemy`, `money`): a Magnetron (TELE), a Prism Tank (SREF) and a
  Mirage Tank (MGTK) fire on two enemy Apocalypses spawned beside them, sonic and magnetron waves are made directly,
  a spy goes into an enemy power plant, the Psychic Dominator and the Lightning Storm are placed through
  SuperClass::Discharged (the map click's path): no trap, the waves in flight, the Apocalypses destroyed, the spy in.
  Not covered: a hijacker (no rules ID found here; TechnoClass::GotHijacked is gamemd.exe's empty slot anyway, lane
  Y14), bridge repair (needs a map with a broken bridge) and the War Factory's unload (production through the
  harness is not written; the AI's factories run Do_MISSION_UNLOAD in every long skirmish).
* **A longer LAN game with orders** (`yr-native-lan-pair`, also `--frames 12000`): both sides' fighting vehicles are
  selected (`select * all`), made team 1 (Ctrl+1), moved as a group (`go`: one move event each in one frame), the team
  recalled and sent back every 600 frames, as RA2's native-lan-pair; from 6,000 frames on every combat unit hunts for
  the last 2,000. 3,000 frames (the ctest) and 12,000 frames (6 min 20 s, 36 group moves a side, the hunt): every
  frame's CRC equal.
* **One game on Windows** (`-DRA2_NATIVE_COMBINED=ON` in the Windows build; yr/combined.cmake's WIN32 branch):
  `ra2-engine.dll` and `yr-engine.dll` are the same objects as ra2-native.exe and yr-native.exe with the engine host
  (main.cpp's `ra2_engine_main`, `__declspec(dllexport)`: the DLLs' one export), libc++/libunwind/winpthreads inside
  each (-static); the build's SDL is then the one shared `SDL3.dll` (copied beside the executables), owning the window
  both games draw in. `RA2YRNative.exe` (GUI) and `ra2yr-native.exe` (console, the tests) are the thin host
  (combined/one_game.cpp's `_WIN32` parts): each start copies the engine's DLL to %TEMP% under a new name and
  LoadLibrary's it (a fresh image: the game's static initialisers again), an ended engine's exit handlers run through
  its DLL entry point with DLL_PROCESS_DETACH (what FreeLibrary would run: that DLL's atexit table, the static
  destructors, Prog_End, the settings save) and its image stays loaded; copies left by earlier runs are deleted at
  the next start. The host's environment changes (`_putenv_s`) reach the engines (one ucrtbase.dll). The last engine's
  exit handlers also run before the host returns (ExitProcess would run them in its DLL detach after ending every
  other thread), and the port's and host's `_exit` is TerminateProcess on Windows (platform/win/port_compat.h): the
  UCRT's goes through ExitProcess and the running engine's detach, and a test run's time limit never ended the
  process. Also: link()
  for the one-game imports' hard links (CreateHardLinkA, platform/win). ctest `one-game-toggle-wine`: the
  one-game-toggle route (RA2 -> YR -> RA2 and a skirmish, RA2 -> YR and a YR skirmish, YR chosen last) on
  ra2yr-native.exe under Wine (`one_game_toggle.py --wine-prefix`; time limits doubled). Not tried on a real PC.
* **native-basics' shroud flake** (RA2, link/harness_orders.cpp): the scouts' destinations were picked among the
  least-mapped directions without asking whether they could be reached; the start position is random, and a game
  whose three picks were all in another movement zone (cliff tops, water: the path finder refuses them, the scout
  never moves) failed. Destinations are now in the scout's own zone (MapClass::Zone with its MovementZone).

### Lane P1 (2026-11-18): packages for a real PC, build commands, the hijacker

* **Builds**: `native/build.sh windows-one` (new) builds the both-games Windows app and gathers what a PC needs in
  `build/release-win-one/RA2YRNative/` (RA2YRNative.exe, ra2-engine.dll, yr-engine.dll, SDL3.dll, SDL's licence;
  only system DLLs, the UCRT and SDL3.dll imported). `one-game-toggle-wine` passed on that build (374 s).
  `native/build.sh mac-one` (lane N1) already produced the owner's hand-made RA2YR.app: the same layout, SDL paths
  and ad-hoc signature (identifier local.ra2native.ra2yr); both engines reach the main menu from the bundle.
* **The hijacker** (ctest `yr-native-standins`, now also a hijack): no infantry in Yuri's Revenge's rules
  (RULESMD.INI in ra2md.mix and the patch's expandmd01.mix, read in place) sets `VehicleThief=` or `Thief=`; only the
  comment block describing the keys names them, so there is no rules ID to spawn. New harness orders: `thief <ID>`
  (sets IsThief, which InfantryClass::Theft_AI tests, and IsVehicleThief, which movement and the click's action test),
  `enter <ID> <TARGET> hijack` (MISSION_CAPTURE with the vehicle as the destination only, as
  FootClass::Active_Click_With's capture click) and `spawn <ID> enemy <dx> <dy>`. The test makes a civilian (CIV1) a
  thief nine cells from the base and sends it at an enemy truck (TRUCKA) thirteen cells out: by frame 1250 the truck
  is the player's and the thief has gone into it (3 of 3 runs). Found on the way: a thief spawned on the vehicle's
  own cell never hijacks (WalkLocomotionClass::Movement_AI's no-path branch clears its destination, and the vehicle
  then drives over it); a walking thief is the normal case. An enemy MCV as the target was flaky (the AI deploys or
  loses it).
* **Bridge repair**: not started. Many multiplayer maps carry a CABHUT (multimd.mix); a test needs a harness order
  that breaks the bridge next to the hut and the skirmish map index of such a map.

### Lane N3 (2026-11-19): the user's two Duo faults

* **The command bar's "select all units of this type" crashed the game.** TabClass::AI (gamemd.exe 0x006D0680) calls
  FUN_00732950 for the bar's TypeSelect button, and the TypeSelect key's quick press (FUN_00732CC0) calls it too; the
  YR tree has not recovered it, so it was the link closure's trap. glue/uicmnds_native.inc: NON-EXACT stand-in from
  gamemd.exe's 729 bytes (yr_dis.py) in the shape of its matched sibling FUN_00732280: the selected objects' type names
  (FUN_00732D10), the player's units on the screen of those types selected; when all are already selected, or on a
  second press (NavCycleMode 2), across the map; the "selected across screen/map" message. Link closure (Mac) 126 -> 125.
  ctest `yr-native-type-select` (yr/tests/yr_type_select.py): with one tank selected the button selects every tank,
  with nothing selected it selects nothing, and the T key does the same as the button. Input scripts take
  `@cmd=TypeSelect` (the bar button's middle; glue/yr_app_hooks.cpp registers the point resolver).
* **No on-screen keyboard in Yuri's Revenge's text fields.** The player-name field is a dialog Edit that OwnerDraw's
  FUN_0060F320 hides and replaces with a ListBox carrying a NewEditLink ("NewEdit") in GWL_USERDATA; port/src/user32.cpp
  answered the host only for windows of the classes Edit and NewEdit, and the YR engine had no
  `ra2_native_new_edit_focused` (Red Alert 2's is native/link/native_text_entry.cpp). glue/yr_app_hooks.cpp now answers
  it (the focus is a shown list box whose link says NewEdit) and `ra2_native_message_edit_open` (Session.Messages'
  edit line: chat and beacon text). As in Red Alert 2, the keyboard comes up when the field is tapped.
* **Checked in the Simulator** ("RA2 Duo Validation", `ios_sim_menus.py`, now 120 s of play by default): in both games
  a tap on the player name field (control 1696, `@ctl=1696`) gives "host: on-screen keyboard shown" and the tap away
  "hidden"; in Yuri's Revenge a tap on the TypeSelect button with one tank selected selects all five. The walk's old
  tap point missed Red Alert 2's field (further right), so it never checked Red Alert 2's keyboard.

* **Stand-ins the campaign triggers, the command bar and the keys reach** (item 1 of PORTS_REMAINING: a static trace of
  the 125 functions still missing at the Mac link, from their callers in the tree; link closure 125 -> 116):
  - BuildingClass::ForceCreate (gamemd.exe 0x00458A80; vtable slot 0x490): the trigger action "create building at"
    and HouseClass::RespawnStartingForces. glue/building_native.inc, from gamemd.exe's instructions on the matched
    FootClass::ForceCreate's shape (foundation cells' overlay cleared, the units there lifted and put down nearby);
  - bindings (data/name_aliases.tsv): IonStormClass::Lightning_Bolt is LightningStorm::Strike (0x0053A140, the
    trigger action "ion lightning strike"), Ion_Storm_End is RequestStop (0x0053A090, "ion storm stop"),
    TechnoTypeClass::Get_Weapon is GetWeapon (0x007177C0, an upgraded building's weapon);
  - XSurface::Draw_Ellipse: a selected Gap Generator's or Psychic Sensor's range ring
    (BuildingClass::Draw_Radial_Indicator). glue/xsurface_native.inc: Red Alert 2's draft of its twin
    (src/recovered/xsurface.cpp, not exact), not compared with gamemd.exe;
  - FUN_00733380 / FUN_007336C0, the HealthNav and VeterancyNav keys (glue/uicmnds_native.inc, from gamemd.exe's
    instructions with the matched helpers they inline);
  - BeaconManagerClass::PlaceBeacon (0x00430BA0) and Drawing::Intersect (glue/beacon_native.inc): the Beacon
    command's placement mode (the command bar's Beacon button in network games; a skirmish's bar has none), then a
    click on the map, then every frame's beacon draw. PlaceBeacon from gamemd.exe's instructions on Red Alert 2's
    accepted twin's shape.
  ctest `yr-native-standins-triggers` (yr/tests/yr_standins_triggers.py; harness orders `create <BLDG> on <ID>`,
  `bolt enemy`, `stormstart`, `stormend`, `beaconmode`, `beacons`, `cmd <name>`): a selected Gap Generator, a power
  plant forced onto the player's tanks (none lost), a bolt, a storm started and stopped, a beacon placed by a map
  click, three presses of each nav key selecting tanks. Input scripts also take `@cmd=Beacon` (network games).
  Still missing (callers found, reach not
  checked in play): the laser fence's and firestorm wall's placement drawing
  (Tactical::Draw_Fence_Placement / Draw_Firestorm_Wall_Placement, for building types with IsLaserFencePost or
  IsFirestormWall); Speech_Name (a trigger's description text); the score font's Print_String; LineTrail's
  constructor (types with line trails); Tunnel_AI (Tiberian Sun's subterranean movement); the random map generator
  (MapGeneratorClass) and the World Domination Tour (WorldDominationTour::*), both Westwood Online.

* **Planning mode** (the command bar's PlanningMode button stopped the game at its first draw with units selected):
  glue/planmgr_planning_native.inc, NON-EXACT stand-ins from gamemd.exe's instructions for 0x00637E00 (the planning
  events: commit, connect, node delete, a planned order), 0x00638120 (a planned order, Red Alert 2's
  Planning_Add_Order), 0x00634E10 (a member's branch), 0x00639CB0 (a closed loop's attack orders become attack-move),
  0x0063B550 (a unit's plan in outline, the tree's filled twin Draw_Filled_Unit_Waypoints in outline) and
  0x0063C690 / 0x0063CAE0 (a node in outline and filled). A fixup in FUN_0063B0A0 empties the drawn-nodes list
  (PlanningNodeClass::Unknown2) through the list itself: gamemd.exe's names for its fields were separate storage, the
  list was never emptied, and the hover test read a deleted node once a plan was committed. Link closure 116 -> 110.
  ctest `yr-native-planning` (yr/tests/yr_planning.py; script point `@cmd=PlanningMode`, harness orders `plan <dx> <dy>`
  (an order through Player_Assign_Mission, a map click's path) and `plans`): every tank selected, planning mode on,
  two waypoints planned, the commit, the tanks follow. Still partial from lane Y3:
  FUN_0063AB60 / FUN_0063A5A0 (the mouse over and clicks on planned nodes in planning mode) return false, so nodes
  cannot be picked up or linked by hand yet.

* **Production left inside the factory** (found by the new War Factory test): glue/building_native.inc's drafts
  (Exit_Object and its neighbours, TibSun's text) gave missions with Assign_Mission, which in the YR tree is
  ObjectClass's empty virtual (it backs the Mission property): a finished vehicle stayed inside the war factory
  (the factory never switched to MISSION_UNLOAD) and every other mission these drafts gave (the soldier's move
  from a barracks door, a harvester's harvest, ...) was dropped. They now call QueueMission(mission, false), what
  gamemd.exe calls there (vtable slot 0x1E8, MissionClass::QueueMission 0x005B35E0; 0x004445F0 for the unload).
  ctest `yr-native-war-factory` (yr/tests/yr_war_factory.py; harness orders `produce <FACTORY> <ID>`, `produced`,
  `mission <ID>`): a Grizzly produced in a spawned War Factory is unloaded (mission 16) and drives out, the factory
  returns to guard; a GI produced in a Barracks walks from the door.

* **The user's Mac app reports** (2026-11-19, a stale app from 2026-11-17): walked through the menus in the current
  packaged app (`native/build.sh mac-one`, the bundle's executable in app mode under a scratch HOME, settings
  `data_dir=`, `lan=1`, `game=yr|ra2`; scripts through `--engine-scripts=`; `RA2_NATIVE_SCRIPT_AS_USER=1` so a scripted
  key also types its character). The immediate skirmish defeat and the Network-screen crash do not happen (fresh and
  after the toggle from Red Alert 2). Saving through the Save dialog failed: loaddlg.cpp's new file name lived in a
  block-scoped array used after the block (VC6 kept the slot, the native compile reused it: a garbage file name).
  fixups.json declares the array in the enclosing block; ctest `yr-native-save-dialog`. Loading an original Yuri's
  Revenge save: no such save on the owner's Mac, not checked.
* **Resume note (lane N3, handed back 2026-11-19)**: PORTS_REMAINING items 0 and 1 done as above (item 1 statically:
  no mission was played through). Item 2: the War Factory test is done; bridge repair has harness orders (`blowhut`,
  `repairhut <ENGINEER>`, `bridge`) but no ctest yet: it needs the skirmish ScenIndex of a map with a bridge repair hut
  (CABHUT) next to a bridge (scan with `30 owner CABHUT` per ScenIndex), then: blowhut, wait for `bridge 1`,
  repairhut ENGINEER, wait for `bridge 0` (infantry_native.inc's Repair_Bridge / Repair_Train_Bridge, map_native.inc).
  Item 3 (Red Alert 2's 110 unreached rewrites) not started. Item 0's open part: loading a save made by the
  original Yuri's Revenge (PORTS_REMAINING item 1). Still partial: FUN_0063AB60 / FUN_0063A5A0 (picking up
  planned waypoints with the mouse). Functions still missing at the Mac link: 110 (gen-yr/link_closure.json).

### Lane N4 (2026-11-19): the user's three new Duo faults

* **GIs sent into a building vanished.** InfantryClass::Per_Cell_Process (glue/infantry_native.inc, TibSun's text) had
  no garrisons: a soldier that reached the cell of the building it was sent to capture (MISSION_CAPTURE: the click on
  an occupiable building is ACTION_CAPTURE, InfantryClass::What_Action) fell through to TibSun's capture code, which
  deletes a unit that is neither an engineer nor a spy. gamemd.exe's 0x00519675-0x00519739 first lets an occupier or
  an assaulter garrison it (BuildingClass::CanBeOccupiedBy, InfantryClass::FUN_00522910), else sends it away
  (Assign_Destination(NULL), Scatter); the draft now does the same. ctest `yr-native-garrison` (yr/tests/yr_garrison.py;
  harness orders `click <ID> <TARGET>`: What_Action and Active_Click_With, a mouse click's order; `occupants <BLDG>`;
  `occupiable` lists the garrisonable types): a GI clicked onto a civilian house (CAHSE07) is inside it, the house the
  player's; the house's deploy lets it out and returns the house to the civilians.
* **The Mirage tank's camouflage drew as corrupted graphics.** TechnoClass::Techno_Draw_Object (glue/techno_native.inc)
  had left out gamemd.exe's 0x00706061-0x00706129: an object the player does not see through draws in its disguise's
  colours (a tree or an overlay: its cell's lighting converter and brightness; another house's unit: that house's
  scheme). The tree came out in the Mirage's house colours as blue and white speckles. ctest `yr-native-mirage`
  (yr/tests/yr_mirage.py; harness orders `disguise <ID>` and `camo <ID> <frames>`: leaf-green against blue/white
  pixels above the tank on the frames it shows its disguise; broken about 85/615, fixed about 730/5).
* **Oil derricks were missing graphics** (their pumpjack and flare): no building animation was drawn. Every animation
  type with an ART(MD).INI `Layer=` key read LAYER_NONE, so it went into no display layer: the layer names (LayerName,
  0x0081DA78) point at strings just after Powerups::Anims (0x0081DAD8), and yr/tools/yr_data.py read
  Powerups::Anims[19]'s declaration from builtype.h's `AnimControlType Anims[...]` (a namespace's members were not
  indexed under the namespace), so it was not loaded and was sized to the next known global (11,768 bytes): the names'
  pointers resolved into its zeroed storage. yr_data.py now indexes a named namespace's members up to its closing brace;
  the generated data changed only for Powerups::Anims (76 bytes, loaded) and Powerups::Naval (19 bytes; it had 1).
  ctest `yr-native-building-anims` (yr/tests/yr_building_anims.py; harness orders `artkey <section> <key>`, `banims
  <BLDG>`, `banimhide <BLDG> <slot> [show]`): Layer=ground reads as 2, the derrick's pump and flame and a power plant's
  animation play in the ground layer. Screen dumps (RA2_NATIVE_PRESENT_DUMP) show the pumpjack moving.
* **Beams crashed one way round**: the sonic wave, laser and Magnetron drafts (glue/wave_native.inc) held the depth
  buffer's address in an `unsigned int` (Get_Buffer_Offset returns uintptr_t): a beam whose direction took that branch
  read a truncated pointer. Found when `yr-native-standins` crashed in the rerun of the YR ctests; now uintptr_t.
* **Checked in the Simulator** ("RA2 Duo Validation", `ios_sim_menus.py --games yr --play-seconds 150`, N4_ORDERS): the
  GI garrisons (1 occupant, the player's), the camouflage 271 green to 0 blue/white, the derrick's two animations in
  the ground layer.
* **Bridge repair (item 2)**: the harness's `bridge` read 1 before and after because the skirmish maps N3 tried
  (and XDEADMAN.MAP, ScenIndex 3, the first with huts: `scenario`, `bridgecells`) start with broken bridges, and no
  repair happened: Per_Cell_Process's TibSun text asked NavCom->Considered_Vehicle(), TibSun's name for the YR tree's
  IsStrange (vtable 0x80, BuildingClass: Class->IsVehicle(); gamemd.exe 0x00519996 calls it with RTTI_BUILDING
  required). The tree's Considered_Vehicle (0x7C) is Height > -20, true for every building, so the engineer took the
  "captured a vehicle" branch and was deleted. The same call in glue/bullet_native.inc, particle_native.inc,
  bounce_native.inc (buildings let shots and debris through) and building_native.inc's sale (the player's sell sound)
  is now IsStrange. A vehicle target still takes the old branch in Per_Cell_Process: the draft hijacks vehicles there
  (yr-native-standins' thief; gamemd.exe's own hijack path not compared yet). Can_Repair_Bridge also gained gamemd.exe's second low-bridge family (0x00587803: overlays
  0xCD-0xE8, Is_Low_Bridge2). ctest `yr-native-bridge-repair` (readings 1, 0, 1, 0: broken, repaired, blown,
  repaired). Harness orders `bridgecells` (the cells around the first hut and every bridge zone connection's span)
  and `scenario`.
* **Resume note (lane N4)**: items 0a and 2 done; next item 3 (Red Alert 2's unreached rewrites). Most-called in
  game.exe by rel32 calls: SystemTimerClass::operator() (but its callers compile inline natively), the WWCRCEngine
  operators (0x00493E80/0x00493FC0/0x00493DF0: the full sync CRC, only on a desync dump), isspace, the LAN dialogs,
  chat (MessageListClass::Get_Edit_Buf), controlled voices (Audio_Event_Handle_Set). Started: Red Alert 2's harness
  gained `spawn <ID> [dx dy]` and `kill <ID>` (link/harness_orders.cpp), coverage_runs.py the scenario `n4-deaths`
  (a demolition truck and two tanks spawned and destroyed); on a coverage build (build/native-cov, the cmake line in
  coverage_runs.py's docstring plus -DRA2_NATIVE_SUBSYSTEM=all -DRA2_NATIVE_DRAFTS=ON -DRA2_NATIVE_LINK_CLOSURE=ON) it
  reached Shake_The_Screen, Audio_Event_Handle_Set and Planning_Active, now "reached in a coverage run" in
  native/behaviour_rewrites.json. Then `n4-ui` (FootClass slot 271), `n4-randmap` (Choose Map @ctl=1450, Create
  Random Map @ctl=1411, Preview @ctl=1568, accept @ctl=1733, Start Game: Read_Settings and 0x0057D3A0) and a chat
  line in lan_pair.py's host script (Get_Edit_Buf, IME_Stub1; native-lan-pair passes). Tried without a new reach:
  a tree destroyed through Take_Damage (it does not die from C4), a Crazy Ivan force-firing at the player's own
  soldier (no bomb), the area-guard Ctrl+Option click and a closed waypoint path (HouseClass::Can_Add_Waypoint_To_Path).
  105 reviewed rows left; most are LAN dialogs, Westwood Online and campaign-menu paths.

### Lane N5 (2026-11-19): the Simulator rerun, the user's own saves, keys that stopped the game

* **Simulator walk on current main** ("RA2 Duo Validation", `ios_sim_menus.py --play-seconds 150`): N4's IsStrange and
  bridge changes were checked on the Mac only. Everything passes in both games (no trap or fault, 61-62 frames/s), and
  the walk gained two checks for the IsStrange fix (N5_ORDERS): tanks fire at a spawned enemy power plant and its
  strength drops (750 -> 590: shells no longer pass through buildings), and an engineer captures a spawned enemy
  Tesla Reactor (the reactor becomes the player's).
* **Saves of the original Yuri's Revenge.** The user's PC has Yuri's Revenge's saves in D:\Games\RA2 (2023 campaign and
  skirmish saves, internal version 178328). Five of them (ALL01UMD, SOV01UMD, SOV07TMD, SAVE0693, SAVE3CAA), fetched
  read-only into a scratch folder (not the repository), each load in the current yr-native and play 1,500 frames; one
  also through the menus' load dialog at 1024x768, and in the one-game app after the toggle from Red Alert 2 (2,000
  frames). The user's crash did not reproduce on the current build. `yr_save_roundtrip.py --original-saves DIR` (or
  `RA2_NATIVE_YR_ORIGINAL_SAVES`) loads every original save in DIR in ctest `yr-native-save` (copied into the run's
  own write folder); without it that part is skipped. One likely cause was found and fixed anyway: loading a game
  (or starting a new one) while a combat light was alive deleted it through the tree's `SpotlightClass` destructor,
  which nothing defined (a trap); now bound to SpotLightClass's (data/name_aliases.tsv).
* **Functions still missing at the Mac link: 110 -> 101.** Traced from their callers to keys and play:
  - Tab in a skirmish (any non-campaign game) opened the Diplomacy dialog, whose fill called RadarClass::FUN_00658330
    (gamemd.exe 0x00658330, one player's row): glue/radar_native.inc, a NON-EXACT stand-in from gamemd.exe's
    instructions (name in the house's colours, flag, alliance and chat boxes, kills). Keyboard_Process's own Tab
    (`SidebarClass::Zoom_Mode_Control` in the tree) is FUN_006ABC40 (alias);
  - the Delete key with no planned node called `BeaconManagerClass::Remove_Beacon`, the tree's other name for
    DeleteBeacon (0x004311C0; alias);
  - hover and mech units refused a destination with TibSun's `Sound_Effect(ScoldSound)`: gamemd.exe calls
    VocClass::PlayGlobal(ScoldSound, 1.0, 0x2000) there (0x00516813, 0x005B03AA; hover_native.inc, mech_native.inc);
  - aliases for one function under two names: `Fill_Surface_Rect(Surface*, Rect const&, int)` (Load_Title_Screen: the
    same 0x0072E820), `OwnerDraw_GetRectangle` (Get_Display_Rect 0x00775690, the progress dialog), the restate dialog's
    `GScreenClassStub::Flag_To_Redraw` (GScreenClass::Flag_To_Redraw 0x004F42F0);
  - LineTrail::LineTrail (0x00556A20; glue/link2_native.inc): types with UseLineTrail=yes.
  ctest `yr-native-keys-load` (yr/tests/yr_keys_load.py; harness order `lights [add]`): Tab shows the Diplomacy dialog
  and its Return closes it, Delete, two combat lights made, then Escape, Load Game, the save's row, Load: the lights are
  deleted and the save plays on. Left (callers checked): Westwood Online and the World Domination Tour, the coop
  campaign, modem play, the random map generator and map previews made from cells, Tiberian Sun's veins and tunnels,
  the score screen of TibSun's ScoreClass (YR uses CampaignScoreClass), VQA movies (YR plays Bink), the trigger
  description writer (Speech_Name, only for writing a scenario), TechnoClass::ForceCreate's base slot.
* **Checks**: the YR and one-game ctests on build/n3 (29) pass; yr-native-standins' hijack failed once in two runs (the
  truck stayed the enemy's; it passed on the rerun and in lanes P1/N4), a flake to watch, not traced.
* **Resume note (lane N5)**: next PORTS_REMAINING item 4 (Red Alert 2's 105 unreached rewrites). Started, not run: Red
  Alert 2's harness `spawn` also makes soldiers, new orders `fire <ID> <TARGET>` (force-fire, own units too) and `hold
  <ID>` (SUSPEND 0x0F, the cameo's right click); coverage_runs.py scenarios `n5-bomb` (a Crazy Ivan bombs the player's
  own Flak Track, a save at 520 while it is planted, `laser`: BombClass Save/GetClassID/Disarm/What_Action,
  Draw_Depth_Antialiased_Line) and `n5-hold` (UnknownFactory::Is_On_Hold). Next: rebuild build/native-cov (the cmake
  line in coverage_runs.py), `coverage_runs.py run ... n5-bomb n5-hold`, `report`, then mark rows in
  native/behaviour_rewrites.json `reach`. The Simulator walk was run before the N5 stand-ins (they are Mac-checked).

### Lane N6 (2026-11-19): Red Alert 2's unreached rewrites, Choose Map, the hijack flake

* **Red Alert 2's unreached rewrites: 105 -> 95** (native/behaviour_rewrites.json `reach` "reached in a coverage
  run"; build/native-cov rebuilt, `coverage_runs.py run`, `report`). N5's `n5-bomb` reached BombClass::Save and
  GetClassID (and the laser line); `n5-hold` reached nothing new: the strip asks UnknownFactory::Is_On_Hold only for a
  cameo the sidebar's own click marked building (+0x10 = 1), never after the harness's PRODUCE event. New scenarios:
  `n6-place` (RA2 harness `spawn` now also puts up buildings: a psychic sensor, a missile silo and a gap generator for
  the player, each clicked, the nuke fired with the silo standing, the sensor destroyed: the sensor array's cell
  marks 0x0047C380/0x0047C390, XSurface::Draw_Ellipse, BuildingClass::Do_MISSION_MISSILE) and `n6-sidebar` (a left
  click on the barracks' cameo: Is_On_Hold; the defence tab's hot key, bound in a KEYBOARD.INI the scenario writes to
  the run's write folder (`files`): 0x006F8A60). Tried without result: a bomb carrier killed before the bomb goes off
  (BombClass::Disarm) and the mouse over it with a tank selected (BombClass What_Action), dropped. `lan_pair.py`'s
  host now presses Tab twice in its lobby (for the network dialogs' Tab branch): run on the coverage build, it reached
  none of the rows (the 4,000-frame coverage pair also stops short of Bravo's second Sound dialog, its exit 1);
  ctest native-lan-pair passes with it. `n6-seeds`: the random map dialog's Save seed button opens the seed list's
  save dialog (MapSeedClass::Save_Menu_Name); its Save did not save, so Load and Delete stayed disabled.
* **Choose Map in a Yuri's Revenge skirmish stopped the game.** Found from the link closure (FUN_005E6920): the
  skirmish lobby's Choose Map (0x5AA) -> Scenario_Dialog -> the dialog's procedure, 0x005E6920, which the pinned tree
  declares and never defines. glue/netshare_native.inc now has it, text adopted from a later ra2-decomp (commit
  5a94489, Scenario_DlgProc and its inline helpers) under the pinned tree's names (0x00608070 is its
  WS_Save_Dialog_Values). ctest `yr-native-choose-map` (yr/tests/yr_choose_map.py): the lobby's own map started, then
  Choose Map, a row clicked, OK, Start Game: the other map is played (XMP29U2 -> XMP02T2 in the first run), no trap.
  Link closure 101 -> 100. **Still stopping**: the dialog's Create Random Map (0x583 -> CreateRandomMap ->
  Do_Random_Map_Dialog -> MapGeneratorClass::Generate_Random_Map), the random map generator being missing from the
  YR tree (Generate_Random_Map, Split_Region, Create_Region_From_Cell, Grow_Water_Region, Load_Settings,
  Pick_Random_Tile_Variant, Blit_Preview, Preview_Tile; the latest ra2-decomp lacks them too).
* **The hijack flake** (yr-native-standins, N5): reproduced, 2 of 4 runs, tied to the start position (one corner
  failed both times, the other passed both). Cause: the YR harness split an order into at most 4 words, so `spawn
  TRUCKA enemy 13 0` lost its offset (as every `spawn <ID> enemy <dx> <dy>` since lane P1) and the truck went on the
  first free cell in rings from the base, up to 11 cells from the thief (63,18 against 74,20; 50,95 against 61,97),
  who had only the 300 frames to 1250. Also, under load (other lanes' builds) the 170 s run reached only 1,229-1,343
  frames, so frame 1250 was at its end or past it. With the truck 4 cells from the thief the thief still stood still
  at that corner (74,20, mission 5 guard at frame 1250: the capture was dropped; the cell 9 east of that base is
  probably cut off, not traced). Now: orders take 8 words; `spawn` takes `near <OTHER> <dx> <dy>` (the offset from the
  player's first OTHER); the thief is placed 3,3 from the base and the truck 3 cells from the thief; the run is 220 s
  (ctest timeout 400 s) and says so when it ends before frame 1250; `owner` lines carry the cell and mission
  (`@x,y m<mission>`). The Simulator walk's N5 orders (`spawn GAPOWR enemy -7 -5`, `spawn NAPOWR enemy 7 -7`) now get
  their offsets too, as written: the walk was not rerun. Checked: 4 of 4 runs pass after the change, two at each of
  the two start corners (the failing one included), one of them under load (1,346 frames in 220 s).
* **Resume note (lane N6)**: item 1 (Red Alert 2's unreached rewrites, 95 left): next, a player leaving a LAN
  game (Destroy_Connection) and other LAN branches, the seed list's Load/Delete (why n6-seeds' Save did not save), BombClass::Disarm (an engineer or a bombed unit entering a transport). Item 3
  (Yuri's Revenge's missing functions, 100): the one a player can click is Create Random Map (PORTS_REMAINING
  "Suggested order" 5); the rest are online, coop, modem, VQA or Tiberian Sun paths (traced by lanes N5/N6). The
  Simulator walk was not run after the YR order fix (its N5 enemy spawns now get their offsets).

### Lane N8 (2026-11-19): Yuri's Revenge's random map generator

* **Create Random Map stopped the game** (PORTS_REMAINING item 5): the skirmish Choose Map dialog's Create Random Map
  (0x583) -> the random map dialog -> Preview (0x620) or OK (0x6C5) -> MapGeneratorClass::Generate_Random_Map, which the
  YR tree (the pinned ra2-decomp and its latest) declares and never defines, nor the generator functions only it
  reaches. glue/mapgen_native.inc (appended to mapgen.cpp): NON-EXACT stand-ins written from gamemd.exe's instructions
  (yr_dis.py) in the shape of Red Alert 2's twins. Each YR body was set beside its RA2 twin in game.exe: Create_Region_From_Cell
  (0x0058C800), Split_Region (0x0058D620), Build_North_Ramp (0x00591D80), Place_Waterfall (0x0059E740), Grow_Water_Region
  (0x005A08D0), Create_Tiberium (0x005A23A0) and Generate_Rural_Roads (0x005A89D0) have the size of their RA2 twins and
  the same calls; Generate_Random_Map (0x00598960, 3,018 bytes against 3,086), Load_Settings (0x005981F0: RMGMD.INI)
  and the start points (0x00594B50: the zone regions inline, the scenario's start points copied at the end, FUN_0058B820)
  were written from gamemd.exe's code where they differ. The text is the rebuild tree's mapgen.cpp and
  main's src/recovered/mapgen.cpp (Split_Region: main's behaviour-checked draft, with gamemd.exe's "no new height 0 on
  the inland and mountainous maps"), under the YR tree's names. Also: MapClass::Pick_Random_Tile_Variant (0x0057B440,
  map_native.inc: main's exact RA2 text; gamemd.exe's shore tables 0x0082A7F4 / 0x0082A89C read at run time),
  IsometricTileTypeClass::Preview_Tile (0x00549E50, isotype_native.inc: the map previews' colours) and an alias:
  Create_Water_Regions' Is_Tile_Swamp is CellClass::Is_Tile_Clear_To_Green_LAT (gamemd.exe 0x0058CFE9 calls it).
  Link closure (Mac) 100 -> 89 (MapPreviewClass::Blit_Preview is left: only the modem dialogs draw with it).
* **About one random map in three had no start point** (both players without units, the game lost at once). The YR
  draft of MapClass::Zone_Reset (map_zones_native.inc, TibSun's text, lane Y3) returned the widest zone's normal
  movement zone; gamemd.exe's (0x0056C510) returns the raw zone itself, which the start points use to index
  Zones[mzone]: no region was made. Now the raw zone (no other caller reads the result). 6 of 6 runs then had both
  players' units, against 5 lost games in 16 runs before.
* **ctest `yr-native-choose-map`** has a third run (yr/tests/yr_choose_map.py, 90 s): Choose Map, Create Random Map,
  Preview, OK 17 s later, Start Game: the seed and preview files (RandMap.Sed, RandMap.img) are written, the game played
  is RANDMAP.SED, the player has units at frame 40 and the game reaches frame 600, no trap or fault.
* **Red Alert 2's random map seed list** (N6's resume list): n6-seeds saved nothing because it clicked the save
  dialog's list (0x527, which copies a row's description into the edit field) and then Cancel (0x686); the dialog's
  Save is 0x6C7 (LoadOptionsClass::Save_Dialog_On_WM_COMMAND). coverage_runs.py's scenario `n8-seeds`: Save, the
  message's OK (0x5AE), then the random map dialog's Load seed (0x6C2; the load dialog's list 0x525 and Load 0x40F) and
  Delete seed (0x6C4; the delete dialog's list 0x528, Delete 0x6C8, the question's OK). Run on build/native-cov (lane
  N6's build; the RA2 side is unchanged since): Red Alert 2's unreached rewrites 95 -> 91 (MapSeedClass::Saved_Message,
  Load_Menu_Name, Delete_Menu_Name, Delete_File; native/behaviour_rewrites.json `reach`).
* **Resume note (lane N8)**: item 1 (YR random map generator) done; the Simulator walk was not run (the generator is
  Mac-checked; the owner's land_ios.sh builds it for iOS). Item 2: the seed list done (above). Next: BombClass::Disarm
  is reached through BuildingClass::Captured (an engineer capturing a building carrying a Crazy Ivan bomb),
  ObjectClass::Delete_Me (a bombed object deleted, e.g. an engineer consumed by a repair or capture) or a BombDisarm
  warhead: the RA2 harness's `capture` takes a cell; a `capture <ID> <TYPE>` form (as `attack`'s) would make the
  scenario; a player leaving a LAN game (Destroy_Connection) not started. Then item 3 (95 unreached rewrites):
  Unknown_006F7020 (select all of a type: the T key with a tank selected, as yr-native-type-select does) looks easy.

### Lane N9 (2026-11-19): the Simulator walk on current main, three more of Red Alert 2's unreached rewrites

* **Simulator walk** ("RA2 Duo Validation", `ios_sim_menus.py --games yr-random,yr,ra2 --play-seconds 150`, the
  simulator app from `native/build.sh ios simulator` at main c6b653428): lane N8's random map generator and lane N6's
  `spawn ... enemy <dx> <dy>` offsets had been checked on the Mac only. Both games pass (60-62 frames/s; the N4/N5
  checks too: the enemy power plant 750 -> 616, the reactor captured). New run `yr-random`: the lobby's Choose Map
  (1450), Create Random Map (1411), Preview (1568), OK (1733) 17 s later, Start Game: RANDMAP.SED plays, 13 player
  units at frame 40, frame 600 reached. The first `yr` run of the walk failed in the lobby: the tap away from the name
  field and the Start Game tap were read 635 ms late in one batch (a two-finger right click); a host stall with the
  Mac's swap nearly full (7.8 of 8 GB). `--games yr` alone then passed.
* **Red Alert 2's unreached rewrites: 91 -> 88** (build/native-cov rebuilt, `coverage_runs.py run n9-bomb n9-ui`,
  `report`; native/behaviour_rewrites.json `reach`). The RA2 harness's `spawn` takes `enemy` (`spawn <ID> enemy
  [<dx> <dy>]`: the house of the first techno that is neither the player's nor an ally's, placed at the offset from the
  player's first techno) and `capture <ID> <TYPE>` (the first non-player object of TYPE, as `attack <ID> <TYPE>`).
  `n9-bomb`: a Crazy Ivan bombs the player's engineer and an enemy missile silo spawned beside the base, the engineer
  captures the silo: BombClass::Disarm (the run does not say whether through the engineer's Delete_Me or the silo's
  Captured); the engineer, alone selected with the mouse over it, asks its bomb's kind for the detonate cursor
  (BombClass::Unknown_00435FF0). `n9-ui`: T held (TypeSelectCommandClass's stopwatch, 0x00AC133D) and a Shift click on a tank: on an
  unselected one every tank is selected (0x006F71E0), on a selected one every tank is unselected (Unknown_006F7020);
  `RA2_NATIVE_INPUT_LOG=10` shows 5 tanks selected, then none.
* **Checks**: ctests native-basics, native-tank-damage, native-hd-vehicles, native-link-closure, native-asm-check on
  build/n3 pass with the harness change.
* **Resume note (lane N9)**: next, a player leaving a LAN game (SessionClass::Unknown_00672F20 through
  Destroy_Connection: lan_pair.py's host leaves at the end, but Bravo's coverage of that was never seen; a lobby
  leave-and-rejoin variant is the other way), then the 88 left, most-called first.

### Lane N10 (2026-11-19): a player leaving a LAN game, eight more of Red Alert 2's unreached rewrites

* **A player leaving a LAN game** (N9's resume list): coverage_runs.py's `n10-leave` runs `lan_pair.py --background
  --extras` (9,000 frames) on the coverage build: Bravo, playing as the iOS app, leaves when its background time runs
  low (the EXIT event); Alpha processes the EXIT on frame 6,755 and Destroy_Connection 0x005BB5D0 removes Bravo from
  the session's players: SessionClass::Unknown_00672F20 reached. The pair passes (6,756 frames with both CRCs, none
  differ; no reconnect dialog). No lobby leave-and-rejoin variant was needed.
* **Red Alert 2's unreached rewrites: 88 -> 80** (native/behaviour_rewrites.json `reach`). An input script's point
  `@cmd=NAME` is the RA2 command bar's button for a command (link/team_button_hook.cpp, asked by harness_input.cpp; as
  the YR glue's). `lan_pair.py --extras` (coverage only, not the ctests): the host presses the multiplayer bar's Beacon,
  places a beacon, closes its text line and presses Delete over it: Unknown_006F64B0; the same run reached
  Audio_Event_Type_Of (a sound with an audio controller; which one not traced). `n10-bar` (skirmish): a tank selected and
  Ctrl+Alt held over open ground: the guard-area cursor Unknown_006F6740; Delete: unknown_find_617a00 (each confirmed by a
  run of just those steps); ObjectClass slots 25, 48 and IsDisguisedAs reached in the first run only (not in a rerun or
  the part runs; trigger not traced, marked so). Not reached: the single-player bar has only Deploy, Guard,
  PlanningMode, TypeSelect and the teams (UI.INI [AdvancedCommandBar]: no Attack Move, so 0x006F6730 stays); waypoint
  mode (Z) with the mouse on an empty cell, also after selecting the path by hovering its waypoint, did not ask
  HouseClass::Can_Add_Waypoint_To_Path; the host screen's Short Game check box after Bravo's Accept (to reset the
  ready states) was not found by `@ctl=1358` on the host's screen (dropped).
* **Yuri's Revenge's missing functions (89)**: re-checked those with a gamemd.exe caller outside the families N5/N6
  listed: OwnerDraw::SysTabHandler (no dialog template in gamemd.exe has a SysTabControl32), PrepareDialogChildControls
  (UI::StandardWndProc's WM_INITDIALOG with a zero init parameter only), FUN_0049B430 (coopcamp.cpp), FUN_0046DFD0
  (CampaignScoreClass::Show's dialog, the coop campaign's), Get_Language_Version (the version dialog), Send_Statistics_Packet
  (Internet games), DoFindPage, FUN_00778A20/0077B2A0/00785360/007A2800/0079FC40 (Westwood Online). None reachable in
  skirmish or campaign play.
* **Resume note (lane N10)**: 80 reviewed rows left, mostly default virtual slots, audio controllers, network-dialog
  branches (Set_Player_Map_State / Handle_Reset_Ready need a host-screen change after an Accept: find the host screen's
  check box with RA2_NATIVE_MSG_LOG on Alpha), EM pulse and overlay saves, the 8-bit and backward blitters.
  Can_Add_Waypoint_To_Path: trace why ScrollClass::What_Action's waypoint branch is not taken (the input log around Z).

### Lane N11 (2026-11-19): Red Alert 2's unreached rewrites, 80 -> 53

* **No open user-reported fault** in docs/PORTS_REMAINING.md (every Duo and Mac fault there is fixed and waits for
  the user's check), so the lane went to N10's resume list.
* **Reached in coverage runs (4)** (build/native-cov rebuilt, `coverage_runs.py run`, `report`): `n11-exit-audio` (the
  main menu's Exit Game with audio on: AudioSource::Finish as the audio cache is freed); `n11-terrain` (RA2 harness
  order `super <index> terrain [mine]`: a super weapon at the terrain object nearest the base; the Iron Curtain over a
  tree reaches ObjectClass::IronCurtain; `terrain kill [mine]` damages that object, which did nothing: the ore mines are
  Immune and C4 is not a Wood warhead); `n11-lan` (lan_pair.py --extras: the host screen's Choose Map (0x5AA), a row,
  OK, then Tab twice, all before Start Game: Unknown_005bbd10 through Scenario_Select_Callback in a LAN game; the
  4,000-frame pair exits 1 only because Bravo's second Sound dialog comes later, as N6 saw); `n11-ui` (a Grizzly tank
  selected, Ctrl+Shift held over open ground: attack-move's cursor Unknown_006F6730). An input script's point
  `@!ID` is now the newest object of type ID of a house the player does not control (harness_input.cpp).
* **Why N10's host-screen click found nothing**: an input script runs its events in their order, each waiting for
  the one before, and lan_pair.py appended the extras after Start Game; the lobby extras now come first. The same
  holds for the ctests' two Tab presses (N6): they land in the game, not the lobby (left as they are).
* **Keys**: shared/keyboard.hpp calls the 0x00A40BB4 pair KeyForceMove and 0x00A40BA4 the unsettled third pair; by
  behaviour 0x00A40BAC is Ctrl, 0x00A40BB4 Shift and 0x00A40BA4 Alt (Ctrl+Alt guard area, Ctrl+Shift attack move).
  src/ is not this lane's to rename.
* **Shown not to run (23)**, from game.exe's instructions (rel32 calls and jumps, imm32 references, `call [reg+slot]`)
  with mischa85's names.csv and the shipped rules.ini (native/behaviour_rewrites.json `reach_reason` has each):
  Unknown_0075ca00 (WOL chat only) and Unknown_0073bdb0 (session type 4 only): "unreachable in the port";
  Can_Add_Waypoint_To_Path (waypoint mode is never switched on: every call of 0x0049C860 passes 0), EMPulseClass
  Save/Kind_Of/Size_Of (its constructor has no caller), MSSfx::~MSSfx (Add_Sfx has no caller), WideString 0x00770400
  (MSPrintAnim's constructor has no caller), UnknownFactory::Get_Special_Item (+0x64 is only ever -1), CanCheer and
  slots 66/92/102/113/114 and TechnoClass 194/240 (called only on technos or classes that override them, or never), and with the shipped rules
  ReceiveGunner/RemoveGunner (Gunner only on FV), ClearDisguise, ObjectClass slot 74 (HasRadialIndicator only on
  buildings), Cell::Chain_Reaction_Damage (ore mines Immune; no ChainReaction overlay) and HouseClass 0x004F3160 (no
  IsThreatRatingNode): "unreachable in the base game".
* **Checks**: ctests native-basics, native-tank-damage, native-link-closure, native-asm-check and native-lan-pair on
  build/n3.
* **Resume note (lane N11)**: 53 reviewed rows left. Next: the ObjectClass default slots with many call sites (35:
  42 sites, 51/50 GetDisguiseHouse/GetDisguise: about 20, 93: 18, 111/112: 35-44, 115: 12) need each site's receiver read (`call [reg+0xNN]` with the slot's offset; a scratch
  scanner is simple); TechnoClass 189/195/202/224 and Exit_Object the same; the game-mode slots need a LAN game in
  Unholy Alliance or with an observer (the host screen's game type list: find its control with RA2_NATIVE_MSG_LOG on
  Alpha, as lobby extras before Start Game); Build_By_String needs a desync (Print_CRCs) or the version dialog, which
  nothing opens (Version_Dialog 0x00516E70 has no caller).

### Lane N12 (2026-11-19): the Mirage tank's first camouflage animation

* **The user's Duo report**: the camouflage still draws corrupted graphics on its first animation, every later one
  correct (after lane N4's colour fix). The "animation" is the owner's morph: a player's disguised Mirage fades out
  (GetDisguiseFlags: translucent at phases 64-79), shows the tree (IsClearlyVisibleTo false at phases 72-119) and
  fades back in (112-127), every 256 frames from DisguiseCreationFrame.
* **Not reproduced on the Mac or in the Simulator.** Window crops of the tank on every frame (harness
  `RA2_NATIVE_CAMO_DUMP=<dir>`) through the first and later morphs: a spawned tank (ScenIndex 0, 2, 5, 9, 14: four
  theaters' maps), one built by a war factory (`produce GAWEAP MGTK`, the first disguise at the facing it drove out
  with), one moved and disguised again, and "RA2 Duo Validation" with the camouflage watched from its spawn: the
  first morph is pixel for pixel the second (the converter choice matches gamemd.exe 0x00705E45-0x00706129; the tree
  takes Unit_Draw_Shape's 16-bit path, TerrainPalette clear). No fix was made: the cause is not on the paths the Mac
  and the Simulator take.
* **ctest `yr-native-mirage` now checks the first animation**: the watch starts with the spawn (frame 60), the box
  around the tank on each frame of the first cycle's morph (phases 64-127; the first frame seen is skipped, since the
  composed view still holds the frame before the disguise) is kept and the second cycle's same frames are compared
  with it (`harness-camo-morph compared <n> differ <max> at frame <f>`; 62 compared, 0 differ). A fault confined to
  the first cycle (tried: house colours until frame 200) gives 2,521 differing pixels and fails, where the old
  averages passed (706 green, 56 blue). The green floor is 40 (an olive tree on desert ground counts about 70; the
  disguise is a random DefaultMirageDisguises tree, so runs differ).
* **Asked of the user**: a screenshot or a description of the first animation on the Duo (noise, the tank's colours,
  the tree in blue and white, a black box), and whether the app on the Duo was built after lane N4's landing.

### Lane N13 (2026-11-19): Guardian GIs that would not deploy

* **The user's Duo report**: in the first Allied mission the Guardian GIs (GGI) are meant to deploy by themselves and
  stood and died. Reproduced on the Mac in a skirmish (harness `deploy GGI`: Do_MISSION_UNLOAD gives the deploy
  action 0x1B and the soldier stays in it, never reaching 0x1C, deployed) and in ALL01UMD (0 of 10 deployed, all 10
  in 0x1B). The GI (E1) and the Desolator did the same.
* **Cause**: InfantryClass::Doing_AI moves on from 0x1B when the animation's stage reaches the sequence's count, and
  Do_Action sets the animation's rate from `MasterDoControls[todo].Rate`. infantry.h declares that table
  `[DO_COUNT]`, TibSun's 21 actions (do.hh); Yuri's Revenge has 42 (infatype.cpp's DOINFO_COUNT; gamemd.exe
  0x007EAF7C holds 42 four-byte entries, entry 0x1B `0 0 0 1`). yr_data.py sizes a global's storage and load from
  its declaration, so 84 of 168 bytes were loaded and entries 21-41 (deploy, deployed, undeploy, cheer, paradrop,
  panic, ...) read the next storage: rate 0. fixups.json now declares `MasterDoControls[42]`. An audit of the loaded
  arrays shorter than the gap to gamemd.exe's next named global found one more of the same kind:
  `MouseClass::MouseControl[MOUSE_COUNT]` (TibSun's 66 cursors; gamemd.exe 0x0082D028 has 86, the tree uses
  (MouseType)0x4C/0x4D), now `[86]`. The others in that list are unnamed gaps or are full (ActionName's 73 entries
  end where code pointers start; Powerups::Anims was lane N4's).
* **Harness orders** (glue/yr_harness.cpp): `doing <ID>` logs `harness-doing <ID> do<DoType> m<mission> deployer
  deployed s<strength>` (a soldier's action, a vehicle's Deployed) for the one the last `deploy <ID>` ordered, else the
  player's first; a second `deploy <ID>` orders the same one again (undeploy); `deployed <ID>` counts every soldier of
  the type on the map: `harness-deployed GGI <n> of <total> (<n> deploying) dead <n>`.
* **ctest `yr-native-deployers`** (yr/tests/yr_deployers.py, two runs): a skirmish where a GGI, a GI and a Desolator
  (away from the others: its radiation kills them) deploy (do28 at frame 300) and undeploy (frame 500) and a Siege
  Chopper deploys; New Campaign's first Allied mission, where every GGI is deployed at frame 300 (10 of 10). On the
  build without the fix it fails (all do27; 0 of 10).
* **Checks**: the Yuri's Revenge and one-game ctests on build/n3 (29 of 30 in the first run, Wine pair left out);
  `yr-native-mirage` failed once (the first and second camouflage morphs differed by 289 pixels at frame 321) and
  passed the next three runs with 0 differing; probably something passing through the box (not traced; watch it).
* **Not checked**: the Simulator (the fix is data the iPhone build loads the same way); Red Alert 2's GI (its table,
  `unknown_do_controls_7a3474`, is loaded with 160 bytes, 40 entries, beyond its 38).
* **N11's list, 53 -> 52**: ObjectClass slot 115 (+0x1CC) shown not to run: game.exe's 12 calls through it are all on
  technos (BuildingClass members on the building, TechnoClass draws, ParasiteClass::UpdateSquid's owner, and
  ObjectClass::Paradrop, whose only caller through slot 56 is AircraftClass::Paradrop_Cargo's passengers), and
  TechnoClass overrides it (behaviour_rewrites.json has each site). Slot 93 (+0x174): 17 of its 18 calls are on
  technos (the locomotors' LinkedTo, Tunnel_AI); the 18th is ObjectClass::AI 0x005D2904 on `this` when a falling
  object lands, so it needs which non-techno objects can have IsFalling set (left reviewed).
* **Resume note (lane N13)**: the GGI item is done. Next: N11's list (52 reviewed RA2 rewrites): slot 93's falling
  objects, then slots 111/112 (+0x1BC, 44 calls; +0x1C0, 35), 35, 51/50 the same way (a scratch scanner over
  `call/jmp dword ptr [reg+off]` with names.csv for the enclosing function is quick).

### Lane N14 (2026-11-19): the mirage flake traced

* **`yr-native-mirage`'s intermittent failure** (lane N13: 289 pixels at frame 321; again here under ctest -j4: 508 at
  frame 332, and 691 at frame 318 in a run with `RA2_NATIVE_CAMO_DUMP`): the boxes of the two disguise cycles differed
  only below and right of the tank, where the start's GIs stand: they had moved and changed pose between frame 62 and
  frame 318. The camouflage itself (the tree above the tank) was the same pixel for pixel. Not a game fault: the
  comparison now counts only the disguise's own box (Camo_Pixels': 40 left to 10 right of the tank, 70 above it; on
  the dumped failure 20 pixels, under the 40 tolerance, every other dumped run 0). Moving the spawn (0,-6 or 6,0
  instead of 3,0) did not help: GIs wander there too. Eight runs after the change, four at a time: all pass (0 differing in six, 20 in two).
* The Red Alert 2 side of the lane (data extents, unreached rewrites) is in docs/PORTS_FACTS.md.

### Lane N16 (2026-11-19): the deployers flake traced

* **`yr-native-deployers`' intermittent "E1 did not deploy: animation 0 at frame 300"** (lanes N14, N15; here 3 of 4
  runs at four at a time, animations 0, 9, 10): not a game fault and not timing. `deploy E1` ordered the player's first
  GI, which in the skirmish is one of the ten starting soldiers, not the one spawned beside the Guardian GI. That
  GI deployed (do28 by frame ~110 in every traced run), but depending on the start position and where the starting
  soldiers stand it was within the deployed Desolator's radiation: its strength fell 125 -> 7 over frames 100-200 and
  it died. `doing E1` then fell back to the player's next GI (standing, walking or firing: do0, do2, do9, do10) and
  frame 320's `deploy E1` deployed that one, hence "deployed 1" at frame 500. The radiation killing a GI is the
  original's behaviour. The spawned GI and the Guardian GI (12 cells from the Desolator) were never hurt.
* **Harness** (glue/yr_harness.cpp): a player's `spawn <ID>` now names that techno for its type, so `deploy <ID>` and
  `doing <ID>` take the spawned one; once the named one has left the map (killed, or into limbo) `doing` logs
  `harness-doing <ID> gone` and `deploy` `harness-deploy: <ID> gone` instead of quietly taking another. The test reports
  a gone soldier as such. `deploy CAHSE07` (yr-native-garrison) and `deploy AMCV` (yr-native-graphics) take the same
  unit as before (the spawned house; the MCV is not spawned).
* **Checks**: see the lane's hand-back (10 consecutive deployers passes, runs four at a time and under ctest -j4).

## Next: iOS and Windows

* **iOS.** No `dlopen` of app-supplied code on a device (and no loose bundles in an app): both
  games must be linked into the one executable. The design and its size: "iOS in one executable"
  below (lane Y8).
* **Windows** (built by lane Y16: "One game on Windows" above). Two DLLs (`ra2-engine.dll`, `yr-engine.dll`) built from the same objects with
  only `ra2_engine_main` exported (PE exports are explicit: nothing else is visible), loaded with
  `LoadLibrary` from a fresh copy per start, `RA2Native.exe` as the thin host. The engines' C
  runtime must be the shared UCRT DLL (each DLL with a static CRT would keep its own heap and
  `atexit` list; the DLL's own list runs at `FreeLibrary`, which is not called for the reason
  above: the thin host calls the engine's exported finaliser instead). The window and SDL3.dll
  are shared as on the Mac.

### iOS in one executable: design (lane Y8; stage 1 built)

The Mac's engines are bundles mapped afresh per start; iOS allows neither `dlopen` of app code nor
writable-executable pages, so both games are linked into `RA2YR` (one app, bundle id to choose:
keep `io.github.ra2native.RA2` so RA2.app's saves and settings carry over) and a game's fresh start
becomes a reset of its own data in place.

1. **Symbols, at the object level.** Each game's objects (`ra2_recovered_objs` + `ra2_data` + its
   link_closure.c + its copy of `ra2_port` and of the SDL host compiled with `RA2_NATIVE_ENGINE`)
   are linked into one relocatable object, `ld -r -d -exported_symbol _ra2_engine_main_<game>`
   (`-d` defines commons; ld64 turns the symbols not exported into static ones in a `-r` output
   unless `-keep_private_externs` is given), with `-alias_list` applied there. The two objects and
   the thin host link into the app; nothing else of either game is visible to the other, as with
   the bundles. Each `-r` input list starts and ends with a marker object (`engine_begin_<game>.c`,
   `engine_end_<game>.c`: one symbol in each of `__text`, `__data`, `__bss`, `__common`,
   `__const`), so the game's share of every section is contiguous and bounded by known symbols.
2. **What stays single.** SDL (static on iOS, with its Objective-C classes) and the Apple host
   (`host_apple.mm`: `RA2ImportPicker`, `RA2PassView`, `RA2Toolbar`, the glass buttons) are left out
   of the engine objects and linked once: Objective-C class names are global in the runtime whatever
   the symbol visibility, so two copies would collide. The engines' references to `SDL_*` and
   `hostos::*` stay undefined in their `-r` objects and bind to the one copy. hostos keeps no game
   state across a swap except the toolbar, which the thin host owns (as the window already is).
   The import flow (RA2 files plus YR's 28 files, one folder picker) and the settings' `game=` line
   move to the thin host (`combined/one_game.cpp` gains the iOS app start that `main.cpp`'s app mode
   has now).
3. **Initialisers.** Both games' static initialisers must run at that game's start, not at launch:
   the YR units already have theirs moved to `__DATA,__yr_init_func` (defer_initializers.py); the
   same pass runs over the RA2 game's objects and the per-engine port and host objects into
   `__ra2_init_func` / `__yr_init_func`. On iOS 17 the linker emits `__TEXT,__init_offsets`
   (32-bit offsets) for the app's own initialisers; the moved sections keep plain pointers, rebased
   by the chained fixups like YR's today.
4. **A fresh start.** The first start of an engine copies its data range (the markers' `__data`,
   `__bss`, `__common` spans, about 5 MB for YR) to a pristine buffer before its initialisers run
   (pointers are already rebased: the copy is taken after dyld). Each later start copies the
   pristine bytes back, runs the engine's initialisers again and loads the original's data again
   (yr_data / native data, as now), so it starts exactly as a process start would. `__DATA_CONST`
   (`__got`, read-only after launch) is never touched.
5. **An end.** The game's own shutdown runs as on the Mac (Exit Game, `WinMain` returns). Its exit
   handlers run through `__cxa_finalize_ranges` over its `__text` span (the markers), as the Mac
   host does over a bundle's image range. Unlike the Mac, the code stays live for the next start of
   the same game, so its threads must have ended first: the port gains `ra2port_stop_threads()`
   (the multimedia timers' `std::thread`s, `CreateThread`'s pthreads: the mouse thread, the
   network's async thread, run_progress's watchdog) called before the reset, each joined or
   signalled and waited for. Memory the game did not free stays allocated (as on the Mac).
6. **Toggle, movies, saves.** Unchanged from the Mac (`ra2port::set_game_toggle`, `skip_intro`,
   `Documents/Game` and `Documents/Game/YR`).

**Built so far (lane Y8).** Stage 1, `-DRA2_NATIVE_ONE_EXECUTABLE=ON` (with COMBINED): `ra2yr-one`
on the Mac and `native/build-ios.command simulator --one` (`RA2YR.app`, bundle id
`io.github.ra2native.RA2YR` for now). Items 1, 3, 4 and 5 as above, except that the thread stop is
not done (a swap back to a game whose threads still run has not failed yet) and that item 2 is
an interim: SDL is linked once (the engines leave it undefined), but each engine still has its
own `host_apple.mm`, with Yuri's Revenge's three Objective-C classes renamed (`YRImportPicker`,
`YRPassView`, `YRToolbar`) so the runtime has no duplicate. The markers: `__data`, `__bss` and
`__common` (ld -r puts zero-fill there too: the closure's placeholders are built `-fno-common` so
none stays a tentative definition the final link would merge between the games). ctest
`one-exe-toggle` runs lane Y5's route on `ra2yr-one` (Red Alert 2 -> Yuri's Revenge -> Red Alert 2,
the third start a reset of its data, then a skirmish): passes. Simulator: `ios_sim_skirmish.py
--app build/ios-one-simulator/RA2YR.app --bundle io.github.ra2native.RA2YR --launch-arg=--game=yr
[--toggle]`: a 180 s Yuri's Revenge skirmish against three computer players passes; `--toggle`
(the YR main menu's toggle, then the play in Red Alert 2) found YR's exit crashing with sound on
(lane Y8 section: wpaudio's device, fixed). The import is one flow: each engine of the one-game app takes both games' files
(app_mode.hpp `import_wanted` under RA2_NATIVE_ENGINE; an import replaces the whole GameData, so
before this the second game's import threw the first game's files away): a fresh RA2YR.app started
in Red Alert 2 imported 29 files (1.89 GB) once, and Yuri's Revenge after the toggle found its own
(`--game=ra2 --toggle`: the swap with sound, Yuri's Revenge's menus and a skirmish at 62 frames a
second, 140 MB). Not done: the thread stop, one host_apple copy, a device build of RA2YR.app.

Size (lane time, one lane): the two `-r` objects with markers and the app link in CMake (iOS and
the Mac, so the Mac can test it without bundles) 1 day; RA2's initialisers deferred, the pristine
copy and reset, finalisers by range 1.5 days; the port's thread stop 0.5 day; SDL and hostos once,
the thin host's iOS start (window, toolbar, import of both games, settings) 1 day; the Simulator
route (toggle both ways, a skirmish in each, a swap during the menus) and a device build with the
bundle audit 1 day. About 5 days (35-45 hours), the main risks being state that survives a reset
outside the data range (the port's heap-held singletons are inside it; SDL's and hostos's are not
and must be released per swap) and the executable's size (YR.app's is 14 MB, 7 MB of it code; two games about twice that).

### The one-game app as the iPhone app (lane Y9, 2026-11-16)

* **The app.** `native/build-ios.command simulator|device` builds the one-game app by default
  (`build/ios-one-<platform>/RA2YR.app`; `--ra2` the Red Alert 2 app alone, `--yr` Yuri's Revenge's).
  Owner ruling: it replaces the Red Alert 2 app, with its bundle id `io.github.ra2native.RA2` and
  name "Red Alert 2", so the Red Alert 2 app's import (GameData), saves (Documents/Game), settings
  and RA2.INI carry over; Yuri's Revenge writes under Documents/Game/YR. The standalone YR app
  (`io.github.ra2native.YR`) stays buildable.
* **Import.** In the one-game app an import adds to what was imported before
  (`app_mode.hpp import_game_files` under RA2_NATIVE_ENGINE): a file the folder has the same
  (same size; small files the same bytes) is hard-linked from the old GameData, a file it lacks is
  kept (linked), a file it has otherwise replaces the old one; the staging folder is checked and
  swapped in by rename as before. So a player who had the Red Alert 2 app is asked, at the toggle to
  Yuri's Revenge, only for Yuri's Revenge's files (the prompt says the Red Alert 2 files are already
  there; picking the full installation copies only Yuri's Revenge's ~0.9 GB); a fresh install
  imports both games in one pick (the prompt names both). "Not Now" at Yuri's Revenge's import
  goes back to Red Alert 2's main menu (`app::other_game_ready`, engine->swap) instead of leaving
  the app. After a swap the engine takes the window the previous one used (it made a second
  full-screen window before).
* **Threads before a reset.** `ra2port_stop_threads` (port/src/run_progress.cpp), handed to the
  thin host as `ra2_engine_host::stop_threads` (API version 2) and called after the ended engine's
  exit handlers: those end the game's own threads (Prog_End's Kill_Mouse_Thread, YR's
  FUN_007B86B0); then the multimedia timers it left are stopped and joined, the WSAAsyncSelect
  thread joined (it was detached and looped for ever), the run's watchdog woken and joined (it
  slept on a plain sleep), and the CreateThread threads still alive are waited for (3 s) and named
  in the log. The port's state those use is never destroyed (function-local heap objects), since
  the exit handlers' static destructors run first. Mac: `one-exe-toggle` and `one-game-toggle`
  show "0 timer(s), the watchdog, 0 game thread(s) still running" at every swap (before the order
  change the mouse threads of both games were still running at the stop).
* **One Apple host.** `host_apple.mm` is compiled once (`ra2_engine_hostos_objs`): linked into
  `ra2yr-one` beside the thin host and into each Mac bundle; the engines' `-r` objects leave
  `hostos::*` undefined and bind to it. The YR class renames are gone. What it keeps across a swap
  calls the engine that started last: `toolbar_create`, `skip_button_create`,
  `keys_button_create` and `watch_audio_interruptions` rebind their callbacks when the toolbar or
  observer already exists.
* **Saves.** Red Alert 2's saves work in the one-game app as in RA2.app. (Since lane Y10 Yuri's Revenge
  saves in gamemd.exe's format: "Saves in gamemd.exe's format" below.) Until then Yuri's Revenge could not save: the port writes saves only in the original's compound format, from the field maps of Red
  Alert 2's classes (gen/original_layouts.cpp), and the YR build has none, so YR's Save_Game fails
  cleanly ("stg: ... no field maps"). With the port's own container (the test hook
  RA2_NATIVE_TEST_PORT_SAVE=1) YR's Put_All writes a 762 KB save on the Mac without a trap; loading
  it is untested. Test harness: RA2_NATIVE_SAVE_AT=<frame>[,<file>] in the YR build too
  (glue/yr_app_hooks.cpp).
* **Simulator route** `yr/tests/ios_sim_one_game.py` (ctest-style, "RA2 Duo Validation" only):
  the carry-over route (a Red Alert 2 import only -> an RA2 skirmish and save -> the toggle, YR's
  files added, a YR skirmish and save attempt -> YR chosen last -> RA2 -> YR again, a reset -> a YR
  skirmish -> RA2 loads its save) and `--fresh` (one import of both games, then the toggle).

### Saves in gamemd.exe's format (lane Y10, 2026-11-16)

Owner ruling (lane Y9's open item): Yuri's Revenge saves only in the original's format, gamemd.exe's, as
Red Alert 2 does game.exe's (native/README.md "Original saves"): a compound file whose CONTENTS stream
holds the i686 object images, pointers as 4-byte swizzle ids, vtables as gamemd.exe's.

* **Field maps.** `tools/original_layouts.py --game yr` (a build step: `gen-yr/original_layouts.cpp`,
  linked with `yr_data_obj`) lays out the YR units for i686-pc-windows-msvc and for arm64 and pairs them
  member by member, for the types in `yr/data/original_save_types.txt` (95 rows; each size is gamemd.exe
  1.001's: Fetch_Object_Size, vtable slot 12, a locomotor's slot 9, or the push before a sizeof read) and
  every record of the units that read streams: 3,000 maps, 0 problems. The i686 dump has no C++ library
  and no host headers: `yr/sdk/i686` stands in for the few C++ headers the tree includes, and the tool
  makes i686 preludes in the build (ra2port/prelude.hpp with RA2_LONG as long, the YR prelude without the
  host's C types and _WIN64, mingw's `__LONG32` as long). ctest `yr-native-original-layouts` checks it.
* **Vtables at run time.** There is no gamemd.exe manifest, so a vtable pointer becomes a VPTR_NAMED
  operation naming its class by its MSVC RTTI type descriptor (`.?AVUnitClass@@`,
  `.?AV?$DynamicVectorClass@PBVAircraftTypeClass@@@@`; the tool mangles the names, 853 of them); the port
  finds that class's vtable at that offset among the complete-object locators of the user's gamemd.exe
  (`original_vtable_named`, port/src/original_image.cpp: vtables in data sections whose first slot is
  code). Every type of the save list resolves in gamemd.exe 1.001.
* **The streams.** The Win64 headers' ISequentialStream gets the RA2 tree's typed Read/Write overloads
  (`tools/yr_sdk.py`, objidlbase.h/objidl.h, in an extern "C++" block).
* **Fixes in the YR units** (`fixups.json`, lane Y10): swizzle ids are intptr_t (the tree's LONG truncated
  addresses; the Win64 LONG_PTR is long long, which the typed reads would take as 8 bytes) and an object's
  own id is written as its address, a pointer, so it gets the id every pointer to it gets; the swizzle
  walk is bounded and reports unmatched fields as the RA2 tree's does; LZO1X-1's dictionary keeps 32-bit
  offsets (8-byte pointers ran 64 KB past CStreamClass's buffer: the CONTENTS stream would not
  decompress); LocomotionClass::Load reads its image through its type, not void *; HouseClass's starting
  units write their Type through the member. In the port a pointer written with VC6's size 4
  (LayerClass::Save's sizeof(int)) is written as its id. Draft: FUN_004E02C0 (the ILocomotion pointer's
  _Release, glue/foot_native.inc).
* **Loading for tests.** `--load-save=<file>` works in yr-native too (LoadOptionsClass::Load takes the
  host's resume request and runs the load dialog's steps with that row); `--run-to=frames --frames=N`
  ends N frames into the game (glue/yr_app_hooks.cpp).

ctest `yr-native-save` (yr/tests/yr_save_roundtrip.py): a skirmish saved at frame 500, loaded from the
menus and played 300 frames, saved again at frame 50 (a save written after a load keeps the ids) and that
loaded and played 300 frames; a save of the user's installed Yuri's Revenge is loaded too when the data
folder holds one (by its internal version, 178328; read in place, never copied). The data folder here
has none (its saves are Red Alert 2's), so that part is reported as skipped; the user can make one on
Windows. ctest `one-game-saves` / `one-exe-saves` (yr/tests/one_game_saves.py): in the one-game app each
game saves after the toggle (RA2 -> YR, YR -> RA2) and each save loads back in its game.

iOS: the one-game app's Simulator route (`yr/tests/ios_sim_one_game.py`, "RA2 Duo Validation") now
expects Yuri's Revenge's save in Documents/Game/YR after the toggle and loads it (step 3b): all checks
pass (the save at frame 900, the loaded game at 62 frames a second; the iOS maps: 2,999).

Not checked yet: the installed gamemd.exe loading a save the port wrote (needs Windows). A loaded game
redraws its command bar only once the mouse passes over it.

## Lane Y11 (2026-11-17): stability, the shell's backdrop, the toggle as a menu button

* **TechnoClass::Destroy** (ObjectClass vtable slot 0x170) is the tree's TechnoClass::FUN_00710460
  (gamemd.exe's TechnoClass, FootClass, BuildingClass, InfantryClass and AircraftClass vtables hold
  0x00710460 there; data/name_aliases.tsv). A unit dying while mind-controlling others reached the
  trap in `yr-native-skirmish-long` under ctest -j4 (496 s); the test passes under -j4 since.
* **NON-EXACT drafts** (each labelled in its file): TeamClass::AI (glue/team_native.inc, gamemd.exe
  0x006E9140: the RA2 twin's text, the script step through the tree's own TMission_* handlers by
  gamemd.exe's jump table), BulletClass::Unlimbo, AI and Shrapnel and Projectile_Motion
  (glue/bullet_native.inc: the RA2 twins and TibSun's text with gamemd.exe's YR changes). A 20-minute
  game against three hard opponents first reached TechnoClass::Fire_At (gamemd.exe 0x006FDD50, 7,167
  bytes) about 16 minutes in; it is still a trap (the RA2 twin, src/recovered/techno.cpp, is exact and
  the natural text for the draft).
* **The shell dialogs' backdrop** (user report 2026-11-17, the Duo: Options black, Enhancements noise,
  Set Game Resolution blank): FUN_0060CF00 keeps each dialog's backdrop art (a converter and two shapes)
  as ints at the record's 32-bit offsets 0x74/0xE0/0xE4, read back by FUN_0060C4E0/FUN_0060CE40/
  FUN_0060CEA0 and passed by OwnerDraw::Paint to FUN_0072E730 through an int declaration. Natively the
  pointers were cut to 32 bits (and the dialog id was read off the record start, not the Dialog view),
  so the art was drawn from garbage. fixups.json keeps them by window at pointer width. Every shell
  dialog now has its own backdrop (the skirmish setup its parachute picture, which had been the red
  map). ctest `one-game-menu-backdrop` (yr/tests/menu_backdrop.py) walks main menu -> Options ->
  Enhancements -> Options -> main menu in both games and checks the backdrop region.
* **Set Game Resolution**: Yuri's Revenge runs its shell at 800x600 and only the game at the host's
  play size, so the port's mode list (current mode plus the usual ones) lacked the play size; the host
  now gives it to the port (ra2port_set_play_mode) and the list carries it.
* **Focus loss in the menus**: the one-game app's YR engine had a link-closure trap for
  ra2port_game_redraw_pending and aborted when the app went to the background (the Duo's log);
  glue/yr_app_hooks.cpp answers it from SurfacesRestored as RA2's hook does.
* **Touch**: YR's own "NewEdit" text fields raise the on-screen keyboard (port: ra2port_text_input_wanted),
  and the game cursor is hidden on touch (glue/yr_app_hooks.cpp ra2_native_hide_game_cursor) as in RA2.
* **The game toggle** is a regular main-menu sidebar button in the slot above Exit Game in both games
  ("Play Red Alert 2" / "Play Yuri's Revenge"; port/src/user32.cpp place_toggle, the game-side hooks give
  it Exit Game's kind); the corner push button and its movie-frame redraw are gone. one-game-toggle
  still clicks it by id.
* Simulator walk: `yr/tests/ios_sim_menus.py` (not a ctest) runs both games' shell, a skirmish's setup,
  load and first play with the same settings on "RA2 Duo Validation" and checks the backdrops.

## Lane Y11b (2026-11-17): the loading screen, Fire_At, the menus' size

* **The loading screen** (glue/ldprgmgr_native.inc, NON-EXACT): the map preview (MapPreviewClass, the
  scenario's [Preview] or a random map's picture) and the texts: the country's name on a dimmed strip,
  its special unit in black on the title band, its briefing (lower in a cooperative game) and Loading,
  from Red Alert 2's loading-screen text with YR's places and tenth country (gamemd.exe
  0x005536A0-0x005540E7). The campaign screen is still not drawn.
* **NON-EXACT drafts**: TechnoClass::Fire_At (glue/fire_at_native.inc, gamemd.exe 0x006FDD50: Red
  Alert 2's text with YR's drain weapon, spray attack, area fire, disk laser, damage multipliers,
  occupant cycling, berserk rearm and magnetron; the railgun's beam damage, gamemd.exe 0x0070C690, left
  out), BuildingClass::Drop_Debris (glue/building_native.inc, 0x00442D90: TibSun's text with YR's
  release of absorbed units) and ParticleClass::Spark_Behavior_AI (glue/particle_native.inc,
  0x0062C6E0). Two 20-minute games (three hard opponents on map 30, one on The Alamo) reached no trap.
* **The menus' size** (user rule 2026-11-17: the original by default, filling as an enhancement):
  Yuri's Revenge runs its menus in a separate display mode, GameOptionsClass::ShellWidth x ShellHeight
  (800x600 from its constructor, read from no INI; gamemd.exe WinMain 0x006BDB7E-0x006BDB90 passes
  Options+0x2C/+0x30 to Set_Video_Mode, and ScenarioClass returns to it after a game, ra2-decomp
  src/scenario.cpp:605), so its menu fills the screen; Red Alert 2 runs its menus at the game
  resolution with the 800x600 menu centred (game.exe 0x005115F0). Both stay so by default. Menu fill's
  new choice Scale to fit shows the 800x600 menu as large as the screen holds in both games (Red Alert
  2's then fills as Yuri's Revenge's does; off in the Original preset; the Enhanced preset keeps Blurred
  backdrop). one-game-menu-backdrop checks both looks at 1024x768. Lane N28 (user report 2026-11-21
  from the Duo: Red Alert 2's menus letterboxed on all four sides under Scale to fit, Yuri's Revenge's
  filling): with Zoom gestures on Red Alert 2's screen is twice as large in its map column (render
  scale 2, link/native_map_zoom.cpp Native_Render_Size) and the host's render-scale layout
  (host.cpp update_layout_scaled) showed the menus at that screen's nominal size, skipping Scale to
  fit; it now fits the 800x600 menu there too. Checked in a 1136x800 window with the phone's settings
  (RA2_NATIVE_WINDOW_SIZE now holds over the game's mode changes): both games' menus the same size.
* More traps off the play paths: HouseClass::Is_Build_Limited (a second unit queued in a factory),
  MindControlBuildings and RelinquishBuildings are the tree's FUN_0050B370, FUN_0050D290 and
  FUN_0050D2D0 (data/name_aliases.tsv, from gamemd.exe's callers); the TibSun-text drafts called
  TibSun's Speak, Sound_Effect, Harvested and Remove_Target, which YR does not have: they call
  VoxClass::Play, VocClass::PlayAt/PlayGlobal, Give_Tiberium and AbstractClass::BecomeUntargetable as
  gamemd.exe does there (selling, repairing bridges, walls and bridges destroyed, barrels); NON-EXACT
  BuildingClass::FUN_00457DE0 (gamemd.exe 0x00457DE0: a garrisoned building lets its occupants out).
  Link closure 236 (lane Y11 start) -> 219.
* Fix on main: lane CB s2's native rekey had renamed the YR glue's DisplayClass::Set_Cursor_Shape to
  RA2's Set_Copy_Cursor_Shape (yr-native did not compile).

## Lane Y12 (2026-11-17): combat, long games, both campaigns to their endings

* **Test harness** (`glue/yr_harness.cpp`, test scaffolding as RA2's link/harness_orders.cpp): RA2_NATIVE_ORDERS
  (`list`, `health`, `shoot <ID|*>`, `watch`, `hunt`) and RA2_NATIVE_FORCE_END (`<f1>,<f2>,...[,lose]`: PlayerWins /
  PlayerLoses at those loop frames), called from yr_native_main_loop.
* **ctest `yr-native-combat`** (yr/tests/yr_combat.py): a Grizzly force-fires on another of the player's (the order a
  force-fire click queues): TechnoClass::Fire_At runs, the target's strength drops and it dies (about frame 1,300).
* **ctests `yr-native-campaign-allied` / `-soviet`** (yr/tests/yr_campaign_chain.py, as RA2's campaign_chain.py):
  Single Player, New Campaign, the side's emblem, seven missions won in turn through their score screens, the final
  movie, the credits and the main menu. Both pass (ALL01UMD..ALL07SMD and SOV01UMD..SOV07TMD: all 14 missions start and
  end, both endings, the credits' 565 lines, menu-return; about 6 minutes a side).
* **The campaign loading screen** (glue/ldprgmgr_native.inc Draw_Campaign, gamemd.exe 0x00552E65-0x005532FD): the
  side's title band, the scenario's LS640/LS800 picture with its palette, the load bar, the mission name and the
  briefing box. The load bar's drawer was cut to 32 bits on its way to Draw_Bar (scenario.cpp declared FUN_0072B380
  as returning int; ProgressScreenClass Field74 and Set_Graphic_Data's argument were int): fixups.json.
* **NON-EXACT drafts** (each labelled in its file), in the order the runs reached them: the spotlight's brightening
  blitters (glue/yr_spotlight.cpp, gamemd.exe 0x007DEEFA-0x007DF640, hand-written assembly the tree declares);
  the railgun's beam damage (TechnoClass::Railgun_Beam_Damage, 0x0070C690, called from Fire_At); the credits
  (Show_Who_Was_Responsible, 0x004C3E30, glue/egos_native.inc: RA2's draft with CREDITSMD.TXT); OwnerDraw::ListBoxHandler
  (0x00618D40, glue/ownrdraw_list_native.inc: the campaign dialog's list; columns and cells not yet);
  BuildingClass::Captured (0x00448260), BuildingClass::Fire_Direction (0x0044D7D0), the rocking from an impact
  (TechnoClass vtable slot 0x3D8, 0x0070B280), MapClass::Reveal (0x00577D90); the ship locomotor (glue/ship_native.inc:
  gamemd.exe's ShipLocomotionClass is a clone of the drive locomotor, Draw_Matrix compared instruction by instruction,
  so the drive drafts' text under the ship's name); the hover locomotor's Process, While_Moving and Start_Of_Move
  (TibSun's text, glue/hover_native.inc, not yet compared with gamemd.exe); off the chains' paths but on the campaign's
  play paths: TActionClass::TAction_IRON_CURTAIN (0x006E36E0) and the voxel debris' VoxelAnimClass constructor
  (0x007493B0, TibSun's text with gamemd.exe's audio and particle changes).
* **Bindings** (data/name_aliases.tsv): CellClass::On_Ground = Is_Tile_Not_Water (0x004867E0), FUN_004C3E30 =
  Show_Who_Was_Responsible, Submit_Radar_Event = RadarEventClass::Create (0x0065FA70), FUN_00622B50 =
  UI::StandardWndProc, FUN_005BF390 = Play_Ingame_Movie, FUN_0041C610 = IndexClass<int, AbstractClass *>::search_compfunc.
  The rocket locomotor's Save, Do_Turn and Mark_All_Occupation_Bits declarations go (fixups.json): gamemd.exe's
  vtables hold LocomotionClass's.
* **Modern controls on touch**: RA2's tap across player-controlled houses (lane C18) carried over
  (glue/yr_enhancements.cpp Tap_Selects_Across_Houses); the rest was wired by lane Y8.
* **Simulator** ("RA2 Duo Validation", RA2YR.app, `--game=yr`, yr/tests/ios_sim_skirmish.py): 12 starts of a skirmish
  (30 s of play each) all reached the time limit without a trap or a fault (the one start crash in 10 seen by lane Y7
  did not recur); a 90 s run with `controls=modern` passed, its two-finger tap giving Modern's order.
* **Long game**: 30 minutes against three hard opponents on map 30 (Mac, `RA2_NATIVE_TRAP_CONTINUE=1`): no trap, no
  fault.
* Link closure 219 (lane Y11b) -> 186. Of the 186: Westwood Online and internet 36, LAN / multiplayer / coop 42,
  the random map generator 11, TibSun leftovers YR does not use (veins, fences, tunnels, ion storms, VQA) 40, shell
  and Win32 16, Speech_Name (the trigger INI writer) 1, play paths 40 (planning mode, the rocket's Process, radiation beams, gas particles, hijackers, spies,
  psychic dominator AI) (lane Y12 trace, by caller).
* **Open**: an LTNK (Allied 2, Soviet 7) gets a garbage position from its drive locomotor; MapClass::Place_Down and
  Pick_Up now log a cell off the cell array and leave it alone (`place-down:` / `pick-up:` lines, five each) instead
  of faulting. The Mac window dumps of a skirmish show a black tactical view with only health bars and the
  `Primary` tag (also on main, lane N1's build): not investigated. The campaign dialog's Difficulty label reads
  `MISSING:''` on its right end. The rocket locomotor's Process (0x006622C0) is still a trap (not reached by the chains).

## Lane N19 (2026-11-19): Game interface size and HD vehicles; the plan for Map zoom below 1x

Game interface size and HD vehicles are Yuri's Revenge's too (the "Not shared, and why" item above;
ctests `yr-native-interface-size`, `yr-native-hd-vehicles`). Map zoom below 1x (Zoom gestures zooming out
past 1x, Red Alert 2's lane C16 render scale) is not carried over; it is a lane of its own. The plan, from
Red Alert 2's pieces (link/native_map_zoom.cpp, port/fixups.json rows naming Native_Render_Size,
Native_Dialog_Screen and Native_View_Asked) and the YR tree's twins:

1. The render scale only for the game's mode. YR runs its menus in their own 800x600 shell mode
   (Options.ShellWidth x ShellHeight) and changes to Options.ScreenWidth x ScreenHeight at Start_Scenario
   0x00683AB0 and Toggle_Display_Mode 0x006842F0; RA2 scaled at WinMain's RA2.INI read and Set_Display_Mode.
   Scale in Change_Display_Mode 0x00560BF0's top only for the game's size ((w - 168) * 2 + 168 by
   (h - 32) * 2 + 32, Zoom gestures on, 800x600 or more), and make the mode checks compare against the scaled
   size (the original compares VisibleRect with Options.ScreenWidth/Height: a scaled screen would change mode
   at every check), e.g. one Yr_Game_Mode_Size(&w, &h) the three checks and the change ask. The host's side
   (ra2_enh_set_render_scale, update_layout_scaled, ra2_enh_sidebar_height's render-scale branch) is shared.
2. The zoom: kZoomMin 0.5 and the game's zoom the player's times the render scale (yr_map_zoom.cpp, as RA2's
   apply_zoom/Native_Map_View), Native_View_Asked's twin at Tactical::Set_Tactical_Position (the start
   centred on the point asked before the larger screen's map edge moved it).
3. The in-play dialogs at the window's size: RA2 wraps Special_Dialog 0x00481850's loop; YR's twin is
   conquer.cpp's `while (Game::SpecialDialog != SDLG_NONE)` loop (around line 223 of the pinned tree): for
   its time VisibleRect/Options' screen size is the window's and the two interface art layouts are made
   again for it (YR: FUN_00730720 = FUN_00730530 + FUN_0072FC60, and the backdrop layout FUN_0072E1B0), then
   restored; ra2_enh_set_dialog_screen tells the host. Unlike RA2, Game interface size needs no part of this
   (YR's art layout keeps the screen's height: see above).
4. The score screen (Do_Win/Do_Lose) is already shown whole (N18's Yr_Map_Zoom_End).
5. Test: yr_map_zoom.py's zoom steps extended below 1x (0.5x: the view twice the tactical area's pixels at
   the art's own size), a mapping check, the in-game menu's dump at the window's layout; then the Duo
   (in-game menu, Abort, Save/Load, Diplomacy at 0.5x).

## Lane N20 (2026-11-20): Map zoom below 1x

Zoom gestures now zoom out to 0.5x in Yuri's Revenge, as in Red Alert 2 (lane C16's render scale), following
lane N19's plan; `glue/yr_map_zoom.cpp` (its second half) and 12 fixups (yr/fixups.json, lane N20; two of
N19's rows changed).

1. **The render scale, for the game's mode only.** With Zoom gestures on and a game size of 800x600 or more,
   the game's own display mode is Options.ScreenWidth x ScreenHeight made twice as large in its map column
   and above the command bar ((w - 168) * 2 + 168 by (h - 32) * 2 + 32: 1432x1168 for 800x600), while the
   menus' 800x600 shell mode keeps its size. Yr_Game_Mode_Size answers that size where the game asks for its
   mode: Start_Scenario's and Toggle_Display_Mode's mode checks (Yr_Game_Mode_Change: the screen against the
   scaled size, so a scaled screen is not changed at every check, then N19's Yr_Sidebar_Remake), the
   tactical rectangle for the game's size FUN_0072AD90 (SidebarClass 0x006A5130's full layout, the command
   bar's width) and the interface art layout FUN_0072FC60's three callers. Options keeps the player's size
   (the Options dialog and RA2MD.INI). Change_Display_Mode 0x00560BF0 tells the host the render scale of
   each mode it sets (Yr_Render_Size: 2 for the scaled game mode, 1 for the shell's and any other) before
   the surfaces are made, and makes the interface art layout (laid out once at Init_Game) again when the
   size it needs differs (Yr_Art_Relayout; FUN_0072FC60 records its size). A game's zoom keeps the player's
   zoom across a change of render scale (a game loaded from the menus takes its zoom in the shell's mode,
   Get_All, and changes mode after). The host's side (the window, the scaled play layout, the sidebar
   surface's rows) is Red Alert 2's, shared.
2. **The zoom.** kZoomMin 0.5; the game's zoom is the player's times the render scale (1 at 0.5x: the art's
   own pixels, the view the whole tactical area). Tactical::Set_Tactical_Position 0x006D6000 records the
   point asked before its clamp (Yr_View_Asked, RA2's Native_View_Asked): the start's zoomed view is
   centred on it.
3. **The in-play dialogs at the window's size.** Ingame_Menu_Dialog 0x0048C8B0 (conquer.cpp) wraps its
   dialog loop (the in-game menu, Game Controls, Abort, Save/Load, Diplomacy) in Yr_Dialog_Screen: for its
   time Drawing::RenderWidth x RenderHeight and VideoModeWidth x Height (the dialogs' size and placement)
   are the window's, the interface art (FUN_00730720) and the dialogs' backdrop (FUN_0072E1B0) are laid out
   for it, and the host shows that top-left part at the window's scale (ra2_enh_set_dialog_screen); restored
   after. The host (shared, platform/sdl/host.cpp) now shows only that part: with a Game interface size
   the window is wider than it, and the last game frame showed beside the menu (Red Alert 2 too).
4. The score screen: Do_Win/Do_Lose change to the shell's mode first, so it is the menus' (render scale 1).

ctest `yr-native-map-zoom` (extended): with Map zoom 2x and Zoom gestures on, the game's mode is 1432x1168
for the 800x600 window, the start's view half the window's tactical area, the steps 2 -> 2.6 -> 0.5 (the
whole 1264x1136 tactical area), mapping checks at each, then the in-game menu laid out at 800x600 (its dump
within 0.7 of 255 of the defaults' menu) and Resume Mission clicked through the window, play back at 0.5x.
Combinations run once by hand (Mac): Interface size 1.5x with HD vehicles 2x and Zoom gestures (sidebar 400
of 1168 rows, a 252-point column in the 884-wide window, mapping checks, the in-game menu at 800x600);
yr_keys_load.py with Zoom gestures and Map zoom 2x (Diplomacy and the in-game Load at the window's layout,
the loaded game at 2x); yr_save_roundtrip.py with Zoom gestures, Map zoom 1.5x, Interface 1.25x and HD 2x
(loads from the menus). Red Alert 2's `native-zoom-menu` and `native-zoom-menu-ui` pass with the host change.

## Lane N21 (2026-11-20): two Duo faults (Abort Mission when scaled, the dark end banner)

1. **Abort Mission with Game interface size / Zoom gestures** showed the map small in the screen's top left corner,
   the rest black. Main_Loop's true return only dropped the play layout (yr_native_main_loop), and only the win and
   loss cleared the zoomed frame (lane N18's Yr_Map_Zoom_End before Do_Win / Do_Lose), so after an abort the larger
   screen's last frame (its zoomed view in a corner) was shown whole through Do_Abort's wait for EVA (seconds with
   sound). Now Main_Loop's true return calls Yr_Map_Zoom_End (as Red Alert 2's Native_Map_Zoom_End), and two fixups
   call it before Do_Abort and Do_Restart (the in-game menu's Restart reaches the end check without Main_Loop's
   return). ctest `yr-native-zoom-abort` (yr/tests/yr_zoom_abort.py): every picture after the in-game menu ends
   (host.cpp RA2_NATIVE_WINDOW_DUMP_LOG numbers each in the log) must not be the screen shown whole; 2 such pictures
   on the old build on the Mac (no EVA wait with -noaudio), none now. The menu's own first frame or two (before it
   draws, the game frame at the window's top left) remain, as in Red Alert 2.
2. **The end-of-game banner** ("Game Over", "Mission Accomplished": GRFXTXT.SHP, Tactical::Draw_Screen_Text) was
   drawn at a quarter of its brightness: Init_Game's draft (glue/init_game_native.inc) left GRFXTXT.PAL's 6-bit
   colours unscaled and gave its drawer 0x35 shades; gamemd.exe (0x0052C233-0x0052C2AB, read instruction by
   instruction) shifts each colour left by 2 and makes the drawer with one shade, as Red Alert 2's matched Init_Game.
   ctest `yr-native-end-banner` (yr/tests/yr_end_banner.py; RA2_NATIVE_FORCE_END gains `flag`: Flag_To_Lose /
   Flag_To_Win as the house AI raises them, banner and EVA, glue/yr_harness.cpp): the banner's middle 120 of 255,
   27 on the old build.


## Lane N23 (2026-11-21): planning mode's mouse, the 89 functions still missing at the Mac link

1. **Planning mode: picking up planned waypoints with the mouse** (PORTS_REMAINING "Campaign stand-ins", left partial
   since lane Y3). FUN_0063AB60 (the mouse over the map, ScrollClass::Scroll_AI) and FUN_0063A5A0 (a left press,
   ScrollClass::Message_Handler) returned false, so every press on a planned node fell through to the map and planned
   another waypoint there: a path could not be closed into a loop by hand. glue/planmgr_native.inc: NON-EXACT
   stand-ins from gamemd.exe's instructions (0x0063AB60, 252 bytes; 0x0063A5A0, 818 bytes) in the shape of Red Alert
   2's twins (Planning_Mouse_Move_00617900, Planning_Left_Down_00617340); gamemd.exe inlines the hovered node's hit
   test and the node search (the tree's Hit_Test and Find_Node_At) and shows each refusal once with the scold sound
   ("InvalidNodeX", "NoAircraft", "NoBuildings", "NoUnknowns"). ctest `yr-native-planning` gained the mouse part
   (harness orders `centermap`, `planloop`; script point `@node=N`, the N-th planned node, glue/yr_harness.cpp and
   glue/yr_app_hooks.cpp): three waypoints; a press on the second (next to last) leaves the path as it was (the
   refusal; the old build's map click there changed nothing either, so this part is a guard, not a discriminator);
   the mouse on the first makes it the hovered node, the press picks it up, the release closes the path into a loop
   (loop start 0 over 3 nodes); the commit; the tanks set out. On the old build the node is hovered but never
   picked up and no loop closes (the test fails there).
2. **The 89 functions still missing at the Mac link** (gen-yr/link_closure.json; each symbol's referencing object from
   `nm -u` over closure_inputs.txt, each caller's guard read in the unit and in gamemd.exe: call and pointer
   references over .text and data, the writes to its guard globals). None is reached in skirmish, LAN or either
   campaign with the shipped data. By family, with the evidence:
   - **Westwood Online** (42): webbrowser.cpp FUN_00778A20 and FUN_00789500 (WebBrowser::Show_Page); laser.cpp's
     ViewHTML and its GetWindowsDirectoryA / GetTempFileNameA (only caller gamemd.exe 0x00773834,
     WebBrowser::Show_Page); netshare.cpp FUN_0077DB90, FUN_0077E430, FUN_0077E900, FUN_0077EBA0, FUN_00785360,
     FUN_0078F000, FUN_007A2750, FUN_007A27D0, FUN_007A28B0 (each under `g_pChat != NULL`: the WOL chat object, which
     no unit ever sets), FUN_007A2730, FUN_007A2800, FUN_007A0E50, FUN_0077F900 (each under `Session.Type ==
     GAME_INTERNET`; FUN_0077F900 also in campscor.cpp's coop score); init.cpp FUN_0077B2A0 (Select_Game's
     GAME_INTERNET case); sidebar.cpp FUN_0077DA50 and mainloop.cpp FUN_0078A470 / FUN_0078A6B0 / FUN_0078A8F0 (chat
     page commands and the Diplomacy screen's ladder stats, `Session.Type == GAME_INTERNET`); sendfile.cpp
     FUN_007931A0 (the non-IPX, non-modem branch: Internet); queue.cpp / mainloop.cpp Send_Statistics_Packet (both
     forms) and GameStatisticsPacketSent (`Session.Type == GAME_INTERNET`); conquer.cpp DoFindPage (Game::SpecialDialog 7,
     which nothing sets: the special dialogs set are SURRENDER, ABORT, SETTINGS, OPTIONS, WOL_OPTIONS, KEYBOARD, SOUND);
     pk.cpp Generate_Prime and Int<64>::Error / Borrow (PKey::Generate, no caller in gamemd.exe); the World Domination
     Tour (worlddom.cpp FUN_0076ED10 / FUN_00797800 / FUN_0079FC40, wdtsel.cpp's four WorldDominationTour::Selection
     functions); msanim.cpp's five MSTextLayout functions (MSButtonAnim's text, made only by _Graphic_Menu <-
     Do_Graphic_Menu, whose one caller is WDT_Theater_Choice_Menu 0x007AFB15).
   - **Coop campaign** (3): coopcamp.cpp FUN_0049B430, campscor.cpp FUN_0046DFD0 (CampaignScoreClass::Show's coop
     dialog), mpcoop.cpp MPCoopReporter::MPCoopReporter (lane N10).
   - **Modem** (1): modemgst.cpp MapPreviewClass::Blit_Preview (lane N8).
   - **VQA movies** (17, with the interpolation helpers): vqa.cpp / vqalib__*.cpp's ASM_UnVQ*, UnVQ1/UnVQ2_4x2_Table, VQA_LCW_Uncompress,
     VQA_sosCODEC*, AudioUnzap, VQA_Reset and ahandle.cpp's Convert_HMI_To_Direct_Sound_Volume (Start_Audio_Handler,
     the VQA player's audio). Play_Movie plays a name that resolves to .BIK as Bink and returns; the VQA path runs only
     for a .VQA file, which Yuri's Revenge does not ship. interpal.cpp's Asm_Interpolate and its two line helpers
     (Interpolate_2X_Scale, no caller in gamemd.exe).
   - **Tiberian Sun only** (8): CellClass::Place_Veins (OverlayClass::Mark for OVERLAY_VEINS and
     Post_Read_Vein_Fixups for solid vein cells: no shipped map has veins), VeinholeMonsterClass::Class_Of (made only
     for OVERLAY_VEINHOLE), InfantryClass / UnitClass::Tunnel_AI (CurrentTube >= 0: TibSun's tunnels, no [Tubes] in the
     shipped maps), Tactical::Draw_Fence_Placement / Draw_Firestorm_Wall_Placement (IsLaserFencePost /
     IsFirestormWall buildings, none in rulesmd.ini), score.cpp ScoreFontClass::Print_String (ScoreClass::Count_Up_Print
     0x0068FB10, no caller: YR uses CampaignScoreClass), isotype.cpp Iso_Blit_Asm2 (under IsoTileUseAsmDrawFunc,
     which nothing sets).
   - **Editor, debug and recording** (9): taction.cpp Speech_Name (TActionClass::PrepareSaveToINI, only from
     TriggerTypeClass::Write_INI: scenario writing); lcwpipe.cpp / lcwstraw.cpp LCW_Comp (compressing pipes: MapClass's
     Write_Binary_1/2, with no caller, and OverlayClass::Write_INI from Write_Scenario_INI, which the random map dialog
     calls only under Debug_Map); mainloop.cpp TeamEvent / TeamNumber (written under Session.Record: gamemd.exe only
     clears that bit, 0x0052E968 `and al, 0xfe`, never sets it) and Debug_Trap_Check_Heap (under [MultiPlayer]
     CheckHeap, a debugging key; data, a zero placeholder at the link); motlib.cpp AudioGetDevice(int, int) (the sound
     test window's key handler FUN_005BC6E0, whose window FUN_005BC530 makes only when DAT_00ABF239 is set, by
     FUN_005BC370, which has no caller, or from queue.cpp under DAT_00B04874, which gamemd.exe only reads);
     startup.cpp OutOfLineDtor::~OutOfLineDtor (Prog_End deletes DAT_00A83DB8, which gamemd.exe never writes);
     facing.cpp getch / __conio_common_vcprintf (Exception_Dialog's recursive-exception console, after a crash).
   - **COM template members** (5): CLSIDFromProgID, GetActiveObject, VariantInit / VariantClear / VariantChangeType
     come only from `_com_ptr_t<...>::GetActiveObject` and `QueryStdInterfaces(_variant_t
     const &)` in the units' explicit instantiations (`template class _com_ptr_t<...>`, building.cpp, drive.cpp, fly.cpp,
     foot.cpp, saveload.cpp, savever.cpp, startup.cpp, super.cpp); no unit calls those members (a
     `_variant_t` is never made).
   - **Default virtual slots and dialogs** (4): TechnoClass::ForceCreate (the base slot; every techno is a FootClass or
     BuildingClass, which override it), OwnerDraw::SysTabHandler and PrepareDialogChildControls (lane N10),
     init.cpp Get_Language_Version (Version_Dialog_Proc, the version dialog).
   Nothing reachable was found, so no stand-in was written for them; the count stays 89.

## Lane N27 (2026-11-21): the command bar's thumbs with Zoom gestures on

The user's Duo report: "the bottom bar blanked out. I think I tapped somewhere on the very left side." The
original's command bar has a close thumb at the left end of its buttons (ButtonA, 0xF0, "Tip:ThumbOpen") that hides
the buttons, leaving the bare bar, and an open thumb at its right end beside the sidebar (ButtonB, 0xF1,
"Tip:ThumbClosed") that brings them back (TabClass::AI, gamemd.exe 0x006D0680; FUN_0072FC60 places both). Closing
the bar is the original's; the port's fault was the way back: with Zoom gestures on (the render scale, lane N20) the
interface art is laid out for the twice-as-large screen, so the bar's right end, its end piece and the open thumb lay
at the larger screen's sidebar x (1208 for 800x600) while the host shows only the bar's left part, the window's map
column (0-632). A native fixup in FUN_0072FC60 lays the bar out from the shown part's right end
(glue/yr_map_zoom.cpp `Yr_Command_Bar_Right`; the original's x at render scale 1). Harness: input-script point
`@thumb` (the thumb shown now) and order `thumb` (open or closed, both thumbs' places, which is a button). ctest
`yr-native-command-bar-thumb` (yr/tests/yr_command_bar_thumb.py): closes and opens the bar with Zoom gestures on and
interface 1.25x (the open thumb must lie in the shown part: 576 now, 1208 before) and with the defaults (576, the
original's).

## Lane N32 (2026-11-23): touch pan and the zoom gestures wait for a mission intro's input lock

Red Alert 2's lane N31 (the user's Duo report from Hail to the Chief: the map could be panned during the locked
intro) carried over. A mission intro locks the player's input with the LOCK_INPUT trigger action (gamemd.exe
0x006E2660, which ra2-decomp calls TAction_CLEAR_MODES) -> Lock_Scenario_Input 0x00684240, which sets the flag at
0x00A8ED9C until the unlock; ra2-decomp names it both IgnoreInput (globals.h; ScrollClass::Scroll_AI 0x00692F30 and
Message_Handler refuse the edge and drag scrolling while it is set) and UserInputLocked (scenario.cpp, taction.cpp,
keyboard.cpp), and globals.tsv gives both names the one address, so they are one variable natively too. The touch
controls' and the trackpad's pan (`ra2_native_scroll_map`, glue/yr_app_hooks.cpp) take the pan and drop it, and the
zoom gestures (`ra2_native_zoom_by`, glue/yr_map_zoom.cpp) do nothing, while it is set; each refusal is logged once
a gesture. Harness order `view` (`harness-view <x>,<y> locked <0|1> zoom <z> frame <f>`). ctest
`yr-native-locked-input` (yr/tests/yr_locked_input.py): Allied mission 1 (ALL01UMD) from the menus, Zoom gestures
and the touch controls on; a swipe and a pinch step while the intro's camera rests (frames 500-520, input locked)
leave the view and the zoom as they were, the same at frame 1600 (after the unlock at about 1490) move and zoom it.

## Lane P15 (2026-11-24): the Skirmish Game screen's column and the Start column's "???"

The user's Duo report (the combined app; the earlier port had it too): the map labels on the right sat too low, the
map name across a panel border, and the Start column's "???" wrapped onto two lines. Not the resolution or the
interface size: Yuri's Revenge shows its shell in its own 800x600 mode and the same picture was drawn at 800x600.
The labels are where the template puts them (0x6EC at 432,103 DLU, 0x5A8 at 432,116); the column's art behind them
was wrong. gamemd.exe sets record flags at 32-bit offsets 0xD5 (the preview's frame, FUN_0060CAF0) and 0xD6 (the
label box under it, FUN_0060C930) for the skirmish, LAN host and guest dialogs (0xD5 also Choose Map, 0x105, 0x113;
0xD7 for 0x103/0xBC7), and OwnerDraw::Paint draws those pieces (FUN_0072E8C0, FUN_0072E860) and the slide starts the
button rows one slot lower. The flag accessors spell the offsets `(unsigned char *)data + 0xd5` / `+ 213`, which
lane Y3's raw-offset fixup (`(char *)data + 0x`) did not cover, so natively they wrote 0x20 bytes before the Dialog
view's fields that Paint and the slide read; the plain column (the main menu's monitor ring, an empty first slot)
was drawn. A fixup takes them through the Dialog view too. The combo's field text (draft in
glue/ownrdraw_combo_native.inc) was drawn in a box ending at the arrow (15 pixels in the 37-pixel Start field);
gamemd.exe (0x00617B42-0x00617C04) cuts the text while it is wider than the client width less 20 and draws it to
the field's right edge. ctest `yr-native-skirmish-layout` (yr/tests/yr_skirmish_layout.py): 800x600, the Duo
(1024x720, the phone's settings, 1136x800 window) and 1024x768 in a 1440x900 window: the label box dark behind
the labels, the labels inside it, "???" on one line.

Still different from the original on this screen (seen against a picture of the original's skirmish setup, not
fixed here): the fields' frames are red where the original's are grey and drawn inside the control's rectangle
where the original's sit outside it; the closed dropdowns are 20 rows high where the original's rows touch; the
port's dialog units truncate (x * 6 / 4) where Windows rounds each of x, y, cx, cy (MulDiv), so the Start field
is 37 pixels wide, not 38, and "???" fills its room exactly.

## Lane P18 (2026-11-24): three faults from the user's Duo run of Allied 1

Reproduced on the Mac from New Campaign (Allied), with logs; all three are in the old port's text too.

* **The IFV kept its missile turret when the opening's engineer boarded it.** The map's Load_Engineer trigger
  makes TEAM_Start_Engineer (an ENGINEER and an FV, script "load onto transport"); the engineer's IFVMode is 1, the
  repair turret. InfantryClass::Per_Cell_Process is a TibSun-text draft (lane Y4) whose boarding put the soldier into
  the transport's Cargo directly. gamemd.exe 0x0051A3FC-0x0051A47E boards through the transport's AddPassenger
  (vtable +0x394; FootClass::AddPassenger -> UnitClass::ReceiveGunner -> IFVMode) after dropping the archive target,
  the weapon stage and any mind control, the open-topped call and `Transporter`, as UnitClass::Per_Cell_Process's
  matched text does: now the same steps (glue/infantry_native.inc). Harness: `weapon <ID>` logs a techno's weapon,
  turret and passengers, `enter <ID> <TARGET> own` boards the player's own transport. In Allied 1 the FV goes from
  w0 t0 to w1 t2 with its engineer; ctest yr-native-standins' new gunner case (an engineer into the player's IFV).
* **The radar's movie played under "Incoming transmission", and was over by the time the radar opened.** Two
  faults. Call_Back 0x0048D080 polls IngameVQ[0] through its vtable's third slot (gamemd.exe `call [edx+8]`:
  MovieHandle::Waiting, which only asks BinkWait); the tree calls a placeholder `V2`, and with a virtual destructor
  in two slots (Itanium ABI: Mac, iPhone, MinGW) that slot is AdvanceFrame, so every frame decoded the movie from
  the moment the trigger queued it (fixups.json conquer.cpp rows: the call through MovieHandle). Then port/src/bink.cpp
  timed frames from BinkOpen, so once the radar asked for frames, the decode loop ran through every overdue frame in
  one call; the clock now starts at the first decoded frame (Red Alert 2's radar movies share this code). Log of
  the fixed run: the trigger at frame 390, EVA_IncomingTransmission 391-462, the radar opens, the movie's 45 frames
  505-595 at 15 per second, then the mission's A01_p01 line at 615 (before: frames 1-55 decoded from frame 390 with
  the transmission, and none left at 505).
* **No Psychic Dominator countdown.** One is expected: the map's Confederation owns the Dominator (YAPPET) and its
  Set_PsyDom_Time trigger sets it to 90% one second in (SetSWChargeLevel, superweapon 7 = PsychicDominatorSpecial,
  ShowTimer=yes), so 01:00 shows (the Mac draws it). With Pinch zoom on (the iPhone's default, render scale 2)
  Tactical::DrawTimer put the countdowns at the view's corner, and while zoomed the view is the part shown rounded up
  to whole cells, which the host does not show (window dump: "Psychic Dominator" with its time cut off at the right).
  Yr_Timer_Corner (glue/yr_map_zoom.cpp) answers the shown part's corner while zoomed. Red Alert 2's zoom draws its
  timers the same way (not checked here).

## Lane P44 (2026-11-26): Yuri's Revenge under AddressSanitizer

Lane P35's sweep (docs/ra2-port-notes.md) on this port. Build: RA2_NATIVE_ASAN=ON with RA2_NATIVE_YR=ON in its own
folder; every yr- and one- test there runs with ASAN_OPTIONS halt_on_error=0:log_path=<build>/asan-yr-logs/<test>/asan
(yr/yr.cmake), so a run writes every report and goes on (before, each run stopped at its first: lane P35's seven
failures). Two full runs of the 35 yr-native tests and the mission soak of both campaigns found the sites listed in
docs/regressions.md (lane P44 section); after them no test writes a report.

Most were the original's own reads past an object that gamemd.exe's layout absorbs, answered with the original's bytes
(read from gamemd.exe: the pip table's neighbours, the theater table's predecessor, the combo strings' padding, the bytes
before MissionControl). Two were the data generator's, not the original's: a name sharing its address with a
structure (Special, UnitCompositeDirtyRect) or declared a pointer in another unit got storage too small for what
the native code writes, and yr_data.py now sizes them (the shared-address names of a structure become offsets into it).

The layout check: tests/mission_soak.py --reference <binary> plays each mission with both binaries and compares the
digests (every techno's type, place, strength and mission every 250 frames, the houses' money, the scenario generator).
Release and ASan parted at frame 2 of Soviet 1. The game's generator calls were the same in both (an lldb trace of every
Random2Class call); a trace of fastmath::sin/cos arguments put the first difference in WalkLocomotionClass::Movement_AI,
an angle of -1.7689 against 4.5145: DirType::From_Radian's double-to-short conversion out of range, undefined, and the
two builds' code kept different values. Fixed as Red Alert 2 fixed it in lane P9 (gamemd.exe's multiply and _ftol);
after it all 14 missions match. The digest's generator hash had included Random2Class's padding, which differed between
builds by itself; it no longer does. ctests: yr-native-asan-soak-allied / -soviet (label yr-asan).

## Next

0. The Windows builds on a real PC (RA2YRNative.exe with its two DLLs and SDL3.dll is on the user's PC, lane P1;
   YRNative.exe alone is not); the stand-ins not yet seen at work (bridge repair; the War Factory's unload through
   the harness; the hijacker is in yr-native-standins since lane P1); the -MPDEBUG sync
   record's object lists.

1. The buttons' targets (input not yet exercised; the runs are unattended): each submenu
   will stop at its unmatched handlers (ListBoxHandler 14.7 KB, ComboBoxHandler 6.9 KB,
   ComboDropWindowHandler 7.6 KB, SysTabHandler, NewEditHandler, PrepareDialogChildControls,
   UI::RegisterWindow; RA2 counterparts in native/link/behaviour_m14_ownerdraw.cpp).
2. The owner-draw unit's other 32-bit pointer assumptions: globals declared `int` that hold
   pointers (FUN_00603870's shapes, DAT_00833674), `(char *)p + 0x74`-style raw offsets on other
   variables, union views with pointer members read at the same offsets.
3. Init_Game's CD search when no search path is set (not drafted: the host always passes -CD.).
4. Variant* and the other Win32 imports the port does not yet supply (the run lists them).
5. (lane Y10) Yuri's Revenge's saves: the installed gamemd.exe on Windows loading a save the port wrote,
   and the port loading one the installed game wrote (yr-native-save picks it up from the data folder).
6. (lane Y8) Map zoom, Zoom gestures, Game interface size and HD vehicles for Yuri's Revenge: a YR
   copy of each native/link module and its fixups in the YR units (lane Y8 section: why). Map zoom and
   the gestures from 1x up (lane N18), Game interface size and HD vehicles (lane N19) and Map zoom below 1x
   (lane N20) are done: every Enhancements row Red Alert 2 has is Yuri's Revenge's too.

## Lane P50 (2026-11-27): the step server for Yuri's Revenge; campaign missions played to wins

Red Alert 2's step server (ra2/glue/stepserver_native.inc, lanes P21-P47) carried over to yr-native as
`yr/glue/yr_stepserver.cpp` against the YR tree's classes (vendor/ra2-decomp's names: TacticalMap, SidebarClass::Column,
SidebarClass::TabButtons, Repair, Power, SuperClass::IsPresent, IsReady, FactoryClass::Completion, ...), called once a
loop frame from yr_native_main_loop (glue/yr_app_hooks.cpp) after the harness's orders. The same environment and command
set: RA2_NATIVE_STEP, _MISSION, _RECORD, _PRELOAD, _REPLAY, _SLOW, the frame clock (RA2_NATIVE_FRAME_CLOCK, "# frame
clock" in a recording), state (houses' power, drain and credits; every house's super weapons), look, input (Escape
refused), order (glue/yr_harness.cpp's RA2_NATIVE_ORDERS orders, yr_harness_run_order), save, cell, terrain, buildable,
mapfile, screenshot, release, quit. The input script's "@^ID", "@=X,Y", "@ID[:n]", "@!ID", "@tab=N", "@cameo=ID" points
are answered by yr_step_resolve_point (asked by yr_app_hooks.cpp's resolver after its own), plus "@repair" and "@sell".
The game's end is logged as Red Alert 2 logs it ("game-over: player wins at frame <n>"). SuperClass::Is_Charging is not
ported in the YR tree (a trap when called): the state line reads `!IsReady && !IsSuspended` instead.

Clients: `ra2/tools/ra2_step.py start --game yr` (the 800x600 menus, RA2MD.INI), ra2/tools/ra2_commander.py (reads the
800x600 screen from the view line: Game.yr, the view's middle, the band box's bounds, "@sell"/"@repair"; Yuri's defence
types), `ra2/tests/ra2_campaign.py --game yr` (gamemd.exe; `--soak <n>` puts the mission soak's digests in the log, to
find where two runs part). Replays in yr/tests/campaign/, ctests `yr-native-campaign-won-<mission>` (yr/yr.cmake yr_won
list, label yr-campaign-won).

Determinism (found here): YR's FUN_007B84D0 (the per-frame mouse call Red Alert 2's step server makes) does nothing while
the mouse thread runs, so the pointer the game read at a click's press was wherever the mouse thread had last left it:
a press read the parked pointer and the click became a band box from there (two tanks selected in two of six runs of one
replay). The step server now calls WWMouseClass::Process_Mouse itself once a frame; six parallel runs of the replay then
gave the same digest at every 50th frame.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 1 (ALL01UMD, Time Shift) | won at tick 33,939 (lane P52's re-recording after lane P44's facing fix; P50's won at 24,522) | 1.5 (with the step server's port) + 0.3 (P52) | win: Yuri's new Psychic Dominator (YAPPPT, 47,70 on Alcatraz) gone (PsychicDead / Check_New_Dom). The tanks clear the initiates at the power plants (131,130); the IFV's engineer takes CAPOWR01, four built engineers CAPOWR02-04 after the tanks (six more MTNK bought) cleared the gattling tower at 125,148; full power (800/736) is Power_Online: the time shift. The Apocalypses and the Kirov die to the Grand Cannons and the Guardian GIs (Apocs_Gone), Tanya swims to Alcatraz (74,96 - 68,86 - 63,76 - 62,68) and dies to the hydrofoils on landing; her death sends six carriers (Tanya_Death), which, ordered at the Dominator, sink it. Saves in private/campaign_saves/yr/all01. P52's re-recording (P50's scripts) parts at the IFV: its engineer never reached CAPOWR01, so CAPOWR01-02 were taken later by two built engineers (tick 25,000) and the Kirov died near 122,78 before Tanya came; saves in all01/p52 |
| Allied 2 (ALL02UMD) | shelved (lane P52) | 1.3 + 1.0 + 0.8 (P52, try 4) | win (Win_Check): every building of YuriCountry (h9: the main base, 55-134,55-182, 31 gattling towers, 9 tank bunkers, a psychic tower) and of Yuri2 (h14: the west base 19-48,109-147 and power plants at 105,134, 165,79, 197,101) destroyed, and the four grinders (Yuri3 YAGRND 105,128, 129,79, 152,70, 194,103). Two tries. The base (MCV deployed at 167,128) is taken building by building by Yuri4's engineers (YENGINEER, from Yuri4's barracks YABRCK at 158,78, sent while it stands: YuriBarrack_Gone) whenever the army is away; try 1 took the east grinder and Yuri4's base with 24 MTNK/SREF but lost every building. Try 2's early strike (9 MTNK, tick 11,600) on Yuri4's barracks died at 160,95-112. GATECH needs AMRADR first (then SREF, BFRT). Next: radar, tech centre, 12+ SREF and a home guard (dogs, GGI, pillboxes north at 158-165,116-121) before any push; Yuri4's barracks first. Recordings and saves in private/campaign_saves/yr/all02 (a2-try1.replay: A2ARMY.SAV at tick 26,815, 24 tanks) **Lane P52's try 3** (a2-p52.replay, saves in all02/p52): the plan held: base (2 GAPILL, 2 ATESLA, a GTGCAN north), radar and tech centre by 14,000, 3 war factories, 13 SREF + 6 MTNK; Yuri4's barracks and power plant killed at 26,940 (15 tanks left), then the grinders 152,70 (28,608), 194,103 and the plant 197,101 (32,466), 129,79 (38,752), 105,128 and the plant 105,134 (46,718: every grinder gone, 29 tanks, A2G5.SAV); Yuri4 never took a building. Yuri2's west base (incl. its captured CAMIAM04s and CAOILD) fell by 61,958 at a cost of 23 tanks, and the economy was dry (credits 0: the ore near 144,116 is the only field the four miners work); a 13-tank push on YuriCountry's main base (48 buildings: 25 YAGGUN, 8 NATBNK; 29 INIT, 16 LTNK, 7 YTNK, 7 YURI) died for one refinery (83,269, A2END.SAV). Shelved: resume from a2-p52.replay --until 46718 (all grinders gone), and fix the economy first (ore purifier, the Special oil derricks at 106,143 and 93,177 by engineer, a refinery by a fresh field) before the west base; a preload to 46,718 takes about 25 minutes. **Try 4 (P52, after the owner's video dossier, 0.8 h of its fresh 1.5 h):** a fresh opening with the dossier's build order lost the base by tick 10,800 (the Nod initiates and Clint's fight at 157-160,118-126 went another way); try 3's scripts rerun from a fresh start (scripts/p52/a2redo.sh, after advancing to the opening at 1,800) reproduce it exactly (the same saves at the same ticks, about 35 minutes of wall time to 46,718). From there the 29 tanks went for YuriCountry's power plants from the south (100,118 - 88,104 - 78,94 - 74,90 - 68,85; YuriCountry power 3,500 against 1,535 drain, nine YAPOWR at 62-86,62-90): two plants and eight buildings fell, 46 units were lost (mind control and the 29 initiates, 16 lasher tanks), credits 70 at 60,542 (a2-p52-try4.replay). Shelved: the main base needs about twice the army try 3's economy can buy; the next try should add the dossier's Chronosphere and Spy Satellite and a second ore field before tick 30,000, not after. |
| Allied 3 (ALL03UMD, Power Play) | won at tick 12,952 (lane P52) | 0.3 | win: YuriCountry's nuclear silo (NAMISL 49,90) destroyed (Nuke_Gone). The Lightning Storm is ready at 1,700 but only takes the silo to 570/1,000 (repaired); the nuke (every ~4,500 ticks) takes the war factory and power plants at 2,700, so the storm stops charging and nothing can be rebuilt. Three snipers bought, the start's tanks and the second tank group (113,130) by party_legs west along y 96 (110,96 - 100,96 - 90,96 - 78,94 - 66,92; the MSGuard group and the Confederation side groups fall on the way; the GGIs die), then the 21 tanks ordered at the silo from 66,92: it falls with four tanks left. Saves in private/campaign_saves/yr/all03 |
| Allied 4 (ALL04DMD, Tomb Raided) | shelved (lane P52) | 0.4 + 0.1 (try 5) | win (from the map's tags; the trigger dump alone misleads): the eight Arabs power plants (YAPOWR, 74-75 and 92-93 x 90-103, inside the walled Psychic Dominator compound 71-97,87-107) share the semi-persistent tag "Einstein Rescued (O)" (persist 1: it springs only when all eight are gone). Rescue with the Dominator (YAPPET 84,94) already destroyed (local 1) runs EinsRescue2: local 3 and "Destroy Yuri's base", won by It's OVER! (destroyed-all YuriCountry, 89 objects at 99-158,27-79); rescue with Yuri's buildings gone first (local 10) runs EinsRescue3, Einstein Timer 3 and We Won!. Either way Yuri's base must fall. Found: two Harriers (GAAIRC) on the oil tank CAMISC02 (84,108, 5 HP) open the compound's south wall (Oil Explodes!); Tanya resists the psychic towers (Mis_xA4_TanyaResistHint) and, sent through the breach with short attack orders on the initiates, C4s the Dominator and the four east plants by tick 16,900 (she heals between kills; she dies to the west gattlings at 71,107/70,97); once every plant is gone the compound has no power, and four rocketeers (JUMPJET) finish the west plants untouched. Killing the Dominator sends every guard to hunt (Kill SB Front Guards): the base at 83,160 fell each time. Lightning Storm and Harriers do not keep the Dominator down (it is repaired, 40 per 60 ticks). Economy weak (ore only at 40-50,90 and 20,110; the Russians' oil field 162-179,84-101 is the intended income, Oil Field Free). **Port finding:** robot tanks (ROBO, GAROBO built and powered) stay deactivated inside the war factory (m2, cannot move) and block every later vehicle; HouseClass::PoweredUnitCenters / BuildingClass::FUN_004549B0 is the place to look. Next: oil field first, base defences before the Dominator, Yuri's power plants (129-153,33-53) by Harriers/rocketeers so his defences go dark, then the buildings, then Tanya and rocketeers on the compound (EinsRescue3). Replays and saves in private/campaign_saves/yr/all04 (try4.replay: Dominator and all eight plants down, local 3, tick 25,110, no base) **Try 5** (after lane P53's factory-exit fix; try5-robots.replay): 14 robot tanks by 22,000 leave the factory and clear the Russians' oil field (Oil Field Free, 26,920); one derrick taken by engineer, then raids took the base's power and refinery and the robots died at the oil field (175-178,86) by 47,600. |
| Allied 5 (ALL05UMD, Clones Down Under) | won at tick 112,634 (lane P60, resuming P52's recording at 90,439; the crash fixed by P55, the stuck orders were the step server's planning-mode click at @=163,140, lane P56) | 0.7 + 0.8 (P60) | win (Player Wins): locals 12 and 13, set by destroyed-all of BadGuy1 (h1, 104 objects at 73-161,93-157, its captured civilian buildings included) and BadGuy2 (h2, 37 at 114-152,74-129); lose: every Player building gone. Base on the west shore (51,90) with 12,050 credits and a navy; ore at 60-70,79-88. 32 SREF by tick 31,600 (four war factories), BadGuy2 wiped by 52,170 (110,100 - 128,100 - 120,82), BadGuy1's two north power plants by 90,439 (A5W4.SAV). **yr-native crashed** at about tick 95,600 (signal 11: UnitClass::Take_Damage -> As_Building(BunkerLinkedItem)->ClearBunker() with a non-building link, unit.cpp:887; replay all05/crash/crash.replay, lane P55 fixing). Resumed from a preload to 90,439 (about 7 minutes) by another route: the east plants fell (97,653), then every prism tank stopped answering orders (mission 5, no movement, move and force-fire orders shown but not carried out): 16 inside the cloning-vat block at 160-165,130-136 and 20 new ones at the home war factories (55,80-86), while harvesters still moved; BadGuy1 left at 40 buildings. Resume after the bunker fix from all05/crash/crash.replay --until 90439 (or resume-from-90439.replay). **Lane P60:** from the preload to 90,439 (crash.replay --until 90439) the 22 prism tanks went 160,121 - 157,127 - 152,128 - 150,133 - 145,130 and took the east plants, the cloning vats, the barracks, war factory, construction yard and every gattling by 106,569 (7 tanks left). Destroying the garrisoned civilian buildings (BadGuy1 holds ten: CANEWY*, CASTL01, CAMIAM02, CALOND03, CAWASH05/06) lets out their initiates, and with the paradrops from the captured airport (CAAIRP 141,157) about 56 initiates hunted: in the first try they killed the last tanks and then the whole home base (try1-baselost.replay). The win: back to 106,569, cash from the finished spy satellite cancelled and three power plants, the service depot and three war factories sold (6,000), four Harriers (ORCA) and rocketeers (JUMPJET; initiates cannot shoot at air), the airport killed by the front prism tanks with the Harriers (111,689; it is repaired at about 56 per 100 ticks, two Harriers alone do not keep up), then BadGuy1's last plant and vat; "Player Wins" sprang at 112,634 with ten garrisoned civilian buildings still BadGuy1's. Replay yr/tests/campaign/all05.replay (ctest yr-native-campaign-won-all05, about ten minutes); saves and tries in private/campaign_saves/yr/all05/p60 |
| Allied 6 (ALL06UMD, Trick or Treaty) | shelved (lane P60, at the cap) | 1.5 + 0.9 (P60) | win: every Yuri factory gone (Detect_Yuri Facts Dead: no-factories-left; YABRCK 75,33, YAWEAP 82,35, YACNST 80,31, YAYARD 49,32 in the walled base 49-92,22-53), then All to Hunt_Yuri and Conditions_Win (destroyed-unit-type 9); lose: the Parliament (CALOND04 94,116, Neutral, tag Parliament: a quarter of its strength) or any of the leaders' limousines and helicopters (Neutral teams tagged Detect_Leader Dead) destroyed. Yuri has no power at the start (his plants are the Africans' house) and no units. The medium timer (1,200 s, about 19,800 ticks) ends with Yuri's waves (initiates, lashers, Brutes, Masterminds, virus snipers, floating discs, hovercraft) on the Parliament from the river: it fell at 20,700-28,400 in eight tries. Found: the shipyard places only after power plants reach 90,111 (it lands at 90,105); 12 destroyers, 4 Aegis and 11 Grizzlies held to 28,400. Orders into the walled base were not carried out (tanks stood at 84,62 and 97,50 under attack and force-fire orders): probably the step server's planning-mode click (lane P56). Replays in private/campaign_saves/yr/all06. **Lane P60** (five tries, the owner's video dossier from the third): the factory win is not the first objective. Detect_Yuri Facts Dead is enabled only by Setpiece2_End, which follows "Yuri Wave Attack 5 Dead" (destroyed-units-all of YuriCountry), the treaty movie and the Soviets' arrival, so killing the factories early does nothing; the mission is to hold the Parliament through five waves. They come overland from Yuri's base (reinforcements at 90,49, 61,74, 71,69, 80,60, 96,43) over the three bridges (x 65, 84, 111; decks y 97-100 and 107) from about tick 19,100 (the Medium timer: the harness plays the campaign's default difficulty, the "diff m" triggers fire). Wave 5, at about 27,000, puts about 80 units on the map (19 initiates, 18 lashers, 13-15 Gatling tanks, 10-11 masterminds, virus snipers, brutes, discs). There is ore west of the Parliament (127 TIB01 cells at 68-78,118-124), worked out by about 19,000. Tries: a tank rush on the base (lost at 21,754); a tank and tower hold (waves 1-4 held, discs drained the power, lost 27,108); the same with priority fire on discs and masterminds (Parliament 4,814 until 25,800, lost 27,683); the dossier's build (two refineries, five miners, 3 Prism Towers, 4 Patriots, Battle Fortresses, engineers repairing the Parliament: lost 27,891, the masterminds took the vehicles); towers only with 20 GGIs (lost 27,483; wave 4's brutes and masterminds had the Parliament at 3,100 before wave 5). Sending GIs into CAPARS09 and CALOND05 did not garrison them. Next: walls round the Parliament, the dossier's garrisons found by type (the blue-roof building north-west of Big Ben), every credit spent before 19,000 (try 4 had 18,500 unspent at the first wave), and something that kills masterminds at range. Replays in private/campaign_saves/yr/all06/p60 (try4-dossier.replay is the best base: economy and defences by 18,104) |
| Soviet 1 (SOV01UMD, Time Shift) | blocked (lane P52) | 0.3 | win: SP3.6c_Psy Dom 2 Dead after the time machine chain (objectives: Allied navy and Grand Cannons; the pillboxes round the time machine CATIME01 144,97; four tech power plants; the Soviet base on the Psychic Dominator). The start fleet (dreadnought, 3 hydrofoils, 2 squids, 3 subs) sinks the 3 destroyers, 2 carriers and both Grand Cannons by 7,558 (Complete_Objective 1). **Soft lock:** input stays locked from the landing on (checked to tick 148,551): SP2.7_Transport 4's team TM_Sov_1 Trans_1 Boris_5 Tesla_3 Dog_3 Eng (script Move to X_BG: wp23, wp58 80,82, unload 8,1, 20,15, local 9, wp11, exit) never puts Boris's team on the map, so local 9 (SP2.14) and local 23 (Boris Intro script, 39,23) never set and SP2.13_Unlock never runs; the other three transports unload (4 HTNK, 12 E2, 4 HTK). Twice from a fresh start. Replays and the map in private/campaign_saves/yr/sov01 |
| Soviet 2 (SOV02UMD, Deja Vu) | won at tick 8,320 (lane P60) | 0.4 + 0.1 (P60) | win: Einstein's lab (CALAB 100,175, tagged) destroyed (local 7) and the Chronosphere (GACSPH 100,178) gone (local 8); lose: everything gone. The three starting groups each take the German outpost beside them (German1 76-83,52-63; German2 107-113,36-45; German3 141-150,68-76; their tank destroyers kill units that wait) by 4,400 (s2b.py); the MCV arrives at 110,14 (6,256). At 8,474 many units died at once in three places (Mirage Warning: mirage tanks) and the MCV was lost at 110,22 before it deployed. Replays in private/campaign_saves/yr/sov02. **Lane P60** (after an under-par video, which never builds a base): no outposts and no MCV; at tick 1,200 the whole starting army (12 HTNK, 6 HTK, 6 V3) in one team by fight_legs 92,60 - 92,100 - 93,140 - 97,165, west of BadGuy1's base (105-121,119-134), fighting what it meets (half of it lost on the way, mostly to the mirage tanks and the infantry by the lab), then force fire at the lab (gone at 7,621) and the Chronosphere (8,151): won at 8,320 with three tanks left. Replay yr/tests/campaign/sov02.replay (ctest yr-native-campaign-won-sov02) |
| Allied 7 (ALL07SMD, Brain Dead) | in progress (lane P60, a video study in hand) | 1.0 | win (Win_Check): destroyed-units-all of YuriCountry (h9: 162 objects, 90 buildings at 150-220,90-190) and Yuri2 (h14); Yuri fire-sales and hunts after 60 of his buildings are destroyed (Yuri2 after 11); lose: the NACNST gone (Player_Lose_Tierra) or everything. Opening: an engineer into the Special house's damaged Soviet construction yard (84,125) makes it the player's; a Soviet radar (SRadar_Built) starts a 120 s countdown, then the Allied MCV and an escort (3 MTNK, 2 FV, 2 MGTK, 1 SREF) are chronoed to 105,194 (tick about 8,700). Try 1: Yuri's raids from the north (87,114) took the Soviet war factory and most units by 20,000, credits 850. The owner's dossier (private/dossiers/yr_allied07_video_strategy.md): delay the radar to about game 6:00, defend both flanks first, capture the two tech buildings, Iron Curtain plus Chronosphere drops into the Dominator compound. Replay private/campaign_saves/yr/all07/p60/try1.replay. **Try 2** (the dossier's opening; try2-dossier.replay, P7D.SAV at 20,502): engineers into the construction yard, the power plant, CATECH01, the oil derrick and CAMACH; sentry guns and flak cannons at the north gate (86,113) and the west road (72,128) before the radar (queued at 7,000, built 8,895): the Soviet base held to 20,500 (10 HTNK, 4 flak tracks, credits about 3,000-4,000) and the Allied MCV, deployed at once at 105,192, has power plants and barracks; the Grand Cannon (GTGCAN) finished but found no legal spot near 105,192 and blocks the defence queue. Next: place it by hand, refinery and war factory on the plateau, Battle Lab, Chronosphere and Iron Curtain, then the dossier's curtained chrono drop of nine tanks into the Dominator compound (the step server has no super weapon command yet; the sidebar cameo and a map click should do) |
| Soviet 3 (SOV03UMD, Brain Wash) | in progress (lane P60) | 0.3 | win: destroy the Psychic Beacon (NAPSYB 48,95, inside BadGuy1's Allied base 36-70,87-110, west of a cliff-banked river crossed only by the bridge at y 88, x 72-82; SP2.1_Destroyed_Beacon frees the Allies), then the Psychic Dominator (YAPPET 62,21; Destroyed_PsyDom: fire-sale, reveal), then destroyed-units-all of YuriCountry (Complete_Objective 3). Start: no construction yard, 2,500 credits, Boris, 3 HTNK, 2 HTK, 6 E2, 3 terror drones, an amphibious APC and 2 engineers at 90,100. Two tries: the army dies at the bridge approach (66-69,88: pillboxes, GGIs, tanks), and Boris ordered at the beacon walks on to 68,91 and dies without calling his airstrike (his range is under 20 cells). Replays in private/campaign_saves/yr/sov03/p60; a video study is asked for |
| Soviet 4 (SOV04UMD, Romanov on the Run) | in progress (lane P60) | 0.4 | win: "Romanov In!" (local 0, set by Romanov's own team at the airport CAAIRP 103,17) or local 29; lose: Romanov killed. Base at 80,138 (NACNST, 8,000 credits, a small ore field at 108-116,108-116). Entering the bar by 139,69 (Romanov Found!, about 23,100) brings Romanov (RMNV, Germans, not player-controlled) at 23,737; he waits there. The airport lies behind Yuri's north base (YACNST 90,20; Arab gattlings at 100,10-30 with their own reactor; the Africans' psychic towers, which go Neutral when Yuri's power is low: Power Low). Try 1: 13 HTNK, 10 E2, 5 dogs by 16,000 reached the bar, then the push up the road (132,60 - 124,48 - 118,36) died in the north base and Romanov was captured (Romanov Slave: British, 118,33; freed when the holder dies: Free Romanov). Replay and S4A.SAV in private/campaign_saves/yr/sov04/p60 |

## Lane P53 (2026-11-26): Robot Tanks that never left the War Factory

Found by lane P52 in Allied 4: a Robot Tank (ROBO) stayed in the War Factory (mission 2, not moving) and blocked every
later vehicle; taken there for the tank being deactivated. It was not: with GAROBO powered, HouseClass::PoweredUnitCenters
is 1 (BuildingClass::FUN_004549B0 registers the centre through HouseClass::FUN_0050E010 as gamemd.exe does) and
UnitClass::IsDeactivated is false. The cause is the exit. BuildingClass::Do_MISSION_UNLOAD (glue/building_native.inc)
force-tracks a drive-locomotor vehicle out, but sends any other locomotor (the Robot Tank hovers) with Assign_Destination to
the factory's cell + 4,1. UnitClass::Assign_Destination (glue/unit_native.inc, a TibSun-shaped draft) keeps a unit in
radio contact with a weapons factory on its way to the exit cell, and queues any other order behind it; the draft's exit cell
was + 3,1, so the factory's own order was queued as "another" and the tank never got a destination. gamemd.exe
(0x00742D46-0x00742E41 in UnitClass::Assign_Destination, 0x00741970, vtable slot 0x480) uses + 4,1 from the factory's
Position, skips the check for MISSION_ENTER, and for a unit held by a locomotor weapon (LocomotorSource) sends
RADIO_OVER_OUT instead; the draft now does the same. Other hover vehicles built in a war factory had the same fault.

Check: ctest `yr-native-robot-tank` (yr/tests/yr_robot_tank.py): power plant, Robot Control Center and War Factory spawned,
a ROBO produced; the harness's new `robots` order logs the powered centres, the tank's deactivated flag and each centre's
registration. Before the fix the tank sat at its factory cell with the factory unloading at frame 420; after it the tank is at
67,22 when the factory returns to guard and drives 6 cells when ordered.


## Lane P55 (2026-11-27): the tank bunker sold with its tank inside (YR Allied 5's crash)

Lane P52's Allied 5 play ended in signal 11 at tick ~96,300 (UnitClass::Take_Damage, then
`As_Building(BunkerLinkedItem)->ClearBunker()` with a NULL building, vendor/ra2-decomp/src/unit.cpp). gamemd.exe
(0x00737D73) makes the same checked cast and calls ClearBunker with whatever it gives, so on Windows a link that named
anything but a building would fault the same way: the original keeps the link from ever going stale. Its
Do_MISSION_DECONSTRUCTION lets the tank out of a bunker being sold (0x0044AAA4: `if (BunkerLinkedItem) UnloadBunker()`,
after the slave manager's release and ahead of the upgrade refund); BuildingClass::Take_Damage does the same when the
bunker dies (0x004424EA), ClearBunker when the tank dies (0x00737D97), TemporalClass::AI and the chrono warp when either is
warped; TechnoClass::Detach drops the link only while a scenario loads (0x00707849, ScenarioInit), as the tree has it.
The deconstruction draft (glue/building_native.inc, TibSun's text, which has no bunkers) had no such call, and Yuri's AI
sold two NATBNKs with lasher tanks inside at frame 94,718: the tanks kept naming the deleted bunkers, and the first to
die read the freed memory (by then another object's). The draft now calls UnloadBunker there.

Checks: yr-native-tank-bunker (harness orders `bunker`, `sell`, `kill`, glue/yr_harness.cpp: a Grizzly killed inside a
bunker, an IFV whose bunker is killed, the IFV in a second bunker that is sold; without the fix the IFV named the object
that took the sold bunker's memory, a GAWEAP); yr-native-campaign-crash-all05 (yr/tests/campaign/all05-crash.replay, the
recording to tick 95,575, replayed to tick 97,000 with `ra2_campaign.py --survive`; label yr-campaign-won). Not done here:
the slave manager call just before it (0x0044AA3D: SlaveManagerClass::FUN_006B0DB0 when the building has a
SlaveManager, under a test of its +0x218 object) is also missing from the draft.

## Lane P56 (2026-11-27): the prism tanks that stopped answering orders (Allied 5)

Found by lane P52 in Allied 5 (resume-from-90439.replay): from some point every prism tank (SREF) kept mission 5 (guard)
whatever it was ordered; move and force-fire orders drew their lines and nothing happened (16 tanks in BadGuy1's cloning
vat block at 160-165,130-136, 20 new ones by the home war factories at 55,80-86), while the harvesters worked. A5STUCK.SAV
(tick 191,948), loaded, gave tanks that drove when ordered; the harness's new `orders <ID>` (glue/yr_harness.cpp: mission,
MissionQueue, MegaMission, NavCom, the navigation and route queues, radio contact, IsDeactivated, MindControlledBy, Team,
the locomotor's Is_Moving and Is_Powered, LocomotorSource) showed every tank clean: no queued mission, no destination, no
radio contact (so not P53's factory-exit check, gamemd.exe 0x00742D46, which this path never reached), not deactivated, not
mind controlled, the locomotor powered. The live state (the replay preloaded to 191,948, about 15 minutes) differed in one
thing: `plans` showed planning mode on. The step server now logs planning mode on and off; it came on at tick 94,642,
eight ticks after a move click at @=163,140 (a cell by the map's south edge, which `look` cannot centre). The step server
resolved that point with Tactical::Coord_To_Pixel, which answers true up to 180 pixels past the view's bottom (and 360
beside it): 316,568, the command bar's Planning Mode button (the bar starts at the view's height, 568). In planning mode
gamemd.exe makes a move click a planned waypoint and carries it out only when planning mode ends (PLAN_COMMIT), so the
game did as the original; a load ends planning mode (it is not saved), which is why the save looked free. Fixed in the step
server: "@=X,Y" and "@ID+dx,dy" give a point only inside the tactical view (as "@^ID" already did), else no point (the
host drops the event, and the server logs "step: @<spec> is at x,y, outside the tactical view"); news lines `planning on`
and `planning off`. Replayed to 95,000 with the fix the click is refused, planning mode stays off and the tanks drive. In
the 191,948 ticks of the replay that was the only point refused; the won replays (Allied 1, 3) still win.

Red Alert 2's resolver (ra2/glue/harness_input_native.inc RB_Resolve, "@=X,Y" and "@ID") has the same gap; not changed here.

Check: ctest `yr-native-planning-click` (yr/tests/yr_planning_click.py, Allied 1 by the step server): a click 10,10 cells
below a tank the view is centred on (316,584, on the command bar) is refused and planning mode stays off; the bar's Planning
Mode button starts planning mode, a move click leaves the tank where it is; the button again ends it and the tank drives to
the planned cell; a plain move click moves it. Without the fix the first click starts planning mode and the last order is
not carried out.
## Lane P57 (2026-11-27): YR's own steps in the TibSun- and RA2-shaped drafts

Lanes P53 and P55 found two faults from glue drafts whose text is Tiberian Sun's (UnitClass::Assign_Destination's factory
exit, the tank bunker left linked to a sold bunker). This lane compared BuildingClass::Do_MISSION_DECONSTRUCTION
(glue/building_native.inc) with gamemd.exe 0x00449C30-0x0044ABC5 step by step and screened the other drafts.

**Do_MISSION_DECONSTRUCTION, every step now as gamemd.exe's** (marked "YR (P57)" in the draft):
* INITIAL: an undeployer (BuildingClass::Can_Be_Undeployed, 0x00449BC0: any UndeploysInto type but a construction yard,
  a yard only for MCV redeploy and not mind controlled) that is artillery or a tick tank drops its target (0x0044A8DF; the
  draft had IsVehicle, a 1x1 foundation); an undeployer speaks VoiceDeploy and plays its DeploySound (0x0044A95E); a
  deployed slave miner whose archive target is a tiberium cell other than its own calls SlaveManagerClass::FUN_006B0DB0
  (0x0044AA3D: the manager finds the ore field and sends the slaves there; the step P55 named); the bunker release
  (P55); the upgrade refund sets RecalcPower and RecalcRadar only (the draft's extra IsRecalcNeeded removed); radio
  RUN_AWAY goes to every link (RadioClass::SendToEachLink, 0x0044AB68; TibSun's Transmit_Message reaches the contact
  only); the damage fires are removed (DamageFireAnims, FUN_0043C2A0's loop, 0x0044AB87) where TibSun ended every
  building animation (DestroyNthAnim(BANIM_ALL), not in gamemd.exe). Removed as not in gamemd.exe: TibSun's repair-bay
  sale (selling what stands on a Service Depot) and its AuxSound2.
* HOLDING: SendToEachLink(RADIO_OVER_OUT) (0x0044A2F5); absorbed passengers (a bio reactor's) are unlimboed at the
  building's own position with its cell's centre height (0x0044A499: gamemd.exe works out a per-cell exit point at
  0x0044A3C4 and never reads it) and scattered; each counts as its house's infantry again with no RTTI test; one that
  cannot be placed is RegisterDestruction(NULL)'d and deleted (no Limbo). No sell sound for an undeployer (0x0044A7CF);
  the type's PackupSound on the building's Audio7 (0x0044A85B).
* DURING: the pack-up sound follows the building (FUN_00750D40, 0x00449C94). Undeploying is Can_Be_Undeployed (0x00449CEA;
  the draft's IsVehicle test sold every larger undeployer instead: a deployed Slave Miner, YAREFN, was refunded and
  deleted with its slave manager); a foundation up to 2x2 undeploys in place, a larger one at its south-east neighbour
  (0x00449E8E); the unit's EstimatedHealth is its strength (0x0044A039); **the building's SlaveManager goes to the unit
  (SlaveManagerClass::SetOwner, 0x0044A03C): the slaves follow the undeployed miner**; the queued voice moves to the unit
  (0x0044A0D4); the unit is selected with AllowVoice off (0x0044A123); the building leaves by Detach_All(true),
  Audio7.EndLooping and UnInit (0x0044A1B5). A sold building's kill is RegisterDestruction(NULL) (vtable 0xE0, 0x0044A1F9;
  TibSun's Record_The_Kill is slot 0xE4).
* Looked for and not in gamemd.exe's function (handled elsewhere, if anywhere): the grinder, cloning vat, garrisoned or
  ice buildings' occupants beyond FUN_00457DE0 (already called), psychic or radiation effects, spy plane or super weapon
  cleanup.

**Other drafts fixed:**
* BuildingClass::Captured (an RA2 twin's text, game.exe 0x00444660): a tech building (NeedsEngineer) is powered on by
  its capture (IsPoweredOn, light, fence post, powered animations; 0x004484B6); the gap generator's DestroyGap before and
  CreateGap after (0x00448BB7, 0x00448C3E); vt_entry_48C(false, false, false, NULL) before TechnoClass::Captured and
  UpdateSight for a player's capture (0x00448BD9, 0x00448D83); FUN_00448070(false) (the factory's primary flag,
  0x00448C69); the new owner gains the ore purifier, self-healing and helipad docks the old owner lost (0x004491D5); both
  houses' Update_Factories for a factory type (0x00449251); in a multiplayer game a computer house that captures a
  non-neutral building sells it at once, upgrades refunded (0x004492D7).
* BuildingClass::Exit_Object's AI placement (TibSun's text): a placement that Flush_For_Placement could not clear counts
  the node's failures (BaseClass::FailedToPlaceNode) and in multiplayer drops a node past
  Rule->MaximumBuildingPlacementFailures (0x00445237; before, the AI retried the node for ever); a placed slave miner
  calls SlaveManagerClass::Deploy2 (0x004452F0); a placed wall piece becomes its overlay wall (Place_Firestorm_Wall,
  Place_Wall, 0x00445345).

**Check:** ctest `yr-native-slave-miner` (yr/tests/yr_slave_miner.py; harness order `slaves <ID>`, glue/yr_harness.cpp):
a YAREFN with five live slaves is sold; it undeploys into an SMIN whose manager holds the same five slaves (by
AbstractClass::ID) and every slave names the SMIN; the SMIN is then destroyed and no slave names a deleted techno. (Where
the skirmish's random start has no ore near, the miner undeploys by itself before the sale, through the same
deconstruction: the check holds either way.)

**Screen of the rest** (gamemd.exe's calls, direct and by vtable slot, whose names the draft's text lacks; research
scripts in the lane's scratch; vtable slots from the native build's Itanium dump at (index - 8) * 4). Most hits are the
tree's other names for one function (aliases, data/name_aliases.tsv). Left for later, each a candidate YR step:

| Draft | gamemd.exe | Not compared / candidate YR steps |
| --- | --- | --- |
| BuildingClass::Exit_Object (building_native.inc) | 0x00443C60, 6,759 B | the factory exits: HasFreeLink (0x00443CD4, 0x004440BC), FUN_0053A130 (0x00443CE7), the exit facing by atan2, Cell_Building/Cell_Techno and In_Local_Radar checks (0x00444294-0x0044433E), HouseClass::FUN_0050B760 and FUN_005060B0 (0x00444FBA-0x004450BD) |
| BuildingClass::Captured | 0x00448260 | Session GAME_INTERNET's CapturedBuildings count (0x00448524); the laser fence's ScenarioInit test and LaserFenceFrame reset (0x00448CF5); the lists re-added after Grand_Opening in gamemd.exe (before it in the draft); the tint/anim drawer (0x00448DB9, Update_Anim_Appearance in the draft, not compared) |
| BuildingClass::Do_MISSION_UNLOAD | 0x0044D880, 2,848 B | DelayTimerClass Count_Up/Count_Down and their completion tests (0x0044DD95-0x0044E239) |
| InfantryClass::Per_Cell_Process (infantry_native.inc) | 0x00519630, 5,083 B | partly compared (lanes N4, P18); not: the engineer's refund and building animations (0x0051986E-0x0051992F), the radar event (0x00519BB6), RepairWoodBridgeAt/RepairConcreteBridgeAt (0x00519CF6) against the draft's Repair_Bridge, IsAbsorbAllowed (0x0051A322), AbortMotion, MapClass::DamageArea (0x0051A6C1-0x0051A7D3), vt_entry_48C/UpdateSight and RevealArea3 (0x0051A8D7-0x0051A925) |
| FootClass::Active_Click_With (foot_native.inc) | 0x004D74E0 | Get_Height_AGL (0x004D7A49), FUN_0056D100 (0x004D7B51) |
| MapClass::Set_Map_Dimensions (map_native.inc) | 0x00565C10 | FUN_00588C10 (0x00565D6F, 0x00566581), HouseClass::FUN_0050D250 (0x00566DDF) |
| FlyLocomotionClass::Nearing_Target | 0x004CEFB0 | FUN_004D03D0, AbstractClass::GetTargetDirection |
| MechLocomotionClass::Movement_AI | 0x005B01C0 | FUN_0056D100, HasAnyLink (0x005B0AAE) |
| ParticleClass::Spark_Behavior_AI | 0x0062C6E0 | the slope matrix bounce (Get_Cell_Ramp, Get_Slope_Matrix, Rotate_Vector) |
| UnitClass::Do_MISSION_HARVEST, InfantryClass::Assign_Destination, TechnoClass::Can_Deploy_Now, HouseClass::Can_Build, CellClass::Closest_Free_Spot, BuildingClass::Drop_Debris, Fire_Direction | | no call missing beyond aliases (Assign_Archive_Target is the ArchiveTarget store; Get_Gate is FindObjectOfType) |
| WalkLocomotionClass::Movement_AI, HoverLocomotionClass::Process, HouseClass::Update_Present_Super_Weapons, Where_To_Place_Building, Can_Build_Here, CellClass::Reduce_Tiberium, DisplayClass::Mouse_Left_Up, UnitClass::Is_Route_Broken, MapClass::Repair_Train_Bridge, the draw drafts | address not found in this pass | not screened |
## Lane P58 (2026-11-27): YR Soviet 1's lock after the naval phase (traced, not fixed)

Found by lane P52: after Objective 1 (tick ~7,550) input stays locked; local 9 is never set, so SP2.14 → ... →
SP2.13_Unlock never runs. Traced with try1-locked.replay (private/campaign_saves/yr/sov01) and the new harness orders
below. What happens:

1. Team "TM_Sov_1 Trans_1 Boris_5 ..." (Priority 7, AvoidThreats=yes, script "Move to X_BG": 3,23 3,58 8,1 20,15 39,9 ...)
   reaches wp23 (57,95) at ~8,100. Its path to wp58 (80,82) goes north round the island (61,71 - 77,71 - 85,70 - 86,76),
   ~70 cells, not over the west beach (~25 cells): every target with x >= 80 does, x <= 79 goes west. The choice is
   AStarClass::Find_Path_Hierarchical's (matched): on the coarse subzone graph the north route scores 3.000 (water
   subzones score 0) and the west one 3.001 (one diagonal, cross-block edge 115 -> 532 next to the rubble CANEWY11
   leaves at 75,82 on frame 1). Without threat avoidance the result is the same.
2. "TM_Sov_1 Trans_4 Flak Trak" (script "Move to X_BF": 3,23 3,57 39,8 8,3) sets local 8 at wp57 (85,74) at ~9,200;
   75 frames later (SP2.6b "Trans 3 Leaves", TEVENT_TIME 5) team "TM_Sov_1 Trans (Flak)" (Priority 10, SAPC, script
   "Secret Trans (Flak) Leaves" 3,15 3,13 3,23 3,10 37,0) is created and recruits the SAPC nearest wp57. The emptied
   Flak Trak SAPC is still in MISSION_UNLOAD (not recruitable) on that frame (it turns to guard one frame later), so
   TeamClass::Recruit/Can_Add (both matched) take Boris's SAPC, 2 cells away, moving (recruitable) in a priority-7
   team. Its 12 passengers stay members of the Boris team in limbo; the SAPC sails west and leaves (P52's "lost at
   43,93"). The Boris team never unloads, so 39,9 never runs.

Compared with gamemd.exe and found the same: TeamClass::AI's dispatch (0x006E9140, jump table 0x006E9F74: cases 3, 8,
0x14, 0x27 call or inline what the draft calls), Flag_Into_Action, the recruit condition, TMission_UNLOAD/Can_Add/
Recruit (matched), AStarClass::Find_Path_Regular (0x00429A90) against astar_native.inc, MapClass::Region_Threat
(0x00585F40, its quadrant tables), Update_Cell_Subzones (0x00584550) and Subzone_Span's span and cross-block rules
(0x005824A0). The SAPC moves at Current_Speed (15 leptons a frame). Not settled: whether gamemd.exe takes the north
route here and loses the same one-frame race (the race turns on Random_Pick in Do_MISSION_UNLOAD's delays), or a
draft upstream differs: HoverLocomotionClass::Process/While_Moving/Start_Of_Move (hover_native.inc, TibSun's text, not
compared; gamemd.exe's Process 0x00514310 already differs at a stopped start: it sets speed 1.0 and calls
Start_Of_Move(0)/While_Moving(1) where the draft calls Start(), and it has a radio-contact branch), the initial subzone
build (Reset_Subzone, Zone_Reset). No source change to game code; no test added (nothing to pass yet).

Harness orders added (glue/yr_harness.cpp): `teams [<text>]` (each team's script line and members: mission, limbo,
cargo, NavCom), `units <ID>` (mission, queue, team, recruitability, threat avoidance), `locals <i>...`,
`path <ID|&addr> <x> <y> [nothreat]` (the path AStarClass::Find_Path gives), `zonemap <x0> <y0> <x1> <y1> <ID>`.

## Lane P62 (2026-11-27): the hover locomotor against gamemd.exe; YR Soviet 1's lock is not the hover glue

Compared HoverLocomotionClass::Process (gamemd.exe 0x00514310), While_Moving (0x00514F70, ra2-decomp's sub_514F70) and
Start_Of_Move (0x005164D0, sub_5164D0) in yr/glue/hover_native.inc step by step (yr_dis.py; vtable slots read from
UnitClass's table at 0x007F5C70). P58's "Start() replaced" is not a difference: the stopped start in gamemd.exe's Process
is the matched HoverLocomotionClass::Start (0x00517110) inlined (Height 1.0, Set_Speed(1.0), Start_Of_Move(0),
While_Moving(true) when HeadToCoord is none or the unit's own cell). Stop and Stop_Driver are likewise inlined; the wake's
AnimClass arguments are the defaults (0x600 is SHAPE_WIN_REL|SHAPE_CENTER); Distance and Distance_To on Coord are 3D as
gamemd.exe's. The steps the draft lacked, now marked "YR (P62)":
* Process: returns false when the unit is no longer active after Motion_AI (0x00514349); the arrival step (While_Moving
  and the per-cell check) also runs when the unit stands on its radio contact's docking cell (Contact_With_Whom,
  Destination_Coord(unit), 0x00514584) and not only when it has a destination; on arriving (MOVE_OK with no next cell)
  Set_Occupy_Bit(PositionCoord) before Per_Cell_Process (0x005146BD): the draft left a stopped hovercraft's cell without
  its occupy bit (it is cleared when the move starts).
* While_Moving: a paralysed unit (FootClass::IsParalyzed) returns MOVE_NO after HeadToCoord is released (0x00514FCD).
* Start_Of_Move: the "close enough" stop for a friendly blockage tests RadioClass::HasAnyLink (0x00516ADE), not the first
  link only. Otherwise the same as the draft.

Also fixed: the P58 merge (a407dc5) had dropped the `return(1); }` that ends the harness's `slaves` order, so
yr_harness.cpp did not compile at main's head.

**Soviet 1 rerun** (try1-locked.replay from a fresh start through the step server, then 2-3 frame steps with `units SAPC`
and `locals 8`): unchanged. The lock is a 3-frame race, timed:
| tick | Flak Trak SAPC (team Trans_4 Flak Trak) | Boris's SAPC |
| --- | --- | --- |
| 9,179 | reaches 85,74 (wp57) | 85,72, north route, moving |
| 9,200-9,203 | guard, then local 8 set and MISSION_UNLOAD (16) | |
| 9,206 / 9,221 / 9,236 / ~9,251 | the four HTK leave (one per 15 frames); its team disbands | 86,74 - 86,76 |
| 9,278 | still MISSION_UNLOAD (door closing) | recruited by "TM_Sov_1 Trans (Flak)" (SP2.6b, local 8 + 75 frames) at 85,76 |
| 9,281 | guard: recruitable, three frames too late | sails west with its 12 passengers; local 9 never set |

UnitClass::Do_MISSION_UNLOAD (0x0073D630) is matched; each of its states waits the mission control's rate plus
Scen->RandomNumber(0, 2) (0x0073DD4B, 0x0073DE5F, 0x0073E2B0), so the unload's length moves by a few frames with the
random sequence, and the 3-frame margin can go either way in gamemd.exe too. **Next suspect:** whatever shifts the
random sequence or the Flak SAPC's arrival by a few frames before 9,203 (another draft that draws Scen random numbers, or
the team's route timing); without a gamemd.exe run of the same inputs the two cannot be told apart. Not ground further.
Boris's SAPC still takes the north route (Find_Path_Hierarchical's 3.000 against 3.001, P58). No hovercraft behaviour
visibly changed in this mission, so no ctest added. Checks: yr-native-campaign-won-all01, -all03, -campaign-crash-all05, -tank-bunker, -robot-tank, -slave-miner, -planning-click pass.

## Lane P63 (2026-11-27): YR campaign wins continued (Soviet 3; Allied 5 re-recorded)

Main at 458350f did not build yr-native (the P58 merge dropped the end of the harness's `slaves` order); main fixed it in
865e8d3, the same two lines. The four won replays rerun on main with P57's and P59's changes (and again after P62's
hover merge): Allied 1, 3 and Soviet 2 still win at their ticks. Allied 5's did not (the replay ran out at 112,634 with
BadGuy2's base standing and 83 prism tanks parked at home: its early commands now meet another game), so it was
re-recorded as lane P59 re-recorded Red Alert 2's: the whole recording preloaded, then the prism tanks band-selected and
walked through BadGuy2's base (90,100 - 110,100 - 124,100 - 130,103) and on to the nearest BadGuy1 object in turn: won at
141,010, a rerun wins at the same tick.

Soviet 3: Boris's range is not a port bug. Ordered at the beacon from 12.5 cells (55,84, reached round the pillboxes by
69,82 - 60,81), he stepped one cell and painted at 11.7 (the Flare, Range 12), four BPLN came and the beacon fell
(the dossier's test). The earlier deaths were the route through the pillbox cluster at 60-66,88-93.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Soviet 3 (SOV03UMD, Brain Wash) | won at tick 29,470 | 0.3 (P60) + 1.6 | Tanks to the docks (70,79), Boris by 60,81 to 55,84 and the beacon painted (gone 5,512); the Allied base turns at about 6,900, input opens 8,213. The park (55-75,38-52: gattlings, power plants, the war factory, initiates) ate two armies of 17; the west road (57,66 - 53,56 - 49,48 - 46,41, path found with `order path`) avoids it. The Dominator fires about 9,400 ticks after the turn (about 17,300), at the biggest cluster (once at the army's rally point: three tanks controlled, Boris lost). Boris must keep painting until the planes arrive (about 600 ticks from the south edge): moved away after painting, the strike never lands. From 52,30 one strike took YAPPET to 192 (repaired about 0.5 a tick) and Boris died before a second; the win: nine tanks beside Boris at 51,32, sent at YAPPET when the planes came within 18 cells: the strike plus the tanks killed it at 24,882 (the tanks and Boris died). The fire sale leaves initiates and slaves hunting; Tanya, the sniper, the SEAL and two tanks killed the last 26 (slaves need not die): won. Replay yr/tests/campaign/sov03.replay; tries in private/campaign_saves/yr/sov03/p63 |
| Soviet 1 (SOV01UMD) | lock avoided; not won | 0.3 (P52) + 1.1 | P62 found the lock a three-frame unload race. As a player: P52's naval script (s1a.py: the fleet at the nearest destroyer, carrier or cannon) started 650 ticks after input opens: Boris's team lands (locals 9 and 23 set) and input opens at 14,302 with Boris, 3 dogs, 5 tesla troopers, 3 engineers, 4 HTNK, 4 HTK and 12 conscripts ashore. Started at once, 200 or 400 ticks late, or P52's recording shifted by 41-500 ticks (its clicks then miss), it locks. Then the time-machine defences (4 GAPILL, Confederation, 4 NASAM, Alliance, round CATIME01 144,97, which must not be hit: its loss loses): the whole landing died at 124-138,90-97 (pillboxes, GGIs, E1s; Boris at 16,515) with one pillbox down; each loss of the conscripts or Rhinos brings 12 E2 or 4 HTNK back at 77-81,74-79; a second wave of 17 died at 124-132,90-94. Next: the landing's tesla troopers and Boris's strikes on the GGI nests first (not on the pillboxes next to the time machine), then tanks on the pillboxes one by one. Recordings and scripts in private/campaign_saves/yr/sov01/p63 (s1-delay650-to14302.replay: the unlocked landing) |
| Soviet 4 (SOV04UMD) | in progress | 0.4 (P60) + 0.5 | P60's try1.replay still preloads to its S4A state at 15,975 (13 HTNK, 10 E2, 5 dogs, credits 25). Credits stayed at 25 to 21,500 with three harvesters and the oil derrick at 42,114 taken (52,68's engineer did not arrive); 20 HTNK up the central road died by 26,189 at 95,78-84 to the garrisoned Moroccan houses there (CAMORR05/07, YuriCountry) and the plaza's gattlings. Next: Romanov is the Germans' (PlayerControl=yes: the owner's dossier), the economy first (more refineries are not possible on this ore; the derricks), V3 or Ivans for the garrisons, and the reactors at 103-107,32 for the airport's towers. Recording private/campaign_saves/yr/sov04/p63/s4-try1.replay |
| Soviet 5 (SOV05UMD) | shelved for now | 1.8 | Medium: the Submarine Pen (YAYARD 138,151) alone wins. The map has 16 Boomers preplaced in [Units] off the south coast and in the pen's inlet besides the team ones at waypoints AN-AY and others (the dossier counted 12): about 44 at the start. The fleet ordered at the pen took the west route (the pathfinder's choice: cost 330 against about 220 by the east channel, which is navigable: the fleet sailed it to 236,172 in legs) and the dreadnoughts swung between the two routes on the way; both times the escorts and dreadnoughts sank to the Boomers (east channel by 8,900, west by 12,600). The MCV deploys only off the ore at the AL beach (origin 211,120: move it to 212,121; at 214,124 the footprint is on TIB01). A beach base (two refineries, coils, sentry guns) lost a refinery at 25,752 and went broke; one Kirov by 45,000. Next: the derricks early, a naval yard and Typhoons against the inlet's Boomers, or Kirovs with a second ore field. Recordings in private/campaign_saves/yr/sov05/p63 |
| Allied 7 (ALL07SMD) | not started here | 1.0 (P60) | P60's try2-dossier.replay no longer matches after P57/P59/P62: preloaded to 20,502 its Soviet base (NACNST, war factory, refineries, tanks) is gone, only the Allied MCV's base stands. A fresh start is needed; the super weapons' sidebar cameos (tab 1 shows them as ...Special) and a map click may do without a new step-server command |
## Lane P61 (2026-11-27): Per_Cell_Process and Exit_Object brought to gamemd's shape; Grinder, Cloning Vats, pads

Lane P57's "screened but unfixed" list, taken in order of player impact. Each draft was disassembled from gamemd.exe
(capstone, named from vendor/ra2-decomp's manifest and FUNCTION comments, vtable slots from gamemd.exe's RTTI) and its
member offsets named from the i686 layouts (tools/original_layouts.py --game yr --members). Changes are marked
"YR (P61)" in the drafts.

**InfantryClass::Per_Cell_Process** (glue/infantry_native.inc, gamemd.exe 0x00519630, 5,083 B; every step compared):
* **The Grinder (MISSION_EATEN, 0x0051973C-0x00519945): the draft had no step for it**, so a soldier sent into a Grinder
  (YAGRND) walked up to it and stood there, no refund. Now as gamemd.exe (and UnitClass's matched text): the die voice
  (the player's) and die sound, the grinder's house refunded Refund_Amount, the soldier's tag, EnterGrinderSound and the
  grinding animation (BANIM_ACTIVE_ONE ended, BANIM_SPECIAL_ONE), the soldier deleted.
* Entering a building (0x0051A2AD-0x0051A39D): the weapon state and any mind control dropped; a bio reactor
  (InfantryAbsorb) plays EnterBioReactorSound and marks the soldier Absorbed; a soldier counted as its house's
  infantry (CountedAsOwnedSpecial) stops counting (lane P57's deconstruction counts it again); the reactor gets no radio
  HELLO but its house's RecalcPower when it has ExtraPower; AbortMotion (the draft cleared NavCom).
* Bridge repair (0x00519B90): the player's EVA line only when RadarEventClass::Create(RADAREVENT_BRIDGE_REPAIRED) makes
  the event; RepairBridgeSound at the hut; a low bridge overlay (74-101) near the hut counts as the train bridge tiles do.
* Engineer (0x00519D47-0x00519F2C): a neutral house's garrisonable building (IsMultiplayPassive, CanBeOccupied) is
  repaired as an ally's; a building at full strength sends the engineer away instead of using it up; a building being
  sold or warped out is not captured.
* Other soldiers sent into a building they target (0x0051A03E) are sent away, not deleted; the spy's EVA line is
  BuildingClass::Infiltrate's own (the draft played EVA_BuildingInfiltrated a second time).
* Sabotage (0x0051A4BF): only a bomber (IsBomber); no charge on an iron-curtained building; a charge already set sends
  the soldier away; AbortMotion.
* The idle test is HasAnyLink (any radio link, 0x0051A8AA); the sight moves by vt_entry_48C, UpdateSight and
  Map.RevealArea3(coord, 0, LastSightRange + 3) as UnitClass::Per_Cell_Process (0x0051A8CB; the draft called Look).
* **No ore damage** (the last commit): the draft (TibSun) hurt a soldier entering an ore cell (Power / 10, at least 1)
  and could make a visceroid; gamemd.exe goes from the land test straight to FootClass::Per_Cell_Process (0x0051A9EB) and
  nothing else in the tree hurts infantry on ore. Owner's ruling: the original's behaviour; the won replays recorded
  under the draft's ore damage are re-recorded by the campaign lane (see below).
* Compared and the same: the garrison step (lane N4), the strange-building capture, the transport boarding (lane P18),
  the bridge sabotage, the tether, the land test. Kept and noted: the draft's vehicle-thief branch (gamemd.exe takes only
  IsStrange buildings there; its hijack path is elsewhere, not compared).

**BuildingClass::Exit_Object** (glue/building_native.inc, gamemd.exe 0x00443C60, 6,759 B):
* **Cloning Vats (0x004449D9-0x00444A51): the draft never cloned.** A soldier leaving a barracks (ToBuild
  INFANTRYTYPE, not a vat) is copied by each of the house's CloningVats: a new one of its type for the vat's house, put
  out by the vat's own Exit_Object.
* Aircraft (0x00443CB4-0x00444091): onto a pad while the building has a free radio link (HasFreeLink; an Airforce
  Command's four pads take one each), tethered to the aircraft itself, DockNowHeadingTo set; an airport-bound aircraft
  with no free pad is not made (0) and is not sent to the rally point. The draft took the first link only, so a second
  Harrier made while the first held its pad came in from the map edge. gamemd.exe's ion storm test (FUN_0053A130)
  always returns false, so its storm placement is never reached and is not carried.
* Units and infantry are refused only when every link is taken (0x004440BC); a cloning vat is not a vehicle exit
  (0x0044462A).
* Naval yard (0x00444167-0x0044448F): the boat goes straight onto the water: from the yard's centre cell towards the
  rally point to the first cell outside the yard, that cell when it is open water in the visible map, else
  Map.Nearby_Location for the boat's SpeedType; faces east; sent to the rally point; no radio contact (the draft took
  the war factory's tethered unload). Not covered by a test (the test skirmish map has no water).
* Yuri's barracks' door (+2,1; IsYuriBarracks) as GDI's and Nod's (0x00444857, 0x00444C2C).
* Compared and the same: the war factory (the other-factory hand-off), the AI building placement (lane P57).
  Not changed: the infantry path after Unlimbo moves the soldier to the exit cell only when it has a NavCom (and is not
  a jump jet or teleporter, 0x00444CA3) and gives a hospital's patients GUARD_AREA; the draft moves every one. Left for
  a later lane (the barracks' exit works and the replays lean on it).

**Checks:** ctest `yr-native-grinder` (yr/tests/yr_grinder.py; harness orders `enter <ID> <BLDG> eaten`, `absorb <BLDG>`):
a GI sent into the player's Grinder is refunded (10,000 -> 10,100) and gone; one sent into a Bio Reactor is a passenger
and the power rises (150 -> 250). ctest `yr-native-factory-exits` (yr/tests/yr_factory_exits.py): an Initiate made at a
barracks with a Cloning Vats (NACLON) gives two; two Harriers made at an Airforce Command both start on its pads (before:
the second at 9,65 with the Command at 53,98). Garrison, standins, standins-triggers, combat, tank-bunker, bridge-repair,
slave-miner, deployers and crash-all05 pass.

**Won replays** (the owner's ruling: the original's behaviour; re-recorded once by the campaign lane after lane P64's pin
move): at the branch tip all four YR won replays fail: all01 is lost at tick 138,623 (won at 33,939), all03 does not end
(won at 12,952), all05 does not end (won at 112,634), sov02 does not end (won at 8,320). Bisected with a temporary
toggle per step on the first two: all01 depends only on the draft's ore damage (it wins with every other P61 step in);
all03 depends on the ore damage and on the building-entry step (it wins when both are put back). all05 and sov02 were
only run at the tip. crash-all05, tank-bunker, slave-miner, garrison, standins, standins-triggers, combat,
bridge-repair and deployers pass.

**Still unfixed from P57's list** (not reached in the time box): Do_MISSION_UNLOAD's delay timers (lanes P58/P62 work
there), FootClass::Active_Click_With (Get_Height_AGL, FUN_0056D100), MapClass::Set_Map_Dimensions (FUN_00588C10,
HouseClass::FUN_0050D250), Fly/Mech locomotors and Spark_Behavior_AI, the drafts with no address found, and Exit_Object's
infantry move-after-Unlimbo noted above.

## Lane P64 (2026-11-27): the vendor pin moved to ra2-decomp ac45ee9e; glue drafts retired where upstream matches

**New pin.** `vendor/ra2-decomp` is gibbo101/ra2-decomp `ac45ee9ec1dc44ae2a9c54e37570c25de0fea994` (27 Nov, 909 commits after
`8719494727`), copied by `tools/update_yr_vendor.sh` from redalert2-decomp `c768a31a42` (its `yr/ra2-decomp`, archived unchanged
from the local reference clone; on redalert2-decomp main since f31bb57b0b). `vendor/tibsun` is unchanged (upstream's
`TIBSUN_COMMIT` is still `dbfbfbf`). New upstream text includes `src/ra2asm/*.asm` (MASM, not built natively). Licences
unchanged (GPL-3.0-or-later with EA's section 7 terms; `data/` MIT); THIRD_PARTY.md updated.

**What upstream still lacks.** `data/functions.csv` lists 20,386 functions; the manifest matches 19,326. Of the 1,060
unmatched, 1,033 are library code (CRT, MFC-style helpers: `data/library.csv`), so **27 game functions** are unmatched (the
README's "19,326 of 19,353"), 6 of them "reconstructed" (`data/reconstructed.csv`: 0x0047EFE0 Draw_Shroud_Or_Fog_Shape,
0x00492830 BlitTransLucent50::BlitForward, and four TibSun asm routines). The other 21: 0x00443C60, 0x00481180
CellClass::FindInfantrySubposition, 0x004BC750, 0x00547CF0, 0x00565C10 MapClass::Set_Map_Dimensions, 0x0056D100
MapClass::Is_Same_Cell_Zone, 0x0058D620, 0x005981F0, 0x00598960 MapGeneratorClass::Generate_Random_Map, 0x0060A5B0
OwnerDraw::ResetControlDrawModeAndTimerProc, 0x0060B950, 0x00610CA0 OwnerDraw::DefaultHandler, 0x00617250
OwnerDraw::ComboBoxHandler, 0x00618D40 OwnerDraw::ListBoxHandler, 0x0064BDD0 (Extract_Compressed_Events), 0x006D3D10
TacticalClass::Render, 0x006D7F20, 0x0075AEC0, 0x00789B60, 0x007BAB90, 0x007BB350. Upstream carries markerless drafts of
some of them (Set_Map_Dimensions, Generate_Random_Map, ListBoxHandler); the port removes those (fixups.json regex rows) and
keeps its own, which the played checks have run on.

**Retired** (the draft removed, upstream's matched function used; each checked twice: clang's redefinition of the name in
the unit, and the address in `data/manifest.csv`). 152 functions; 37 glue files deleted outright (`ahandle2`, `alphashp`,
`astar`, `bounce`, `builtype`, `bullet`, `combat`, `credits`, `fire_at`, `fly`, `hover`, `infantry`, `init_game`,
`ionblast`, `ipxmgr`, `ldprgmgr`, `link2`, `mech`, `partsys`, `pcx`, `progress`, `radar`, `radbeam`, `rocket`,
`scenario`, `slavemanager`, `srfcache`, `startup_winmain`, `team`, `techno`, `techtype`, `ui_register`, `vanim`,
`wave`, and the emptied helpers):

| Glue | Retired |
| --- | --- |
| building | Do_MISSION_DECONSTRUCTION, Drop_Debris, FUN_00457DE0, Captured, Fire_Direction, Do_MISSION_UNLOAD |
| bullet | Unlimbo, AI, Shrapnel, Projectile_Motion |
| combat / fire_at | Explosion_Damage, DamageGroup / Railgun_Beam_Damage, TechnoClass::Fire_At |
| display / foot / fly / mech / hover / drive / ship / rocket / walk | Set_Cursor_Pos, Mouse_Left_Up / Active_Click_With / Nearing_Target / Movement_AI / Process, While_Moving, Start_Of_Move / Draw_Matrix, While_Moving (both) / Process |
| house | Update_Present_Super_Weapons, Can_Build, Where_To_Place_Building, AI_Build_Defense, Random_Cell_In_Zone, Fire_PsyDom |
| infantry / unit | Assign_Destination, Per_Cell_Process / Assign_Destination, Do_MISSION_HARVEST |
| map / map_zones | AddContentAt, Reveal, Repair_Train_Bridge, Repair_Bridge, Pick_Random_Tile_Variant / Zone_Reset, Compute_Zone_Connections, Reset_Subzone, Register_Zone_Connection_Entries, Update_Cell_Subzones, Region_Threat, Can_Repair_Bridge, Zone_Span, Subzone_Span and their pack helpers |
| mapgen | Create_Region_From_Cell, Build_North_Ramp, Generate_Starting_Points, Place_Waterfall, Grow_Water_Region, Create_Tiberium, Generate_Rural_Roads |
| netshare / queue | LBSaveSelections, DecodePubGameopt, FUN_005EA3E0, FUN_005EC9F0, FUN_005E6920 / Execute_DoList, Queue_AI_Multiplayer, Wait_For_Players, Draw_Sync_Bars, Game::LogFrameCRC |
| ownrdraw (+combo, ui_register) | StandardWndProc, OwnerDrawButtonHandler, StaticHandler, FUN_0060C0C0, NewEditHandler, FUN_0060D540, UI::RegisterWindow |
| sidebar / tactical drafts' neighbours | Reposition_Sidebar, both AI, Draw_It, Recalc, Action |
| techno / techtype / vanim | Techno_Draw_Object, Draw_Pre_Render, Techno_Render_Voxel_Object, Draw_Health_Bar, Can_Deploy_Now, vt_entry_3D8, Draw_A_Link_To / the TechnoTypeClass constructor / the VoxelAnimClass constructor |
| startup / init | WinMain / Init_Game, Allocate_Surfaces, ConvertClass::CreateFromFile, FUN_00534E50 |
| planmgr / slavemanager / uicmnds / wave / vox / voxel / palette / radar / radbeam | the FUN_ drafts (FUN_0063AB60, FUN_0063A5A0, FUN_00639CB0, FUN_00637E00, FUN_0063B550, FUN_00634E10, FUN_0063C690, FUN_0063CAE0; FUN_006AF6C0, FUN_006AFD60, FUN_006B0880; FUN_00732950, FUN_00733380, FUN_007336C0; Draw_Sonic, Draw_Laser, FUN_007602E0; FUN_00752760; FUN_00754CB0; FUN_006267A0; FUN_00658330; FUN_00659AC0, FUN_00659FF0) |
| others | FillCurrentVector, AlphaShapeClass::Draw_All, Find_Path_Regular, PlaceBeacon, BounceClass::AI, Get_Draw_Rect, Reduce_Tiberium, Graphic_Logic, Show_Who_Was_Responsible, IonBlastClass::One_Time, Service, Preview_Tile, LoadProgressManager::Draw, LineTrail, the Options dialog FUN_0055FDB0, Spark/Gas_Behavior_AI, Smoke_AI, Spark_AI, Draw_Player, Wait_For_Players_To_Load, SurfaceCache Draw/DrawMasked, SuperClass::Launch, Impatient_Click, TAction_IRON_CURTAIN, TeamClass::AI, Read_PCX_File |

The YR (P5x/P6x) gamemd fixes inside retired drafts (P53 factory exit, P57 deconstruction/capture, P61 Per_Cell_Process,
P62 hover) go with them: they brought drafts to gamemd.exe's shape, which the matched functions are by construction.
The native-only parts of retired drafts are carried as fixups on upstream's text: the interface size's sidebar height
(WinMain), the Enhancements button and its label (FUN_0055FDB0), the HD vehicles' owner call (Techno_Render_Voxel_Object),
the credits' wait servicing the callback and their "credits:" log line (Show_Who_Was_Responsible), Queue five with Shift
(StripClass::Action's PRODUCE events), the LAN CRC log in Queue_AI_Multiplayer, the -CD. disk check (Init_Game), no SEH frame
around Main_Game. WS_Save_Dialog_Values (the old pin's name of 0x00608070, called by the Enhancements page and the network
screens) is aliased to upstream's FUN_00608070. The divide-by-zero and bounds guards the drafts carried (AI_Build_Defense, Smoke_AI, Spark_AI, the slave
manager's null owner, Explosion_Damage's ring bound, Bullet AI's SpawnDelay) are not carried: arm64 integer division by
zero does not trap, and the matched code is what gamemd.exe runs.

**Kept** (upstream lacks the function, or the glue is port-only): Tactical::Render and Draw_Tiles(Cell, Rect) (tactical),
BuildingClass::Exit_Object and ForceCreate (building, with P61's fixes), OwnerDraw::DefaultHandler, ComboBoxHandler,
ResetControlDrawModeAndTimerProc, FUN_0060B950 (ownrdraw, ported from the old pin's Dialog/Wide views to upstream's named
WinData; the window side table for movie/shape pointers is gone, the record's members hold them), ListBoxHandler
(ownrdraw_list; upstream's draft removed), Set_Map_Dimensions (map), Generate_Random_Map and its helpers (mapgen),
FUN_0064BDD0 and the LAN CRC log (queue), CellClass::Draw_Shroud_Or_Fog_Shape and Closest_Free_Spot (cell), the map-zone
helpers upstream declares but does not define, MultiplayerGameMode's base virtuals (mpgamemode: upstream matches that code
as MultiplayerBattle's), the FUN_004E02C0 / FUN_00652540 forwarders (upstream matches them as templates the native build
does not emit by those names), Game::CallBack, the movie and multiplayer bindings, wsproto's CRC wrappers, xsurface,
dsurface, isotype Draw_Tile, walk Movement_AI, the ccini/uicmnds2 aliases, and every `yr_*.cpp` (step server, harness,
platform glue, enhancements, map zoom, LAN). New: `yr_wol_globals.cpp` (storage for Westwood Online globals upstream's units
now reference while wonline.cpp stays out), `_itow` (yr_crt), `set_control_word` (yr_asm).

**fixups.json.** Dropped as fixed upstream: MouseControl[86], the Audio*Attribs pointer types, the SidebarRect DAT_ names,
the sidebar mouse class, LightningStorm_RequestStop, WinData's raw offsets (13 rows), and the four class-array aliases in
globals.cpp (Sides/HouseTypes/TeamTypes/TechnoTypes: upstream no longer defines the ::Array statics, so the rows left the
vectors undefined and the house types unbuilt). Rewritten for moved text: the backdrop art (now WinData members),
Tactical::DrawTimer, Change_Display_Mode, Ingame_Menu_Dialog, the score sort, the progress unit. New for clang: explicit
specializations declared before use (map.cpp, srfcache.cpp), WideWstring::truncate's 64-bit WPARAM, INVOKE_YR's case values.
globals.tsv gains the addresses of names upstream introduced (CameoDrawer, Keys, NoExceptionHandler, the Drive/Tunnel
locomotor CLSIDs, ...); name_aliases.tsv binds Main_Game's Init_Game(argc, argv) to Init_Game(void). The link closure
went from 89 to 88 undefined symbols; the new ones are Westwood Online functions only.

**Pointer width in newly matched code.** The tests found upstream functions that keep pointers in 32-bit ints: the
techno-list helpers' callback context (uicmnds.cpp, type select) and the sonic wave's depth-buffer address (wave.cpp,
stand-ins and the Allied 5 / Soviet 6 soaks); both are fixups now. One is still open: OwnerDraw's combo drop-down procedure
FUN_0060D540 (matched upstream, the port's draft retired) faults on a cut pointer (+3752) in yr-native-interface-size's
1.5x run.

**Checks** (Mac, -j4). Pass: skirmish-smoke, -score, -long, save, original-layouts, member-aliases (table regenerated),
map-zoom, zoom-abort, end-banner, hd-vehicles, combat, controls, graphics, touch-team, type-select, command-bar-thumb,
locked-input, standins, standins-triggers, war-factory, robot-tank, tank-bunker, slave-miner, grinder, factory-exits,
garrison, mirage, deployers, building-anims, bridge-repair, keys-load, save-dialog, choose-map, skirmish-layout,
planning-click, window-resize, lan-lobby, campaign-allied (with the credits row), campaign-crash-all05, mission-soak-yr-allied and -soviet; ra2-quick
15/15. Fail: interface-size (FUN_0060D540, above), planning (planning mode's node placement and pick-up: the matched planning
mouse handlers FUN_0063AB60/FUN_0063A5A0 replaced the port's drafts and the harness reads no nodes; not yet traced),
campaign-soviet (failed only on the credits log before that row; not rerun), lan-pair (both games stay in the lobby: "never
reached the game", no CRCs; the retired LAN drafts were the frame loop and Wait_For_Players_To_Load, whose draft returned
true for every session type, and upstream's matched network start wait now runs; not yet traced). ubsan labels not run.

**Won replays** (not re-recorded; a campaign lane re-records them): all01 lost at tick 186481 (P61's ore-damage removal,
as P61 expected); all03, all05, sov02 and sov03 reach their recorded win tick without the game ending (no game over; no
crash, no fault). P61 expected all01 and all03 to fail; all05, sov02 and sov03 now diverge too, which the pin move explains
(152 drafts replaced by matched code changes the simulation); none showed a crash.


## Lane P66 (2026-11-27): the five P64 failures

* **yr-native-interface-size** (port gap: pointer width, plus a record layout). Upstream's matched
  OwnerDraw::ComboDropWindowHandler (FUN_0060D540) keeps an item's CB_GETITEMDATA in an `int` and reads it as a
  `WinNode` (text at +8 on i686, +0x10 on arm64): the cut pointer faulted at +3752 in the drop list's paint. A
  fixups.json row keeps it as LRESULT. The port's combo draft (ComboBoxHandler, kept) stored its own record
  {Reserved, Data, Text, Next} as the item data, whose text was at +8 natively; its fields now follow WinNode's order
  (next, id, text, data), with a static_assert on the text offset. Passes.
* **yr-native-planning** (an upstream difference, gamemd.exe's behaviour; no hook was missing). Upstream's matched
  Execute_DoList gathers a frame's MEGAMISSION events and spreads a group's orders to one cell over the cells around
  it (FUN_0064CDA0), planning events included; the TibSun-text draft it replaced had no group move. Five selected
  tanks therefore plan 15 nodes, one path each, and gamemd.exe's hover test FUN_0063AC70 refuses a node that is not on
  every selected tank's path (5), so no node was hovered or picked up. Checked with a temporary trace: the drawn-nodes
  list and the hit test work (the mouse on node 0's lead box finds it; FUN_0063AC70 refuses it). The test now plans
  with one tank (`select MTNK 0`). Passes.
* **yr-native-lan-pair** (a test timing race the matched code makes tighter; the real start wait works). The host's
  Start Game starts only once every guest has accepted (a guest not yet accepted is told to press Accept and the press
  is undone, netdlg2.cpp); the script pressed it at s3+40000 and the guest accepted at its own s3+20000, about 14 s
  later on the wall clock. Upstream's matched DecodePubGameopt refreshes the guest's slot and option controls on every
  options packet (gamemd.exe's code), which under ctest -j4 kept the guest screen from settling for 16 s more, so the
  host pressed first and both stayed in the lobby. Unloaded the same build passes (3,000 frames, CRCs equal): upstream's
  Wait_For_Players_To_Load and Wait_For_Players run natively; no stub or hook needed. The host now presses at s3+60000.
* **yr-native-campaign-soviet** and **yr-native-member-aliases**: rerun, see the checks below.

**Checks** (Mac, -j4, with lane P65's RA2 campaign replays running beside them): the yr-native suite without the won
replays, 44 of 44 (lan-pair-wine skipped: no Windows build given), interface-size, planning, lan-pair, campaign-soviet
(with P64's credits row) and member-aliases among them; ra2-quick 15/15; check_tree clean. The yr-ubsan labels were not
run (they need a separate RA2_NATIVE_UBSAN_FLOAT build and run the soaks and won replays).

**Won replays** at lane P66's tip (left failing; a campaign lane re-records them): all01 lost at tick 186,481 (as at
P64); all03, all05 and sov02 do not end (no game over by ctest's 2,400 s limit); sov03 now ends lost (game over, lose,
after 1,608 s; at P64 it did not end). No fault or trap in any of the five logs.

## Lane P68 (2026-11-27): the won replays re-recorded on the ac45ee9e pin

Each of the five pending recordings preloaded on the new pin to find where it parts, then played on from there or from
the mission's scripted plan (lanes P52, P60 and P63's scripts), with a cap of 1.5 lane-hours a mission. One re-recorded
win; four shelved and still in `yr_won_pending`. No crash, trap or assertion in any run. Recordings and scripts are in
private/campaign_saves/yr/<mission>/p68 and scripts/p68.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 1 (ALL01UMD) | won at tick 33,374; a rerun (ra2_campaign.py --game yr) wins at the same tick | 0.8 | P52's recording plays out unchanged to 16,982 (A1POWER) but no plant has been taken: the IFV's engineer never got in, and six engineers wait at home. From there: engineers at CAPOWR01-04. The south pair (120-122,147) cost two rounds of engineers: three initiates at 113,125, then something at 122,146. Full power (1,180) brings the time shift, then P52's a1p7-a1p10 unchanged: Apocs_Gone, Tanya along 74,96 - 68,86 - 63,76 - 62,68 dies on landing, six carriers sink the Dominator. yr/tests/campaign/all01.replay replaced; all01 back in yr_won_all |
| Soviet 2 (SOV02UMD) | shelved at the cap | 1.5 | P60's rush (s2r.py from 1,200) now loses 14 of 24 by the first leg. The three groups merged by plain moves at 100,48 (16 left at 3,983), then plain moves to 93,145 (14 left). The lab plateau (100,175) is reached only through 115,174; three V3s at 108,161 reach both the lab and the Chronosphere. At about 9,354 the Chronosphere goes to BadGuy2 (Chronosphere Transfer), which repairs it, so it must fall first. Best tries: two V3s on the lab and one on the Chronosphere: the lab gone at 9,050, the Chronosphere at 434/750 at the transfer (try7-labdown). All three on the Chronosphere: gone at about 9,000, then the lab took almost no V3 damage, and nine Rhinos died at the gap (try8-chronodown). Next: the same split with the V3s sent ahead of the merge, about 300 ticks sooner. A move click on a cell near the map's iso edge (120,20; 125,30) is refused by the step server ("outside the tactical view"): the view cannot centre there |
| Allied 3 (ALL03UMD) | shelved at the cap | 1.5 | P52's recording parts by 3,043: P52's nuke took the war factory at 2,700, here it stands; then the Weather Control Device has no power and the Lightning Storm never recharges (3,086 ticks left). The two six-tank reinforcements come only after the army moves west (about 7,800 and 17,000). Four tries along y 96 (P52's party_legs, the reinforcements gathered at 90-100,96, focus fire on the gattlings at 56,89 and 56,94): the leg from 78,94 to 66-68,92 costs half the army (initiates and Yuris from 57-77,79, mind control), and the silo never fell below 868. Next: snipers or GGIs on the Yuris and initiates first, from the 23,200 unspent credits (the barracks stand) |
| Soviet 3 (SOV03UMD) | shelved at the cap | 1.4 | P63's recording holds to the beacon (5,512) and the gather (11,004), and parts on the west road (one HTNK at 46,41 at 14,818 where P63 had 13). From 11,004 P63's s3w, s3q and s3r: about ten tanks at 47,42 by 22,000 and Boris walked to 52-53,29-31. Each time Boris dies 50-100 ticks after he calls the strike (four BPLN come), before it lands; killing the gattling at 56,29 first did not help. The six initiates at 54-57,25-29 (they come back) are the likely cause. Next: clear them, or paint from further west. sov03-try1.replay preloads to 24,682 with Boris at 47,44 |
| Allied 5 (ALL05UMD) | shelved | 1.1 | The recording parts at its opening (credits 7,336 at 3,520 against P52's 11,061). Its placements then miss and the base is gone by about 15,500, with 16,775 credits unspent. A fresh start with P52's a5a.py and a5b.py holds: four war factories and 20 SREF by 33,365. a5war.py along 75,95 - 95,98 - 110,100 - 128,100 - 120,82 razes BadGuy2 by 43,377 (8 SLAV and 2 SMIN left, 23 SREF). A sweep of the nearest objects (a5y.py, after P63's a5x.py) then lost every prism tank to the garrisoned civilian buildings at 101-120,114-127. Next: from all05-try1.replay --until 43377, build 60 or more SREF (P63 won with about 87), then BadGuy1 by P60's east route, leaving the garrisoned buildings alone |
## Lane P69 (2026-11-27): Allied 5's early credits on the new pin are gamemd's (the Chrono Miner drives out)

P68 found that Allied 5's recorded commands give 7,336 credits at tick 3,520 on the new pin (ra2-decomp ac45ee9e)
against 11,061 on the old one (8719494, main at 86bd477). Both pins were stepped side by side through two step servers
(yr/tests/campaign/all05.replay, `state` every 20 ticks, frame clock), and both reproduce those two numbers.

The game parts at the first frame, before any command: the map's two CMIN (ALL05UMD [Units] 4 and 5, Harvest at 52,93
and 52,95, the refinery GAREFN at 50,94) stand at 55,99 and 56,99 on the old pin at loop frame 0, but at 52,93 and
52,95 on the new one. On the old pin each Chrono Miner teleported out to the ore every trip (after each unload at the
dock, 51,94, it is at the ore 20 ticks later) and back when full: an unload about every 120 ticks a miner. On the new
pin it drives out (about one cell every 20 ticks, 52,95 to 55,101) and teleports back when full: an unload about every
380 ticks a miner. First credit difference: tick 480 (10,500 against 10,000, the old pin's first unload). The object
IDs part too (12 fewer by tick 60, 210 by tick 1,603: the old pin's extra chrono warp animations), so a recording's
"@^ID" points made on one pin do not name the same object on the other.

Cause: CMIN's Locomotor is the teleport one (rulesmd.ini [CMIN] Locomotor={4A582747-...}, Teleporter=yes). Upstream's
matched UnitClass::Assign_Destination (gamemd.exe 0x00741970, 6,167 B, in vendor/ra2-decomp/data/manifest.csv) has the
Teleporter block: a teleporter given any destination but an unload dock's free cell ends its teleport and rides a Drive
locomotor piggybacked on it (gamemd.exe: the Teleporter byte test at 0x007424BD, the CLSID_TeleportLocomotion compare at
0x007424E9, the CLSID_DriveLocomotion compare at 0x007425EB and the Drive locomotor made at 0x00742688). The old pin's
glue draft (yr/glue/unit_native.inc, TibSun's text, retired by P64) had no such block, so the miner kept its teleport
locomotor for the trip out. rulesmd.ini's own comment on ChronoHarvTooFarDistance ("teleport super far and drive super
far back") says the same. Do_MISSION_HARVEST's LOOKING state is the same in the draft and upstream. Verdict: gamemd's
behaviour, not a port fault. Nothing changed in fixups or glue; the Allied 5 recording must be re-recorded (P68). The
Soviet missions' miners are not Chrono Miners, so this does not explain Soviet 2 or Soviet 3.

## Lane P70 (2026-11-27): YR campaign wins on the ac45ee9e pin continued

The pending re-records first, from P68's saves and next steps, then the not-won missions with the best notes; cap 3
lane-hours a mission in all (a re-record counted fresh at 1.5 h on the new pin). Recordings and scripts in
private/campaign_saves/yr/<mission>/p70 and scripts/p70.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Soviet 2 (SOV02UMD) | won at tick 9,464; a rerun (ra2_campaign.py --game yr) wins at the same tick | 0.3 (1.8 on the new pin) | P68's try7 preloaded to 7,031 (the army at 93,123 on its way to 93,145, four V3s alive), then at once the four V3s to 108,161 and the tanks to 106,158 to guard (s2c.py: AT=7031 NCH=2): two V3s at the Chronosphere, two at Einstein's lab, any V3 out of its attack mission ordered again. Each V3 lands a rocket about every 480 ticks (the Chronosphere takes 158 a hit, the lab 315). Lab gone at 8,784, the Chronosphere at 9,224, before its transfer to BadGuy2 (9,354). What made P68's tries and P70's first two fail: BadGuy2 builds a Patriot (NASAM) at 108,173 at about 9,200, after which every V3 rocket is shot down (the Chronosphere is repaired from 215 to 750 while two V3s keep their attack mission). Patriots engaging V3 rockets is the game's design (rulesmd: the V3 rocket is an air target), so this is taken as gamemd's behaviour, not a port fault; P68's "V3s doing poorly against a second target" matches it (not checked against gamemd's code). yr/tests/campaign/sov02.replay replaced; sov02 back in yr_won_all |
| Allied 5 (ALL05UMD) | won at tick 177,682; a rerun wins at the same tick | 0.3 (1.4 on the new pin) | all05-try1.replay (P68) preloaded to 43,377 (BadGuy2 razed, 23 SREF, 13 credits, one refinery). The base manager (a5p.py: miners to 14, prism tanks; no buildings) gave about one prism tank per 1,000 ticks: 62 by 150,857. Two refineries placed by the ore at 65-68,82-87 at about 93,000 were destroyed within 4,000 ticks, and a prism tank at home was lost about every 3,700 ticks, so the queue only just keeps ahead. A finished spy satellite waits in the defense queue throughout (place_legal could not place it: the plan stalled on it until it was dropped from the plan). Then a5e.py: all 62 by 75,95 - 95,98 - 110,100 - 128,100 - 145,106 - 152,112 (42 left at the base's edge), then BadGuy1's object nearest the army in turn, buildings first, every CA* building but the airport skipped: the base gone by about 171,000, the fire sale's 57 initiates and the airport next, won at 177,682 with 43 prism tanks. yr/tests/campaign/all05.replay replaced (the rerun takes about four minutes); all05 back in yr_won_all |
| Soviet 3 (SOV03UMD) | won at tick 34,606; a rerun wins at the same tick | 0.2 (1.6 on the new pin) | sov03-try1.replay (P68) preloaded to 24,682 (Boris at 47,44, ten tanks round him). Tanks sent at the initiates (s3c.py) lose eight of nine to them (they keep coming from the barracks at 64,30) and Boris dies on arrival (try1). The sniper (SNIPE, from 59,67) ordered at each initiate in 49-61,21-34 walks north to 56,38 and kills them from about 13 cells, out of their reach: the box clear at 28,115 (s3e.py). With only that, Boris at 53,29 still died before the strike landed (try2); the gattlings at 56,29 and 59,24 (within about 6 cells of him) were the cause: the tanks killed both (29,026), Boris painted from 53,29 unhurt, four BPLN came, the tanks joined when they were within 18 cells, YAPPET gone at 30,274. Then s3m.py: every combat unit at YuriCountry's object nearest the army (the fire sale's initiates, the Slave Miners, the Yuri tanks), won at 34,606. yr/tests/campaign/sov03.replay replaced; sov03 back in yr_won_all |
| Allied 3 (ALL03UMD) | shelved | 0.6 (2.1 on the new pin) | From P68's try4 at 4,398 (16 tanks, 4 snipers, 8,200 credits; the Weather Control Device at 600/720 power: the storm never recharges once used). Power cannot be had: the civilian plants AA and AB are lost later, CAPOWRTD (110,110, Yuri's) was captured by an engineer and gave power 363/720 after AA/AB fell, CAPOWRAC (159,116) is Special-owned and cannot be captured. Tries 1-3 (a3s.py, a3t.py, a3f.py, a3h.py; 20 then 10 snipers bought, each sniper group firing at every Yuri, initiate and virus within 13 cells before moving on): each group was wiped before reaching 72,94, by Yuri's nuke (it fires about every 4,500 ticks, here at the sniper group, and its radiation kills infantry walking through) and by mind control (snipers ordered at targets they could not see walked into a Yuri's reach; two were left as Yuri's); 13 tanks alone killed the gattling at 56,89 and took the silo to 737 (repaired) before all died (lost at 61,694). Try 4 (a3n.py, preload all03-try4-nostorm-to4398.replay: P68's storm at 1,700 dropped so it stays ready; only seen targets; every tank re-teamed each leg): 22 tanks at 72,93 by 10,826, but they fell to 6 at the gattling at 56,89 without killing it, and Yuri's nuke took the base, the Weather Control Device with it, before the storm was fired (its cameo gone: "no point for @cameo=LightningStormSpecial"). Try 5 (a3z.py, from try4 at 10,826: every tank straight at the silo, the storm when they are near): 18 tanks fell to 8 between 68,94 and 60,91 in 200 ticks (the Yuris at 56,86-88), and the storm did not fire. A ready super weapon is not fired while its house is short of power (600/720 by 4,398): the cameo click and the map click go in, and the storm stays ready with charging 0. That is the game's rule (a low-power super weapon is suspended), not a port fault. Try 6 (a3e.py, a3w.py): the tanks cleared 112,106-112 (16 left 12) and two new engineers took CAPOWRTD at 6,110 (power 799/720), but Yuri's nuke at about 7,232 took plants again (564/720 by 8,000), the snipers were lost on the way (11 to 0 between 100,96 and 90,96), and the base was lost at 15,473. Next: P52's opening, where the nuke took the war factory at 2,700 and P52 won at 12,952 with 21 tanks; or the storm fired at once at 1,700 with the whole army already near the silo. Recordings in private/campaign_saves/yr/all03/p70 |
| Soviet 1 (SOV01UMD) | locks on the new pin; not won | 0.1 (P52 0.3 + P63 1.1 + 0.1) | A fresh start with P63's s1run.py (s1naval.py: the fleet at the nearest destroyer, carrier or cannon, started DELAY ticks after input opens; the navy and cannons gone by about 4,500) with DELAY 0, 100, 200, 300, 400, 500, 600, 650, 700, 800 and 1,000: every run locks (input locked at 14,000-14,450 and still at 17,052; Boris's SAPC never lands and is gone from the map; the Flak SAPC stands at 85,74). P63's DELAY 650, which unlocked on the old pin, locks here. P62's three-frame unload race (UnitClass::Do_MISSION_UNLOAD's Scen->RandomNumber(0, 2) waits against SP2.6b's recruit at local 8 + 75 frames) is the mechanism; on this pin no tried naval timing moves it to the unlocked side. Recordings in private/campaign_saves/yr/sov01/p70 |
| Soviet 4 (SOV04UMD) | not won; base economy does not hold | 0.3 (P60 0.4 + P63 0.5 + 0.3) | A fresh start (P60's try1 and P63's s4-try1 are old-pin recordings). P60's s4a.py plan (s4p.py): Yuri's first raid (five YURI, two BRUTE, then initiates) comes from 95,130 at about 7,300; the four starting Rhinos at the rally 88,128 were taken by mind control and the base was lost at 16,769 (try1). With two tesla coils on the east side (88,133 and 86,128), six dogs and conscripts, the rally inside the base (84,132) and home defence to 18 cells, the base holds to 60,260 (three refineries, seven harvesters, two war factories, four coils), but credits stay at 0-15 from about 21,000 and the Rhinos never gather: 0 at 60,260 with only four recorded as killed (the rest mind-controlled), while lost buildings are rebuilt. Next: the derricks (41,113, 51,67, 160,68) for income and the ore oasis north of the base (dossier), dogs or tesla troopers screening the Rhinos from YURI, Romanov found early by one dog at the bar (136-140,64-70) and kept at home (he is Germans', PlayerControl=yes). Recordings in private/campaign_saves/yr/sov04/p70 |

Allied 7 was not started: the dossier's plan needs the Iron Curtain and the Chronosphere fired on a 3x3 tank group (two map clicks for the Chronosphere), a navy for the Genetic Mutator, and an Allied base on a plateau reachable only by Chronosphere, all on a fresh start (P60's try2 does not preload on the new pin); more than this lane's remaining time. No crash, trap or assertion in any P70 run.

## Lane P71 (2026-11-27): Soviet 5 and Soviet 4 on the ac45ee9e pin; the sell button's tooltip fault

Fresh starts on the ac45ee9e pin; the cap is 3 lane-hours a mission in all (a mission first tried on the old pin counts
1.5 h fresh here). Recordings and scripts are in private/campaign_saves/yr/<mission>/p71 and scripts/p71.

**Port fault found and fixed (commit 8b4dea9, kept separate from the mission work):** yr-native faulted (signal 11 in
StringTable::LoadString's bsearch, from ToolTipManager::Process) when the pointer rested on the sidebar's sell button
(Soviet 5, tick 11,588, a building sold through "@sell"). The cause is vendor/ra2-decomp/src/sidebar.cpp, new on this pin.
It stores the sell and repair buttons' tooltip labels as `tooltip.Text = (int)"TXT_SELL_MODE"` /
`(int)"TXT_REPAIR_MODE"`. ToolTip::Text is intptr_t natively (lane Y3), but the cast cut the pointer. gamemd.exe 0x00724AD0
reads the label pointer at +0x14 and passes it whole to StringTable::LoadString (0x00734E60).

The fix was followed by a sweep of every `(int)`, `(unsigned)`, `(long)` and `(DWORD)` cast of a string or an address in
vendor/ra2-decomp src and include, checked against the generated units. Rows were given to:
- LAN co-op's campaign info. DecodePubGameopt 0x005E3D10 passes `(unsigned)stats` and `(unsigned)&string[2]` through
  vt_entry_5C and vt_entry_74, and MPCooperative reads them back as MPlayerScoreType * and char *. The argument is now
  uintptr_t in multiplayer.h, mpbattle.h and mpcoop.h, in their definitions and in yr/glue/mpgamemode_native.inc; the
  vtable slots are unchanged.
- The Session slot loops' pointer compares (session.cpp 0x0069BC60).
- The audio failsafe timer's high-word pointer (failsafeGetTime 0x004093F0). This one is latent: it is used only if
  QueryPerformanceFrequency fails.

Left as they are:
- wonline.cpp: the unit is left out natively.
- building.cpp, radar.cpp, tab.cpp, queue.cpp, audevent.cpp: already rewritten by earlier rows.
- FirewallHelperClass::Find_Mangled_Port's IPX channel op (0x005D9220): Westwood Online only.
- SpawnBaseUnits' unused argument.
- Tiberium_Adjust's pregame pick (0x004818E0): gamemd.exe's own arithmetic, and the pregame path is never called.
- cdcntrl.cpp's DOS ioctl registers: the CD check is skipped.
- priority.h's debug bounds.
- VectorClass::ID: a difference of two cut pointers is still right modulo 2^32.

Checks: the crash replay passes ra2_campaign.py --survive 11700; yr-native-skirmish-smoke and
yr-native-campaign-won-sov02 pass.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Soviet 5 (SOV05UMD) | not won; shelved for this lane | 0.75 (1.8 on the old pin) | Eight tries (sov05/p71/try1-8). **Dreadnoughts by sea:** a route kept 9+ cells from the 16 static Boomers (s5lib.py LEGS: the east channel, south of the six-boomer pack at 141-164,197-203, north up the bay to 129,172, 23 cells from the pen) still loses them. About 12 Boomers patrol the whole coast (mission 2), the north-east channel included: dreads were hit at 171-186,61-76 from tick 4,173, one was lost by 4,647 and all three by about 9,000. **The AL beach base:** the MCV deploys at 212,121 once the transports and the starting units are moved off the footprint; in try 1 a click on the cell where a transport stood put the MCV back in it. From a deploy at 5,200, a Yuri raid from both Yuri houses (about 25 units: Yuri, Gattling and Lasher tanks, initiates, our transports mind-controlled) comes in through the north gap (200-210,111-115) and from the south-west at 11,300-13,600. It razes the base by 15,000-17,000, with or without a sentry gun and a tesla coil. In try 2, where the base went up later and slower with two coils, it held to 37,000. The base's legal building spots are few (buildable: the war factory only at 208,117), and a war factory in the pocket at 203,113 sealed a War Miner in. **Kirovs:** one Kirov alone, flown over the sea from the south, died 6 cells short of the pen (about 2.2 HP a tick from the gattlings). Two Kirovs, ready by 14,337 after the leanest base (no defences, the radar and barracks sold), brought the pen to 1,282/1,500; the last one died at 140,153 (about 4 HP a tick). The plan needs about 4 healthy Kirovs, but Gattling tanks in the raid hit them as they leave the base. Next: hold the 13,000 raid first, as try 2 did, then four Kirovs over the sea (east, then south, then west along y 176 to 140,176, then north) |
| Soviet 4 (SOV04UMD) | not won; shelved for this lane | 0.25 (P60 0.4 + P63 0.5 + P70 0.3) | Fresh start (sov04/p71). With refineries by the east (99,131) and west (58,125) ore, three coils and seven War Miners, the base had 22 Rhinos by 44,565 (s4m.py). Two problems followed. (1) In the Moroccan town on the road north (82-102,64-100), initiates garrison the civilian houses (CAMORR05 86,67 and CAMORR08 102,81 become YuriCountry's) and the plaza's four gattlings stand at its corners. Elite initiates mauled the Rhinos: 7 of 22 died in about 500 ticks at 86-90,78-80, against 11 initiates. fight_legs does not target garrisoned houses. Given them as targets (buildings=YuriCountry, CA* first, s4n2.py), the push still died before the plaza. In the replay the same 22 Rhinos stood unhurt at 80-90,90-99 when given no order. (2) Without a home guard (Mgr defend off) the base fell to a raid from 61,800 (try 2, V3s in production). The dogs Mgr's home defence keeps buying are lost chasing raiders up to 30 cells out (about 40 dogs in try 1). Next: V3s (range 18) on the garrisoned houses and the plaza from the south (about 88,92) with Rhinos screening, a home guard of Rhinos and coils only. Then over the causeway (x 95-97, y 44-55, the only way into the north base, whose other sides are cliffs): first the gate's psychic towers (94,33 and 98,33, range 7), then Yuri's seven reactors (89,31; 104-108,33; 110-114,16-20). Yuri's power is 2,050 against a drain of about 1,100, and low power turns the towers and gattlings Neutral. Romanov (RMNV, 50 HP, no weapon) is found last and carried in a flak track |

Allied 7 was not started this lane. No other crash, trap or assertion came up in any P71 run.

## Lane P72 (2026-11-27): Allied 7, Soviet 6 and Soviet 7 on the ac45ee9e pin

Fresh starts; the cap is 3 lane-hours a mission in all. Recordings are in private/campaign_saves/yr/<mission>/p72 and the
scripts in scripts/p72. There was no crash, trap or assertion in any P72 run.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 7 (ALL07SMD) | not won; shelved for this lane | 0.25 (P60 1.0) | Three tries (a7o.py, a7r.py, a7s.py). The Soviet base (84,125) has no ground path to Yuri's base (150-220,90-190); the Allied plateau (105,192) has one. So "no radar, mass Rhinos" is out, and the win (Win_Check: destroyed-units-all of YuriCountry and Yuri2; vehicles and infantry only, TEVENT_UNITS_DESTROYED) has to come from the plateau or by Chronosphere. Radar at 6,285: the MCV landed at 8,613 and deployed at 105,194. The Psychic Dominator (YAPPET) stands at 222,145 and the Genetic Mutator (YAGNTC) at 169,109. The Dominator first fired at 21,967, about 13,350 ticks after the MCV, on the Soviet base (six buildings round 85,129 lost at once; the NACNST lived); its next shot was due at about 35,400. On the plateau the Allied War Factory needs GAPILE, and the Allied radar is AMRADR (GAAIRC is not on the sidebar). Credits sat at 0 from about 17,000 while Chrono Miners were killed on the gold band east of the plateau (117-129,191-204), and both bases fell by 25,000-30,000. A super weapon fires by its cameo and a map click: SpyPlaneSpecial was fired that way (tab 1). Next: two refineries and a home army before the radar; miners kept off the east band, with prism towers there; NATECH and the Iron Curtain at once after the radar, so the Dominator is killed or survived before MCV+13,350 |
| Soviet 6 (SOV06UMD, the moon) | not won; needs a video study | 0.75 | The win: Fire Sale A, B and C (Americans, Alliance and YuriCountry each without their tech types 300-303, plus 354 for Yuri), then 60 s to the fire sale and 8 s to winner-is. 70,000 credits and no ore. The MCV cannot deploy where it lands (118,145) or at 118-126,128-132 ("cannot deploy here"). It deploys at 134,126; the dossier's "go east" matches this. I suspected a port fault (the lunar WaterSet is ISOTILE_INVALID, so Is_Tile_Water is true for ITType -1..12), but gamemd does the same: 0x00546C95 sets WaterSet to -1 on lunar, and 0x00485060 makes the same signed compare. Not a port fault. A base of power plants, two war factories, the battle lab and teslas (s6b.py), Apocalypses after it. Gattling tanks (YTNK) raid from the north (140,110) and cosmonauts from the west (110-125,115-125) without pause: the teslas and power plants on the west side fall, and the base is lost at 40,553 (try 1, 14 Apocalypses sent at the Americans' War Factory at 79,102, lost to the psychic towers by 89,108) and at 22,345 (try 2 from 10,000, home guard only). Next: a video study for the base's site and defence |
| Soviet 7 (SOV07TMD) | not won; started | 0.2 | The win: destroying the object tagged "Yuri's Castle" (05BC8BAC). That is CATRAN03 at 65,67 (BadGuy1, strength 256), inside BadGuy1's base (brutes, initiates, tank bunkers at 50-90,50-90). It is not CASLAB at 172,99: the starting army (2 TTNK, 2 HTNK, 2 HTK, infantry) walked east to it unopposed and razed it by 2,680, and nothing followed. "Castle Attacked" (attacked-by-any) sends five teams and a nuke siren. Next: a base at 140-150,140-150 and V3s or Kirovs on 65,67 |

## Lane P73 (2026-11-27): the trigger decoder's event and action numbers; YR campaign wins continued

**ra2/tools/ra2_mapinfo.py's tables were wrong past event 52 and action 105** (found by the Soviet 6 video study). The
tables are now read from the executables' own dispatch: the jump tables of Event_Needs, TEventClass::operator() and
TActionClass::operator() (game.exe 1.006: 0x006E6970, 0x006E6050, 0x006ADAE0; gamemd.exe 1.001: 0x0071F5B0, 0x0071E940,
0x006DD8B0), with the case groups matched to vendor/ra2-decomp's tevent.cpp and taction.cpp (YR's 143 action targets
fall in the decompilation's case order exactly; game.exe's 115 fall in the same order for cases 1-117). Both games number
the shared entries alike; game.exe stops at event 57 and action 117, gamemd.exe at 61 and 145 (`--game ra2|yr` prints
numbers past the end as eventN/actionN). Events that were misnamed: 53 spy entering as a house, 54 spy entering as an
infantry type, 55 every naval unit destroyed, 56 every land unit destroyed (vehicles not on water, and infantry), 57
building type N does not exist (the trigger's house, ABQuantity 0), 58 power full, 59 entered or overflown by, 60
TechType exists (count >= N), 61 TechType does not exist. Actions 106-145 were shifted (106 set tech level, 107 chrono
warp team, 108 create crate, 109 Iron Curtain at, 110 pause, 111 evict occupants, 112 centre camera at ... 145 jump to base);
the waypoint notes now follow the handlers (the location parameter for 26 actions, the data parameter for 9). The event
parser takes the fourth field when the parameter code is 2, as TEventClass::Read_INI does, not by event number.
tests/mapinfo_tables.py (ctest mapinfo-tables, label ra2-quick) keeps them. The YR dumps in
private/campaign_saves/yr/scripts/maps/*.trig are regenerated (`--game yr`); no Red Alert 2 dumps were kept.

Mission notes whose win or lose reading changes under the fixed table:
- **YR Soviet 6** (P72): the Fire Sale triggers are "building does not exist" for YACNST, YAPOWR, YABRCK, YAWEAP (indices
  300-303) in each of the three enemy houses, and YACOMD (354, the Lunar Command) for Yuri. P72 read them as "without"
  already; the dump had said tech-type-exists.
- **YR Allied 6** (P60): Conditions_Win is every land unit of YuriCountry destroyed (event 56), not "destroyed-unit-type 9".
- **YR Soviet 5** (P60/P71): "Detect_Yuri Land Dead", on the _Winner chain (Completed_Objective 3), is every land unit of
  houses 9 and 14 destroyed (56), not destroyed-unit-type; "the pen alone wins on Medium" needs a re-read against it.
- **YR Soviet 1** (P52/P63/P70): SP3.1_Detect_Full Power, on both the win and lose chains, is house 8's power full (58),
  and Detect_Dominator Active is house 6's power full; the dump had said tech-type-does-not-exist.
- **YR Allied 7, Soviet 4, Allied 1** (lose on NACNST, Romanov killed, the time machine and power plants): the dump said
  tech-type-exists-n, but the notes already read "gone" / "killed", which is right (61).
- **YR Soviet 2** (won): the Chronosphere (GACSPH, 27) is "does not exist", as P60 read it.
- **Red Alert 2 Allied 12** (P37, P38: "three tech types to exist"): event 57 there is "building does not exist" for
  NALASR, NAFLAK and TESLA; P39 already corrected this from the code.
- No change on the win or lose chains of YR Soviet 7 (the CATRAN03 "Yuri's Castle" tag), Allied 4, Allied 2, Allied 3,
  Allied 5 and Soviet 3: only action names changed there.

The missions (cap 3 lane-hours each, cumulative). Recordings in private/campaign_saves/yr/<mission>/p73, scripts in
scripts/p73 (q.py <work> <script> <until> <save>; start.sh).

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Soviet 6 (SOV06UMD, the moon) | won at tick 222,711; a rerun (ra2_campaign.py --game yr) wins at the same tick | 0.5 (P72 0.75) | The dossier's plan (s6a.py): the MCV moved to 134,126 and deployed; three Tesla Reactors, barracks, war factory, radar, battle lab, industrial plant; cosmonauts to 41, then Apocalypses. 26 cosmonauts razed the Americans' base by 22,278 (its one Gattling Cannon first). The Apocalypses were lost three times at the Alliance base, sent one by one or through its middle (try 1-3: a 24-tank group walking from the south-west into the tower's and four cannons' fire lost 13 in 1,000 ticks; the psychic tower takes three). What worked (try 4, s6e.py, from try 3 at 48,700): the whole group staged at 70,72, then a fixed order from the south edge: cannons 81,64 and 84,66, the psychic tower 83,57 and the reactors; 12 of 16 left. Try 5 (s6f.py, from try 4 at 57,420): the two tech centres and the Alliance's last cosmonauts for the bonus crates (3,839 credits), two more reactors at home, then 15 Apocalypses and 17 cosmonauts staged at 70,40 and sent at Yuri from the north-east, Masterminds within 10 and psychic towers within 8 first: the Lunar Command fell at about 68,100; Yuri's power went low after two reactors (1,800 for a drain of 1,635); his construction yard kept rebuilding reactors (135,000 credits) until it was attacked again at 220,400 (the script had skipped it after 3,000 ticks), then the last two reactors; the fire sale and the win followed. yr/tests/campaign/sov06.replay; sov06 in yr_won_all. No crash, trap or assertion |
| Soviet 7 (SOV07TMD) | not won; shelved for this lane | 0.6 (P72 0.2) | The win under the fixed decoder is unchanged: "_Win (Bldgs KIA)" springs from the "Yuri's Castle" tag, i.e. CATRAN03 at 65,67 (BadGuy1, 5,000 strength, concrete). Weapons against concrete: APOC 70% (140 a double shot), Kirov 50%, nuke 8% (48: useless), demolition truck 10%. BadGuy1's base has seven psychic towers and seven gattling cannons, with power 6,050 for a drain of 1,635, so cutting its power is out. The map's other facts: destroying a psychic beacon frees that house ("Allies Freed"/"Soviets Freed": all-change-house). The Soviet beacon NAPSYB is at 96,43, inside two tesla coils, two flak cannons and two sentry guns; BadGuy2's power is 2,750 for 1,055, most of it the nuclear reactor at 86,44. The Allied beacon is at 28,101. The freedom fighters at 187,110 unlock TERROR and DTRUCK when discovered. Raids: 5 E2 + dog + 2 FLAKT (BadGuy2), dog + 5 E1 + 2 GGI (BadGuy3), 5 initiates + Brute (BadGuy1) from 30 s, repeating. Try 1 (s7a.py): the base at the landing spot (150,149) with 10,000 credits, two refineries and War Miners, 14 APOCs by 30,000, then lost building by building (the battle lab at 30,632), so APOCs stopped. Try 2 (s7b.py: sentry guns, coils, lost key buildings re-queued): 37 APOCs by 120,000, but a 35-unit selection made team 9 of one unit, so the army never left (s7c.py moved it in groups of six). At about 122,200 the base and the first group (at 102-111,101-110) were destroyed within about 500 ticks; the cause was not identified (no enemy super weapon on the state line). Of 35 APOCs, 9 were left by 167,000 and the castle was untouched. Next: free the Soviets first (the beacon's cluster from the north back door, wp53 107,35, or the reactor at 86,44 to cut their power), then the castle from their base with APOCs, towers and Masterminds first. Recordings in private/campaign_saves/yr/sov07/p73. No crash, trap or assertion |

Allied 4 was not started this lane. Its win reading is unchanged under the fixed decoder.

## Lane P75 (2026-11-27): Soviet 7's two oddities; Soviet 7 won; Soviet 1's lock re-read

**The 35-unit team of one was the harness's.** Replaying P73's try2.replay to 121,580 and feeding the rest through the
live step server reproduced it: after Ctrl+9 at 122,050 one unit was selected, and game.log had 44 "no point" lines.
s7c.py selected the 35 Apocalypses from the object list it had read at 120,881, after ordering them off in groups of
six: each click's look and "@^ID" went where the list put the unit, the unit was outside the view by then, and the step
server dropped the click (lane P56/P65's rule). gamemd.exe's team create (CreateTeamCommandClass::Execute 0x00535CC0 ->
0x00731060; vendor/ra2-decomp uicmnds.cpp FUN_00731060, compared instruction by instruction) gives the group to every
selected unit of the player's and clears the old members; no cap. Fixed in ra2/tools/ra2_commander.py `Game.select`:
each click aimed from a fresh object list, Shift-clicks only on units not yet selected (a Shift-click on a selected one
drops it), the selection checked and the missed ones clicked again (three rounds); it takes Obj or IDs and returns those
left unselected. tests/commander_select.py (ctest commander-select). Also found: `Mgr(defend=True)` defends home with
`select_all_combat` (P, then P again for the whole map), which takes every combatant, the army away included, so a
script that keeps calling it while its army is out calls the army home; try 2's army "never left" partly for this.
Scripts with an army out should pass defend=False and defend with a home team (scripts/p75/s7p.py home_guard).

**The collapse at about 122,100 was BadGuy2's Kirovs, not a super weapon or a trigger.** The construction yard and a
refinery were lost in the same tick (122,099-100), then the power plants, barracks, war factory and the rest; the
`teams` order showed BadGuy2's AI team "SKirov_M+H" (ZEP) over the base at 149,151, and two Kirovs were shot down there
(122,650 and 123,104; their wrecks killed Apocalypses under them). The base had no anti-air (no flak cannons), and with the
selection fault about 30 Apocalypses were still at home. The first group at 102-111,101-110 died to the Temp house's
"Combine Shock_IFV" set-piece IFVs and BadGuy1's slaves and infantry. The state line does list the AI houses' super
weapons: only PsychicReveal, SpyPlane and ForceShield were there; no Dominator, Mutator, weather device or silo
existed, and "Castle Attacked" had not sprung. From the map: building an Iron Curtain (24) or a nuclear silo (54) sets
local 0, which on Medium has BadGuy1 build a Psychic Dominator at wp60 (58,85) 445 s later and then a Genetic
Mutator, BadGuy2 a nuclear silo and BadGuy3 a weather device; P73 and P75 built neither. Real game behaviour.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Soviet 7 (SOV07TMD) | won at tick 128,891; a rerun (ra2_campaign.py --game yr) wins at the same tick | 0.3 (P72 0.2, P73 0.6) | From P73's try2.replay at 42,520 (20 APOCs, base intact; preload about 2 minutes). Try 1 (s7p.py): the two Kirovs shot down at home, 16 APOCs as team 9 (16 of 16 selected) by fight_legs along the road north; worn down leg by leg, the last died at 104,57 by 57,122. Try 2 (s7q.py, ARMYN=30 WANT=32): two more power plants first (395/485 had slowed production), two flak cannons, a third refinery, six War Miners, APOCs to 31 by 108,000 with the home team defending; 30 APOCs along 138,107 - 127,95 - 116,84 - 107,76 - 105,66 - 104,57 - 101,49 (two lost) and force fire on the Soviet Psychic Beacon (NAPSYB 96,43, BadGuy1's): gone at 122,132, Soviets Freed, BadGuy2's 37 buildings the player's (44 buildings, power 3,544). s7r.py: 1,500 ticks' rest, then 28 APOCs west by 91,49 - 83,55 - 75,56 - 67,60 (62,56 has no path; the psychic tower 80,54, NATBNK 82,52 and the gattlings first) and 16 left at the castle: CATRAN03 (67,69) gone at 128,658, then _Win (Bldgs KIA), the cheer and winner-is. yr/tests/campaign/sov07.replay; sov07 in yr_won_all. Recordings in private/campaign_saves/yr/sov07/p75, scripts in scripts/p75. No crash, trap or assertion |
| Soviet 1 (SOV01UMD) | the lock is not the power condition (read from the dump; not run) | 0.1 | Under the fixed decoder the lock's chain (SP2.3_Sov Landing ... SP2.7_Transport 4, SP2.14 on local 9, SP2.15/16, SP2.10_BorisIntro, SP2.11 on local 23, SP2.12/12a, SP2.13_Unlock) uses only elapsed-time, destroyed-by-anything and local-set, which the fix did not change. SP3.1_Detect_Full Power (power-full, house 8) is enabled only by "Audio_Time Machine Off" (after the time machine's defences fall) or "Detect_Player Has Time" (building 314 exists), both after the lock, and it is disabled on the lose chain by the time machine's loss. Input stays locked before power can matter, so P62's three-frame unload race (Boris's SAPC recruited by "TM_Sov_1 Trans (Flak)") is still the mechanism. No port fault found |
| Allied 4 (ALL04DMD, Tomb Raided) | not won; shelved for this lane | 0.8 (P52 0.5) | Fresh starts, nine tries (scripts/p75/a4*.py; recordings in all04/p75). **The win route, read again from the dump:** destroying all eight Arabs power plants with the Dominator (YAPPET 84,94) standing runs EinsRescue1: local 3, and "Change House" (local 3) gives the Dominator to the player (super ready at once; "New Timer for PD" sets its recharge, "Countdown to PD Gone" 302 s, then "Destroy This"); "It's OVER!" is destroyed-all of YuriCountry plus local 3. What works: two Harriers force-fired at the oil tank CAMISC02 (84,108) open the south wall (Oil Explodes!); Tanya walked 84,128 - 84,113 - 84,109 and given attack orders (initiates within 4 first) C4s the four east plants and then the four west plants (from the fourth on the compound's power is 600/900 and its gattlings and towers are dark): all eight by 22,064 (try 1) and 30,115 (try 3), EinsRescue1, input back about 500 ticks later; the Dominator, fired by its cameo (tab 1) and a click at 140,43, destroyed four of Yuri's 40 buildings (its recharge, 8,597, outlasts the self-destruct timer: one shot). The Russians' oil guards (two lasher tanks, four initiates) cannot fire at aircraft: three Harriers kill them from about 19,500 to 27,500 without loss, and the two starting engineers flown in the starting Nighthawk (board: the engineers ordered onto it; unload: move, then D) take two derricks (try 7, 29,251). Dogs kept as their own team against infantry near home (scripts/p75/a4lib.py) stop Yuri's engineers, which otherwise capture the base building by building (try 8: a GAPOWR YuriCountry in the base at 24,300). What fails: the economy. The ore near the base is a few cells (48-53,157-164 and 79-81,180-182); credits are near 0 from about 6,500 in every try; Yuri raids from the start (gattling tanks, Yuri engineers and Yuri from the west at 65-72,150-155 and the north at 79-100,142-148; lasher tanks, mind control) and the base fell at 15,000-31,000 in six tries and at 56,000-82,000 in two; the derricks were lost by 34,000-44,000; no try had more than one robot tank. **Harness fault found and fixed** (commit 762943d): place_legal clicked GAROBO (2x2, missing from SIZE) a cell off, so the finished building held the build queue from 22,452 on (try 7); YR's buildable now reports the footprint. Resume plan: try 7's opening (scripts/p75/a4i.py: Airforce Command third, Harriers at the oil guards, the Nighthawk airlift) with try 9's dog team, then three engineers more by air and Harriers held over the derricks; the Tanya op only once 15-20 robot tanks exist (the Arabs' Dominator never fired in any try, so waiting costs nothing); after the Dominator's shot, sell it for credits before its timer; then robots and prism tanks at Yuri's base (129-155,29-56: 13 gattlings, 7 psychic towers, 11 Bio Reactors; robots are immune to mind control). A video study of a Medium play would help most |

**Placement (harness).** ra2_commander.py's place_legal aimed its click from its SIZE table, 3x3 for a type it lacked:
GAROBO (2x2) was missing and GATECH stood as 2x3 for a 3x2 footprint. yr/glue/yr_stepserver.cpp's `buildable` now ends
"buildable <type> <n> legal size WxH" (the occupy list's extent, Get_Occupy_Dimensions' measure) and place_legal uses it
before SIZE (Red Alert 2's step server is unchanged, so RA2 keeps SIZE). tests/commander_place.py (ctest
commander-place).

No crash, trap or assertion in any P75 run. Checks: commander-select, commander-place, mapinfo-tables, repo-hygiene and
yr-native-campaign-won-sov07 pass (after the step server change too).

## Lane P76 (2026-11-27): YR Allied 4 won by the oil-first plan; Allied 7, Soviet 4 and Allied 3 tried

Fresh starts on the ac45ee9e pin; the cap is 3 lane-hours a mission in all. Recordings are in
private/campaign_saves/yr/<mission>/p76 and the scripts in scripts/p76 (start.sh, q.py; a4lib.py, a4o.py ... a4x.py).

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 4 (ALL04DMD, Tomb Raided) | won at tick 122,158; a rerun (ra2_campaign.py --game yr) wins at the same tick | 0.6 (P52 0.5, P75 0.8) | The video study's plan. **Oil first:** at input open (1,920) Tanya and both engineers board the Nighthawk and fly to the oil compound's south gap (160,104; 168,106 is too near the map's edge for the view to centre, "outside the tactical view"); Tanya kills the Russians' two lasher tanks and four initiates (Oil Field Free at 4,232); one engineer takes the derrick at 162,101, the other dies. The derrick gives build room: a barracks placed beside it (163,98) and made primary (a click on it while selected) trains engineers there, who take the other four derricks by 12,577 (10,030 credits at 13,235, about 3.9 a tick idle). Then the Airforce Command, a war factory in the compound (made primary), prism towers on its north side and at home, robot control, battle lab. **Einstein:** robots alone (11 hover tanks over the lake into the compound) die to the initiates and gattlings without a plant (try 1). Tanya must be kept out of fights (in try 1 the oil guard team took her to Yuri's base, dead at 147,60): she boards the Nighthawk at the oil field and waits in it at home (91,167); two Harriers force-fired at the oil tank CAMISC02 open the south wall (Oil Explodes!, 15,968); a drop at 84,112 by the Nighthawk did not unload (D after arrival left her inside), so she is unloaded at home and walked, swimming, by P75's route 84,140 - 84,128 - 84,113 - 84,109; she C4s the four east plants, then the four west ones (initiates within 4 first): EinsRescue1 at 22,072, the Dominator the player's. It fired at 140,43 by its cameo and a map click with 5-tick pauses between the clicks (P75's a4dom.py; the same clicks with no pause did not fire): 40 to 36 Yuri buildings. Selling it (the sell button, then a click) did nothing; it self-destructs at 27,380. **Yuri's base:** 16 prism tanks by 48,650 with power, a weather control device and an ore purifier; the army gathers at 160,74 and goes in over the north low bridge with a lightning storm each recharge on the densest reactors and towers. From about 42,000 to 84,000 the prism tanks stood at the bridge's east end (169,66, mission Move, no progress) while robots crossed alone and died: the bridge's west end was down (LOBRDG22/09/27 and a row of LOBRDG08, the cells' land water; Bridge Detect had sent Yuri's repair engineers, which the oil guard killed as "engineers first"). An engineer ordered into the east hut (CABHUT 169,70) got no order (InfantryClass::What_Action gives ACTION_GREPAIR only when Map.Can_Repair_Bridge(the hut's cell); not compared with gamemd.exe, taken as the game's rule); flown by the Nighthawk to 152,67 and sent into the west hut (154,66) it repaired the bridge (LOBRDG21 again, 83,697). Then 60 prism tanks: Yuri down to 17 buildings by 108,600 while he rebuilt reactors from 65,000 credits; a4x.py sent every prism tank away from home at the construction yard first (YACNST 147,37), then the nearest YuriCountry object by priority, idle tanks re-ordered, Harriers at far units: Yuri Force Sell at 116,792, Yuri's Gone 117,792, the Slave Miners hunted down, It's OVER! and the win at 122,158 (60 prism tanks). yr/tests/campaign/all04.replay; all04 in yr_won_all. No crash, trap or assertion |
| Allied 7 (ALL07SMD, Brain Dead) | not won; shelved for this lane | 0.15 (P60 1.0, P72 0.25) | One fresh try (a7a.py, a7b.py; all07/p76/try1.replay). The dossier's opening: engineers into the Soviet construction yard, the power plant, CATECH01, the derrick and CAMACH; sentry guns, flak and coils at both gates, two refineries; the radar queued at 5,400 and built at 8,424; the Allied MCV chronoed in at 10,449 and deployed at 105,194 (Build_Yuri_SW: the Mutator at 168,108 and the Dominator at 221,144 appear then). From the dump: building the Soviet barracks (type 11) sets locals 0 and 4, which start the Arabs' barracks raids; the Soviet war factory (14) sets locals 1 and 10, which end them and start the war factory raids; a Chronosphere (27) sends Yuri2's "Irritate_Player" team. Building an Iron Curtain or silo starts nothing here (unlike Soviet 7). On Medium a lost Allied construction yard brings a second MCV (Allied_MCV_Gone(Med), New_Countdown, New_MCV: it landed at 17,482). The plateau has a ground path into Yuri's base (105,192 - 137,157 - 152,139 - 166,121 - 221,144, about 128 cells). The Dominator fired at 23,998 (MCV + 13,550) and 37,534 on the plateau (refinery and four power plants each time). The Mutator fired at 38,006. The Soviet construction yard fell to raids at 31,042. By 40,000 there were 7 prism tanks, credits near 0 and both bases half gone. Next: the dossier's curtained chrono strike on the Dominator before MCV + 13,500 needs a Soviet battle lab and Iron Curtain and an Allied battle lab and Chronosphere by about 20,000, which this economy (two Soviet refineries, the gold band at 117-129,191-204) did not pay for |
| Soviet 4 (SOV04UMD, Romanov on the Run) | not won; shelved for this lane (nearly: It's Over! sprang) | 0.75 (P60 0.4, P63 0.5, P70 0.3, P71 0.25) | Eleven tries, all from P71's try1-rhinos22.replay (it preloads on this pin: 22 Rhinos at 44,565; s4a.py takes the derrick at 42,114 and builds six V3s to 63,766, sov04/p76/try1.replay). **Through the plaza** (s4b.py: V3s on the Yuri-held houses from 88,92): all 18 Rhinos and 6 V3s lost at 86-96,77-88 by 71,200. **By the east road** (s4c.py: 100,110 - 116,92 - 126,74 - 120,54, safe with no losses) the junction south of the causeway is the trap: the Yuri-held houses CAMORR07 (99,60) and CAMORR08 (93,60), the barracks 97,64 and initiates kill everything at 95-104,55-58. From a stand-off at 105-111,52-55 the V3s killed CAMORR07 but never damaged CAMORR08, whether attacked or force-fired, and wandered to 111,60 and died (s4d.py, tries 4-6). Kirovs are not on this mission's sidebar, even with a battle lab (APOC is). **Romanov in a Flak Track under the Iron Curtain (try 8, s4s.py):** a Flak Track (HTK) alone by the east road to the bar (139,69, Romanov Found! at 75,223); Romanov (Germans, PlayerControl) boards it (a click on the Flak Track), and it goes back home. The Iron Curtain unlocks (NAIRON, built at 92,164) and charges. The Flak Track goes 100,110 - 116,92 - 126,74 - 124,54 - 106,56, is curtained there (98,354), then 97,50 - 96,44 - 93,36 - 96,29 - 101,19 untouched, and Romanov is unloaded in the ring. **It's Over! sprang at 99,226**, but Romanov Killed (no RMNV; It's Over! does not disarm it) lost the mission at 99,346. He died within about 30 ticks of the unload, inside the Arabs' gattlings (100,21, 106,21, 106,15). The evacuation Nighthawk (team 089D5EFC, empty SHAD from wp2 95,10 to wp4 103,20, then local 29) had not arrived. Read from the code: destroy-team (TAction_DESTROY_TEAM 0x006E1F90 -> TeamTypeClass::Destroy_All_Of 0x006F0A70 -> ~TeamClass) removes Romanov from his team, it does not kill him; the "Romanov Captured" trigger-gone at 99,241 is the team's tag being deleted. Game behaviour, not a port fault. The curtain covers the 3x3 cells round its cell (SuperClass::Place, SUPER_IRON_CURTAIN) and kills infantry (InfantryClass::IronCurtain: C4 damage of its full strength), so Romanov cannot be curtained, and a curtained unit cannot be mind-controlled (CaptureManagerClass::CanCapture). Tries 9-11 curtained eight Rhinos with the Flak Track to clear the airport's gattlings or Yuri's reactor row, but the group bunched over 4x4 cells: only 3-5 were covered, the uncovered ones died at the junction, and the slower group outlasted the curtain (about 750 ticks). Next: from try8-almost.replay --until 96600 (Romanov aboard at home, the curtain about to be ready), kill or neutralise the three airport gattlings before the unload (Yuri's power below his drain turns them and the Africans' towers Neutral: Power Low), or unload only when the evacuation Nighthawk is close |
| Allied 3 (ALL03UMD, Power Play) | not won | 0.1 (P52 0.3, P68 1.5, P70 0.6) | One try (a3a.py, a3n.py: P70's party along y 96 with the storm held until power >= drain). There is no construction yard in this mission (no tab 0), so power plants cannot be built: P76's idea of spare plants against the nuke does not exist. 19 tanks reached 72,93 by 8,681, then everything was lost by 11,129 (PlayerLose, destroyed-all). Next: P70's list (capture the Yuri-held tech plants for power, or the storm at once at 1,700 with the army already by the silo) |

No crash, trap or assertion in any P76 run. One observation, not compared with gamemd.exe: in Allied 4 an engineer
ordered into the north bridge's east repair hut (CABHUT 169,70) got no order while the bridge's west end was down;
InfantryClass::What_Action gives the repair action only when Map.Can_Repair_Bridge(the hut's cell) holds, so this is
taken as the game's rule. The west hut (154,66) repaired it.

## Lane P74 (2026-11-27): the port's remaining own drafts against gamemd.exe

The drafts P64 kept (upstream ra2-decomp ac45ee9e lacks the function, or only declares it), compared with gamemd.exe the way
lanes P57 and P61 did: each one disassembled with yr/tools/yr_dis.py (capstone; names from vendor/ra2-decomp's manifest),
its control flow followed step by step against the glue's text, member offsets checked against upstream's headers and
vtable slots against the calls upstream's matched code makes. Fixes are marked "YR (P74)" in the glue; nothing in vendor/.

| Function (gamemd.exe) | Glue | Result |
| --- | --- | --- |
| BuildingClass::Exit_Object (0x00443C60, 6,759 B) | building | **fixed in the lane's last commit**, which lands with the re-recorded wins (see "Exit_Object" below) |
| BuildingClass::ForceCreate (0x00458A80, 809 B) | building | matches (overlay cleared per foundation cell, occupiers lifted and put back, Nearby_Location with the defaults) |
| WalkLocomotionClass::Movement_AI (0x0075AEC0, 4,988 B) | walk | **fixed**: two steps, see below; the rest (path delay, blocked and moving-block paths, tubes, gates, destroyable blockers, the cloak shimmer, the cell crossing, bridges, veins) the same |
| Tactical::Render (0x006D3D10, 3,643 B) | tactical | **fixed**: five foreground steps the draft left out, see below; the background pass (pan, wipe, the eight Render_ layers, dirty areas, blit) and the timers the same |
| Tactical::Draw_Tiles(Cell, Rect) (0x006D7F20, 824 B) | tactical | **fixed**: the elevation test, see below |
| MapClass::Set_Map_Dimensions (0x00565C10, 5,374 B) | map | matches on the path every caller takes (reset_cells true: scenario load, the random map, the display and mouse resizes); the copy-and-shift path (reset_cells false) is reached by no caller and was not compared |
| MapClass::Is_Same_Cell_Zone (0x0056D100, 290 B) | map_zones | matches (In_Local_Radar(cell, true), the inline In_Radar, the leave-map case, Get_Cell_Zone) |
| CellClass::FindInfantrySubposition / Closest_Free_Spot (0x00481180, 795 B) | cell | matches (the spot tables 0x0081CC84/0x0081CC98, the gate test, Scen's random alternate, Get_Height_GL and the bridge height); the cell origin is rounded down by masking in gamemd.exe and by division in Coord_Whole, the same for map coordinates (never negative) |
| FUN_0064BDD0 Extract_Compressed_Events (1,455 B) | queue | matches (FRAMESYNC/FRAMEINFO/MEGAMISSION/RESPONSE_TIME/ADDPLAYER, the unit-list cache by house and slot, the debug lines); the port's bounds checks on malformed packets are kept (no effect on a well-formed packet) |
| OwnerDraw::ResetControlDrawModeAndTimerProc (0x0060A5B0, 1,184 B) | ownrdraw | matches (typed text, special and animated controls, the timers), with Get_Static_Drawer (0x006035F0) and FUN_00603870's tables, which the glue repeats natively |
| FUN_0060B950 (1,895 B) | ownrdraw | matches (every dialog and control table, the 0x103 typed lines' render-height nudge) |
| MultiplayerGameMode's base virtuals (vtable 0x007EED60) | mpgamemode | match (all 24 slots: their constants and argument sizes) |
| MapGeneratorClass::Generate_Random_Map (0x00598960, 3,018 B) | mapgen | matches in its call sequence (progress, previews, regions, starting points, tiberium, hills, vegetation, cleanup); N8's per-step notes stand |
| XSurface::Plot_Line (0x007BAB90, 511 B) | xsurface | matches |
| MapClass::Update_Cell_Zone, Cell_Threat, Place_Down, Pick_Up | map_zones | the same as what gamemd.exe calls at their call sites (ResetZones 0x0056D460, GetThreatPosed 0x0056BCD0, AddContentAt/RemoveContentAt for foot objects); HouseClass::Can_Build_Here the same as FUN_0050B760 |
| OwnerDraw::DefaultHandler (0x00610CA0, 7,265 B), ComboBoxHandler (0x00617250, 6,895 B), ListBoxHandler (0x00618D40, 14,666 B) | ownrdraw, _combo, _list | **not compared in full** (time box): DefaultHandler's opening (IME, SubclassProcs, WM_SYSKEYUP Tab to MainWindow, the statics, ActiveWindows' filter) checked; the combo and list handlers keep their items in native records (P66), so a step comparison would be against a different storage; the shell-frames, save-dialog, choose-map and interface-size checks play them |
| Draw_Shroud_Or_Fog_Shape, isotype Draw_Tile, DSurface::Draw_Depth_Antialiased_Line, XSurface::Draw_Ellipse, Drawing::Intersect, Attaches_To, All_To_Look, Set_Cursor_Shape, Is_Route_Broken | various | not compared this lane (written from gamemd.exe by lanes Y8/N3 or drawing only) |

**Movement_AI** (Walk, the infantry locomotor):
* When pathing fails and the walker gives up, its target is dropped if the target's destination is not in the walker's zone;
  gamemd.exe asks both destinations with the walker as the docker (TarCom->Destination_Coord(LinkedTo),
  LinkedTo->Destination_Coord(LinkedTo), Is_Moving_Onto_Bridge(LinkedTo); 0x0075B1A2-0x0075B2B6). TibSun asked with none
  (a helipad's, repair depot's or bunker's docking point differs from its centre).
* A walker blocked by a friend (MOVE_TEMP) close to its destination stops there only when it has no radio link at all
  (RadioClass::HasAnyLink, 0x0075B773); TibSun tested the first link only.

**Render** (the foreground pass, 0x006D45F2-0x006D4B37; the helpers are matched upstream since the ac45ee9e pin):
* Draw_Radial_Indicators after the beams (0x006D467F);
* for every techno not the player's, a powered Psychic Sensor of the player's covering its target or destination
  (FUN_0043B150) draws its intent line (FootClass::FUN_004DC340), in planning mode too (0x006D4764);
* the airstrike designator's laser onto the building it paints (0x006D47FB: TechnoClass::FUN_00705860 from its fire
  coordinate to a point circling the building's centre, timeGetTime() >> DAT_00842900, 15 leptons);
* in planning mode the planned nodes (FUN_0063B2F0, 0x006D491A); Draw_Sparkles over the view (0x006D492B).
  The loop order is gamemd.exe's: the player's technos skip the rest in planning mode, others after the intent line.

**Draw_Tiles** (0x006D8078-0x006D80C9): a raised cell below the current level is drawn when its elevation reaches it; the
side columns compare height - level and the middle one height - level + 1. TibSun's text compared one less in both, so a
ramp's overhang one level up could be left undrawn (or drawn one level early).

**Checks.** ctest `yr-native-render-steps` (yr/tests/yr_render_steps.py; harness orders `airstrike <ID>` and
`rendercounts`): Boris ordered onto an enemy power plant paints it (his airstrike's target is GAPOWR) and the foreground pass
draws his laser; enemy Rhinos near the player's powered Psychic Sensor get their intent lines drawn (the draft drew neither).
The yr-native suite (46 tests without the won replays and the Wine, ubsan and asan labels) passes with all four P74 fixes
in (yr-native-factory-exits after its hospital placement fix); all six YR won replays (all01, all05, sov02, sov03, sov06,
sov07) win at their ticks with the Walk, Render and Draw_Tiles fixes; ra2-quick not run (no RA2 code changed); check_tree is
clean. Calibration used: no.

**gamemd.exe addresses used** (every fix): Movement_AI 0x0075B1A2-0x0075B2B6 (Destination_Coord(LinkedTo) on TarCom,
0x0075B216; on the walker, 0x0075B214; Is_Moving_Onto_Bridge(LinkedTo), 0x0075B27F) and 0x0075B773 (HasAnyLink);
Render 0x006D467F (Draw_Radial_Indicators), 0x006D4764-0x006D478E (FUN_0043B150, the Is_Foot and IsInLimbo tests,
FUN_004DC340), 0x006D47FB-0x006D48F1 (the airstrike laser: Airstrike at TechnoClass+0x294, Owner +0x4C, Target +0x50,
DAT_00842900 = 7, 15.0 at 0x007ECE60, 0.37 at 0x007F43B0, Fire_Coord vtable +0xB0, FUN_00705860), 0x006D491A (FUN_0063B2F0),
0x006D492B (Draw_Sparkles); Draw_Tiles 0x006D8078-0x006D80C9.

**Exit_Object** (P61's open items; the lane's last commit, landed with the re-recorded wins). gamemd.exe addresses: the
NavCom test 0x00444CA3 (`mov eax, [edi+0x5A4]; test eax, eax; je 0x00444D11`: FootClass+0x5A4 is NavCom, the offset matched
FootClass::FUN_004DC340 reads after TarCom at 0x004DC372), the jump-jet test 0x00444CB7 (TechnoTypeClass+0xD94, JumpJet) and
the teleporter test 0x00444CCB (+0xCD4, Teleporter), both through Techno_Type_Class (vtable +0x84); the archive target
0x00444CE2 (TechnoClass::Assign_Archive_Target 0x0070C610), QueueMission(MISSION_MOVE) 0x00444CEF, Assign_Destination(cell,
true) 0x00444D0B; the human test 0x00444D17 (HouseClass::FUN_0050B730) and the hospital test 0x00444D26
(BuildingTypeClass+0x16C1); GUARD_AREA 0x00444D3C; Where_To_Go 0x00444D51; RADIO_HELLO 0x00444DC3 and RADIO_UNLOAD to the
soldier 0x00444DD9; the vehicle path's human test 0x004448FE.
* A soldier leaving a barracks, hospital, armory or cloning vat is sent to the exit cell (MISSION_MOVE, Assign_Destination)
  only when it already has a NavCom and is neither a jump jet nor a teleporter; its NavCom becomes the archive target. Every
  other one stays in its mission, at the door in radio contact, and the building's radio unload walks it out. TibSun's text
  gave every soldier MISSION_MOVE to the exit cell. The radio RADIO_UNLOAD goes to the soldier itself.
* GUARD_AREA and the Where_To_Go archive target for a soldier when the house is not human (HouseClass::FUN_0050B730: IsHuman,
  or IsPlayerControl in a campaign) **or the building is a hospital** (a hospital's patients, whoever owns it). The vehicle
  path's human test is FUN_0050B730 too (TibSun: Is_Human_Player).

**Exit_Object checks.** ctest `yr-native-factory-exits` (extended): a GI made at a barracks leaves Exit_Object in its own
mission (not MISSION_MOVE: the draft gave m2), is at the door in radio contact one frame later and has been walked out by
frame 400; a GI made at a hospital (CAHOSP, placed beside the barracks) is on the map. New harness log lines
`harness-produce-missions` and `harness-produced-missions` (the current and queued missions, the radio links). The suite
passes with it. **It breaks the YR won replays** (owner's ruling: gamemd.exe's behaviour; a campaign lane re-records the
wins on top of it): all01 lost at tick 468,891, all05 lost at tick 16,249, sov02, sov03 and sov06 do not end (no game over by the 2,400 s limit), sov07 lost at tick 28,513; all04 (added to yr_won_all by lane P76 after these runs) was not run. No crash or trap in any log.
On main (4117ecb), and with only the Walk, Render and Draw_Tiles fixes, all six win.


## Lane P77 (2026-11-27): the YR won replays re-recorded on P74's Exit_Object

P74's last commit (afe3bd4: a soldier without a NavCom waits at the barracks door for the radio unload, as gamemd.exe)
parts every YR won recording. Two causes were seen: the object IDs part from the first frame in some missions (Soviet 7's
MCV is 1037552 on afe3bd4 and 1037554 before it), so a recording's "@^ID" clicks name other objects; and the player's and
the computer's soldiers stand elsewhere (a cosmonaut at the door no longer blocks a placement the old run retried, Soviet 6
8,000; the enemy raids come at other times). Nothing in the game was changed. Two tools (private, scripts/p77r):
lockstep.py replays a recording on main's binary and on afe3bd4 side by side, each "@^ID" click translated to the new run's
object of the same type and house nearest where the old run's object stands (with SHADOW=1 the player's new soldiers are
paired with the old run's by type and order of appearance and walked to their pair's place, the old exit-cell walk as a
player order); and the recorded route's own scripts rerun from a fresh start. Box about an hour a mission. Recordings in
private/campaign_saves/yr/<mission>/p77.

| Mission | Result | Notes |
| --- | --- | --- |
| Soviet 2 | won at tick 9,858; a rerun (ra2_campaign.py --game yr) wins at the same tick | The translated recording to 7,031 (three V3s and ten tanks at 93,123; one V3 more was lost to the pillboxes at 106-113,36 on the way), then P70's s2c.py as s2d.py: the tanks first at the two Mirage tanks by 114-116,168 (they killed the V3s at 110-112,163), the V3s at 107,159 firing from the first step (FIREBY=1), two at the Chronosphere and one at Einstein's lab: the Chronosphere gone before its transfer (9,354), the lab by 9,644, won at 9,858 |
| Allied 1 | won at tick 34,100; a rerun wins at the same tick | The translated recording to 32,892: Tanya is alive at Alcatraz's shore (60,64; P68's Tanya died to the hydrofoils on landing, and none are left). The Dominator (YAPPPT, Confederation) takes neither her attack nor her force fire. She C4s the Confederation's nuclear reactor (NANRCT 57,74) and dies in its blast, which takes the Dominator to 301/1000; her death (Tanya_Death, local 4) sends the six carriers, and P68's a1p10.py orders them at it |
| Allied 5 | won at tick 131,434; a rerun wins at the same tick | P68's a5a.py (to 16,000), a5b.py (to 33,365) and a5war.py from a fresh start: BadGuy2 not razed (25 objects left), the army and the battle lab lost by 36,609. P70's a5p.py with the battle lab, a power plant and a war factory in its plan (62 prism tanks) and a5e.py at both enemy houses: both gone by 125,235, then BadGuy1's last unit, an ostrich (CAOS, a vehicle), hunted down for It's Over Part 1/2 |
| Soviet 3 | not re-recorded (box) | The translated recording to 24,682 (Boris at 53,56, four tanks at 46-50,42-48). Nine tries with P70's s3e.py and s3m.py (s3run*.py: Grizzlies bought and gathered at 48,45 to 9 or 13 first; the tanks screening at 57,27; the initiates within 11 of 52,30 cleared first; the tanks sent at the Dominator with Boris's order): the gattlings at 56,29 and 59,24 fall with 13 tanks, but Boris dies each time on his way to 52,30, before he paints YAPPET: a trace shows the two gattlings at 51,33 and 51,36 beside his path from 48,44 kill him (200 to 0 between 43,025 and 43,225). Try 9 (GUNS also 51,36 and 51,33): Boris reaches his spot and four BPLN come, but only one tank is left by then and YAPPET stays at 1,000. Next: more tanks before the four-gattling phase, or Boris round the gattlings by the west |
| Soviet 6 | not re-recorded (box) | The translated recording loses the base by 20,000 (with SHADOW=1 by 50,457). P73's s6a.py from a fresh start razes the Americans by 22,300 (12 APOCs, 25 cosmonauts, power 309/385), then s6e.py (ARMY 14, then 24): the army is lost at the Alliance base both times, the 70,000 credits spent by about 43,000. Next: P73's try 3 state (24 APOCs and 21 cosmonauts by 48,700 with 2,500 credits) before s6e.py, more reactors (power stayed low) |
| Soviet 7 | not re-recorded (box) | The translated recording loses the starting infantry by 4,000 and the game at 14,433. P73's s7b.py from a fresh start (20 APOCs by 42,592), then P75's s7q.py (ARMYN=30 WANT=32): the base holds, but APOCs stay at 17-20 to 87,000 (lost buildings rebuilt from the income), so the army never reaches 30 and the beacon is not taken. Next: the economy (a third refinery and miners first, as P75's try 2) |
| Allied 4 | not re-recorded (box) | The translated recording loses Tanya by 10,000 and the base by 22,000. P76's chain from a fresh start (a4o.py LAND=160,104, a4p.py, a4r.py ...): the oil field free at 4,232 but one derrick taken (P76 five by 12,577); the Nighthawk is shot down at 159,104 at 10,167 with Tanya aboard, so a4r.py (Tanya's flight) stops. Next: the derricks' engineers and the Nighthawk's flight path in a4p.py |

No crash, trap or assertion in any P77 run. Calibration used: no.

## Lane P78 (2026-11-28): YR Allied 3 won by cutting Yuri's power; Soviet 5 at its cap; Allied 7 surveyed

Cap 3 lane-hours a mission in all. Scripts in private/campaign_saves/yr/scripts/p78 (start.sh, q.py as P77's),
recordings in private/campaign_saves/yr/<mission>/p78. Nothing in the game was changed.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 3 (ALL03UMD, Power Play) | won at tick 15,417; a rerun (ra2_campaign.py --game yr) wins at the same tick | 0.5 (2.6 before) | **Yuri's nuclear silo drains 810 (NAMISL Power=-810), and YuriCountry starts at 1650/1145**: a super weapon whose house is short of power stops charging, as P70 found for our storm. a3t.py: the storm (ready at 1,700) on the three bio reactors beside the silo (43,90: six buildings, Yuri 1200/1020) and the start's engineers into CAPOWRTD (110,110: Yuri 1000/1020); the nuke stops at "left 334" and stays there all game (Yuri does not rebuild the reactors; CAPOWRTD fell to a raid later, Yuri still 1000/1020). The base is never nuked, so the war factory builds 29 tanks by 8,422 and the storm recharges (ready 6,622 at 795/720). a3x.py from 3,211 (a3tu.replay): the tanks hold the base until 8,500. a3z.py: the second storm on the Arabs/MSGuardHouse nest on the y 96 route (107,107: Yuris, initiates, Lashers, Gattlings; P76's party never fought it, as its threat filter took YuriCountry alone), then a3y.py (a3u.py with every enemy house as a threat, GUNS empty): 19 tanks at 80,95 at 13,772, the third storm on the silo at 14,128 as the tanks close in, the silo gone at 15,214 with 13 tanks left, won at 15,417 (the 10,000-credit bonus at about 12,000 was not spent). yr/tests/campaign/all03.replay; all03 in yr_won_all |
| Soviet 5 (SOV05UMD) | not won; shelved at the cap | 0.45 (2.55 before; 3.0 in all) | Re-read of the dump: on Easy/Medium "Destroyed_Sub Pen" enables "_Win Conditions_Ez/Med", which leads to "_Winner" without "Detect_Yuri Land Dead" (that one is on the Hard and no-factories chains only), so the pen alone wins on Medium, as the speedrun dossier says. **A move click outside the map's LocalSize is no order** (LocalSize=2,4,131,135): here a cell needs about x-y <= 125 and x+y >= 150, so P71's and P78's first legs into the north-east corner (186,44 and 157,25) left the Dreadnoughts where they were (P71's "dreads at 147-149,34" for 3,000 ticks is this). s5path.py routes by water inside those bounds and away from the 21 Boomers. Try 3 (s5d.py, the whole fleet as one group by the west and south coasts): the three Dreadnoughts reached the south bay at 139,175-185 at 11,458 (487, 448 and 467 of 800) and were ordered at the pen (136,149) at 11,755, but were sunk by 12,295 with the pen untouched (1,500/1,500): the gattlings at 136-138,143-144 likely shoot their missiles down (V3-like spawned missiles), and the Yuri statues at 134,159 and 143,159 stand 8 cells from the bay. Next: the statues and the pier gattlings first (the Dreadnoughts' 25-cell range reaches 137,144 from 140,168), escorts kept with them, or the fleet sent south at once along s5path.py's route with the subs ahead |
| Allied 7 (ALL07SMD) | not won; not tried beyond a survey | 0.1 (1.4 before) | The Allied 3 power trick does not carry: YuriCountry has 32 bio reactors, 7800/3835 (Yuri2 1700/1135). At the start YuriCountry holds about 50 units (15 slaves, 12 Lashers, 8 Gattling tanks, Yuris, Brutes, Masterminds) and Yuri2 31; Win_Check needs every unit of both. The dump's way to it: Yuri2_All_Attack (11 Yuri2 buildings destroyed and no YACNST, YABRCK or YAWEAP: fire sale and hunt) and Yuri_All_Attack (60 YuriCountry buildings destroyed, no YAPPET, no YAGNTC and no YACNST, YABRCK or YAWEAP). Dreadnoughts and Kirovs need NATECH, so the radar (which starts Build_Yuri_SW) cannot be skipped. Left for a lane with the full 1.5 h: P76's chain with the dossier's curtained chrono strike |

No crash, trap or assertion in any P78 run. Calibration used: no.

## Lane P79 (2026-11-28): YR Soviet 1 won; Allied 4, Soviet 3 and Soviet 7 re-recorded on P74's Exit_Object; Allied 7 and Soviet 6 not

Cap 3 lane-hours a mission in all (re-recording time not counted against it, a box of about an hour each). Scripts in
private/campaign_saves/yr/scripts/p79 (start.sh, q.py; r/ holds P77's rerun scripts and this lane's chains), recordings in
private/campaign_saves/yr/<mission>/p79. Nothing in the game was changed. No crash, trap or assertion in any P79 run.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Soviet 1 (SOV01UMD, Time Shift) | won at tick 102,543; a rerun (ra2_campaign.py --game yr) wins at the same tick | 0.3 (1.6 before) | **The lock is avoidable on this pin.** P70's s1run.py (the start fleet at the nearest destroyer, carrier or cannon, started DELAY ticks after input opens) at DELAY 0, 25, 50, 75, 100, 150, 250, 350, 550, 625, 675 locks; **750 unlocks** (input open at 14,002 with Boris, 3 dogs, 5 tesla troopers, 3 engineers, 4 Rhinos, 4 Flak Tracks, 12 conscripts ashore; s1-delay750-unlocked.replay). Then: the landing fights east by 95,82 - 110,86 - 122,88 - 132,87 (fight_legs, the time machine and the pillboxes skipped: the nests at 123,85-91 and 133,82-91 and two Patriots); **Boris's airstrike** (Boris ordered at a building: two MiGs, about 600 ticks a pillbox) takes the pillboxes 132,99 and 132,103; Boris died walking at the next target (the Patriot 137,104 within reach of the pillbox 141,97 and the nest 140,108); the Rhinos took three pillboxes beside CATIME01 (it lost 11 of 2,500) and the reinforcements (4 Rhinos, 12 conscripts, sent when the first ones are lost) the last: Destroy_Time Defenses at 36,968, the time machine the player's. **Power:** the Arabs' engineers took five of the six tech plants (GAPOWRA-F) first; the army cleared each plant's initiates and two engineers retook GAPOWRE, GAPOWRD and GAPOWRF: power 713/625 at 46,783, SP3.1 Full Power, the Jurassic trip (47,100-50,400, the group kept together against the beasts) and the return to 141,100. **Part III:** the group walked to the Soviet base at 87,117 (entered-by: the base and 25,000 credits the player's; Boris reinforced at 94,110). The Psychic Dominator YAPPPT (40,70) stands on an island; this mission's sidebar has no battle lab, so no Dreadnoughts. An amphibious transport (SAPC) ordered to a land cell drives on land by the nearest landing and died to the gattling 75,93 (killed first by the Rhinos); Boris and four conscripts waiting at the road's end (72,101) for it died there. Two SAPCs loaded at 75,101 (on land: they drive up), sailed 64,92 - 60,78 - 56,65, and were driven onto the beach 52,64 (D in the water does not unload): both were destroyed there, their dogs and conscripts came out alive; the dogs killed the five initiates, the conscripts attacked YAPPPT (978 -> 923 -> 329 in about 600 ticks) and SP3.6c's win followed. yr/tests/campaign/sov01.replay; sov01 in yr_won_all |
| Allied 7 (ALL07SMD, Brain Dead) | not won; shelved for this lane | 0.5 (1.5 before; 2.0 in all) | Eight tries from try1.replay (P79's Soviet opening) at 7,249. **Facts found:** the Soviet base is an island (no ground path to the plateau or to Yuri; BFS on `terrain`); the plateau (105,192) reaches Yuri2 (56 cells) and the Dominator (128). Win_Check (destroyed-units-all of houses 14 and 9) is enabled from the start: every vehicle and soldier of Yuri2 (31) and YuriCountry (50, growing to 80), with 100,000 credits each. A Soviet base without barracks or war factory (they start the Arabs' raids) still builds the radar (at 4,792; the MCV at 6,987); the Allied barracks makes NAFLAK buildable; NATECH never appears without a Soviet war factory, so no Iron Curtain. **The Floating Discs** (two YuriCountry DISK after the radar) razed the Soviet base in tries 2-4 (two flak cannons did not stop them); Patriots (NASAM, two per base) did. **The Genetic Mutator** fired early at a clump of ten GIs (15,830) and the Brutes razed the plateau: build no infantry. **The Dominator** (fires at the MCV + 13,450) hits the largest cluster: in try 1 it took the escorts and the base (the mind-controlled escorts finished the base); units spread one per spot 11+ cells out before it fires (left < 450) left the base standing in tries 4 and 6 (two buildings lost), but in try 7 the first shot took the whole plateau again, and the second (33,995 in try 6) took both war factories. Lost Allied construction yards on Medium bring new MCVs, which the controlled units killed one by one (try 7). The Force Shield on the sidebar's tab is Yuri's (no player super line). Best state: try 6 at 40,000, both bases up, 20,000 credits unspent (one war factory, prism tanks 11 at most). Next: keep the economy and tech on the Soviet island (Allied buildings place there too), put only war factories and defences on the plateau, spread units before each shot, and send prism tanks at Yuri2's factories and construction yard first (its fire sale and hunt needs 11 buildings, YACNST, YABRCK and YAWEAP); the dossier's Iron Curtain needs the Soviet war factory and its raids |
| Allied 4 (re-record) | won at tick 92,191; a rerun wins at the same tick | 0.2 | P77's chain (P76's scripts) from a fresh start, with one change: a4p.py sends the Nighthawk home (91,167) as the derricks phase begins. P77's run had it parked at the oil gap, where it and then Tanya were killed at 10,167 and 10,482; here five derricks by 12,640 with both alive. a4r.py, a4s.py, a4dom.py, a4b.py and a4w.py to 83,000 (52 prism tanks at Yuri's base by 79,800); hut.py found no engineer and stopped (the army had already crossed), so a4w.py to 110,337 and a4x.py followed without it: won at 92,191. yr/tests/campaign/all04.replay; all04 in yr_won_all |
| Soviet 3 (re-record) | won at tick 91,742; a rerun wins at the same tick | 0.15 | P77's translated recording to 24,682 (sov03/p77/sov03-lock.replay), then P77's s3run3.py with WANT=16 WAIT=90 and GUNS "51,36 51,33 56,29 59,24" (the two gattlings by Boris's path first): 13 Grizzlies gathered by 45,355; the sniper cleared the box, the four gattlings fell; Boris reached 52,30 and died there (51,085) with YAPPET untouched, but s3m.py's sweep with the tanks left went on, YAPPET among the last, and the win came at 91,742. yr/tests/campaign/sov03.replay; sov03 in yr_won_all |
| Soviet 7 (re-record) | won at tick 156,119; a rerun wins at the same tick | 0.5 | P77's s7run.py chain gave 17-20 APOCs (refineries lost to raids and four Kirovs, credits near 0). s7b2.py (P73's s7b.py with two more flak cannons, a third refinery, six War Miners, a fifth power plant) and P75's s7q.py with ARMYN=26 WANT=28: 26 APOCs left at 140,436 and 19 reached the beacon approach (101,49) at 149,802, after s7q.py's END had passed, so its beacon loop never ran (try 2; try 3 with a later END collapsed at 192,000). s7beacon.py from try 2's recording at 149,802: s7q.py's beacon loop (force fire on NAPSYB 96,43): gone at 150,369, BadGuy2's base the player's; then P75's s7r.py west: CATRAN03 gone at 155,877, won at 156,119. yr/tests/campaign/sov07.replay; sov07 in yr_won_all |
| Soviet 6 (re-record) | not re-recorded (box) | 0.55 | Seven runs. The moon has no ore: the money is the start's plus the crates. A fresh start through P73's s6a.py reproduces P77's s6run1.replay exactly (22,338: 7 APOCs, 36 cosmonauts, 17,930 credits; P73's try 3 had 10, 26 and 24,842 at 22,445). From there: s6d.py's attack takes the Americans and part of the Alliance and loses the army (the base lost at 51,129); s6d.py stopped when only Yuri is left on its list, then s6f.py: 15 APOCs and 2 cosmonauts take the Lunar Command and two reactors (Yuri 1650/1435) and are lost by 66,925 with no money left; keeping refineries was wrong (there is no ore); more cosmonauts in s6d.py left no APOCs; s6d.py building only (fixed: GO also when credits < 300, a production stuck for money had blocked it) then P73's s6e.py: the 18 staged at the Alliance base were all lost. Next: s6a.py with fewer cosmonauts (its LMAX) to keep P73's credits for APOCs, then s6e.py's staged attack |

Checks: yr-native-campaign-won-all01, -all03, -all04, -all05, -sov01, -sov02, -sov03, -sov07 and yr-native-factory-exits
pass (9 of 9); tools/check_tree.py is clean. yr_won_pending keeps sov06 alone. Calibration used: no.
