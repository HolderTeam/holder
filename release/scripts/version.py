#!/usr/bin/env python3
"""Set or check Holder's version, which every component shares.

Native components and Python package builds read root VERSION. Exported source
trees receive generated version/build metadata with --prepare-source.

    release/scripts/version.py              print the version
    release/scripts/version.py --check      validate root and dynamic package metadata
    release/scripts/version.py 0.2.6        set only root VERSION
    release/scripts/version.py --prepare-source core --destination /tmp/core-source
"""
import argparse
from pathlib import Path
import re
import sys
import tomllib

ROOT = Path(__file__).resolve().parents[2]
VERSION_FILES = ("VERSION",)
SOURCE_COMPONENTS = {"core": "core", "daemon": "daemon", "desktop": "apps/desktop/gtk",
                     "kit": "python/kit", "holder-python": "python/holder"}
PYPROJECTS = ("python/kit/pyproject.toml", "python/holder/pyproject.toml")


def read(root=ROOT):
    """The sole committed product-version value."""
    return {"VERSION": (root / 'VERSION').read_text().strip()}


def write(version, root=ROOT):
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
        raise ValueError(f"A release version is three numbers, like 0.2.5; got {version!r}")
    for name in VERSION_FILES:
        (root / name).write_text(version + "\n")


def check(root=ROOT):
    version = read(root)['VERSION']
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', version):
        raise ValueError(f'Invalid release version: {version!r}')
    for name in PYPROJECTS:
        project = tomllib.loads((root / name).read_text())['project']
        if 'version' in project or 'version' not in project.get('dynamic', []):
            raise ValueError(f'{name} must derive its version dynamically from root VERSION')


def prepare_source(component, destination=None, root=ROOT):
    version = (root / 'VERSION').read_text().strip()
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?', version):
        raise ValueError(f'Invalid release version: {version!r}')
    destination = Path(destination) if destination is not None else root / SOURCE_COMPONENTS[component]
    marker = ('meson.build' if component == 'desktop' else
              'pyproject.toml' if component in ('kit', 'holder-python') else 'CMakeLists.txt')
    if not (destination / marker).is_file():
        raise ValueError(f'{destination} is not prepared {component} source (missing {marker})')
    (destination / 'VERSION').write_text(version + '\n')
    if component in ('kit', 'holder-python'):
        (destination / '_build_support.py').write_bytes((root / 'python/_build_support.py').read_bytes())


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version", nargs="?", help="set root VERSION")
    parser.add_argument("--check", action="store_true", help="validate root version and dynamic Python metadata")
    parser.add_argument('--prepare-source', choices=SOURCE_COMPONENTS,
                        help='generate version/build metadata in a component source tree')
    parser.add_argument('--destination', type=Path, help='exported component source directory')
    args = parser.parse_args(argv)
    if sum((bool(args.version), args.check, bool(args.prepare_source))) > 1:
        parser.error('choose a version, --check or --prepare-source')
    if args.destination and not args.prepare_source:
        parser.error('--destination requires --prepare-source')
    if args.prepare_source:
        prepare_source(args.prepare_source, args.destination)
    elif args.version:
        write(args.version)
        print(f"Set root VERSION to {args.version}")
    elif args.check:
        check()
        print(f"Root version is {read()['VERSION']}; Python metadata derives it dynamically.")
    else:
        print(read()["VERSION"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
