#!/usr/bin/env bash
# Publishes a tested Linux daemon build (the staged backend archive and its .sha256) to
# HolderTeam/holder-sdk as the prerelease daemon-<key>, where <key> comes from
# linux-daemon-cache-key.sh, so Python / Check and others can take it instead of building the
# daemon. Does nothing when the release already exists. The release is created as a draft and
# published only once every file is uploaded.
#
# Usage: publish-daemon-build.sh <archive dir> <cache key>   (needs GH_TOKEN and RUN_URL)
set -euo pipefail

dir="$1"
key="$2"
repo=HolderTeam/holder-sdk
tag="daemon-${key#linux-daemon-}"

(cd "$dir" && sha256sum -c ./*.sha256)

# Drafts have no tag yet, so look releases up by listing them rather than by tag.
release() {
  gh api "repos/$repo/releases" --paginate \
    --jq ".[] | select(.tag_name == \"$tag\") | \"\(.id) \(.draft)\""
}

while read -r id draft; do
  if [[ "$draft" == false ]]; then
    echo "$repo already has $tag"
    exit 0
  fi
  echo "Deleting an unfinished draft of $tag"
  gh api -X DELETE "repos/$repo/releases/$id"
done < <(release)

commit=$(git rev-parse HEAD)
case "$key" in
  linux-daemon-ubuntu-26.04-*) ubuntu_series=26.04 ;;
  *) ubuntu_series=24.04 ;;
esac
notes="Holder's Linux daemon (holderd and holderctl, staged for Ubuntu $ubuntu_series), built and tested from HolderTeam/holder commit $commit by $RUN_URL. The tag names a hash of every source tree the build depends on.

A development build: see the README. Each archive's SHA-256 checksum is in the .sha256 file beside it."

gh release create "$tag" --repo "$repo" --target main --draft --prerelease --latest=false \
  --title "Daemon (Linux) ${key##*-}" --notes "$notes" "$dir"/*
read -r id draft < <(release)
gh api -X PATCH "repos/$repo/releases/$id" -F draft=false -f make_latest=false > /dev/null
echo "Published https://github.com/$repo/releases/tag/$tag"
