#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# One command per platform (README.md "Building"):
#
#   ./build.sh mac                    Red Alert 2 for the Mac:                build/mac/RA2.app
#   ./build.sh ios [simulator]        the iPhone/iPad app for the Simulator:  build/ios-simulator/RBTrial.app
#   ./build.sh ios device             the same, for your devices (needs RA2_IOS_TEAM): build/ios-device/RBTrial.app
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

build_ios() {
  common_checks
  local platform="${1:-simulator}"
  if [[ $# -gt 0 ]]; then shift; fi
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
  RA2_LLVM_MINGW="$mingw" cmake -S . -B "$b" -G Ninja -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT="$sdk" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET="${RA2_IOS_MIN:-17.0}" -DRA2_SDL_SOURCE="$sdl" \
        -DCMAKE_C_FLAGS="$opt" -DCMAKE_CXX_FLAGS="$opt" -DCMAKE_OBJCXX_FLAGS="$opt" "$@"
  RA2_LLVM_MINGW="$mingw" cmake --build "$b" --target rb-native-trial
  local app="$b/RBTrial.app"
  if [[ "$platform" == simulator ]]; then
    codesign --force --sign - --timestamp=none "$app"
  elif [[ -n "${RA2_IOS_TEAM:-}" ]]; then
    # Your team id and a development profile Xcode has already made for io.github.ra2native.RBTrial (the team
    # wildcard profile covers it); the team id comes from the environment, never from Git.
    python3 -I packaging/ios_sign.py "$app" --team "$RA2_IOS_TEAM"
  else
    echo "build.sh: device app left unsigned (set RA2_IOS_TEAM to sign it with your development profile)"
  fi
  python3 -I packaging/audit_ios_bundle.py "$app" --executable RBTrial --bundle-id io.github.ra2native.RBTrial
  echo "Built: $app"
}

what="${1:-}"
if [[ $# -gt 0 ]]; then shift; fi
case "$what" in
  mac) build_mac "$@" ;;
  ios) build_ios "$@" ;;
  *) sed -n '3,7p' "$0" | sed 's/^# \{0,1\}//' >&2; exit 64 ;;
esac
