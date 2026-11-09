# Framework release manifests

Development staging follows the latest Daemon CI run on main. For an RC or production release,
commit one manifest and pass the same `release_manifest` path to all three
staging workflows. The path is relative to `release/staging/` (for example,
`release-manifests/0.2.5.json`). Each reads its platform entry, uses the shared product
version/core pin, and bundles the original manifest in the package.

`release-manifests/validation-0.2.1-rc.1.json` pins the branch artifacts used to
validate this machinery. It is a test candidate, not a production release; it
predates the desktop and core moving into this repository, its artifacts have
expired, and staging no longer accepts its separate desktop repository.

1. Dispatch `daemon-ci.yml` on the commit to release with `core_build_type=Release`. It
   builds core from `core/` in that commit and records the SDK in its `core-selection`
   artifact; the core commit to pin is the `commit` in that selection.
2. Dispatch Desktop / CI (`desktop-ci.yml`) on the same commit with `build_type=release`.
3. After the selected runs pass, generate the manifest from their explicit run IDs.
4. Commit the manifest and its evidence record and run staging with the manifest's relative path. Leave run
   overrides at defaults. Windows branch validation can additionally use
   `sign_test_installer=false`; normal main staging retains protected signing.

## Generate a candidate manifest

Use Python 3.9+ and an authenticated GitHub CLI with permission to read repository
contents and Actions artifacts. Both selected runs must be successful Release builds
of the **same full candidate commit**. The generator does not pick the latest run or
start builds. Run it from the repository root:

```sh
python3 release/staging/scripts/generate-release-manifest.py \
  --candidate <full-candidate-SHA> --version 0.2.6 \
  --desktop-run <Desktop-CI-run-ID> --backend-run <Daemon-CI-run-ID> \
  --output release/staging/release-manifests/0.2.6-candidate-1.json
```

It checks root VERSION at that commit, run repository/workflow/source identity,
all six unexpired Release staging artifacts, payload metadata and binary presence,
the Linux tarball checksums, and the daemon run's core selection. Core provenance
uses the same verifier as staging. Cached SDK producer commits may differ from
the candidate only when their core tree and root version match it. A retried run
during generation, duplicate artifact name, missing platform or incompatible
metadata stops generation without emitting a candidate.

The output retains the schema below, adding `artifact_id` and `artifact_sha256`
to each component and `selection_artifact_id` / `selection_artifact_sha256` to
`core`. All three staging workflows download these immutable artifact IDs;
rerunning a component workflow cannot silently substitute artifacts with the same
names. Older manual manifests without artifact pins retain their existing behaviour.
Partial artifact pins are rejected.

The companion `0.2.6-candidate-1.evidence.json` records candidate identity, run
attempts, artifact IDs, downloaded ZIP SHA256 values, GitHub digests, expiry
times, and the core selection with each SDK producer and archive digest. These
ZIP hashes identify the archives inspected by the generator; staging's download
action checks GitHub's digest when retrieving the pinned IDs. Retain both files
in review. Existing output files are never overwritten; select a new candidate
filename when inputs change.

The evidence record's `inputs_expire_at` is the earliest expiry of the staging
inputs, not a retention extension. Stage all three platforms before that time.
If inputs expire or are deleted, rebuild/select the candidate components and
generate a new record; never silently fall back to newer runs. SDK publication
still performs its own SDK download/digest checks. The next actual release will
validate the complete staging procedure with a generated candidate.

The [candidate-preparation coordinator](../../docs/candidate-preparation.md) uses
this same generator and places the manifest/evidence in a candidate metadata
commit. Staging's optional `release_manifest_ref` accepts that full commit SHA
while the workflows run on `main`. Only the selected JSON under `release-manifests/`
is fetched; all three platform selections are validated before it replaces the
local manifest. A blank ref retains checked-in-manifest behaviour.

## Existing manifest shape

```json
{
  "version": "0.2.1-rc.1",
  "core": {"commit": "<full core SHA>", "build_type": "Release"},
  "platforms": {
    "linux": {
      "desktop": {"repository": "HolderTeam/holder", "run_id": "<Desktop / CI release run>", "commit": "<desktop SHA>"},
      "backend": {"repository": "HolderTeam/holder", "run_id": "<Daemon CI Release run>", "commit": "<daemon SHA>"}
    },
    "windows": {
      "desktop": {"repository": "HolderTeam/holder", "run_id": "<Desktop / CI release run>", "commit": "<desktop SHA>"},
      "backend": {"repository": "HolderTeam/holder", "run_id": "<Daemon CI Release run>", "commit": "<daemon SHA>"}
    },
    "macos": {
      "desktop": {"repository": "HolderTeam/holder", "run_id": "<Desktop / CI release run>", "commit": "<desktop SHA>"},
      "backend": {"repository": "HolderTeam/holder", "run_id": "<Daemon CI Release run>", "commit": "<daemon SHA>"}
    }
  }
}
```

Replace placeholders with full lowercase commit SHAs and numeric run IDs.
The same Daemon CI run supplies all three platform backends, and the same
Desktop / CI run all three desktops. The desktop and the backend must come from
the same repository, and the desktop run must be built from the same
`apps/desktop/` source and root `VERSION` as the backend's commit; each entry is also checked
against its selected run and build metadata.

A Release manifest selects artifact names ending in `-release` and rejects
development desktop or daemon/core configurations. Backend's own
source revision is checked independently of the workflow run revision. Missing
platform entries, expired artifacts, failed runs or mismatched pins stop staging
before assembly. Existing Linux-only manifests with `appimage_version` and
top-level component entries remain supported.

Actions component artifacts have limited retention. Stage the candidate while
those inputs are available; signing and promotion subsequently consume the
verified staged product. Pin changes require rebuilding/rechecking the affected
candidate rather than silently selecting different component runs.

Manifests written before the launcher was removed may still carry a `launcher` entry for Windows and
macOS (for example `release-manifests/validation-0.2.1-rc.1.json`). It is ignored: the launcher is no
longer a component, and nothing is downloaded or verified for it.
