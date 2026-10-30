# Ubuntu daemon packages in CI

`daemon-ubuntu-packages.yml` supplies the same daemon runtime packages to Daemon CI's
Ubuntu package checks and Desktop CI's Behave job. PRs check Ubuntu 26.04 (`resolute`);
main also checks Ubuntu 24.04 (`noble`). Desktop compilation remains in Desktop CI.

For the default `core_packages=source` mode, each caller:

1. Computes an input key with `ubuntu-daemon-package-key.sh`. It covers the daemon,
   core, API, CLI, common code, versions and package build/download recipes, plus
   the Ubuntu series, amd64 architecture and RelWithDebInfo configuration.
2. Acquires a job lock for that key and Git ref, then checks the Actions cache and
   the matching `HolderTeam/holder-sdk` release. Callers on the same PR share the
   cache; main's cache is also available to PRs. Different refs retain separate
   locks because their writable caches have separate GitHub scopes.
3. On a miss, builds the Debian packages, builds and runs the daemon unit tests
   using the same build directory, and verifies shared-core linkage and package
   version constraints. The self-contained Debian source export is still checked.
4. Bundles `holder-daemon`, the matching `libholder0`, and the exact `libcaste0`
   runtime installed during the build, with `packages.sha256`. Only a successful
   build and test run is cached. Each caller uploads its own workflow artifact.

Behave verifies the checksums and installs that bundle with apt, which resolves
Ubuntu runtime dependencies. It then launches an isolated instance of the installed
`/usr/bin/holderd`. It does not compile a second daemon or require development packages.

Daemon CI publishes main's successfully tested bundles as immutable prereleases
in `holder-sdk`, using the existing `holder-sdk-publish` environment and publishing
app. A separate `ubuntu-daemon-debs-` tag prefix keeps package retention independent
of the staged daemon archives used by other clients. The latest 64 package bundles
are retained. First builds of changed backend inputs still compile; desktop/test
changes reuse a matching backend.

The explicit `core_packages=ppa` dispatch mode continues to test published core
packages. It bypasses bundle reuse and publication because PPA contents can change
without the source key changing. The existing `ci:skip-launchpad-packages` label
still skips Daemon CI's package checks. Behave now requires the package path; a cold
build therefore needs the PPA for its caste build dependency.

The runtime bundle freezes the PPA caste version used for that source build.
To deliberately refresh external dependencies for unchanged source, change the
package build recipe (which changes the key). Neither package version strings nor
an unrelated latest release are used to select a reusable bundle.

Local checks:

```sh
python3 -m unittest discover -s .github/scripts/tests -p 'test_ubuntu_daemon_packages.py' -v
bash -n .github/scripts/ubuntu-daemon-package-key.sh .github/scripts/fetch-daemon-debs.sh .github/scripts/publish-daemon-debs.sh
```
