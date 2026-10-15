#!/usr/bin/env python3
"""Set or check Holder's version, which every component shares.

The root VERSION is the version. Parts that are built on their own from their own directory keep a
copy, because their source package or archive holds only that directory: core (core/VERSION, for
its Launchpad tarball and standalone build), the desktop app (apps/desktop/gtk/VERSION, for its
native source package) and the Python packages (their pyproject.toml, for their sdists). The
daemon reads the root VERSION, and its source package copies it in.

    release/scripts/version.py              print the version
    release/scripts/version.py --check      fail unless every copy equals the root VERSION
    release/scripts/version.py 0.2.6        set the version everywhere
"""
import argparse
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
VERSION_FILES = ("VERSION", "core/VERSION", "apps/desktop/gtk/VERSION")
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


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version", nargs="?", help="set this version everywhere")
    parser.add_argument("--check", action="store_true", help="fail unless every copy equals the root VERSION")
    args = parser.parse_args(argv)
    if args.version and args.check:
        parser.error("give a version or --check, not both")
    if args.version:
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
        print(f"Every component is at {read()['VERSION']}.")
    else:
        print(read()["VERSION"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
