#!/usr/bin/env bash
# Print the cache key for a staged Linux holderd. Daemon CI saves under it and Python / Check
# restores from it, so both must compute it here. Git tree IDs cover every file that feeds the
# build, including the caste submodule's commit and core/, which the daemon's core SDK is built from.
set -euo pipefail

build_type=$1
ubuntu_series=${2:-24.04}

case "$ubuntu_series" in
  24.04) ;;
  *) echo "Unsupported Ubuntu series: $ubuntu_series" >&2; exit 2 ;;
esac

inputs=$(git rev-parse \
  HEAD:core \
  HEAD:daemon \
  HEAD:api \
  HEAD:common \
  HEAD:cli \
  HEAD:VERSION \
  HEAD:.gitmodules \
  HEAD:.github/actions/linux-daemon-deps \
  HEAD:.github/actions/linux-daemon-build)

printf 'linux-daemon-ubuntu-%s-%s-%s\n' \
  "$ubuntu_series" "$build_type" "$(sha256sum <<< "$inputs" | cut -c1-16)"
