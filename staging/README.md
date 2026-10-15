# Staging
Builds and stages Holder release candidates

For a whole-framework RC or production release, use one
[framework release manifest](docs/framework-release-manifest.md) across Linux,
Windows and macOS. It pins one core SDK revision/configuration, product version
and each platform's component runs and source commits. Release manifests require
Release backend/core artifacts and release Meson desktop artifacts.
Normal development staging follows the latest successful Daemon CI run on main.

## Windows

The `Staging / Windows` workflow assembles the two Windows build outputs into a release-candidate layout:

- `holder-desktop-windows` from this repository's Desktop / CI run
- `holder-daemon-windows-backend` from this repository's Daemon CI run

The desktop is the program the installer's shortcuts start (`bin\holder-desktop.exe`); it starts its own
backend with `holderctl ensure`. There is no launcher.

Run it manually from:

https://github.com/HolderTeam/holder/actions/workflows/staging-windows-stage.yml

By default, staging selects the newest successful Daemon CI run on main, then the
newest successful Desktop / CI run built from the same desktop source: its
`apps/desktop/` Git tree must equal the one at the backend's commit. The two
commits can differ, because Desktop / CI runs on main only when the desktop
changes, but the desktop source cannot. All selected artifacts must be
unexpired. Explicit component run IDs support reproducible staging and are held
to the same rule. Desktop installer metadata is checked out at the selected
desktop run's commit.

Core comes with the backend: Daemon CI builds the daemon against core from the
same commit, and its `core-selection` artifact records that SDK. Staging takes
the selection from the backend run it picked and rejects a backend whose
recorded core differs from it.

The staged directory and installed package preserve `core-build.json` and
`release/holder-core-provenance.json`; `release-candidate.json` records the
core SDK asset checksum and the backend run it came from. `backend_run_commit`
identifies the workflow run revision.

The normal workflow uploads two artifacts:

- `Holder-windows-staged`: unsigned canonical staging directory for release signing.
- `Holder-windows-test-signed-installer`: self-signed installer for trusted manual testing.

Set `sign_test_installer=false` to validate an unsigned branch build without
using the protected signing environment. This still assembles the staged
directory and builds, installs and smoke-tests the unsigned installer.

The self-signed tester installer is only for internal validation. It is not the release signing path. To test it on a clean Windows machine, import the included `Holder-windows-test-signing.cer` into the trusted certificate store only if the fingerprint matches `Holder-windows-test-signing-fingerprint.txt`.

The workflow needs a protected GitHub environment named `windows-staging` with access limited to `main` and these environment secrets:

- `WINDOWS_TEST_SIGNING_PFX`: base64 encoded test signing PFX.
- `WINDOWS_TEST_SIGNING_PFX_PASSWORD`: PFX export password.

If the artifact downloads need cross-repository authentication, add a repository secret named `HOLDER_CI_ARTIFACT_TOKEN` with read-only access to the build artifacts.

## Linux AppImage

The Linux AppImage workflow combines the native Ubuntu 24.04 artifacts of the
desktop (Desktop / CI) and the daemon (Daemon CI), bundles their GTK/GIO runtime, and uploads
both an AppImage and its assembled AppDir:

https://github.com/HolderTeam/holder/actions/workflows/staging-linux-appimage-stage.yml

All workflow inputs are optional. Development staging selects the newest
successful Daemon CI (`daemon-ci.yml`) artifact on main that is still
downloadable, and the newest Desktop / CI (`desktop-ci.yml`) artifact built from
the same desktop source, as for Windows. The Daemon CI run built the daemon against
core from the same commit, and staging takes the core selection from that run.
Daemon CI runs on main only when the daemon, core or their shared code changes,
and its artifacts last 7 days, so after a quiet week run Daemon CI on main
again before staging.

Run IDs, repositories, branches and the AppImage version can be overridden for
compatibility testing. An older backend run from `HolderTeam/holder-core`'s
`daemon-integration.yml` still works while its artifacts last: it recorded the
published SDK it used in the same way.

For an RC or release, commit a JSON manifest to this repository and supply its
relative path through `release_manifest`. The manifest controls the version,
repositories, immutable component runs and core commit; leave run ID and product
version overrides at their defaults. The core commit is the one the backend
run's core selection records. For example:

```json
{
  "appimage_version": "0.2.1-rc.1",
  "core": {"commit": "<full core SHA>", "build_type": "RelWithDebInfo"},
  "desktop": {
    "repository": "HolderTeam/holder",
    "run_id": "<successful Desktop / CI run ID>",
    "commit": "<full desktop SHA>"
  },
  "backend": {
    "repository": "HolderTeam/holder",
    "run_id": "<successful Daemon CI run ID>",
    "commit": "<full daemon SHA>"
  }
}
```

Every component commit and the core build configuration are checked against
the extracted artifacts. Development daemon artifacts use `RelWithDebInfo`;
production artifacts use daemon's `core_build_type=Release` workflow input and
distinct `-release` artifact names.
Component Actions artifacts expire, so stage the pinned candidate while they
are available. Signing and promotion use that staged AppImage afterwards.

Desktop and daemon package versions do not have to be identical. The workflow
uses the compatibility metadata published in their artifacts instead: the daemon
declares one API version and the desktop declares the supported API range. A
stable AppImage version will not accept prerelease component builds.

The `Holder-linux-appimage-staged` artifact contains:

- `Holder-<version>-x86_64.AppImage`
- A matching AppDir archive for inspection and debugging
- SHA-256 checksums and component/core provenance, including the SDK asset digest

The AppDir and final AppImage retain daemon's original `core-build.json` and
`holder-core-provenance.json`. The external product provenance also includes
the same core selection. A supplied framework release manifest is bundled in
`usr/share/holder/release/holder-framework-release.json`.

`AppRun` sets up the bundled runtime (library, GTK and schema paths) and starts the
desktop. The desktop starts its own backend with `holderctl ensure` (the one in the
AppImage's `usr/bin`): it uses an already-running compatible daemon when there is one,
and otherwise starts the bundled `holderd` with `--idle-exit 60`, so the daemon stops
by itself shortly after the window closes. `AppRun` no longer owns or stops a daemon. With
`HOLDER_APPRUN_SMOKE_TEST=1` it starts the bundled daemon the same way, checks it is healthy
within the API range the desktop was built for, stops the daemon it started and exits (CI and
the release verification use this).

## Mac

macOS staging takes core from the backend run in the same way and rejects
mismatched backend SDKs. Artifact runs must be successful and unexpired, and desktop
packaging metadata comes from the selected run's exact source commit. Core
provenance is retained in `Holder.app/Contents/Resources/core-build.json`,
`release/holder-core-provenance.json` and the release-candidate metadata.
Explicit component run overrides support reproducible staging.

Homebrew's GTK is built without the macOS accessibility bridge (AccessKit), so staging rebuilds the
`libgtk-4` library from Homebrew's source version with `-Daccesskit=enabled`, using checksum-pinned
AccessKit C 0.18.0, before bundling (`staging/scripts/build-macos-accessible-gtk.sh`). Staging fails if the
final bundle's GTK no longer depends on AccessKit, and records the versions in `gtk-accessibility-build.json`.
Before a release, try a staged app with VoiceOver: a linked bridge does not mean every Holder interaction is accessible.

How to manually test a prerelease development version of Holder, aka a "staged copy".

1. Go to the Mac OS staged app action:

https://github.com/HolderTeam/holder/actions/workflows/staging-macos-stage.yml

2. Click on the latest (or your target) successful run.

3. Look under Artifacts

4. Click the Download button.

5. Open the Terminal (Applications/Utility).

6. Get to where you downloaded the asset. E.g.

	cd ~/Downloads

7. Check the artifact has all the files it needs:

  codesign --verify --deep --strict --verbose=2 Holder.app

8. For local test only, remove quarantine:

  xattr -dr com.apple.quarantine Holder.app

9. Open it:

  open Holder.app
