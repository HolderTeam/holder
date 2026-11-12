"""Generated C and C++ go through clang-format-18 with the .clang-format of the component that holds
them (core or daemon), so they read like the hand-written code around them and the component's
format check needs no exceptions for them.

The version is pinned: another clang-format lays code out differently, and compiler check, which
compares regenerated files byte for byte, would then report them out of date on that machine.
"""

from __future__ import annotations

import functools
import shutil
import subprocess
from pathlib import Path

EXECUTABLE = "clang-format-18"
VERSION = "18."
REPOSITORY = Path(__file__).resolve().parents[2]


class FormatterUnavailable(RuntimeError):
    """clang-format-18 is not installed, or is not the version the generated files need."""


@functools.cache
def _executable() -> str:
    path = shutil.which(EXECUTABLE)
    if path is None:
        raise FormatterUnavailable(
            f"{EXECUTABLE} is required to generate the C and C++ files; install it "
            f"(for example: sudo apt install {EXECUTABLE}) and make sure it is on PATH.")
    version = subprocess.run([path, "--version"], capture_output=True, text=True, check=True).stdout
    if f"version {VERSION}" not in version:
        raise FormatterUnavailable(f"{path} reports {version.strip()!r}; the generated files need {EXECUTABLE}.")
    return path


def clang_format(text: str, path: str) -> str:
    """text laid out by clang-format-18; path, relative to the repository, picks the style (the
    .clang-format of its top-level directory, such as core or daemon) and the language, and is what
    clang-format names in its messages."""
    style = REPOSITORY / path.split("/", 1)[0] / ".clang-format"
    result = subprocess.run(
        [_executable(), f"--style=file:{style}", f"--assume-filename={path}"],
        input=text, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise FormatterUnavailable(f"{EXECUTABLE} could not format {path}: {result.stderr.strip()}")
    return result.stdout
