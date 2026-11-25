#!/bin/zsh
# SPDX-License-Identifier: GPL-3.0-or-later
# Run the Mac game binary to a milestone with the user's installed game read at run time: nothing is copied;
# the game writes only to a scratch folder.
#
#   ra2/tools/ra2_run.sh [--bin <binary>] [milestone, default menu] [seconds, default 60] [extra host args...]
#
# --bin: the binary (default build/mac/ra2-native, ./build.sh mac's).
# RA2_DATA_DIR: the installation (read only; required). RA2_GAME_EXE: the game.exe whose stored data is read
# (default the one in RA2_DATA_DIR; a supported build, port/include/ra2port/game_builds.hpp).
# RB_WRITE_DIR: where the game writes (default build/run).
HERE=${0:A:h}
ROOT=${HERE:h:h}
BIN=$ROOT/build/mac/ra2-native
if [[ $1 == --bin ]]; then BIN=${2:A}; shift 2; fi
TARGET=${1:-menu}; SECONDS_=${2:-60}; shift 2 2>/dev/null
DATA=${RA2_DATA_DIR:?set RA2_DATA_DIR to the installed game folder}
export RA2_GAME_EXE=${RA2_GAME_EXE:-$DATA/game.exe}
W=${RB_WRITE_DIR:-$ROOT/build/run}
mkdir -p $W
export RA2_NATIVE_CRASH_TRACE=1
$BIN --data-dir "$DATA" --run-to=$TARGET --run-seconds=$SECONDS_ --write-dir=$W --game-args="-noaudio -CD." "$@" &
pid=$!
for i in {1..$((SECONDS_ + 30))}; do kill -0 $pid 2>/dev/null || break; sleep 1; done
kill $pid 2>/dev/null
wait $pid
