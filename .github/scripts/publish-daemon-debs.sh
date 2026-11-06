#!/usr/bin/env bash
# Publish a tested .deb runtime bundle under its immutable source/recipe key.
# Usage: publish-daemon-debs.sh <package dir> <package key> (GH_TOKEN, RUN_URL required)
set -euo pipefail

dir="$1"
key="$2"
repo=HolderTeam/holder-sdk
tag="$key"

(cd "$dir" && sha256sum -c packages.sha256)

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
notes="Holder daemon Ubuntu runtime packages, built and unit-tested from HolderTeam/holder commit $commit by $RUN_URL. Includes the matching libholder and caste runtime packages. The tag identifies the Ubuntu series, architecture, build configuration and source/recipe inputs.

A development build: see the README. Verify packages.sha256 before installing."

gh release create "$tag" --repo "$repo" --target main --draft --prerelease --latest=false \
  --title "$(date -u +%Y-%m-%d-%H-%M) Daemon (Ubuntu packages) ${key##*-}" --notes "$notes" "$dir"/*
read -r id draft < <(release)
gh api -X PATCH "repos/$repo/releases/$id" -F draft=false -f make_latest=false > /dev/null
echo "Published https://github.com/$repo/releases/tag/$tag"
