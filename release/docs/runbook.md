# Holder release runbook

This is the current procedure for preparing, checking and publishing a Holder
release from this monorepo. Candidate preparation has a single resumable entry
point; publication handoffs still use the individual workflows below. Run commands from the repository
root; workflow names and inputs below refer to `HolderTeam/holder`.

Last source check: 2026-11-09. Workflow inputs and manifest preparation have been
checked against the repository. Use the next release using this procedure to validate the
complete sequence and record corrections. A separate full rehearsal, TestPyPI
upload or deliberately failed release is not a prerequisite. Until that release,
this is a source-checked procedure, not a claim of end-to-end execution.

The maintainer makes the release decision. Preparation does not authorize a
production upload. Signing and notarization are treated here as artifact
handoffs and verification gates; account setup and signing operations are outside
this document.

## 1. Start a release record

Keep one record for the candidate. Record public source/artifact facts and
validation results, without credentials or account configuration:

| Record | Required value |
| --- | --- |
| Candidate | Product version, candidate number, full source commit SHA and frozen build branch |
| Build policy | Workflow revision for each dispatch, all non-default inputs, run ID and run attempt |
| Components | Daemon/Desktop run IDs, their source SHAs, core tree and `core-selection.json` |
| Staging | Framework manifest path and digest; Linux, Windows and macOS staging run IDs |
| Artifacts | Names, IDs, expiry dates, sizes, SHA-256 digests and retained provenance |
| Validation | Platform/upgrade matrix, results, diagnostics, known limitations and go/no-go decision |
| Publication | Exact tag/commit, destination, publication run ID, resulting URL/version and verification result |

Update the record after each completed step. A failed attempt stays in the
record; a newer candidate must not silently replace its pins or validation.
Preserve downloaded artifacts and their manifests before Actions retention
expires. Component and staging artifacts currently last seven days; the Linux
signed-candidate artifact lasts 30 days. Check each artifact's actual expiry.

Decide the distribution scope up front: Linux AppImage, macOS disk image,
Windows installer, core SDKs, Python packages and Ubuntu packages. Android is
not published by this procedure; its release channel is separate. If a platform
is omitted or distributed as a development build, say so in the release record,
release notes and website instead of implying a complete stable platform set.

## 2. Prepare and freeze the source

Merge the intended product changes and version update through normal review.
The version setter updates only root `VERSION`. Native builds, Android and both Python
packages read it directly in a checkout. Python source distributions include
generated version/build metadata; other exported source trees receive it during
source preparation:

```sh
python3 release/scripts/version.py <next-version>
python3 release/scripts/version.py --check
```

The setter currently accepts numeric `major.minor.patch` release versions.
Use candidate numbers and branches while preparing a release; no separate RC
version scheme is required. Android derives `versionCode` as
`major * 1000000 + minor * 1000 + patch` (major 0–2099, minor/patch 0–999,
positive result); debug builds append `-debug` to the displayed version.
Every published Android release needs a greater product version/code; rebuilding
the same product version retains its code. See [#130](https://github.com/HolderTeam/holder/issues/130)
for the full artifact version audit.

Product version and HTTP compatibility are different contracts. Review
`api/API_VERSION` and `apps/desktop/gtk/compatibility/daemon-api.json` separately;
do not change the API version just to match a product release.

Choose the reviewed full source SHA. The coordinator creates a frozen remote
build branch at that commit, for example `rc/<version>-candidate-1`; create it
yourself when using the manual dispatch procedure. Leave it unchanged while
building the candidate. Dispatch accepts a branch or tag name, so record the
resolved SHA of every run, not merely its branch. Do not create a `holder-v*` tag
for an optional test: pushing that tag starts production Launchpad uploads, even if
the corresponding GitHub release is a draft or prerelease.

Confirm the applicable CI results on the selected source, including core/API
conformance, package and desktop checks. A skipped component result on an
unrelated PR is not evidence of a release build. Release-configured builds in
the next step are additional to ordinary PR checks.

## 3. Build the release components

For coordinated preparation through all three staging outputs, use:

```sh
python3 release/scripts/release.py prepare \
  --record release/records/<version>-candidate-1.json \
  --candidate <full-reviewed-SHA> --version <version> --number 1 --wait
```

This creates the frozen build branch, starts the Release builds below, generates
and commits verified manifest/evidence on a candidate metadata branch, and stages
all three platforms on `main` using that immutable manifest commit. It saves
progress and a reviewable summary beside the record. Resume with the same record;
see [candidate preparation](candidate-preparation.md) for inputs, targeted retries,
uncertain requests, expiry and recovery points. After `candidate_staged`, continue
with section 5's validation and the publication decision. The manual steps in
sections 3–4 remain available independently.

Dispatch both workflows on the frozen build branch:

```sh
gh workflow run daemon-ci.yml --repo HolderTeam/holder \
  --ref <candidate-branch> -f core_build_type=Release -f core_packages=source
gh workflow run desktop-ci.yml --repo HolderTeam/holder \
  --ref <candidate-branch> -f build_type=release
```

Wait for both complete runs to succeed and confirm their `headSha` equals the
selected candidate SHA. Record their run IDs and attempts; do not select runs
only by newest timestamp. The expected inputs and outputs are:

| Workflow | Inputs | Artifacts consumed by staging/promotion |
| --- | --- | --- |
| [Daemon / CI](../../.github/workflows/daemon-ci.yml) | `core_build_type=Release`, `core_packages=source` | `core-selection`, `core-sdk-<platform>-Release`, `holder-daemon-linux-x86_64-release`, `holder-daemon-macos-backend-release`, `holder-daemon-windows-backend-release` |
| [Desktop / CI](../../.github/workflows/desktop-ci.yml) | `build_type=release` | `holder-desktop-linux-x86_64-release`, `holder-desktop-macos-release`, `holder-desktop-windows-release` |

The daemon builds against verified installed core SDKs. Each SDK can have been
produced at an earlier commit with the same valid core tree; retain both the
workflow selection and each SDK's producing revision and digest. Do not replace
the SDK relationship with an independently chosen latest SDK.

Daemon CI's `Release` mode skips its Ubuntu-package job. These backend archives
are not proof of a tested PPA installation or upgrade. Desktop CI's package-backed
Behave tests likewise do not replace the release package validation below.

## 4. Pin and stage the candidate

Generate the framework manifest from the successful runs and frozen candidate SHA:

```sh
python3 release/staging/scripts/generate-release-manifest.py \
  --candidate <full-candidate-SHA> --version <version> \
  --desktop-run <Desktop-CI-run-ID> --backend-run <Daemon-CI-run-ID> \
  --output release/staging/release-manifests/<version>-candidate-1.json
```

Review the manifest and companion `.evidence.json` using the
[manifest contract](../staging/docs/framework-release-manifest.md). Generation
checks the Release staging artifacts and core provenance and records:

- The product `version` and `core.build_type=Release`.
- `core.commit` from that daemon run's selection.
- The recorded Desktop and Daemon repository, run ID and source SHA for each of
  `linux`, `windows` and `macos`.
- Immutable artifact IDs, archive digests, run attempts and the earliest staging
  input expiry; SDK producer commits/digests remain separate from candidate identity.

Commit both files under `release/staging/release-manifests/` and
merge it to `main` before staging. The manifest commit can differ from the
candidate source commit: it records already-built inputs, rather than changing
them. Record the staging workflow revision as well as those component revisions.
The coordinator generates and retains this metadata automatically; these are
the independent manual steps. #121 retains the next actual release's staging
validation evidence.

Dispatch the three staging workflows on `main`, using the same manifest input:

```sh
gh workflow run staging-linux-appimage-stage.yml --repo HolderTeam/holder \
  --ref main -f release_manifest=release-manifests/<version>-candidate-1.json
gh workflow run staging-macos-stage.yml --repo HolderTeam/holder \
  --ref main -f release_manifest=release-manifests/<version>-candidate-1.json
gh workflow run staging-windows-stage.yml --repo HolderTeam/holder \
  --ref main -f release_manifest=release-manifests/<version>-candidate-1.json \
  -f sign_test_installer=false
```

The input path is relative to `release/staging/`. Leave component-run overrides
blank, keep `backend_repository=HolderTeam/holder`, and leave the Linux
`appimage_version` override blank. The manifest controls those values; conflicting
run/version overrides are rejected. Keep release configuration pins: development
defaults do not produce a production candidate.

The Windows example assembles and smoke-tests the unsigned installer without the
tester-signing environment. If the chosen validation/distribution route needs
the self-signed tester installer, explicitly dispatch with
`sign_test_installer=true` on an allowed `main` revision instead and record that
run. It is a development installer, not a substitute for a production signing
decision. See [staging](../staging/README.md) for platform-specific inspection.

| Staging workflow | Result |
| --- | --- |
| [Linux AppImage](../../.github/workflows/staging-linux-appimage-stage.yml) | `Holder-linux-appimage-staged`: AppImage, AppDir archive, checksums and provenance |
| [macOS](../../.github/workflows/staging-macos-stage.yml) | `Holder-macos-staged`: `Holder.app`, preview disk image and checksums |
| [Windows](../../.github/workflows/staging-windows-stage.yml) | `Holder-windows-staged`; additionally `Holder-windows-test-signed-installer` when requested |

The macOS staging output is a test candidate. Pass the exact staged bundle through
the signing/notarization step when preparing a distributable disk image. Record
the resulting `Holder-macos-<version>.dmg`, checksum and verification outcome;
use the [macOS delivery commands](macos-delivery.md) to retain the handoff input,
final verification and destination evidence.

## 5. Run the essential checks

Use the successful component builds, staging checks, checksums and provenance
already produced for this release. Reuse recent platform/upgrade evidence where
it still applies, and repeat manual checks affected by the changes. Do not run
another complete build matrix just to call it a rehearsal. The normal Python
publication workflows build and validate distributions before uploading them.
The coordinator can now prepare both Python sets once and retain their verified
identities alongside the staged candidate:

```sh
python3 release/scripts/release.py packages \
  --record release/records/0.2.6-candidate-1.json --wait
```

Kit reuses the candidate's exact Release SDK archives. The record retains both
packages' artifact IDs, archive and file hashes, source/SDK identity and expiry;
the summary lists the files. See [Python preparation](python-preparation.md) for
resume and targeted retry commands and [Python publication](python-publication.md)
for the upload handoff. Manual publishing dispatches rebuild unless given the
prepared-release inputs.
Standalone build-only inputs remain available when needed:

| Workflow | Build-only inputs | Outputs |
| --- | --- | --- |
| [Kit to PyPI](../../.github/workflows/release-kit-pypi.yml) | `destination=none`; optional generated `release_manifest` and immutable `release_manifest_ref` for pinned SDK reuse | `kit-release-distributions`, `kit-release-manifest` (18 platform/interpreter wheels plus sdist) |
| [Holder Python to PyPI](../../.github/workflows/release-holder-python-pypi.yml) | `destination=none`, `preview_number=0` | `holder-python-distributions`, `holder-python-release-manifest` (wheel and sdist) |

Retain distribution manifests/digests from whichever runs are used. For Ubuntu
packaging changes, reuse `Release / Launchpad`'s pull-request checks: they build
unsigned source packages without uploading. Its manual dispatch uploads to the
PPA and has no build-only switch.

Only use TestPyPI if a specific publishing change needs it. It is an actual
upload to a separate index, and the workflows currently differ:

- Holder Python accepts `destination=testpypi`, a fresh positive `preview_number`
  and `ref=main`; this uploads `<version>.post<number>`. That ref must still name
  the intended source. If `main` has advanced, a preview is evidence about that
  newer revision, not the frozen candidate.
- Kit requires the matching `holder-v<version>` tag for both TestPyPI and PyPI;
  there is no corresponding preview-number input. Do not create a test tag
  in that namespace just to reach TestPyPI, because of the Launchpad trigger.

Choose the relevant checks below and record results on disposable profiles or
machines. The next release using this procedure supplies end-to-end publication evidence;
do not invent a parallel release ceremony to obtain it:

- Linux: normal desktop AppImage launch, selected backend, close/reopen/idle
  behaviour, and coexistence with an installed daemon. Track PPA install/upgrade
  evidence separately in [#132](https://github.com/HolderTeam/holder/issues/132).
- Windows: interactive install, backend resolution, PATH options, shortcuts and
  supported upgrade path ([#131](https://github.com/HolderTeam/holder/issues/131)).
- macOS: app launch, bundle verification, menus, dialogs, editor text/caret and
  VoiceOver behaviour on the exact staged/final artifact.
- Data/API upgrades: selected available released baselines, plain/encrypted
  projects, sync/recovery state and an older running daemon
  ([#196](https://github.com/HolderTeam/holder/issues/196)). Record rollback limits
  and back up test profiles before migration.

Actual PPA availability and final signed-package checks occur only after their
respective artifacts exist. Keep them pending in the record rather than treating
an unsigned/local build as equivalent evidence. A source defect means a new
candidate with new pins and repeated affected checks, not an edited old record.

## 6. Review the publication decision and approvals

Request the Linux signed candidate once its staging output is ready:

```sh
python3 release/scripts/release.py sign-linux \
  --record release/records/0.2.6-candidate-1.json --wait
```

This pins the exact staged artifact and waits at the existing Linux reviewer
gate. Review its verification summary before approving. It records signed
outputs without publishing or creating a tag, so it can run before the tag
boundary below. See [Linux signing](linux-signing.md) for retained evidence,
approval states, targeted retries and expiry recovery.

Before making a production tag or upload, review this go/no-go list:

- [ ] Version and candidate SHA are fixed; component/staging runs and core
  provenance match; required artifacts are retained and unexpired.
- [ ] The chosen platform and upgrade checks passed, or omissions/limitations
  have an explicit maintainer decision and accurate user-facing notes.
- [ ] Distribution scope and signing/notarization outcomes are recorded. A
  development installer is labelled as such.
- [ ] Existing versions/tags/assets at every destination have been inspected.
  An already-published version will not be overwritten or silently repurposed.
- [ ] Release notes describe supported platforms, changes, upgrade instructions,
  backup recommendations and rollback/compatibility limitations.
- [ ] The maintainer authorizes the production tag/package uploads. Steps still
  waiting on remote builds or package verification remain visible.

Environment permissions and the release decision are separate. At the source
check date, the public Linux signing workflow has a reviewer gate on
`linux-release-signing`; inspect its verification summary before approving it.
`windows-staging`, `holder-sdk-publish` and `github-pages` have branch restrictions.
`pypi` and `testpypi` have no configured reviewer gate. Recheck live settings;
do not infer an approval requirement just from an environment's name. This
runbook grants no approval and describes no account configuration.

## 7. Tag and assemble the public release

Once authorized, create `holder-v<version>` on the exact candidate source SHA.
If a draft already names this tag, check its target before publication; do not
let tag creation select the then-current tip of `main` instead. Never move a
published tag. Pushing the tag starts `Release / Launchpad`; GitHub draft state
does not postpone that upload. Record the resulting run immediately.

Linux signing/promotion, Windows development delivery, the macOS handoff and
core SDK publication use the candidate record. The existing workflows also
provide independent entry points:

| Operation | Public workflow/ref and inputs | Handoff/result |
| --- | --- | --- |
| Linux signing step | `release.py sign-linux --record <candidate.json> --wait`; or the existing [manual workflow](../../.github/workflows/sign-linux-appimage.yml) on `main` | After approval and verification: accepted `Holder-linux-appimage-signed-<version>`, pinned staging identity, signatures, metadata and attestation |
| Linux promotion | `release.py promote-linux --record <candidate.json> --wait`; the [manual workflow](../../.github/workflows/promote-linux-appimage.yml) remains available on `main` | Verifies pinned signed bytes and the existing candidate tag; creates/reuses the matching draft and delivers only missing Linux files; records exact destination IDs/hashes |
| Core SDK publication | `release.py publish-sdk --record <candidate.json> --wait`; the [manual workflow](../../.github/workflows/promote-core-sdk.yml) remains available on `main` | Publishes the daemon run's pinned three SDKs and compatible `sdk-index.json` in `HolderTeam/holder-sdk`; records destination identities and per-SDK producers |
| Windows development installer, if chosen | `release.py promote-windows --record <candidate.json> --wait`; the [manual workflow](../../.github/workflows/promote-windows-dev-build.yml) remains available on `main` | Verifies the final installer evidence against the candidate, pins its archive, and delivers only missing development assets to the existing shared draft; records destination IDs/hashes |
| macOS disk image | `release.py prepare-macos`, then `accept-macos --macos-directory <final-output>` on a Mac, then `promote-macos --macos-directory <final-output>`, each with `--record <candidate.json>` | Pins staging input, verifies the final DMG and embedded candidate evidence, and delivers the accepted DMG/checksum to the shared draft with recorded destination IDs/hashes |

Create/verify the shared draft before coordinated Windows development delivery.
Windows remains a development build. See [Windows delivery](windows-promotion.md)
for the required staging evidence, pinned inputs and recovery procedure. The
independent manual entry point can create a missing draft at an existing product
tag. Without an explicit tag, it publishes a separate
`holder-windows-dev-<run>` development prerelease after verifying all assets.
Linux promotion leaves matching existing assets intact and rejects conflicts.
It can verify a complete published stable release, but cannot modify one. Keep
its published-prerelease exception disabled for the normal release path.
Intentional Linux draft replacement is now an explicit manual-only input,
`replace_draft_assets=true`, unavailable with pinned inputs. See
[Linux promotion](linux-promotion.md) for receipts, partial upload recovery,
expiry handling and that separate mutable-draft operation.

[Core SDK publication](sdk-publication.md) likewise verifies matching assets and
resumes partial drafts without deletion/replacement. This publishes the SDK
release, separate from Holder's draft. The Holder source tag must already exist;
the SDK repository's own release tag target is a different identity. Preserve
both along with each cached SDK producer and the publication receipt.

Signing can change artifact bytes. Verify and record the final signed outputs
and test those outputs; do not compare them to an unsigned checksum as if they
must be identical. Preserve the candidate relationship and publish the verified
outputs without rebuilding them merely to upload.

Review the draft's full asset set and release notes before changing it to a
published release. Retain development labels where applicable. Confirm the
public tag resolves to the intended candidate SHA and download/check the assets
from the destination. GitHub release publication does not itself finish PyPI,
Launchpad binary builds or website updates.

## 8. Complete package publication

| Destination | Current operation | Completion evidence |
| --- | --- | --- |
| Ubuntu PPA | `Release / Launchpad` uploads on the pushed `holder-v*` tag; `release.py observe-ppa --record <candidate.json> --wait` checks the exact packages for noble and resolute | Recorded source publications, remote builds and APT index entries for the selected architecture coverage; separately verify installs and supported upgrades |
| Kit | `release.py publish-python --record <candidate.json> --package kit --destination pypi --wait` | Prepared files fetched/verified without rebuilding, expected index hashes and publication receipt |
| Holder Python | `release.py publish-python --record <candidate.json> --package python --destination pypi --wait` | Prepared wheel/sdist on the index with matching hashes and publication receipt |

The matching release tag must already point to the prepared source. The command
does not create it. Each package/destination is separately resumable and recorded;
matching existing files are reused and conflicting bytes stop publication. See
[Python publication](python-publication.md) for inputs, receipts, partial upload
recovery and expiry handling. Existing manual package dispatches retain their
build-and-publish behavior when prepared-release inputs are omitted; with both
`prepared_release_path` and immutable `prepared_release_ref`, they publish the
recorded files without rebuilding. Retain the source/core provenance, file hashes
and actual index/installation evidence for whichever path is used.

TestPyPI and PyPI file/version identities cannot be reused for changed uploads.
If using TestPyPI after the release tag exists, apply each workflow's documented
rules and inspect index state before retrying. The old holder-python 0.2.5
package-only backfill exception is not a general release route. See the
[Python publishing](../../python/holder/docs/publishing.md) and
[Kit packaging](../../python/kit/docs/packaging.md) contracts.

Launchpad accepts source uploads before all dependent builds are ready. Use
[the PPA observer](launchpad-availability.md) to record each exact source/series
version, remote builds and APT index entries. Wait for matching core development
packages and then daemon/Kit packages. Record installed test results separately.
A packaging-only repair can be dispatched on the intended source ref
with `packages=<affected package>`, `series=<affected series>`,
`version=<expected product version>` and a fresh positive `package_revision`.
That manual dispatch uploads to the PPA; it is not a build-only switch. Core/Kit
orig tarballs are shared across series/revisions for an upstream version:
packaging revision changes do not permit replacing different upstream source.
Select the repaired version with `observe-ppa --ppa-revision <package>:<series>=<revision>`;
the prior observation remains in history and other targets are unchanged.

## 9. Update the website and installed-app feed

Publishing a stable `holder-v<version>` release starts a small dispatcher in
`release-website.yml`, which starts that workflow on `main` to satisfy the Pages
environment's branch rule. The main run checks out website/tooling source and runs
`release/scripts/website-release.py` before building. The highest published
numeric Holder version supplies the compatible schema-1 `/version.json` feed,
download catalog and checksum copies. Root development VERSION is not the public
release selector.

Generation requires the complete, unique supported platform asset set and verifies
checksums and the Linux checksum signature. See the [website data contract](../website/README.md#published-release-data).
Incomplete releases stop deployment and leave the previous site/feed in place;
there is no fallback to an older release to hide an incomplete newer one. Correct
the release assets/metadata, then manually dispatch `release-website.yml` on `main`
to retry. Do not edit generated feed or download files.

PPA availability is separate: after verifying the intended packages, update
[`website/_data/ubuntu.json`](../website/_data/ubuntu.json) in a reviewed PR.
Use an explicitly declared version or mark availability unconfirmed. GitHub
publication never advances this record automatically. Update platform/upgrade
guidance when behaviour changes; release-specific version labels come from the
generated catalog.

After publication, observe the deployed website from the candidate record.

```sh
python3 release/scripts/release.py observe-website --record <candidate.json> --wait
```

The command verifies the candidate tag and the stable release selected by the
website generator. It checks the live feed, checksum copies and Linux, macOS and
Windows download links. It records the release and asset IDs, feed, checksum
hashes and observation time in `website_verified`. Omit `--wait` for a single
check. An older feed or missing public file means deployment is still pending.
Inspect the existing Pages workflow if the site does not catch up, then resume
the observer after recovery. Review the live Ubuntu instructions separately
against the reviewed PPA declaration.

## 10. Recover from a failed or partial release

| Stopping point | Resume rule |
| --- | --- |
| Build or staging fails | Keep the failed run and diagnostics. An infrastructure retry may use the same source, but record the new run/attempt and artifact IDs. Source fixes require a new candidate and affected checks; do not silently refresh pins. |
| Artifacts expire | Check the recorded handoff and retained final files. Completed handoffs can remain verifiable from their accepted evidence. Missing unrecorded inputs require a new validated artifact set; follow the platform recovery procedure and preserve the original pins. |
| Approval or signing step is pending | Wait for its recorded outcome. Resume from the retained exact input; do not rebuild or start another signing attempt simply because an external operation takes time. |
| SDK publication is interrupted | The helper can recreate an unfinished draft on retry. An existing published SDK version is left untouched; its existence alone is not proof that it matches this candidate. Compare the published index/digests before accepting it as complete. |
| Draft asset upload is interrupted | Inventory current assets and hashes first. Linux/Windows/macOS promotion reuses matching bytes and uploads only missing files. Conflicts stop the normal path. For conflicting draft assets, follow the platform's documented recovery procedure and record/recheck any intentional replacement. |
| PPA upload/build is incomplete | Inspect the recorded package/series state and build logs. Repair only the affected target where needed, preserving upstream tarball identity, then resume `observe-ppa` with any explicitly revised target. |
| PyPI/TestPyPI is partially uploaded | Inspect the index and local manifest before retrying. Existing files cannot be changed in place; do not assume the workflow can safely resume a partial upload. A new package/release version may be necessary. |
| GitHub release is already public but another destination failed | Record partial completion and address the failed destination explicitly. Do not move tags or erase successful publications to simulate an atomic release. Keep website/channel claims accurate. |
| Website deployment fails | Keep the verified release files and metadata revision, fix the site/deployment issue and redeploy that reviewed data. Do not advertise unverified package availability as a workaround. |

Do not solve artifact mismatches by disabling provenance checks. Close the
release record only after every chosen destination and the website/feed are
verified, or after an explicit decision documents an incomplete/withdrawn release.

## Keeping this runbook current

Update this document in the same PR that changes an operational input, artifact,
gate or retry rule. During the next release using this procedure, record which steps worked,
correct the procedure where needed, and retain evidence for any recovery that
was necessary. Do not manufacture failures or require a separate rehearsal to
finish the documentation work. The [roadmap](https://github.com/HolderTeam/holder/issues/122)
tracks manifest generation (#121), version derivation (#130), website generation
(#110) and a future single entry point (#120). Replace manual steps when their
automation is proven; monthly scheduling (#192) remains a separate decision.
