#!/usr/bin/env python3
"""Select core and SDK consumer work from the actual PR or push diff."""

import argparse
import json
import os
import re
import subprocess
from pathlib import Path

SHARED = re.compile(
    r"^(\.github/workflows/(daemon-ci|core-sdk)\.yml$|"
    r"\.github/actions/core-sdk/|\.github/scripts/(select-sdk-ci|sdk-ci-result|verify-shared-sdk)\.py$|"
    r"\.github/scripts/tests/)"
)
PATTERNS = {
    "core": re.compile(r"^(core/|\.github/workflows/core-(ci|sdk-publish)\.yml$)"),
    "daemon": re.compile(
        r"^(daemon/|cli/|common/|api/|core/|VERSION$|\.gitmodules$|"
        r"\.github/workflows/daemon-|\.github/actions/core-ubuntu-packages/|"
        r"\.github/scripts/fetch-core-debs\.sh$|\.github/actions/linux-daemon-|"
        r"\.github/scripts/linux-daemon-cache-key\.sh$|\.github/scripts/check-core-boundary\.py$)"
    ),
    "kit": re.compile(
        r"^(python/kit/|core/|\.github/workflows/kit-ci\.yml$|"
        r"\.github/actions/core-ubuntu-packages/|\.github/scripts/fetch-core-debs\.sh$|"
        r"\.github/scripts/check-core-boundary\.py$)"
    ),
    "python": re.compile(
        r"^(python/holder/|api/openapi\.yaml$|\.github/workflows/python-check\.yml$|"
        r"\.github/scripts/fetch-daemon-build\.sh$)"
    ),
}


def select(paths, scope="changed"):
    if scope not in {"changed", "all", "none", *PATTERNS}:
        raise ValueError(f"Unknown CI scope: {scope}")
    result = {}
    for component, pattern in PATTERNS.items():
        relevant = [
            p
            for p in paths
            if component != "daemon"
            or not re.match(r"^core/(include/holder/holder\.h$|src/capi/|tests/)", p)
        ]
        result[component] = (
            scope == "all"
            or scope == component
            or (
                scope == "changed"
                and any(SHARED.search(p) or pattern.search(p) for p in relevant)
            )
        )
    result["sdk"] = any(result.values())
    result["all_platforms"] = result["core"] or result["daemon"] or result["kit"]
    return result


def changed_paths(event_name, event):
    if event_name == "pull_request":
        revision = "HEAD^1...HEAD^2"
    elif event_name == "push" and event.get("before", "").strip("0"):
        revision = event["before"] + "...HEAD"
    else:
        revision = "origin/main...HEAD"
    return subprocess.check_output(
        ["git", "diff", "--name-only", revision], text=True
    ).splitlines()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scope", default="changed")
    parser.add_argument("--build-type", default="RelWithDebInfo")
    args = parser.parse_args()
    event_name = os.environ["GITHUB_EVENT_NAME"]
    if args.build_type not in {"RelWithDebInfo", "Release"}:
        parser.error("Unsupported SDK build type")
    if args.build_type == "Release" and (
        event_name != "workflow_dispatch" or args.scope != "daemon"
    ):
        parser.error("Release is supported by the manual daemon scope")
    event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text())
    paths = changed_paths(event_name, event) if args.scope == "changed" else []
    result = select(paths, args.scope)
    print(json.dumps({"paths": paths, "selected": result}, indent=2))
    with open(os.environ["GITHUB_OUTPUT"], "a") as output:
        output.writelines(
            f"{name}={str(value).lower()}\n" for name, value in result.items()
        )


if __name__ == "__main__":
    main()
