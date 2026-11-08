"""Shared PEP 517 version preparation; copied into standalone source packages."""
from __future__ import annotations

import functools
import importlib
import os
from pathlib import Path
import re
from email.parser import Parser


def read_version(package: Path, *, preview: bool = False, env=None) -> str:
    package = package.resolve()
    checkout = (package.parent / '_build_support.py').is_file()
    source = package.parent.parent / 'VERSION' if checkout else package / 'VERSION'
    if not source.is_file():
        raise ValueError(f'Missing version input: {source}; prepare the exported source before building')
    version = source.read_text().strip()
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:[-+.][0-9A-Za-z.-]+)?', version):
        raise ValueError(f'Invalid product version in {source}: {version!r}')
    if preview:
        number = int((os.environ if env is None else env).get('HOLDER_PYTHON_PREVIEW_NUMBER', '0'))
        if number < 0:
            raise ValueError('Preview number must be non-negative')
        if number:
            if checkout:
                version = f'{version}.post{number}'
            elif not version.endswith(f'.post{number}'):
                raise ValueError('Preview number differs from the prepared source version')
    metadata = package / 'PKG-INFO'
    if not checkout and metadata.is_file():
        recorded = Parser().parsestr(metadata.read_text())['Version']
        if recorded != version:
            raise ValueError('Prepared VERSION differs from source-distribution metadata')
    return version


def prepare(package: Path, *, preview: bool = False) -> None:
    version = read_version(package, preview=preview)
    (package / 'VERSION').write_text(version + '\n')
    support = package / '_build_support.py'
    if support.resolve() != Path(__file__).resolve():
        support.write_bytes(Path(__file__).read_bytes())


def backend_hooks(package: Path, backend: str, *, preview: bool = False) -> dict:
    delegate = importlib.import_module(backend)

    def wrapped(hook):
        @functools.wraps(hook)
        def call(*args, **kwargs):
            prepare(package, preview=preview)
            return hook(*args, **kwargs)
        return call

    names = ('build_wheel', 'build_sdist', 'build_editable',
             'prepare_metadata_for_build_wheel', 'prepare_metadata_for_build_editable',
             'get_requires_for_build_wheel', 'get_requires_for_build_sdist',
             'get_requires_for_build_editable')
    return {name: wrapped(getattr(delegate, name)) for name in names if hasattr(delegate, name)}
