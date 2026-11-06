"""Run one Meson test with a scratch tree owned by this invocation."""

import os
import pathlib
import shutil
import subprocess
import sys
import tempfile


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: run_with_test_tmp.py PROGRAM [ARGS...]", file=sys.stderr)
        return 2

    root = pathlib.Path(tempfile.mkdtemp(prefix="holder_gtk_test_"))
    env = os.environ.copy()
    for key in ("TMPDIR", "TMP", "TEMP"):
        env[key] = str(root)
    for key, leaf in (
        ("XDG_CACHE_HOME", "cache"),
        ("XDG_CONFIG_HOME", "config"),
        ("XDG_DATA_HOME", "data"),
        ("XDG_STATE_HOME", "state"),
    ):
        path = root / leaf
        path.mkdir()
        env[key] = str(path)

    try:
        return subprocess.run(sys.argv[1:], env=env, check=False).returncode
    finally:
        if "HOLDER_TEST_KEEP_TMP" in os.environ:
            print(f"Retained GTK test scratch directory: {root}", file=sys.stderr)
        else:
            try:
                shutil.rmtree(root)
            except OSError as error:
                print(f"Could not remove GTK test scratch directory {root}: {error}", file=sys.stderr)


if __name__ == "__main__":
    sys.exit(main())
