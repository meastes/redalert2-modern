#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# The iOS Simulator form of this port as a drop-in binary for tests/ra2_routes.py (lane P7, branch
# ports-on-rebuild): `ra2_routes.py --binary ra2/tools/ra2_ios_sim.sh ...` runs a route's scripted run in
# RA2.app on a Simulator device instead of the Mac binary, with the same arguments and checks.
#
#   RB_IOS_UDID=<device> [RB_IOS_APP=<RA2.app>] ra2_ios_sim.sh <the host arguments...>
#   ra2_ios_sim.sh --setup <udid> [app]       boot the device if needed (remembered), install the app
#   ra2_ios_sim.sh --cleanup <udid>           terminate the app; shut the device down if --setup booted it
#
# A run: every RA2_* variable of the caller's environment reaches the app as SIMCTL_CHILD_RA2_* (simctl's rule), with
# the host's log on stderr and the progress lines on; `simctl launch --console-pty` relays the app's output to stdout
# until it exits (the route's --run-seconds limit) or 60 s past it, then the app is terminated. The user's installed
# game (--data-dir) and the write folder (--write-dir) are Mac paths the Simulator reads and writes in place; nothing
# is copied into the app. Simulator only: no device install, no signing (the user's decisions).
set -uo pipefail
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
export DEVELOPER_DIR="${DEVELOPER_DIR:-/Applications/Xcode.app/Contents/Developer}"
bundle=io.github.ra2native.RA2Single

device_state() { xcrun simctl list devices | grep "$1" || true; }

case "${1:-}" in
--setup)
  udid="${2:?usage: ra2_ios_sim.sh --setup <udid> [app]}"
  app="${3:-${RB_IOS_APP:-$root/build/ios-simulator/RA2.app}}"
  [[ -d "$app" ]] || { echo "ra2_ios_sim: skipped: no Simulator app at $app (./build.sh ios simulator)"; exit 77; }
  state=$(device_state "$udid")
  [[ -n "$state" ]] || { echo "ra2_ios_sim: skipped: no Simulator device $udid"; exit 77; }
  mark="$root/build/ios-simulator/.booted-$udid"
  if [[ "$state" != *Booted* ]]; then xcrun simctl boot "$udid" && touch "$mark"; fi
  xcrun simctl bootstatus "$udid" -b >/dev/null || exit 1
  xcrun simctl install "$udid" "$app" || exit 1
  echo "ra2_ios_sim: $udid booted, $app installed"
  exit 0 ;;
--cleanup)
  udid="${2:?usage: ra2_ios_sim.sh --cleanup <udid>}"
  xcrun simctl terminate "$udid" "$bundle" >/dev/null 2>&1
  mark="$root/build/ios-simulator/.booted-$udid"
  if [[ -f "$mark" ]]; then xcrun simctl shutdown "$udid"; /bin/rm -f "$mark"; echo "ra2_ios_sim: $udid shut down"; fi
  exit 0 ;;
esac

udid="${RB_IOS_UDID:?ra2_ios_sim: set RB_IOS_UDID to the Simulator device}"
[[ "$(device_state "$udid")" == *Booted* ]] || { echo "ra2_ios_sim: $udid is not booted (ra2_ios_sim.sh --setup)" >&2; exit 1; }
seconds=90
for a in "$@"; do
  case "$a" in --run-seconds=*) seconds="${a#--run-seconds=}" ;; esac
done
while IFS='=' read -r name _; do
  [[ "$name" == RA2_* ]] && export "SIMCTL_CHILD_$name=${!name}"
done < <(env)
export SIMCTL_CHILD_RA2_NATIVE_LOG_STDERR=1 SIMCTL_CHILD_RA2_NATIVE_PROGRESS_LOG=1 SIMCTL_CHILD_RA2_NATIVE_NO_CRASH_DIALOG=1
# Lane P13: iOS starts with Zoom gestures on (lane P12), which makes the whole screen render scale 2 times larger; the
# routes' clicks are the Mac's (off by default), so a run that does not name the setting gets it off.
enh="${RA2_NATIVE_ENHANCEMENTS:-}"
[[ "$enh" == *zoomgestures=* ]] || export SIMCTL_CHILD_RA2_NATIVE_ENHANCEMENTS="${enh:+$enh,}zoomgestures=off"
xcrun simctl launch --console-pty --terminate-running-process "$udid" "$bundle" "$@" &
pid=$!
trap 'kill $pid 2>/dev/null; xcrun simctl terminate "$udid" "$bundle" >/dev/null 2>&1' TERM INT
for ((i = 0; i < seconds + 60; ++i)); do kill -0 $pid 2>/dev/null || break; sleep 1; done
kill $pid 2>/dev/null
wait $pid
code=$?
xcrun simctl terminate "$udid" "$bundle" >/dev/null 2>&1
exit $code
