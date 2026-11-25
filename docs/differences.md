# What is not the same code as the played port

The played port (redalert2-decomp's `native/`, built from `src/`) and this port (the rebuild tree in
`vendor/ra2-rebuild`) are two different sources of the same game. The SDL host and the Win32/DirectX seams
(`platform/`, `port/`) are the played port's, byte for byte apart from licence lines, so the touch toolbar, the
Enhancements page's storage, the window, audio mixing and the iOS life cycle run the same code. Everything below is
in the game layer. Lane P14 (2026-11-24) wrote this from the code and from the differential gate
(`ra2/tests/rb_gate.py`), which runs both ports on the same seed and orders and compares every techno, every
house's money and the random generator every 50 game frames.

Measured by the gate (details in "The gate" at the end): 18 of 24 campaign missions identical for 6,000 frames,
5 of 6 skirmish set-ups, the order scripts, all five of the user's own saves and 5 of 6 save crossings. The 6 missions
that part were traced to code that is exact in this port and only a draft in the played port: there this port is
the one that follows the original game.

## Where to look while playing

### Campaign

- **AI units find targets more often in this port**, and this port is the one that matches the original game. The
  played port uses a draft of `TechnoClass::Evaluate_Object` (0x006C5570, the "is this a target worth attacking"
  test; 91.7% matched, not exact) that turns down some targets the original accepts. This port runs the rebuild's
  text, which the compiler reproduces byte for byte from the original (rebuild provenance class a, lane O38). You may
  see a soldier on guard or hunting start an attack a few seconds earlier than in the played port. Traced with a
  debugger on both binaries (the calls and results of Evaluate_Object, frame by frame): Allied 8 (frame 299, a
  hunting GI), Allied 12 (frame 262, a conscript on area guard), Soviet 9 (frame 350); Allied 7 (frame 1,818) and
  Allied 11 (frame 3,148, the player's hunting GIs) show the same pattern (more targets accepted here).
- **Soviet 2 (frame 2,264): a tank takes a different path.** `DriveLocomotionClass::Start_Of_Move` gets another first
  step (facing 4 here, 2 then 1 then 4 in the played port); the path comes from `AStarClass::Find_Path_Regular`,
  exact in this port and a draft (0x00427370) in the played port. Probable cause, not proven step by step.
- 45 functions in all are exact in this port and only drafts in the played port (path finding
  `AStarClass::Find_Path_Regular`, zones `MapClass::Zone_Reset`, `Compute_Zone_Connections`, `Reset_Subzone`,
  infantry `Per_Cell_Process` and `Tunnel_AI`, `TechnoClass::Can_Deploy_Now`, `Area_Modify`, `Rock`, bridges'
  `Damage_Low_Bridge*`, `IonBlastClass::One_Time`, the credits, `Read_PCX_File` and others). Where the two ports
  differ in these, this port is the one that follows the original.
- Reinforcements (`Do_Reinforcements`, `Do_Chrono_Reinforcements`), `Dropship_Screen` and the score screen's
  `ScoreClass::Presentation` are the rebuild's drafts (not exact). Reinforcements were compared line by line with
  the played port's exact text (lane P11: same behaviour) and the chains of all 24 missions start and end.
- The mission timer, triggers and scripted teams are the rebuild's exact code; the gate compares every trigger
  firing in both ports and found no difference.

### Skirmish AI

- Base building: `HouseClass::Where_To_Place_Building`, `AI_Build_Defense` and `Random_Cell_In_Zone` are the
  rebuild's drafts (line-compared with the played port's exact text by lane P11: same behaviour). Before lane P10
  fixed the sides, the radar against sentry gun choice at frame 3,960 came from here.
- The same `Evaluate_Object` difference as the campaign: AI units may pick a target where the played port's units wait.
- One of six skirmish set-ups (map 14, four computer players) parts at frame 4,521: an AI engineer on its way to
  capture walks 10 leptons less far in one step here (on a slope, height 312). The step is
  `WalkLocomotionClass::Movement_AI`, a draft in both ports (different texts, neither exact), so neither side can be
  called right; open.

### Combat

- `TechnoClass::Fire_At` (flak scatter length summed in double, as the original: lane P11 carried the played
  port's form), `Explosion_Damage`, `BulletClass::AI` and `Shrapnel`, `Coord_Scatter` (the played port's snap at
  the map's top and left edge, carried), `Projectile_Motion`, the building turret charge
  (`Charge_Turret`, `Charging_AI`, `Discharge_Turret`, Prism towers), `BuildingClass::Take_Damage`,
  `UnitClass::EMPulse_Blast`, `Reload_AI`, `Get_Max_Speed`, `TechnoTypeClass::In_Range`: the rebuild's drafts.
- Movement steps: `WalkLocomotionClass::Movement_AI` (infantry) and `MechLocomotionClass::Movement_AI` are drafts
  in both ports, different texts (neither exact). Infantry walking is where an unexplained difference would most
  likely show.
- `fastmath` (facing from coordinates) carries the played port's double-precision forms (lane P9); `DirType`'s
  radian conversion is the original's product (lane P9).

### Menus and shell

- The dialog drawing (`OwnerDraw::Combo_Proc`, `List_Proc`, `Static_Proc`, `Control_Proc`,
  `Dialog_On_WM_PAINT`, `Init_Control_Animation`) is the rebuild's drafts plus lane P12's rows: dropdown and list
  rows are 2 to 6 pixels taller than in the played port because this port follows the original's
  `CB_SETITEMHEIGHT` (0x005F6460); the played port keeps user32's default. Dropdown frames and the open sound were
  fixed toward the played port in lane P12.
- The command bar's icons follow UI.INI's order (Deploy, Guard, ...); before lane N33 the played port showed the
  beacon and attack-move icons in those slots.
- Options and the Enhancements page (`Main_Options_Dialog_Proc`) are the played port's behaviour text compiled
  under the rebuild's names (`glue/mainopt_native.inc`).
- Fullscreen "Automatic" is light grey, as the played port (an owner ruling: the original draws it dark red).
- iOS starts with Pinch zoom on (the played app has it on only because the user turned it on).
- The Westwood Online (Internet) code is not built in either port; the button is disabled.
- **The Movies menu's list was empty** (lane P14, fixed): the intro and campaign movie tables (`IntroMovieEntry`,
  `AlliedMovieList`, `SovietMovieList`) were zero storage, because the data loader did not read records with
  pointers. They are now loaded from game.exe as pointer words (`ra2/tools/rb_original_data.py`), so the list shows
  and campaign movies unlock as missions are won. The same tool now loads `PixelFXTypes` (the pixel effects'
  colours, all zero before).
- Start-up: `Allocate_Surfaces`, `RegisterClasses`, `StringTable::LoadFile`, the loading-screen drawer and
  `WinMain` are the rebuild's drafts; the static initialisers run in a different mechanism (below).

### Saves

- Saves are the original's compound-file format in both ports, through field maps generated from each tree's own
  layouts (`ra2/tools/rb_original_layouts.py`). Fixes carried for 64-bit pointers: LayerClass::Save's 4-byte
  count against Load's pointer, LZO's dictionary, the swizzle ids, `LocomotionClass::Load`'s typed read,
  `ObjectClass::Load`'s swizzle of a 4-byte field (fixups rows 129-157). `Put_All` is a rebuild draft.
- A save loaded in one port continues the same game in the other: the user's five saves, and saves made by either
  port in Allied 2 and Soviet 3, give the same game for 8,000 to 12,000 frames after the load. **Open:** a skirmish
  save made by the played port (loop frame 900), loaded by both, is the same through frame 9,650, but at 9,664 the
  played port declares the human player defeated and this port does not (11 of the player's units still on the map at
  9,700). With a save made by this port both declare it at 9,664, so this port reads some field of the played port's
  save differently, outside the digest (the house's counts and flags, which decide a defeat). Look for it if a
  skirmish loaded from an old save does not end when it should.

### Multiplayer (LAN)

- LAN pacing (`Sync_Delay`, `Wait_For_Players`), the install-unique `UniqueID` and the Session field-globals are
  the played port's rows carried (lanes P8, P9, P12). `Net2DecodeGameopt`, `Change_Slot_Type`, `Set_Slot_Names`,
  `Send_Map_Preview`, `Scenario_DlgProc`, `MPlayer_Host_Dialog_Proc` are the rebuild's drafts. The LAN pair check
  ran 18,000 frames with no CRC difference between two copies of this port; this port against the played port in
  one LAN game was not tried.

### iPhone touch

- The touch toolbar, gestures and life cycle are the shared host (same code). The game-side hooks are carried
  rows: the team-button hold (`glue/tab_native.inc`), the touch keyboard for name and chat fields
  (`glue/ownrdraw_native.inc`), zoom and interface size (`glue/zoom_native.inc` plus about 55 rows), Queue five
  with Shift (`glue/controls_native.inc`).

### Drawing

- Assembly routines the rebuild does not carry as C are the played port's behaviour bodies
  (`glue/rb_asm_draw.cpp`: voxel drawers, spotlight brighteners; `glue/rb_vqa.cpp`: VQA/LCW codecs), and
  `XSurface::Draw_Ellipse`, `DSurface::Draw_Depth_Antialiased_Line`, `AlphaShapeClass::Draw_All` likewise.
  `Tactical::Draw_Tiles`, `Draw_Radial_Indicators`, `CellClass::Draw_Shroud_Or_Fog_Shape`, `XSurface::Plot_Line`,
  the ship and drive `Draw_Matrix` are the rebuild's drafts. HD vehicles are `glue/unit/hd_vehicles.cpp`.

## Inventory (for maintainers)

| Kind | Count | Where |
| --- | ---: | --- |
| Rebuild drafts compiled (`RA2_NON_MATCHING`, not exact) | 183 functions in 50 units (wonline's 10 and the WDT and ice ones not reached) | `vendor/ra2-rebuild/src`, marked in place |
| Played-port text carried as glue (append rows) | 24 `.inc` files + `rb_behaviour.cpp`, `rb_asm_draw.cpp`, `rb_vqa.cpp`, `unit/hd_vehicles.cpp` | `ra2/glue` |
| Of which test scaffolding only (never in play) | `harness_native.inc`, `harness_input_native.inc` | `ra2/glue` |
| Exact in this port, draft in the played port | 45 functions | listed above (Campaign) |
| `fixups.json` rows | 353 over 102 files | pointer width 158, port features (zoom, touch, interface size, HD) 55, declarations 37, compiler spelling 26, saves 19, glue appends 24, floating point 2, other 31 |
| Rows that change behaviour against the original on purpose | the 55 port-feature rows (off unless the Enhancements setting is on), Light_Grey "Automatic", Internet disabled, the 16 ms frame floor at game speed 0, LAN pacing, UniqueID, C17's Draw_Char guard, C23's mouse mutex, Restore_Check while inactive | each row's `why` names the played port's row |
| Rows that change behaviour toward the original | Coord_Scatter, Fire_At (the played port's forms, which are the original's), `-fno-rounding-math` with the fastmath and `DirType` rows, the divisions the original makes products | lane P9, P11 |

**Start-up order.** The played port runs the original's `_initterm` table order through explicit marks
(`native_units.py initterm_order`, `ra2port_initterm_mark`). This port links the 438 units in the order of each
unit's first original initialiser (`ra2/data/initterm.json`, keyed by entry address, re-keyed to rebuild units by
`rb_rekey.py` in redalert2-decomp), moves every unit's static initialisers into `__DATA,__rb_init_func`
(`yr/tools/defer_initializers.py`) and runs them after the game.exe data load in that link order
(`glue/rb_host_glue.cpp`). Within a unit the compiler's order applies. No behaviour difference has been traced to
it; the identical campaign starts in all 24 missions suggest none at start-up.

## The gate

See `ra2/tests/rb_gate.py` (modes `missions`, `skirmish`, `load`, `cross`, `recheck`). Results at lane P14 are in
the lane's hand-back and in `docs/ra2-trial-notes.md`.
