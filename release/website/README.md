# holder-website
Website for the Holder project

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
`bundle install`. The GitHub Pages deployment itself remains pinned to Ruby
3.2 in [the Pages workflow](.github/workflows/pages.yml).
