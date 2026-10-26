"""Configure core's CI compiler cache without rewriting source or fixture paths."""

import hashlib
import os
import shutil
import subprocess
from pathlib import Path


def main():
    workspace = Path(os.environ["GITHUB_WORKSPACE"]).resolve()
    compiler = shutil.which("cl.exe" if os.name == "nt" else "c++")
    if compiler is None:
        raise SystemExit("C++ compiler is not on PATH")
    compiler = Path(compiler).resolve()
    identity = hashlib.sha256()
    identity.update(compiler.read_bytes())
    for value in (
        str(compiler),
        str(workspace),
        os.environ.get("ImageOS", ""),
        subprocess.check_output(["ccache", "--version"], text=True),
    ):
        identity.update(value.encode())
        identity.update(b"\0")
    prefix = "core-ccache-v1-{}-{}-{}-{}-{}-".format(
        os.environ["RUNNER_OS"],
        os.environ["RUNNER_ARCH"],
        os.environ["CACHE_VARIANT"],
        os.environ["CACHE_BUILD_TYPE"],
        identity.hexdigest()[:16],
    )
    key = prefix + os.environ["GITHUB_RUN_ID"] + "-" + os.environ["GITHUB_RUN_ATTEMPT"]
    # Avoid CCACHE_BASEDIR/NOHASHDIR: tests use absolute __FILE__ paths for fixtures;
    # Windows also needs absolute vcpkg include paths. A moved checkout safely misses.
    directory = (
        ".core-ccache-measure"
        if os.environ["CACHE_MEASURE"] == "true"
        else ".core-ccache"
    )
    with open(os.environ["GITHUB_ENV"], "a") as output:
        output.write(f"CCACHE_DIR={workspace / directory}\n")
        output.write("CCACHE_COMPILERCHECK=content\nCCACHE_MAXSIZE=500M\n")
    with open(os.environ["GITHUB_OUTPUT"], "a") as output:
        output.write(f"prefix={prefix}\nkey={key}\n")
    print(f"Core compiler cache: {prefix} (maximum 500 MB)")


if __name__ == "__main__":
    main()
