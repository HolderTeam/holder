import importlib.util
from pathlib import Path
import unittest
from unittest import mock
import urllib.error
from pathlib import Path
import subprocess
import tempfile

spec = importlib.util.spec_from_file_location("launchpad_orig", Path(__file__).parents[1] / "scripts/launchpad-orig.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


class Response:
    def __init__(self, data):
        self.data = data

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False

    def read(self):
        return self.data


def http_error(code):
    return urllib.error.HTTPError("https://api.launchpad.net/x", code, "error", {}, None)


class FetchTests(unittest.TestCase):
    def fetch(self, outcomes, attempts=5):
        waits = []
        with mock.patch.object(tool.urllib.request, "urlopen", side_effect=outcomes) as urlopen:
            try:
                return tool.fetch("https://api.launchpad.net/x", attempts=attempts, sleep=waits.append), waits
            finally:
                self.calls = urlopen.call_count

    def test_retries_a_bad_gateway(self):
        data, waits = self.fetch([http_error(502), http_error(503), Response(b"ok")])
        self.assertEqual((data, waits, self.calls), (b"ok", [10, 20], 3))

    def test_retries_a_dropped_connection(self):
        data, _ = self.fetch([urllib.error.URLError("reset"), Response(b"ok")])
        self.assertEqual(data, b"ok")

    def test_does_not_retry_a_missing_page(self):
        with self.assertRaises(urllib.error.HTTPError):
            self.fetch([http_error(404), Response(b"ok")])
        self.assertEqual(self.calls, 1)

    def test_gives_up_after_the_last_attempt(self):
        with self.assertRaises(urllib.error.HTTPError):
            self.fetch([http_error(502)] * 3, attempts=3)
        self.assertEqual(self.calls, 3)


class SourceArchiveTests(unittest.TestCase):
    def test_core_export_injects_the_committed_root_version_deterministically(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            def git(*args):
                return subprocess.run(['git', '-C', str(root), *args], check=True, capture_output=True)
            git('init')
            git('config', 'user.name', 'Archive test')
            git('config', 'user.email', 'test@example.invalid')
            (root / 'core').mkdir()
            (root / 'core/CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.22)\n')
            (root / 'VERSION').write_text('1.2.3\n')
            git('add', '.')
            git('commit', '-m', 'Fixture')
            # Exports follow the committed source, not a dirty working-tree version.
            (root / 'VERSION').write_text('9.9.9\n')
            original_run = subprocess.run
            def run(args, **kwargs):
                return original_run(args, cwd=root, **kwargs)
            with mock.patch.object(tool.subprocess, 'run', side_effect=run):
                data = tool.git_archive('core', 'holder-core-1.2.3/')
                self.assertEqual(data, tool.git_archive('core', 'holder-core-1.2.3/'))
            members = tool.contents(data)
            self.assertEqual(members['holder-core-1.2.3/VERSION'][2], b'1.2.3\n')
            self.assertIn('holder-core-1.2.3/CMakeLists.txt', members)


if __name__ == "__main__":
    unittest.main()
