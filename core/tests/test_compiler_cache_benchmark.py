"""The manual cache benchmark must leave source files intact when a build fails."""

import importlib.util
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/benchmark-compiler-cache.py"
spec = importlib.util.spec_from_file_location("cache_benchmark", SCRIPT)
benchmark = importlib.util.module_from_spec(spec)
spec.loader.exec_module(benchmark)


class BenchmarkSafetyTests(unittest.TestCase):
    def test_rejects_a_shared_cache_before_running_commands(self):
        with (
            patch.dict(os.environ, {"CCACHE_DIR": "/tmp/shared-cache"}),
            patch("sys.argv", [str(SCRIPT), "build"]),
            patch.object(benchmark, "run") as run,
            self.assertRaises(SystemExit),
        ):
            benchmark.main()
        run.assert_not_called()

    def test_restores_the_compiled_source_after_a_failed_warm_build(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "src").mkdir()
            source = root / "src/built.cpp"
            original = b"int value() { return 1; }\n"
            source.write_bytes(original)
            unused = root / "src/not-in-this-platform.cpp"
            unused.write_bytes(b"")
            build = root / "build"
            build.mkdir()
            (build / "compile_commands.json").write_text(
                json.dumps([{"file": str(source)}])
            )

            def compile_build(directory):
                if source.read_bytes() != original:
                    self.assertIn(b"static_assert", source.read_bytes())
                    raise subprocess.CalledProcessError(1, "cmake")
                return {"seconds": 1, "hits": 0, "misses": 1}

            with (
                patch.dict(
                    os.environ, {"CCACHE_DIR": str(root / ".core-ccache-measure")}
                ),
                patch("sys.argv", [str(SCRIPT), str(build)]),
                patch.object(benchmark, "__file__", str(root / "scripts/benchmark.py")),
                patch.object(benchmark, "run"),
                patch.object(benchmark, "build", side_effect=compile_build),
                self.assertRaises(subprocess.CalledProcessError),
            ):
                benchmark.main()
            self.assertEqual(source.read_bytes(), original)
            self.assertEqual(unused.read_bytes(), b"")


if __name__ == "__main__":
    unittest.main()
