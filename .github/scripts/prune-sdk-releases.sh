#!/usr/bin/env bash
# Deletes all but the newest <keep> published releases in HolderTeam/holder-sdk whose tag starts
# with <prefix> (for example core-), together with their tags. Releases with other names, such as
# holder-v<version>, are never touched. Drafts are left to the publish script.
#
# Usage: prune-sdk-releases.sh <prefix> <keep>   (needs GH_TOKEN with contents: write on holder-sdk)
set -euo pipefail

prefix="$1"
keep="$2"
repo=HolderTeam/holder-sdk

gh api "repos/$repo/releases" --paginate \
  --jq ".[] | select(.draft == false and (.tag_name | startswith(\"$prefix\"))) | \"\(.created_at) \(.id) \(.tag_name)\"" |
  LC_ALL=C sort -r | tail -n +"$((keep + 1))" |
  while read -r _created id tag; do
    echo "Deleting $tag"
    gh api -X DELETE "repos/$repo/releases/$id"
    gh api -X DELETE "repos/$repo/git/refs/tags/$tag" || echo "No tag $tag to delete"
  done
