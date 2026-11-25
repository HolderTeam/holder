#!/usr/bin/env bash
# Starts every Android Virtual Device on this machine that isn't already running, and waits for
# them to boot. Companion to scripts/deploy-all.sh -- see README.md's Local Development section
# for the two-script workflow: run this once to bring your emulator set up, then
# scripts/deploy-all.sh as often as you like while developing.
#
# Two kinds of AVD are started:
#  - Ordinary AVDs (Android Studio's Device Manager / `emulator -list-avds`), with a window.
#  - Gradle-managed AVDs (app/build.gradle.kts' managedDevices, kept under
#    ~/.android/avd/gradle-managed), headless with host graphics -- see TESTING.md's
#    "Running instrumentation tests on a local emulator" for why they're launched that way.
#
# Usage: scripts/start-emulators.sh            start everything, wait for boot
#        scripts/start-emulators.sh --no-wait  start everything, don't wait
#        scripts/start-emulators.sh --stop     stop every running emulator

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
MANAGED_AVD_HOME="${HOME}/.android/avd/gradle-managed"
BOOT_TIMEOUT_SECONDS=300

log() { echo "[start-emulators] $*"; }
warn() { echo "[start-emulators] $*" >&2; }

mode="start"
wait_for_boot=1
case "${1:-}" in
  -h|--help)
    cat <<'EOF'
Usage: scripts/start-emulators.sh [--no-wait | --stop]

Starts every AVD on this machine that isn't already running: ordinary AVDs
(from `emulator -list-avds`) with a window, and Gradle-managed AVDs (from
~/.android/avd/gradle-managed) headless with host graphics. Then waits until
each one has booted and lists them.

  --no-wait  Start them and return immediately.
  --stop     Stop every running emulator (phones are left alone).

Never starts a second instance of an AVD that's already running, and never
creates, modifies, or deletes AVD definitions. To create the Gradle-managed
ones: ./gradlew :app:pixel2Api30Setup (or another managed device's Setup task).
EOF
    exit 0
    ;;
  --no-wait) wait_for_boot=0 ;;
  --stop) mode="stop" ;;
  "") ;;
  *) warn "Unknown option: $1 (see --help)"; exit 1 ;;
esac

# Same PATH check deploy-all uses for adb -- needed to see which AVDs are already running and
# to wait for boot, not to start anything itself.
command -v adb >/dev/null 2>&1 || {
  warn "adb not found on PATH. Install Android platform-tools (or open a shell where Android Studio's SDK platform-tools directory is on PATH) and try again."
  exit 1
}

# Every emulator-* serial adb currently knows about, in any state.
emulator_serials() {
  adb devices | awk -F'\t' '$1 ~ /^emulator-/ { print $1 }'
}

if [[ "${mode}" == "stop" ]]; then
  stopped=0
  while IFS= read -r serial; do
    [[ -n "${serial}" ]] || continue
    log "Stopping ${serial}..."
    adb -s "${serial}" emu kill >/dev/null 2>&1 || warn "Couldn't stop ${serial}."
    stopped=$((stopped + 1))
  done < <(emulator_serials)
  log "Stopped ${stopped} emulator(s)."
  exit 0
fi

# `emulator` isn't always on PATH the way `adb` commonly is -- it ships in its own SDK
# "Android Emulator" package, not platform-tools. Fall back to the same SDK location the
# project's own Gradle build already knows about (local.properties' sdk.dir, written by
# Android Studio -- see README.md) rather than inventing a separate setup mechanism.
find_emulator_bin() {
  if command -v emulator >/dev/null 2>&1; then
    command -v emulator
    return 0
  fi
  local sdk_dir=""
  if [[ -n "${ANDROID_HOME:-}" ]]; then
    sdk_dir="${ANDROID_HOME}"
  elif [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then
    sdk_dir="${ANDROID_SDK_ROOT}"
  elif [[ -f "${REPO_ROOT}/local.properties" ]]; then
    sdk_dir="$(sed -n 's/^sdk\.dir=//p' "${REPO_ROOT}/local.properties" | tail -1)"
  fi
  if [[ -n "${sdk_dir}" && -x "${sdk_dir}/emulator/emulator" ]]; then
    echo "${sdk_dir}/emulator/emulator"
    return 0
  fi
  return 1
}

EMULATOR_BIN="$(find_emulator_bin)" || {
  warn "Could not find the 'emulator' tool on PATH, via \$ANDROID_HOME/\$ANDROID_SDK_ROOT, or via local.properties' sdk.dir."
  warn "Install the \"Android Emulator\" SDK package (Android Studio > SDK Manager > SDK Tools), or add it to PATH, then try again."
  exit 1
}

# Ordinary and Gradle-managed AVDs -- purely local config discovery. Does not start, stop, or
# touch any AVD definition.
list_avds() { "${EMULATOR_BIN}" -list-avds 2>/dev/null | grep -v '^INFO' || true; }
ordinary_avds=()
while IFS= read -r name; do
  [[ -n "${name}" ]] && ordinary_avds+=("${name}")
done < <(list_avds)
managed_avds=()
if [[ -d "${MANAGED_AVD_HOME}" ]]; then
  while IFS= read -r name; do
    [[ -n "${name}" ]] && managed_avds+=("${name}")
  done < <(ANDROID_AVD_HOME="${MANAGED_AVD_HOME}" list_avds)
fi

if [[ "${#ordinary_avds[@]}" -eq 0 && "${#managed_avds[@]}" -eq 0 ]]; then
  warn "No AVDs are configured on this machine."
  warn "Create one in Android Studio's Device Manager, or a Gradle-managed one with"
  warn "./gradlew :app:pixel2Api30Setup, then try again."
  exit 1
fi

# Which AVDs are already running: every attached emulator-* serial (a booting/offline one is
# still a live process for that AVD, and starting a second instance of it would be exactly the
# duplicate this script must avoid), mapped back to its AVD name via `adb emu avd name`.
running_avds=()
while IFS= read -r serial; do
  [[ -n "${serial}" ]] || continue
  avd_name="$(adb -s "${serial}" emu avd name 2>/dev/null | head -n 1 | tr -d '\r')"
  [[ -n "${avd_name}" ]] && running_avds+=("${avd_name}")
done < <(emulator_serials)

is_running() {
  local needle="$1" candidate
  for candidate in "${running_avds[@]+"${running_avds[@]}"}"; do
    [[ "${candidate}" == "${needle}" ]] && return 0
  done
  return 1
}

# Headless managed AVDs crash in the emulator's render thread with swiftshader/software
# graphics on a Wayland desktop, so they use host graphics, which needs an X connection
# (Xwayland). Its auth file comes from, in order:
#  1. a running Xwayland's -auth argument,
#  2. XAUTHORITY already set in this session,
#  3. GNOME/mutter's on-demand Xwayland auth file in the runtime dir (GNOME only starts
#     Xwayland when an X client asks for it, so 1. can find nothing).
find_x_auth() {
  local runtime_dir="/run/user/$(id -u)" auth
  auth="$(ps -C Xwayland -o args= 2>/dev/null | sed -n 's/.*-auth \([^ ]*\).*/\1/p' | head -1 || true)"
  if [[ -z "${auth}" && -n "${XAUTHORITY:-}" && -f "${XAUTHORITY}" ]]; then
    auth="${XAUTHORITY}"
  fi
  if [[ -z "${auth}" ]]; then
    auth="$(ls -t "${runtime_dir}"/.mutter-Xwaylandauth.* 2>/dev/null | head -1 || true)"
  fi
  [[ -n "${auth}" ]] && echo "${auth}"
}

start_avd() {
  local avd="$1" managed="$2" log_file="${TMPDIR:-/tmp}/holder-emulator-$1.log"
  if [[ "${managed}" == 1 ]]; then
    local x_auth
    x_auth="$(find_x_auth)" || {
      warn "Skipping ${avd}: no X auth file found (no Xwayland running, XAUTHORITY unset,"
      warn "nothing in /run/user/$(id -u)). Open any X11 app (e.g. xterm) and try again."
      return 1
    }
    log "Starting ${avd} (Gradle-managed, headless) in the background (log: ${log_file})..."
    ANDROID_AVD_HOME="${MANAGED_AVD_HOME}" \
      XDG_RUNTIME_DIR="/run/user/$(id -u)" \
      WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}" \
      DISPLAY="${DISPLAY:-:0}" \
      XAUTHORITY="${x_auth}" \
      QT_QPA_PLATFORM=xcb \
      nohup "${EMULATOR_BIN}" -avd "${avd}" -no-window -no-audio -no-boot-anim -gpu host -no-snapshot \
      >"${log_file}" 2>&1 < /dev/null &
  else
    log "Starting ${avd} in the background (log: ${log_file})..."
    nohup "${EMULATOR_BIN}" -avd "${avd}" >"${log_file}" 2>&1 < /dev/null &
  fi
  disown
}

started=()
for avd in "${ordinary_avds[@]+"${ordinary_avds[@]}"}"; do
  if is_running "${avd}"; then log "Skipping ${avd}: already running."; continue; fi
  start_avd "${avd}" 0 && started+=("${avd}")
done
for avd in "${managed_avds[@]+"${managed_avds[@]}"}"; do
  if is_running "${avd}"; then log "Skipping ${avd}: already running."; continue; fi
  start_avd "${avd}" 1 && started+=("${avd}")
done

if [[ "${#started[@]}" -eq 0 ]]; then
  log "Nothing new to start."
elif [[ "${wait_for_boot}" -eq 0 ]]; then
  log "Started ${#started[@]} emulator(s). They can take a minute or more to finish booting"
  log "before 'adb devices' reports them as ready -- check the log file(s) above if one doesn't"
  log "come up, then run scripts/deploy-all.sh once they do."
  exit 0
else
  # Each started AVD registers with adb as a new emulator-* serial; wait until every one of
  # them that has appeared reports sys.boot_completed, and as many have appeared as were
  # started -- or give up after BOOT_TIMEOUT_SECONDS and point at the logs.
  log "Waiting for ${#started[@]} emulator(s) to boot (up to ${BOOT_TIMEOUT_SECONDS}s)..."
  deadline=$((SECONDS + BOOT_TIMEOUT_SECONDS))
  while :; do
    booted=0
    while IFS= read -r serial; do
      [[ -n "${serial}" ]] || continue
      [[ "$(adb -s "${serial}" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" == "1" ]] && booted=$((booted + 1))
    done < <(emulator_serials)
    if [[ "${booted}" -ge $(( ${#running_avds[@]} + ${#started[@]} )) ]]; then
      break
    fi
    if [[ "${SECONDS}" -ge "${deadline}" ]]; then
      warn "Timed out waiting for boot. Check the log file(s) above for the AVD that didn't come up."
      break
    fi
    sleep 3
  done
fi

log "Running emulators:"
while IFS= read -r serial; do
  [[ -n "${serial}" ]] || continue
  name="$(adb -s "${serial}" emu avd name 2>/dev/null | head -n 1 | tr -d '\r')"
  state="$(adb -s "${serial}" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')"
  log "  ${serial}  ${name:-?}  $([[ "${state}" == "1" ]] && echo booted || echo booting)"
done < <(emulator_serials)
log "Stop them all with: scripts/start-emulators.sh --stop"
