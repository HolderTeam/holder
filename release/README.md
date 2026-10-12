# Release

Signing, promotion and publishing of Holder release assets.

The workflows are in `.github/workflows/` (`sign-*` and `promote-*`), as GitHub requires. This directory holds what they use:

- `docs/` - how release signing and verification work.
- `keys/` - the public signing key that releases are checked against.
- `scripts/` - the helpers the workflows call to verify and sign artifacts.
- `website/` - the website, [www.holder.team](https://www.holder.team/): downloads, guides and the desktop update feed. See its README.

## Windows development builds

The `Release / Promote Windows dev build` workflow can publish a self-signed Windows tester installer from a successful Windows staging run of this repository.

This is for prerelease testing only. It does not replace the final Windows signing path.

## Linux AppImage release candidates

The `Release / Sign Linux AppImage` workflow verifies and signs one exact
successful AppImage staging run of this repository. It emits a tested,
OpenPGP-signed candidate with checksums, provenance, and a GitHub artifact
attestation; it does not publish a GitHub release.

The protected-environment setup and verification process are documented in
[`docs/linux-appimage-signing.md`](docs/linux-appimage-signing.md).

After signing, `Release / Promote Linux AppImage` verifies one exact signing
run and adds the AppImage, checksum manifest, manifest signature, and public
key to the draft for that exact version tag. An existing published prerelease
can also receive the assets when `allow_published_prerelease` is explicitly
enabled. Existing release titles and descriptions are never edited by the workflow.

## Core SDKs

`Release / Promote core SDK` adds the core SDK archives to the draft for the
exact version tag, creating the draft if no other promotion has. Give it the
successful Daemon CI run, dispatched with `core_build_type=Release`, that the
release's backends were staged from: it downloads that run's Release SDKs for
Linux, macOS and Windows, checks them against the run's `core-selection` index
and the release version, and uploads them with the index as
`core-sdk-index.json`. The SDKs on the release are therefore the core the
shipped daemon was built against. Nothing else publishes core SDKs.

## Holder Kit on PyPI

`Release / Kit to PyPI` builds Holder Kit's wheels and source archive against
Release core SDKs from `core/` in the same commit, checks them, and publishes them
to TestPyPI or PyPI through Trusted Publishing (the `testpypi` and `pypi`
environments). It publishes only from the `holder-v<version>` tag, with Kit,
core and `VERSION` all at that version. See
[`python/kit/docs/packaging.md`](../python/kit/docs/packaging.md).

## Ubuntu packages

`Release / Launchpad` uploads the `holder-core`, `holder-daemon`, `holder-kit` and
`holder-desktop` source packages to `ppa:holderteam/holder` for each `holder-v*` tag.
There is no separate desktop tag any more. See the comments
at the top of `.github/workflows/release-launchpad.yml`.
