"""Exercise version identity through the real PEP 517 backends, without native compilation."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import unittest
import zipfile
from email.parser import Parser

ROOT = Path(__file__).resolve().parents[2]
BACKENDS = all(importlib.util.find_spec(name) is not None
               for name in ('build', 'hatchling', 'scikit_build_core'))


@unittest.skipUnless(BACKENDS, 'install build, hatchling and scikit-build-core')
class DistributionVersionTests(unittest.TestCase):
    def build(self, package, destination, kind, env):
        result = subprocess.run([sys.executable, '-m', 'build', '--no-isolation',
                                 f'--{kind}', '--outdir', str(destination), str(package)],
                                env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def wheel_version(self, path):
        with zipfile.ZipFile(path) as wheel:
            metadata = next(name for name in wheel.namelist() if name.endswith('.dist-info/METADATA'))
            return Parser().parsestr(wheel.read(metadata).decode())['Version']

    def check_package(self, component, preview=0):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            checkout = base / 'checkout'
            package = checkout / 'python' / component
            package.mkdir(parents=True)
            original = ROOT / 'python' / component
            shutil.copyfile(original / 'pyproject.toml', package / 'pyproject.toml')
            shutil.copytree(original / '_build_backend', package / '_build_backend')
            shutil.copyfile(ROOT / 'python/_build_support.py', checkout / 'python/_build_support.py')
            (package / 'README.md').write_text('Version contract test\n')
            module = package / 'src' / ('holder' if component == 'holder' else 'holderkit')
            module.mkdir(parents=True)
            (module / '__init__.py').write_text('')
            if component == 'kit':
                # Use the real Python backend/configuration with a tiny CMake project:
                # native compilation is already covered by Kit's own CI matrix.
                (package / 'CMakeLists.txt').write_text(
                    'cmake_minimum_required(VERSION 3.22)\nproject(version_test LANGUAGES NONE)\n')
            (checkout / 'VERSION').write_text('1.2.3\n')
            env = dict(os.environ, HOLDER_PYTHON_PREVIEW_NUMBER=str(preview))
            suffix = f'.post{preview}' if preview else ''
            outputs = base / 'original'
            self.build(package, outputs, 'sdist', env)
            self.build(package, outputs, 'wheel', env)
            self.assertEqual(self.wheel_version(next(outputs.glob('*.whl'))), '1.2.3' + suffix)
            # A root-only edit must override previously generated package metadata.
            (checkout / 'VERSION').write_text('1.2.4\n')
            self.build(package, base / 'bumped', 'wheel', env)
            self.assertEqual(self.wheel_version(next((base / 'bumped').glob('*.whl'))), '1.2.4' + suffix)
            exported = base / 'exported'
            with tarfile.open(next(outputs.glob('*.tar.gz'))) as archive:
                archive.extractall(exported, filter='data')
            shutil.rmtree(checkout)
            # No checkout and no preview environment: the archive retains its identity.
            env.pop('HOLDER_PYTHON_PREVIEW_NUMBER')
            self.build(next(exported.iterdir()), base / 'rebuilt', 'wheel', env)
            self.assertEqual(self.wheel_version(next((base / 'rebuilt').glob('*.whl'))), '1.2.3' + suffix)

    def test_holder_distributions_and_preview_rebuild(self):
        self.check_package('holder', preview=2)

    def test_kit_distributions_and_rebuild(self):
        self.check_package('kit')
