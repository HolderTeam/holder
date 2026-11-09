# Publish the prepared Python files

Use this after [Python preparation](python-preparation.md), the essential release
checks and the maintainer's publication decision. The reviewed source must contain
the prepared-publication workflow inputs introduced here.

The matching `holder-v<version>` tag must already exist and point to the exact
candidate commit. The command checks lightweight and annotated tags; it creates
neither kind. Creating an ordinary Holder release tag starts the existing
Launchpad publication workflow. Follow the tag boundary in the
[runbook](runbook.md); neither a draft release nor TestPyPI isolates that trigger.

Publish one package to one explicit destination:

```sh
python3 release/scripts/release.py publish-python \
  --record release/records/0.2.6-candidate-1.json \
  --package kit --destination pypi --wait
python3 release/scripts/release.py publish-python \
  --record release/records/0.2.6-candidate-1.json \
  --package python --destination pypi --wait
```

The coordinator records the accepted package evidence in an immutable metadata
commit on `release/python/0.2.6-candidate-1`, with a JSON file under
`release/python-manifests/`. The application source/tag remains the original
candidate. This metadata contains public source, run, artifact and checksum facts.

The existing Kit/holder-python PyPI workflows run on the release tag. Their
prepared path skips SDK and distribution builds, fetches the recorded artifact
IDs, checks run/source/attempt identity and archive hashes, then repeats the
distribution metadata, file-hash and Kit SDK identity checks. Uploads retain the
existing workflow filenames, environments and Trusted Publisher action.

Before uploading, the publisher checks the destination's
[release JSON endpoint](https://docs.pypi.org/api/json/#get-a-release). Existing
files must have the expected name, size and SHA256; unexpected, duplicate, yanked
or conflicting files stop publication. Only missing files enter the upload
directory. Matching existing files are reused, including when a previous upload
stopped partway through. There is no blanket skip-existing or replacement switch.
An upload racing another publisher may fail; resume after inspecting the run.

After upload, the workflow waits briefly for all expected file hashes to appear
in the index. It retains a `<package>-<destination>-publication` artifact containing
`publication.json`: source, original package run/artifact, prepared metadata
commit, destination, complete file hashes and verification time. The coordinator
checks that receipt and independently verifies the index, then records completion
under `publications.kit_pypi` or `publications.python_pypi`. The local summary
retains each completed destination and receipt hash.

## Resume and recover

Repeat the same command and record to observe a pending run or recheck a completed
destination. The dispatch request is persisted before the API write and uncertain
responses reconcile without creating another publishing run. Completed
destinations are checked against the public index even after their CI artifacts
expire; successful siblings need no new upload.

For a failed run, inspect and correct the cause, then retry its failed jobs:

```sh
python3 release/scripts/release.py publish-python \
  --record release/records/0.2.6-candidate-1.json \
  --package kit --destination pypi --retry kit_pypi --wait
```

Use `python_pypi` for holder-python. Retry selection must match the explicit
package/destination. If uploading succeeded but a later check failed, the next
attempt selects the remaining files, or skips the upload entirely when all
expected hashes already exist. A conflicting published file always stops the
attempt; it cannot be replaced by retrying.

Prepared inputs must remain downloadable until that destination is complete.
Retain the exact distribution bytes/evidence when starting publication and watch
the recorded artifact expiry. If missing bytes expire during a partial upload,
recover those exact bytes through a retained copy and the manual procedure; do
not rebuild and call the result the tested candidate. A changed build needs a
new candidate and, where published files would conflict, a new version.

TestPyPI remains optional. Use `--destination testpypi` only for a specific
publishing check; it has independent completion records and still requires the
matching release tag for these stable prepared files. This path does not produce
holder-python `.postN` previews. Existing direct manual publishing inputs and
preview rules remain available when prepared-release inputs are omitted.

For manual publication of prepared bytes, dispatch the existing package workflow
on the matching release tag, set `destination`, and supply the recorded
`prepared_release_path` and full immutable `prepared_release_ref`. Leave Kit's
SDK-manifest inputs empty and holder-python's `preview_number=0`.

This completes the Python artifact handoff, not desktop/core promotion, signing
handoffs, Launchpad build/availability checks or website completion. Exercise the
full remote path through the next actual release; a separate rehearsal or
TestPyPI upload is not required.
