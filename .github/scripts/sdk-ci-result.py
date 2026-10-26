#!/usr/bin/env python3
"""Report one stable required check, including a failed shared SDK prerequisite."""

import argparse
import json
import os


def check(scope, selected, sdk_selected, results, sdk_job="core-sdk"):
    if results["select"]["result"] != "success":
        raise ValueError(
            "CI selection failed; no skipped work can be treated as passing"
        )
    if scope == "core" and not sdk_selected and results[sdk_job]["result"] != "skipped":
        raise ValueError("SDK stage ran although no SDK consumer was selected")
    if scope == "core" and sdk_selected and results[sdk_job]["result"] != "success":
        raise ValueError(
            "Shared core SDK stage failed or was cancelled; see the platform SDK jobs"
        )
    if selected and results[sdk_job]["result"] != "success":
        raise ValueError(
            "Blocked by the shared core SDK stage; consumer builds did not start"
        )
    expected = "success" if selected else "skipped"
    if results[scope]["result"] != expected:
        raise ValueError(
            f"{scope}: expected {expected}, got {results[scope]['result']}"
        )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scope", choices=["core", "daemon", "kit", "python"])
    args = parser.parse_args()
    results = json.loads(os.environ["RESULTS"])
    print(json.dumps(results, indent=2))
    try:
        check(
            args.scope,
            os.environ["SELECTED"] == "true",
            os.environ["SDK_SELECTED"] == "true",
            results,
            os.environ.get("SDK_JOB", "core-sdk"),
        )
    except ValueError as error:
        message = str(error)
        if os.environ.get("GITHUB_STEP_SUMMARY"):
            with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as output:
                output.write(f"## {args.scope} CI\n\n{message}.\n")
        raise SystemExit(message) from error


if __name__ == "__main__":
    main()
