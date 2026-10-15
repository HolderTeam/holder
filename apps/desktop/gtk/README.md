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
  libgee-0.8-dev libsoup-3.0-dev libjson-glib-dev libvte-2.91-gtk4-dev
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

Fresh Fedora setup:
```bash
sudo dnf install xorg-x11-server-Xvfb
```

Configure, build, and test:

```bash
meson setup build-macos --prefix=/
meson compile -C build-macos
meson test -C build-macos --print-errorlogs
```

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

## Opt-in accessible source runtime

From this directory, run `./make.sh access` to build, test, and launch with
accessibility enabled. The existing `build`, `test`, `run`, and `coverage` modes
keep their current runtime. On Linux, `access` uses GTK's existing AT-SPI backend.
Windows does not currently offer this mode.

On macOS, install the development dependencies above plus Rust (`brew install
rust`) and Xcode Command Line Tools (`xcode-select --install`). First use needs
network access and builds an isolated copy of the Homebrew-selected GTK source
with AccessKit C 0.18.0. Preparation can take several minutes. The shared recipe
is `../../../staging/scripts/build-macos-accessible-gtk.sh`, taken from PR #26;
source archives are checksum verified. It does not install into or alter any
Homebrew keg, and does not enable accessibility in packaged releases.

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
permissions. The shared recipe patches GTK's internal menu label to use its
normal `LABEL` role, preserving the existing `labelled-by` relation and matching
ordinary GTK buttons. This replaces the earlier AccessKit bridge workaround;
the new patch name and checksum select a fresh runtime cache and are retained
in source provenance. See `../../../staging/scripts/patches/README.md`.

The launcher sets `GTK_A11Y=accesskit` and `DYLD_LIBRARY_PATH` immediately before
executing the native Holder binary. Do not put `/bin/sh`, `/usr/bin/env`, or
`open -a` between selection and the app: macOS can strip loader variables at
those boundaries. `run` accepts a native executable and its arguments, for
example `python3 scripts/accessible_runtime.py run ./build/holder-desktop`.
When running directly, supply `GSETTINGS_SCHEMA_DIR="$PWD/build/data"` as needed;
`./make.sh access` supplies the normal source-tree environment automatically.

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
AccessKit library. PR #26 found that the bridge exposes named controls but did
not establish complete VoiceOver usability. Check menu navigation and actions
with VoiceOver as well as the automated native menu test.
The normal label role exposes a text child under each menu item; specifically
check for duplicate announcements or extra navigation stops.

Cache lifecycle tests can be run with
`python3 tests/accessible_runtime_test.py`.

Native validation on 2026-10-15 (Apple Silicon, GTK 4.24.1, AccessKit C 0.18.0):
first-use preparation and reuse passed, including reuse from both the repository
root and desktop directory. `./make.sh access` passed all 157 Meson tests and
launched; `vmmap` confirmed one isolated GTK/AccessKit pair alongside Homebrew
libadwaita 1.10.0, GtkSourceView 5.22.0 and VTE 0.84.1. The native observer needed
a temporary `.app` wrapper containing a copy of the same source binary. It
exposed named toolbar controls, toggle states, project/card lists, search and
the editor role; keyboard focus and opening/dismissing the new-project dialog
worked. The initial unpatched runtime exposed menu items with empty native
titles, which the observer omitted. The normal-label-role patch fixes those
names and GTK's own name calculation, replacing the initial backend workaround.
Holder's menu now exposes Find/Replace, Move Card to Trash, Print, Local info,
Preferences and About in the native observer; activating About opens its dialog.
Editor text/caret and full VoiceOver usability remain unchecked. Dependency
changes and damaged or incomplete caches are covered by the cache lifecycle
tests.

The replacement normal-label-role runtime was freshly prepared and reused on
the same Mac. Its expanded native regression passes names, markup, explicit
name preservation/reset, focus, states, activation and reopening; it rejects
the former bridge workaround's presentation role. `./make.sh access` still
passes all 157 Meson tests. VoiceOver announcement/navigation behavior remains
a manual check.
