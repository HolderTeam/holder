# Prepare the Python release sets

After [candidate staging](candidate-preparation.md), use the same local record to
prepare Kit and holder-python for the selected source and version:

```sh
python3 release/scripts/release.py packages \
  --record release/records/0.2.6-candidate-1.json --wait
```

This advances any incomplete build/staging work first, then dispatches the two
existing Python release workflows on the frozen RC branch with `destination=none`.
Kit receives the immutable candidate manifest commit. It skips its Core / SDK
producer and fetches the candidate's pinned core selection and Release SDK
archives instead. The selection ZIP must match the recorded artifact ID and
checksum; each SDK archive must match the selection's checksum and size, and
passes the existing SDK manifest/interface verifier. A cached SDK producer may
legitimately precede the candidate source SHA; its identity remains recorded.

The normal wheel repair, installation, live Kit tests, clean Debian runtime and
source-install checks remain in the workflows. Successful outputs are:

| Package | Artifacts | Contents |
| --- | --- | --- |
| Kit | `kit-release-distributions`, `kit-release-manifest` | 18 platform/interpreter wheels, one sdist; source, SDK identity and file hashes |
| holder-python | `holder-python-distributions`, `holder-python-release-manifest` | One wheel and one sdist; source and file hashes |

The coordinator checks successful run/source/attempt identity and unexpired
artifact ownership, downloads accepted artifact IDs and verifies archive hashes,
the complete file set, package metadata and per-file hashes. Kit wheel core
metadata and its release manifest must agree with the original candidate's core
selection. The local record retains the exact artifacts, distribution manifests,
file hashes, attempts and expiry. The summary includes every distribution hash.
`packages_prepared` means these checks passed at the last observation.

Resume with the same command after interruption. Dispatch reconciliation, locking
and failed-job retries use the same mechanism as candidate preparation:

```sh
python3 release/scripts/release.py packages \
  --record release/records/0.2.6-candidate-1.json --retry kit --wait
```

Use `--retry python` for holder-python. Successful siblings and native builds are
reused. Once both Python sets have been accepted, their pins cannot be replaced
or retried in that candidate. Missing, expired, changed or mismatched accepted
inputs require a new candidate. Preserve the record for operator handoffs;
`status` displays it locally without dispatching anything. Refreshing `packages`
rechecks run/artifact identities and expiry, without downloading accepted
distribution bytes again.

This command starts no uploads and creates no production tag. Continue with
[prepared Python publication](python-publication.md) after the maintainer's
publication decision. The coordinator publishes those recorded bytes; direct
manual PyPI dispatches still build their own distributions unless supplied with
the prepared-release inputs. Do not treat that default manual path as promotion
of this prepared set.
Use the next actual release to validate the complete path; no extra rehearsal or
TestPyPI upload is required.

For a manual build-only Kit dispatch, supply both `release_manifest` and
`release_manifest_ref`, run on the matching candidate SHA, and leave
`destination=none`. Omitting both retains the existing same-run SDK build path.
