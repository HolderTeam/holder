#!/usr/bin/env bash
# Publishes core SDK archives to HolderTeam/holder-sdk, with an sdk-index.json describing them.
# By default the release is the development snapshot core-<tree>, a prerelease. With
# SDK_RELEASE=holder-v<version> it is instead the SDK shipped with that Holder release: a full
# release (a prerelease only for a pre-release version), marked latest. Does nothing when the
# release already exists: published SDKs are never replaced. The release is created as a draft and
# published only once every file is uploaded, so a failed run leaves no half-published release.
#
# Usage: publish-core-sdk.sh <archives dir> <core-selection.json> <core tree>
# Needs GH_TOKEN with contents: write on HolderTeam/holder-sdk, and RUN_URL; SDK_RELEASE optional.
# SDK_PACKAGES may name a directory of core's Ubuntu packages (libholder0 and libholder-dev .debs,
# with a libholder-<series>.sha256 per series) to publish in the same release, listed in the index.
set -euo pipefail

assets="$1"
selection="$2"
tree="$3"
repo=HolderTeam/holder-sdk
tag="${SDK_RELEASE:-core-$tree}"
if [[ "$tag" == holder-v* ]]; then
  version="${tag#holder-v}"
  [[ "$(jq -r .version "$selection")" == "$version" ]] || { echo "Core is not at version $version" >&2; exit 1; }
  title="Holder $version core SDK"
  about="The core SDK (libholder) shipped with Holder $version: use it for anything you distribute."
  if [[ "$version" == *-* ]]; then flags=(--prerelease --latest=false); latest=false; else flags=(--latest); latest=true; fi
else
  title="Core SDK ${tree:0:12}"
  about="A development build: see the README for which release to use."
  flags=(--prerelease --latest=false)
  latest=false
fi

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

if [[ -n "${SDK_PACKAGES:-}" ]]; then
  (cd "$SDK_PACKAGES" && for sums in libholder-*.sha256; do sha256sum -c "$sums"; done)
  for deb in "$SDK_PACKAGES"/*.deb; do
    name=$(basename "$deb")
    jq -n --arg name "$name" --arg repo "$repo" --arg tag "$tag" \
      --arg sha "$(sha256sum "$deb" | cut -d' ' -f1)" --argjson size "$(stat -c %s "$deb")" \
      '{name: $name, size: $size, sha256: $sha, url: "https://github.com/\($repo)/releases/download/\($tag)/\($name)"}'
  done | jq -s . > "$assets/packages.json"
  jq --slurpfile packages "$assets/packages.json" '.packages = $packages[0]' "$assets/sdk-index.json" > "$assets/index.tmp"
  mv "$assets/index.tmp" "$assets/sdk-index.json"
  rm "$assets/packages.json"
  cp "$SDK_PACKAGES"/*.deb "$SDK_PACKAGES"/libholder-*.sha256 "$assets/"
fi

notes="Holder core's SDK (libholder), built from HolderTeam/holder commit $commit (core tree $tree) by $RUN_URL.

$about sdk-index.json lists each archive's size and SHA-256 checksum."

gh release create "$tag" --repo "$repo" --target main --draft "${flags[@]}" \
  --title "$title" --notes "$notes" "$assets"/*
read -r id draft < <(release)
gh api -X PATCH "repos/$repo/releases/$id" -F draft=false -f make_latest="$latest" > /dev/null
echo "Published https://github.com/$repo/releases/tag/$tag"
