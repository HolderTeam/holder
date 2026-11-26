#!/usr/bin/env bash
# Issue #106: install an older Holder build, fill it with data, install this build over it, and
# check that everything opens (UpgradedProfileProbe).
#
#   scripts/upgrade-probe.sh SERIAL OLD_APK OLD_TEST_APK
#
# OLD_TEST_APK is the older build's androidTest APK with scripts/upgrade-probe/OldProfileSeed.kt
# added to it (the seed must use that build's own HolderNative API). For the schema-6 baseline,
# Android cdb1a22d with core 009551b:
#
#   git worktree add --detach OLD cdb1a22d
#   git worktree add --detach OLDCORE 009551b05efa2a912178e7acb1dd2105f364939e
#   ln -s OLDCORE/core OLD/apps/mobile/android/submodules/holder-core   # then remove the empty dir first
#   cp local.properties OLD/apps/mobile/android/
#   cp scripts/upgrade-probe/OldProfileSeed.kt OLD/apps/mobile/android/app/src/androidTest/java/team/holder/android/
#   (cd OLD/apps/mobile/android && ./gradlew --no-configuration-cache :app:assembleDebug :app:assembleDebugAndroidTest)
#
# UNINSTALLS the debug app first: run it on a test device only. Leaves this build installed.
set -euo pipefail

serial=${1:?usage: $0 SERIAL OLD_APK OLD_TEST_APK}
old_apk=${2:?}
old_test_apk=${3:?}
adb=${ADB:-$(command -v adb || echo "${ANDROID_HOME:-$HOME/Android/Sdk}/platform-tools/adb")}
pkg=team.holder.android.debug
runner=$pkg.test/androidx.test.runner.AndroidJUnitRunner
here=$(cd "$(dirname "$0")/.." && pwd)

on_device() { "$adb" -s "$serial" "$@"; }
instrument() { on_device shell am instrument -w "$@" "$runner"; }

on_device uninstall "$pkg.test" >/dev/null 2>&1 || true
on_device uninstall "$pkg" >/dev/null 2>&1 || true
on_device install "$old_apk" >/dev/null
on_device install "$old_test_apk" >/dev/null
out=$(instrument -e class team.holder.android.OldProfileSeed)
grep -q "OK (1 test)" <<<"$out" || { echo "seeding with the old build failed"; echo "$out"; exit 1; }
echo "seeded with the old build"

(cd "$here" && ANDROID_SERIAL=$serial ./gradlew -q :app:assembleDebug :app:assembleDebugAndroidTest)
on_device install -r "$here/app/build/outputs/apk/debug/app-debug.apk" >/dev/null
on_device install -r "$here/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk" >/dev/null
out=$(instrument -e class team.holder.android.UpgradedProfileProbe -e upgradeVerify true)
grep -q "OK (1 test)" <<<"$out" || { echo "UPGRADE CHECK FAILED"; echo "$out"; exit 1; }
echo "upgraded to this build: every card, link, tag, attachment and the encryption check passed, before and after a rebuild"
