#!/usr/bin/env python3
"""Set or check Holder's version, which every component shares.

Native components read root VERSION. Exported source trees receive a generated
copy with --prepare-source. The Python pyproject versions remain synchronized
until their metadata migration is complete.

    release/scripts/version.py              print the version
    release/scripts/version.py --check      fail unless every copy equals the root VERSION
    release/scripts/version.py 0.2.6        set root VERSION and remaining Python copies
    release/scripts/version.py --prepare-source core --destination /tmp/core-source
"""
import argparse
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
VERSION_FILES = ("VERSION",)
SOURCE_COMPONENTS = {"core": "core", "daemon": "daemon", "desktop": "apps/desktop/gtk"}
PYPROJECTS = ("python/kit/pyproject.toml", "python/holder/pyproject.toml")
PROJECT_VERSION = re.compile(r'^(version = ")([^"]*)(")$', re.MULTILINE)


def read(root=ROOT):
    """Each file and the version it holds."""
    found = {name: (root / name).read_text().strip() for name in VERSION_FILES}
    for name in PYPROJECTS:
        matches = PROJECT_VERSION.findall((root / name).read_text())
        if len(matches) != 1:
            raise ValueError(f"{name} must have exactly one project version line")
        found[name] = matches[0][1]
    return found


def write(version, root=ROOT):
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
        raise ValueError(f"A release version is three numbers, like 0.2.5; got {version!r}")
    for name in VERSION_FILES:
        (root / name).write_text(version + "\n")
    for name in PYPROJECTS:
        path = root / name
        path.write_text(PROJECT_VERSION.sub(lambda m: m[1] + version + m[3], path.read_text(), count=1))


def mismatches(root=ROOT):
    found = read(root)
    return {name: value for name, value in found.items() if value != found["VERSION"]}


def prepare_source(component, destination=None, root=ROOT):
    version = (root / 'VERSION').read_text().strip()
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?', version):
        raise ValueError(f'Invalid release version: {version!r}')
    destination = Path(destination) if destination is not None else root / SOURCE_COMPONENTS[component]
    marker = 'meson.build' if component == 'desktop' else 'CMakeLists.txt'
    if not (destination / marker).is_file():
        raise ValueError(f'{destination} is not prepared {component} source (missing {marker})')
    (destination / 'VERSION').write_text(version + '\n')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version", nargs="?", help="set this version everywhere")
    parser.add_argument("--check", action="store_true", help="fail unless every copy equals the root VERSION")
    parser.add_argument('--prepare-source', choices=SOURCE_COMPONENTS,
                        help='generate VERSION in a native component source tree')
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
        print(f"Set version {args.version} in {', '.join(VERSION_FILES + PYPROJECTS)}")
    elif args.check:
        wrong = mismatches()
        if wrong:
            root_version = read()["VERSION"]
            for name, value in wrong.items():
                print(f"{name} has {value}, but VERSION has {root_version}", file=sys.stderr)
            print("Set the version everywhere with release/scripts/version.py <version>.", file=sys.stderr)
            return 1
        print(f"Root version and remaining Python metadata agree at {read()['VERSION']}.")
    else:
        print(read()["VERSION"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
