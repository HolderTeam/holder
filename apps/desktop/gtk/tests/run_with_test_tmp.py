"""Run one Meson test with a scratch tree owned by this invocation."""

import os
import pathlib
import shutil
import subprocess
import sys
import tempfile


def configure_macos_test_limits() -> None:
    import resource

    # VTE's macOS fdwalk allocates open_max * sizeof(proc_fdinfo) on the stack
    # between fork and exec. Large inherited limits (e.g. 1048575) overflow the
    # child's stack before it can execute even /bin/cat or report an exec error.
    # Bound only this test launcher's soft limit; preserve the hard limit and
    # any lower soft limit. 65536 descriptors need about 512 KiB of stack.
    soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
    if soft == resource.RLIM_INFINITY or soft > 65536:
        resource.setrlimit(resource.RLIMIT_NOFILE, (65536, hard))


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: run_with_test_tmp.py PROGRAM [ARGS...]", file=sys.stderr)
        return 2

    env = os.environ.copy()
    if sys.platform == "darwin":
        configure_macos_test_limits()
        runtime = env.get("HOLDER_DESKTOP_TEST_ACCESSIBLE_RUNTIME")
        if not runtime or not (pathlib.Path(runtime) / "lib/libgtk-4.1.dylib").is_file():
            print("macOS tests require a prepared AccessKit runtime. Run ./make.sh test "
                  "or set HOLDER_DESKTOP_TEST_ACCESSIBLE_RUNTIME to the output of "
                  "python3 scripts/accessible_runtime.py prepare.", file=sys.stderr)
            return 2
        sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "scripts"))
        from accessible_runtime import runtime_env

        # Apply DYLD settings at the native executable boundary.
        env = runtime_env(pathlib.Path(runtime), env)

    root = pathlib.Path(tempfile.mkdtemp(prefix="holder_gtk_test_"))
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
