# Website

The Holder website, [www.holder.team](https://www.holder.team/): what Holder is, downloads,
guides, and the update feed (`version.json`) that installed desktop apps check. It lives under
`release/` because it is the public face of a release; it is a static Jekyll site, not part of
Holder itself.

## Local development

The GitHub Pages workflow builds with Ruby 3.2, Jekyll, and a small Node.js
download check. For Fedora development, install the Ruby build tools, Bundler,
Node.js, Python, GitHub CLI and GnuPG:

```sh
sudo dnf install -y \
  ruby ruby-devel rubygem-bundler \
  gcc-c++ make redhat-rpm-config nodejs python3 gh gnupg2
```

Install the locked gems, build the site, and run the check used in CI:

```sh
bundle install
python3 ../scripts/website-release.py
JEKYLL_ENV=production bundle exec jekyll build
node scripts/test-downloads.js
```

To preview the site locally:

```sh
bundle exec jekyll serve
```

The checked-in lockfile was generated with Bundler 2.4.20. Fedora 45 supplies
a newer Ruby and Bundler; Bundler will report any compatibility issue during
`bundle install`. CI and the deployment pin Ruby 3.2: see
[Website / CI](../../.github/workflows/website-ci.yml), which checks pull requests, and
[Release / Website](../../.github/workflows/release-website.yml), which deploys to GitHub Pages
whenever this directory changes on `main`. Run these commands from this directory.

## Published release data

The build generates `_data/downloads.json`, `version.json` and the current checksum
copies from public GitHub releases. Authenticate GitHub CLI locally, or supply
`GH_TOKEN` in CI. The generator uses the highest numeric `holder-vX.Y.Z` version
that is published, not a draft or prerelease. Other product tags are ignored.
It does not read development root `VERSION` or fall back to an older release if
the newest stable release is incomplete.

Supported assets must be unique, completely uploaded and have GitHub SHA256 digests:

- Linux: `Holder-X.Y.Z-x86_64.AppImage`, `Holder-X.Y.Z-SHA256SUMS`, its `.asc`
  signature, and `Holder-linux-release-key.asc`.
- macOS: `Holder-macos-X.Y.Z.dmg` and its `.sha256` file.
- Windows: exactly one `Holder-windows-X.Y.Z-dev-Setup.exe` or
  `Holder-windows-X.Y.Z-Setup.exe`, with the corresponding `.sha256` file.
  The `dev` name remains labelled development; a published stable framework tag
  does not change that installer channel.

The Linux checksum signature is verified against the committed public release key.
All checksum entries must name the expected binary and match GitHub's binary
digest. Small downloaded checksum/signature/key files are also hash checked.
Binary installers are not downloaded or executed during a website build.
Generated feed/catalog/checksum files are ignored by Git; the older 0.2.0/0.2.1
checksum files remain as historical public URLs.

`_data/ubuntu.json` is maintained separately. Its `available` status and version
carry forward the site's declared PPA availability (currently 0.2.5). Change that
record after checking the intended PPA packages; GitHub publication never updates
it automatically. Use `{"status":"unconfirmed","version":null}` when availability
has not been established. The site can show an older PPA version alongside newer
GitHub downloads without claiming they are the same release.

Website CI exercises the generator and builds/tests the site against generated
data. Deployment also runs when a stable Holder release is published, always
using website/tooling source from `main`. The release event dispatches the normal
workflow on `main` so deployments obey the Pages environment's branch rule;
the dispatcher's `actions: write` permission starts that run. Generation failure stops deployment and
leaves the existing site/feed in place. Complete the release's missing assets or
correct the metadata/signatures, then dispatch **Release / Website** on `main`;
do not edit generated files or publish a placeholder feed.

The desktop feed remains at `/version.json` with schema 1 and the existing
platform download fields. Run generator unit tests from the repository root:

```sh
python3 -m unittest discover -s release/tests -p test_website_release.py -v
```
