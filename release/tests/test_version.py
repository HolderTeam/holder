import importlib.util
from pathlib import Path
import tempfile
import unittest
import shutil
import subprocess

spec = importlib.util.spec_from_file_location("version", Path(__file__).parents[1] / "scripts/version.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


class VersionTests(unittest.TestCase):
    def tree(self, directory, version="0.2.5"):
        root = Path(directory)
        (root / 'VERSION').write_text(version + '\n')
        for name in tool.PYPROJECTS:
            (root / name).parent.mkdir(parents=True, exist_ok=True)
            (root / name).write_text('[project]\nname = "x"\ndynamic = ["version"]\n\n[tool.x]\ntarget-version = "py310"\n')
        shutil.copyfile(Path(__file__).resolve().parents[2] / 'python/_build_support.py', root / 'python/_build_support.py')
        return root

    def test_dynamic_metadata_passes(self):
        with tempfile.TemporaryDirectory() as directory:
            tool.check(self.tree(directory))

    def test_static_package_version_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = self.tree(directory)
            (root / tool.PYPROJECTS[0]).write_text('[project]\nversion = "0.1.0"\n')
            with self.assertRaisesRegex(ValueError, 'derive its version dynamically'):
                tool.check(root)

    def test_write_changes_only_root_version(self):
        with tempfile.TemporaryDirectory() as directory:
            root = self.tree(directory, '0.2.1')
            before = {name: (root / name).read_bytes() for name in tool.PYPROJECTS}
            tool.write("0.2.6", root)
            self.assertEqual(tool.read(root), {'VERSION': '0.2.6'})
            self.assertEqual(before, {name: (root / name).read_bytes() for name in tool.PYPROJECTS})

    def test_write_rejects_other_version_forms(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):
                tool.write("0.2.5-rc.1", self.tree(directory))

    def test_exported_sources_get_the_root_version(self):
        with tempfile.TemporaryDirectory() as directory:
            root = self.tree(directory)
            for component in tool.SOURCE_COMPONENTS:
                exported = root / ('export-' + component)
                exported.mkdir()
                marker = ('meson.build' if component == 'desktop' else
                          'pyproject.toml' if component in ('kit', 'holder-python') else 'CMakeLists.txt')
                (exported / marker).touch()
                (exported / 'VERSION').write_text('0.1.0\n')
                tool.prepare_source(component, exported, root)
                self.assertEqual((exported / 'VERSION').read_text(), '0.2.5\n')


class NativeVersionTests(unittest.TestCase):
    ROOT = Path(__file__).resolve().parents[2]

    @unittest.skipUnless(shutil.which('cmake'), 'CMake is needed for the daemon version check')
    def test_daemon_root_version_and_standalone_fallback(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'daemon'
            source.mkdir()
            # Exercise the production version-selection/validation block without
            # configuring the daemon's unrelated native dependencies.
            prelude = (self.ROOT / 'daemon/CMakeLists.txt').read_text().split('# api/')[0]
            (source / 'CMakeLists.txt').write_text(prelude +
                'project(VersionProbe VERSION ${HOLDER_DAEMON_CMAKE_VERSION} LANGUAGES NONE)\n'
                'file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${HOLDER_DAEMON_VERSION}")\n')
            (root / 'VERSION').write_text('1.2.3\n')
            (source / 'VERSION').write_text('0.1.0\n')
            build = root / 'build'

            def configure():
                return subprocess.run(['cmake', '-S', str(source), '-B', str(build)],
                                      capture_output=True, text=True)

            result = configure()
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual((build / 'version.txt').read_text(), '1.2.3')
            (root / 'VERSION').write_text('1.2.4\n')
            subprocess.run(['cmake', '--build', str(build)], check=True, capture_output=True)
            self.assertEqual((build / 'version.txt').read_text(), '1.2.4')
            (root / 'VERSION').write_text('invalid\n')
            result = configure()
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('must contain a semantic version', result.stderr)
            (root / 'VERSION').unlink()
            for worktree in (False, True):
                with self.subTest(worktree=worktree):
                    marker = root / '.git'
                    if worktree:
                        marker.write_text('gitdir: /unused/worktree-metadata\n')
                    else:
                        marker.mkdir()
                    try:
                        result = configure()
                        self.assertNotEqual(result.returncode, 0, result.stdout)
                        self.assertIn('Missing repository-root VERSION', result.stderr)
                    finally:
                        if worktree:
                            marker.unlink()
                        else:
                            marker.rmdir()
            # Exported sources without Git metadata still use their generated input.
            result = configure()
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual((build / 'version.txt').read_text(), '0.1.0')
            (source / 'VERSION').unlink()
            result = configure()
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('prepare the standalone daemon source', result.stderr)

    def test_desktop_root_version_wins_and_export_requires_prepared_metadata(self):
        spec = importlib.util.spec_from_file_location('product_version', self.ROOT / 'apps/desktop/gtk/scripts/product_version.py')
        reader = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(reader)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'apps/desktop/gtk'
            source.mkdir(parents=True)
            (source / 'VERSION').write_text('0.1.0\n')
            (root / 'VERSION').write_text('0.2.5\n')
            self.assertEqual(reader.read(source), '0.2.5')
            (root / 'VERSION').write_text('0.2.6\n')
            self.assertEqual(reader.read(source), '0.2.6')
            (root / 'VERSION').unlink()
            self.assertEqual(reader.read(source), '0.1.0')
            (root / '.git').mkdir()
            with self.assertRaisesRegex(ValueError, 'repository-root VERSION'):
                reader.read(source)
            (root / '.git').rmdir()
            (source / 'VERSION').unlink()
            with self.assertRaisesRegex(ValueError, 'Missing VERSION'):
                reader.read(source)

    @unittest.skipUnless(shutil.which('cmake'), 'CMake is needed for the native version check')
    def test_core_reconfigures_after_a_root_only_version_change(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'core'
            (source / 'cmake').mkdir(parents=True)
            shutil.copyfile(self.ROOT / 'core/cmake/HolderVersion.cmake', source / 'cmake/HolderVersion.cmake')
            (root / 'VERSION').write_text('0.2.5\n')
            (source / 'VERSION').write_text('0.1.0\n')
            (source / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.22)\ninclude(cmake/HolderVersion.cmake)\n'
                'project(VersionProbe VERSION ${HOLDER_CORE_CMAKE_VERSION} LANGUAGES NONE)\n'
                'file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${HOLDER_CORE_VERSION}")\n')
            build = root / 'build'
            def run(*args):
                subprocess.run(['cmake', *map(str, args)], check=True, capture_output=True)
            run('-S', source, '-B', build)
            self.assertEqual((build / 'version.txt').read_text(), '0.2.5')
            (root / 'VERSION').write_text('0.2.6\n')
            run('--build', build)
            self.assertEqual((build / 'version.txt').read_text(), '0.2.6')
            (root / 'VERSION').unlink()
            run('-S', source, '-B', build)
            self.assertEqual((build / 'version.txt').read_text(), '0.1.0')
            (root / '.git').mkdir()
            with self.assertRaises(subprocess.CalledProcessError):
                run('-S', source, '-B', build)

    @unittest.skipUnless(shutil.which('meson') and shutil.which('valac'), 'Meson/Vala are needed for the desktop version check')
    def test_meson_reconfigures_after_a_root_only_version_change(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'apps/desktop/gtk'
            (source / 'scripts').mkdir(parents=True)
            shutil.copyfile(self.ROOT / 'apps/desktop/gtk/scripts/product_version.py', source / 'scripts/product_version.py')
            # Exercise the real project-version declaration and tracked input, without
            # requiring all of the GUI dependencies in the repository-check job.
            lines = (self.ROOT / 'apps/desktop/gtk/meson.build').read_text().split("gnome = import('gnome')")[0]
            (source / 'meson.build').write_text(lines)
            (root / 'VERSION').write_text('1.2.3\n')
            build = root / 'build'
            subprocess.run(['meson', 'setup', str(build), str(source)], check=True, capture_output=True)
            self.assertEqual((build / 'product-version.txt').read_text(), '1.2.3\n')
            (root / 'VERSION').write_text('1.2.4\n')
            subprocess.run(['meson', 'compile', '-C', str(build)], check=True, capture_output=True)
            import json
            info = json.loads(subprocess.check_output(['meson', 'introspect', '--projectinfo', str(build)]))
            self.assertEqual(info['version'], '1.2.4')


if __name__ == "__main__":
    unittest.main()
