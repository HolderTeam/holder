# Core builds

Core lives beside the daemon in this repository, in `../core`. The daemon always
builds against the core in the same commit, but it gets it in two ways.

## Local builds

`./make.sh` builds core from `../core` together with the daemon. A change to core
is picked up the next time you build the daemon; there is nothing to download or
update. Diagnostic commands default to `Debug` when building core from source.

```sh
./make.sh build               # Build core from ../core with the daemon
./make.sh test Debug          # Any CMake build type works from source
```

## Choosing core yourself

Set one of these before running `./make.sh` to use something other than `../core`:

| Variable | Core dependency |
| --- | --- |
| `HOLDER_CORE_SOURCE_DIR` | Another core source checkout |
| `HOLDER_CORE_SDK` | An installed core SDK directory |
| `HOLDER_USE_SYSTEM_CORE=ON` | An installed core package (`libholder-dev`) |

An SDK's build type must match the daemon's: SDKs are built as `RelWithDebInfo` or
`Release`. Direct CMake builds use `-DHOLDER_CORE_SOURCE_DIR=<source-path>`, and the
Windows Debug presets use `../core`.

## Windows

`./make.sh` uses the `windows-vcpkg-debug` presets and needs `VCPKG_ROOT` set; vcpkg
installs core's dependencies from this directory's `vcpkg.json`. With
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
