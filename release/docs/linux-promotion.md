# Promote the recorded signed Linux files

After [Linux signing](linux-signing.md), essential platform checks and the
maintainer's publication decision, create/verify the normal release tag through
the [runbook](runbook.md). The tag must already identify the accepted candidate.
Pushing that tag starts the existing Launchpad upload; draft state does not
isolate that trigger.

```sh
python3 release/scripts/release.py promote-linux \
  --record release/records/0.2.6-candidate-1.json --wait
```

The command checks the retained signed evidence and the existing
`holder-v<version>` tag, then checks the destination's four Linux assets. Matching
existing bytes are already complete. Otherwise it dispatches the existing Linux
promotion workflow on `main` with the exact signing run, signed artifact ID,
archive checksum, source SHA and release tag. Published-prerelease modification
and draft replacement are disabled for this coordinated path.

Both workflow jobs download the pinned signed bundle and verify its archive,
source and signing-run identity. Existing signature verification, signed-image
smoke testing, artifact attestation verification and non-executing payload
reverification remain in the workflow. No native build or signing is repeated.

Promotion requires the tag to exist; it does not implicitly create one. With no
matching release, it creates a draft at that tag. Existing release title, notes
and other platforms' assets are retained. Only these files are delivered:

- `Holder-<version>-x86_64.AppImage`
- `Holder-<version>-SHA256SUMS`
- `Holder-<version>-SHA256SUMS.asc`
- `Holder-linux-release-key.asc`

The publisher compares existing asset names, upload state, sizes and SHA256
digests with the verified local files. For older assets without a GitHub digest,
it downloads the asset by ID to check its bytes. Conflicts stop the normal path
before any mutation. Only missing assets are uploaded, without `--clobber`.
Release/tag state is checked again before each write. The release and asset
lists are paginated. These checks use the
[GitHub release asset API](https://docs.github.com/en/rest/releases/assets).

The workflow retains `linux-release-promotion`, containing
`linux-promotion.json` with destination IDs/hashes, source input and workflow
identity. Independently, the coordinator verifies the release assets and saves
`linux_promoted`: a receipt, its checksum, exact release/asset IDs, the signed
input relationship and the last observed draft/prerelease state. `linux_promoted`
means the Linux files are delivered; publishing the shared release remains the
maintainer's operation after reviewing its complete platform set and notes.

## Resume and recover

Repeat the same command and record after interruption. Dispatch reconciliation
uses the existing persisted request identifier, and never automatically starts a
duplicate run. A partial upload reuses files whose hashes already match.

For a failed run with missing destination assets, correct the cause and retry
only its failed jobs:

```sh
python3 release/scripts/release.py promote-linux \
  --record release/records/0.2.6-candidate-1.json --retry linux_promotion --wait
```

If all destination files arrived before a later workflow failure, simply repeat
the command without `--retry`. The independent destination check can record
completion without uploading again; the workflow failure remains visible in
its recorded step. The same check can recognize a matching manual promotion or
an already-published release without starting a workflow. Accepted destination
IDs and hashes cannot silently change. A draft becoming published is a normal
state transition and appears in the refreshed summary.

Missing assets require an available signed artifact before a new dispatch or
failed-job retry. If that archive expires, recover the exact retained bytes or
choose a new candidate as appropriate; do not rebuild and claim the new bytes
were the tested signed output. A complete destination remains verifiable after
source/receipt artifacts expire. A published stable release can be checked when
complete, but this workflow cannot add to or replace its assets.

## Independent manual entry point

The existing version/signing-run inputs remain available. For a pinned manual
dispatch, add `signing_artifact_id`, `signing_artifact_sha256` and full
`candidate_commit`. A tag is required on the manual path too.

Draft replacement is now an explicit operation: `replace_draft_assets=true`
allows a manual, unpinned dispatch to replace conflicting Linux draft assets.
It cannot replace any published asset, cannot be combined with pinned inputs,
and is never selected by the coordinator. The separate existing
`allow_published_prerelease=true` option may add missing files to a published
prerelease; it does not permit replacing conflicting files. A conflict may
require a new candidate/version rather than treating a replacement as a retry.

[Core SDK publication](sdk-publication.md) and
[Windows delivery](windows-promotion.md) use the same record. The remaining
platform handoff, Launchpad binary availability and website completion remain subsequent #120
work. Validate this complete remote promotion path through the next actual
release; no additional publication rehearsal is required.
