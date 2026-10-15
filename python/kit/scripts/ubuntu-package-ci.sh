#!/usr/bin/env bash
# Install and test python3-holder-kit in a fresh Ubuntu container of the given series.
# Kit / CI builds the packages (libholder0 from core/ in the same commit, and python3-holder-kit)
# and passes their directory; this checks them as a user would install them.
set -euo pipefail
series="${1:?Ubuntu series required}"
packages="${2:?directory of the built .deb packages required}"
source /etc/os-release
test "$VERSION_CODENAME" = "$series"
export DEBIAN_FRONTEND=noninteractive
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
packages="$(cd "$packages" && pwd)"
apt-get update
apt-get install -y --no-install-recommends "$packages"/libholder0_*.deb "$packages"/python3-holder-kit_*.deb
# These checks run before installing optional adapters or test dependencies.
! dpkg-query -W -f='${Status}' libholder-dev 2>/dev/null | grep -q 'install ok installed'
! command -v cc
cd /tmp
python3 -I "$root/scripts/test-installed-debian.py"
apt-get install -y --no-install-recommends python3-pytest python3-pandas python3-networkx
python3 -I -m pytest -q --import-mode=importlib -o cache_dir=/tmp/pytest-cache \
  "$root/tests/test_holder.py" "$root/tests/test_connections.py" \
  "$root/tests/test_pandas.py" "$root/tests/test_graph.py"
