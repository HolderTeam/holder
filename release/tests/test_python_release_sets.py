import base64
import copy
from datetime import datetime
import hashlib
import io
import json
import os
from pathlib import Path
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch
import zipfile

import test_release_coordinator as preparation
from test_release_coordinator import load, FakeGitHub, CANDIDATE, MANIFEST, NOW, REPOSITORY, tool

packages = tool.package_tool
sdk = load('fetch_candidate_sdk', 'release/scripts/fetch-candidate-sdk.py')
fixtures = load('package_manifest_fixtures', 'release/staging/tests/test_release_manifest.py')


def tar_bytes(files):
    stream = io.BytesIO()
    with tarfile.open(fileobj=stream, mode='w:gz') as archive:
        for name, data in files.items():
            data = data.encode() if isinstance(data, str) else data
            entry = tarfile.TarInfo(name)
            entry.size = len(data)
            archive.addfile(entry, io.BytesIO(data))
    return stream.getvalue()


def distribution_files(package, selection):
    result = {}
    name = 'holder-kit' if package == 'kit' else 'holder-python'
    stem = name.replace('-', '_') + '-0.2.6'
    metadata = f'Name: {name}\nVersion: 0.2.6\nLicense-File: third-party/test.txt\n\nTest package\n'
    for filename in packages.filenames(package, '0.2.6'):
        if filename.endswith('.whl'):
            output = io.BytesIO()
            with zipfile.ZipFile(output, 'w') as archive:
                archive.writestr(stem + '.dist-info/METADATA', metadata)
                if package == 'kit':
                    platform = packages.PLATFORMS[filename.removesuffix('.whl').split('-')[-1]]
                    core = next(asset for asset in selection['assets'] if asset['platform'] == platform)
                    archive.writestr('holderkit/_core_build.json', json.dumps(core))
            result[filename] = output.getvalue()
        else:
            result[filename] = tar_bytes({stem + '/PKG-INFO': metadata, stem + '/README.md': 'Test package\n'})
    return result


def release_manifest(package, files, selection):
    document = {'source': CANDIDATE, 'version': '0.2.6', 'files': [
        {'name': name, 'sha256': hashlib.sha256(data).hexdigest(), 'size': len(data)}
        for name, data in sorted(files.items())]}
    if package == 'kit':
        document['core'] = {'commit': CANDIDATE, 'version': '0.2.6', 'core_tree': selection['core_tree'],
            'sdk_assets': selection['assets'], 'sdk_commits': {
                asset['platform']: asset.get('commit', CANDIDATE) for asset in selection['assets']}}
    return document


class PackageGitHub(FakeGitHub):
    def __init__(self, directory):
        super().__init__(directory)
        self.collections = 0
        for package, identifier in (('kit', 60), ('python', 70)):
            self.artifacts[identifier] = []
            files = distribution_files(package, self.payloads.selection)
            document = release_manifest(package, files, self.payloads.selection)
            self.payloads.add(identifier, packages.ARTIFACTS[package][0], files)
            self.payloads.add(identifier, packages.ARTIFACTS[package][1], {'release-manifest.json': json.dumps(document)})

    def packages(self, record):
        self.collections += 1
        self.payloads.runs.update(copy.deepcopy(self.runs))
        return packages.collect(record, self.payloads)


class PackageCoordinatorTests(unittest.TestCase):
    reload = preparation.CoordinatorTests.reload
    reach_staging = preparation.CoordinatorTests.reach_staging

    def setUp(self):
        preparation.CoordinatorTests.setUp(self)
        clock = patch.object(packages.manifest_tool, 'datetime', wraps=datetime)
        clock.start().now.return_value = NOW
        self.addCleanup(clock.stop)
        self.github = PackageGitHub(self.directory.name)
        self.coordinator = tool.Coordinator(self.path, self.record, self.github)

    def reach_packages(self):
        self.reach_staging()
        for name in tool.PLATFORMS:
            self.github.finish(name)
        self.coordinator.advance_packages()

    def finish_packages(self):
        self.reach_packages()
        for name in tool.PACKAGES:
            self.github.finish(name)
        self.reload()
        self.coordinator.advance_packages()

    def test_package_preparation_pins_sdk_inputs_and_never_publishes(self):
        self.finish_packages()
        self.assertEqual(self.record['status'], 'packages_prepared')
        self.assertEqual(len(self.github.dispatches()), 7)
        self.assertEqual(self.record['steps']['kit']['inputs']['release_manifest_ref'], MANIFEST)
        for name in tool.PACKAGES:
            self.assertEqual(self.record['steps'][name]['inputs']['destination'], 'none')
            self.assertEqual(self.record['steps'][name]['workflow_commit'], CANDIDATE)
        self.assertEqual(len(self.record['python_packages']['kit']['files']), 19)
        self.assertEqual(len(self.record['python_packages']['python']['files']), 2)
        self.reload()
        self.coordinator.advance_packages()
        self.assertEqual(len(self.github.dispatches()), 7)
        self.assertEqual(self.github.collections, 1)
        self.assertIn('Python release set SHA256', tool.summary(self.record))

    def test_packages_wait_for_staging(self):
        self.reach_staging()
        self.coordinator.advance_packages()
        self.assertEqual(len(self.github.dispatches()), 5)

    def test_failed_package_retry_keeps_successful_sibling_and_sdk_builds(self):
        self.reach_packages()
        self.github.finish('python')
        self.github.finish('kit', 'failure')
        with self.assertRaisesRegex(ValueError, 'kit attempt 1 ended failure'):
            self.coordinator.advance_packages()
        self.coordinator.retry('kit')
        self.github.finish('kit')
        self.coordinator.advance_packages()
        self.assertEqual(self.record['status'], 'packages_prepared')
        self.assertEqual(len(self.github.dispatches()), 7)
        self.assertEqual(self.record['steps']['kit']['history'][0]['conclusion'], 'failure')

    def test_lost_package_dispatch_recovers_without_duplicate(self):
        self.github.lost_dispatch = 'kit'
        with self.assertRaises(tool.APIError):
            self.reach_packages()
        self.reload()
        self.coordinator.advance_packages()
        self.assertEqual(len(self.github.dispatches('release-kit-pypi.yml')), 1)

    def test_saved_package_set_cannot_change_or_be_retried(self):
        self.finish_packages()
        with self.assertRaisesRegex(ValueError, 'verified release set'):
            self.coordinator.retry('kit')
        self.record['python_packages']['kit']['files'][0]['sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'Saved Python release set changed'):
            self.coordinator.advance_packages()

    def test_changed_package_source_is_rejected(self):
        self.reach_packages()
        for name in tool.PACKAGES:
            self.github.finish(name)
        self.github.runs[60]['head_sha'] = 'b' * 40
        with self.assertRaisesRegex(ValueError, 'differs from candidate source'):
            self.coordinator.advance_packages()

    def test_package_archive_digest_failure_stops_acceptance(self):
        self.reach_packages()
        for name in tool.PACKAGES:
            self.github.finish(name)
        self.github.payloads.corrupt_digest = True
        with self.assertRaisesRegex(ValueError, 'checksum differs from GitHub'):
            self.coordinator.advance_packages()
        self.assertNotIn('python_packages', self.record)

    def test_package_attempt_drift_during_download_is_rejected(self):
        self.reach_packages()
        for name in tool.PACKAGES:
            self.github.finish(name)
        original = self.github.payloads.request
        observations = []

        def request(route, **query):
            value = original(route, **query)
            if route.endswith('/actions/runs/60'):
                observations.append(route)
                if len(observations) > 1:
                    value['run_attempt'] += 1
            return value

        self.github.payloads.request = request
        with self.assertRaisesRegex(ValueError, 'retried during verification'):
            self.coordinator.advance_packages()
        self.assertNotIn('python_packages', self.record)

    def test_package_artifact_expiry_and_replacement_are_rejected(self):
        self.finish_packages()
        item = self.github.artifacts[60][0]
        item['expired'] = True
        with self.assertRaisesRegex(ValueError, 'expired'):
            self.coordinator.advance_packages()
        item['expired'] = False
        item['id'] += 100
        with self.assertRaisesRegex(ValueError, 'accepted artifacts changed'):
            self.coordinator.advance_packages()

    def test_packages_cli_resumes_to_its_own_completion_state(self):
        self.reach_packages()
        for name in tool.PACKAGES:
            self.github.finish(name)
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', [
                'release.py', 'packages', '--record', str(self.path), '--wait']), \
                patch('builtins.print'), patch.object(tool.time, 'sleep', side_effect=AssertionError('already ready')):
            tool.main()
        self.reload()
        self.assertEqual(self.record['status'], 'packages_prepared')


class DistributionEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.selection = fixtures.Fixture(self.root).selection

    def write(self, package):
        directory = self.root / package
        directory.mkdir()
        files = distribution_files(package, self.selection)
        for name, data in files.items():
            (directory / name).write_bytes(data)
        return directory, release_manifest(package, files, self.selection)

    def verify(self, directory, package, document):
        return packages.verify(directory, package, '0.2.6', CANDIDATE, document, self.selection)

    def test_both_complete_sets_and_metadata_are_verified(self):
        for package, count in (('kit', 19), ('python', 2)):
            directory, document = self.write(package)
            self.assertEqual(len(self.verify(directory, package, document)), count)

    def test_actual_kit_manifest_producer_agrees_with_the_coordinator_verifier(self):
        directory, _ = self.write('kit')
        (self.root / 'README.md').write_text('Test package\n')
        selection = self.root / 'core-selection.json'
        selection.write_text(json.dumps(self.selection))
        output = self.root / 'release-manifest.json'
        with patch.object(sys, 'path', [str(tool.ROOT / 'python/kit/scripts'), *sys.path]):
            producer = load('kit_manifest_producer', 'python/kit/scripts/check-release-distributions.py')
        previous = Path.cwd()
        try:
            os.chdir(self.root)
            with patch.object(producer.subprocess, 'check_output', return_value=CANDIDATE), \
                    patch.object(producer, 'render_readme', return_value='Test package\n'), \
                    patch.object(sys, 'argv', ['check-release-distributions.py', str(directory),
                        '--version', '0.2.6', '--selection', str(selection), '--manifest', str(output)]), \
                    patch('builtins.print'):
                producer.main()
        finally:
            os.chdir(previous)
        self.assertEqual(len(self.verify(directory, 'kit', json.loads(output.read_text()))), 19)

    def test_missing_extra_and_changed_files_are_rejected(self):
        directory, document = self.write('python')
        path = next(directory.glob('*.whl'))
        original = path.read_bytes()
        path.unlink()
        with self.assertRaisesRegex(ValueError, 'Incomplete'):
            self.verify(directory, 'python', document)
        path.write_bytes(original)
        extra = directory / '../extra.whl'
        extra.write_bytes(b'extra')
        (directory / extra.name).write_bytes(b'extra')
        with self.assertRaisesRegex(ValueError, 'unexpected'):
            self.verify(directory, 'python', document)
        (directory / extra.name).unlink()
        document['files'][0]['sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'hashes differ'):
            self.verify(directory, 'python', document)

    def test_wrong_candidate_and_sdk_provenance_are_rejected(self):
        directory, document = self.write('kit')
        document['source'] = 'b' * 40
        with self.assertRaisesRegex(ValueError, 'source/version'):
            self.verify(directory, 'kit', document)
        document['source'] = CANDIDATE
        document['core']['sdk_assets'] = []
        with self.assertRaisesRegex(ValueError, 'pinned SDK selection'):
            self.verify(directory, 'kit', document)

    def test_wrong_wheel_core_and_package_metadata_are_rejected(self):
        directory, document = self.write('kit')
        path = next(directory.glob('*.whl'))
        with zipfile.ZipFile(path) as archive:
            files = {name: archive.read(name) for name in archive.namelist()}
        core = json.loads(files['holderkit/_core_build.json'])
        core['commit'] = 'f' * 40
        files['holderkit/_core_build.json'] = json.dumps(core)
        with zipfile.ZipFile(path, 'w') as archive:
            for name, data in files.items():
                archive.writestr(name, data)
        with self.assertRaisesRegex(ValueError, 'wheel differs from the pinned SDK'):
            self.verify(directory, 'kit', document)
        directory, document = self.write('python')
        path = next(directory.glob('*.whl'))
        with zipfile.ZipFile(path, 'w') as archive:
            archive.writestr('holder_python-0.2.6.dist-info/METADATA', 'Name: holder-python\nVersion: 0.2.5\n')
        with self.assertRaisesRegex(ValueError, 'metadata differs'):
            self.verify(directory, 'python', document)


class PinnedSDKTests(unittest.TestCase):
    def setUp(self):
        clock = patch.object(sdk.manifest_tool, 'datetime', wraps=datetime)
        clock.start().now.return_value = NOW
        self.addCleanup(clock.stop)
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / 'VERSION').write_text('0.2.6\n')
        checkout = patch.object(sdk, 'ROOT', self.root)
        checkout.start()
        self.addCleanup(checkout.stop)
        self.github = fixtures.Fixture(self.root)
        self.core = self.github.selection['assets'][0]
        data = tar_bytes({'libholder-sdk/libholder-manifest.json': json.dumps(self.core),
                          'libholder-sdk/share/holder/schema/schema.sql': 'schema',
                          'libholder-sdk/lib/cmake/holder/holderConfig.cmake': 'cmake'})
        self.core.update(size=len(data), sha256=hashlib.sha256(data).hexdigest())
        selection_item = self.github.item('core-selection')
        self.github.files[selection_item['id']]['core-selection.json'] = json.dumps(self.github.selection)
        self.github.add(20, 'core-sdk-linux-Release', {self.core['name']: data})
        self.document, self.evidence = fixtures.tool.generate(REPOSITORY, CANDIDATE, '0.2.6', '10', '20', self.github, now=NOW)
        original = self.github.request

        def request(route, **query):
            if '/contents/release/staging/' in route:
                return {'type': 'file', 'encoding': 'base64',
                        'content': base64.b64encode(json.dumps(self.document).encode()).decode()}
            return original(route, **query)
        self.github.request = request

    def fetch(self, candidate=CANDIDATE):
        sdk.fetch(self.root / 'out', 'release-manifests/example.json', MANIFEST, REPOSITORY,
                  candidate, 'linux', 'x86_64', self.github)

    def test_pinned_sdk_download_and_existing_installer_preserve_cached_producer(self):
        self.fetch()
        selection = json.loads((self.root / 'out/core-selection.json').read_text())
        installed = sdk.sdk_tool.fetch(selection, self.root / 'cache', 'linux', 'x86_64', 'Release',
                                       self.root / 'out/core-sdk-assets')
        core = json.loads((installed / 'libholder-manifest.json').read_text())
        self.assertEqual(core['commit'], fixtures.SDK_COMMIT)
        self.assertNotEqual(core['commit'], CANDIDATE)

    def test_other_candidate_and_replaced_selection_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'differs from candidate source'):
            self.fetch('f' * 40)
        self.document['core']['selection_artifact_sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'selection archive checksum changed'):
            self.fetch()

    def test_changed_sdk_bytes_and_expiry_are_rejected(self):
        item = self.github.item('core-sdk-linux-Release')
        self.github.files[item['id']][self.core['name']] += b'damaged'
        with self.assertRaisesRegex(ValueError, 'differs from the pinned core selection'):
            self.fetch()
        item['expired'] = True
        with self.assertRaisesRegex(ValueError, 'expired'):
            self.fetch()


if __name__ == '__main__':
    unittest.main()
