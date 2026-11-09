# Request and retain the Linux signed candidate

After Linux staging succeeds, request signing using the same candidate record:

```sh
python3 release/scripts/release.py sign-linux \
  --record release/records/0.2.6-candidate-1.json --wait
```

This reads the selected staged artifact without executing it. The coordinator
checks its run/attempt, workflow revision, ownership, expiry, ZIP digest, file
checksums and component/SDK provenance against the accepted framework manifest.
It saves the exact staging artifact ID, archive checksum, provenance and file
hashes before dispatching the existing signing workflow on `main`.

The workflow verifies the pinned download before the `linux-release-signing`
environment gate. Inspect its verification summary, including the artifact ID
and ZIP checksum, and approve through GitHub when appropriate. The coordinator
reports `awaiting_linux_signing_approval` using
[GitHub's pending deployment state](https://docs.github.com/en/rest/actions/workflow-runs#get-pending-deployments-for-a-workflow-run).
It observes that gate; it does not approve or change it. Other pending execution
appears as `signing_linux`.

After approval, the protected job downloads the same artifact ID and checks its
ZIP checksum again. The existing archive/provenance verification, signing,
signature verification, signed-image smoke test and artifact attestation remain
in the workflow. The signing metadata now includes the pinned staging artifact
ID and checksum alongside the existing source run relationship.

When the workflow succeeds, the coordinator reads the signed artifact, checks
its checksums and provenance, and binds its metadata to the original staging
artifact and signing run. `linux_signed` means that workflow succeeded and the
record contains the accepted signed artifact ID, archive checksum, seven output
file hashes, metadata and expiry. The summary lists the signed file hashes and
the signed evidence hash. Signing changes image bytes; preserve both the staged
and signed identities rather than expecting their image hashes to match.

## Resume and recover

Repeat the command with the same record after interruption, or omit `--wait` to
make one observation. `status` reads the saved summary without network calls.
An uncertain dispatch is reconciled by its unique request identifier and never
automatically dispatched a second time.

For a failed signing run, inspect and correct the cause, then retry only its
failed jobs:

```sh
python3 release/scripts/release.py sign-linux \
  --record release/records/0.2.6-candidate-1.json --retry linux_signing --wait
```

Approval waits are not failures. Retrying a pending run is rejected. Failed
attempts remain in history; native builds, staging and successful sibling jobs
are reused. A failed signing attempt can leave unaccepted output; the accepted
output is recorded only after a successful attempt and verification.

Once staging is pinned for signing, it cannot be replaced in that record. Once
signed bytes are accepted, signing cannot be retried to replace them. Changed
source, moved inputs, unexpected attempts or changed accepted artifacts stop the
command and require a new candidate or explicit investigation.

The staged artifact must remain available while signing is pending or retried.
If it expires during an approval wait, preparation stops rather than rebuilding
or substituting another staging run. A successfully finished signing run can
still be captured after the staging artifact expires, as long as its signed
output is available and matches the retained input evidence. Signed artifacts
retain the workflow's 30-day lifetime; re-observation checks their identity and
expiry without downloading accepted bundles again.

This command can run before the release tag exists. It requests no publication
and creates no tag or release. Continue the runbook's platform checks and
publication decision, then [promotion of the accepted signed output](linux-promotion.md).
Core publication and final destination reporting remain subsequent #120 work. Validate
this remote handoff through the next actual release; no extra signing rehearsal
is required.

Manual signing remains available with the existing version/run inputs. To pin a
manual dispatch, provide both `staging_artifact_id` and
`staging_artifact_sha256`; leaving both blank retains selection by the existing
artifact-name/run inputs.
