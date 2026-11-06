#!/usr/bin/env bash
# Fetch the complete daemon/libholder/caste runtime bundle. An absent shelf entry is a miss;
# incomplete downloads and invalid checksums fail instead of using a partial bundle.
set -euo pipefail
key=$1
dir=$2
url="https://github.com/HolderTeam/holder-sdk/releases/download/$key"
mkdir -p "$dir"
found=false
if curl -fsSL --retry 3 -o "$dir/packages.sha256" "$url/packages.sha256"; then
  while read -r sum name; do
    # A manifest may name only plain .deb filenames inside the bundle.
    if [[ ! "$sum" =~ ^[0-9a-f]{64}$ || ! "$name" =~ ^[a-zA-Z0-9][a-zA-Z0-9.+_-]*\.deb$ ]]; then
      echo 'Invalid package manifest' >&2
      exit 1
    fi
    curl -fsSL --retry 3 -o "$dir/$name" "$url/$name"
  done < "$dir/packages.sha256"
  (cd "$dir" && sha256sum -c packages.sha256)
  found=true
  echo "Took Ubuntu runtime packages from $key"
else
  rm -f "$dir/packages.sha256"
  echo "holder-sdk has no $key; building packages"
fi
echo "found=$found" >> "${GITHUB_OUTPUT:-/dev/null}"
