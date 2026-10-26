# Linux AppImage signing

`Release / Sign Linux AppImage` turns one exact, successful
`HolderTeam/holder` AppImage artifact into a signed and tested release
candidate. It deliberately does not create or update a GitHub release.

## Trust boundary

Holder's source, component builds, staging, signing, and promotion all live in
this monorepo. A source change can also change the workflows, verification
scripts, or public-key policy.

Both release workflows are manually dispatched. The signing job uses the fixed
`linux-release-signing` environment and has `actions: read`, `contents: read`,
`attestations: write`, and `id-token: write`. It can sign and attest a release
candidate but has no repository contents write permission to publish a release.
The separate promotion job has `actions: read` and `contents: write`; it verifies
the signed candidate before uploading release assets and does not use a GitHub
environment approval gate.

The GitHub configuration checked on 26 October 2026 restricts the signing
environment to the `main` branch and requires approval from `zeth` before its
environment secrets become available to the job. Self-review is allowed, and
administrators may bypass environment protection. This is an approval gate for
access to signing material, not a requirement for independent review of changes
to signing policy.

[`.github/CODEOWNERS`](../../.github/CODEOWNERS) assigns the release directory,
signing and promotion workflows, and CODEOWNERS itself to `zeth`. The active
`main` ruleset requires pull requests and status checks, but requires neither
code-owner review nor approving reviews. CODEOWNERS identifies ownership;
enforcing owner approval would require a future ruleset change.

The workflow:

1. Downloads an explicitly selected staging run and checks its SHA-256 manifest.
2. Checks the archive layout and requires the external provenance to exactly
   match the provenance embedded in the AppDir.
3. Repeats those checks inside the protected signing job.
4. Imports one passphrase-protected OpenPGP secret key into a temporary keyring.
5. Rebuilds and signs the AppImage with a reviewed, checksum-pinned AppImage
   runtime.
6. Signs the final SHA-256 manifest, exports the public key, and destroys the
   temporary secret keyring.
7. Verifies the manifest signature and smoke-tests the signed AppImage only
   after the private key is gone.
8. Creates a GitHub artifact attestation and uploads a 30-day release-candidate
   artifact.

Official GitHub actions, appimagetool 1.9.1, and AppImage runtime 20251108 are
pinned by digest or commit. Changing those pins is a reviewable release-policy
change.

## Required GitHub configuration

The environment named exactly `linux-release-signing` in
`HolderTeam/holder` must contain:

- Environment secret `HOLDER_GPG_SIGNING_KEY_B64`: a base64 encoding of the
  exported private signing subkeys.
- Environment secret `HOLDER_GPG_SIGNING_PASSPHRASE`: the key passphrase.

The expected public identity is anchored by
`release/keys/holder-linux-release.asc` in this repository. Its primary fingerprint is
`E60EEFC6E1CCB99988DA2E274B0C85A5A86B24A2`; the workflow derives and checks the
fingerprint from that reviewed file rather than trusting a mutable GitHub
variable.

Keep the required reviewer and `main` deployment-branch restriction described
above. Keep the private key and passphrase at environment scope, not as ordinary
repository secrets. Recheck the environment and branch rules when changing the
release process; the configuration snapshot above is not enforced by this document.

Cross-repository artifact downloads may also require the existing repository or
organisation secret `HOLDER_CI_ARTIFACT_TOKEN`. It needs only Actions read access
to `HolderTeam/holder`; it is not a signing secret.

## Running the workflow

Start `Release / Sign Linux AppImage` manually and provide:

- `expected_version`: the exact product version in the staging artifact.
- `staging_run_id`: the exact successful `Staging / Linux AppImage` run.
- The default staging repository and artifact name normally need no changes.

Requiring an exact run is intentional. “Latest green main” is convenient for
staging, but signing should leave an unambiguous approval record.

The uploaded artifact is named
`Holder-linux-appimage-signed-<version>` and contains:

- The signed AppImage.
- The staged component provenance.
- Signing metadata identifying both workflow runs and the key fingerprint.
- The armored public key and a copy of the embedded AppImage signature.
- A one-file SHA-256 manifest for the AppImage and a detached OpenPGP signature
  over that manifest.

## Independent verification

After downloading and extracting the release-candidate artifact:

```sh
gpg --import Holder-linux-release-key.asc
gpg --verify Holder-<version>-SHA256SUMS.asc Holder-<version>-SHA256SUMS
sha256sum -c Holder-<version>-SHA256SUMS
gh attestation verify Holder-<version>-x86_64.AppImage \
  --repo HolderTeam/holder
```

This repository was previously named `holder-release` and then
`holder-framework`, and an attestation records the repository name from the
time it was signed. Releases up to and including 0.2.1 were signed as
`holder-release`, so verify those with `--repo HolderTeam/holder-release`.
Later releases use `--repo HolderTeam/holder`.

The detached manifest signature is the straightforward end-user verification
path. The AppImage also contains an embedded OpenPGP signature for AppImage-aware
tools.

## Promoting to a draft release

Run `Release / Promote Linux AppImage` with the exact version and
successful signing workflow run ID. The workflow re-verifies the signature,
provenance, smoke test, and GitHub artifact attestation before uploading these
release-facing files:

- `Holder-<version>-x86_64.AppImage`
- `Holder-<version>-SHA256SUMS`
- `Holder-<version>-SHA256SUMS.asc`
- `Holder-linux-release-key.asc`

The default tag is `holder-v<version>`, matching the macOS release tag. An override is accepted only when it still
identifies the exact same version. The workflow uses an existing draft for that
tag or creates one with placeholder notes. It refuses to alter a published
release unless `allow_published_prerelease` is explicitly enabled and the
existing release is a prerelease. It never edits an existing release's title or
description.
