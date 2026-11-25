#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# The Windows build's ra2-native.exe under Wine as a drop-in --binary for ra2/tests/ra2_routes.py (lane P13), as
# tools/ra2_ios_sim.sh is for the Simulator app: RB_WINDOWS_EXE names the .exe (default: build/windows/ra2-native.exe),
# RB_WINE_PREFIX a prefix folder of the run's own (made with wineboot when missing), RA2_NATIVE_WINE the wine
# executable (else `wine` on PATH). Exits 77 without Wine or the .exe. Unix paths work as they are (Z: is the root).
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
exe="${RB_WINDOWS_EXE:-$root/build/windows/ra2-native.exe}"
wine="${RA2_NATIVE_WINE:-$(command -v wine || true)}"
[[ -x "$wine" && -f "$exe" ]] || { echo "ra2_wine: no Wine or no $exe: skipped" >&2; exit 77; }
prefix="${RB_WINE_PREFIX:-$root/build/windows/wine/rb}"
export WINEPREFIX="$prefix" WINEDEBUG=-all WINEDLLOVERRIDES='mscoree,mshtml='
if [[ ! -f "$prefix/drive_c/windows/system32/kernel32.dll" ]]; then
  mkdir -p "$prefix"
  "$wine" wineboot -i </dev/null >/dev/null 2>&1 || true
fi
exec "$wine" "$exe" "$@"
