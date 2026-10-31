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

   **Local Pixel 2 API 30 emulator on a Wayland desktop.** Gradle stores this
   AVD under `~/.android/avd/gradle-managed`, so a plain `emulator -list-avds`
   can show nothing even when the API 30 image and AVD are installed. Check with:

   ```bash
   ANDROID_AVD_HOME="$HOME/.android/avd/gradle-managed" "$ANDROID_HOME/emulator/emulator" -list-avds
   ```

   On the Fedora Wayland host, Gradle's `pixel2Api30DebugAndroidTest` snapshot
   setup and direct headless launches with `swiftshader_indirect` or `software`
   graphics crashed in the emulator's render thread (exit 139). A direct launch
   with the active Xwayland credentials and host graphics booted successfully.
   Find the current display and `-auth` path with `ps -C Xwayland -o args=`;
   replace the example values below with those from that command:

   ```bash
   export ANDROID_AVD_HOME="$HOME/.android/avd/gradle-managed"
   export XDG_RUNTIME_DIR="/run/user/$(id -u)"
   export WAYLAND_DISPLAY=wayland-0
   export DISPLAY=:0
   export XAUTHORITY=/run/user/1000/.mutter-Xwaylandauth.EXAMPLE
   export QT_QPA_PLATFORM=xcb
   "$ANDROID_HOME/emulator/emulator" -avd dev30_aosp_atd_x86_64_Pixel_2 \
     -no-window -no-audio -no-boot-anim -gpu host -no-snapshot
   ```

   Keep that process running while testing in another terminal. Build the
   x86_64 APKs, confirm the emulator's serial with `adb devices`, then install
   and run the needed instrumentation class:

   ```bash
   ./gradlew :app:assembleDebug :app:assembleDebugAndroidTest -Pholder.android.abis=x86_64
   adb -s emulator-5554 install -r app/build/outputs/apk/debug/app-debug.apk
   adb -s emulator-5554 install -r app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
   adb -s emulator-5554 shell am instrument -w \
     -e class team.holder.android.HolderNativeIntegrationTest \
     team.holder.android.debug.test/androidx.test.runner.AndroidJUnitRunner
   adb -s emulator-5554 emu kill
   ```

   The serial may differ if another emulator is running. These commands passed
   the full 23-test `HolderNativeIntegrationTest` class on the local API 30 AVD.

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
