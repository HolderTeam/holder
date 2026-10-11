"""Exercise make.sh's core selection and default flow without compiling or running a Home."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

REPOSITORY = Path(__file__).resolve().parents[1]

FAKE_COMMAND = '''import json, os, sys
from pathlib import Path
name = Path(sys.argv[0]).name
args = sys.argv[1:]
if name == "git":
    sys.exit(1)
if name == "uname":
    print("MINGW64_NT" if os.environ.get("MAKE_TEST_WINDOWS") else "Linux")
    sys.exit(0)
with open(os.environ["MAKE_TEST_LOG"], "a") as log:
    log.write(json.dumps([name, *args]) + "\\n")
if name == "cmake" and ("-S" in args or "--preset" in args):
    if "--preset" in args:
        build = Path("out/build") / args[args.index("--preset") + 1]
        executable = build / "holderd.exe"
    else:
        build = Path(args[args.index("-B") + 1])
        executable = build / "holderd"
    build.mkdir(parents=True, exist_ok=True)
    executable.write_text(Path(os.environ["MAKE_TEST_COMMAND"]).read_text())
    executable.chmod(0o755)
'''


class MakeTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="holder make test ")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.repo = self.root / "holder" / "daemon"
        self.repo.mkdir(parents=True)
        shutil.copy2(REPOSITORY / "make.sh", self.repo / "make.sh")
        caste = self.repo / "submodules/caste"
        caste.mkdir(parents=True)
        (caste / "CMakeLists.txt").write_text("# ready")
        self.core = self.root / "holder" / "core"
        self.core.mkdir()
        (self.core / "CMakeLists.txt").write_text("# core fixture")
        commands = self.root / "bin"
        commands.mkdir()
        command = f"#!{sys.executable}\n" + FAKE_COMMAND
        self.command_file = self.root / "command.py"
        self.command_file.write_text(command)
        for name in ("git", "uname", "cmake", "ctest", "valgrind", "lcov",
                     "genhtml", "gcovr", "clang-tidy-18", "run-clang-tidy"):
            path = commands / name
            path.write_text(command)
            path.chmod(0o755)
        self.log = self.root / "commands.jsonl"
        self.sdk = self.root / "installed sdk"
        self.env = {key: value for key, value in os.environ.items()
                    if not key.startswith(("HOLDER_", "MAKE_TEST_"))}
        self.env.update(PATH=str(commands) + os.pathsep + os.environ["PATH"],
                        HOLDER_CCACHE="0", CMAKE_BUILD_PARALLEL_LEVEL="2",
                        MAKE_TEST_LOG=str(self.log), MAKE_TEST_COMMAND=str(self.command_file))
        self.env.pop("OS", None)

    def run_make(self, *args, **environment):
        return subprocess.run(["bash", "./make.sh", *args], cwd=self.repo,
                              env=self.env | environment, text=True, capture_output=True)

    def calls(self):
        return [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []

    def require_success(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def configure_call(self):
        return next(call for call in self.calls() if call[0] == "cmake" and "--build" not in call)

    def test_no_arguments_builds_core_from_this_checkout_tests_and_runs(self):
        self.require_success(self.run_make())
        self.assertEqual([call[0] for call in self.calls()], ["cmake", "cmake", "ctest", "holderd"])
        configure = self.configure_call()
        self.assertIn("-DHOLDER_CORE_SOURCE_DIR=" + str(self.core), configure)
        self.assertIn("-DHOLDER_CORE_SDK=", configure)
        self.assertIn("-DCMAKE_BUILD_TYPE=RelWithDebInfo", configure)
        self.assertEqual(self.calls()[1][-1], "2")

    def test_explicit_sdk_source_and_system_modes_replace_the_default(self):
        for environment, expected in (
            ({"HOLDER_CORE_SDK": str(self.sdk)}, "-DHOLDER_CORE_SDK=" + str(self.sdk)),
            ({"HOLDER_CORE_SOURCE_DIR": "../other core"}, "-DHOLDER_CORE_SOURCE_DIR=../other core"),
            ({"HOLDER_USE_SYSTEM_CORE": "ON"}, "-DHOLDER_USE_SYSTEM_CORE=ON"),
        ):
            with self.subTest(environment=environment):
                self.log.unlink(missing_ok=True)
                self.require_success(self.run_make("build", **environment))
                configure = self.configure_call()
                self.assertIn(expected, configure)
                if "HOLDER_CORE_SOURCE_DIR" not in environment:
                    self.assertIn("-DHOLDER_CORE_SOURCE_DIR=", configure)
        self.assertNotEqual(self.run_make("build", HOLDER_CORE_SOURCE_DIR="../core",
                                         HOLDER_USE_SYSTEM_CORE="ON").returncode, 0)

    def test_any_build_type_works_from_source(self):
        self.require_success(self.run_make("test", "Debug"))
        self.assertIn("-DCMAKE_BUILD_TYPE=Debug", self.configure_call())

    def test_diagnostic_commands_default_to_debug_from_source_and_sdk_configuration_otherwise(self):
        for environment, build_type in (({}, "Debug"), ({"HOLDER_CORE_SDK": str(self.sdk)}, "RelWithDebInfo")):
            for args in (("warnings",), ("san",), ("coverage",), ("memcheck",), ("tidy",)):
                with self.subTest(args=args, environment=environment):
                    self.log.unlink(missing_ok=True)
                    self.require_success(self.run_make(*args, **environment))
                    self.assertIn("-DCMAKE_BUILD_TYPE=" + build_type, self.configure_call())

    def test_missing_core_fails_before_configure(self):
        shutil.rmtree(self.core)
        result = self.run_make("build")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Missing ../core", result.stderr)
        self.assertEqual(self.calls(), [])
        self.require_success(self.run_make("build", HOLDER_CORE_SDK=str(self.sdk)))

    def test_windows_builds_core_from_source_unless_an_sdk_is_named(self):
        windows = {"MAKE_TEST_WINDOWS": "1", "VCPKG_ROOT": str(self.root / "vcpkg")}
        self.require_success(self.run_make(**windows))
        self.assertEqual(self.calls()[-1][0], "holderd.exe")
        self.assertIn("windows-vcpkg-tests-debug", self.configure_call())
        self.log.unlink()
        self.require_success(self.run_make("build", "Release", HOLDER_CORE_SDK=str(self.sdk), **windows))
        self.assertIn("windows-sdk-tests", self.configure_call())
        self.assertIn("-DCMAKE_BUILD_TYPE=Release", self.configure_call())

    def test_help_does_not_prepare_dependencies(self):
        shutil.rmtree(self.repo / "submodules")
        self.require_success(self.run_make("help"))
        self.assertEqual(self.calls(), [])


if __name__ == "__main__":
    unittest.main()
