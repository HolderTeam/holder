Use the native Android testing stack. Do not duplicate libholder/C API tests in Kotlin.

1. **C/C++ tests**
   - Keep core's own Catch2/C API coverage (in `core/`) for business logic, Git, crypto, cards, projects, search, etc.

2. **Kotlin unit tests**
   - JUnit for ViewModels, state handling, validation and Kotlin↔native result mapping.
   - Prefer plain JVM tests where Android framework APIs are not required.

3. **Compose UI tests**
   - Use `androidx.compose.ui.test`.
   - Test screens and user behaviour through the Compose semantics tree.
   - Cover project list, card list, card view/edit, search, save/discard, navigation and wikilinks.
   - This is the preferred replacement for Behave-style frontend tests.

4. **Android/JNI integration tests**
   - Put in `androidTest`.
   - Verify Kotlin → JNI → libholder → Android filesystem/native dependencies → Kotlin.
   - Test representative round trips, UTF-8, errors, persistence, Git operations, Android Keystore integration and reconstruction of the disposable SQLite projection.
   - Do not rerun every C API test through JNI.

5. **Whole-app smoke tests**
   - Small number of critical flows, e.g.:
     - launch app
     - open Home project
     - create card
     - edit/save
     - reopen and verify persistence

   - Use Compose UI Test; use UI Automator only where interaction outside the Compose app is required.
   - GitHub sign-in and repository setup leave the app for the browser, so
     `GitHubMockSignInTest` uses UI Automator against the local mock GitHub;
     `mock-github/run_device_tests.py` runs it on Android 9 and Android 17 emulators. See
     [mock-github/README.md](mock-github/README.md).

6. **Device matrix**
   - Use Gradle Managed Devices in CI.
   - Minimum:
     - API 28, Holder's minimum Android version
     - one current Android API

   - Do not test every API level.

   To run the instrumentation tests on a local emulator, see
   [Running instrumentation tests on a local emulator](#running-instrumentation-tests-on-a-local-emulator).

7. **Screenshot tests**
   - Add after core behavioural tests.
   - Capture major screens in light/dark mode and useful size/font configurations.
   - Intended to catch visual regressions such as theme flashes, broken layouts and contrast issues.

8. **Physical-device smoke testing**
   - Before releases, test at least:
     - old supported Android device
     - current Pixel
     - non-Google OEM device
   - `scripts/deploy-all.sh` builds once and installs + launches on every connected
     device/emulator `adb` sees, for quickly spreading a manual check across
     several of these at once.

9. **Later**
   - Robolectric only where simulated Android framework behaviour is genuinely useful.
   - Macrobenchmark/Baseline Profiles for startup, scrolling, search and large-project performance once functionality stabilises.

Target testing pyramid:

`C API tests >> Kotlin tests > Compose tests > JNI/integration tests > full-app E2E tests`

## Running instrumentation tests on a local emulator

These steps run `androidTest` classes on the Pixel 2 API 30 emulator that
Gradle manages, on a Fedora Wayland desktop.

**SDK tools.** The commands below assume `ANDROID_HOME` names the SDK
(`sdk.dir` in `local.properties`) and that `adb` is on `PATH`:

```bash
export ANDROID_HOME="$HOME/Android/Sdk"
export PATH="$ANDROID_HOME/platform-tools:$PATH"
```

**The AVD.** It is the `pixel2Api30` managed device in `app/build.gradle.kts`.
Gradle creates it, with its system image, the first time it is needed:

```bash
./gradlew :app:pixel2Api30Setup
```

Gradle keeps it under `~/.android/avd/gradle-managed`, so a plain
`emulator -list-avds` shows nothing. Check it is there with:

```bash
ANDROID_AVD_HOME="$HOME/.android/avd/gradle-managed" "$ANDROID_HOME/emulator/emulator" -list-avds
```

**Starting it.** Gradle's own `pixel2Api30DebugAndroidTest` snapshot setup,
and headless launches with `swiftshader_indirect` or `software` graphics,
crash in the emulator's render thread (exit 139). Launch it directly with host
graphics and the session's Xwayland credentials, which `ps` shows as Xwayland's
`-auth` argument:

```bash
export ANDROID_AVD_HOME="$HOME/.android/avd/gradle-managed"
export XDG_RUNTIME_DIR="/run/user/$(id -u)"
export WAYLAND_DISPLAY=wayland-0
export DISPLAY=:0
export XAUTHORITY="$(ps -C Xwayland -o args= | sed -n 's/.*-auth \([^ ]*\).*/\1/p' | head -1)"
export QT_QPA_PLATFORM=xcb
"$ANDROID_HOME/emulator/emulator" -avd dev30_aosp_atd_x86_64_Pixel_2 \
  -no-window -no-audio -no-boot-anim -gpu host -no-snapshot
```

Keep that running, and in another terminal wait until it has booted:

```bash
adb -s emulator-5554 wait-for-device shell \
  'while [ -z "$(getprop sys.boot_completed)" ]; do sleep 2; done'
```

**Which device.** `adb devices` lists every emulator and connected phone. With
more than one, pass the emulator's serial (usually `emulator-5554`) with `-s`,
as below, or set `ANDROID_SERIAL`.

**Running the tests.** Build the x86_64 APKs, install both, and run the
instrumentation class you need:

```bash
./gradlew :app:assembleDebug :app:assembleDebugAndroidTest -Pholder.android.abis=x86_64
adb -s emulator-5554 install -r app/build/outputs/apk/debug/app-debug.apk
adb -s emulator-5554 install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb -s emulator-5554 shell am instrument -w \
  -e class team.holder.android.HolderNativeIntegrationTest \
  team.holder.android.debug.test/androidx.test.runner.AndroidJUnitRunner
```

Stop the emulator with `adb -s emulator-5554 emu kill`. These steps passed all
41 tests in `HolderNativeIntegrationTest` on the API 30 AVD.
