#!/usr/bin/env bash
# Publishes this run's core SDK archives to HolderTeam/holder-sdk as the prerelease core-<tree>,
# with an sdk-index.json describing them. Does nothing when that release already exists: core/'s
# tree is unchanged, so its SDKs are already published. The release is created as a draft and
# published only once every file is uploaded, so a failed run leaves no half-published release.
#
# Usage: publish-core-sdk.sh <archives dir> <core-selection.json> <core tree>
# Needs GH_TOKEN with contents: write on HolderTeam/holder-sdk, and RUN_URL.
set -euo pipefail

assets="$1"
selection="$2"
tree="$3"
repo=HolderTeam/holder-sdk
tag="core-$tree"

[[ "$(jq -r .core_tree "$selection")" == "$tree" ]] || { echo "Selection is for another core tree" >&2; exit 1; }

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

commit=$(jq -r .commit "$selection")
jq --arg repo "$repo" --arg tag "$tag" --arg run "$RUN_URL" '
  {schema_version: 1, repository: $repo, release_tag: $tag, core_tree: .core_tree,
   commit: .commit, version: .version, source: "https://github.com/HolderTeam/holder",
   workflow_run: $run,
   assets: [.assets[] | . + {url: "https://github.com/\($repo)/releases/download/\($tag)/\(.name)"}]}
' "$selection" > "$assets/sdk-index.json"

# Every archive the index lists must be here, and nothing else.
diff <(jq -r '.assets[].name' "$assets/sdk-index.json" | sort) \
     <(find "$assets" -maxdepth 1 -type f ! -name sdk-index.json -printf '%f\n' | sort)

notes="Holder core's SDK (libholder), built from HolderTeam/holder commit $commit (core tree $tree) by $RUN_URL.

A development build: see the README for which release to use, and sdk-index.json for each archive's size and SHA-256 checksum."

gh release create "$tag" --repo "$repo" --target main --draft --prerelease --latest=false \
  --title "Core SDK ${tree:0:12}" --notes "$notes" "$assets"/*
read -r id draft < <(release)
gh api -X PATCH "repos/$repo/releases/$id" -F draft=false -f make_latest=false > /dev/null
echo "Published https://github.com/$repo/releases/tag/$tag"
