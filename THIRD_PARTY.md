# Source and dependency provenance

This repository is GPL-3.0-or-later (`LICENSE`). Retain `LICENSE`, `vendor/ra2-rebuild/LICENSE.md` and every
notice below. It makes no new grant of rights in the original game: no game file, executable, extracted table
or other original data is stored here, and the apps read the player's own installation at run time.

## Where this repository's own text comes from

The port code is an initial import from **redalert2-decomp** (the Red Alert 2 matching decompilation and its
native ports; same owner, GPL-3.0-or-later), with no history carried over:

- branch `ports-on-rebuild` at commit `7c03cb9ab2` (lane P11, 2026-11-24): `native/rb` (here `ra2/`), the
  platform layer `native/platform` (here `platform/`), the port library `native/port/src` and `include`
  (here `port/`), `native/packaging` (here `packaging/`), the native tools the build uses (`native/tools`, here
  `tools/`), the pieces of the Yuri's Revenge port the RA2 build shares (`native/yr/tools`, `sdk`,
  `glue/yr_crt.cpp`, `data/original_save_types.txt`, here `yr/`), the feature tests the checks run
  (`native/tests`, here `tests/`), and `manifests/imports.json` and `manifests/rtti_classes.json` (import names
  and RTTI class/vtable names; no bytes);
- changes on import: paths for the new layout, the build reads `vendor/ra2-rebuild` instead of `git archive` and
  `symbols.tsv` instead of the 57 MB catalog, the target's section layout from `ra2/data/target_sections.tsv`
  instead of the target executable, the toolchain and SDL by variable, an SPDX line on each own file that had
  none, and a top-level `CMakeLists.txt` that builds the port library and SDL host itself (the branch linked them
  from a separate build). `docs/ra2-port-notes.md` is the branch's `native/rb/README.md`, unchanged
  below a short preface: its paths name the decompilation repository's layout.

Some of that text (the glue files in `ra2/glue/`, parts of `port/`) was carried from the decompilation's own
recovered source, which adopted text from the projects below with attribution; each such file keeps its notice.

## Vendored rebuild tree: `vendor/ra2-rebuild`

`rebuild/src`, `rebuild/include`, `rebuild/LICENSE.md` and `rebuild/manifest/adopted.csv` of redalert2-decomp at
the commit in `vendor/ra2-rebuild/PIN` (local main commit `b873d2b3c6`, the tree the ports-on-rebuild branch
reached identity on), copied unchanged by `tools/update_rebuild.sh`; `symbols.tsv` is derived from the rebuild
catalog (names and addresses). `tools/check_tree.py` fails when a vendored file differs from its PIN digest.

Most of the rebuild's text is adopted whole-file from **mischa85/RedAlert2** (`github.com/mischa85/RedAlert2`;
commits per file in `adopted.csv`). That project builds on the **TibSun reconstruction** (OpenTS-Developers/TibSun,
Electronic Arts source code released under the GPL), **ra2-decomp** (gibbo101/ra2-decomp) and, for the Westwood
Online headers, **electronicarts/CnC_Renegade**. The licence is GPL-3.0-or-later; Electronic Arts' GPL section 7
additional terms apply to the files that carry an Electronic Arts copyright line.

- `adopted.csv` indexes every adopted file: origin, origin commit, origin path and the SHA-256 of the file as
  adopted. Each adopted file keeps its own SPDX and copyright lines.
- `LICENSE.md` is mischa85's LICENSE.md: the GPL text plus Electronic Arts' section 7 terms the adopted files
  refer to. Those terms include that no right is granted to use Electronic Arts' trademarks.

The projects the rebuild draws on, for reference:

- **mischa85/RedAlert2**: a matching decompilation of the same RA2 1.006 game.exe; GPL-3.0-or-later with Electronic
  Arts' section 7 terms on files carrying an EA copyright line; its files carry ra2-decomp and OpenTS copyright
  lines.
- **TibSun reconstruction** (`OpenTS-Developers/TibSun`): GPL-3.0-or-later with Electronic Arts' section 7 terms
  (its `LICENSE.md`).
- **ra2-decomp** (`gibbo101/ra2-decomp`): game source GPL-3.0-or-later, Copyright 2026 ra2-decomp contributors, with
  Electronic Arts' section 7 terms on files carrying an EA copyright line (`COPYING.md`); its `data/` and tooling
  text MIT.
- **electronicarts/CnC_Renegade**: the Westwood Online API headers (`include/wolapi.h`, `wolapi_i.c` in the rebuild):
  Electronic Arts' GPL release, notice kept, marked modified.

Not used by this tree: the Generals `GameText.cpp` adoption (redalert2-decomp, 2026-11-23) is later than the
vendored commit and not in it; OpenSSL and LZO, which the decompilation's earlier native work linked, are not
linked here.

## Vendored Yuri's Revenge tree: `vendor/ra2-decomp` and `vendor/tibsun`

The Yuri's Revenge port (`yr/`) builds from the tree the decompilation repository pinned for it (`yr/` of
redalert2-decomp, here copied from its commit `887b1a2e11` by `tools/update_yr_vendor.sh`, unchanged; each folder's
`PIN` holds the upstream commit, the redalert2-decomp commit and the SHA-256 of every file, and `tools/check_tree.py`
fails when a file differs):

- `vendor/ra2-decomp`: **ra2-decomp** (gibbo101/ra2-decomp) at commit `8719494727dd9d5db16f92087e3cc13ea99f4dd3`,
  copied whole (`src/`, `include/`, `data/`, `COPYING.md`, `LICENSE`, `README.md`). Game source GPL-3.0-or-later,
  Copyright 2026 ra2-decomp contributors, with Electronic Arts' section 7 terms on files that carry Electronic Arts or
  OpenTS material (`COPYING.md`); its `data/` (the function map, manifest and names the YR tools read) MIT (`LICENSE`).
- `vendor/tibsun`: the **TibSun reconstruction** (OpenTS-Developers/TibSun) at commit
  `dbfbfbfaac40364ce7e1cd8fad7bcade5c6fb0d4` (ra2-decomp's pinned `TIBSUN_COMMIT`), headers only (`code/**/*.h`,
  `*.hh`), which ra2-decomp's build falls back on, with `LICENSE.md` and `README.md`: GPL-3.0-or-later with
  Electronic Arts' section 7 terms.

The YR port's own text (`yr/yr.cmake`, `yr/combined.cmake`, `yr/combined/`, `yr/fixups.json`, `yr/glue/`, `yr/data/`,
`yr/tools/`, `yr/tests/`, `docs/yr-port-notes.md`) is imported from redalert2-decomp's `native/yr` at the same
commit (same owner, GPL-3.0-or-later), paths changed for this layout and an SPDX line added where none was. Several
glue files carry text written from the decompilation's Red Alert 2 native tree (their comments name its
`native/link` files) and from the YR tree's own functions; each keeps its notice. `yr/data/globals.tsv` and
`member_aliases.tsv` are names and addresses of gamemd.exe, no bytes; the port reads the user's own `gamemd.exe` at
run time as it reads `game.exe`.

## Linked libraries

| Library | Version | Licence | Use |
| --- | --- | --- | --- |
| SDL3 | 3.4.2 (iOS, built from source); Homebrew's on the Mac | zlib | window, input, audio, texture upload |
| zlib | the system's (`-lz`) | zlib | `.zip` unpacking for mod import (platform/sdl/mod_import.cpp) |

The Mac app bundles Homebrew's SDL3 library with its licence (`packaging/make_app.sh`); the iOS app links SDL3
statically and carries its `LICENSE.txt` (`ra2/ra2.cmake`; `packaging/audit_ios_bundle.py` requires it).
Neither library is copied into Git.

## Build-time only

- **llvm-mingw** 20261020 (`github.com/mstorsjo/llvm-mingw`, the macOS universal release): its mingw-w64 Win32 and
  DirectX headers are read at build time and rewritten into the build folder by `yr/tools/yr_sdk.py`; nothing of
  it is copied into Git or linked into the apps (mingw-w64 headers: ZPL and public domain).
- CMake, Ninja, Python 3, Xcode.

## App icon

`packaging/icon/` (ra2-ios.png, ra2-mac.png and the ra2-ico-*.png sizes) holds the app icon, supplied by the
repository owner on 2026-11-24 and made with an image generator. It contains no artwork taken from the game or
from Electronic Arts. `packaging/make_icon.py` copies it into the Mac, iOS and Windows bundles; its drawn shapes
remain only for the Yuri's Revenge stand-alone app (`--yr`).
