"""Version identity across checkout builds, prepared sources and previews."""
import importlib.util
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('build_support', ROOT / 'python/_build_support.py')
support = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(support)


class PythonVersionTests(unittest.TestCase):
    def test_root_only_changes_override_stale_generated_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'python').mkdir()
            shutil.copyfile(ROOT / 'python/_build_support.py', root / 'python/_build_support.py')
            package = root / 'python/holder'
            package.mkdir()
            (root / 'VERSION').write_text('1.2.3\n')
            (package / 'VERSION').write_text('0.0.1\n')
            self.assertEqual(support.read_version(package), '1.2.3')
            (root / 'VERSION').write_text('1.2.4\n')
            self.assertEqual(support.read_version(package), '1.2.4')
            self.assertEqual(support.read_version(package, preview=True,
                env={'HOLDER_PYTHON_PREVIEW_NUMBER': '2'}), '1.2.4.post2')
            self.assertEqual((root / 'VERSION').read_text(), '1.2.4\n')
            (root / 'VERSION').unlink()
            with self.assertRaisesRegex(ValueError, 'Missing version input'):
                support.read_version(package)

    def test_standalone_metadata_is_self_contained_and_consistent(self):
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory)
            with self.assertRaisesRegex(ValueError, 'Missing version input'):
                support.read_version(package)
            (package / 'VERSION').write_text('1.2.3.post2\n')
            (package / 'PKG-INFO').write_text('Metadata-Version: 2.3\nVersion: 1.2.3.post2\n')
            self.assertEqual(support.read_version(package, preview=True,
                env={'HOLDER_PYTHON_PREVIEW_NUMBER': '2'}), '1.2.3.post2')
            with self.assertRaisesRegex(ValueError, 'Preview number differs'):
                support.read_version(package, preview=True, env={'HOLDER_PYTHON_PREVIEW_NUMBER': '3'})
            (package / 'VERSION').write_text('9.9.9\n')
            with self.assertRaisesRegex(ValueError, 'source-distribution metadata'):
                support.read_version(package)
