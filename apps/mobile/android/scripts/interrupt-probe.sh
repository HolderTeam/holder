#!/usr/bin/env bash
# Issue #106: kill Holder with `kill -9` partway through trashing a card with many children,
# then check on the next launch that the trash either fully happened or didn't.
# Drives InterruptedLifecycleProbe one phase at a time.
#
#   scripts/interrupt-probe.sh SERIAL [RUNS] [MAX_DELAY_MS] [MIN_DELAY_MS]
#
# The kill comes MIN_DELAY_MS to MAX_DELAY_MS after the trash starts. MAX_DELAY_MS bounds the kill's delay after the trash starts: set it a little above the
# trash's own time, which the trash phase logs as `trash-done ms=N`. Core only writes its
# recovery journal near the end, so a MIN_DELAY_MS close to that time aims the kills there.
#
# Install both APKs first (./gradlew :app:installDebug :app:installDebugAndroidTest). Leaves the
# app installed; the probe's data is under the app's files directory and is removed at the end.
set -euo pipefail

serial=${1:?usage: $0 SERIAL [RUNS] [MAX_DELAY_MS] [MIN_DELAY_MS]}
runs=${2:-20}
max_delay_ms=${3:-300}
min_delay_ms=${4:-0}
adb=${ADB:-$(command -v adb || echo "${ANDROID_HOME:-$HOME/Android/Sdk}/platform-tools/adb")}
pkg=team.holder.android.debug
runner=$pkg.test/androidx.test.runner.AndroidJUnitRunner
probe=team.holder.android.InterruptedLifecycleProbe

on_device() { "$adb" -s "$serial" "$@"; }
phase() { on_device shell am instrument -w -e class "$probe" -e interruptPhase "$1" "$runner"; }
passed() { grep -q "OK (1 test)" <<<"$1"; }

declare -A tally=()
for ((run = 1; run <= runs; run++)); do
  out=$(phase prepare) || true
  passed "$out" || { echo "run $run: prepare failed"; echo "$out"; exit 1; }

  on_device logcat -c
  trash_out=$(mktemp)
  phase trash >"$trash_out" 2>&1 &
  trash_pid=$!
  # Kill a random time after the trash itself starts, not after the process starts.
  on_device logcat -m 1 -e trash-start -s InterruptProbe:I >/dev/null
  delay_ms=$((min_delay_ms + RANDOM % (max_delay_ms - min_delay_ms)))
  sleep "$(printf '%d.%03d' $((delay_ms / 1000)) $((delay_ms % 1000)))"
  pid=$(on_device shell pidof "$pkg" | tr -d '\r' || true)
  [[ -n $pid ]] && on_device shell run-as "$pkg" kill -9 "$pid" || true
  wait "$trash_pid" || true
  if passed "$(cat "$trash_out")"; then how=finished; else how=killed; fi
  rm -f "$trash_out"

  on_device logcat -c
  out=$(phase verify) || true
  if ! passed "$out"; then
    echo "run $run ($how after ${delay_ms}ms): VERIFY FAILED"
    echo "$out"
    exit 1
  fi
  result=$(on_device logcat -d -s InterruptProbe:I | grep -o 'outcome=[a-z]* journal=[a-z]*' | tail -1)
  echo "run $run: $how after ${delay_ms}ms -> $result"
  key="$how $result"
  tally[$key]=$((${tally[$key]:-0} + 1))
done

on_device shell run-as "$pkg" rm -rf files/holder-interrupt-probe files/holder-interrupt-probe.json
echo "--- $serial, $runs runs"
for key in "${!tally[@]}"; do echo "${tally[$key]}  $key"; done | sort -rn
