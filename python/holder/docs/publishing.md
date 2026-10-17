# Publishing Holder for Python

The distribution is `holder-python`; users import `holder`. It is a pure Python
package, so one `py3-none-any` wheel and one source archive cover all supported
platforms. The version follows the repository's root `VERSION`.

[`Release / Holder Python to PyPI`](../../../.github/workflows/release-holder-python-pypi.yml)
builds both files, checks their metadata, installs each in a fresh virtual
environment, and saves them as the `holder-python-distributions` artifact. A
pull request or a manual run with `destination=none` only builds. Publishing
uses Trusted Publishing and requires a manual run from the
`holder-v<version>` tag with a matching `python/holder/pyproject.toml` version.
For the initial `0.2.5` upload only, the workflow also accepts
`holder-python-v0.2.5`. That tag releases this package alone and starts no
Holder-wide release workflow.

## One-time setup for each package index

PyPI and TestPyPI are separate services. Sign in to each one where you want to
publish and configure a **GitHub Actions Trusted Publisher** for the project
`holder-python`. If that project has not yet been created, use your account's
**Publishing** page to add a *pending publisher*. If it already belongs to you,
open the project's **Manage → Publishing** page and add a publisher there.
Use these values on both sites:

| Field | Value |
| --- | --- |
| PyPI project name (pending publisher only) | `holder-python` |
| GitHub owner | `HolderTeam` |
| GitHub repository | `holder` |
| Workflow filename | `release-holder-python-pypi.yml` |
| Environment name | `pypi` on PyPI; `testpypi` on TestPyPI |

The repository's GitHub environments must have those exact names. If they use
deployment tag rules, allow `holder-v*` and, for the backfill,
`holder-python-v0.2.5`; add required reviewers if desired. The Kit release
workflow already uses these environment names, but **each PyPI project needs
its own Trusted Publisher entry**. No PyPI
API token or GitHub secret is needed. A pending publisher does not reserve the
project name until the first successful upload.

## First `0.2.5` package-only release

1. Merge the publishing workflow. Check that `holder-python` version `0.2.5`
   has not already been uploaded to either index. The existing
   `holder-v0.2.5` tag points to an older commit, so leave it alone.
2. In GitHub Actions, run **Release / Holder Python to PyPI** on `main` with
   `destination=none`. Download the `holder-python-distributions` artifact and
   inspect the wheel and source archive if desired. Pull requests also run the
   same build checks.
3. Create and push `holder-python-v0.2.5` on the reviewed `main` commit. Run
   this workflow from that **tag**, first with `destination=testpypi`. The
   selected GitHub environment and the corresponding TestPyPI publisher must
   both be configured before the upload. The workflow then downloads the
   published wheel from TestPyPI and imports it in a fresh environment; wait
   for that check to pass before proceeding.
4. Run the same workflow from the same tag with `destination=pypi`. Confirm
   that `0.2.5` appears on the `holder-python` PyPI project page and
   install it in a fresh environment with
   `python -m pip install holder-python==0.2.5`.

## Later releases

Update the unified version with `release/scripts/version.py <new-version>`, then
publish `holder-python` from the matching `holder-v<version>` tag as part of
the normal Holder release. The package-specific tag is accepted only for
`0.2.5`.

The workflow resolves README links to URLs at the release commit in its build
checkout, so the PyPI description links back to that exact source. A failed
upload does not make an existing file replaceable: inspect the index before
retrying, and choose a new version if its files were already published.
