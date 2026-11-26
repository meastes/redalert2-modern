> **History.** This is the record of how the port began: redalert2-decomp's `native/rb/README.md` from the
> `ports-on-rebuild` branch (lanes P1 to P11), kept as written. There the work was a trial beside the src-based port,
> and its paths, names (`rb-native-trial`, `RBTrial.app`, `rb_trial.sh`) and "trial" wording are that repository's.
> In this repository it is the port: the target is `ra2-native`, the iOS app `RA2.app`, the scripts `ra2/tools/ra2_*`,
> and the src-based port is the old port (README.md, History).

# native/rb: the native ports on the rebuild tree (branch ports-on-rebuild, lanes P1 to P11)

User, 2026-11-23: start moving the ports onto the rebuild code on a branch, below recovery work.
Plan: `docs/REBUILD_HANDOVER_PLAN.md` section 2 (work items 1 and 2 here; item 3, `link/` and
`drafts/` renaming, waits for the user's 11-29 flex decisions). This area never edits `src/`,
`rebuild/` or the rest of `native/`; the src-based port stays the built and played one, and nothing
here lands on main until the rebuild-based port passes what the src-based port passes.

| Path | What |
| --- | --- |
| `PIN` | the main commit whose `rebuild/` the branch builds against (re-pinned only deliberately) |
| `tools/rb_units.py` | the generator (from `native/yr/tools/yr_units.py`): the pinned tree by `git archive`, one native TU per `rebuild/src` unit, the header directory, the port rewrites (`native_units.port`), `fixups.json`; units ordered by the original `_initterm` order (`data/initterm.json`) |
| `fixups.json` | clang fixups on the generated copies, each with its VC6 meaning |
| `glue/`, `sdk/` | native replacements appended to units; headers searched before `native/yr/sdk` |
| `tools/rb_probe.py` | compiles the units with the YR build's flags (`--obj`; `--non-matching` adds the rebuild's parked drafts) |
| `tools/rb_data.py` | the rebuild's `data_globals.py`: zero storage for undefined .bss names bound by the rebuild catalog, the initialised ones listed for the run-time load |
| `tools/rb_original_data.py` | the initialised globals' storage and the run-time load table (`ra2_original_globals`, `ra2_original_storage`) the port fills from the user's game.exe, from `yr_data.py`'s declaration reader |
| `tools/rb_rekey.py` | main's address-keyed tables re-keyed to rebuild names: `data/initterm.json`, `data/rekeyed.tsv` |
| `tools/rb_trial.sh` | the whole trial outside CMake: generate, compile, bind data, bind functions by address, close the link, weaken repeated header constants, defer the static initialisers, link against a src-based build's `ra2_port` and host (read only). `RB_NON_MATCHING=1`: the parked drafts too (WinMain is one; the played trial); `RB_ASAN=1`: AddressSanitizer units (diagnosis) |
| `tools/rb_aliases.py` | undefined functions bound to the rebuild's definition at the same original address (catalog references and regions; same argument shape required; overloads told apart by their decorated argument types) |
| `tools/rb_run.sh` | runs a trial binary to a milestone with the user's installed game read at run time (`--nm`, `--bin`; `RA2_DATA_DIR`, `RA2_GAME_EXE`, `RB_WRITE_DIR`) |
| `tests/rb_routes.py` | the checks (lanes P3 to P7; P7 adds `forcewin\|forcelose\|chain --side allied\|soviet`): the scripted runs through the real menus, each in a fresh write folder (`--route skirmish\|soak\|exit\|audio\|save\|resave\|original\|combat\|combatsave\|campaign\|production`); ctest rows in `CMakeLists.txt` |
| `tools/rb_ios_sim.sh` | lane P7: the Simulator app as a drop-in `--binary` for rb_routes.py (RA2_* passed as SIMCTL_CHILD_*; `--setup <udid>` boots and installs, `--cleanup` shuts down what it booted); the rb-ios checks |
| `tools/rb_ios.sh` | lane P6: the iOS form, opt in (`build simulator\|device`, `run <udid> [seconds] [run-to] [host args]`; lane P11: `run-imported <udid> [seconds]`, the import path without Mac paths): RBTrial.app (bundle id io.github.ra2native.RBTrial) from native/rb's CMake in its iOS form, against a src-based iOS build's port library, host objects and static SDL3 (only those built) |
| `tools/rb_skirmish_wrap.py`, `tests/rb_identity.py` | lane P8: main's tests on the trial (rb-main); frame identity against the src-based port |
| `glue/harness_native.inc` | lane P5: the order harness (`RA2_NATIVE_ORDERS`: spawn, select, fire, health, draws), appended to the conquer unit and called each loop frame from the Main_Loop seam; test scaffolding |
| `tools/rb_original_layouts.py`, `data/original_save_types.txt` | lane P4: the field maps for the original's save format over the rebuild units (`native/tools/original_layouts.py` imported, its dump replaced) |
| `CMakeLists.txt` | the opt-in CMake form of the trial, a project of its own beside `ra2-native` (this lane edits only `native/rb/`): `cmake -S native/rb -B build/rb-cmake -G Ninja [-DRA2_SRC_BUILD=...] [-DRB_NON_MATCHING=OFF]`, target `rb-native-trial`; `weaken_duplicates.py` and `defer_initializers.py` as PRE_LINK. Hooking it into `native/CMakeLists.txt` (one include behind an option, as `yr/yr.cmake`) is left to a lane that may edit that file |
| `glue/rb_host_glue.cpp` | the host's `ra2_native_original_records`: the units' static initialisers (moved to `__DATA,__rb_init_func`) run after the game.exe load, in the units' (original `_initterm`) order |
| `glue/rb_behaviour.cpp`, `glue/rb_asm_draw.cpp`, `glue/*_native.inc` | native bodies the trial reaches that the rebuild carries neither exact nor as a compiling draft: main's text (link/, drafts/, exact rows the rebuild lacks) under the rebuild's names; each `.inc` is appended to its unit by a `fixups.json` row. Item 3 replaces them with the renamed link/ and drafts/ |

## State at PIN 4d8ce026ed (lane P1, 2026-11-23)

* All 438 units compile under clang for arm64 (`wonline.cpp`, Westwood Online, left out as in the
  YR port: 439 units in the tree); also with `-DRA2_NON_MATCHING` (the rebuild's parked drafts).
* The two header fixes the probe found: `int.h:175` (the friends become friend templates) and
  `tag.h`'s static `Init` (renamed `Init_Static` with its definition and one caller); 35 fixups in all.
* The trial links (`rb-native-trial`, about 10 MB; not run yet). Link closure against the units,
  `ra2_port` and the SDL host, without any of main's `native/link`, `drafts` or data objects:
  602 undefined; 385 after `rb_data.py`'s .bss storage (217 names); **202** after
  `rb_original_data.py` (172 initialised globals, 163 loaded from game.exe) and the YR port's CRT
  glue: all functions, no data. With the parked drafts compiled (`RB_NON_MATCHING=1`): **158**.
  Main's src-based port closes at 180 with all of its `link/`, `drafts/` and data. Of the 202: about
  93 are main-exact functions (item 4 overlay; many Westwood Online, whose unit is left out), 77
  have a behaviour rewrite or draft in main's port by address (item 3: the assembly routines,
  blitters, codecs), 22 carry no catalog address (WinMain, constructors the rebuild parks), 9 are
  library or assembly at or past 0x781000, 1 is exact in neither tree.
* `data/initterm.json`: 3,553 of main's 3,610 `_initterm` entries re-keyed to rebuild units and
  symbols (the 57 others are header-static copies in units outside the rebuild's 438, and three
  seed views); in the rebuild every initialiser is a unit's own static object, so no explicit calls.

## State at PIN 5932814e42 (lane P2, 2026-11-23)

Re-pinned from 4d8ce026ed (deliberately: the skirmish dialog trapped on `OwnerDraw::Combo_Proc`, whose
first draft, with `List_Proc`'s, landed in chunk 77 after the old pin).

**The trial plays a skirmish.** `RB_NON_MATCHING=1 native/rb/tools/rb_trial.sh`, then
`native/rb/tools/rb_run.sh --nm frame 100 --input-script=<s1+1000 click 834 304 / s2+1000 click 834 388 /
s3+1500 click 834 346>` (Single Player, Skirmish, Start Game), reading the installed game at run time:
WinMain, RA2.INI, LANGUAGE.MIX and the MIX keys, the window and DirectDraw surfaces, the main menu (about
14 s in, as main's port), Single Player, the skirmish setup, the map load, then Main_Loop: map, shroud,
units, sidebar and command bar drawn, logic and AI running, no trap and no fault: 4,803 frames in a
100 s run (main's port: 4,602 on the same script), with the Main_Loop seam (`glue/mainloop_native.inc`:
main's native frame floor, 62 frames a second at the slider's default GameSpeed 0, and the run-to frame
count); a 420 s soak played 16,918 frames with no trap until the idle player lost and the score
screen showed; an exit route (frame 600 Escape, Abort Mission, Quit, then the menus) returns to the main
menu and ends with WinMain returning 0 (`build/rb-native/run/exit.script`, as main's
`tests/exit_route.py`). Without the floor the game ran uncapped (frame 19,301 after a minute) and the computer won. The
CMake build (`build/rb-cmake/rb-native-trial`) gives the same closure and runs the same way. Without the drafts the
trial stops at WinMain (parked in the rebuild).

Link closure (functions with no definition; the rest are bound): **93** with
the parked drafts, the played build (P1: 158; 202 for the exact text, now 179: the glue bodies call functions only the drafts define); main's src-based port closes at 180 with all of its link/, drafts/
and data. 33 functions bound by address (`rb_aliases.py`), 168 initialised globals loaded from game.exe.

Fixed (each a `fixups.json` row or a glue file, with its why):

* start-up: static initialisers deferred until after the game.exe load; the WinMain draft compiled
  (SEH dropped, as main's port); the launcher strings loaded (host declarations read too); base64's
  `BIG_ENDIAN` test (Darwin defines it: the MIX header keys decoded to garbage; YR's row); the
  theater, z-gradient and cursor tables loaded as plain records;
* pointer width: WinData's drawer and backdrop shapes, its raw 32-bit offsets, the replacement list
  box's GWL_USERDATA words, `TextNode`'s header size, `NodeList`'s view of WinData, message 0x4E9's
  two record declarations; the notice interfaces' pointer arguments; the radio parameter (`intptr_t &`
  in every Receive_Message/Transmit_Message, `LParam`); IsoBlitState's source pointers; RLE_Blit's and
  the wave draws' buffer addresses; YR's rows for abuffer, zbuffer, isotype, audmpool, radar, voxlib
  where the rebuild has the same text;
* native bodies (main's text, rebuild names): set_control_word, the CPU probes and Get_CPU_Type(_Name),
  the mono pages, Adjust_Color_565/555, Intersect, AlphaShapeClass::Draw_All,
  Building_Player_Detector_Covers_Target, TechnoClass::Is_Weapon_Valid, the assembly voxel drawers,
  Get_CPU_Clock, the spot-light brighteners.

Findings for the rebuild (no edit here): `ScorePalette` names two objects (`_palette.cpp`'s
PaletteClass and `txtlabel2.cpp`'s pointer at 0x00AC1160), which per-unit compilation never links
together; message 0x4E9's record and WinData's Nodes each have two declarations (TextRequest /
ToolTipTextRequest, NodeList); `TechnoClass::Is_Weapon_Valid` and `Is_Weapon_Present` are one address
under two declarations.

Next:

1. Turn the scripted runs (skirmish, soak, exit route) into checks beside the src-based port's (YR's
   yr_skirmish_smoke.py / yr_skirmish_score.py are the model), then the score screen's Continue.
2. Use a fresh `RB_WRITE_DIR` per comparison: the game writes its RA2.INI there (the skirmish setup's
   last choices).
3. The rest of the closure as the runs reach it (93 with drafts: Westwood Online, the VQA/LCW codecs,
   the 556/655 colour formats, Asm_Interpolate, WDT, a few class methods), then item 3 (link/, drafts/
   renamed) after the 11-29 flex decisions; item 4's overlay for main-exact rows the rebuild lacks
   (the mono pages and Get_CPU_Type here are its first members).
4. Member aliases by offset in the rebuild layouts (P1's step 3) are still open; this lane did the
   function aliases by address.
5. The movies and audio (the runs use -noaudio; the VQA codecs are still traps), then saves (item 6).


## Checks, audio and saves (lane P3, 2026-11-23, same PIN)

**Checks.** `ctest --test-dir build/rb-cmake` (or `-L rb-quick`, everything but the soak) runs
`tests/rb_routes.py` against `rb-native-trial`, each run in a fresh write folder under
`build/rb-cmake/tests/<route>`, the user's installed game read at run time (`RB_DATA_DIR`, default the
Lesalia install; `RB_GAME_EXE`, the pinned target); skipped (77) without them. All five pass:

| ctest | route | checks |
| --- | --- | --- |
| `rb-native-skirmish` | Single Player, Skirmish, Start Game, 75 s | no trap or fault; the time limit ends it; the game loop reached; at least 3,000 frames (3,747) |
| `rb-native-soak` | the same, 420 s idle | at least 6,000 frames (the count stops when the computer wins: 12,339 to 16,918 seen); the score screen (Do_Lose's 50-control dialog) shown |
| `rb-native-exit-route` | frame 600 Escape, Abort Mission, Quit, Back, Back, Exit Game, OK (main's `tests/exit_route.py` route) | the main menu shown again; WinMain returns 0; exit 0 |
| `rb-native-audio` | the skirmish route without `-noaudio` and without the movie skip | the start-up movie opened with its sound track (Bink, the user's BINKW32.DLL tables); the mixer not silent before the menu; in the game at least three sound buffers playing with a nonzero peak (`RA2_PORT_TRACE`, `RA2_NATIVE_AUDIO_LOG`) |
| `rb-native-save` | skirmish with a harness save at loop frame 600, then a new process with `--load-save` | the save written; `resume: loaded RBROUTE.SAV`; the game loop reached again, at least 1,500 frames |

**Audio and movies work.** RA2's movies are Bink (the port's player, as main's port): the intro plays with
sound, the menu's animated backdrop plays, and the game's sound effects and music mix. Carried for it:
`Sound_Effect` (TibSun's two declarations, called by parked drafts: through VocClass::PlayGlobal/PlayAt,
`glue/voc_native.inc`) and MSSfx (the rebuild's own mssfx unit text from chunk 80, after this pin,
`glue/mssfx_native.inc`). The VQA/LCW codecs are no longer traps (`glue/rb_vqa.cpp`: main's
`behaviour_asm_vqa.cpp`, the C16 `UnVQ1_4x2_Table` and `LCW_Comp` under the rebuild's names; RA2 itself
never reached them in these runs).

**Saves round-trip, in the port's container (not yet the original's format).** The harness save
(`RA2_NATIVE_SAVE_AT=<frame>[,<file>]`, `RA2_NATIVE_SAVE_LEAVE`) is in the Main_Loop seam
(`glue/mainloop_native.inc`), resume at start (`--load-save`) in LoadOptionsClass::Load
(`glue/loaddlg_native.inc`). Fixed on the way (each a `fixups.json` row): SaveVersionInfo's constructor
(`glue/savever_native.inc`; the unit defines it under saveload.h's SavegameInformation, the same class
declared twice), LZO1X-1's pointer dictionary (YR's row), LayerClass::Save's 4-byte pointers against
Load's 8, the swizzle manager's ids and its 4-byte pointer store in Process_Tables, the object ids of
AbstractClass/LocomotionClass/the veinhole monsters (YR's rows). The check sets
`RA2_NATIVE_TEST_PORT_SAVE=1`: without it the port refuses to save ("no field maps for the original's
save format"): the original's format needs `native/tools/original_layouts.py` run over the rebuild units
(a `--game rb` mode beside `--game yr`: the i686 and arm64 layouts of the types in
`data/original_save_types.txt`), which is the next save step and where member aliases by offset come in.

Link closure: **75** with the parked drafts (93 before: the VQA/LCW helpers, Sound_Effect, MSSfx,
SaveVersionInfo). Left: Westwood Online (unit left out), the 556/655 colour formats, Asm_Interpolate,
Iso_Blit_Asm2, WDT, the ice tile statics (no catalog reference), a few class methods
(TechnoClass::Get_Collateral_Damage, Unknown_Virtual_3DC, CellClass::Is_Tile_Ice,
XSurface::Draw_Ellipse, DSurface::Draw_Depth_Antialiased_Line), the host self-tests.

Findings for the rebuild (no edit here): `savever.cpp` defines SaveVersionInfo's constructor and
Set_Version/Get_Version as `SavegameInformation::` (saveload.h's declaration of the same class);
LayerClass::Save writes `sizeof(int)` where Load reads `sizeof(ObjectClass *)` (equal on VC6).

Next (lane P3's view):

1. Original-format saves: an rb mode for `native/tools/original_layouts.py` (or a copy under
   `native/rb/tools/`), generating `ra2port_original_layouts` for the rebuild units; then drop
   `RA2_NATIVE_TEST_PORT_SAVE` from the save check and add a load of one of the user's original saves
   (main's `original_save_load.py` route).
2. The score screen's Continue (a won or lost game back to the menus) as a check; a combat route
   (YR's yr_combat.py) to reach Get_Collateral_Damage and the other class-method traps.
3. Re-pin when the rebuild's mssfx and mono rows (chunks 80, 81) make the overlay copies redundant:
   drop `glue/mssfx_native.inc` and the duplicate parts of `glue/mono_native.inc` at the same time.
4. Item 3 (link/, drafts/ renamed) after the 11-29 flex decisions.


## Original-format saves, score Continue, surface slots (lane P4, 2026-11-23, same PIN)

**The user's original RA2 saves load.** `tools/rb_original_layouts.py` runs `native/tools/original_layouts.py`
(imported, not edited: the src and YR modes are untouched) over the generated rebuild units, its record
layouts dumped with the units' own flags (`rb_probe.py`: the YR dialect, `native/rb/sdk`, `-DRA2_NATIVE_RB`);
the vtable pointers by address from `manifests/rtti_classes.json` (the rebuild's classes carry the RTTI
names). Its type list is `data/original_save_types.txt` (main's list under the rebuild's names: EnvironmentClass
read by its own Load, VeinholeMonsterClass left out, no rebuild unit defines it at this pin): 74 listed types and
2,884 member records, 21,057 copy operations, 0 problems, 0 unmatched members. The trial links the strong
`ra2port_original_layouts` tables (`rb_trial.sh`, and `rb_data_objs` in CMake) over the port's weak empty
ones, so the port reads and writes the original's compound-file format; its own container is no longer used.
All five of the user's saves in the install folder (ALL01T, ALL02S, ALL03U, SAVE397A, SAVE4FBD), read in
place at run time and never copied, load and play on (1,624 to 2,122 frames in 60 to 75 s, no swizzle failure).

Fixed on the way (fixups.json rows, each with its why): LocomotionClass::Load read the locomotor's image
through `void *` (raw i686 bytes over the arm64 object; the stream desynchronised after the first ship
locomotor: YR row Y10, the typed read); SwizzleManagerClass::Process_Tables' walk bounded and an unmatched
request reported and nulled before the original's 1 / 0 (YR row Y10; arm64 division does not trap, the walk
spun); Static_Sounds_Load's unnamed four-int entry read through `void *` (the same raw read, without the
"no layout" report); ObjectClass::Load's swizzle of AbstractClass +0x18 (a 4-byte int natively, so Swizzle
patched it and RefCount as one 8-byte id) skipped for a null field, main's port fixup for the same case (a
resaved bullet with references failed to load: unmatched id 0x200000000).

**Checks** (`ctest --test-dir build/rb-cmake`: all nine pass, rb-native-soak run through rb_routes.py on the trial binary): the save check no longer sets `RA2_NATIVE_TEST_PORT_SAVE`; it
requires the harness save to be a compound file and its reload to open as an original. New:
`rb-native-original` (ALL01T.sav from `RB_DATA_DIR`: opened as an original, loaded, objects converted by the
field maps, no type read without a map, the game loop, at least 900 frames: a loaded game runs at its save's
game speed, about 20 frames a second), `rb-native-resave` (ALL01T.sav loaded, a harness save at frame 300
in the original's format, that save loaded by a new process and played on: main's original_save_write.py for
one save) and `rb-native-original-layouts` (the field-map check, `--check`).
The soak route now band-selects the player's units at frame 300 (brackets and health bars drawn) and after
the score screen presses Continue, the setup's Back and Single Player's Back: the main menu must show again.

**Closure 72** with the parked drafts (75 before): main's behaviour bodies under the rebuild's names for
XSurface::Draw_Ellipse 0x007746C0 (`glue/xsurface_native.inc`) and DSurface::Draw_Depth_Antialiased_Line
0x004AC650 (`glue/dsurface_native.inc`; main's buffer names mapped to the rebuild's ZBuffer/ABuffer
accessors), both kept by the rebuild as TibSun text under `#if 0`; TechnoClass slot 247 (+0x3DC) 0x006C3B20, an
empty body in RA2, which the rebuild defines as Draw_Health_Bar_Old and declares apart as the virtual
Unknown_Virtual_3DC (`glue/techno_native.inc`). Left as traps on purpose: TechnoClass::Get_Collateral_Damage
(called only by BuildingClass::Get_Collateral_Damage, an unplaced draft no RA2 code calls), CellClass::Is_Tile_Ice,
Pick_Ice_Tile_Set and the ice/swamp tile statics (called only by the unplaced Seed_Ice / Smoothen_Full_Ice
drafts: TibSun's ice, absent from RA2); main has no body for any of them.

Not re-pinned (nothing above needed it): `glue/mssfx_native.inc` and the duplicate parts of
`glue/mono_native.inc` stay until a re-pin. Member aliases by offset were not needed for the saves (the field
maps pair the i686 and arm64 layouts of the same rebuild source by member path); they remain for carrying
main's link/ and drafts/ text over the rebuild layouts (item 3).

Findings for the rebuild (no edit here): LocomotionClass::Load reads `(void *)this` where Save writes `this`
(equal on VC6); TechnoClass 0x006C3B20 has two declarations (Draw_Health_Bar_Old, Unknown_Virtual_3DC); the
`#if 0` Draw_Ellipse/Draw_Depth_Antialiased_Line text is not RA2's (main's notes give RA2's walk).

Next (lane P4's view):

1. A combat route reaching the laser and radial-indicator draws (Prism towers; a placed Gap Generator or
   Psychic Sensor) to exercise the two surface bodies; YR's yr_combat.py needs a harness order the rebuild
   trial lacks.
2. The resave check over all five saves (`--save`), and a save made mid-combat.
3. Re-pin when chunks 80/81 matter (drop `glue/mssfx_native.inc` and the mono duplicates then).
4. Item 3 (link/, drafts/ renamed) after the 11-29 flex decisions.


## Combat, all saves resaved, re-pin (lane P5, 2026-11-23, PIN b873d2b3c6)

**Combat route** (`rb-native-combat`). The trial had no order harness; `glue/harness_native.inc` (appended to the
conquer unit, called each loop frame from the Main_Loop seam) is a small subset of the YR port's and main's
under the rebuild's names: `RA2_NATIVE_ORDERS` with `spawn <ID> [enemy] [dx dy]`, `select <ID>`, `fire <ID> <TARGET>`
(Queue_Mission MISSION_ATTACK, the target watched to its death), `health` and `draws` (the call counts of the
carried XSurface::Draw_Ellipse and DSurface::Draw_Depth_Antialiased_Line bodies and the laser beams seen). The route:
the skirmish start, then three Allied power plants, two Prism towers and a Gap Generator for the player, the Gap
Generator selected, a Prism tank, two enemy Rhinos, the Prism tank sent at one. Seen: the Gap Generator's radial
indicator (two Draw_Ellipse calls a frame, four sweep spokes through Draw_Depth_Antialiased_Line), 17 to 18 laser beams
(the tank and towers), the Rhino killed at about frame 350, a tower damaged; no trap, no fault, about 2,800 frames
in 60 s. Finding: in RA2 every laser weapon in use is house-coloured (PrismShot, PrismSupport; LaserFire/LaserFire2
are defined but no unit carries them, the railgun particle systems likewise), so lasers draw through
LaserDrawClass::Draw_House_Color (Draw_RGB_Multiplying_Line, Draw_Depth_Shaded_Line) and the anti-aliased line is
reached only by a radial indicator's sweep: the check counts both bodies and the beams.

**Saves.** `rb-native-combatsave`: the combat route saved at frame 260 (the Prism tank's attack under way), in the
original's format, then loaded by a new process and played on (1,103 frames in 60 s at the save's game speed).
`rb-native-resave-<save>` (label `rb-resaves`): the resave route over the other four saves in the install folder;
all five (ALL01T, ALL02S, ALL03U, SAVE397A, SAVE4FBD) load, resave at frame 300 in the original's format, reload
and play on (2,121 to 2,128 frames in 75 s), read in place, never copied.

**Re-pin** from 5932814e42 to b873d2b3c6 (main's head at 2026-11-23: chunks 80-84, the mono and mssfx units, the
voxel and astar twins): 439 units compile (mssfx new; wonline still left out). Dropped in the same change, now the
rebuild's own: `glue/mono_native.inc` (Mono_Window_Init, Get_Page, Mono_Clear_All_Pages, Mono_Window_Shutdown),
`glue/mssfx_native.inc` (MSSfx), `Get_CPU_Type_Name` from `glue/getcpu_native.inc` and UnVQ1_4x2_Table from
`glue/rb_vqa.cpp`. One new fixup: the parked Mono_Dialog_Proc draft reads vox.cpp's `Voxs`, which no header declares
(declared after the draft's includes). `data/initterm.json` and `data/rekeyed.tsv` re-keyed at the new pin
(`rb_rekey.py`). **Closure 70** with the parked drafts (72 before). After the re-pin all 14 ctest checks pass
(`ctest --test-dir build/rb-cmake`, 27 minutes: the nine before, rb-native-combat, rb-native-combatsave and the four
rb-resaves rows; the soak played 13,611 frames to the score screen and back to the main menu).

Findings for the rebuild (no edit here): mono.cpp's Mono_Dialog_Proc draft uses `Voxs` with no declaration in scope
(its unit's extern was dropped for the structure gate; a compile of the guarded draft needs it).

Next (lane P5's view):

1. The trial's remaining reachable traps are the same as P4's list (Get_Collateral_Damage and the ice statics,
   reached only by unplaced drafts); a campaign-mission route (a loaded Allied save played to a trigger) would
   widen coverage next.
2. More harness orders as routes need them (main's `harness_orders.cpp` is the model: super weapons, production,
   infiltration).
3. Item 3 (link/, drafts/ renamed) after the 11-29 flex decisions.


## Campaign route and iOS (lane P6, 2026-11-23, same PIN b873d2b3c6)

**Campaign route** (`rb-native-campaign-ALL02S.sav`, `rb-native-campaign-ALL03U.sav`, label `rb-campaign`). The user's
two later Allied saves, read in place from the install folder at run time, are loaded and played on with the harness's
new trigger watch (`RA2_NATIVE_TRIGGERS=1`: `harness-trigger <frame> gone|new|tripped <ID> <name>`, main's
harness_soak.cpp under the rebuild's names; TriggerClass::IsTripped is private, read through a member pointer) and its
`hunt` order at frames 1,200 and 2,400. Each check requires the mission's scripted triggers (`CAMPAIGN` in
`tests/rb_routes.py`) to fire:

| Save | Seconds | Triggers required (frame seen) |
| --- | ---: | --- |
| ALL02S (Allied 2) | 120 | Destroy Airplane Team (76), Play Rocketeer Movie (451), Rocketeers Move (452), Rocketeers Have Arrived (526: the reinforcement), Rocketeers Become USA (601: the house change), Attack Here - first time (901), Txt Flak Cannons (1,351), txt Flak Reminder (2,026), 1st Patrol has been shot (2,675 to 2,706, after the hunt) |
| ALL03U (Allied 3) | 150 | Garrison1 (46), Hint-IFV -- pt1 (76), Garrison3 (226), UnlockInput (421), StateObjective (511), AttackParty (1,463), ShockTrooperHint-pt2 (1,528), CivAttack1 (1,801), CivAttack2 (3,301), ShockTroopIntro (3,751) |

About 29 frames a second (the save's game speed); no trap, no fault.

**An intermittent fault, open:** one load of ALL02S in about a dozen (rb-native-resave-ALL02S.sav in this lane's full ctest
run; the re-run passed) stopped at loop frame 20 with SIGBUS in the rebuild's `Draw_Voxel_Regular_Normals_Lighting`
(VoxelLibrary::Render_Object, UnitClass::Draw_It), reading at 0xa13fcf0b1. The drawer's text is the same as main's exact
0x0071A950, so the bad input comes from upstream (the layer's StartOffset/DataOffset or ZSize from Get_Layer_Info, or a
run running past its column); malloc scribbling (MallocPreScribble) did not make it repeat. Next: an AddressSanitizer
trial (`RB_ASAN=1 tools/rb_trial.sh`) looped over ALL02S loads. Neither mission is won or lost in the time (the
`state` order reports `win 0 lose 0`); a forced end (main's RA2_NATIVE_FORCE_END) is not in this harness yet.

**Production and a super weapon** (`rb-native-production`, rb-quick). New harness orders under the rebuild's names, after
main's harness_orders.cpp: `train <ID>` (PRODUCE of the infantry or unit type, the factory cameo's click), `count <ID>`,
`supers`, `super <index> enemy` (SuperClass::Enable and Forced_Charge, then SPECIAL_PLACE on the strongest enemy house's
first building, which is then watched). The route: the skirmish start, power plants, a barracks and a war factory
placed, a GI and a Grizzly ordered, the nuclear missile (SuperWeapon 0, MultiSpecial; the player's list is MultiSpecial,
IronCurtainSpecial, LightningStormSpecial, ChronoSphereSpecial, ChronoWarpSpecial, ParaDropSpecial,
AmericanParaDropSpecial) launched at the Soviet construction yard. Seen: one more GI (7 to 8) and Grizzly (5 to 6), money
10,000 to 9,100, the missile's hit at about frame 600 (the yard 1,000 to 383, then repaired by the computer); no trap.
Infiltration (a spy or engineer order) is not in the harness yet: the enemy base on this map is about 60 cells away.

**The fault it found:** VoxelAnims (0x008399D0) is declared by the rebuild (`_vanim.h`) but defined by no unit at the
pin (main: `VoxelAnimClass_list_init.cpp`, `_initterm` 0x0040EC40, the one real global among the 41 unmapped `_initterm`
entries; the other 40 are header-static rectangle copies the rebuild's units carry themselves). rb_data.py bound it to
zero storage, so its constructor never ran and the vector had no vtable: the first VoxelAnimClass destroyed
(Process_Deferred_Deletion, VoxelAnims.Delete) faulted at address 0x30 in ALL03U at about frame 3,500.
`glue/vanim_native.inc` defines it (finding for the rebuild). No other zero-bound global has a class type with a
vtable (checked over all 217 in `rb_bss_globals.cpp`).

**iOS (plan item 8), opt in.** `native/rb/tools/rb_ios.sh build simulator|device` configures a src-based iOS build as
`native/build-ios.command --ra2` does (`build/rb-ios-src-<platform>`) but builds only `ra2_port`, `ra2_host_objs` and
`SDL3-static`, then native/rb's CMake in its iOS form (`CMAKE_SYSTEM_NAME=iOS`: the static SDL3, the iOS and SDL
frameworks, the link closure over the iOS SDK, main's `packaging/ios/Info.plist.in`, SDL's licence and the placeholder
icons): `build/rb-ios-<platform>/RBTrial.app`, bundle id io.github.ra2native.RBTrial and home-screen name "RA2
Rebuild Trial", so it never replaces the played app. Both bundles pass main's `packaging/audit_ios_bundle.py` (no game
files, no libraries outside the system); closure 70, the Mac's.

* **Simulator: reaches the main menu** (`rb_ios.sh run <udid>`: installed in the iPhone 17 Simulator, launched with the
  user's install folder as `--data-dir` and the pinned game.exe, read at run time: main menu at 6.4 s), **plays a
  skirmish** (the skirmish route's script, 3,727 frames in 75 s; the Mac: 3,747) and **plays the ALL02S campaign route**
  (3,515 frames in 120 s, the same triggers as on the Mac, the patrol at 2,704), no trap or fault. Extra environment
  reaches the app as `SIMCTL_CHILD_<name>`.
* (The iOS builds were made before the production orders; `rb_ios.sh build` picks them up.)
* **Device: builds, unsigned, not installed.** `rb_ios.sh build device` links an iphoneos arm64 RBTrial.app (platform 2,
  minimum iOS 17.0). To run it on the user's iPhone it needs: (1) signing with the user's team id (RA2_IOS_TEAM on the
  command line) through `native/packaging/ios_sign.py`, whose profile must cover io.github.ra2native.RBTrial (the team
  wildcard profile does; an explicit RA2 profile does not); (2) the user's Red Alert 2 files in the app's own Documents
  (the port's import flow, as the played app; RBTrial's container is separate, so the files would be imported again,
  or shared another way); (3) the pinned target game.exe the trial reads its stored data from (RA2_GAME_EXE on the Mac
  and in the Simulator; the device has no Mac path: the port's per-build reading of the user's installed game.exe, or
  the target game.exe imported beside it, must be wired for the trial as the played app does); (4) a devicectl install,
  which is the user's decision and was not done.

Checks after lane P6: 17 (the 14 before, rb-native-campaign-ALL02S.sav, rb-native-campaign-ALL03U.sav and
rb-native-production). Closure 70 with the parked drafts (Mac and
iOS). Not re-pinned.

Next (lane P6's view):

0. The intermittent ALL02S voxel fault above (ASan loop).
1. More campaign coverage: a forced end (RA2_NATIVE_FORCE_END) to reach the score screens and the next mission's
   briefing, then main's campaign start sweep (24 missions) over the rebuild.
2. iOS: a ctest-like simulator check (`rb_ios.sh run` in a test script with a booted device), touch input through
   the touch layer, then the device questions above.
3. Windows (the other half of item 8) and the frame-identity tests.
4. Item 3 (link/, drafts/ renamed) after the 11-29 flex decisions.

## Heap fault fixed, forced end, campaign chains, iOS checks (lane P7, 2026-11-24, same PIN b873d2b3c6)

**The intermittent fault (P6's open item) was heap damage, not the voxel drawer.** A loop of plain ALL02S loads (14 s each,
four at a time) failed 1 in 12 and then 3 in 24; with a diagnostic on VoxelLibrary::Render_Object's inputs (layer, info
index, every column's span offset inside the library's data) the library was always consistent, and the faults moved:
SIGBUS in Draw_Voxel_Regular_Normals_Lighting, SIGBUS in AStarClass::Find_Path_Hierarchical (a subzone connection's
garbage SubzoneID), SIGSEGV in a Metal command buffer's completion handler. The ASan trial (`RB_ASAN=1 tools/rb_trial.sh`,
`ASAN_OPTIONS=halt_on_error=0:detect_odr_violation=0`; ODR reports for header-static tables like CELL_SLOPE_ANGLE are
expected) showed the writer: **IsometricTileTypeClass::Draw_Tile stored depth values up to 1.7 KB past the end of the
856x736 depth BSurface** (WRITE at isotype.cpp's depth store, plus the reads beside it). Its four "does this span reach the
ring's end" tests compare `(unsigned int)&IsoDrawData.DepthPtr[...]` (and AlphaPtr) with Get_Buffer_End(), which is
uintptr_t natively: the cut address was always below the end, so the wrapping path never ran and a tile crossing the
depth or alpha ring's end wrote past it. Fixed by `fixups.json` (the four casts to uintptr_t). After the fix: ASan reports
nothing in Draw_Tile; **40 of 40** plain ALL02S loads clean (16 s each, past frame 20; before: 3 of 24 failed). The voxel
drawer and A* were victims. A sweep for other pointers cast to 32 bits in the generated units found one more real one,
audevent.cpp's container-of for a volume bucket's node (`(unsigned)event - ...`, audio only), also fixed; the rest are
benign (cdcntrl's DOS registers, unused arguments of SpawnBaseUnits and the empty slot 247 Unknown_Virtual_3DC,
session.cpp's slot loop comparing two cut addresses of one array, cell.cpp's Random_Pick over a tiberium's overlay
addresses, unreached). Still reported by ASan, benign: RadarClass::Init_For_House reads TooltipColor (3 bytes) as a
4-byte TextColor.

**Forced end** (`RA2_NATIVE_FORCE_END=<f1>[,<f2>...][,lose]`, `glue/mainloop_native.inc`, main's harness_save.cpp under
the rebuild's names: PlayerWins / PlayerLoses at the first loop frame at or past each entry; `force-end: win|loss at frame
<f> in <scenario>`). Checks `rb-native-forcewin` and `rb-native-forcelose` (rb-quick, 50 s): skirmish start, forced end at
frame 300, the 50-control score screen, Continue, Back, Back, the main menu again (menu-return at about 31 s).

**Campaign chains** (`rb-native-chain-allied`, `rb-native-chain-soviet`, label rb-chain, about 7 minutes each; main's
tests/campaign_chain.py as the `chain` route): New Campaign through the menus, each mission won by the forced end (an
entry every 200 loop frames), its score screen left by Continue; all twelve missions of each side start and end in order
(ALL01T ALL02S ALL03U ALL04U ALL05S ALL06U ALL07T ALL08U ALL09T ALL10S ALL11T ALL12S; SOV01..SOV12), the ending, the main
menu again: the 24-mission start sweep, no trap or fault.

**iOS Simulator checks** (label rb-ios, opt in: `cmake -DRB_IOS_UDID=<device>` on build/rb-cmake). The Simulator app
rebuilt with the production orders and the fixes above (`rb_ios.sh build simulator`; bundle audit passes, closure 70);
`tools/rb_ios_sim.sh` stands in for the Mac binary, so the same rb_routes.py routes and checks run in RBTrial.app:
`rb-ios-sim-skirmish` (3,782 frames in 75 s), `rb-ios-sim-production`, `rb-ios-sim-forcewin`,
`rb-ios-sim-campaign-ALL02S.sav` (the nine triggers, 3,521 frames), between a setup fixture (boot if needed, install) and a
cleanup (shut down what it booted). No device install and no signing (the user's decisions; P6's list of what the device
needs stands: the team id and a profile covering io.github.ra2native.RBTrial, the game files in the app's Documents, the
target game.exe or a per-build reading of the installed one, a devicectl install).

Checks after lane P7: **27, all pass** in one full `ctest --test-dir build/rb-cmake` run (52 minutes): the 17 before,
rb-native-forcewin, rb-native-forcelose, rb-native-chain-allied, rb-native-chain-soviet, and the six rb-ios tests (setup,
skirmish, production, forcewin, campaign ALL02S, cleanup; RB_IOS_UDID set to the iPhone 17 Simulator). Not re-pinned.

Next (lane P7's view): touch input through the touch layer in the Simulator; the rest of main's 246 width fixups by
pattern (the `(unsigned int)&` and `(unsigned)` pointer casts are now swept; `(int)` pointer subtraction and `DWORD`
parameters remain); a played (not forced) mission past Allied 3 per side, as main's mission soak; Windows; item 3 after
11-29.

## Width sweep and the host's hooks (lane P7, second round, 2026-11-24, same PIN)

**Pointer-width sweep by pattern.** Every unit compiled once with only clang's cast warnings on (`-Wpointer-to-int-cast
-Wvoid-pointer-to-int-cast -Wmicrosoft-cast -Wint-to-pointer-cast`, the units' flags without `-w`): 709 sites, 386 of them
the one VectorClass::ID template. Fixed (fixups.json, the rows after the Draw_Tile one): VectorClass::ID; the open-node
heap's 32-bit pointer bounds (priority.h); **the techno collection helpers' int context (uicmnds.cpp: select-same-type and
the named selections cut a name list's address and read it back as a pointer)**; **Wstring/WideWstring Replace (a wild
pointer built from a cut address)**; audtimer's fail-safe high word; queue.cpp's and session.cpp's loops bounded by cut
addresses compared as signed ints (wrong when the array straddles a 2 GB line, an ASLR slide); **ToolTip::Text, still
`int` in the header while radar/sidebar/tab store a label's address in it**; **WinData's ItemState, RangeMax and
HadTabStop, which hold the list view's row colours, selection states and columns (ArrayList pointers)**. Left, with the
reason: abstype's `%08X` of `this`, radar's and display's hashes, audlists' head marker, house.cpp's type-or-code sign
test, isotype's BaseDepth arithmetic on an int, lcw's alignment masks, the empty slot 247 and unused SpawnBaseUnits and
Attack_Move_Action arguments, PlanningBlockedNode (written, never read back as a pointer), the HWND words through
GetWindowLong (the port's handles fit), cell.cpp's Random_Pick over a tiberium's overlay addresses (unreached; a rebuild
reading to check), and the subsystems no route reaches: VQA (vqalib_*, vqa.cpp; RA2 plays Bink), the network
(netshare, ipx*, netdlg2, nulldlg, gdlgsupp, wincomm), cdcntrl, except.cpp, ownrdraw's Control_Table HWND keys.
Not swept by this method: pointer bytes moved by memcpy/Read with a 4-byte count, unions, and pointers already stored
in int fields without a cast at the store (main's remaining width rows of that kind were not re-checked one by one).
rb-quick (12) passes after the sweep, and again with the hooks below.

**The host's game-side hooks** (the feature audit: almost every hook the host calls is a weak symbol in
platform/sdl/host.cpp, so a missing one never shows in the link closure; the trial silently had the defaults). Attached
in `glue/mainloop_native.inc`: `ra2port_game_redraw_pending` (**it was a link-closure trap: a focus change while a menu
dialog was shown aborted the trial**; closure 70 to 69), `ra2_native_network_game`/`leave_network_game`,
`ra2_native_save_settings` (Cmd+Q), `ra2_native_hide_game_cursor` (touch), `ra2_native_scroll_map` (trackpad/touch pan,
ignored in locked missions: N31), `ra2_enh_in_play` at the seam (the split layout, touch play mode and pinch need it),
and checkpoint saves (Scout1248, CHECKPT.SAV when the host asks). Still missing (the audit's list, by size): the shell
Options dialog (`Main_Options_Dialog_Proc` is `#if 0` in the rebuild's mainopt.cpp, a trap: Options and the
Enhancements page abort); map zoom and its gestures; interface size; the command bar collapse/reopen and flash (N31);
in-game dialogs at window size; Queue Five; Modern controls; HD vehicles; Mac widescreen; the NewEdit/chat keyboard
hooks and the team-button hold (touch); the menu panel centred above 800x600 (Scout1274, iOS sizes); Draw_Char's
negative row offset (C17); the primary-lock MouseMutex (C23) and Restore_Check's refusal while inactive; LAN pacing;
mod subtitles. Carried already: N30 (audevent), the original-format save bridge, resume at start, the Exit Game
delete form, the audio driver. The YR port's `yr/glue/yr_*.cpp` are near drop-in templates for most of them.

## Options, the feature audit closed, main's tests on the trial, frame identity, touch in the Simulator (lane P8, 2026-11-24, same PIN)

**The shell Options dialog and the Enhancements page.** `Main_Options_Dialog_Proc` (`#if 0` in the rebuild's mainopt.cpp, a
link-closure trap: Options aborted) is carried as that text compiled natively (`glue/mainopt_native.inc`, checked against main's
behaviour row `link/behaviour_m22_options.cpp`), with the Enhancements button (0x7F80, listed among 0xD5's column buttons so it takes
its slot) and the page (main's `link/native_enhancements.cpp` under the rebuild's names).

**The feature audit's missing items, attached** (each a `fixups.json` row at main's site, the glue named):

| Item | Where | Check |
| --- | --- | --- |
| Touch keyboard: NewEdit fields, chat line | `glue/ownrdraw_native.inc`, `mainloop_native.inc` | (host hook; no route types into them) |
| Team-button hold (Y15) | `glue/tab_native.inc` | rb-main-touch-team-hold, rb-ios-sim-touch-team-hold |
| C17 Draw_Char guard (the rebuild's Pitch is already signed) | bitfont.cpp rows | (report-and-skip only) |
| C23 primary lock under MouseMutex | dsurface.cpp rows, `glue/wwmouse_native.inc` | (cursor-stress not run) |
| Restore_Check's refusal while inactive | dsurface.cpp row | |
| Scout1274 menu panel centred above 800x600 | txtlabel2.cpp rows | |
| LAN pacing (Sync_Delay, Wait_For_Players) | mainloop.cpp, queue.cpp rows | (no LAN route) |
| Mod subtitles | already the rebuild's own: `SubtitleTokens`/`SubtitleAlignments` are compiled source, not original data | |
| Map zoom, zoom gestures (wheel, pinch), render scale 2 | `glue/zoom_native.inc` + display/scenario/saveload/sidebar/radar/tab/wwmouse/cctooltip/tactical/startup/mainopt rows | rb-main-zoom-steps, -pinch, rb-ios-sim-zoom-pinch |
| Game interface size | the same glue; surfaces at start-up, mode change, load and new game | rb-main-zoom-menu-ui, rb-main-locked-input |
| Command bar collapse/reopen and flash (N31) | txtlabel2.cpp rows | rb-main-command-bar-thumb |
| In-game dialogs at window size; score screen and Abort at render scale 2 | conquer.cpp, mainloop.cpp rows | rb-main-zoom-menu, -menu-ui, -score, -abort |
| Queue five | `glue/controls_native.inc`, sidebar.cpp rows | rb-main-controls-queue-five(-off) |
| Modern controls (and Classic's tap across houses) | the same glue, scroll/display/radar rows | rb-main-controls-modern/-classic, rb-main-touch-* |
| HD vehicles 2x/3x | `glue/unit/hd_vehicles.cpp` (the YR port's glue/yr_hd_vehicles.cpp: the same TibSun names), blit/voxdrsys/voxlib/techno/dsurface rows | rb-main-hd-vehicles, -hd-vehicles-3x |
| Mac widescreen | `glue/ownrdraw_native.inc`, init.cpp and static-movie rows | rb-main-widescreen |

Still not carried: nothing from the audit list. Not checked by a route: the NewEdit/chat keyboard hooks, C23 (main's
cursor_stress.py not run), LAN pacing (main's lan_pair.py not run), the HD air-base case (the wrapper's Patriot is placed off the
view), enhancements_newgame.py (the page works and the next game makes its 1.25x sidebar surface, but main's click lands one dropdown
row higher on the trial).

**Main's own tests on the trial** (label rb-main, 24 checks: `tools/rb_skirmish_wrap.py` stands in for the binary and starts
their `--skirmish` game through the menus; `glue/harness_input_native.inc` answers their `@` points and harness-state lines; a
`--skirmish` run plays one game; RA2_NATIVE_SKIRMISH_SPEED=3, RA2.INI's speed, which main's harness keeps). They found five more
width faults, fixed: the replacement list box's TextNode item data (the Save/Load lists' FileEntryClass pointers: the in-game Save
screen faulted), Get_Item_Text's int node and its ListItemData view (every dropdown's open list), the dropdown opening one row tall
(the port's dropped height, `combo_height`), and the Enhancements button's column slot.

**Width sweep leftovers (item 5).** Main's width rows were checked one by one against the rebuild text (about 65 already fixed by
rows here, about 200 safe in the rebuild's own types, about 22 unreached, 5 not found); the ones that still needed it are rows now:
the LAN lobby slot combos (NodeNameType pointers in combo data), Random Map's scenario item, Extras > Movies (Dialog_Box's
result, the movie list's item data), StaticRecord and ScrollBarRecord laid out from WinData's own offsets (with WinData's movie
fields pointer-sized), a leftover raw 0x1FC offset, the atan table's index wrap, the in-game movie subtitle record. No 4-byte-count
copy moves a pointer; the only pointer/int union (EventClass) is typed.

**Frame identity** (`tests/rb_identity.py`, main's soak digest in both: RA2_NATIVE_SOAK=50, SOAK_SEED, SOAK_SPEED=0, SOAK_END;
Allied 1 from New Campaign, orders `+30 deploy *; +600/600 hunt`, the src-based port `build/release-mac-one/ra2-native` against the
trial). Found and fixed on the trial's side: its seed was set at Start_Scenario's top, after Select_Game's Init_Random (the trial's
own runs differed from each other); now at Select_Game's top as main's seam. After that the two ports give the same digest (every
techno's type, house, coordinate, strength, mission, the money and the random generator) at every 50 frames through frame 900. **First
divergence: game frame 938**, a Soviet conscript (E2, house 1) that appears at frame 937 at 16832,22464: its first step goes straight
south in the trial (16832,22473) and one pixel west of south in the src-based port (16831,22474); the random generator is still equal
at frame 950. The cause is in its facing (Direction -> fastmath::atan2/atan2f -> DirType::From_Radian); From_Radian's quotient
(the rebuild) against main's product was tried and is not it. Not fixed: the next step is a dump of that conscript's HeadToCoord
and facing in both.

**Checks at the lane's head: 57** (`ctest --test-dir build/rb-cmake`, RB_IOS_UDID set): the full run passed 54; the three failures
were rb-native-production and rb-ios-sim-production (a harness fault this lane exposed: `train` used EventClass's 4-argument PRODUCE
form, which leaves the naval flag uninitialised, so with the harness's new stack layout the Grizzly went to the naval queue; the
5-argument form with FALSE fixes it) and rb-main-hd-vehicles-3x (passed alone; it timed out under load). All three pass on rerun.

**Touch in the Simulator** (label rb-ios): main's touch tests run in RBTrial.app (`RB_TRIAL=tools/rb_ios_sim.sh` behind the wrapper):
two fingers, A-Move, Modern, the team-button hold, the second-tank taps (Classic and Modern), Allied 2's taps across houses and the
pinch zoom all pass in the iPhone 17 Simulator.

## Frame identity, the sidebar frame, LAN, the original's field-globals (lane P9, 2026-11-24, same PIN)

**Frame identity** (`tests/rb_identity.py`; `--reuse-src <dir>` reuses an earlier src-port run of the same route). Allied 1 and
Soviet 1 now give main's soak digest at every 50 frames **through frame 5,000** in both ports (ctest rows rb-identity-allied and
rb-identity-soviet, label rb-identity). The causes, each found by dumping the step's inputs and outputs beside main's formula:

| Divergence | Cause | Fix |
| --- | --- | --- |
| Allied 1 frame 938, a conscript's first step straight south | atan2f's `x == 0` answer `M_PI/2` converted to float at run time under the game's round-toward-zero mode (-frounding-math): 1.57079625 against VC6's folded 1.5707964f, so the facing came out 0x7FFE, not 0x7FFF | the units are built with `-fno-rounding-math` (`tools/rb_probe.py`): constant expressions fold at compile time to nearest, as VC6 folded them; run-time arithmetic still rounds toward zero (FPCR) |
| (same path, latent) | the fastmath bodies narrowed their argument to float and divided or multiplied in float, where the original keeps the x87 value unnarrowed (main's rows 378-384) | `fastmath.cpp` rows and `glue/fastmath_native.inc`: sin/cos/asin/acos/atan on the double, atan2f's quotient in double through `_ftol` |
| Allied 1 frame 1606, a cargo plane's descent | `DirType::From_Radian`: the double-to-short conversion of a value past 32767 (west of south) is undefined in clang and, inlined, the following widening was folded away (As_Radian read 49150, not -16386); and the header divides where the original multiplies by -10430.06 (0x0079B628) | `hdr/face.h` row: the product, through `int`, then the low 16 bits |
| Soviet 1 frame 215, a paradropped conscript | harness only: the trial's digest hashed `Get_Mission()` (the queued mission while none is current), main's the field | `glue/harness_input_native.inc`: `CurrentMission` |

Also a row for the divisions the original makes products (`/ 100.0`, `/ 10000.0`, `/ 1000000.0`, `/ 11.0`, `/ 50.0`: the image has
only fmul by their reciprocals). **Skirmish: not identical yet.** The route's start through the menus is not repeatable (about
half the runs start a different game: 52 technos at frame 50 instead of 60), and in the runs that start the same game the digests
agree through frame 3,950; at 3,960 the computer player builds a radar where the src port builds a sentry gun: its defence node
was dropped earlier in the trial (AI_Build_Defense and Where_To_Place_Building are parked drafts in the rebuild, exact in main).

**The sidebar's frame was never drawn** (every game since lane P1: SIDE1/SIDE2/SIDE3/ADDON missing, the strip column black). The
rebuild's `SidebarClass::IsSidebarRedrawPending` (0x8373C3) is in the original the byte `Map.IsForceCompleteRedraw` (0x8324E0 +
0x4EE3): Add_Buttons and the rest set it by address, Draw_It reads the member. Natively they were two variables. The same pattern,
fixed the same way (each global a reference to its member): `Session_Busy` = Session.NetOpen (the LAN host faulted: the lobby's
dialogs ran Main_Loop), `IsCameoRefreshPending` = Map's SidebarClass::IsToRedraw, `AllowDisableableSupers` =
Session.Options.SWAllowed (no unit wrote the global), `Game::PlayerColor` = Session.PrefColor, `MuteSWLaunches` =
Session.MultiplayerObserver, `DiplomacyChatFlags` = Session.PlayerChatEnabled, `Online_Session_State`/`Return` = Session +0x0C/+0x10.
Still to check by the same test (a bound global's address inside Map 0x8324E0..0x8373C8): MapSizeX/Y (Map +0xF0), CurrentSuperWeaponType
(+0x11B4), SidebarMovieX/Y (+0x11EC), SidebarClass::DirtyRect (+0x1208); the rebuild's headers name no member at those offsets.

**Main's tests.** cursor_stress.py (C23): passed (3,600 frames, HD 2x, zoom 1.5x, two backgrounds). hd_vehicles.py --case air-base:
passed; the wrapper's Patriot (the third build-list entry) now goes below the yard on screen. lan_pair.py: passed (18,000 frames
with both CRCs, none different; the Sound dialog twice; network edge scroll 13 pixels a frame against the skirmish's 26; ctest
rb-main-lan-pair). It needed Session_Busy, the two Westwood Online channel tests answering false in `glue/rb_behaviour.cpp`, DebugString
printing under RA2_NATIVE_DEBUG_LOG and the CRC log. enhancements_newgame.py: the trial's dropdown rows are taller because the
trial follows the original: Combo_Proc's OD_SUBCLASSED sets CB_SETITEMHEIGHT -1 to LineHeight + 2 and 0 to LineHeight + 6
(0x005F6460-0x005F6496); main's behaviour row (link/behaviour_m14_ownerdraw.cpp) omits both, so main's port keeps user32's 16 and
the test's click positions are main's. Not changed here; a main-port gap.

**Static controls** (window dumps through the shell in both ports, 500 ms apart): the main menu frames are identical; the campaign
briefing with its typed objectives is pixel-identical once typed; the side choice's movie and the menu animations differ only by
frame timing; the skirmish setup differs only by the combo heights above. Two src-port findings from the comparison: its Map
object overruns its native storage into the globals after it (Draw_It's IsForceCompleteRedraw is main's
SidebarClass_IsSidebarRedrawPending by accident, which is why main draws the frame), and its CommandNames table loads in another
order (AttackMove, Guard, Beacon, Deploy at 4-7 against the image's Deploy, AttackMove, Guard, Beacon), so main's command bar shows
the beacon and attack-move icons where UI.INI puts Deploy and Guard. The trial's bar follows the image.

**Checks at the lane's head: 61, all passed in one full run** (`ctest --test-dir build/rb-cmake`, RB_IOS_UDID set; 91 minutes):
the 57 of lane P8, rb-identity-allied, rb-identity-soviet, rb-main-lan-pair and rb-main-hd-air-base.

## Lane P10 (2026-11-24, same PIN): paused for usage pacing; resume note

**Item 2, the repeatable skirmish start: done.** The cause: the setup's sides and colours default to Random (-2). Start Game
resolves them (`SessionClass::Assign_Random_Choices`: `Random_Side`, `Random_Color`) from `Scen->RandomNumber` before
Select_Game's `Init_Random` applies CustomSeed. That generator still holds `Init_Game`'s own `Init_Random` seed, which is
`GetTickCount()`, because CustomSeed is still 0 at that point. So each run drew its own human and computer sides (Americans or
Arabs, ...). Both ports behave this way. The fix is in the test only (`tests/rb_identity.py`, neither port changed): each
skirmish run's write folder gets the installed RA2.INI plus `[MultiPlayer] Side=Americans, Color=0` and `[Skirmish]
Slot01=6,8,1` (the computer is Russians), which the original reads (`Read_MultiPlayer_Settings`, `SessionOptionsClass::Read`);
`--sides human,computer` changes them. Checks: five short starts across both ports gave the same frame-50 digest, and one full
run gave **every digest the same through frame 5,000** (src and trial). The new ctest row `rb-identity-skirmish` is in
CMakeLists.txt; it has passed once and not yet in a full ctest run. With these sides P9's divergence at frame 3,960 (radar
against sentry gun) does not appear. Other side pairs (Arabs,6 / Russians,0 / Africans,7, 8,000 frames) were started but
stopped by the pause before finishing.

**Item 1, the overlay: partial (classification only, no text carried yet).** `data/overlay_candidates.csv` lists 194
functions. Each is parked or not exact in the rebuild at the PIN (`rebuild/manifest/tail_buckets.csv`) and exact in main
(`manifests/original_target/modules`). By bucket: exact_when_open 63 (behaviour-safe), inline_call 40, no_draft 29,
register_order 18, calls_differ 15, inline_body 15, unbound 9, stack_layout 3, not_emitted 1, text_if0 1. Wonline rows are
not compiled in the trial, and World Domination Tour is probably unreached. Line-by-line comparison against main's text
(behaviour, not bucket) so far:
* map and drawing batch, all A (behaviour-identical): Destroy_Bridge2_Near, Destroy_Low_Bridge_Near, Repair_Bridge,
  Pick_Random_Tile_Variant, Register_Zone_Connection_Entries, Zone_Span, Create_Region_From_Cell, Init_Voxel_Matrices,
  Techno_Render_Voxel_Object, LaserDrawClass::Draw_It, Draw_Post_Render.
* Where_To_Place_Building: probably B, needs a check. The trial's facing comes from `TVelocity2D::Get_Yaw` and `As_Facing`;
  main's is `(unsigned short)((atan2 - pi/2) * -10430.06)` and `Dir_To_8`. The reverse step is `Facing_Sub(facing, 4)`
  against main's `Adjacent_Cell(Cell, int)` with `facing - 4`.
* Not yet compared: the house, team and trigger batch (HouseClass::AI, AI_Build_Defense, TeamClass::AI, Suggested_New_Team,
  Do_Reinforcements, Discharged, TActionClass::operator(), Do_MISSION_GUARD, Captured, Docking_Coord, Take_Damage,
  BuildingLightClass::AI, Get_Draw_Rect); the combat and movement batch (Fire_At, Explosion_Damage, Railgun_Beam_Damage,
  Shrapnel, Coord_Scatter, Nearing_Target, Hover and Rocket Process, While_Moving, FootClass::AI,
  Search_For_Tiberium_Weighted, Start_Driver, Gas_Behavior_AI, TechnoClass(NoInitClass), WaveClass::Load); the UI and
  start-up batch (Init_Game, WinMain, Allocate_Surfaces, LoadFile, LBSaveSelections, Set_Slot_Names, Presentation,
  SidebarClass::AI, Reposition_Sidebar, Select_By_*, Type_Select_Key_Up, planmgr, Dropship_Screen, Shade_Text_Area,
  WDT_Write_Map_Ini). Each is to be read beside main's text; for each B, carry main's text (a `cut_function` row plus a glue
  `.inc`, or a local fixup).

**Item 3, the field-alias globals: swept, not yet fixed.** Every name bound in the rebuild catalog (`catalog/populations`
references) was checked against the i686 extent of every class-typed global. The extents are the `[sizeof=]` values in the
gen `original_layouts/*.i686.txt` dumps: MouseClass 20,284 (0x4F3C, so Map runs 0x8324E0 to 0x83741C, not 0x8373C8),
SessionClass 12,420. The hits beyond P9's ten:
| Global | Address | Member (i686 offset, rebuild name) |
| --- | --- | --- |
| MapSizeX / MapSizeY | 0x8325D0 / D4 | Map +0xF0 / +0xF4: MapClass::PlayRect.Width / .Height. Natively never written, so mainloop's force-attack scroll is 0 |
| CurrentSuperWeaponType | 0x833694 | Map +0x11B4: DisplayClass::IsTargettingMode. super.cpp's chrono-warp second stage writes the global |
| SidebarMovieX / SidebarMovieY | 0x8336CC / D0 | Map +0x11EC / +0x11F0: RadarClass::RadHeight / RadOffX (the members at those offsets, whatever the names) |
| SidebarClass::DirtyRect | 0x8336E8 | Map +0x1208: RadarClass::LastDrawRect |
| IsCameoTextShown | 0x8373C0 | Map +0x4EE0: SidebarClass::IsCameoText (new; not on P9's list) |
| PacketRouterConnection | 0x00A409C4 | Ipx +0x4C: IPXManagerClass::LobbyChannel (IPXGlobalConnClass *; main: "compatible") |
Each should become a reference to its member, as P9's rows do. For protected members, take a pointer to member through a
derived accessor (`struct A : RadarClass { static int RadarClass::* p() { return &A::RadHeight; } }`). Nothing else is
inside an extent. Scen and Rule are heap pointers, so nothing can alias into them by address. g_PlayerReady and
g_PlayerChecked are in wonline only.

**Item 4, identity and full ctest at the head: not run.** Only the skirmish identity above was run.

## Lane P11 (2026-11-24, same PIN): field aliases, the overlay compared, identity on six routes, device plan

**Field-alias globals: fixed** (`fixups.json`, the rows marked lane P11). Each name P10's sweep found inside Map or Ipx is now a
reference to its member, as P9's rows: MapSizeX/Y = Map.PlayRect.Width/Height (+0xF0/+0xF4; Main_Loop's force-attack scroll was
0), CurrentSuperWeaponType = Map.IsTargettingMode (+0x11B4; super.cpp's Chronosphere second stage wrote a variable the display
never read), SidebarMovieX/Y = Map.RadHeight/RadOffX (+0x11EC/+0x11F0), SidebarClass::DirtyRect = Map.LastDrawRect (+0x1208),
IsCameoTextShown = Map.IsCameoText (+0x4EE0), PacketRouterConnection = Ipx.LobbyChannel (+0x4C; a friend struct reads the private
member; ConnectionClass is the primary base, so the pointer is the same value). The Map references are defined in `_map.cpp`
beside Map, IsCameoTextShown in `sidebar.cpp`, PacketRouterConnection in `globals.cpp` after Ipx. Lane N33's three in main's port
(PendingObjectPtr/PendingObject 0x833680/84, RadarScale 0x833968) need nothing here: the rebuild declares them as Map's members
(display.h, radar.h) and no unit names a global at those addresses.

**The overlay: compared.** `data/overlay_candidates.csv` has a `behaviour` column for all 194 rows. Line-by-line against main's
exact text (P10's map and drawing batch, then this lane's three batches, two of them read by read-only helpers whose B findings
were checked here):

| Group | Rows | Result |
| --- | ---: | --- |
| map and drawing (lane P10) | 11 | all A |
| house, team, trigger (Where_To_Place_Building, AI_Build_Defense, HouseClass::AI, TeamClass::AI, Suggested_New_Team, Do_Reinforcements, Discharged, TActionClass::operator(), Do_MISSION_GUARD, Captured, Docking_Coord, Take_Damage, BuildingLightClass::AI, GetItem) | 14 | all A |
| combat and movement (Fire_At, Explosion_Damage, LaserDrawClass::Draw_It, Shrapnel, Coord_Scatter, Nearing_Target, Hover/Rocket Process, Ship/Drive While_Moving, FootClass::AI, Search_For_Tiberium_Weighted, Start_Driver, Gas_Behavior_AI, TechnoClass(NoInitClass), WaveClass::Load, Railgun_Beam_Damage) | 17 | 15 A, **2 B, main's text carried** |
| UI and start-up (Init_Game, WinMain, Allocate_Surfaces, LoadFile, LBSaveSelections, SidebarClass::AI, Select_By_Health/Rank, Shade_Text_Area, Dialog_On_WM_PAINT, planmgr's node and filled-techno draws) | 13 | all A |
| not line-compared | 139 | exact once the unit is open 48; compiler-generated, template or glue 36; wonline (not compiled) 26; World Domination Tour (not reached) 20; codegen-only buckets (register order, stack layout) 6; planning-mode draws 2; Dropship_Screen (only with Scen->StartingDropships) 1 |
| unsure | 1 | Enable_Animated_Control (the dialog control enable test: main's helpers not expanded; the dialogs matched in P9's window dumps) |

The two B, each now a `fixups.json` row carrying main's exact form under the rebuild's names:
* **Coord_Scatter** (parked draft): the lock snap divided (`X / 256 * 256 + 128`, toward zero) where the original subtracts the
  low byte (`X - (X & 255) + 128`, down). A negative X or Y (-255..-1 passes the bounds test) snapped a cell off: a locked scatter
  at the map's top or left edge.
* **Fire_At** (judgment-call row): the flak-scatter branch's length summed X*X+Y*Y+Z*Z in float (Vector3::Length) where the
  original sums each float as double, Z first; past about 4,096 leptons per component (int)length could be one lower.

Where_To_Place_Building and AI_Build_Defense, the two P9 suspected: both A. The draft's `TVelocity2D(-x, -y).Get_Yaw()` and
`As_Facing` give main's `atan2(y, -x)` product and `Dir_To_8` (the junk upper half of DirType::Raw drops out of every rounding),
and `Facing_Sub(facing, 4)` equals main's `Adjacent_Cell(cell, facing - 4)` (main's overload masks `& 7`); AI_Build_Defense's five
bytes are operand order only. Helper notes worth keeping: rebuild/manifest/behaviour_notes.csv's behavior_diff note on
Do_MISSION_GUARD describes an older draft (the current one compares A); the rebuild's TVelocity3D::Set_Speed tests only X and Y
for zero where main tests X, Y and Z (Shrapnel cannot reach the difference; other callers could).

**Identity: six routes identical through frame 5,000** (both ports, main's soak digest every 50 frames): Allied 1, Soviet 1, the
default skirmish (Americans against Russians), and the three side pairs P10 started, now ctest rows `rb-identity-skirmish-Arabs`
(Arabs against Arabs), `-Russians` (Russians against Americans), `-Africans` (Africans against Confederation). The six were run
after the alias fix and before the two carried rows, and again in the full ctest after them.

**Full ctest at the lane's head: 65 checks, 64 passed in one full run** (`RB_IOS_UDID` set; 104 minutes): the 62 of lane P10
and the three new side-pair rows. The one failure, rb-main-hd-vehicles ("2 pictures showed the units at 1x after one at 2x
(flicker)", with 2 dropped pictures in its third count), passed in both re-runs straight after; it is a timing check of the HD
units path that none of this lane's rows touch. rb-main-lan-pair passed with PacketRouterConnection now Ipx.LobbyChannel.

**Device: planned, not built at the head, not installed.** The trial app links main's host objects built with `RA2_NATIVE_GAME`,
so it has the played app's app mode: on the phone it asks for the game folder once (the Files picker), copies the files into
`Library/Application Support/RA2Native/GameData`, and points the port at the copied game.exe (`main.cpp` sets RA2_GAME_EXE from
`setup_imported`). The port reads the user's installed game.exe through its per-build table (`ra2port/game_builds.hpp`): the
install here is the 1.006 signed/no-CD build (4,489,184 bytes, SHA-256 7bb2c7e1...), read at the target's addresses. So the
phone needs no Mac path and no copy of the target game.exe. Shown in the Simulator by the new `tools/rb_ios.sh run-imported
<udid>`: a fresh container, no --data-dir and no RA2_GAME_EXE, RA2_NATIVE_IMPORT standing in for the picker: 21 files (1.1 GB)
imported, "another build with the target's data: read at the target's addresses", 168 globals from game.exe, the main menu
(`progress: menu`); the copy is removed afterwards. What the device needs (P6's list, updated):
1. the user's team id (RA2_IOS_TEAM) and a provisioning profile covering io.github.ra2native.RBTrial (the team wildcard
   profile does; the played app's explicit profile does not), signing through `native/packaging/ios_sign.py`;
2. the game files once, through the app's own import (RBTrial's container is separate from the played app's, so they are
   imported again, about 1.1 GB, or copied with `xcrun devicectl device copy to ... --destination Documents/<folder>` and
   imported with RA2_NATIVE_IMPORT=<folder>);
3. nothing for game.exe beyond that: the imported game.exe is read per build (target, 1.006 signed/no-CD or Steam);
4. the install (`devicectl`), which is the user's decision.
The unsigned device binary was not rebuilt at this head: free space was 28 to 29 GiB, under the lane's 30 GiB floor for starting a
build. `rb_ios.sh build device` makes it; `build/rb-ios-device/RBTrial.app` is
lane P7's (2026-11-23), without lanes P8 to P11.

Process note: one stray `rb_ios.sh` call with no arguments rebuilt the Simulator app a second time (same sources) while free space
was 29 GiB.

**Before the next re-pin** (owner, 2026-11-24): the rebuild becomes the tree of record today and its sources lose the bare
trailing `// RA2: as above` markers (`private/tools/strip_as_above.py`, pattern `[ \t]*// RA2: as above[ \t]*$` per line). About
52 rows of `fixups.json` (56 occurrences, in globals.h, conquer, gdlgsupp, init, mainloop, object, ownrdraw, sidebar, txtlabel2 and
voc) carry the marker in their old/new text. When re-pinning to a commit after the switch, apply the same pattern (per line,
multiline mode) to those rows' `old` and `new` strings first, or rb_units.py stops with "fixups that match nothing". The pin
b873d2b3c6 is unaffected; this lane's rows carry no marker.

## Pointer-width and run-time faults so far (plan item 7; lane P6's list, 2026-11-23)

Every fault the rebuild text has shown under arm64 clang, by pattern, each fixed by a `fixups.json` row (index in
brackets) or a glue file, the why on its row. Main's 246 width fixups were the patterns to look for; these are the
ones the played routes reached. The rebuild is unchanged by all of them (findings only, no edit there).

| Pattern | Where (fixups.json rows) | Found by |
| --- | --- | --- |
| Pointers kept in 32-bit ints inside records | WinData's drawer and backdrop shapes (80-84), the replacement list box's GWL_USERDATA words (85, 86), IsoBlitState's source pixel and depth (106, 107), ToolTip::Text (56) | main menu paint, list boxes, the first terrain frame, radar tooltips |
| Raw 32-bit offsets into a record whose pointers grew | WinData fields reached at `(char *)data + 0x24...0x1FC` (88-93), TextNode's 0x10 header (95), NodeList's view of WinData::Nodes (96) | dialogs, tooltips, ~WinData |
| Addresses folded through `unsigned int` arithmetic | the ABuffer and ZBuffer rings (40-55, 58-69), audmpool and audstrm (71-75), RLE_Blit's buffer positions (108), the wave draws (109, 110) | the first skirmish frame, the sound pools |
| Pointers passed as `int` / `DWORD` arguments | the notice interfaces (97-103), the radio parameter (`intptr_t &` in Receive/Transmit_Message, LParam: 114-120), the VQA timer callback's user value and handle (4, 6), the list box `wparam` overload (27) | the loading screen, transports and docking, movies |
| Varargs of a class through `%s` | Wstring in debug prints (22, 23) | compile |
| Saves: VC6's 4-byte pointer as the stream's width | LayerClass::Save's `sizeof(int)` against Load's pointer (130), LZO1X-1's pointer dictionary (129), the swizzle ids and Process_Tables (131-146), the object's own id written as `this` (147-152), ObjectClass::Load's swizzle of the int at AbstractClass +0x18 (157) | save and original-save routes |
| Raw images read through `void *` | LocomotionClass::Load (153), Static_Sounds_Load's unnamed record (154) | the user's original saves |
| Two declarations of one record or global, distinct natively | message 0x4E9's TextRequest/ToolTipTextRequest (87), ScorePalette (94), SaveVersionInfo/SavegameInformation (`glue/savever_native.inc`), Is_Weapon_Valid/Is_Weapon_Present and slot 247 (`glue/techno_native.inc`) | dialogs, score screen, saves, selection |
| Pointers cut to 32 bits for a comparison or container-of | Draw_Tile's four ring-end tests against Get_Buffer_End (lane P7: tiles wrote past the depth surface; the intermittent heap faults), audevent's node-to-event step (lane P7) | ASan over ALL02S loads |
| Allocation mismatches VC6 tolerated | Free_Dialog_Palettes' `delete []` of a scalar `new` (124) | the score screen |
| Declared but defined by no unit, so bound to zero storage and never constructed | VoxelAnims (`glue/vanim_native.inc`, lane P6): the vector had no vtable; the first VoxelAnimClass destroyed faulted at address 0x30 (Process_Deferred_Deletion) | the ALL03U campaign route |
| Arithmetic that traps on VC6 and not on arm64 | the swizzle walk's deliberate 1 / 0 (146: the walk bounded, the failure reported) | original saves |
| Platform macros | base64's `BIG_ENDIAN` test (Darwin defines it) | the MIX header keys |

Still to sweep (not reached by any route yet): the rest of main's 246 width fixups by pattern against the rebuild's
text, the multiplayer and Westwood Online paths (wonline left out), and the campaign missions past Allied 3.

## The Enhancements page, list boxes, device parity (lane P12, 2026-11-24, same PIN, in redalert2-modern)

From the user's iPhone Duo report and a code comparison with the played port (redalert2-decomp native/). The host and
port layers (`platform/`, `port/`) are byte-identical to the played port's apart from SPDX lines, so every difference is
in the game-side layer (`ra2/fixups.json`, `ra2/glue`) or a parked draft of the rebuild. Fixed (each a `fixups.json` row
naming main's row):

| Item | Cause | Fix |
| --- | --- | --- |
| Enhancements page: Cancel a grey push button in the page body | 0x005E72C0's bottom-slot list lacked 0x7F03 (main's C7 row) | Is_Default_Button lists 0x7F03 |
| Dropdown lists: no frame, rows left on screen after closing, arrow never pressed | parked Combo_Proc: no parent repaint, Win32 dropped state not set | open/close repaint the parent and set the state (main's case 0x14F) |
| Dropdown click sound | the draft played GenericClick | GUIComboOpenSound (Rules +0x19C, 0x005F5DF2) |
| Fullscreen "Automatic" dark red on touch hosts | the draft (and the original) use Disabled_Text_Color | Light_Grey as main's port (owner ruling: the played look; not the original's) |
| Internet button live | main's C20 row not carried | disabled at the main menu's creation |
| Every list box empty (Load/Save/Delete, Keyboard commands, ...) | the parked List_Proc stops when GetUpdateRect reports no region; the port reports none | the client rectangle when none is reported |
| Column lists (Load/Save/Delete) drew no text | the draft drew each cell in a 0x0 rectangle, which clips the font | the column's width by the row's height |
| Game interface size could write outside a surface | main's C6 Prep_For_Blit window clip not carried | carried (xsurface.cpp row) |
| LAN UniqueID could collide between devices | main's C20 install id not carried | carried (session.cpp row) |
| Load error left nothing to click | main's R684 row not carried | the list is shown again |
| 16 ms frame floor at every game speed | main applies it at speed 0 only | Options.GameSpeed > 0 skips it |
| Pinch zoom off on iOS (so no 0.5x map zoom) | every setting defaults off on every platform, in both ports; the played app's On is the user's saved choice | iOS starts with Pinch zoom on (`platform/sdl/enhancements.cpp` default_value; the user's word via the owner) |
| iOS home-screen name | "RA2 Rebuild Trial" | "RA2" (bundle id stays io.github.ra2native.RBTrial until parity) |

Kept, following the original (each evidenced): dropdown and list row heights (Combo_Proc 0x005F6460, List_Proc
0x005F6B4D), the list's filled selection bar (0x005F6DA6) and the Load list's date/time columns (Fill_List's OD_SETCELL).
Not reproduced: the played app's 976x720 on the Duo; the same code gives 1024x720 for 944x634 usable points at interface
1.25x (`main.cpp` game_resolution and the interface-size narrowing are identical in both ports); the played app's
"app: usable ..." log line on the device would settle it.

**Shell frames check** (`ra2/tests/rb_shell_frames.py`, ctest rb-shell-frames-mac and -iphone, with RB_IDENTITY_SRC): both
ports through main menu, Options, Enhancements, Keyboard, Network, Single Player, Load and the skirmish setup, compared in
8x8 tiles outside the listed allowed areas. It found the two list box faults above.

## Yuri's Revenge, the combined app, Windows (lane P13, 2026-11-24, same PIN, in redalert2-modern)

**Vendored**: `vendor/ra2-decomp` (gibbo101/ra2-decomp 8719494727, whole) and `vendor/tibsun` (TibSun dbfbfbfaac, the headers
ra2-decomp falls back on), copied unchanged from redalert2-decomp's `yr/` at 887b1a2e11 by `tools/update_yr_vendor.sh`, each
with a PIN of digests; `check_tree.py` now checks every vendor folder's PIN. **Imported**: native/yr (yr.cmake, combined.cmake,
combined/, fixups.json, glue, data, tools, tests) and native/cmake (the Windows toolchain file and windows.cmake), re-pathed.
Yuri's Revenge uses none of Red Alert 2's game code; its build is the old port's.

**The combined app's Red Alert 2 engine is the rebuild-based game**: `ra2-engine` (Mac bundle, Windows DLL) and the iOS
one-executable engine object link rb_units_objs, rb_data_objs, rb_glue_objs and the rebuild's closure, after rb-native-trial's
PRE_LINK has weakened the duplicates and moved the units' initialisers to `__DATA,__rb_init_func` (Windows: `.rbini$m`, run
between `rb_host_glue.cpp`'s `.rbini$a`/`.rbini$z` markers). On iOS the engine object's remaining initialisers move to
`__ra2e_init` and the thin host resets the engine's `__data`/`__bss`/`__common` span before each later start, as for YR; the
rebuild's `__rb_init_func` list then runs again after the game.exe load. SDL and the Apple host are linked once.

Fixups (ra2/fixups.json):
* the 0x7F5E toggle as a main-menu sidebar button in Assign_Button_Kind (main's port row 499, lanes C23/Y11);
* Owner_Button_Proc's pressed state also from BST_PUSHED (main's Paint_Button): the port sends no WM_DRAWITEM, so no push
  button of the rebuild port ever drew its pressed frame (found by one-game-toggle, "the toggle shows no press");
* crc.h: a `long` overload on Windows (RA2_LONG is long there; every `crc((RA2_LONG)x)` was ambiguous).

**Windows**: `rb_probe.py --target windows` gives the units the YR units' Windows form (yr_win_first.h and dialect_win.hpp
first, the rewritten headers by -isystem); the generated data sources are compiled from COFF copies of their `__asm__`
names; rb_data.py and rb_aliases.py read COFF objects in the Mach-O spelling, as link_closure.py does; the original-layout
dumps' i686 side reads a second, Mac-style header tree (`gen/mingw-i686`: the Windows rewrite leaves out the C library's
headers). Link closure: 63 undefined (the Mac's 65). `tools/rb_wine.sh` runs ra2-native.exe under Wine as a drop-in
`--binary` for rb_routes.py (ctest label rb-wine on the Windows build folder).

**Simulator tests**: iOS now starts with Zoom gestures on (lane P12), which makes Red Alert 2's whole screen render scale 2
times larger, menus too; the YR Simulator tests' taps assume the unscaled screen, so they pass `zoomgestures=off`.

**build.sh**: mac, mac-one, ios [simulator|device] (RA2YR.app by default; `--ra2` for RBTrial.app), windows, yr-windows,
windows-one, all: the old native/build.sh list. The iOS combined app has the played app's bundle id (io.github.ra2native.RA2,
as the old combined app had) unless `-DRA2_ONE_BUNDLE_ID` names another.

## The differential gate (lane P14, 2026-11-24, same PIN, in redalert2-modern)

`ra2/tests/rb_gate.py` runs this port and the src-based played port (redalert2-decomp `build/native-play/ra2-native`,
built after lane N33; the older `build/release-mac-one` lacks N28-N33 and parts from both on a sidebar placement
click) on one route with one seed, the same harness orders and the same frame-timed input script, and compares main's
soak digest every 50 game frames (every techno's type, house, coordinate, strength, mission; the money; the random
generator) and every trigger, timer and defeat line. Modes: `missions` (mission_soak.py's route to each mission),
`skirmish` (RA2.INI's [Skirmish] ScenIndex and slots), `load` (a save loaded by both), `cross` (a harness save by one
port loaded by both), `recheck`. The rebuild's trigger watch counts loop frames and lists a new scenario's triggers at
its first frame; the gate moves them to game frames and drops the reset lines. Diagnosis: `GATE_WRAP_SRC/RB` run a
side under lldb; `build/p14/lldb/rngtrace.py` (every Random2Class call of a frame with its caller) and `rettrace.py`
(a function's calls and results) found the causes below on both release binaries (symbols, no debug info).

| Set | Result |
| --- | --- |
| Campaign, every mission, 6,000 frames | 18 of 24 identical (Allied 1-6, 9, 10; Soviet 1, 3-8, 10-12) |
| Skirmish, six set-ups (maps 0, 3, 7, 10, 14, 20; 1 to 4 computer players, nine houses) | 5 identical for 6,000 frames; map 14 (four computers) parts at frame 4,521: an AI engineer's walking step on a slope, `WalkLocomotionClass::Movement_AI`, a draft in both ports (open) |
| Orders | Allied 1 and Soviet 1 played by clicks (select, band box, move, Ctrl, D key) plus harness hunt, deploy and train: identical for 4,000 frames. A skirmish with harness orders only (MCV deploy, two GIs trained, hunts, two nuclear missiles at the player's base through `super 0 home`): identical for 5,000 frames. Skirmish click scripts (sidebar build and place): **not repeatable in either port** (the input script counts loop frames; a port's second run differs from its first in the frame the placement lands), so they prove nothing either way and are not a ctest row |
| The user's five saves (ALL01T, ALL02S, ALL03U, SAVE397A, SAVE4FBD) | all identical in both ports for about 11,000 frames after the load |
| Save crossings (src save loaded by both, rebuild save loaded by both; skirmish, Allied 2, Soviet 3) | 5 of 6 identical for 8,000 to 9,900 frames after the load; the src-made skirmish save: identical through 9,650, then the src port declares the player defeated at 9,664 and this port does not (the rebuild-made save: both at 9,664). Open: a field of the src port's save read differently, outside the digest |

The six missions that part, and why (`docs/differences.md`): the played port's `TechnoClass::Evaluate_Object` is a
non-exact draft (0x006C5570, 91.7%) and turns down targets the original accepts; this port runs the rebuild's exact
text (provenance class a). Seen frame by frame in Allied 8 (299), Allied 12 (262), Soviet 9 (350), the same pattern
in Allied 7 (1,818) and Allied 11 (3,148). Soviet 2 (2,264): a tank's path differs, the path from
`AStarClass::Find_Path_Regular`, exact here and a draft (0x00427370) in the played port. No fix toward the played
port: there it is the played port that is wrong against the original.

Fixed on the way (rows, each with its why): the Movies menu's list was empty and Play Movie did nothing (the movie
tables `IntroMovieEntry`, `AlliedMovieList`, `SovietMovieList` were zero storage: records with pointers were not
loaded; now pointer words, `rb_original_data.py`), found by main's `menu_movie.py`, now ctest `rb-main-menu-movie`;
`PixelFXTypes` loaded (zero before); the harness's `super <i> home` picks main's cell (the first building) and
`super <i> <x> <y>` is main's; the soak's defeat line prints the house's name as main's. New ctest rows:
`rb-native-tankdamage` (the user's C22), `rb-main-menu-movie`, and with RB_IDENTITY_SRC the `rb-gate-*` rows
(label rb-gate: the identical missions, four skirmish set-ups, two order scripts, the user's saves, two crossings).

The user's own bugs against this port: `docs/regressions.md`. What is not the same code, by game area:
`docs/differences.md`.

**Checks at the lane's head** (worktree build, `ctest -E '^(rb-gate|rb-identity)'`, RB_IOS_UDID set): 64 rows. Mac 53: all
pass (rb-main-hd-vehicles-3x, the known timing check, on its rerun). iOS Simulator 11: the first run failed 5 at the main
menu: since lane P12 iOS starts with Zoom gestures on, the menus run at render scale 2 and the routes' and main tests'
menu clicks missed; `rb_routes.py` and `rb_skirmish_wrap.py` now set `zoomgestures=off` unless a test asks for it. Then
all 11 pass (3 of them on a rerun after the app ended at start-up once). The rb-gate rows run so far pass (skirmish m10,
m20, orders-allied1, load-user-saves). Windows (Wine): pending lane P13's targets.

Later in lane P13 (after the full ctest): `ra2_native_new_edit_focused` also takes the rebuild's name fields (a ListBox
whose GWL_USERDATA is `{"NewEdit", style}`), so the iOS keyboard comes up in Red Alert 2's skirmish name field; without
`--skirmish-frames`/`--frames` the harness frame count is 1 only in a test run (`--run-to`), so normal play writes no
harness lines; one_game_saves.py accepts the rebuild's 16-byte voc.cpp record written as is; tools/original_save_compare.py
imported; yr_member_aliases.py finds llvm-mingw by RA2_LLVM_MINGW. Merged: the icon branch, lanes P14, P15 and P16.
Full ctest (109 rows, label soak left out) at efc9665: 102 passed; the 7 failures were rb-main-hd-vehicles-3x (the known
flicker timing check; passed on the rerun), rb-ios-sim-skirmish and -production (Zoom gestures, fixed), yr-native-
member-aliases and yr-native-save (test setup, fixed), one-game-saves (the voc.cpp record, fixed), one-game-ios-sim-menus
(the Red Alert 2 name field's keyboard, fixed; the YR Skip check passed on the rerun).

## Windows faults from codex1's PC check of 3182afe (lane P19, 2026-11-25)

codex1's report: `build/win-playtest/codex1-ports-windows-check.md` in redalert2-native. Both failures reproduced under Wine.

**Yuri's Revenge saves** (a save made by the save menu, loaded in a fresh process: "3,333 unmatched fields", signal 8):
the Windows build had field maps for 7 of YR's 85 saved types. Every i686 layout dump of the YR units stopped at
`<sys/types.h>`, because the Windows rewrite of the mingw-w64 headers (`gen-yr/mingw`) leaves out the C library's;
lane P13 had given the rebuild a second, Mac-style tree (`gen/mingw-i686`), not YR. `yr/yr.cmake` now writes
`gen-yr/mingw-i686` on Windows and `tools/original_layouts.py` reads it for the i686 side (85 types, 0 problems, as the
Mac). The dumps' cache kept a dump that had stopped at an error, so it now keeps only one that compiled, named by its
command, and a dump that does not compile is a problem (`--check` fails). Saves written by 3182afe's Windows YR were
written without maps and cannot be loaded. New ctests on the Windows build: yr-native-save-wine
(`yr_save_roundtrip.py --wine-prefix`; on 3182afe's yr-native.exe it fails as the PC did) and yr-native-original-layouts.

**Yuri's Revenge stopped after the MCV was selected**: `one_game_yr_play.py` (Red Alert 2, the toggle, a YR skirmish at
interface 1.25x, map zoom 1.5x, speed 2) found two faults. The click on the selected MCV stopped at once in
`TechnoClass::VoiceDeploy` (0x00708E00), which the tree has falling off its end with no return (VC6 left eax): the
Mac's clang returns there (Darwin defaults to -fno-strict-return), mingw's clang puts int3 there. The Windows units
(the YR dialect in CMakeLists.txt, `rb_probe.py --target windows`) now take `-fno-strict-return`. After 8,190 frames,
`fastmath::atan2f` (0x004CB3D0) read 8 GB before `atan_tab`: a ratio past `_ftol`'s range is INT_MIN from x86_64's
cvttss2si, which abs keeps negative; yr/fixups.json converts with `ra2port::x87_ftol` and wraps the index, as the
rebuild's row (and `asinf` the same way). Values in range convert as before. The crash dialog of an engine started by
a swap said "printed to the terminal": that engine never opened the log, so its own `log_path` was empty; it now
names the log the first engine opened (`app::adopt_log`). Input scripts take `@unit=<ID>` (the player's first techno
of that type on the view) for the click.

## Campaign missions played to wins (lane P22, 2026-11-25, same PIN)

Lane P21's step server (ra2/glue/stepserver_native.inc) driven by commander helpers (ra2/tools/ra2_commander.py: clicks
as raw pointer events, selection by clicks with Shift, Ctrl+<n> teams, attack, attack move through the command bar,
production and placement), each phase a short script; plans from each mission's own map file read at run time
(ra2/tools/ra2_mapinfo.py) and text walkthroughs. Each win is a replay (ra2/tests/campaign/<mission>.replay, inputs and
ticks only), rerun to the same winning tick before its ctest row (ra2-native-campaign-won-<mission>, label
ra2-campaign-won) was added. A mission is shelved at 3 lane-hours of wall time from its first step.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 2 (ALL02S, Eagle Dawn) | won at tick 30,335 | 0.5 | Tanya through the pond onto both flak cannons, engineers' drop takes the Academy, rocketeers on the reactors |
| Allied 3 (ALL03U, Hail to the Chief) | deferred (to be resumed) | 0.15 | the base built; Rhino raids and armed civilians took the power, refinery and barracks while the tanks idled; best point A3BASE1.SAV (tick 7,940, the base before the war factory) |
| Soviet 2 (SOV02T, Hostile Shore) | parked at tick 36,422, to be retried | 0.2 | land phase won (the Allied base, Grizzlies, pillboxes and German refineries destroyed; best point S2MID.SAV at tick 24,561 in the first try), but raids killed the harvesters and, with the army away, a lone Grizzly razed the base; six destroyers remain and need a navy (no money left for a naval yard and submarines). Hard to script, no port fault. Resume from S2WEAP.SAV (tick 13,613) with a defence team at home (ra2_commander.war) and a naval yard before the push |
| Soviet 3 (SOV03T, Big Apple) | won at tick 54,669 | 0.4 | base with two sentry guns, the derricks taken, Rhinos behind a home defence, an engineer into the battle lab, the beacon held (the lab must be skipped as a target: destroying it loses) |
| Soviet 5 (SOV05T, City of Lights) | parked at tick 4,500, to be retried | 0.05 | 36 conscripts by attack move lost at the soccer field and the GIs around (56,52); the walkthrough's way is building by building in garrisons, then the tesla troopers into the tower (CAPARS01 at 48,34). First try only; run folder build/p22/sov05 (S5MID.SAV is after the loss, not a resume point) |
| Soviet 1 (SOV01T, Red Dawn) | won at tick 49,448 | 0.2 | engineer repairs the bridge, about 70 conscripts by attack move through both Allied positions, the Pentagon's four parts |

Helper findings (the scripted-input path, not the game): a click's selection shows two or three frames after the button
is released (the game's mouse is read once a frame), so a click is followed by ten frames before the next one; a look
must be followed by a drawn frame before a click on an object (objects register where they were last drawn); with an
infantry selected, a left click on another controlled (allied house) infantry is a move, so a selection starts with a
right click; Escape opens the in-game options menu, whose modal loop runs no Main_Loop frame, so the step server stops
answering (the run had to be restarted from its recording). The step server now refuses "input key Escape" with an error (not queued, not recorded), so a script cannot wedge the session. The command bar's AttackMove
button is not on the bar in every state (input-script: no command bar button), so a scripted attack move through it fell
back to a plain move; the game's own attack move is Ctrl+Shift with a left click (Can_Force_Order), which the helper now
uses. Units standing at a barracks' door block its exit: the finished infantry is refunded, not built (the original's
behaviour), so the helpers walk new units away. The Game object reads the current tick at its start (timeouts counted
from 0 ended at once).

## Campaign missions played to wins (lane P23, 2026-11-25, same PIN)

Lane P22's sweep continued with the same tools (ra2/tools/ra2_step.py, ra2/tools/ra2_commander.py) and rules: one replay
per win (ra2/tests/campaign/<mission>.replay), rerun to the same winning tick before its ctest row; a mission is shelved at
3 lane-hours of wall time from its first step.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 3 (ALL03U, Hail to the Chief) | won at tick 39,041 | 0.1 | resumed from P22's A3BASE1 (tick 7,940): GIs into two houses by the bridges, barracks, war factory, a pillbox; 22 Grizzlies massed behind ra2_commander.hold (radius 10: a wider one chased across the western bridge into the second Soviet base); a plain push up the eastern road lost them all at the Soviet base's east side; walked instead in short legs with focus fire (fight_along) up the eastern road and round the north of the base, and the beacon force-fired through its cell |
| Allied 4 (ALL04U, Last Chance) | won at tick 32,994 | 0.2 | the starting rocketeers and transports belong to PlayerAlt2, a house with PlayerControl=yes (the player clicks them; the step server's "own" flag shows only the player's house) until the construction yard is built; rocketeers on both grand cannons, destroyers through the subs, the shore defences and the naval yard, transports unloaded on the beach (82,128), the MCV deployed at (79,126). First try: the Soviet yard sells itself 180 s after ours is built and an engineer from it captured our yard; second: attacking the Soviet yard first woke the shore forces (V3, tesla troopers) and they razed it; third: the infantry kept on the yard (base_defended) and no Soviet building touched. Then a naval yard, ten destroyers up the river (fight_along, sentry guns and subs on the way) and the amplifier force-fired from the water |
| Allied 5 (ALL05S, Dark Night) | won at tick 22,647 | 0.4 | Tanya and the spies belong to houses with PlayerControl=yes (fight_along takes houses=); about seven restarts from the recording: a coil kills Tanya in two hits and dogs in one bite while she walks (she shoots only when stopped, so legs of three cells with focus fire); a click on a building outside her sight is taken as a move (she walked into a coil), so targets are clicked only once seen, and barrels are force-fired by cell; the base exit north was reached only through the Soviet base, so its three reactors were C4'd first (the coils go dead); a silo's fence keeps Tanya out until its barrels are shot, and the Orange silo is reached only past three coils, so the last spy walked the whole way into a reactor and the silo was C4'd during the outage |
| Allied 6 (ALL06U, Liberty) | deferred (to be resumed) | 0.65 | win needs every BadGuy1 and BadGuy2 unit and building gone. The MCV deploys only on a legal 4x4 spot: at the Pentagon's south edge (111,177) the yard falls outside the local map area, among infantry it is blocked; it deployed at (88,170) (tick 4,834, best point build/p23/all06/try3.replay). From there in three tries a raid from the north-west (98,160) and Soviet engineers razed or took the yard at about tick 17,000-18,000; the base needs pillboxes and prism towers on the north ramp before anything else, and a war factory early. Not a port fault (the deploy refusal traced with lldb: Try_To_Deploy refused by Legal_Placement) |
| Allied 7 (ALL07T, Deep Sea) | deferred (to be resumed) | 0.45 | win: every BadGuy1 building destroyed. The Allied island falls by tick 3,600 and the Soviet MCV deploys at (73,193) at about tick 4,700, then a full base with four flak cannons and a silo. The fleet reaches the island's north water (75,180) by tick 3,200 from the start, but out-of-range force fire there sends it round into the southern inlet, where it guards without firing; a 25-ship siege from the north (ra2_commander.siege) killed the northern half but the yard kept rebuilding. Next: the inlet via the south (86,206), destroyers within 6 cells of the yard; or rocketeers early (barracks gives ten by tick 4,900, too late for the walkthrough's trick unless queued at once) |
| Soviet 7 (SOV07S, Chrono Defense) | deferred (to be resumed) | 0.25 | lost at about tick 8,100-8,700 three times (the lab spied or entered: groups of four "E1" walk straight to the lab at tick 7,700; dogs must meet them). The scripted builder was too slow (placement retries, a click-heavy loop of several hundred ticks per pass): the next try should place the sentry guns and coils by hand first, then a dog team kept on the lab |

New helpers (ra2/tools/ra2_commander.py): `fight_along` (a team walked along waypoints in short legs, focus-firing the
enemy unit or ground defence nearest its centre); `Game.force_cell` (Ctrl with a left click on a cell); `defend_step` (one home-defence order, engineers first) and
`base_defended` (base() with a home defence while each building is bought); `siege` (a fleet at a waypoint shells the
houses' buildings in reach by force fire, units first, prefer= types first). Finding: a left
click on an enemy building the player cannot see (outside every unit's sight, though mapped) is taken as a move, as in
the original (Allied 3's Psychic Beacon, Allied 5's coils: the units walk up to it); the force fire on one of its cells
attacks it, and once a unit sees it the click attacks (Tanya's C4).

## Campaign missions played to wins (lane P24, 2026-11-25, same PIN)

Lane P23's sweep continued with the same tools and rules (one replay per win in ra2/tests/campaign/, rerun to the same
winning tick before its ctest row; a mission is shelved at 3 lane-hours of wall time from its first step), the
not-started missions first.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 8 (ALL08U, Free Gateway) | won at tick 106,268 | 1.2 | win: every BadGuy1 and BadGuy2 building and unit gone, the beacon within 20 minutes (timer 18,000 frames). Tanya, the GIs and the engineers move to houses with PlayerControl=yes (TanyaLand, GILand, EngineerLand) a few frames after the input opens, and back to the player once the beacon falls (then "player KIA" loses the mission if Tanya dies: P, select all combatants, sent her into the base once). The road south runs past BadGuy1's base; the way is the eastern ramp (86-88,38) onto the elevated highway (x=83), south to its ramp at (83,100) and west along row 104. Dogs kill Tanya in one bite and she shoots only when not walking, so every check shoots dogs within six cells first (vip_kill). The eastern barrel (55,91) chain takes the barracks, a coil, the gate and two reactors; C4 on the two northern reactors puts the compound on low power and the coils go dead, but killing a coil restores the power (Tanya died to the next coil), so the southern reactors came first. Grouping a unit into a second Ctrl group takes it out of the first. The MCV deploys only on a spot with no occupant (the park by the arch is full of trees and lamps: free_spots). A builder stuck on an unplaceable refinery waited 48,000 ticks; power plants were lost to a raid while the army was out; 18 prism tanks then took the base in eight short sieges without a loss of more than six |
| Allied 9 (ALL09T, Sun Temple) | deferred (to be resumed) | 0.4 | win: every building of BadGuy1 (59, barrels included) and of the airport's house gone. Seals freed the village prisoners (ten GIs, two Grizzlies, five Hack engineers, a Night Hawk); the north barrels and the sentry gun, then engineers took the first pyramid and a tesla reactor; the eastern barrels killed a flak track and a reactor, and the second pyramid was taken (best point build/p24/all09/best.replay, tick 26,436: second pyramid ours and powered, the Soviet reactors and lab gone, three seals, one engineer). Captured pyramids draw 100 power each against the one captured reactor's 150: with both, neither fires (one was C4'd by our own seal to power the other). Blockers: the war factory keeps sending Rhinos (four of them razed the pyramid) and three flak tracks guard the yard (engineers die on the way). The airport must be captured first (an escorted engineer, not alone: one died to a lone conscript): its trigger sends engineers again and again, which the rest needs (the yard and the war factory captured) |
| Soviet 6 (SOV06T, Sub-Divide) | won at tick 30,200 | 0.15 | win: the Korean fleet and the Allied navy's warships sunk (transports and yards not needed). First try lost the base at about tick 15,000 to Grizzlies and GIs landed by the Allies (no defences: the walkthrough's coils matter); second, two tesla coils by the yard (one lost, a third bought) and the dreadnoughts' missiles on the next landing. The Korean fleet came to the base when the 15-minute timer ran out; submarines could not hit carriers in the shallows by the beach, the dreadnoughts could. Dreadnoughts sent at the harbour's prism towers all died; twenty submarines went straight in, destroyers and cruisers first, and the win followed |
| Soviet 8 (SOV08T, Desecration) | deferred (to be resumed) | 0.25 | win: a unit of the player enters the White House (CAWASH07 at 89,21, inside Vlad2's base behind four coils and two sentry guns). Three tries lost the construction yard by tick 3,300-4,400 to the first paradrop (about 30 conscripts at 58-63,109-116, beside the first power plant): a base guard (ra2_commander-style guard on the yard) kept the units alive but not the yard; most of the dropped conscripts are not "seen" in the step server's listing though beside the base, so the guard did not click them. Next: deploy the MCV further north-east, or desolators deployed (D) on the drop zones before the planes come (walkthrough: north, then south-west, then east). Run folder build/p24/sov08 (try4.replay) |

## Campaign missions played to wins (lane P25, 2026-11-26, same PIN)

Lane P24's sweep continued with the same tools and rules (one replay per win in ra2/tests/campaign/, rerun to the same
winning tick before its ctest row; a mission is shelved at 3 lane-hours of wall time, cumulative across lanes), the
not-started missions first.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 10 (ALL10S, Mirage) | won at tick 42,660 | 0.25 | win: every BadGuy1, BadGuy2 and BadGuy3 object gone (the Soviet houses that overrun the three German bases and build there); Einstein's lab must stand. The MCV deployed by the lab (112,162), a refinery and the battle lab by it, a second refinery at the main base's only legal spot (102,124), two power plants and a second war factory by the lab; the main base's own pillboxes, prism towers and GIs held its raids (hold-style focus fire, no losses after the first raid). Prism tanks by short sieges (ra2_commander.siege, prefer coils and sentry guns): 11 on BadGuy3 (8 lost in its centre once its yard fell), 14 on BadGuy1 (5 lost), 16 on BadGuy2 (2 lost) |
| Allied 11 (ALL11T, Fallout) | deferred (to be resumed) | 0.1 | win: the three BadGuy1 nuclear silos (NAMISL at 185,203, 187,197, 193,195) destroyed, inside BadGuy2's base on Cuba (four Apocalypses, about fifteen flak cannons, ten tesla coils, a nuclear reactor at 174,205 beside barrels at 172-175,207-208, a second reactor of BadGuy3's at 164,228 that feeds it). A first try built a war factory and battle lab (13,000 credits, tick 3,745: best point build/p25/all11/try1.replay up to that tick) and lost the war factory, the naval yard, the airfield and two refineries by tick 7,000 to two dreadnoughts' missiles from the south channel (112,136), sea scorpions, V3s and conscripts over the northern bridge (110,85) and terror drones at the north-west refinery; a refinery was taken by an engineer. Next: the walkthrough's opening at once (destroyers and dolphins hunting the dreadnoughts, GIs at the northern bridge, prism tanks against the V3s), then prism tanks landed on the west beach (158,207) to force-fire the barrels from (165,207) once BadGuy3's reactor is down |
| Soviet 9 (SOV09U, The Fox and the Hound) | won at tick 27,285 | 0.2 | win: the President (French house, 63,60) mind-controlled; losing: any damage to him, or every unit lost. The psi troopers and the IFV belong to the Africans house (PlayerControl=yes). One psi trooper turned a deployed GI (the other GI killed it); both engineers controlled into the barracks and battle lab (they become the player's); ten GIs and two spies trained (4,000 credits). The damaged psi trooper died to the patrolling IFV, so one psi trooper did the rest. The sniper in the Light Blue compound (83,31) is walled in: controlled, it shot the compound's five GIs, but walking out it passed the compound's Grizzly and died; on a rerun the psi trooper switched to the Grizzly (it broke one pillbox before dying), GIs finished the pillboxes, and the sniper, controlled again, walked out of the gate and round by the north road to (48,58) at the Dark Blue base's west gate, where it shot the eight seals and two dogs (range about 8 cells, outside the prism towers'). A spy disguised on a civilian walked through the gate into the power plant at (65,53); the sniper then stood at the gate (54,59) and shot the guards, and the psi trooper took the President from (56,59) |
| Soviet 11 (SOV11S, Red Revolution) | won at tick 35,900 | 0.2 | win: the Kremlin (CARUS03 at 47,45, BadGuy3's, strength only 800) destroyed. A first try built a base (radar, battle lab, nuclear reactor) but Yuri's psi troopers turned the conscripts and tanks and the base fell by tick 12,000 before a silo was affordable. The two starting Kirovs sent straight at the Kremlin died over BadGuy1's flak belt (77-89,105-117) by tick 3,300; flown instead round the map's edges (waypoints 60,138, 38,112, 22,82, 13,62, 25,46, 36,37, 42,40, inside the local map area: a waypoint outside it is no move order), one Kirov reached the Kremlin and bombed it down under its two flak cannons |

Placement (lane P25): a building's footprint is legal only within its Adjacent range of the player's own IsBase buildings
(DisplayClass::Passes_Proximity_Check), and walls do not count, so a walled base such as Allied 10's has almost no legal
spot beside it; the clicks that "failed" in earlier lanes' builders were on illegal origins, not a port fault. The step
server now answers `buildable <TYPE> <x> <y> [<r>]` (ra2/glue/stepserver_native.inc): the origins near x,y where the
building would place as the placement cursor judges it (proximity, shroud, every footprint cell clear to build), and with
r 0 the reason for each blocked cell. A click puts the footprint's origin at the clicked cell less ((w-1)/2, (h-1)/2). The
first map click after the sidebar icon places the building; the footprint is drawn only after that first click.

Sidebar names (lane P25): the step server's sidebar listing and the input script's `@cameo=` point now name super weapon
entries too (their SuperWeaponTypes names), so an armed super weapon's cameo can be clicked as a player clicks it
(ra2/glue/harness_input_native.inc RB_Buildable_Name).


## Campaign missions played to wins (lane P26, 2026-11-26, same PIN)

Lane P25's sweep continued with the same tools and rules (one replay per win in ra2/tests/campaign/, rerun to the same
winning tick before its ctest row; a mission is shelved at 3 lane-hours of wall time, cumulative across lanes), the
not-started missions first.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Soviet 12 (SOV12S, Polar Storm) | won at tick 64,411 | 1.05 | win: the chronosphere (GACSPH at 68,129, 750 strength, BadGuy3's) destroyed. The scripted chrono attacks come at fixed ticks: six IFVs at (170,100) about tick 4,600, rocketeers, four to six Grizzlies at (168,112) about 6,700, four seals from a landing craft at (178,129) about 8,400, a carrier's Hornets about 9,900, Grizzlies at (176,120) about 11,000. A lone coil loses to the six IFVs (one shot per 120-150 ticks); two coils north, one west and tesla troopers held them; the seals C4'd every power plant and the yard past two coils on low power (drain above output) and past infantry, so three Rhinos waited at the beach and killed the craft and seals. Two refineries by the ore (the first one far from it starved the build). The nuke (silo at tick 13,553, ten minutes) took the chronosphere only to 271 and BadGuy3 repaired it in 850 ticks; the patriots (NASAM: range 12, Powered, 23 of BadGuy3's 44 power plants' worth) ring it nine deep. Eight Kirovs flew round the south coast (waypoints inside the local map: x+y up to about 308, x-y down to about -72; the AEGIS at 136,149 hurts on the inner line) to a stage at (95,162) out of every patriot's reach, where they heal; from (76,142) the Iron Curtain on them (750 frames, about 14 cells of Kirov flight) and an attack on the building once seen: down in about 600 ticks |
| Allied 12 (ALL12S, Chrono Storm) | deferred (to be resumed) | 0.4 | win: Romanov's four Apocalypse tanks (BadGuy3's APOC at 118,121, 118,123, 122,117, 124,117, by the Kremlin CARUS03 at 125,122, which must stand) destroyed ("Kremlin Secured 1" then the victory chain). The seals' C4 on the reactor at (65,65) brings the MCV; it deploys only inside the reactor's fence ring, at (65,64) (the yard at 66,65, tick 1,320). A base with four derricks reaches the chronosphere at tick 10,305 (ready 16,605): the Chronosphere moved two Apocalypses (3x3 round 118,122) into the lake at (130,84) and they drowned (both runs; it targets into shroud). But two super weapon blasts hit the base centre at about ticks 18,500 and 18,970 every run (after Rhino and V3 raids from the east and north), so the chronosphere or its power never lasts the seven minutes to the second shot (23,000). Chrono legionnaires (range 5) teleport only to mapped cells (the spy satellite maps the area); one at (127,113) did not take the attack click on the second pair (the Apocalypses stand among BadGuy1's buildings) and died to BadGuy1's Apocalypses at (123,110) and (127,108). Best points: build/p26/all12/s4.replay (tick 17,368: first pair drowned) and l4.replay (legionnaires, spy satellite). Next: chronosphere and power plants away from the centre (south-west corner), the war factory sold before 18,500, or the second pair by force-fired legionnaires from (127,113) |
| Soviet 10 (SOV10T, Weathered Alliance) | deferred (first look) | 0.1 | win: the weather control device (GAWEAT at 17,95, Other3's, strength only 200, outside the local map until the map expands) destroyed or entered by a player's unit ("WCD Destroyed" / "WCD Captured"); the expansion comes with BadGuy1's battle lab (GATECH at 163,97, tag "Battle Lab") captured. The start: two amphibious transports land the MCV, two Rhinos, four conscripts and an engineer at (145,175); the lab is on the north shore behind BadGuy1's base (patriots at 160,96 and 166,96, a prism tower at 169,99), and by sea it is reached only the long way round the west (the north sea joins the west sea through 124-130,98-104, and the west sea the southern one west of x 80). A first try: three of five units loaded, the transport stopped at (120,172) on the way to (90,170) and was sunk; the MCV was not deployed. Run folder build/p26/sov10 (try1.replay). Next: deploy the MCV first, submarines with the transport, a waypoint chain checked cell by cell (cell command), land at (157,96) beside the lab with two engineers; then a nuke or Kirovs on the 200-strength device |

Step server (lane P26): `state` lists the defense queue (`factory defense`, BuildCat=Combat: tesla coils, flak cannons, the
silo and the Iron Curtain build apart from other buildings, and a finished defense waits there for placement) and the naval
yard's (`factory ship`). `mapfile <out> <name>` copies another of the game's data files read through the mix files (RULES.INI:
weapon ranges, speeds, super weapon timers), for planning; never into the repository.


## Campaign missions played to wins (lane P27, 2026-11-26, same PIN)

Lane P26's sweep continued with the same tools and rules (one replay per win in ra2/tests/campaign/, rerun to the same
winning tick before its ctest row; a mission is shelved at 3 lane-hours of wall time, cumulative across lanes); every
mission left is a retry. Lane-hours are cumulative (earlier lanes' time included).

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Soviet 2 (SOV02T, Hostile Shore) | won at tick 46,156 | 0.2 + 0.15 | win: every Americans object gone (the Germans' two refineries too; "Player Wins" on destroyed-all of houses 0 and 3, then a 5-second delay). The pillbox at (72,53) brings the MCV, deployed where it lands (76,59). Rhinos force-fired the four barrels by the German refineries (five conscripts lost to the blast); the Germans' harvesters fell to the home defence. Power, a refinery at (69,62), barracks, naval yard, two sentry guns south-west by the ore; engineers to the two derricks (34,61, 37,61) died to the alligators (Nod house, ALL) the first time: four Rhinos shot them, then the capture. The naval yard drew the destroyer attacks ("2 Destroyer Attack") and fell twice. 18 Rhinos down the road at x 78 killed the fifteen Grizzlies; a second push of eleven razed the whole Allied base (a building's destruction fires "End Mission": fire sale, every unit hunting) and died to the GIs and pillboxes at (86,130). An Allied engineer took a derrick: retaken. With two refineries, two harvesters and both derricks (about 1,240 credits a minute), the last four destroyers, guarding at (107-119,87-96), came into the bay when the third naval yard was placed (93,52) and died to three submarines and the Rhinos at the shore |
| Soviet 5 (SOV05U, City of Lights) | won at tick 51,032 | 0.05 + 0.35 | win: the Paris Tower (CAPARS01 at 48,34, Other2's) entered by a tesla trooper of Other3 ("Player Near Tower": the tower turns Other3's and kills with one shot), then every BadGuy1 unit dead ("Win Timer", destroyed-units-all; buildings not needed). Other3 has PlayerControl=yes: its four starting tesla troopers by the coil at (80,78), the six landed by the transport at (33,53) when it is discovered, and every SHK the player trains ("Tesla To Other3" hands each over two seconds after) are the player's to command. Barrel clusters stand by every Allied post and chain: a party walking past (42-48,70-74) lost 14 conscripts and three tesla troopers in one blast; shot from four cells instead they took a Grizzly's GIs (party_legs, then a force fire on the cluster). Parties chasing a target ran into the next nest (the soccer field at 52,56, the tower's west posts at 41,31 and 41,36): zone= keeps a party's targets on its side of the street. The pathfinder walked new tesla troopers through the GI nest at (70,64): walked by waypoints along y=76 and x=39-40 instead. The derrick (65,90) taken once its post was cleared pays for a tesla trooper about every 1,500 ticks. Two Ivans' bombs took the pillbox at (41,36) (the blast killed both); four tesla troopers walked to (44,39) and a click on the tower took them in. A first energized run lost: the Final Rush (all-to-hunt) razed the undefended barracks, the engineer and the coil and the mission was lost at tick 42,953 (the losing trigger not traced further); a rerun from the recording kept 18 conscripts at home (six in a garrison) and the tower's last targets died at our barracks (tick 50,863) |
| Allied 9 (ALL09T, Sun Temple) | deferred again | 0.4 + 0.5 | win ("_Win"): destroyed-buildings-all of houses 14 and 21, which by the map's numbering (the Player House is 13, then [Houses] in order) are BadGuy1 (every building, its 34 barrels included) and House Balabos (the two pyramids MAYAN at 87,107 and 106,126 and six CAMEX temples), not the airport's house as lane P24 read it. Capturing the airport (CAAIRP at 19,91; an engineer walked there with the freed GIs by (52,84), (41,84), (40,95), (30,97), (24,94)) gives a paradrop of six to eight GIs every 3,600 ticks but also starts "BG3 Airport Engineers", BadGuy3 engineers that retake it; 49 GIs were massed there by tick 47,764 and then died to the war factory's stream of flak tracks round the village (47-55,73-85), as did the Hack engineers on every walk through it (selecting forty GIs by clicks toggled units on the same cell: band_select). A fast opening instead (the village freed at 14,780, the two Grizzlies, three seals and two engineers straight north to 78,96): the seals' C4 took the north reactor (91,106) and the north pyramid by tick 16,954 (best point build/p27/all09/try6.replay to 16,954), but flak tracks and Rhinos then hunt the seals (they cannot C4 tanks) and engineers die to the first flak track. A Hack engineer into the war factory (82,126) makes it the player's ("Captured War Factory", entered-by house 22, Hack), which is the plan's key. Next: the fast opening, then the seals' C4 on the power plants (86,114 first) only with the Grizzlies and paradropped GIs answering the flak tracks, engineers kept back until the war factory is reachable |
| Soviet 7 (SOV07S, Chrono Defense) | won at tick 42,844 | 0.25 + 1.9 | lost when the lab (NATECH at 77,66, footprint 76-78,65-67) is destroyed, spied or entered; the win ("PlayerWin1", enabled by the last wave at 2,550 s, tick 38,250: the triggers' seconds are 15 ticks) is every BadGuy1 unit dead. Coils need the radar, which needs a refinery (refinery, radar, power, then coils). What breaks the base: groups of eight to ten GIs from the south, east and west about ticks 7,800-9,600 (they kill reactors and flak, and the coils go dead on low power), rocketeers (only flak and Apocalypse missiles reach them), Grizzlies from the west about 10,900, six prism tanks (SREF) from the south-west about 24,300 that beam the construction yard from twelve cells (it fell in every run until the tanks met them at 53,76 by tick 23,900), prism tanks, engineers and spies at the lab about 33,000-39,600, and Harrier strikes (ORCA) on the lab about 38,800. The winning run: refinery, sentry guns south and east, a war factory at 68,64, coils west, reactors north and east, eight Rhinos then Apocalypse tanks (build/p27/sov07/try10.replay to 23,700), the prism-tank intercept, seven flak cannons round the lab, then from 32,464 sixteen tanks parked on the ring of cells round the lab (ra2_commander.park_ring: engineers and spies cannot reach it) with the rest answering prism tanks, engineers, spies (force fire) and rocketeers (Apocalypse missiles), the lab kept repaired to full, and after tick 41,500 the hunters sent at every BadGuy1 unit left |
| Allied 6 (ALL06U, Liberty) | deferred again | 0.65 + 0.15 | the MCV (AMCV, landed at 92,162 about tick 2,100) deployed inside the Pentagon's west defences: moved to (94,173) and D gave the yard at (95,174) (tick 3,340; at 98,169, 96,170 and 95,172 it was refused). With a refinery by the south ore (98,179), barracks, war factory, an air force command and a battle lab the base outlived lane P23's raids (tick 95,000 reached; Mgr kept the defence queue apart for Allied types, ATESLA, GAPILL and NASAM, now in DEFENSE_TYPES). Prism tanks are not on this mission's sidebar. Terror drones (DRON) get into the base and kill tanks; the war factory at the south-west corner (90,177) was destroyed three times (cause not traced); Mgr's home defence drags every tank into fights (the count stayed at 16-25 for 30,000 ticks). Two Grizzly pushes on the Brown base (BadGuy2, 110-121,112-132: a tesla coil at 117,125, five sentry guns, two V3s) lost 21 and then 23 Grizzlies, the second losing 15 of them before (112,148), so the base was untouched. Best point build/p27/all06/try2.replay (yard and battle lab, 25 Grizzlies about tick 81,000). Next: find what kills the Grizzlies on the road north (V3 range, drones), rocketeers or spies, and the walkthrough's west route (a dozen GIs to the hospital at 32,115 across the west bridge) |


## Campaign missions played to wins (lane P29, 2026-11-26, same PIN)

Lane P27's sweep continued with the same tools and rules (one replay per win in ra2/tests/campaign/, rerun to the same
winning tick before its ctest row; a mission is shelved at 3 lane-hours, cumulative across lanes). No new win; three
missions deferred again with what they need. Lane-hours are cumulative.

| Mission | Result | Lane-hours | Notes |
| --- | --- | --- | --- |
| Allied 9 (ALL09T, Sun Temple) | deferred again | 1.15 + 0.45 | The map's own rules (in the scenario file) make the pyramids MAYAN Capturable=true, Powered=yes (Power=-100), range 18; House Balabos is BadGuy1's ally, the player's enemy. The five engineers belong to Hack House (PlayerControl=yes, allied to the player; ra2_commander's mine() does not list them: select them as `o.house=='Hack House'`); every capture trigger (war factory, construction yard, barracks, radar, battle lab, refineries) needs house 22, Hack. Capturing the war factory or destroying it ("Destroyed War Factory") disables the flak-track patrol (a team of HTKs every 60 seconds once the vehicles are freed, Medium/Hard), which is what kills everything near the village and the north entrance; the construction yard (89,130) captured gives the player a yard. From try6 at 15,996 (two engineers at 78,96): engineers sent at the north pyramid died at its west edge (83-86,105-106) in four runs, to two dogs from (89,106) and an unseen tesla trooper at (89,105); a cleared village (14,720 on, the HTKs killed by the Grizzlies, nine GIs kept) lost the seals that stay at (52,71) to the conscript nest at (48,72), and the sentry gun at (81,102) with a tesla trooper kills what stands at (72-75,93-96). Next: kill the dogs and the tesla trooper east of the pyramid first (seals at 86,103), the sentry gun by the Grizzlies before anything walks past (75,95), then engineers into the pyramid; or seals' C4 on the war factory (80,125) first to stop the HTK stream |
| Allied 6 (ALL06U, Liberty) | deferred again | 0.95 + 0.2 | try2's base (25 Grizzlies, 14,000 credits at 80,866) is gone by 94,434 in the replay: raids from the west ramp (conscripts, tesla troopers, a V3 reaching 92,164) and tesla troopers at the north rally (117,157) took two war factories, the barracks, power and the air force command between 83,500 and 88,400 even with every Grizzly answering threats within 26 cells. The way north to the Brown base is one three-cell bridge (x 127-129, y 133-153). Rebuilding the air force command puts ATESLA, rocketeers (JUMPJET) and Harriers (ORCA) on the sidebar. Next: from earlier (try2 at 51,769 or 60,771) prism towers and pillboxes on the west ramp (95-100,160-165) before the Grizzlies leave, then a 40-Grizzly push over the bridge, or rocketeers over the water |
| Soviet 8 (SOV08T, Desecration) | deferred again | 0.25 + 0.25 | The three paradrops come at waypoints 14 (45,94, 60 s), 17 (60,112, 120 s) and 16 (64,90, 210 s); the MCV deploys where it lands (yard at 62,104) and the base holds through all three with two desolators deployed on each zone (the third drop razed it when they stood at the first two). Then three V3s at (80-84,90-99) east of the bridge (79,94) shell the base from about tick 14,500 and keep coming; flak cannons and a tank squad sent at them hold only with the Apocalypses at home, and the harvesters die on the open ore, so the money runs out at about tick 32,000 with four Apocalypses (APOC needs the battle lab). Desolators' radiation where they stood kills each new engineer at a barracks built on that spot (60,112). Best points build/p29/sov08/p29c.replay (base through the drops, tick 7,315) and p29d.replay (tick 14,380, 17,000 credits). Next: the crates (the Lincoln Memorial north, the Washington Monument) and the enemy refinery east of the bridge captured, as the walkthrough says, before massing Apocalypses |

Walls (lane P29, the Soviet 7 finding): `buildable NAWALL ...` answered "no building type" because ObjectTypeClass::From_Name
finds the wall's overlay type of the same name first; the step server now looks the name up among the building types when
the first match is not one (ra2/glue/stepserver_native.inc). A wall clicked onto a legal cell (Soviet 7 at 77,61, tick
31,000, the cameo clicked then the cell) leaves the defense factory as placed, yet no wall overlay appears on the cell or
anywhere near; instrumented once, OverlayClass::Mark's wall branch printed only at scenario load and HouseClass::Place_Object
printed nothing in a replayed run, so the placement path for a wall was not found within the 20-minute box. Open: a likely
port fault in wall placement (BuildingClass::Unlimbo's IsWall branch, OverlayClass::Mark), to be traced with lldb on a
placement click rather than through a replay.

## Lane P30 (2026-11-26, same PIN): walls; campaign sweep

Walls are placed (lane P29's open fault was a reading of the step server's output, not a port fault). A live click
(Soviet 7, the wall cameo then cell 77,61, tick 31,078) traced with lldb runs EventClass::Execute (PLACE) ->
HouseClass::Place_Object (RTTI_BUILDING, building type 16) -> BuildingClass::Unlimbo's IsWall branch ->
OverlayTypeClass::Create_One_Of -> ObjectClass::Unlimbo -> OverlayClass::Mark (the wall branch) -> MapClass::Place_Wall;
the cell holds overlay 26 (NAWALL, the 27th of [OverlayTypes]) and the wall post is drawn. From Soviet 7's start, walls
clicked at 75,64 and 78,64 fill the line between them (76,64 and 77,64), as the original's Place_Wall does. The cell's
land type stays clear: OverlayTypeClass::Read_INI (exact, 0x5DCAA0) takes the land from a Land= key, which none of
RULES.INI's walls has, and Recalc_Attributes takes the NoUseTileLandType path for them; so `terrain` (which drew the land
type) never showed a wall and lane P29 took the wall for lost (its other try, at 74,59, was a rock cell). `terrain` now
draws a wall overlay as W and `cell` names each cell's overlay after a '+'; ra2-native-walls (ra2/tests/ra2_walls.py,
the clicks in ra2/tests/regress/walls-sov07.replay) checks the four cells through the step server. Lane P29's debug
prints that never fired were not reproduced: in the same replayed run lldb stopped in Place_Object for every placement.
Also: a debugger attaching made the step server's accept() or read() fail with EINTR, which ended serving (the game ran
on freely); both now retry (platform/sdl/host.cpp).

Campaign sweep (lane P30): Allied 11 (ALL11T, Fallout) looked at again, 0.1 + 0.1 lane-hours, no try played (the lane's
context went to the walls). With the wall overlays now drawn by `terrain`, BadGuy2's base on Cuba reads as a walled
compound (x 171-199, y 180-209; the wall's west side at x 171, the south side at y 209, gates by the roads at about
(176-178,195-197) and (199,196)), the three silos inside it 30 cells from the west coast (156,195). No battle lab or
prism tank is on this mission's sidebar (power, refinery, barracks, war factory, air force command, naval yard; walls,
pillbox, Patriot missile (NASAM), prism tower (ATESLA); GI, engineer, dog, rocketeer, SEAL; Harrier, landing craft, destroyer, Aegis). The
loss is "_Loss (Game Over)" five seconds after it is enabled; the nukes (BadGuy2's, from the silos) aim by "NUKE
Targeting 1/2/3+" (710 s, then 720 s each). The plan stays lane P25's.
