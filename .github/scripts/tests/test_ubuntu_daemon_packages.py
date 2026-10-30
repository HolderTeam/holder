"""Exercise package reuse without a network, compiler or Ubuntu host."""
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPTS = Path(__file__).resolve().parents[1]
KEY = SCRIPTS / "ubuntu-daemon-package-key.sh"
FETCH = SCRIPTS / "fetch-daemon-debs.sh"


class PackageKeyTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = Path(self.tmp.name)
        self.env = {**os.environ, "GIT_CONFIG_NOSYSTEM": "1", "GIT_CONFIG_GLOBAL": "/dev/null"}
        self.git("init", "-q")
        self.git("config", "user.name", "Package test")
        self.git("config", "user.email", "test@example.invalid")
        self.inputs = [
            "core/input", "daemon/input", "api/input", "common/input", "cli/input",
            "VERSION", ".gitmodules", ".github/workflows/daemon-ubuntu-packages.yml",
            ".github/actions/core-ubuntu-packages/action.yml",
            ".github/scripts/fetch-core-debs.sh",
            ".github/scripts/ubuntu-daemon-package-key.sh",
            ".github/scripts/fetch-daemon-debs.sh",
        ]
        for name in self.inputs:
            self.write(name, "original\n")
        self.commit()

    def git(self, *args):
        return subprocess.run(["git", *args], cwd=self.repo, env=self.env,
                              check=True, capture_output=True, text=True).stdout.strip()

    def write(self, name, content):
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)

    def commit(self):
        self.git("add", ".")
        self.git("commit", "-qm", "fixture")

    def key(self, series="resolute"):
        return subprocess.run([str(KEY), series], cwd=self.repo, env=self.env,
                              check=True, capture_output=True, text=True).stdout.strip()

    def test_desktop_and_test_only_commits_reuse_backend(self):
        original = self.key()
        self.write("apps/desktop/gtk/integration/test.feature", "changed\n")
        self.commit()
        self.assertEqual(original, self.key())

    def test_each_source_and_recipe_input_invalidates_bundle(self):
        previous = self.key()
        for name in self.inputs:
            with self.subTest(name=name):
                self.write(name, "changed\n")
                self.commit()
                current = self.key()
                self.assertNotEqual(previous, current)
                previous = current

    def test_ubuntu_series_have_distinct_keys(self):
        self.assertNotEqual(self.key("noble"), self.key("resolute"))

    def test_unsupported_series_fails(self):
        result = subprocess.run([str(KEY), "unknown"], cwd=self.repo, capture_output=True)
        self.assertNotEqual(result.returncode, 0)


class FetchTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.shelf = self.root / "shelf"
        self.shelf.mkdir()
        self.dest = self.root / "download"
        self.output = self.root / "output"
        self.bin = self.root / "bin"
        self.bin.mkdir()
        curl = self.bin / "curl"
        curl.write_text(f"#!{sys.executable}\n" + '''import os, pathlib, shutil, sys
args = sys.argv[1:]
source = pathlib.Path(os.environ["TEST_SHELF"]) / args[-1].rsplit("/", 1)[-1]
if not source.exists():
    sys.exit(22)
shutil.copyfile(source, args[args.index("-o") + 1])
''')
        curl.chmod(0o755)
        self.env = {**os.environ, "PATH": f"{self.bin}:{os.environ['PATH']}",
                    "TEST_SHELF": str(self.shelf), "GITHUB_OUTPUT": str(self.output)}

    def manifest(self, packages):
        lines = []
        for name, data in packages.items():
            (self.shelf / name).write_bytes(data)
            lines.append(f"{hashlib.sha256(data).hexdigest()}  {name}\n")
        (self.shelf / "packages.sha256").write_text("".join(lines))

    def fetch(self):
        return subprocess.run([str(FETCH), "fixture-key", str(self.dest)], env=self.env,
                              capture_output=True, text=True)

    def test_missing_bundle_is_a_cache_miss(self):
        self.assertEqual(self.fetch().returncode, 0)
        self.assertEqual(self.output.read_text(), "found=false\n")
        self.assertFalse((self.dest / "packages.sha256").exists())

    def test_complete_bundle_preserves_exact_packages(self):
        packages = {"holder-daemon_1_amd64.deb": b"daemon", "libholder0_1_amd64.deb": b"core",
                    "libcaste0_1_amd64.deb": b"caste"}
        self.manifest(packages)
        result = self.fetch()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.output.read_text(), "found=true\n")
        for name, content in packages.items():
            self.assertEqual((self.dest / name).read_bytes(), content)

    def test_corrupt_package_fails_instead_of_rebuilding(self):
        self.manifest({"holder-daemon_1_amd64.deb": b"original"})
        (self.shelf / "holder-daemon_1_amd64.deb").write_bytes(b"corrupted")
        self.assertNotEqual(self.fetch().returncode, 0)
        self.assertFalse(self.output.exists())

    def test_incomplete_published_bundle_fails(self):
        self.manifest({"holder-daemon_1_amd64.deb": b"daemon"})
        (self.shelf / "holder-daemon_1_amd64.deb").unlink()
        self.assertNotEqual(self.fetch().returncode, 0)

    def test_manifest_cannot_write_outside_bundle(self):
        (self.shelf / "packages.sha256").write_text(f"{'0' * 64}  ../outside.deb\n")
        self.assertNotEqual(self.fetch().returncode, 0)
        self.assertFalse((self.root / "outside.deb").exists())

    def test_empty_manifest_fails(self):
        (self.shelf / "packages.sha256").write_text("")
        self.assertNotEqual(self.fetch().returncode, 0)


if __name__ == "__main__":
    unittest.main()
