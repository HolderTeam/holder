# Candidate preparation entry point

`release/scripts/release.py` coordinates the existing Release builds and all three
staging workflows. Use it after merging the intended release source and version.
It creates candidate branches, starts build/staging runs and saves public
source/run/artifact facts in one local JSON record. Use authenticated GitHub CLI
with repository contents and Actions write permissions, and Python 3.9+.

From the repository root, choose the reviewed full source SHA:

```sh
python3 release/scripts/release.py prepare \
  --record release/records/0.2.6-candidate-1.json \
  --candidate <full-reviewed-SHA> --version 0.2.6 --number 1 --wait
```

The candidate must contain the coordinator's current workflow input contracts.
The command checks root VERSION, creates `rc/0.2.6-candidate-1` at that exact SHA,
and dispatches Daemon CI with `core_build_type=Release`, `core_packages=source` and
Desktop CI with `build_type=release`. An existing build branch must still point
at that SHA. Every dispatch includes a unique request identifier and its returned
run ID is recorded; run URLs, attempts and actual workflow revisions appear in the
record and companion `.summary.md` file.

Once both runs succeed, preparation requires their unexpired component/SDK
artifacts and calls the shared manifest generator. It retains the manifest,
archive checksums, expiry evidence and each SDK producer's revision. It creates
`release/candidates/0.2.6-candidate-1` containing only the generated manifest and
evidence changes above the candidate source. That branch's immutable commit is
recorded separately from the application source SHA.

All staging workflows are dispatched on `main`, with `release_manifest` and
`release_manifest_ref` selecting that exact manifest commit. They fetch only the
JSON manifest and continue to download its pinned artifact IDs. This preserves
the existing main-only Windows staging environment policy. Windows tester signing
retains the workflow's default enabled setting; to explicitly prepare unsigned
Windows artifacts, add `--windows-sign-test-installer false` when creating the
record. That choice is fixed for the candidate and appears in its summary.

## Resume and inspect

The JSON record is the resume input. Preserve it and its summary, and use the
same record on a handoff to another operator. One preparation process owns a
record at a time; the local lock prevents simultaneous updates to that file.
Keep a single record for each candidate rather than starting independent copies.

```sh
python3 release/scripts/release.py prepare \
  --record release/records/0.2.6-candidate-1.json --wait
python3 release/scripts/release.py status \
  --record release/records/0.2.6-candidate-1.json
```

Without `--wait`, preparation makes one progress update and returns. Repeat it
to advance after remote jobs progress. With `--wait`, it polls every 30 seconds
until staged or stopped; Ctrl+C leaves the record resumable. `status` reads the
last saved record without network access and works while preparation is running.

Completed steps are revalidated and their accepted artifact IDs remain fixed.
An interrupted dispatch response is reconciled by its unique request identifier,
workflow and source/ref identity. A missing or ambiguous response never causes
an automatic duplicate dispatch. GitHub's
[current dispatch API](https://docs.github.com/en/rest/actions/workflows#create-a-workflow-dispatch-event)
returns run IDs directly; the reconciliation path also covers older/uncertain
responses. Requests are persisted before remote dispatch, and record updates are
atomic. If a process is killed without cleanup, confirm it has stopped before
removing the adjacent `.json.lock` file.

## Retry and recovery

For a failed, cancelled or timed-out step, correct the underlying problem and
retry only that step through the same entry point:

```sh
python3 release/scripts/release.py prepare \
  --record release/records/0.2.6-candidate-1.json --retry linux --wait
```

Step names are `backend`, `desktop`, `linux`, `windows`, `macos`. Failed attempts
remain in history. Retrying uses GitHub's failed-jobs rerun operation, retaining
the run's original source and inputs. Successful sibling steps are reused.
Builds already pinned in a verified manifest cannot be retried in that record.
Expired/deleted/changed accepted artifacts, moved branches or changed source
require a new candidate number and record. Do not edit completed pins to conceal
replacement inputs. For an uncertain retry response, inspect its recorded run
and resume to observe the requested attempt before requesting another retry.

The summary's `candidate_staged` status means the three staging runs succeeded
and their required artifacts were available at the last preparation check. Run
`prepare` again to refresh remote status and expiry checks. It does not make the publication
decision. Continue the [runbook](runbook.md) with platform/install/upgrade checks,
signing handoffs and [Python distribution preparation](python-preparation.md), then the maintainer's
publication decision. [Prepared Python publication](python-publication.md) uses
the same record. Desktop/core promotion, PPA
completion and website completion remain subsequent coordinator slices in #120.
The first end-to-end validation is the next actual release using this entry
point; a separate rehearsal or extra build matrix is not required.

Existing manual component and staging entry points remain supported. For manual
staging, leave `release_manifest_ref` blank to use a checked-in manifest as before,
or supply a full immutable commit holding a complete framework manifest.
