# holder_frontend_tests

Shared behavior-level frontend tests for Holder clients.

This package is intended to hold:
- Shared `.feature` files
- Shared step orchestration
- Per-client driver adapters (Linux, macOS, Windows)

## Current status

Implemented first vertical slice:
- Feature: create a card
- Feature: toggle toolbox panel
- Driver: Linux dogtail adapter

## Quick start (Linux)

On Fedora:

```sh
sudo dnf install python3-behave python3-dogtail python3-xlib xorg-x11-server-Xvfb at-spi2-core dbus-daemon
```

On Ubuntu:

```sh
sudo apt update
sudo apt install -y python3-behave python3-dogtail python3-xlib xvfb at-spi2-core dbus-x11 x11-utils
```

From `integration`, build the daemon and desktop from the same checkout, then run the headless suite:

```sh
cmake --build ../../../../daemon/build --parallel 4
meson compile -C ../build holder-desktop

./make.sh
```

If your app binary is elsewhere:

```bash
HOLDER_FRONTEND_APP_PATH=/path/to/holder-desktop ./make.sh
```

Run visibly on your desktop (debug mode):

```bash
HOLDER_TEST_HEADLESS=0 ./make.sh
```

Optional fallback for maintainers who want editable package installs:

```bash
./make.sh install-dev
```

## What `./make.sh` runs

- A single Behave run for scenarios tagged `@linux`
- Entire run executes with an isolated Holder backend instance
- Entire run executes inside `dbus-run-session` + `xvfb-run` for headless UI automation

## Environment

- `HOLDER_FRONTEND_TARGET` (default: `linux`)
- `HOLDER_FRONTEND_APP_PATH` (default: `../build/holder-desktop`)
- `HOLDER_TEST_HEADLESS` (default: `1`; set `0` for headed/visible run)

## Runner design

- `./make.sh` is intentionally minimal and delegates to `./run.py`.
- `run.py` contains orchestration (Behave invocation + isolated backend lifecycle).

## Next steps

- Add backend-aware lifecycle fixture hooks (shared)
- Add macOS and Windows drivers implementing the same driver contract
- Expand shared features gradually (search, edit/save, AI thread/run)

Use `./make.sh linux --access` to opt into the shared source-runtime selector.
Linux uses AT-SPI. The shared driver adapter applies runtime selection directly
at the native process launch; future macOS drivers can use the same adapter.
See the desktop README's accessible source runtime section for preparation,
inspection and macOS loader details. This does not add a macOS GUI driver.
