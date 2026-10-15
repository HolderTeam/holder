"""Cache lifecycle checks; native loaded-library verification lives in the selector."""
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location(
    'runtime', Path(__file__).resolve().parents[1] / 'scripts/accessible_runtime.py')
runtime = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(runtime)


@unittest.skipUnless(os.name == 'posix', 'macOS runtime cache uses POSIX file locking')
class RuntimeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.current = {'dependencies': {'gtk4': {'version': '4.24.1'}}, 'consumer': 'one'}
        self.builds = 0
        patches = [
            patch.dict(os.environ, HOLDER_ACCESS_CACHE=self.tmp.name),
            patch.object(runtime.sys, 'platform', 'darwin'),
            patch.object(runtime, 'identity', side_effect=lambda: (self.current.copy(), {})),
            patch.object(runtime, 'verify', return_value='GTK 4.24.1\n/test/libgtk.dylib'),
            patch.object(runtime, 'output', return_value=''),
            patch.object(runtime.subprocess, 'run', side_effect=self.build),
        ]
        for item in patches:
            item.start()
            self.addCleanup(item.stop)

    def build(self, args, **kwargs):
        if args[0] == 'bash':
            self.builds += 1
            lib = Path(args[-1]) / 'runtime/lib'
            lib.mkdir(parents=True)
            (lib / 'libgtk-4.1.dylib').write_bytes(b'gtk')
        else:
            Path(args[args.index('-o') + 1]).write_bytes(b'probe')

    def test_first_use_and_reuse(self):
        first = runtime.select_runtime()
        self.assertEqual(first, runtime.select_runtime())
        self.assertEqual(self.builds, 1)
        self.assertTrue((first.parent / 'identity.json').exists())

    def test_consumer_upgrade_prepares_a_different_runtime(self):
        first = runtime.select_runtime()
        self.current['consumer'] = 'two'
        self.assertNotEqual(first, runtime.select_runtime())
        self.assertEqual(self.builds, 2)

    def test_modified_runtime_rejected(self):
        first = runtime.select_runtime()
        (first / 'lib/libgtk-4.1.dylib').write_bytes(b'changed')
        with self.assertRaisesRegex(RuntimeError, 'modified or damaged'):
            runtime.select_runtime()

    def test_incomplete_runtime_rejected(self):
        first = runtime.select_runtime()
        (first.parent / 'identity.json').unlink()
        with self.assertRaisesRegex(RuntimeError, 'Incomplete runtime'):
            runtime.select_runtime()

    def test_linux_uses_atspi_without_preparing_macos_runtime(self):
        with patch.object(runtime.sys, 'platform', 'linux'), patch.object(runtime, 'select_runtime') as select:
            env = runtime.launch_environment({'DISPLAY': ':1', 'GTK_A11Y': 'none'})
        self.assertEqual(env, {'DISPLAY': ':1', 'GTK_A11Y': 'atspi', 'NO_AT_BRIDGE': '0'})
        select.assert_not_called()

    def test_launch_environment_preserves_app_settings(self):
        env = runtime.runtime_env(Path('/cache/runtime'), {'GSETTINGS_SCHEMA_DIR': '/schemas'})
        self.assertEqual(env['DYLD_LIBRARY_PATH'], '/cache/runtime/lib')
        self.assertEqual(env['GTK_A11Y'], 'accesskit')
        self.assertEqual(env['GSETTINGS_SCHEMA_DIR'], '/schemas')


class IdentityTests(unittest.TestCase):
    def test_missing_pkgconfig_prefix_is_independent_of_working_directory(self):
        formula = {'formulae': [{'versions': {'stable': '4.24.1'},
                                'urls': {'stable': {'url': 'source', 'checksum': 'sha'}}}]}

        def pkg_output(*args):
            if args[0] == 'pkg-config':
                return '4.24.1' if args[1] == '--modversion' else ''
            return 'tool-version'

        with patch.object(runtime, 'output', side_effect=pkg_output), patch.object(
            runtime, 'output_with_env', return_value=json.dumps(formula)
        ), tempfile.TemporaryDirectory() as directory:
            previous = Path.cwd()
            try:
                first = runtime.identity()[0]
                os.chdir(directory)
                self.assertEqual(first, runtime.identity()[0])
                self.assertEqual(first['dependencies']['gtk4']['prefix'], '')
            finally:
                os.chdir(previous)


if __name__ == '__main__':
    unittest.main()
