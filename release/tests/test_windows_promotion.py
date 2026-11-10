import copy
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
from urllib.parse import urlsplit

import test_release_coordinator as preparation
import test_linux_promotion as linux
from test_release_coordinator import CANDIDATE, MAIN, NOW, REPOSITORY, tool

windows = tool.windows_tool
promotion = tool.windows_promotion
TAG = 'holder-v0.2.6'


class WindowsGitHub(preparation.FakeGitHub):
    def __init__(self, directory):
        super().__init__(directory)
        self.store = linux.ReleaseStore()
        self.store.create()
        self.reads = 0

    def add_bundle(self):
        asset = next(item for item in self.payloads.selection['assets'] if item['platform'] == 'windows')
        core = {key: asset[key] for key in ('commit', 'version', 'build_type', 'platform', 'architecture')}
        core = windows.manifest.provenance.verify(self.payloads.selection, core, CANDIDATE, CANDIDATE,
            platform='windows', architecture='x86_64', expected_build_type='Release',
            backend_repository=REPOSITORY, backend_run_id='20')
        self.metadata = {'version': '0.2.6', 'platform': 'windows', 'staging_repository': REPOSITORY,
            'staging_run_id': '40', 'core': core, 'desktop_repository': REPOSITORY, 'desktop_run_id': '10',
            'desktop_commit': CANDIDATE, 'backend_repository': REPOSITORY, 'backend_run_id': '20',
            'backend_run_commit': CANDIDATE}
        self.source = self.payloads.directory / 'windows-source'
        self.source.mkdir()
        for name, data in zip(windows.names('0.2.6'), (b'final signed installer', b'public test certificate',
            ('\ufeffSHA256: ' + hashlib.sha256(b'public test certificate').hexdigest().upper() + '\r\nSubject: fixture\r\n').encode())):
            path = self.source / name
            path.parent.mkdir(exist_ok=True)
            path.write_bytes(data)
        windows.record(self.source, self.metadata, REPOSITORY, '40', MAIN)
        files = {path.relative_to(self.source).as_posix(): path.read_bytes() for path in self.source.rglob('*') if path.is_file()}
        self.artifacts[40] = self.artifacts[40][:1]
        self.payloads.add(40, 'Holder-windows-test-signed-installer', files)
        item = self.artifacts[40][-1]
        item['workflow_run']['head_sha'] = MAIN
        self.payloads.read(REPOSITORY, item, [])

    def api(self, route, method='GET', data=None, missing_ok=False, **query):
        path = urlsplit(route).path.removeprefix('repos/' + REPOSITORY + '/')
        if path.startswith(('releases', 'git/ref/tags/', 'git/tags/')):
            return self.store.api(route, method, data, missing_ok, **query)
        try:
            return super().api(route, method, data, missing_ok, **query)
        finally:
            if method == 'POST' and 'staging-windows-stage.yml/dispatches' in path and 40 in self.runs:
                self.add_bundle()
            if method == 'POST' and 'promote-windows-dev-build.yml/dispatches' in path and 150 in self.runs:
                self.artifacts[150] = []

    def windows_bundle(self, record):
        self.reads += 1
        self.payloads.runs.update(copy.deepcopy(self.runs))
        step = record['steps']['windows']
        item = step['artifacts'][-1]
        result = windows.read(REPOSITORY, '40', item['name'], '0.2.6', self.payloads,
            artifact_id=item['id'], workflow_commit=MAIN, clock=NOW)
        tool.require(result['attempt'] == step['attempt'] and result['artifact'] == item, 'Accepted Windows run/artifact changed')
        return result

    def windows_promotion_status(self, record, release_id=None):
        return promotion.observe(self.store, REPOSITORY, TAG,
            windows.payload_files(record['windows_delivery_input'], '0.2.6'), CANDIDATE, release_id)


class WindowsDeliveryTests(unittest.TestCase):
    reload = preparation.CoordinatorTests.reload
    reach_staging = preparation.CoordinatorTests.reach_staging

    def setUp(self):
        preparation.CoordinatorTests.setUp(self)
        self.github = WindowsGitHub(self.directory.name)
        self.coordinator = tool.Coordinator(self.path, self.record, self.github)
        self.reach_staging()
        self.github.finish('windows')
        self.payload = Path(self.directory.name) / 'windows-payload'
        self.payload.mkdir()
        targets = ['Holder-windows-0.2.6-dev-Setup.exe', 'Holder-windows-dev-test-signing.cer',
                   'Holder-windows-dev-test-signing-fingerprint.txt']
        for old, new in zip(windows.names('0.2.6'), targets):
            (self.payload / new).write_bytes((self.github.source / old).read_bytes())
        digest = hashlib.sha256((self.payload / targets[0]).read_bytes()).hexdigest()
        (self.payload / (targets[0] + '.sha256')).write_bytes(f'{digest}  {targets[0]}\n'.encode())

    def deliver(self, **options):
        return promotion.promote(self.github.store, REPOSITORY, TAG, '0.2.6', self.payload, candidate=CANDIDATE, **options)

    def test_complete_handoff_preserves_notes_other_assets_and_accepted_ids(self):
        release = self.github.store.releases[0]
        self.github.store.add(release, 'Linux.AppImage', b'Linux bytes')
        self.coordinator.advance_windows_promotion()
        step = self.record['steps']['windows_promotion']
        bundle = self.record['windows_delivery_input']
        self.assertEqual(step['inputs']['candidate_commit'], CANDIDATE)
        self.assertEqual(step['inputs']['staging_artifact_sha256'], bundle['archive_sha256'])
        self.assertEqual(step['inputs']['staging_artifact_id'], str(bundle['artifact']['id']))
        self.assertEqual(step['inputs']['replace_draft_assets'], 'false')
        self.deliver()
        self.github.finish('windows_promotion')
        self.coordinator.advance_windows_promotion()
        self.assertEqual(self.record['status'], 'windows_promoted')
        self.assertEqual(len(self.record['windows_promoted']['receipt']['files']), 4)
        self.assertEqual(release['body'], 'Reviewed notes')
        self.assertTrue(release['draft'])
        release['draft'] = False
        self.reload()
        self.coordinator.advance_windows_promotion()
        self.assertFalse(self.record['windows_promoted']['last_check']['draft'])
        self.assertEqual(self.github.reads, 1)
        self.assertEqual(len(self.github.dispatches()), 6)
        self.assertEqual(len(self.github.store.uploads), 4)
        self.assertIn('Windows development assets', tool.summary(self.record))

    def test_partial_upload_and_failed_job_retry_reuse_bytes(self):
        self.coordinator.advance_windows_promotion()
        self.github.store.fail_upload = True
        with self.assertRaises(RuntimeError):
            self.deliver()
        self.github.finish('windows_promotion', 'failure')
        with self.assertRaisesRegex(ValueError, 'ended failure'):
            self.coordinator.advance_windows_promotion()
        self.coordinator.retry('windows_promotion')
        self.deliver()
        self.github.finish('windows_promotion', 'failure')
        self.coordinator.advance_windows_promotion()
        self.assertEqual(self.record['status'], 'windows_promoted')
        self.assertEqual(self.record['steps']['windows_promotion']['conclusion'], 'failure')
        self.assertEqual(len(self.github.store.uploads), 4)

    def test_complete_destination_remains_checkable_after_input_expiry(self):
        self.coordinator.prepare_windows_promotion()
        self.deliver()
        with patch.object(tool, 'now', return_value=datetime(2027, 1, 1, tzinfo=timezone.utc)):
            self.coordinator.advance_windows_promotion()
        self.assertIsNone(self.record['windows_promoted']['receipt']['workflow_run_id'])
        self.assertEqual(len(self.github.dispatches()), 5)

    def test_expired_input_blocks_missing_delivery_and_retry(self):
        self.coordinator.prepare_windows_promotion()
        later = datetime(2027, 1, 1, tzinfo=timezone.utc)
        with patch.object(tool, 'now', return_value=later), self.assertRaisesRegex(ValueError, 'expired before promotion'):
            self.coordinator.advance_windows_promotion()
        self.coordinator.advance_windows_promotion()
        self.github.finish('windows_promotion', 'failure')
        with patch.object(tool, 'now', return_value=later), self.assertRaisesRegex(ValueError, 'expired before promotion'):
            self.coordinator.retry('windows_promotion')

    def test_accepted_staging_and_delivery_cannot_be_replaced(self):
        self.coordinator.advance_windows_promotion()
        with self.assertRaisesRegex(ValueError, 'pinned for delivery'):
            self.coordinator.retry('windows')
        self.deliver()
        self.github.finish('windows_promotion')
        self.coordinator.advance_windows_promotion()
        with self.assertRaisesRegex(ValueError, 'already accepted'):
            self.coordinator.retry('windows_promotion')
        asset = self.github.store.assets[1][0]
        asset['id'] += 1000
        with self.assertRaisesRegex(ValueError, 'assets or source changed'):
            self.coordinator.advance_windows_promotion()

    def test_unsigned_policy_and_source_drift_stop_before_dispatch(self):
        self.record['windows_sign_test_installer'] = False
        with self.assertRaisesRegex(ValueError, 'signing enabled'):
            self.coordinator.advance_windows_promotion()
        self.record['windows_sign_test_installer'] = True
        self.github.store.tag['sha'] = 'b' * 40
        with self.assertRaisesRegex(ValueError, 'differs'):
            self.coordinator.advance_windows_promotion()
        self.assertEqual(len(self.github.dispatches()), 5)

    def test_changed_component_and_sdk_provenance_rejected(self):
        self.coordinator.prepare_windows_promotion()
        original = copy.deepcopy(self.record['windows_delivery_input'])
        for component in ('desktop', 'backend', 'core'):
            with self.subTest(component=component):
                bundle = copy.deepcopy(original)
                bundle['provenance'][component]['commit'] = '9' * 40
                self.record.update(windows_delivery_input=bundle, windows_delivery_input_sha256=tool.digest(bundle))
                with self.assertRaises(ValueError):
                    self.coordinator.advance_windows_promotion()
        self.assertEqual(len(self.github.dispatches()), 5)

    def test_staging_final_bytes_and_certificate_fingerprint_are_verified(self):
        item = self.github.artifacts[40][-1]
        data = self.github.payloads.files[item['id']]
        original = copy.deepcopy(data)
        for name in windows.names('0.2.6'):
            with self.subTest(name=name):
                self.github.payloads.files[item['id']] = {**original, name: b'changed'}
                with self.assertRaisesRegex(ValueError, 'final bytes differ'):
                    self.github.windows_bundle({'steps': {'windows': {'artifacts': [item], 'attempt': 1}}})
        self.github.payloads.files[item['id']] = original
        (self.github.source / windows.names('0.2.6')[1]).write_bytes(b'changed certificate')
        with self.assertRaisesRegex(ValueError, 'public fingerprint'):
            windows.record(self.github.source, self.github.metadata, REPOSITORY, '40', MAIN)

    def test_archive_identity_and_extra_files_are_rejected(self):
        self.coordinator.prepare_windows_promotion()
        bundle = self.record['windows_delivery_input']
        for options, message in (({'artifact_id': '0'}, 'artifact ID changed'),
                                 ({'archive_sha256': '0' * 64}, 'archive checksum changed'),
                                 ({'workflow_commit': CANDIDATE}, 'workflow source changed')):
            with self.subTest(options=options), self.assertRaisesRegex(ValueError, message):
                windows.read(REPOSITORY, '40', bundle['artifact']['name'], '0.2.6', self.github.payloads, clock=NOW, **options)
        self.github.payloads.files[bundle['artifact']['id']]['installer/extra.exe'] = b'extra'
        with self.assertRaisesRegex(ValueError, 'Unexpected or duplicate'):
            windows.read(REPOSITORY, '40', bundle['artifact']['name'], '0.2.6', self.github.payloads, clock=NOW)

    def test_missing_shared_draft_and_wrong_tag_never_create_release(self):
        self.github.store.releases.clear()
        with self.assertRaisesRegex(ValueError, 'shared product draft'):
            self.deliver()
        with self.assertRaisesRegex(ValueError, 'matching product tag'):
            promotion.promote(self.github.store, REPOSITORY, 'untagged-anything', '0.2.6', self.payload)
        self.assertFalse(self.github.store.mutations)

    def test_conflicts_are_not_clobbered_and_published_stable_is_read_only(self):
        release = self.github.store.releases[0]
        self.github.store.add(release, next(iter(promotion.names('0.2.6'))), b'wrong')
        with self.assertRaisesRegex(ValueError, 'explicit draft-only'):
            self.deliver()
        self.assertFalse(self.github.store.uploads or self.github.store.mutations)
        self.github.store.assets[1].clear()
        release['draft'] = False
        with self.assertRaisesRegex(ValueError, 'cannot be modified'):
            self.deliver()
        self.assertFalse(self.github.store.uploads or self.github.store.mutations)
        with self.assertRaisesRegex(ValueError, 'cannot receive missing Windows'):
            self.coordinator.advance_windows_promotion()
        self.assertEqual(len(self.github.dispatches()), 5)

    def test_explicit_manual_draft_replacement_and_legacy_asset_digest(self):
        release = self.github.store.releases[0]
        for path in self.payload.iterdir():
            self.github.store.add(release, path.name, path.read_bytes(), digest=False)
        self.deliver()
        self.assertEqual(set(self.github.store.downloads), {item['id'] for item in self.github.store.assets[1]})
        asset = self.github.store.assets[1][0]
        self.github.store.blobs[asset['id']] = b'wrong'
        with self.assertRaisesRegex(ValueError, 'cannot replace'):
            self.deliver(replace_draft_assets=True)
        promotion.promote(self.github.store, REPOSITORY, TAG, '0.2.6', self.payload, replace_draft_assets=True)
        self.assertEqual(len(self.github.store.uploads), 1)

    def test_record_entry_point_accepts_powershell_utf8_metadata(self):
        path = self.github.source / 'metadata.json'
        path.write_text(json.dumps(self.github.metadata), encoding='utf-8-sig')
        with patch.object(sys, 'argv', ['windows-candidate.py', 'record', '--repository', REPOSITORY,
            '--run-id', '40', '--workflow-commit', MAIN, '--metadata', str(path), '--directory', str(self.github.source)]):
            windows.main()
        evidence = json.loads((self.github.source / 'release-evidence.json').read_text())
        self.assertEqual(evidence['channel'], 'windows-dev')
        self.assertEqual(evidence['staging']['workflow_commit'], MAIN)
        self.assertEqual(len(evidence['files']), 3)

    def test_real_download_entry_point_produces_the_expected_four_assets(self):
        self.github.payloads.runs.update(copy.deepcopy(self.github.runs))
        output = Path(self.directory.name) / 'download-output'
        with patch.object(windows.manifest, 'GitHub', return_value=self.github.payloads), patch.object(sys, 'argv', [
            'windows-candidate.py', 'fetch', '--repository', REPOSITORY, '--run-id', '40', '--directory', str(output)]):
            windows.main()
        self.assertEqual({path.name: path.read_bytes() for path in output.iterdir()},
                         {path.name: path.read_bytes() for path in self.payload.iterdir()})

    def test_manual_development_prerelease_publishes_only_after_complete_upload(self):
        class DevStore(linux.ReleaseStore):
            def api(self, route, method='GET', data=None, **options):
                if method == 'PATCH':
                    self.mutations.append((route, method, data))
                    self.releases[0].update(data)
                    return copy.deepcopy(self.releases[0])
                result = super().api(route, method, data, **options)
                if method == 'POST' and route.endswith('/releases'):
                    self.releases[0]['tag_name'] = data['tag_name']
                    result['tag_name'] = data['tag_name']
                return result
        github = DevStore()
        github.fail_upload = True
        def deliver():
            return promotion.promote(github, REPOSITORY, 'holder-windows-dev-40', '0.2.6', self.payload, dev_run='40')
        with self.assertRaises(RuntimeError):
            deliver()
        self.assertTrue(github.releases[0]['draft'])
        deliver()
        self.assertFalse(github.releases[0]['draft'])
        self.assertTrue(github.releases[0]['prerelease'])
        self.assertEqual(len(github.uploads), 4)
        deliver()
        self.assertEqual(len(github.uploads), 4)

    def test_lost_dispatch_reconciles_without_duplicate_delivery_run(self):
        self.github.lost_dispatch = 'windows_promotion'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance_windows_promotion()
        self.reload()
        self.coordinator.advance_windows_promotion()
        self.assertEqual(self.record['steps']['windows_promotion']['run_id'], '150')
        self.assertEqual(len(self.github.dispatches()), 6)


if __name__ == '__main__':
    unittest.main()
