#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# One command per platform (README.md "Building"):
#
#   ./build.sh mac                    Red Alert 2 for the Mac:                   build/mac/RA2.app
#   ./build.sh mac-one                Red Alert 2 + Yuri's Revenge for the Mac:  build/mac/RA2YR.app
#   ./build.sh ios [simulator|device] the iPhone/iPad app, both games:           build/ios-<platform>/RA2YR.app
#   ./build.sh ios [simulator|device] --ra2   Red Alert 2 alone:                 build/ios-<platform>/RBTrial.app
#   ./build.sh windows                Red Alert 2 for Windows (x86_64):          build/windows/RA2Native.exe
#   ./build.sh yr-windows             Yuri's Revenge for Windows (x86_64):       build/windows/YRNative.exe
#   ./build.sh windows-one            both games for Windows:                    build/windows-one/RA2YRNative/
#   ./build.sh all                    mac, mac-one, ios simulator, windows, yr-windows and windows-one in turn
#
# Each configures its own build folder (Release), builds the game, packages it and prints the result's path.
# Rerunning only rebuilds what changed. Extra arguments after the platform go to CMake's configure step;
# RA2_BUILD_DIR picks another build folder. Nothing is downloaded or installed, and no game file is read or
# bundled: the apps ask for the player's installed game at run time.
#
# Prerequisites: a Mac on Apple silicon; Xcode (the command line tools are enough for mac), CMake 3.24+, Ninja,
# Python 3 (brew install cmake ninja); for mac, SDL3 from Homebrew (brew install sdl3); for every build the
# llvm-mingw 20261020 macOS release extracted (its Win32 headers are read at build time, never copied):
# RA2_LLVM_MINGW=<folder>, or extracted under toolchains/; for ios the SDL 3.4.2 source: RA2_SDL_SOURCE=<folder>,
# or toolchains/SDL3-3.4.2.tar.gz (SHA-256 checked) or an extracted toolchains/SDL3-3.4.2/.
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
cd "$root"
LLVM_MINGW_RELEASE=llvm-mingw-20261020-ucrt-macos-universal
SDL_VERSION=3.4.2
SDL_SHA256=ef39a2e3f9a8a78296c40da701967dd1b0d0d6e267e483863ce70f8a03b4050c

die() { echo "build.sh: $*" >&2; exit 1; }
need() { command -v "$1" >/dev/null 2>&1 || die "$1 not found: $2"; }

common_checks() {
  [[ "$(uname -s)" == Darwin ]] || die "the ports build on macOS"
  need cmake "install CMake (brew install cmake)"
  need ninja "install Ninja (brew install ninja)"
  need python3 "install Python 3"
  need clang "install Xcode or its command line tools (xcode-select --install)"
}

llvm_mingw() {
  local d="${RA2_LLVM_MINGW:-}"
  if [[ -z "$d" ]]; then
    d=$(ls -d toolchains/llvm-mingw-*/"$LLVM_MINGW_RELEASE" toolchains/"$LLVM_MINGW_RELEASE" 2>/dev/null | head -1 || true)
  fi
  [[ -n "$d" && -d "$d/generic-w64-mingw32/include" ]] || die "llvm-mingw not found: extract $LLVM_MINGW_RELEASE (github.com/mstorsjo/llvm-mingw, release 20261020) and set RA2_LLVM_MINGW to its folder"
  (cd "$d" && pwd)
}

sdl_source() {
  local d="${RA2_SDL_SOURCE:-toolchains/SDL3-$SDL_VERSION}"
  if [[ ! -f "$d/CMakeLists.txt" ]]; then
    local tarball="toolchains/SDL3-$SDL_VERSION.tar.gz"
    [[ -f "$tarball" ]] || die "no SDL $SDL_VERSION source: put SDL3-$SDL_VERSION.tar.gz (SHA-256 $SDL_SHA256, from libsdl.org) in toolchains/ or set RA2_SDL_SOURCE"
    [[ "$(shasum -a 256 "$tarball" | awk '{print $1}')" == "$SDL_SHA256" ]] || die "$tarball is not SDL $SDL_VERSION (SHA-256 differs)"
    tar -xzf "$tarball" -C toolchains
  fi
  (cd "$d" && pwd)
}

build_mac() {
  common_checks
  if ! { command -v brew >/dev/null 2>&1 && brew --prefix sdl3 >/dev/null 2>&1; }; then
    [[ -n "${SDL3_DIR:-}" ]] || die "SDL3 not found: brew install sdl3 (or set SDL3_DIR to an SDL3 CMake package)"
  fi
  local mingw; mingw=$(llvm_mingw)
  local b="${RA2_BUILD_DIR:-build/mac}"
  RA2_LLVM_MINGW="$mingw" cmake -S . -B "$b" -G Ninja -DCMAKE_BUILD_TYPE=Release "$@"
  RA2_LLVM_MINGW="$mingw" cmake --build "$b" --target rb-native-trial
  RA2_APP_ID="${RA2_APP_ID:-io.github.ra2native.RA2Modern}" sh packaging/make_app.sh "$b/rb-native-trial" "$b"
  echo "Built: $b/RA2.app"
}

build_mac_one() {
  # Both games (yr/combined.cmake): the thin host ra2yr-native with the engine bundles ra2-engine.so (Red Alert 2 from
  # the rebuild) and yr-engine.so (Yuri's Revenge from vendor/ra2-decomp), packaged as RA2YR.app.
  common_checks
  if ! { command -v brew >/dev/null 2>&1 && brew --prefix sdl3 >/dev/null 2>&1; }; then
    [[ -n "${SDL3_DIR:-}" ]] || die "SDL3 not found: brew install sdl3 (or set SDL3_DIR to an SDL3 CMake package)"
  fi
  local mingw; mingw=$(llvm_mingw)
  local b="${RA2_BUILD_DIR:-build/mac}"
  RA2_LLVM_MINGW="$mingw" cmake -S . -B "$b" -G Ninja -DCMAKE_BUILD_TYPE=Release -DRA2_NATIVE_YR=ON \
        -DRA2_NATIVE_COMBINED=ON "$@"
  RA2_LLVM_MINGW="$mingw" cmake --build "$b" --target ra2yr-native ra2-engine yr-engine
  sh packaging/make_app.sh --one-game "$b/ra2yr-native" "$b"
  echo "Built: $b/RA2YR.app"
}

build_ios() {
  common_checks
  local platform="${1:-simulator}"
  if [[ $# -gt 0 ]]; then shift; fi
  # The default: both games in one executable (yr/combined.cmake ra2yr-one, RA2YR.app); --ra2: Red Alert 2 alone.
  local one=1
  if [[ "${1:-}" == --ra2 ]]; then one=0; shift; fi
  if [[ "${1:-}" == --one ]]; then shift; fi
  local sdk
  case "$platform" in
    simulator) sdk=iphonesimulator ;;
    device) sdk=iphoneos ;;
    *) die "ios takes simulator or device" ;;
  esac
  export DEVELOPER_DIR="${DEVELOPER_DIR:-/Applications/Xcode.app/Contents/Developer}"
  local mingw; mingw=$(llvm_mingw)
  local sdl; sdl=$(sdl_source)
  local b="${RA2_BUILD_DIR:-build/ios-$platform}"
  local opt="${RA2_IOS_OPT--O2}"
  local game_opts=()
  [[ $one == 1 ]] && game_opts=(-DRA2_NATIVE_YR=ON -DRA2_NATIVE_COMBINED=ON -DRA2_NATIVE_ONE_EXECUTABLE=ON)
  RA2_LLVM_MINGW="$mingw" cmake -S . -B "$b" -G Ninja -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT="$sdk" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET="${RA2_IOS_MIN:-17.0}" -DRA2_SDL_SOURCE="$sdl" \
        -DCMAKE_C_FLAGS="$opt" -DCMAKE_CXX_FLAGS="$opt" -DCMAKE_OBJCXX_FLAGS="$opt" ${game_opts[@]+"${game_opts[@]}"} "$@"
  local target=rb-native-trial exe=RBTrial bundle_id=io.github.ra2native.RBTrial
  if [[ $one == 1 ]]; then
    target=ra2yr-one exe=RA2YR
    bundle_id=$(sed -n 's/^RA2_ONE_BUNDLE_ID:STRING=//p' "$b/CMakeCache.txt")
  fi
  RA2_LLVM_MINGW="$mingw" cmake --build "$b" --target "$target"
  local app="$b/$exe.app"
  if [[ "$platform" == simulator ]]; then
    codesign --force --sign - --timestamp=none "$app"
  elif [[ -n "${RA2_IOS_TEAM:-}" ]]; then
    # Your team id and a development profile Xcode has already made for the bundle id (the team wildcard profile
    # covers io.github.ra2native.RBTrial; RA2YR.app has the played app's id unless RA2_ONE_BUNDLE_ID names another);
    # the team id comes from the environment, never from Git.
    python3 -I packaging/ios_sign.py "$app" --team "$RA2_IOS_TEAM" ${RA2_IOS_PROFILE:+--profile "$RA2_IOS_PROFILE"}
  else
    echo "build.sh: device app left unsigned (set RA2_IOS_TEAM to sign it with your development profile)"
  fi
  python3 -I packaging/audit_ios_bundle.py "$app" --executable "$exe" --bundle-id "$bundle_id"
  echo "Built: $app"
}

# The Windows builds (cmake/llvm-mingw-x86_64.cmake): cross-compiled here with llvm-mingw, SDL3 built from source.
# windows and yr-windows share build/windows (SDL linked statically: each .exe is self-contained); windows-one has its
# own folder (one SDL3.dll shared by the thin host and the two engine DLLs).
windows_configure() {   # <build folder> <mingw> [CMake options]
  local b="$1" mingw="$2"; shift 2
  local sdl; sdl=$(sdl_source)
  RA2_LLVM_MINGW="$mingw" cmake -S . -B "$b" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$root/cmake/llvm-mingw-x86_64.cmake" \
        -DRA2_LLVM_MINGW="$mingw" -DRA2_SDL_SOURCE="$sdl" "$@"
}

build_windows() {
  local b="${RA2_BUILD_DIR:-build/windows}"
  common_checks; local mingw; mingw=$(llvm_mingw)
  windows_configure "$b" "$mingw" "$@"
  RA2_LLVM_MINGW="$mingw" cmake --build "$b" --target RA2Native rb-native-trial   # ra2-native.exe: the console build
  echo "Built: $b/RA2Native.exe (self-contained: copy it to the Windows PC on its own)"
}

build_yr_windows() {
  local b="${RA2_BUILD_DIR:-build/windows}"
  common_checks; local mingw; mingw=$(llvm_mingw)
  windows_configure "$b" "$mingw" -DRA2_NATIVE_YR=ON "$@"
  RA2_LLVM_MINGW="$mingw" cmake --build "$b" --target YRNative yr-native
  echo "Built: $b/YRNative.exe (self-contained: copy it to the Windows PC on its own)"
}

build_windows_one() {
  # The thin host RA2YRNative.exe loads ra2-engine.dll or yr-engine.dll from its own folder; all three share SDL3.dll.
  # ra2yr-native.exe is the console host the Wine tests run. RA2YRNative/ holds only what a PC needs.
  local b="${RA2_BUILD_DIR:-build/windows-one}"
  common_checks; local mingw; mingw=$(llvm_mingw)
  windows_configure "$b" "$mingw" -DRA2_NATIVE_YR=ON -DRA2_NATIVE_COMBINED=ON "$@"
  RA2_LLVM_MINGW="$mingw" cmake --build "$b" --target RA2YRNative ra2yr-native ra2-engine yr-engine
  local pkg="$b/RA2YRNative" f
  /bin/rm -rf "$pkg"; mkdir -p "$pkg"
  for f in RA2YRNative.exe ra2-engine.dll yr-engine.dll SDL3.dll; do
    [[ -f "$b/$f" ]] || die "$b/$f was not built"
    /bin/cp -f "$b/$f" "$pkg/$f"
  done
  /bin/cp -f "$(sdl_source)/LICENSE.txt" "$pkg/SDL3-LICENSE.txt"
  echo "Built: $pkg (RA2YRNative.exe, ra2-engine.dll, yr-engine.dll, SDL3.dll: copy the folder to the Windows PC)"
}

what="${1:-}"
if [[ $# -gt 0 ]]; then shift; fi
case "$what" in
  mac) build_mac "$@" ;;
  mac-one) build_mac_one "$@" ;;
  windows) build_windows "$@" ;;
  yr-windows) build_yr_windows "$@" ;;
  windows-one) build_windows_one "$@" ;;
  all) build_mac; build_mac_one; build_ios simulator; build_windows; build_yr_windows; build_windows_one ;;
  ios) build_ios "$@" ;;
  *) sed -n '3,12p' "$0" | sed 's/^# \{0,1\}//' >&2; exit 64 ;;
esac
