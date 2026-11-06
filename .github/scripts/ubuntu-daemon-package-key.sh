#!/usr/bin/env bash
# Key the complete, tested runtime bundle by source and packaging inputs, not VERSION alone.
# A recipe change invalidates it as well. System/PPA dependencies are captured in the bundle;
# bump the recipe when deliberately refreshing them for unchanged source.
set -euo pipefail
series=$1
case "$series" in noble|resolute) ;; *) echo "Unsupported Ubuntu series: $series" >&2; exit 2 ;; esac
inputs=$(git rev-parse \
  HEAD:core HEAD:daemon HEAD:api HEAD:common HEAD:cli HEAD:VERSION HEAD:.gitmodules \
  HEAD:.github/workflows/daemon-ubuntu-packages.yml \
  HEAD:.github/actions/core-ubuntu-packages \
  HEAD:.github/scripts/fetch-core-debs.sh \
  HEAD:.github/scripts/ubuntu-daemon-package-key.sh \
  HEAD:.github/scripts/fetch-daemon-debs.sh)
printf 'ubuntu-daemon-debs-%s-amd64-RelWithDebInfo-%s\n' \
  "$series" "$(sha256sum <<< "$inputs" | cut -c1-32)"
