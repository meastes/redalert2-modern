# redalert2-modern

Red Alert 2 and Yuri's Revenge as native apps for the Mac (Apple silicon), iPhone/iPad and Windows (x86_64). Red Alert 2
is built from the **rebuild tree** of
redalert2-decomp: C++ reconstructed from the base Red Alert 2 executable (1.006) and checked there against it
function by function, here compiled by modern clang for arm64 on an SDL3 platform layer. No emulation, Wine,
Rosetta or original code runs.

**You need your own copy of the game.** Nothing of the game is in this repository or in the apps it builds: no
executables, mix files, movies, maps, tables or saves. The apps ask for your installed Red Alert 2 folder the
first time they start (the Mac: a folder; iPhone/iPad: the Files picker, copied once into the app) and read the
game's files and the data the original executables store from your own `game.exe` (and `gamemd.exe` for Yuri's Revenge)
at run time. Supported
`game.exe` builds are listed in `port/include/ra2port/game_builds.hpp` (the base 1.006 executable of 4,485,392
bytes, and copies whose data is unchanged, such as the signed 4,489,184-byte no-CD build). The CD check is skipped.

## What is here

| Path | What |
| --- | --- |
| `vendor/ra2-rebuild/` | the rebuild's `src/` and `include/` at a pinned commit of redalert2-decomp, unchanged, with its `LICENSE.md`, `adopted.csv` (per-file origin), `symbols.tsv` (names and addresses) and `PIN` (commit and SHA-256 of every file) |
| `ra2/` | the Red Alert 2 build on that tree: the generator (`tools/ra2_units.py`), the clang fixups (`fixups.json`, each with its VC6 meaning), native glue for what the rebuild carries neither exact nor as a draft (`glue/`), data binding, the checks (`tests/`) and `ra2.cmake` |
| `port/` | the platform seams: Win32, DirectDraw, DirectSound, Winsock, OLE and the VC6 C runtime the game imports, on POSIX and SDL3 |
| `platform/` | the SDL host (window, input, touch, the first-run setup, the Enhancements page); `platform/win` is the Windows host (built by `./build.sh windows`, `yr-windows` and `windows-one`; played on Windows 10/11) |
| `vendor/ra2-decomp/`, `vendor/tibsun/` | the Yuri's Revenge tree (gibbo101/ra2-decomp) and the TibSun headers it falls back on, at pinned commits, unchanged, each with its `PIN` (`tools/update_yr_vendor.sh`) |
| `yr/` | the Yuri's Revenge port on that tree (`yr.cmake`: generated units, `fixups.json`, `glue/`, data binding, `tests/`), which uses none of Red Alert 2's game code, and the combined app (`combined.cmake`, `combined/`: both games behind one thin host, a toggle in each main menu); also the SDK headers and tools the Red Alert 2 build shares. Notes: `docs/yr-port-notes.md` |
| `cmake/` | the Windows cross-build (llvm-mingw toolchain file, `windows.cmake`) |
| `tools/` | link closure, data binding and layout tools, `update_rebuild.sh` (re-vendor), `check_tree.py` (hygiene) |
| `tests/` | the feature tests the played checks run (scripted input through the real menus) |
| `packaging/` | the Mac app bundle, the iOS bundle audit and device signing, the placeholder icon |
| `docs/ra2-port-notes.md` | the lane-by-lane record of the move onto the rebuild (from the decompilation repository; its paths name that layout; see History) |

Where everything came from, and the licences: `THIRD_PARTY.md`.

Internal names in the Red Alert 2 build keep an `rb`/`RB_` prefix (CMake variables and options such as `RB_DATA_DIR`
and `RB_IDENTITY_SRC`, environment variables such as `RB_IOS_UDID`, the glue files `ra2/glue/rb_*.cpp`, generated
files and the `__rb_init_func` section): it means "built from the rebuild tree". Targets, test names and labels use
`ra2-` (`ra2-native`, `ra2-quick`), as Yuri's Revenge uses `yr-`.

## Building

On a Mac with Apple silicon:

| Command | Result |
| --- | --- |
| `./build.sh mac` | `build/mac/RA2.app`: Red Alert 2 for the Mac |
| `./build.sh ios simulator` | `build/ios-simulator/RA2YR.app`: the iPhone/iPad app, both games in one executable, for the Simulator (home-screen name "Red Alert 2"; bundle id `io.github.ra2native.RA2`, the old app's, so its saves, settings and imported files carry over; `-DRA2_ONE_BUNDLE_ID=` names another). `./build.sh ios simulator --ra2`: Red Alert 2 alone, `RA2.app` (bundle id `io.github.ra2native.RA2Single`) |
| `RA2_IOS_TEAM=<team id> ./build.sh ios device` | `build/ios-device/RA2YR.app`, signed for your devices |
| `./build.sh mac-one` | `build/mac/RA2YR.app`: Red Alert 2 and Yuri's Revenge in one Mac app |
| `./build.sh windows` / `yr-windows` | `build/windows/RA2Native.exe` / `YRNative.exe` (each self-contained) |
| `./build.sh windows-one` | `build/windows-one/RA2YRNative/`: both games for Windows (the folder is the app) |
| `./build.sh all` | mac, mac-one, ios simulator, windows, yr-windows and windows-one in turn |

Prerequisites (the script downloads and installs nothing):

- Xcode (for iOS; the command line tools are enough for the Mac), CMake 3.24 or later, Ninja and Python 3
  (`brew install cmake ninja`).
- Mac: SDL3 from Homebrew (`brew install sdl3`); the app bundles its library and licence.
- All builds: the llvm-mingw 20261020 macOS release (`llvm-mingw-20261020-ucrt-macos-universal`,
  github.com/mstorsjo/llvm-mingw) extracted, at `RA2_LLVM_MINGW` or under `toolchains/`. The game's Win32 and
  DirectX headers come from it at build time; nothing of it is copied or linked.
- iOS and Windows: the SDL 3.4.2 source, `RA2_SDL_SOURCE` or `toolchains/SDL3-3.4.2.tar.gz` (SHA-256 checked). It is
  built statically into the app (windows-one: as the one `SDL3.dll` beside the thin host and the two engine DLLs).
- Windows: llvm-mingw (above) is also the cross compiler (`cmake/llvm-mingw-x86_64.cmake`); the executables link libc++
  statically and need only Windows 10 or later. The Windows build's checks (`ctest --test-dir build/windows`, label
  `ra2-wine`, and the YR and one-game rows) run the .exe under Wine when `RA2_NATIVE_WINE` names one, else skip.
- iOS device: your Apple team id in `RA2_IOS_TEAM` (never stored in Git) and a development profile Xcode has made
  that covers the app's bundle id (`io.github.ra2native.RA2` for the combined app,
  `io.github.ra2native.RA2Single` for `--ra2`'s RA2.app; a wildcard profile covers both);
  `packaging/ios_sign.py` signs. Install with `xcrun devicectl device install app --device <udid> build/ios-device/RA2YR.app`.

The Mac and Simulator apps are signed ad hoc (this Mac only); nothing is notarised or signed for distribution.

## Running from a build folder

```
RA2_DATA_DIR=<your game folder> ra2/tools/ra2_run.sh --bin build/mac/ra2-native menu 60    # to the main menu
RA2_DATA_DIR=<your game folder> ra2/tools/ra2_ios.sh run <simulator udid> 90                # the Simulator app
RA2_DATA_DIR=<your game folder> ra2/tools/ra2_ios.sh run-imported <simulator udid>          # the device's import path
```

The game never writes to your installation: a test run writes to `--write-dir`; the app keeps its own files in
`~/Library/Application Support/RA2Native` (Mac) or its container (iOS).

## Checks

```
ctest --test-dir build/mac -L static                       # hygiene (no game needed)
ctest --test-dir build/mac -R ra2-native-original-layouts  # save-format field maps (no game needed)
cmake -S . -B build/mac -DRB_DATA_DIR=<your game folder> && ctest --test-dir build/mac -L ra2-quick
```

The played checks (labels `ra2-quick`, `ra2-main`, `ra2-campaign`, `ra2-campaign-won`, `ra2-resaves`, `ra2-chain`; `ra2-ios` with
`-DRB_IOS_UDID=<simulator>`) run the game through its real menus with scripted input and skip (77) without your
game. Frame identity against the decompilation's src-based port (`ra2-identity`, and the differential gate `ra2-gate`:
campaign missions, skirmish set-ups, order scripts, saves both ways; `ra2/tests/ra2_gate.py`) is added only with
`-DRB_IDENTITY_SRC=<its ra2-native>`. What is not the same code as that port, by game area: `docs/differences.md`;
the user's own bug reports checked here: `docs/regressions.md`. `ra2-campaign-won` replays campaign missions won through the step server (the game paused and advanced by ticks for a
scripted or agent player, `ra2/tools/ra2_step.py`, `ra2/glue/stepserver_native.inc`): the recorded inputs and ticks in
`ra2/tests/campaign/`, the win and score screen as the pass. An AddressSanitizer build (`-DRA2_NATIVE_ASAN=ON`, in its own build folder) adds `ra2-asan`: two of those replays,
which must also run with no AddressSanitizer report. Yuri's Revenge and the combined app (configure with `-DRA2_NATIVE_YR=ON -DRA2_NATIVE_COMBINED=ON`, as `./build.sh mac-one`
does) add `yr-native-*` (YR alone, all automated runs rather than played: skirmishes, saves in gamemd.exe's format, the start of both campaigns, the enhancements, LAN) and
`one-game-*` (the toggle both ways, saves after a swap, the menus' backdrop); `-DRA2_ONE_IOS_SIM=ON` adds `one-game-ios-*`
(label `one-ios`: the iOS app in the "RA2 Duo Validation" Simulator; any Simulator name works). They read the same installation, which needs
Yuri's Revenge installed. `python3 tools/check_tree.py --game-exe <your game.exe>` also proves that no
piece of the executable's code or resources is in the tree.

## History

The port began on 2026-11-23 as a trial on redalert2-decomp's `ports-on-rebuild` branch (lanes P1 to
P11): the native ports moved from the decompilation's `src/` tree onto its rebuild tree, checked frame for frame
against the src-based port that had been played until then. It moved to this repository at lane P11 and is now the
port; the src-based port (the old port) matters here only for history and for the optional identity checks
(`RB_IDENTITY_SRC`). `docs/ra2-port-notes.md` is that branch's record, kept as written under a short preface.

## Updating the rebuild

`tools/update_rebuild.sh <redalert2-decomp checkout> <commit>` re-vendors `vendor/ra2-rebuild` and rewrites its
`PIN`. Fixups that no longer match stop the configure; fix them, build, and run the played checks before
committing the new pin.

## Licence

GPL-3.0-or-later (`LICENSE`). The vendored rebuild keeps its own notices and `LICENSE.md` (the GPL plus Electronic
Arts' section 7 terms, which grant no right to EA's trademarks). Red Alert 2 is Electronic Arts' game; this
project is not affiliated with or endorsed by Electronic Arts.
