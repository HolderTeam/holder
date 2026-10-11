#!/usr/bin/env python3
"""Reject breaking changes to the HTTP contract unless API_VERSION tells old clients.

Clients state the API_VERSION range they support (MAJOR.MINOR, minimum inclusive, maximum
exclusive), so a breaking change must move API_VERSION out of the range existing clients accept:
a minor bump while the major version is 0, and a major bump from 1.0. oasdiff finds the breaking
changes. A change to something that has not been released yet can be accepted without a bump by a
maintainer's "api: unreleased change" label on the pull request. API_VERSION never goes down."""

import argparse
import json
import os
import re
import subprocess
import sys

ERROR, WARNING = 3, 2


def version(text):
    match = re.fullmatch(r"\s*(\d+)\.(\d+)\s*", text)
    if not match:
        sys.exit(f"API_VERSION must be MAJOR.MINOR, got {text.strip()!r}")
    return int(match[1]), int(match[2])


def excludes_old_clients(base, head):
    if head[0] != base[0]:
        return head[0] > base[0]
    return base[0] == 0 and head[1] > base[1]


def describe(change):
    where = " ".join(part for part in (change.get("operation"), change.get("path")) if part)
    return f"{where}: {change.get('text') or change.get('id')}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--oasdiff", required=True)
    parser.add_argument("--base-spec", required=True)
    parser.add_argument("--head-spec", required=True)
    parser.add_argument("--base-version", required=True, help="File holding the base API_VERSION")
    parser.add_argument("--head-version", required=True, help="File holding the head API_VERSION")
    parser.add_argument("--unreleased", action="store_true", help="The pull request is labelled as an unreleased change")
    args = parser.parse_args()

    base = version(open(args.base_version).read())
    head = version(open(args.head_version).read())
    if head < base:
        sys.exit(f"API_VERSION went down from {base[0]}.{base[1]} to {head[0]}.{head[1]}")

    result = subprocess.run([args.oasdiff, "breaking", args.base_spec, args.head_spec, "--format", "json"],
                            capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"oasdiff failed:\n{result.stderr}")
    changes = json.loads(result.stdout or "[]")
    errors = [c for c in changes if c.get("level") == ERROR]
    warnings = [c for c in changes if c.get("level") == WARNING]

    lines = [f"API_VERSION {base[0]}.{base[1]} -> {head[0]}.{head[1]}; "
             f"{len(errors)} breaking change(s), {len(warnings)} warning(s)."]
    lines += [f"- breaking: {describe(c)}" for c in errors]
    lines += [f"- warning: {describe(c)}" for c in warnings]
    if not errors:
        verdict, ok = "No breaking changes.", True
    elif excludes_old_clients(base, head):
        verdict, ok = "Breaking changes come with an API_VERSION that excludes old clients.", True
    elif args.unreleased:
        verdict, ok = "Breaking changes accepted: labelled as changes to something not yet released.", True
    else:
        bump = f"{base[0]}.{base[1] + 1}" if base[0] == 0 else f"{base[0] + 1}.0"
        verdict, ok = (f"Breaking changes need API_VERSION {bump} or later so that existing clients stop "
                       "accepting this daemon, or, for something not yet released, the "
                       "'api: unreleased change' label."), False
    lines.append(verdict)
    report = "\n".join(lines)
    print(report)
    if os.environ.get("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as summary:
            summary.write("## API contract\n\n" + report + "\n")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
