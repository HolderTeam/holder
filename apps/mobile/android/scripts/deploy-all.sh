#!/usr/bin/env bash
# Builds Holder's debug APK once, then installs and launches it on every connected Android
# phone or running emulator `adb` can see -- for quick manual compatibility testing across
# multiple real devices without repeating the build+install+launch cycle by hand for each one.
# See README.md's Local Development section for the one-line summary.
#
# Usage: scripts/deploy-all.sh

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
PACKAGE="team.holder.android.debug"
APK_PATH="${REPO_ROOT}/app/build/outputs/apk/debug/app-debug.apk"
LOG_DIR="${REPO_ROOT}/build/device-logs"
# Holder's GitHub sign-in and API log tags, plus crashes from any app.
LOG_FILTER=(GitHubOAuth GitHubConnection GitHubApi AndroidRuntime:E)

log() { echo "[deploy-all] $*"; }
warn() { echo "[deploy-all] $*" >&2; }

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  cat <<'EOF'
Usage: scripts/deploy-all.sh

Builds Holder's debug APK once, then installs and launches it on every
connected Android phone or emulator `adb devices` reports as ready (state
"device"). For each one it also starts capturing Holder's GitHub and crash
log lines in the background, to build/device-logs/<serial>-<time>.log, until
the device is unplugged or the script is run again. Targets that are offline, unauthorized, or otherwise not ready
are skipped and reported, not treated as failures on their own.
EOF
  exit 0
fi

command -v adb >/dev/null 2>&1 || {
  warn "adb not found on PATH. Install Android platform-tools (or open a shell where Android Studio's SDK platform-tools directory is on PATH) and try again."
  exit 1
}

build_ref="$(git -C "${REPO_ROOT}" rev-parse --abbrev-ref HEAD) @ $(git -C "${REPO_ROOT}" rev-parse --short HEAD)"
if [[ -n "$(git -C "${REPO_ROOT}" status --porcelain -- app)" ]]; then
  build_ref="${build_ref} (+ uncommitted changes)"
fi
log "Building debug APK from ${build_ref} (./gradlew :app:assembleDebug)..."
"${REPO_ROOT}/gradlew" -p "${REPO_ROOT}" :app:assembleDebug -q

if [[ ! -f "${APK_PATH}" ]]; then
  warn "Build succeeded but the APK is missing at ${APK_PATH} -- check the assembleDebug output path."
  exit 1
fi

# adb devices' plain (non -l) output is one "<serial>\t<state>" line per connected target, e.g.:
#   List of devices attached
#   emulator-5554   device
#   ZY322PVNGV      offline
# Physical phones and emulators are indistinguishable here beyond their serial's shape, and
# nothing below treats them differently. Filtering on a literal tab (rather than e.g. `tail -n
# +2`) skips the header line correctly even when adb also prints "* daemon started successfully
# *"-style banner lines first, which happens the first time it starts its background server.
serials=()
states=()
while IFS=$'\t' read -r serial state; do
  [[ -n "${serial}" ]] || continue
  serials+=("${serial}")
  states+=("${state}")
done < <(adb devices | grep $'\t' || true)

if [[ "${#serials[@]}" -eq 0 ]]; then
  warn "No Android devices or emulators are connected (adb devices found nothing)."
  warn "Connect a phone with USB debugging enabled, or start an emulator, then try again."
  exit 1
fi

# Replaces any capture an earlier run left going for this device, then follows the device's
# log from now on. The capture is a background process outliving this script; adb exits it
# when the device disconnects.
start_log_capture() {
  local serial="$1" pid_file="${LOG_DIR}/${1}.pid" log_file since
  mkdir -p "${LOG_DIR}"
  if [[ -f "${pid_file}" ]]; then
    kill "$(cat "${pid_file}")" 2>/dev/null || true
    rm -f "${pid_file}"
  fi
  since="$(adb -s "${serial}" shell "date +'%m-%d %H:%M:%S.000'" 2>/dev/null | tr -d '\r')"
  log_file="${LOG_DIR}/${serial}-$(date +%Y%m%d-%H%M%S).log"
  nohup adb -s "${serial}" logcat -v time ${since:+-T "${since}"} -s "${LOG_FILTER[@]}" \
    >"${log_file}" 2>&1 </dev/null &
  echo $! >"${pid_file}"
  log_files+=("${serial}: ${log_file}")
}

results=()
log_files=()
usable_count=0
failure_count=0

for i in "${!serials[@]}"; do
  serial="${serials[$i]}"
  state="${states[$i]}"

  if [[ "${state}" != "device" ]]; then
    log "Skipping ${serial}: adb reports \"${state}\", not ready."
    results+=("${serial}: skipped (${state})")
    continue
  fi
  usable_count=$((usable_count + 1))

  log "Installing on ${serial}..."
  install_output="$(adb -s "${serial}" install -r -g "${APK_PATH}" 2>&1)" && install_ok=1 || install_ok=0
  if [[ "${install_ok}" -eq 0 ]]; then
    # -r keeps app data across a matching-signature reinstall (the common case: Holder's debug
    # builds all use the same checked-in debug keystore). If it still fails -- e.g. this device
    # previously got a debug build signed some other way -- fall back to a clean install so the
    # device still gets tested, just without preserved data. That matches "preserve app data
    # during replacement where practical": practical here, not guaranteed.
    warn "install -r failed on ${serial}, retrying as a clean install (app data will be reset): ${install_output}"
    adb -s "${serial}" uninstall "${PACKAGE}" >/dev/null 2>&1 || true
    install_output="$(adb -s "${serial}" install -g "${APK_PATH}" 2>&1)" && install_ok=1 || install_ok=0
  fi

  if [[ "${install_ok}" -eq 0 ]]; then
    warn "Install failed on ${serial}: ${install_output}"
    results+=("${serial}: install failed")
    failure_count=$((failure_count + 1))
    continue
  fi

  # Keep the screen on while plugged into USB and wake it, so a manual test session or a
  # connected test isn't interrupted by the lock screen. A phone with a PIN still needs
  # unlocking by hand once; after that it stays awake until unplugged.
  adb -s "${serial}" shell svc power stayon usb >/dev/null 2>&1 || true
  adb -s "${serial}" shell input keyevent KEYCODE_WAKEUP >/dev/null 2>&1 || true

  start_log_capture "${serial}"

  log "Launching Holder on ${serial}..."
  if adb -s "${serial}" shell monkey -p "${PACKAGE}" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1; then
    results+=("${serial}: installed + launched")
  else
    warn "Installed on ${serial} but launch failed."
    results+=("${serial}: installed, launch failed")
    failure_count=$((failure_count + 1))
  fi
done

echo
log "Summary (${build_ref}):"
for r in "${results[@]}"; do
  echo "  ${r}"
done

if [[ "${#log_files[@]}" -gt 0 ]]; then
  echo
  log "Capturing logs in the background:"
  for f in "${log_files[@]}"; do
    echo "  ${f}"
  done
fi

if [[ "${usable_count}" -eq 0 ]]; then
  warn "No usable devices (adb state \"device\") among the connected targets -- nothing was deployed."
  exit 1
fi

if [[ "${failure_count}" -gt 0 ]]; then
  exit 1
fi
