# Building Holder Kit

This guide covers building and installing a checkout. See
[Development](development.md) for tests and code changes, and
[Packaging](packaging.md) for wheels and Ubuntu packages.

## Prerequisites

Holder Kit requires Python 3.10 or newer with virtualenv support and development
headers, CMake 3.22 or newer, and C/C++ compilers. Linux, macOS and BSD builds
also need `pkg-config` and core's native development dependencies. Use core's
README (`../../core/README.md`) and CMake configuration for that dependency list.

On macOS, install Xcode Command Line Tools and the required Homebrew packages.
The developer script discovers Homebrew and its OpenSSL prefixes; set
`CMAKE_PREFIX_PATH` if CMake cannot find a library. On BSD, use the platform's
ports or package manager. The script does not install system packages.

## Build with core source

`make.sh` is the Bash entry point on Linux, macOS and BSD. It runs
`scripts/develop.py`, creates or reuses `.venv`, and installs Holder Kit as an
editable package with all test dependencies, including pandas and NetworkX.

From `python/kit/` in a checkout of the holder repository:

```sh
./make.sh setup
```

This builds core from source in the same checkout (`../../core`), so Kit is
always built against the core beside it. Set `HOLDER_KIT_CORE_SOURCE` to build
against a different core source directory. Builds leave the checkout on its
current branch and revision. They print core's revision and working-tree status,
and record them in `out/make/core-source.json`.

Before creating a virtualenv or installing Python packages, the source workflow
checks tools, Python headers and core's CMake prerequisites with
`BUILD_TESTING=OFF`. If configuration fails, fix the reported prerequisite and
rerun the command.

The script uses its own directory and invokes the virtualenv directly; shell
activation is optional.

For a manual build without the developer script, create a virtualenv and pass
the core source path explicitly:

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -e . \
  --config-settings=cmake.define.HOLDER_KIT_CORE_SOURCE="$PWD/../../core"
```

Direct pip/CMake builds require an explicit core selection; the default above
belongs to the developer script.

## Build with a core SDK

SDK builds use a prebuilt core library instead of building core: a Holder
release's core SDK, or one that a CI run's `Core / SDK` workflow built. Fetch
and verify it with core's `scripts/core-sdk.py`, then point `HOLDER_CORE_SDK`
at the prepared SDK directory (the one containing `libholder-manifest.json`):

```sh
HOLDER_CORE_SDK=/path/to/sdk ./make.sh --sdk setup
```

`HOLDER_CORE_SDK` selects the SDK workflow unless `HOLDER_KIT_CORE_SOURCE` is
set. Combining `--sdk` with an explicit source path is rejected, and `--sdk`
without a prepared SDK stops before creating a virtualenv. For manual builds on
Linux or macOS, set `HOLDER_CORE_SDK` and `SKBUILD_CMAKE_BUILD_TYPE` (matching
the SDK's configuration) and run `python -m pip install -e .`.

Linux and macOS SDK builds still need core's native development dependencies.
The Linux SDK uses the Ubuntu 24.04 dependency ABI. Mixing it with another
distribution's C++ dependency ABI can cause import failures; use a source build
or the series-specific [Ubuntu package](packaging.md#ubuntu-packages).

### Windows

Use an MSVC developer environment and a supported Python. Run the Python helper
directly if Bash is unavailable:

```powershell
$env:HOLDER_CORE_SDK = "C:\path\to\sdk"
python scripts/develop.py --sdk setup
```

The helper configures the SDK's bundled vcpkg toolchain and prebuilt dependencies.
For direct pip/CMake builds, set `HOLDER_CORE_SDK` and pass these CMake
definitions:

| Definition | Value |
| --- | --- |
| `CMAKE_TOOLCHAIN_FILE` | `<sdk>/vcpkg/scripts/buildsystems/vcpkg.cmake` |
| `VCPKG_INSTALLED_DIR` | `<sdk>/vcpkg/installed` |
| `VCPKG_TARGET_TRIPLET` | `x64-windows` |
| `VCPKG_MANIFEST_MODE` | `OFF` |
| `VCPKG_APPLOCAL_DEPS` | `OFF` |

Holder Kit does not bootstrap vcpkg or compile the SDK's dependencies. Windows
SDKs without bundled development dependencies are rejected.

## Build settings and output

Build commands accept `Debug`, `Release`, `RelWithDebInfo` or `MinSizeRel`:

```sh
./make.sh setup Debug
./make.sh --sdk setup Release
```

The default is `RelWithDebInfo`. Source and SDK builds, and each build type, have
separate native directories under `build/make`.

| Environment variable | Purpose |
| --- | --- |
| `HOLDER_KIT` | Bootstrap Python interpreter; otherwise reuse the selected venv or use `python3` |
| `HOLDER_KIT_VENV` | Virtualenv path; defaults to `.venv` |
| `HOLDER_KIT_CORE_SOURCE` | Core source directory; defaults to `../../core` |
| `HOLDER_CORE_SDK` | Prepared SDK directory containing `libholder-manifest.json` |
| `BUILD_TYPE` | Default native build type |
| `CMAKE_BUILD_PARALLEL_LEVEL` | Build parallelism limit |

`./make.sh clean` removes only `build/make` and `out/make`. It retains `.venv`
and other build directories. Run
`./make.sh --help` for the complete command reference.
