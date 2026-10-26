#!/usr/bin/env python3
"""Check a shared selection against this checkout before consumers fetch its archives."""

import argparse
import json
import subprocess
from pathlib import Path

ARCHITECTURES = {"linux": "x86_64", "macos": "arm64", "windows": "x86_64"}


def verify(selection, commit, tree, build_type, platforms):
    if selection.get("source") != "artifact" or selection.get("commit") != commit:
        raise ValueError("Shared SDK selection does not describe this run's checkout")
    if selection.get("core_tree") != tree:
        raise ValueError("Shared SDK selection has the wrong core tree")
    if build_type not in {"RelWithDebInfo", "Release"}:
        raise ValueError("Unsupported SDK configuration")
    for platform in platforms:
        matches = [
            a
            for a in selection["assets"]
            if (a["platform"], a["architecture"], a["build_type"])
            == (platform, ARCHITECTURES[platform], build_type)
        ]
        if len(matches) != 1:
            raise ValueError(
                f"No unique {platform} {build_type} SDK in the shared selection"
            )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--build-type", required=True)
    parser.add_argument("--platforms", required=True)
    parser.add_argument(
        "--selection", type=Path, default=Path("out/core-selection.json")
    )
    args = parser.parse_args()
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    tree = subprocess.check_output(["git", "rev-parse", "HEAD:core"], text=True).strip()
    if args.commit != commit:
        parser.error("Provided SDK commit differs from this checkout")
    verify(
        json.loads(args.selection.read_text()),
        commit,
        tree,
        args.build_type,
        json.loads(args.platforms),
    )
    print(f"Verified shared SDK selection for {commit}, core tree {tree}")


if __name__ == "__main__":
    main()
