#!/usr/bin/env python3
"""Check that nothing outside contract/ uses the contract's tooling, and that it cannot be published.

The contract describes Holder's interface; generated code implements it. Code that ships, and the
generated code in particular, must never import holder_contract or depend on it, or the contract
would become runtime code. contract/pyproject.toml holds only tool settings, so the tooling cannot
be built or uploaded. Documentation is not checked."""

from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
IMPORT = re.compile(r"^\s*(?:from|import)\s+holder_contract\b")
DEPENDENCY = re.compile(r"holder[_-]contract")
MANIFESTS = {"pyproject.toml", "setup.py", "setup.cfg", "CMakeLists.txt", "meson.build", "package.json",
             "build.gradle", "build.gradle.kts", "settings.gradle.kts", "vcpkg.json"}
PACKAGE_TABLE = re.compile(r"^\[(?:project|build-system)\]", re.MULTILINE)


def tracked_files():
    listing = subprocess.run(["git", "ls-files", "-z"], cwd=ROOT, capture_output=True, check=True).stdout
    for name in listing.decode().split("\0"):
        if name and not name.startswith("contract/"):
            yield ROOT / name


def main():
    problems = []
    for path in tracked_files():
        name = path.relative_to(ROOT).as_posix()
        is_python = path.suffix in (".py", ".pyi")
        is_manifest = path.name in MANIFESTS or (path.name.startswith("requirements") and path.suffix == ".txt")
        if not (is_python or is_manifest) or not path.is_file():
            continue
        for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            if (is_python and IMPORT.search(line)) or (is_manifest and DEPENDENCY.search(line)):
                problems.append(f"{name}:{number}: {line.strip()}")
    if problems:
        print("Only contract/ may use the contract's tooling (holder_contract):")
        print("\n".join(f"  {line}" for line in problems))
    if PACKAGE_TABLE.search((ROOT / "contract" / "pyproject.toml").read_text()):
        problems.append("contract/pyproject.toml")
        print("contract/pyproject.toml must hold only tool settings, not [project] or [build-system]: "
              "the contract's tooling is never built or published.")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
