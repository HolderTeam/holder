"""A root-only version bump must not retrieve the old tree's package bundle."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


class VersionedPackageShelfTests(unittest.TestCase):
    def test_version_only_change_uses_a_distinct_package_shelf(self):
        script = Path(__file__).resolve().parents[2] / '.github/scripts/fetch-core-debs.sh'
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            fakebin = root / 'bin'
            fakebin.mkdir()
            curl = fakebin / 'curl'
            curl.write_text('''#!/usr/bin/env python3
import os,pathlib,shutil,sys
args=sys.argv[1:]
tag,name=args[-1].rsplit('/',2)[-2:]
source=pathlib.Path(os.environ['TEST_SHELF'])/tag/name
if not source.is_file():sys.exit(22)
shutil.copyfile(source,args[args.index('-o')+1])
''')
            curl.chmod(0o755)
            tree = 'a' * 40
            shelf = root / ('core-' + tree + '-1.2.3')
            shelf.mkdir()
            name, data = 'libholder0_1.2.3_amd64.deb', b'fixture-package'
            (shelf / name).write_bytes(data)
            (shelf / 'libholder-noble.sha256').write_text(hashlib.sha256(data).hexdigest() + '  ' + name + '\n')
            env = dict(os.environ, TEST_SHELF=str(root), PATH=str(fakebin) + os.pathsep + os.environ['PATH'], GITHUB_OUTPUT=str(root / 'result'))
            subprocess.run(['bash', str(script), tree, 'noble', str(root / 'old'), '1.2.3'], env=env, check=True, capture_output=True)
            self.assertIn('found=true', (root / 'result').read_text())
            (root / 'result').write_text('')
            subprocess.run(['bash', str(script), tree, 'noble', str(root / 'new'), '1.2.4'], env=env, check=True, capture_output=True)
            self.assertIn('found=false', (root / 'result').read_text())
            self.assertEqual(list((root / 'new').iterdir()), [])
