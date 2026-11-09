"""Exercise the production Gradle version script without compiling the app or core."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ANDROID = Path(__file__).resolve().parents[1]


class ProductVersionTests(unittest.TestCase):
    def test_root_changes_cache_invalidation_and_invalid_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            project = root / 'apps/mobile/android'
            (project / 'gradle').mkdir(parents=True)
            shutil.copyfile(ANDROID / 'gradle/product-version.gradle.kts',
                            project / 'gradle/product-version.gradle.kts')
            (project / 'settings.gradle.kts').write_text('rootProject.name = "version-test"\n')
            (project / 'build.gradle.kts').write_text('''
apply(from = "gradle/product-version.gradle.kts")
val productVersion = extra["holderProductVersion"] as String
val productCode = extra["holderProductVersionCode"] as Int
tasks.register("versionIdentity") {
    inputs.property("version", productVersion)
    inputs.property("code", productCode)
    val identity = "PRODUCT=$productVersion CODE=$productCode"
    doLast { println(identity) }
}
''')

            def run(version):
                if version is None:
                    (root / 'VERSION').unlink()
                else:
                    (root / 'VERSION').write_text(version + '\n')
                return subprocess.run(
                    [str(ANDROID / ('gradlew.bat' if os.name == 'nt' else 'gradlew')),
                     '-p', str(project), '--console=plain',
                     '--configuration-cache', 'versionIdentity'],
                    capture_output=True, text=True)

            # Reuse the same project/configuration cache; edit only root VERSION.
            cases = [('0.2.6', 2006), ('0.2.6', 2006), ('0.2.7', 2007),
                                  ('0.2.999', 2999), ('0.3.0', 3000),
                                  ('1.0.0', 1000000), ('2099.999.999', 2099999999)]
            for index, (version, code) in enumerate(cases):
                with self.subTest(version=version):
                    result = run(version)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertIn(f'PRODUCT={version} CODE={code}', result.stdout)
                    if index == 1:
                        self.assertIn('Reusing configuration cache', result.stdout)
            for version in ('0.2.6-dev', 'invalid', '0.1000.0', '0.2.1000', '2100.0.0',
                            '999999999999999.0.0', '0.0.0', None):
                with self.subTest(version=version):
                    result = run(version)
                    self.assertNotEqual(result.returncode, 0, result.stdout)
                    self.assertIn('VERSION', result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
