import importlib.util
from pathlib import Path
import unittest
from unittest import mock
import urllib.error

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


if __name__ == "__main__":
    unittest.main()
