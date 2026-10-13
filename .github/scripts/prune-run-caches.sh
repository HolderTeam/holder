#!/usr/bin/env bash
# Deletes this branch's Actions caches whose key is <prefix><run id>-<attempt> and that earlier runs
# saved, keeping the one this run saved. Compiler caches and vcpkg binary caches are keyed like
# this and saved by every run outside a pull request, so without this they pile up (a compiler
# cache is about 1.4 GB) until they fill the repository's Actions cache and push out everything
# else. Keys where something other than a run id follows the prefix are never touched.
#
# Usage: prune-run-caches.sh <key prefix>   (needs GH_TOKEN with actions: write)
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
