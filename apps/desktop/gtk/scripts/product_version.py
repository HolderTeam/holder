#!/usr/bin/env python3
"""Read the product version in a checkout or a prepared standalone source tree."""
from pathlib import Path
import re


def read(source: Path) -> str:
    root_version = source / '../../../VERSION'
    local_version = source / 'VERSION'
    if not root_version.is_file() and (root_version.parent / '.git').exists():
        raise ValueError('Missing repository-root VERSION; refusing a stale exported version')
    path = root_version if root_version.is_file() else local_version
    if not path.is_file():
        raise ValueError('Missing VERSION; prepare the standalone desktop source before building')
    version = path.read_text().strip()
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?', version):
        raise ValueError(f'Invalid product version in {path}')
    return version


if __name__ == '__main__':
    print(read(Path(__file__).resolve().parents[1]))
