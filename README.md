# redalert2-modern

Red Alert 2 as a native app for the Mac (Apple silicon) and iPhone/iPad, built from the **rebuild tree** of
redalert2-decomp: C++ reconstructed from the base Red Alert 2 executable (1.006) and checked there against it
function by function, here compiled by modern clang for arm64 on an SDL3 platform layer. No emulation, Wine,
Rosetta or original code runs.

**You need your own copy of the game.** Nothing of the game is in this repository or in the apps it builds: no
executables, mix files, movies, maps, tables or saves. The apps ask for your installed Red Alert 2 folder the
first time they start (the Mac: a folder; iPhone/iPad: the Files picker, copied once into the app) and read the
game's files and the data the original executable stores from your own `game.exe` at run time. Supported
`game.exe` builds are listed in `port/include/ra2port/game_builds.hpp` (the base 1.006 executable of 4,485,392
bytes, and copies whose data is unchanged, such as the signed 4,489,184-byte no-CD build). The CD check is skipped.

## What is here

| Path | What |
| --- | --- |
| `vendor/ra2-rebuild/` | the rebuild's `src/` and `include/` at a pinned commit of redalert2-decomp, unchanged, with its `LICENSE.md`, `adopted.csv` (per-file origin), `symbols.tsv` (names and addresses) and `PIN` (commit and SHA-256 of every file) |
| `ra2/` | the Red Alert 2 build on that tree: the generator (`tools/rb_units.py`), the clang fixups (`fixups.json`, each with its VC6 meaning), native glue for what the rebuild carries neither exact nor as a draft (`glue/`), data binding, the checks (`tests/`) and `ra2.cmake` |
| `port/` | the platform seams: Win32, DirectDraw, DirectSound, Winsock, OLE and the VC6 C runtime the game imports, on POSIX and SDL3 |
| `platform/` | the SDL host (window, input, touch, the first-run setup, the Enhancements page); `platform/win` is the Windows host, not built here yet |
| `yr/` | the pieces of the Yuri's Revenge port the RA2 build shares (SDK headers, the C runtime glue, the initialiser and data tools); the YR port itself is not here yet |
| `tools/` | link closure, data binding and layout tools, `update_rebuild.sh` (re-vendor), `check_tree.py` (hygiene) |
| `tests/` | the feature tests the played checks run (scripted input through the real menus) |
| `packaging/` | the Mac app bundle, the iOS bundle audit and device signing, the placeholder icon |
| `docs/ra2-trial-notes.md` | the lane-by-lane record of the move onto the rebuild (from the decompilation repository; its paths name that layout) |

Where everything came from, and the licences: `THIRD_PARTY.md`.

## Building

On a Mac with Apple silicon:

| Command | Result |
| --- | --- |
| `./build.sh mac` | `build/mac/RA2.app`: Red Alert 2 for the Mac |
| `./build.sh ios simulator` | `build/ios-simulator/RBTrial.app`: the iPhone/iPad app for the Simulator (home-screen name "RA2"; bundle id `io.github.ra2native.RBTrial`, kept apart from the played app's until parity) |
| `RA2_IOS_TEAM=<team id> ./build.sh ios device` | `build/ios-device/RBTrial.app`, signed for your devices |

Prerequisites (the script downloads and installs nothing):

- Xcode (for iOS; the command line tools are enough for the Mac), CMake 3.24 or later, Ninja and Python 3
  (`brew install cmake ninja`).
- Mac: SDL3 from Homebrew (`brew install sdl3`); the app bundles its library and licence.
- All builds: the llvm-mingw 20261020 macOS release (`llvm-mingw-20261020-ucrt-macos-universal`,
  github.com/mstorsjo/llvm-mingw) extracted, at `RA2_LLVM_MINGW` or under `toolchains/`. The game's Win32 and
  DirectX headers come from it at build time; nothing of it is copied or linked.
- iOS: the SDL 3.4.2 source, `RA2_SDL_SOURCE` or `toolchains/SDL3-3.4.2.tar.gz` (SHA-256 checked). It is built
  statically into the app.
- iOS device: your Apple team id in `RA2_IOS_TEAM` (never stored in Git) and a development profile Xcode has made
  that covers `io.github.ra2native.RBTrial` (your team's wildcard profile does); `packaging/ios_sign.py` signs.
  Install with `xcrun devicectl device install app --device <udid> build/ios-device/RBTrial.app`.

The Mac and Simulator apps are signed ad hoc (this Mac only); nothing is notarised or signed for distribution.

## Running from a build folder

```
RA2_DATA_DIR=<your game folder> ra2/tools/rb_run.sh --bin build/mac/rb-native-trial menu 60    # to the main menu
RA2_DATA_DIR=<your game folder> ra2/tools/rb_ios.sh run <simulator udid> 90                    # the Simulator app
RA2_DATA_DIR=<your game folder> ra2/tools/rb_ios.sh run-imported <simulator udid>              # the device's import path
```

The game never writes to your installation: a test run writes to `--write-dir`; the app keeps its own files in
`~/Library/Application Support/RA2Native` (Mac) or its container (iOS).

## Checks

```
ctest --test-dir build/mac -L static                      # hygiene (no game needed)
ctest --test-dir build/mac -R rb-native-original-layouts  # save-format field maps (no game needed)
cmake -S . -B build/mac -DRB_DATA_DIR=<your game folder> && ctest --test-dir build/mac -L rb-quick
```

The played checks (labels `rb-quick`, `rb-main`, `rb-campaign`, `rb-resaves`, `rb-chain`; `rb-ios` with
`-DRB_IOS_UDID=<simulator>`) run the game through its real menus with scripted input and skip (77) without your
game. Frame identity against the decompilation's src-based port (`rb-identity`) is added only with
`-DRB_IDENTITY_SRC=<its ra2-native>`. `python3 tools/check_tree.py --game-exe <your game.exe>` also proves that no
piece of the executable's code or resources is in the tree.

## Updating the rebuild

`tools/update_rebuild.sh <redalert2-decomp checkout> <commit>` re-vendors `vendor/ra2-rebuild` and rewrites its
`PIN`. Fixups that no longer match stop the configure; fix them, build, and run the played checks before
committing the new pin.

## Licence

GPL-3.0-or-later (`LICENSE`). The vendored rebuild keeps its own notices and `LICENSE.md` (the GPL plus Electronic
Arts' section 7 terms, which grant no right to EA's trademarks). Red Alert 2 is Electronic Arts' game; this
project is not affiliated with or endorsed by Electronic Arts.
