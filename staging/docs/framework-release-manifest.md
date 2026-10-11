# Framework release manifests

Development staging follows the latest Daemon CI run on main. For an RC or production release,
commit one manifest and pass the same `release_manifest` path to all three
staging workflows. Each reads its platform entry, uses the shared product
version/core pin, and bundles the original manifest in the package.

`release-manifests/validation-0.2.1-rc.1.json` pins the branch artifacts used to
validate this machinery. It is a test candidate, not a production release;
its component inputs remain subject to Actions artifact retention.

1. Dispatch `daemon-ci.yml` on the commit to release with `core_build_type=Release`. It
   builds core from `core/` in that commit and records the SDK in its `core-selection`
   artifact; the core commit to pin is the `commit` in that selection.
2. Dispatch the desktop artifact workflows with `build_type=release`.
3. After the selected runs pass, record their run IDs and source commits below.
4. Commit the manifest and run staging with its relative path. Leave run
   overrides at defaults. Windows branch validation can additionally use
   `sign_test_installer=false`; normal main staging retains protected signing.

```json
{
  "version": "0.2.1-rc.1",
  "core": {"commit": "<full core SHA>", "build_type": "Release"},
  "platforms": {
    "linux": {
      "desktop": {"repository": "HolderTeam/holder-desktop", "run_id": "<Linux desktop run>", "commit": "<desktop SHA>"},
      "backend": {"repository": "HolderTeam/holder", "run_id": "<Daemon CI Release run>", "commit": "<daemon SHA>"}
    },
    "windows": {
      "desktop": {"repository": "HolderTeam/holder-desktop", "run_id": "<Windows desktop run>", "commit": "<desktop SHA>"},
      "backend": {"repository": "HolderTeam/holder", "run_id": "<Daemon CI Release run>", "commit": "<daemon SHA>"}
    },
    "macos": {
      "desktop": {"repository": "HolderTeam/holder-desktop", "run_id": "<macOS desktop run>", "commit": "<desktop SHA>"},
      "backend": {"repository": "HolderTeam/holder", "run_id": "<Daemon CI Release run>", "commit": "<daemon SHA>"}
    }
  }
}
```

Replace placeholders with full lowercase commit SHAs and numeric run IDs.
The same daemon run supplies all three platform artifacts. Desktop
commits may differ between platform entries when deliberately selected; each
entry is checked against its selected run and build metadata.

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
