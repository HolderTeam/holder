#!/usr/bin/env bash
# Downloads core's libholder0 and libholder-dev packages for an Ubuntu series from the
# HolderTeam/holder-sdk release core-<tree> (published by Core / CI on main), checking them against
# that release's libholder-<series>.sha256, into <dir>. Prints found=true or found=false (no such
# release or packages, or holder-sdk unreachable) to $GITHUB_OUTPUT, so the caller builds the
# packages only when it must. A checksum mismatch is an error.
#
# Usage: fetch-core-debs.sh <core tree> <series> <dir>
set -euo pipefail

tree="$1"
series="$2"
dir="$3"
url="https://github.com/HolderTeam/holder-sdk/releases/download/core-$tree"
sums="libholder-$series.sha256"

mkdir -p "$dir"
found=false
if curl -fsSL --retry 3 -o "$dir/$sums" "$url/$sums"; then
  while read -r _sum name; do
    curl -fsSL --retry 3 -o "$dir/$name" "$url/$name"
  done < "$dir/$sums"
  (cd "$dir" && sha256sum -c "$sums")
  found=true
  echo "Took core's $series packages from core-$tree"
else
  rm -f "$dir/$sums"
  echo "holder-sdk has no $series packages for core-$tree; building them"
fi
echo "found=$found" >> "${GITHUB_OUTPUT:-/dev/null}"
