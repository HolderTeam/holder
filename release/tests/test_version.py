import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("version", Path(__file__).parents[1] / "scripts/version.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


class VersionTests(unittest.TestCase):
    def tree(self, directory, versions=("0.2.5",) * 5):
        root = Path(directory)
        for name, version in zip(tool.VERSION_FILES, versions):
            (root / name).parent.mkdir(parents=True, exist_ok=True)
            (root / name).write_text(version + "\n")
        for name, version in zip(tool.PYPROJECTS, versions[len(tool.VERSION_FILES):]):
            (root / name).parent.mkdir(parents=True, exist_ok=True)
            (root / name).write_text(f'[project]\nname = "x"\nversion = "{version}"\n\n[tool.x]\ntarget-version = "py310"\n')
        return root

    def test_matching_copies_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(tool.mismatches(self.tree(directory)), {})

    def test_a_differing_copy_is_reported(self):
        with tempfile.TemporaryDirectory() as directory:
            root = self.tree(directory, ("0.2.5", "0.2.4", "0.2.5", "0.2.5", "0.0.2"))
            self.assertEqual(tool.mismatches(root), {"core/VERSION": "0.2.4", "python/holder/pyproject.toml": "0.0.2"})

    def test_write_sets_every_copy_and_only_the_project_version(self):
        with tempfile.TemporaryDirectory() as directory:
            root = self.tree(directory, ("0.2.1", "0.2.4", "0.2.1", "0.0.2", "0.1.0"))
            tool.write("0.2.6", root)
            self.assertEqual(set(tool.read(root).values()), {"0.2.6"})
            self.assertIn('target-version = "py310"', (root / tool.PYPROJECTS[0]).read_text())

    def test_write_rejects_other_version_forms(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):
                tool.write("0.2.5-rc.1", self.tree(directory))


if __name__ == "__main__":
    unittest.main()
