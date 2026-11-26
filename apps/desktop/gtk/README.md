# holder-desktop

GTK desktop frontend for Holder, plus behavior-level integration tests.

## Repository layout

- `main.vala` - thin GTK app entrypoint
- `src/` - Vala application source
- `tests/` - GLib/Meson unit and controller/view tests
- `data/` - GSettings schemas, desktop metadata, and resources
- `help/` - in-app help markdown
- `docs/` - desktop architecture/product planning docs
- `integration/` - behavior tests that launch the GTK app

## Development

This frontend uses Vala, GTK4/Libadwaita, Meson, and GLib.Test.

Keep the app a thin client of the daemon. Storage, indexing, search, Git
operations, AI execution and reusable domain behavior belong in the backend
or core. Use native GTK/libadwaita behavior and preserve the supported runtime
baseline.

The app uses only the documented [HTTP contract](../../../api/openapi.yaml),
not daemon or core internals or undocumented response behavior. If the app
needs a new behavior, update the contract first and follow
[Changing the contract](../../../api/README.md#changing-the-contract).
Describe changes in meaning in the contract. `compatibility/daemon-api.json`
records the `API_VERSION` range the app accepts.

Fresh Ubuntu setup:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  meson ninja-build pkg-config valac \
  libgtk-4-dev libadwaita-1-dev libgtksourceview-5-dev \
  libspelling-1-dev \
  libgee-0.8-dev libsoup-3.0-dev libjson-glib-dev libvte-2.91-gtk4-dev \
  at-spi2-core dbus-x11 xvfb
```

Fresh macOS setup:

```bash
brew update
brew install \
  git \
  meson \
  ninja \
  pkgconf \
  vala \
  gtk4 \
  adwaita-icon-theme \
  libadwaita \
  gtksourceview5 \
  libspelling \
  libgee \
  libsoup \
  json-glib \
  vte3
```

On macOS, `./make.sh` launches with [accessibility enabled](#accessible-source-runtime).
Install Rust with `brew install rust` for that workflow, or run `./make.sh noaccess`
to use Homebrew GTK directly.

Fresh Fedora setup:
```bash
sudo dnf install xorg-x11-server-Xvfb at-spi2-core dbus-daemon
```

On Linux, `./make.sh` launches with GTK's standard AT-SPI accessibility backend
enabled. `./make.sh test` and `./make.sh coverage` also enable AT-SPI, using Xvfb
and a private D-Bus session so the accessibility bus does not depend on the
developer's desktop session. The distribution GTK package supplies the backend;
no custom GTK build is needed. Direct `meson test` runs enable AT-SPI too and
need a working accessibility bus. On a headless machine, use
`GDK_BACKEND=x11 xvfb-run -a dbus-run-session -- python3 integration/run.py --headless-child meson test -C build`
to start the AT-SPI services using the same runner as the Behave suite.

Configure, build, and test on macOS:

```bash
meson setup build-macos --prefix=/
meson compile -C build-macos
accessible_runtime="$(python3 scripts/accessible_runtime.py prepare)"
HOLDER_DESKTOP_TEST_ACCESSIBLE_RUNTIME="$accessible_runtime" \
  meson test -C build-macos --print-errorlogs
```

Meson's default test setup runs each desktop test executable in its own
`holder_gtk_test_*` scratch tree. This includes directories made by GLib's
`DirUtils.make_tmp`, files placed under `Environment.get_tmp_dir()`, and the
test process's XDG homes. The wrapper removes the tree after a passing or
failing test; direct execution of a test binary bypasses the wrapper. Set
`HOLDER_TEST_KEEP_TMP=1` when running `meson test` to retain the tree and print
its location. A crash or forced termination of the wrapper can leave its
tree behind for manual inspection or removal. The wrapper never removes
directories supplied by a caller as application profiles.

To run the frontend directly:

```bash
./build-macos/holder-desktop
```

The desktop app expects `holderd` to already be running. For local macOS development, build and
start the backend from `daemon/` in the `holder` repository first, or run a staged `.app` bundle that
contains the backend and launcher.

## How the app finds its backend

On Linux the app runs `holderctl ensure` itself at startup (the `holderctl` installed beside it, or on
`PATH`). It starts the backend if none is running, passes the daemon API range from
`compatibility/daemon-api.json`, and shows "Starting Holder..." meanwhile or the reason it failed. A backend
started this way is given `--idle-exit 60`, so it stops by itself about a minute after the app goes away.
An already-running backend, such as the systemd service on Ubuntu, is left as it is.

While it runs the app holds the backend's `GET /events` stream open. The backend counts an open event stream
as "a client is here" and does not idle out under it; the app reconnects with a growing delay, and looks the
backend up again each time, if the connection drops. Nothing is read from the stream yet.

When running from a source tree, set `HOLDER_CTL` to a built `holderctl`; `./make.sh run` does this itself when
`daemon/build/holderctl` exists in this repository (`../../../daemon/build/holderctl`). The Debug tool says what was used or why the step was skipped.

### Diagnosing failed API requests

The Desktop JSON request layer reports connection failures, timeouts, HTTP
errors, and unreadable response envelopes. This covers startup health checks,
project and card operations, and storage setup. Error feedback gives the next
step and keeps specific server advice for validation failures and conflicts.

The Debug tool records the HTTP method, route with IDs omitted, failure
category, elapsed time, and HTTP status or transport error code. Request
diagnostics leave out URLs, headers, query values, bodies, and raw error text.
When no window is listening, the same diagnostic is available in GLib debug
output with `G_MESSAGES_DEBUG=all`. Backend discovery, file downloads,
event streams, and endpoint-specific payload validation retain their own
reporting.

Windows and macOS still start the backend through their launcher for now.

Run the fast headless-safe test suite from this directory:

```bash
./make.sh test
```

Coverage support also needs `gcovr`:

```bash
sudo apt install -y gcovr
./make.sh coverage
```

Coverage outputs:

- `build-coverage/coverage/summary-lines.txt`
- `build-coverage/coverage/summary-branches.txt`
- `build-coverage/coverage/coverage.json`
- `build-coverage/coverage/index.html`

Two pre-release checks, each in its own build directory:

```bash
./make.sh warnings   # valac and the hand-written C with warnings as errors (build-warnings)
./make.sh san        # the tests under AddressSanitizer, UBSan and leak detection (build-san)
```

`san` reports a leak only when nothing can reach the memory any more. Fontconfig, Pango, HarfBuzz,
Mesa, libxml2 and VTE keep caches until exit, so `tools/lsan/gtk-stack.supp` suppresses those
libraries, and never GTK, GLib or GObject: an object our code creates and loses is still reported.
In Vala that is usually a reference cycle:

- A lambda that uses only `this` is connected without holding it, but any closure that also captures
  a local gets a block, shared by every closure in that method, which holds `this` and each local any
  of them captures. Stored by something `this` owns (a draw function, a list factory, a delegate
  handed to a member), it keeps both alive: build it in a static method holding the owner `unowned`,
  as `HistoryLaneGutter` and `TrashToolView` do.
- A handler that captures the object it is connected on keeps that object alive. Take it from the
  signal instead: a lambda with one parameter more than the signal gets the sender first, as in
  `notify["active"].connect((row, pspec) => ...)`.
- A dialog or form built in one method, whose handlers capture its own widgets, registers them
  with `SignalScope`: `until_closed(dialog)` or `until_removed(widget)` disconnects them when it is
  done with.

`tests/app_views/lifetime_test.vala` checks that the tool views, the preferences dialog and a scoped
dialog and form are freed once their last reference goes.

## Shared integration tests

From `integration`:

```bash
sudo apt update
sudo apt install -y python3-behave python3-dogtail xvfb at-spi2-core dbus-x11 x11-utils

./make.sh
```

By default the integration runner uses:
- `../build/holder-desktop`

Override with:
- `HOLDER_FRONTEND_APP_PATH=/path/to/holder-desktop`

Optional maintainer fallback:
- `./integration/make.sh install-dev`

## Accessible source runtime

On macOS, `./make.sh`, `./make.sh run`, and `./make.sh access` build Holder
against Homebrew GTK and use the same prepared AccessKit runtime for the test
suite and app launch. Standalone tests, coverage, and macOS Desktop CI also
run the suite with AccessKit. Use `./make.sh noaccess` for a build and launch
with Homebrew GTK that skips the test suite. On Linux, `./make.sh access`
selects GTK's existing AT-SPI backend for the app launch.

On macOS, install the development dependencies above plus Rust (`brew install
rust`) and Xcode Command Line Tools (`xcode-select --install`). First use needs
network access and builds an isolated copy of the Homebrew-selected GTK source
with AccessKit C 0.18.0. Preparation can take several minutes. The shared recipe
is `../../../release/staging/scripts/build-macos-accessible-gtk.sh`, taken from PR #26;
source archives are checksum verified. The recipe builds into a separate
directory and macOS staging uses the same recipe for the packaged app.

The selector caches under `build-access-runtime/` (override with
`HOLDER_ACCESS_CACHE=/absolute/path`). Cache identity includes source provenance,
the recipe and selector, architecture, macOS/SDK/compiler, build configuration,
and the versions, locations and dylib hashes of GTK consumers and their
pkg-config dependencies. Dependency upgrades select a new cache. Installed GTK
must match Homebrew's source metadata; incompatible AccessKit APIs fail Meson
configuration rather than silently omitting accessibility. A modified or
incomplete cache is rejected with its directory path: remove that specific
directory and retry to rebuild. Old completed cache directories may be removed
when no running app uses them.

Prepare without opening Holder, or inspect the loaded libraries:

```bash
python3 scripts/accessible_runtime.py prepare
python3 scripts/accessible_runtime.py inspect
python3 scripts/accessible_runtime.py check-menus
```

Preparation and inspection validate the cache and run a native probe that loads libadwaita,
GtkSourceView, VTE and libspelling together. Inspection reports GTK's version and
all loaded image paths and rejects multiple GTK libraries or AccessKit loaded
outside the selected runtime. Source checksums are retained in the runtime's
`gtk-accessibility-build.json`; cache inputs and artifact hashes live beside it
in `identity.json`.

`check-menus` additionally opens a temporary test window and checks its own
native macOS accessibility objects for menu names, mnemonic stripping,
disabled/check states, focus, bounds, activation, reopening, changed labels and
submenus, plus normal label roles, markup, and explicit names across text
updates and reset. It does not use Holder data or require external accessibility
permissions. GTK's internal menu label uses its normal `LABEL` role from GTK
4.24.2 on ([GTK MR !10510](https://gitlab.gnome.org/GNOME/gtk/-/merge_requests/10510),
backported in !10582), so the shared recipe builds Homebrew's GTK source
unpatched. Upgrade GTK with `brew upgrade gtk4` before preparing the runtime
if the installed version predates 4.24.2.

The launcher sets `GTK_A11Y=accesskit` and `DYLD_LIBRARY_PATH` immediately before
executing the native Holder binary. Do not put `/bin/sh`, `/usr/bin/env`, or
`open -a` between selection and the app: macOS can strip loader variables at
those boundaries. `run` accepts a native executable and its arguments, for
example `python3 scripts/accessible_runtime.py run ./build/holder-desktop`.
When running directly, supply `GSETTINGS_SCHEMA_DIR="$PWD/build/data"` as needed;
`./make.sh` supplies the normal source-tree environment automatically.

GUI test drivers can import `scripts/accessible_runtime.py` and call
`launch_environment(base_env)` directly at `subprocess.Popen([native_app],
env=...)`. The integration drivers share this adapter in `drivers/base.py`;
`./integration/make.sh linux --access` opts into it and uses AT-SPI. The existing
integration suite still has only a Linux driver; this mode does not add a macOS
automation driver.

For manual macOS verification, open Accessibility Inspector (Xcode's developer
tools) or turn on VoiceOver and inspect the launched Holder process. Check named
project/card controls, search, editor text/caret and keyboard focus; open and
close a dialog and the app-menu popover. Check actual app mappings with
`vmmap <PID>` and confirm one `libgtk-4.1.dylib` from the printed runtime plus its
AccessKit library. Check menu navigation and actions with VoiceOver as well as
the automated native menu test; full VoiceOver usability remains unverified.
The normal label role exposes a text child under each menu item; specifically
check for duplicate announcements or extra navigation stops.

Cache lifecycle tests can be run with
`python3 tests/accessible_runtime_test.py`.

Native validation on 2026-11-26 (Apple Silicon): a fresh unpatched GTK 4.24.2
runtime with AccessKit C 0.18.0 passed loaded-library verification and every
`check-menus` stage. GDK frame-timing warnings and `gdk_surface_thaw_updates`
assertions appeared, but the test exited successfully. VoiceOver navigation,
announcements, and editor text/caret behavior remain manual checks. See
[issue #398](https://github.com/HolderTeam/holder/issues/398#issuecomment-7198144266)
for the validation details.
