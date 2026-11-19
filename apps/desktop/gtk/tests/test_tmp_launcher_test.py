"""Check macOS subprocess limits without changing the test runner's limits."""

import json
from pathlib import Path
import subprocess
import sys
import unittest


LAUNCHER = Path(__file__).with_name('run_with_test_tmp.py')


@unittest.skipUnless(sys.platform == 'darwin', 'VTE fdwalk workaround is macOS-only')
class MacosTestLimitsTests(unittest.TestCase):
    def probe(self, soft):
        script = '''
import importlib.util
import json
import resource
import sys

spec = importlib.util.spec_from_file_location('launcher', sys.argv[1])
launcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(launcher)
_, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
soft = int(sys.argv[2])
if hard != resource.RLIM_INFINITY and soft > hard:
    print(json.dumps({'unsupported': 'Inherited hard limit is below the test input'}))
    sys.exit(0)
try:
    resource.setrlimit(resource.RLIMIT_NOFILE, (soft, hard))
except (OSError, ValueError) as error:
    print(json.dumps({'unsupported': str(error)}))
    sys.exit(0)
launcher.configure_macos_test_limits()
actual_soft, actual_hard = resource.getrlimit(resource.RLIMIT_NOFILE)
print(json.dumps({'soft': actual_soft, 'hard_preserved': actual_hard == hard}))
'''
        result = json.loads(subprocess.check_output(
            [sys.executable, '-c', script, str(LAUNCHER), str(soft)], text=True))
        if result.get('unsupported'):
            self.skipTest(result['unsupported'])
        return result

    def test_large_inherited_limit_is_bounded(self):
        result = self.probe(1048575)
        self.assertEqual(result, {'soft': 65536, 'hard_preserved': True})

    def test_lower_inherited_limit_is_preserved(self):
        result = self.probe(1024)
        self.assertEqual(result, {'soft': 1024, 'hard_preserved': True})

    def test_boundary_limit_is_preserved(self):
        result = self.probe(65536)
        self.assertEqual(result, {'soft': 65536, 'hard_preserved': True})


if __name__ == '__main__':
    unittest.main()
