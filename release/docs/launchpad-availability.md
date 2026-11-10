# Observe Ubuntu package availability

The existing [Launchpad upload workflow](../../.github/workflows/release-launchpad.yml)
starts when the maintainer pushes `holder-v<version>`. Check its upload jobs as
usual. A successful upload means Launchpad received source packages; it does
not mean the binary builds have finished or that APT can see the packages.

Use the same release record to observe the public archive:

```sh
python3 release/scripts/release.py observe-ppa \
  --record release/records/0.2.6-candidate-1.json --wait
```

The command checks that the existing tag identifies the recorded candidate.
It only reads GitHub and Launchpad and updates the local record. It does not
create a tag, start a workflow, upload packages, retry remote builds, install
packages, or change website availability. Omit `--wait` for one observation;
interrupt a wait and resume with the same record. If a source has not appeared,
inspect the existing upload job before assuming more waiting will help.

The plan covers the four source packages in both `noble` and `resolute`, at
exact versions `<version>-1~<series>1` unless a packaging revision is explicitly
selected. The default architecture coverage is `amd64`. For a broader check,
select the intended architectures on the first observation, for example
`--ppa-architecture amd64 --ppa-architecture arm64`. The saved plan retains that
coverage on resume; an amd64 result does not establish availability on other
architectures.

For each source and series, the observer uses the [Launchpad publication and
build API](https://api.launchpad.net/1.0/#source_package_publishing_history) and
checks the relevant public APT indexes. It reports:

| State | What to check |
| --- | --- |
| `awaiting_source` | The exact active source version has not appeared. Inspect the upload job or wait for Launchpad to accept it. |
| `awaiting_builds` | A build is unscheduled, queued, in progress or waiting for dependencies. Daemon and Kit may wait for the matching core development package. |
| `build_failed` | Inspect the recorded build logs. Correct the cause through the existing Launchpad or packaging procedure, then resume observation. `--wait` stops on build failures. |
| `awaiting_publication` | Builds succeeded but the exact source, binary publications or APT index entries are still pending. |
| `available` | The source and all required binaries for the selected architectures are published and the exact package versions appear in the APT indexes. |

The binary set is `libholder0`, `libholder-dev`, `holder-daemon`,
`python3-holder-kit`, `holder-desktop` and the architecture-independent `holder`
package. The observer follows the selected source publication to its builds and
binaries, verifies the series, version and architecture, and records package
filenames, sizes and SHA-256 values from the index. The local `ppa` ledger keeps
an observation and a completion receipt for each source/series. A later check
reports missing or changed accepted publications rather than accepting different
package IDs or hashes. A temporary API failure leaves earlier recorded results
available for review; run the same command again to refresh them.

For a one-off public archive inspection without a release record, use
`python3 release/scripts/launchpad-release.py --version <version>`. This prints
observations without associating them with a Git tag or saving receipts.

## Packaging repairs

Inspect the failing source and series first. A dependency wait alone is not a
reason to re-upload. If a packaging repair needs a fresh revision, use the
existing targeted upload procedure in the [runbook](runbook.md). That upload is
an explicit maintainer action. Then tell the observer which exact revision to
track, for example:

```sh
python3 release/scripts/release.py observe-ppa \
  --record release/records/0.2.6-candidate-1.json \
  --ppa-revision daemon:noble=2 --wait
```

Only that source/series target changes; its prior plan and evidence are retained
in history. Repeating the same selection is safe. Other targets retain their
versions and receipts. Revisions do not move backwards and newer versions are
never selected automatically.

`ppa_available` means the selected versions appeared in public package metadata
for the recorded architecture coverage. It does not prove that Launchpad's
source tree equals the Git commit, that downloaded packages passed APT signature
verification, or that installs and upgrades work. Keep the source-upload run
links and installation results with the release record. Use the existing
fresh-install and upgrade checks before changing the reviewed website PPA
availability declaration. Validate this handoff through the next actual release.
