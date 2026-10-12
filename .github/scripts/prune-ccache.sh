#!/usr/bin/env bash
# Deletes this branch's compiler caches whose key starts with the given prefix and that earlier
# runs saved, keeping the one this run saved. The keys end in <run id>-<attempt>, and every run
# outside a pull request saves a new one, so without this they pile up (about 1.4 GB each) until
# they fill the repository's Actions cache and push out everything else.
#
# Usage: prune-ccache.sh <key prefix>   (needs GH_TOKEN with actions: write)
set -euo pipefail

prefix="$1"
gh cache list --repo "$GITHUB_REPOSITORY" --ref "$GITHUB_REF" --key "$prefix" --limit 100 \
  --json id,key --jq '.[] | "\(.id) \(.key)"' |
  while read -r id key; do
    run_id="${key#"$prefix"}"
    run_id="${run_id%%-*}"
    # Only older runs: two runs finishing together must not delete each other's new cache.
    if [[ "$run_id" =~ ^[0-9]+$ ]] && (( run_id < GITHUB_RUN_ID )); then
      echo "Deleting $key"
      gh cache delete --repo "$GITHUB_REPOSITORY" "$id"
    fi
  done
