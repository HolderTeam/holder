# Core builds

Core lives beside the daemon in this repository, in `../core`. By default the
build helper uses core from this checkout, built and installed separately. Daemon CMake consumes the installed package.

## Local builds

`./make.sh` first configures and incrementally builds core in `../core/build`,
then installs its library, headers and resources into `../core/out/install/local`.
Only then does it configure and build the daemon against that installed package.
It reuses the ordinary core build directory, so building core yourself does not
create a second copy inside the daemon build. A change to core is picked up on the
next invocation; unchanged files are not recompiled. Core's tests remain a separate
step, run with `./make.sh test` in core's directory.

Diagnostic commands default to `Debug` for automatic local builds. They use
separate core build and install directories per diagnostic command and forward
compiler/instrumentation flags, keeping sanitizer and coverage builds isolated.

```sh
./make.sh build               # Update/install core, then build the daemon
./make.sh test Debug          # Build both as Debug, then run daemon tests
```

To manage the steps independently, start at the repository root:

```sh
(cd core && ./make.sh install RelWithDebInfo)
(cd daemon && HOLDER_CORE_INSTALL="$PWD/../core/out/install/local" ./make.sh build)
```

With `HOLDER_CORE_INSTALL` set, the daemon helper does not build or install core.
You are responsible for updating that installation after core changes. Direct
CMake builds can use `-DHOLDER_CORE_INSTALL=/absolute/path/to/install` too. The
installed package records its build type; daemon rejects a mismatch.

## Choosing core yourself

Set one of these before running `./make.sh` to select a dependency explicitly:

| Variable | Core dependency |
| --- | --- |
| `HOLDER_CORE_INSTALL` | Installed local core package; no automatic core build |
| `HOLDER_CORE_SOURCE_DIR` | Explicit opt-in to compiling a core checkout inside the daemon build |
| `HOLDER_CORE_SDK` | An installed core SDK directory |
| `HOLDER_USE_SYSTEM_CORE=ON` | An installed core package (`libholder-dev`) |

An SDK's build type must match the daemon's: SDKs are built as `RelWithDebInfo` or
`Release`. The source override remains available for development, but is not the
default.

## Windows

`./make.sh` uses the `windows-vcpkg-debug` presets and needs `VCPKG_ROOT` set; vcpkg
installs the daemon's dependencies from this directory's `vcpkg.json`. The helper
builds and installs core separately with its own vcpkg dependencies before
configuring the daemon; it overrides the presets' source embedding option. Direct
use of the Windows Debug presets still opts into source embedding. With
`HOLDER_CORE_SDK` set, it uses the `windows-sdk-tests` preset instead: the SDK
includes core's dependencies, and `packaging/windows/sdk-deps` installs the
additional Boost dependencies for the daemon.

## CI and releases

CI builds the daemon against core's installed SDK, never by compiling core inside
the daemon build, so the daemon can only use core's public interface. The
`Core / SDK` workflow builds the SDK from `core/` in the same commit, caches it by
`core/`'s Git tree, and records it in the run's `core-selection` artifact. Every
platform job uses that SDK, and its configuration matches the daemon's build type.

For a release, run Daemon CI manually with `core_build_type=Release`. All three
platforms build, test and check the release artifacts. Release artifact names end
in `-release`; ordinary CI builds use `RelWithDebInfo`.

Artifacts include core's schema and welcome resource. `core-build.json` records
which core SDK was used. Each artifact's `release/` directory records the daemon's
commit, version and API version. Record the CI run and component commits in the
framework release manifest.

Ubuntu source packages use `libholder-dev` and its shared `libholder0` runtime
through `HOLDER_USE_SYSTEM_CORE=ON`. CI checks these packages separately: it builds
`libholder0` and `libholder-dev` from `core/` in the same commit, then builds and
installs the daemon package against them. Before a release, run Daemon CI manually
with `core_packages=ppa` to build against the published Launchpad packages instead.
