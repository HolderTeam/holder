# Publishing Holder for Python

The distribution is `holder-python`; users import `holder`. It is a pure Python
package, so one `py3-none-any` wheel and one source archive cover all supported
platforms. The version follows the repository's root `VERSION`.

[`Release / Holder Python to PyPI`](../../../.github/workflows/release-holder-python-pypi.yml)
builds both files, checks their metadata, installs each in a fresh virtual
environment, and saves them as the `holder-python-distributions` artifact. A
pull request or a manual run with `destination=none` only builds. Publishing
uses Trusted Publishing. PyPI requires a manual run from the
`holder-v<version>` tag with a matching `python/holder/pyproject.toml` version.
The initial package-only `0.2.5` release also accepts
`holder-python-v0.2.5-r2`; this tag does not start a Holder-wide release.

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
deployment branch or tag rules, allow `main`, `holder-v*`, and, for the
backfill, `holder-python-v0.2.5-r2`; add required reviewers if desired. The Kit
release workflow already uses these environment names, but **each PyPI project
needs its own Trusted Publisher entry**. No PyPI API token or GitHub secret is
needed. A pending publisher does not reserve the
project name until the first successful upload.

## TestPyPI previews

TestPyPI already has `holder-python 0.2.5` from the original README. Uploaded
files cannot be replaced, even after deleting the project. Keep that project
and use a new preview number for each revised build:

1. Merge the changes to `main`. In GitHub Actions, run **Release / Holder
   Python to PyPI** on `main` with `destination=testpypi` and
   `preview_number=1`. The build temporarily changes the package version to
   `0.2.5.post1`; the committed `pyproject.toml` and root `VERSION` stay at
   `0.2.5`.
2. Wait for the TestPyPI upload and published-wheel install check to pass.
   Review the `0.2.5.post1` page on TestPyPI.
3. If the description needs another edit, merge that edit and repeat from
   `main` with `preview_number=2`, then `3`, and so on. Each number creates
   new wheel and source filenames. A preview number can only publish to
   TestPyPI from `main`.

## First `0.2.5` PyPI release

After accepting a preview, create `holder-python-v0.2.5-r2` on its reviewed
`main` commit. The original `holder-python-v0.2.5` tag points to the old README
and must not be moved. Run **Release / Holder Python to PyPI** from the new tag
with `destination=pypi` and `preview_number=0`. This publishes version `0.2.5`
with the corrected README to real PyPI, which has not received that version.
Confirm the project page and install it in a fresh environment with
`python -m pip install holder-python==0.2.5`.

## Later releases

Update the unified version with `release/scripts/version.py <new-version>`, then
publish `holder-python` from the matching `holder-v<version>` tag as part of
the normal Holder release. The package-specific tag is accepted only for
`0.2.5`.

The workflow resolves README links to URLs at the build commit, so the project
description links back to that exact source. A failed upload does not make an
existing file replaceable: inspect the index before retrying, and choose a new
preview number if its files were already published.
