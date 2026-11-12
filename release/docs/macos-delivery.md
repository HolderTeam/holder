# Record and deliver the final macOS disk image

Use the existing candidate record to retain the staging input, final verification
and delivery results. Begin after the selected macOS staging run succeeds.

```sh
python3 release/scripts/release.py prepare-macos \
  --record release/records/0.2.6-candidate-1.json --wait
```

This checks the staged component and SDK provenance against the candidate
manifest. It saves `macos_handoff`, its checksum, the staging run/attempt,
workflow source, artifact ID and archive SHA-256. The summary identifies the
artifact to pass through the existing macOS distribution handoff. Preparation
and final verification can happen before the release tag is created.

Complete that handoff using the selected staged app. Place the resulting
`Holder-macos-<version>.dmg` and `Holder-macos-<version>.dmg.sha256` in a local
directory. The checksum must name the versioned DMG by basename. On a Mac, run

```sh
python3 release/scripts/release.py accept-macos \
  --record release/records/0.2.6-candidate-1.json \
  --macos-directory /path/to/final-output
```

Acceptance pins the staging input automatically if `prepare-macos` was omitted.
It hashes the final files, verifies the disk image, mounts it read-only, and
checks the embedded candidate metadata, framework manifest, SDK evidence and app
version. It verifies the app signature with `codesign --verify --deep --strict`
and assesses the app with `spctl --assess --type execute`, then detaches the
image and checks that the final files still have the same hashes. These are
standard macOS verification commands described in Apple's signing guide.
[Code Signing Tasks](https://developer.apple.com/library/archive/documentation/Security/Conceptual/CodeSigningGuide/Procedures/Procedures.html)

The record saves `macos_verified` with the file names, sizes, hashes, staging
relationship, verification time and passed check names. The summary shows the
hashes and outcomes. A repeated acceptance of the same files reuses that
receipt. Continue the essential app launch and platform checks on the final
image as described in the runbook.

## Deliver to the shared draft

After the maintainer's publication decision, create/verify the normal candidate
tag and shared draft through the runbook. Deliver the accepted final files with

```sh
python3 release/scripts/release.py promote-macos \
  --record release/records/0.2.6-candidate-1.json \
  --macos-directory /path/to/final-output
```

The command verifies the tag's candidate source and local file hashes, compares
the two destination assets, and uploads only missing files to the matching
draft. It preserves matching assets, release notes and the other platform files.
Conflicting bytes stop delivery. The tag and draft state are rechecked before
each upload. The release record retains `macos_promoted` with destination
release/asset IDs, file hashes and the verification receipt relationship.

Copying the record and the two accepted files to another machine lets the
operator continue delivery there. Successful verification receipts contain file
hashes and check outcomes. Protect and retain the record
with the final files until the release is complete.

## Recovery

Repeat the same command and record after an interruption. A partial upload or
lost response reuses matching destination bytes. If both files arrived before a
later error, verify completion by running `promote-macos` with just `--record`.
That also rechecks a complete published release and refreshes its observed state.
Accepted destination IDs and file hashes must continue to match the receipt.

The original staging artifact must be available when the handoff is first
pinned. After that, the retained evidence and accepted final files support
completion even if the staging archive expires. An expired unrecorded staging
input requires recovery of the original evidence or a new candidate. Retrying
macOS staging is blocked once its artifact is pinned for delivery.

A failed verification leaves the accepted-output receipt pending. Correct the
handoff result and rerun acceptance. Once final bytes are accepted, changed bytes
require a new candidate record. If detach fails, inspect `hdiutil info` and detach
the remaining temporary mount before resuming. The helper removes only empty
mount directories during cleanup.

Validate the native verification and complete remote delivery through the next
actual release using this procedure.
