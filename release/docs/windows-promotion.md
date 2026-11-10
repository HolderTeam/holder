# Deliver the recorded Windows development installer

After the essential platform checks and the maintainer's publication decision,
create/verify the normal candidate tag and shared Holder draft through the
[runbook](runbook.md). Then use the same candidate record:

```sh
python3 release/scripts/release.py promote-windows \
  --record release/records/0.2.6-candidate-1.json --wait
```

The candidate must have Windows tester signing enabled during preparation. The
existing Windows staging environment review, signing and installation smoke
checks remain the prerequisites. After those checks succeed, staging writes
`release-evidence.json` alongside the installer and public certificate files.
It records the final file hashes, staging run and workflow source, component
sources and selected core SDK provenance. It contains no private signing material.
Older staging artifacts without this evidence cannot use this delivery path;
select a new candidate rather than fabricating evidence for old bytes.

The coordinator checks this evidence against the accepted framework manifest
and SDK selection, then saves `windows_delivery_input`, its checksum, and the
exact staging artifact ID/archive checksum. It dispatches
`promote-windows-dev-build.yml` on `main` with those pins, source SHA, version and
existing `holder-v<version>` tag. It creates neither the tag nor the shared draft,
and does not publish the shared release. Windows remains a development build;
this does not establish production Windows signing.

The workflow verifies the selected archive and final file hashes without
executing the installer, then delivers exactly four development assets:

- `Holder-windows-<version>-dev-Setup.exe`
- `Holder-windows-<version>-dev-Setup.exe.sha256`
- `Holder-windows-dev-test-signing.cer`
- `Holder-windows-dev-test-signing-fingerprint.txt`

The certificate's SHA-256 must match its public fingerprint file. The installer
checksum names the renamed development asset. The workflow preserves release
notes and other platform assets. Matching destination bytes are reused; only
missing files are uploaded. Conflicts stop delivery before writes. Release/tag
state is checked before each write, and published product releases cannot be
modified by this workflow.

The workflow retains `windows-delivery-receipt`. Independently, the coordinator
checks destination IDs, sizes and hashes and saves `windows_promoted` with a
receipt checksum and source relationship. For assets without a GitHub digest,
it downloads and hashes the bytes. A complete destination can be accepted even
if later workflow cleanup failed or the source artifact has since expired.
Accepted release/asset IDs and hashes cannot silently change. A draft becoming
published is recorded as a normal state transition.

## Recovery and manual delivery

Resume with the same command and record. After a failed run with missing assets,
correct the cause and retry its failed jobs:

```sh
python3 release/scripts/release.py promote-windows \
  --record release/records/0.2.6-candidate-1.json --retry windows_promotion --wait
```

A lost dispatch response is reconciled using the persisted request identity;
resuming does not dispatch another run. Once staging is pinned for delivery,
retrying it to replace the accepted artifact is blocked. Missing destination
files require the original staging archive to remain available for dispatch or
retry. If it expires, recover the exact bytes or select a new candidate; do not
rebuild and describe different bytes as the accepted installer.

The independent manual workflow remains available for new staging artifacts
with evidence. With no `release_tag`, it uses `holder-windows-dev-<staging_run_id>`,
creates a draft, and publishes that development prerelease only after all four
files are verified. An explicit product tag must already exist; the manual
entry point may create a missing product draft, which remains unpublished.

Manual `replace_draft_assets=true` is an explicit recovery operation for
conflicting draft files. It cannot replace published files and cannot accompany
`candidate_commit`; the coordinator always disables it. Review intentional
replacement separately from retrying an interrupted upload.

Validate the complete remote path through the next actual release using this
procedure. No additional publication rehearsal is needed. The remaining platform
handoff, Launchpad binary availability and website completion remain later #120
work.
