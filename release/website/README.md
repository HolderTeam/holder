# Website

The Holder website, [www.holder.team](https://www.holder.team/): what Holder is, downloads,
guides, and the update feed (`version.json`) that installed desktop apps check. It lives under
`release/` because it is the public face of a release; it is a static Jekyll site, not part of
Holder itself.

## Local development

The GitHub Pages workflow builds with Ruby 3.2, Jekyll, and a small Node.js
download check. For Fedora development, install the Ruby build tools, Bundler,
and Node.js:

```sh
sudo dnf install -y \
  ruby ruby-devel rubygem-bundler \
  gcc-c++ make redhat-rpm-config nodejs
```

Install the locked gems, build the site, and run the check used in CI:

```sh
bundle install
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
[Website / CI](../../.github/workflows/website-ci.yml). Run these commands from this directory.
