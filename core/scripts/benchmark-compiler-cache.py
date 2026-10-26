"""Measure cold compilation and a clean warm rebuild with one changed source file.

For manual CI only. Use an isolated empty CCACHE_DIR; no measurement cache is saved.
The temporary, behavior-neutral source edit is restored even if the build fails.
"""

import argparse
import json
import os
import subprocess
import time
from pathlib import Path


def run(*args):
    subprocess.run(args, check=True)


def build(directory):
    run("ccache", "--zero-stats")
    start = time.monotonic()
    run("cmake", "--build", str(directory), "--clean-first", "--parallel")
    elapsed = time.monotonic() - start
    stats = dict(
        line.split()
        for line in subprocess.check_output(
            ["ccache", "--print-stats"], text=True
        ).splitlines()
    )
    run("ccache", "--show-stats", "--verbose")
    return {
        "seconds": round(elapsed, 2),
        "hits": int(stats["direct_cache_hit"]) + int(stats["preprocessed_cache_hit"]),
        "misses": int(stats["cache_miss"]),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    # Only the explicitly isolated measurement cache can be cleared.
    if Path(os.environ["CCACHE_DIR"]).name != ".core-ccache-measure":
        parser.error("Use an isolated .core-ccache-measure directory")
    root = Path(__file__).resolve().parents[1]
    commands = json.loads((args.build / "compile_commands.json").read_text())
    if os.name == "nt" and not any(
        "/Z7" in entry.get("command", "") for entry in commands
    ):
        raise RuntimeError("Expected MSVC /Z7 in the actual compile commands")
    sources = {Path(entry["file"]).resolve() for entry in commands}
    source = min(
        (path for path in sources if path.is_relative_to(root / "src")),
        key=lambda path: path.stat().st_size,
    )
    original = source.read_bytes()
    run("ccache", "--clear")
    cold = build(args.build)
    try:
        source.write_bytes(
            original + b'\nstatic_assert(true, "compiler cache measurement");\n'
        )
        warm = build(args.build)
        # Keep the normal CI test/install/package checks on the changed-source build.
        if cold["misses"] == 0 or warm["hits"] == 0 or warm["misses"] == 0:
            raise RuntimeError(
                "Expected cold misses, then warm hits and a changed-source miss"
            )
        result = {
            "cold": cold,
            "warm_changed_source": warm,
            "source": str(source.relative_to(root)),
        }
        print(json.dumps(result, indent=2), flush=True)
        summary = os.environ.get("GITHUB_STEP_SUMMARY")
        if summary:
            with open(summary, "a") as output:
                output.write(
                    "## Core compiler cache measurement\n\n```json\n"
                    + json.dumps(result, indent=2)
                    + "\n```\n"
                )
    finally:
        source.write_bytes(original)


if __name__ == "__main__":
    main()
