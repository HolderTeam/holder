#!/usr/bin/env bash
# Downloads the tested Linux daemon build for this tree from HolderTeam/holder-sdk (published by
# Daemon / CI on main), checks it against its .sha256 and unpacks it into <stage dir>. Prints
# found=true or found=false (no such release, or holder-sdk unreachable) to $GITHUB_OUTPUT, so
# the caller builds the daemon only when it must. A checksum mismatch is an error.
#
# Usage: fetch-daemon-build.sh <cache key> <stage dir>
set -euo pipefail

key="$1"
stage="$2"
tag="daemon-${key#linux-daemon-}"
url="https://github.com/HolderTeam/holder-sdk/releases/download/$tag"
archive=holder-daemon-linux-x86_64.tar.gz
work=$(mktemp -d)

found=false
if curl -fsSL --retry 3 -o "$work/$archive" "$url/$archive" &&
   curl -fsSL --retry 3 -o "$work/$archive.sha256" "$url/$archive.sha256"; then
  (cd "$work" && sha256sum -c "$archive.sha256")
  mkdir -p "$stage"
  tar -xzf "$work/$archive" -C "$stage"
  found=true
  echo "Took the daemon from $tag"
else
  echo "holder-sdk has no $tag; building the daemon"
fi
echo "found=$found" >> "${GITHUB_OUTPUT:-/dev/null}"
