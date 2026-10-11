#!/usr/bin/env python3
"""Check that core and the code built on it do not reach into each other's files by path.

Core must build on its own, so nothing in core/ may name the directories beside it. The daemon,
holderctl, the shared platform code and Holder Kit use core only through what core installs (its
headers, CMake package and resources), so their code, CMake and scripts may not name files in
core/. Kit goes further and uses only core's stable C API, so its C sources may include no other
core header. The builds against core's SDK catch includes of anything core does not install; this
check catches the paths that a build in one tree would not notice. Documentation is not checked."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
CHECKED = {".c", ".cc", ".cpp", ".h", ".hpp", ".cmake", ".in", ".sh", ".py", ".json"}
CORE_NEIGHBOURS = ("daemon", "cli", "common", "api", "python", "staging", "release")
CORE_USERS = ("daemon", "cli", "common", "python/kit")
C_API_USERS = ("python/kit",)
C_SOURCES = {".c", ".cc", ".cpp", ".h", ".hpp"}

OUT_OF_CORE = re.compile(r"(?:\.\./)+(?:%s)(?:/|\b)" % "|".join(CORE_NEIGHBOURS))
INTO_CORE = re.compile(r"(?:\.\./)+core/(?!\.\.)\S|(?:\.\./)+holder-core\b|(?<![\w.-])core/(?:src|include|tests|scripts|schema|resources|cmake)/")
# The one sanctioned way in: building core from source for local development (the daemon's make.sh
# and Windows Debug presets point HOLDER_CORE_SOURCE_DIR at ../core itself, and Kit's developer
# script at ../../core, not at a file inside it).
SOURCE_DIR = re.compile(r"""(?:\.\./)+core["']?\s*$|(?:\.\./)+core["'),]|(?:\.\./)+core\s*&&|(?:\.\./)+core/CMakeLists\.txt""")
# Any core header other than the C API's.
NOT_C_API = re.compile(r"""#\s*include\s*[<"]holder/(?!holder\.h[>"])""")


def checked_files(top):
    for path in (ROOT / top).rglob("*"):
        if not path.is_file() or any(part.startswith(("build", "out", ".")) for part in path.relative_to(ROOT / top).parts[:-1]):
            continue
        if path.suffix in CHECKED or path.name in ("CMakeLists.txt", "make.sh"):
            yield path


def scan(tops, pattern, allowed=None, suffixes=None):
    for top in tops:
        for path in checked_files(top):
            if suffixes and path.suffix not in suffixes:
                continue
            for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
                if pattern.search(line) and not (allowed and allowed.search(line)):
                    yield f"{path.relative_to(ROOT)}:{number}: {line.strip()}"


def main():
    problems = [("core/ names a directory beside it", list(scan(["core"], OUT_OF_CORE))),
                ("code built on core names a file in core/", list(scan(CORE_USERS, INTO_CORE, SOURCE_DIR))),
                ("Kit includes a core header other than the C API", list(scan(C_API_USERS, NOT_C_API, suffixes=C_SOURCES)))]
    failed = False
    for title, lines in problems:
        if lines:
            failed = True
            print(f"{title}:")
            print("\n".join(f"  {line}" for line in lines))
    if failed:
        print("Use core's installed interface instead; see daemon/AGENTS.md and python/kit/AGENTS.md.", file=sys.stderr)
        return 1
    print("Core boundary: no paths across it in either direction.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
