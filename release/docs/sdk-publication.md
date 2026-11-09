# Publish the candidate's core SDKs

After candidate preparation, essential checks and the maintainer's publication
decision, create/verify the Holder release tag through the
[runbook](runbook.md). It must already identify the accepted candidate. Pushing
that Holder tag starts the existing Launchpad upload.

```sh
python3 release/scripts/release.py publish-sdk \
  --record release/records/0.2.6-candidate-1.json --wait
```

The command uses the selected Daemon CI run's three Release SDK archives and
`core-selection`. It does not rebuild them or choose a newer SDK. For a new
publication, the coordinator saves their immutable artifact IDs/digests, run
attempt, source revision, selection and per-archive producer identity in a
metadata commit on `release/sdk/0.2.6-candidate-1`, under
`release/sdk-manifests/`. It dispatches the existing SDK promotion workflow on
`main` with that immutable metadata reference. The existing SDK publishing
environment and publisher remain in use.

The workflow checks the source tag and root VERSION, run/source/attempt identity,
artifact pins and ZIP hashes. It verifies each SDK's size/hash and re-indexes the
downloaded archives using the production SDK manifest reader. There must be
exactly Linux x86_64, Windows x86_64 and macOS arm64 Release SDKs. Cached producer
commits may precede the Holder candidate; their core trees and root version
inputs must match it. No downloaded SDK code is executed during publication.

## Destination and identity

This operation publishes `holder-v<version>` in **HolderTeam/holder-sdk**, with
the three original archives and `sdk-index.json`. It is a public SDK release,
not an addition to Holder's application-release draft. The SDK repository's own
tag is created by that repository's normal release publication and identifies
its metadata repository revision; it is not the Holder candidate commit.

Keep the three identities distinct:

- Holder candidate and Daemon CI selection commit.
- Per-platform SDK producer commits recorded inside the archives.
- The release tag target in the SDK repository.

The index retains schema 1 and the existing source/run URLs, core tree, version,
archive sizes/hashes, producer commits and download URLs. A matching existing
index is checked as JSON and retained with its actual byte hash; formatting
differences alone do not cause it to be replaced. Existing archives require
matching sizes and SHA256 values. Missing GitHub asset digests are checked by
downloading the existing asset by ID.

An incomplete draft is resumed by uploading only missing files. Conflicting
archives/indexes, unexpected assets, duplicates and incorrect publication state
stop the operation. Drafts are not deleted and existing assets are not clobbered.
The Holder source tag is checked before each write. Only after all four files
are verified does the workflow publish the SDK draft, retaining the existing
stable/prerelease and latest-release policy. A complete published SDK release is
verified without mutation; an incomplete or conflicting published release is
not silently repaired.

The workflow retains `core-sdk-publication`, containing the input evidence and
`sdk-publication.json`. Independently, the coordinator verifies the public
release/index/archives and saves `core_sdk_published`, including exact release
and asset IDs, file hashes, index content, source input hash and SDK tag target.
The summary lists each archive's producer commit and checksum.

## Resume and recover

Repeat the same command and record after interruption. An uncertain dispatch
reconciles through its saved request identifier without automatically starting
a second publishing run. For a failed workflow with an incomplete destination,
correct the cause and retry its failed jobs:

```sh
python3 release/scripts/release.py publish-sdk \
  --record release/records/0.2.6-candidate-1.json --retry core_sdk_publication --wait
```

Successful uploads remain in place. If publication succeeded before a later
response, receipt or cleanup failure, re-run the command without `--retry`: its
independent destination check can accept completion while retaining the failed
workflow result. It can also recognize an existing matching manual publication
without dispatching a workflow or committing new input metadata.

Original source artifacts must remain available for a new dispatch or retry.
Their recorded expiry is checked. If a partial draft outlives those inputs,
recover the exact retained archives/input evidence through the manual procedure;
do not rebuild and describe replacement bytes as the validated candidate. A
changed candidate may need a new version rather than rewriting the existing
Holder tag. Complete published destinations remain verifiable after source and
receipt artifacts expire. Accepted destination IDs, SDK tag target and file
hashes cannot silently change.

The manual workflow's existing version/run/tag inputs remain available and now
use the same verification/resume behavior. Supply both `prepared_sdk_path` and
full immutable `prepared_sdk_ref` to use coordinator pins; leave both blank to
capture the explicitly selected run's current artifacts. All paths require the
Holder release tag at that Daemon run's source SHA.

Development snapshot publication, including its Ubuntu SDK-package payload,
continues through the existing Core CI publisher. Windows delivery, the remaining
platform handoff, Launchpad binary availability and website completion remain
subsequent #120 work. Validate this complete remote handoff through the next
actual release; an extra SDK build matrix or publication rehearsal is not
required.
