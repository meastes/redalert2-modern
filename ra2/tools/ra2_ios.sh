#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# The iOS app (RA2.app, bundle id io.github.ra2native.RA2Single) in the Simulator. From redalert2-decomp's
# branch ports-on-rebuild (native/rb/tools/rb_ios.sh, lanes P6 and P11); the build is ./build.sh ios here.
#
#   ra2/tools/ra2_ios.sh build [simulator|device]          ./build.sh ios <platform> --ra2 (build/ios-<platform>/RA2.app)
#   ra2/tools/ra2_ios.sh run <udid> [seconds] [run-to] [host args...]
#                                                          the Simulator app to a milestone (default "menu", the main
#                                                          menu; "never": the time limit), the user's installed game
#                                                          read at run time from a Mac path (--data-dir), nothing
#                                                          copied into the app
#   ra2/tools/ra2_ios.sh run-imported <udid> [seconds]     the device's way in: the app imports the game folder into
#                                                          its container (RA2_NATIVE_IMPORT stands in for the Files
#                                                          picker) and reads the imported game.exe; removed afterwards
#
# RA2_DATA_DIR: the installation (required for run and run-imported); RA2_GAME_EXE: the game.exe whose stored data
# is read (run; default the one in RA2_DATA_DIR).
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
cd "$root"
export DEVELOPER_DIR="${DEVELOPER_DIR:-/Applications/Xcode.app/Contents/Developer}"
cmd="${1:-build}"
if [[ $# -gt 0 ]]; then shift; fi

case "$cmd" in
build)
  exec "$root/build.sh" ios "${1:-simulator}" --ra2
  ;;
run)
  udid="${1:?usage: ra2_ios.sh run <simulator udid> [seconds]}"
  seconds="${2:-90}"
  run_to="${3:-menu}"
  shift; shift 2>/dev/null || true; shift 2>/dev/null || true
  app="${RB_IOS_APP:-build/ios-simulator/RA2.app}"
  data="${RA2_DATA_DIR:?set RA2_DATA_DIR to the installed game folder}"
  exe="${RA2_GAME_EXE:-$data/game.exe}"
  log="${RB_IOS_LOG:-build/ios-simulator/run.log}"
  state=$(xcrun simctl list devices | grep "$udid" || true)
  [[ -n "$state" ]] || { echo "ra2_ios: no Simulator device $udid" >&2; exit 1; }
  booted=0
  if [[ "$state" != *Booted* ]]; then xcrun simctl boot "$udid"; booted=1; fi
  xcrun simctl bootstatus "$udid" -b >/dev/null
  xcrun simctl install "$udid" "$app"
  export SIMCTL_CHILD_RA2_GAME_EXE="$exe" SIMCTL_CHILD_RA2_NATIVE_LOG_STDERR=1 SIMCTL_CHILD_RA2_NATIVE_PROGRESS_LOG=1
  export SIMCTL_CHILD_RA2_NATIVE_MOVIE_SKIP=1 SIMCTL_CHILD_RA2_NATIVE_NO_CRASH_DIALOG=1 SIMCTL_CHILD_RA2_NATIVE_CRASH_TRACE=1
  export SIMCTL_CHILD_RA2_NATIVE_DIALOG_LOG=1
  set +e
  xcrun simctl launch --console-pty --terminate-running-process "$udid" io.github.ra2native.RA2Single \
       --data-dir "$data" --run-to="$run_to" --run-seconds="$seconds" --game-args="-noaudio -CD." "$@" >"$log" 2>&1 &
  pid=$!
  for ((i = 0; i < seconds + 60; ++i)); do kill -0 $pid 2>/dev/null || break; sleep 1; done
  kill $pid 2>/dev/null
  wait $pid
  xcrun simctl terminate "$udid" io.github.ra2native.RA2Single >/dev/null 2>&1
  [[ $booted == 1 ]] && xcrun simctl shutdown "$udid"
  set -e
  grep -E 'milestone|stopped|frames run|trap|signal|crash' "$log" | grep -v '^run-to: reached' | head -40
  if [[ "$run_to" == never ]]; then grep -q 'stopped: time limit' "$log"; else grep -q "milestone $run_to" "$log"; fi
  ;;
run-imported)
  # Lane P11: the device's way in the Simulator. No --data-dir and no RA2_GAME_EXE: the host's app mode (main.cpp,
  # app_mode.hpp setup_imported) imports the game folder into the app's Library/Application Support/RA2Native/GameData (RA2_NATIVE_IMPORT, the
  # scripted stand-in for the Files picker) and points the port at the imported game.exe, as the app does on the
  # phone. The imported copy (about 1 GB) is removed afterwards unless RB_IOS_KEEP_IMPORT=1.
  udid="${1:?usage: ra2_ios.sh run-imported <simulator udid> [seconds]}"
  seconds="${2:-120}"
  app="${RB_IOS_APP:-build/ios-simulator/RA2.app}"
  data="${RA2_DATA_DIR:?set RA2_DATA_DIR to the installed game folder}"
  log="${RB_IOS_LOG:-build/ios-simulator/run-imported.log}"
  state=$(xcrun simctl list devices | grep "$udid" || true)
  [[ -n "$state" ]] || { echo "ra2_ios: no Simulator device $udid" >&2; exit 1; }
  booted=0
  if [[ "$state" != *Booted* ]]; then xcrun simctl boot "$udid"; booted=1; fi
  xcrun simctl bootstatus "$udid" -b >/dev/null
  xcrun simctl uninstall "$udid" io.github.ra2native.RA2Single >/dev/null 2>&1 || true   # a fresh container: nothing imported
  xcrun simctl install "$udid" "$app"
  unset SIMCTL_CHILD_RA2_GAME_EXE SIMCTL_CHILD_RA2_DATA_DIR
  export SIMCTL_CHILD_RA2_NATIVE_IMPORT="$data" SIMCTL_CHILD_RA2_NATIVE_LOG_STDERR=1 SIMCTL_CHILD_RA2_NATIVE_PROGRESS_LOG=1
  export SIMCTL_CHILD_RA2_NATIVE_MOVIE_SKIP=1 SIMCTL_CHILD_RA2_NATIVE_NO_CRASH_DIALOG=1 SIMCTL_CHILD_RA2_NATIVE_CRASH_TRACE=1
  set +e
  xcrun simctl launch --console-pty --terminate-running-process "$udid" io.github.ra2native.RA2Single \
       --game-args="-noaudio -CD." >"$log" 2>&1 &
  pid=$!
  # No --run-to (it needs a --data-dir): the milestones come from RA2_NATIVE_PROGRESS_LOG ("progress: menu").
  for ((i = 0; i < seconds + 300; ++i)); do
    kill -0 $pid 2>/dev/null || break
    grep -q '^progress: menu' "$log" 2>/dev/null && { sleep 5; break; }
    sleep 1
  done
  kill $pid 2>/dev/null
  wait $pid
  xcrun simctl terminate "$udid" io.github.ra2native.RA2Single >/dev/null 2>&1
  container=$(xcrun simctl get_app_container "$udid" io.github.ra2native.RA2Single data 2>/dev/null)
  [[ "${RB_IOS_KEEP_IMPORT:-0}" == 1 || -z "$container" ]] || /bin/rm -rf "$container/Library/Application Support/RA2Native/GameData"
  [[ $booted == 1 ]] && xcrun simctl shutdown "$udid"
  set -e
  grep -E 'app: (imported|game folder|game files|RA2_NATIVE_IMPORT)|ra2_port: .*(build|not)|^progress: (main|menu)|trap|signal|crash' "$log" | head -20
  grep -q 'app: game folder .*/RA2Native/GameData, game.exe .*/RA2Native/GameData/game.exe' "$log" && grep -q "^progress: menu" "$log"
  ;;
*)
  echo 'usage: ra2_ios.sh build [simulator|device] | run <udid> [seconds] | run-imported <udid> [seconds]' >&2; exit 2 ;;
esac
