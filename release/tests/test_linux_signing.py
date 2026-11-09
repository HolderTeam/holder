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
from test_release_coordinator import CANDIDATE, MAIN, NOW, REPOSITORY, tool

linux = tool.linux_tool


class SigningGitHub(preparation.FakeGitHub):
    def __init__(self, directory):
        super().__init__(directory)
        self.waiting = False
        self.bundle_reads = []
        asset = self.payloads.selection['assets'][0]
        core = {key: asset[key] for key in ('commit', 'version', 'build_type', 'platform', 'architecture')}
        self.provenance = {'appimage_version': '0.2.6',
            'desktop': {'repository': REPOSITORY, 'run_id': '10', 'commit': CANDIDATE, 'version': '0.2.6'},
            'backend': {'repository': REPOSITORY, 'run_id': '20', 'commit': CANDIDATE, 'version': '0.2.6', 'api_version': '1.0'},
            'core': linux.manifest_tool.provenance.verify(self.payloads.selection, core, CANDIDATE, CANDIDATE,
                platform='linux', architecture='x86_64', expected_build_type='Release',
                backend_repository=REPOSITORY, backend_run_id='20')}

    def add_bundle(self, signed):
        identifier = 120 if signed else 30
        self.artifacts[identifier] = []
        stem = 'Holder-0.2.6'
        files = {stem + '-x86_64.AppImage': b'signed output' if signed else b'staged output',
                 stem + '-provenance.json': json.dumps(self.provenance)}
        checksums = [stem + '-x86_64.AppImage']
        if signed:
            artifact = self.artifacts[30][0]
            path = self.payloads.directory / f"{artifact['id']}.zip"
            files.update({stem + '-embedded-signature.asc': 'signature fixture',
                stem + '-SHA256SUMS.asc': 'signature fixture', 'Holder-linux-release-key.asc': 'public key fixture',
                stem + '-signing-metadata.json': json.dumps({'appimage_version': '0.2.6',
                    'staging': {'repository': REPOSITORY, 'run_id': '30', 'artifact_id': str(artifact['id']),
                        'archive_sha256': hashlib.sha256(path.read_bytes()).hexdigest()},
                    'signing_workflow': {'repository': REPOSITORY, 'run_id': '120'}})})
        else:
            files[stem + '-x86_64.AppDir.tar.gz'] = b'AppDir fixture'
            checksums.append(stem + '-x86_64.AppDir.tar.gz')
        files[stem + '-SHA256SUMS'] = ''.join(f'{hashlib.sha256(files[name]).hexdigest()}  {name}\n' for name in checksums)
        self.payloads.add(identifier, 'Holder-linux-appimage-signed-0.2.6' if signed else 'Holder-linux-appimage-staged', files)
        item = self.artifacts[identifier][0]
        item['workflow_run']['head_sha'] = MAIN
        if signed:
            item['expires_at'] = '2026-12-09T00:00:00Z'
        # Give API metadata the real ZIP digest before the coordinator accepts it.
        self.payloads.read(REPOSITORY, item, [])

    def api(self, route, method='GET', data=None, missing_ok=False, **query):
        path = urlsplit(route).path
        if path.endswith('/pending_deployments'):
            return [{'environment': {'name': 'linux-release-signing'}}] if self.waiting else []
        try:
            return super().api(route, method, data, missing_ok, **query)
        finally:
            if method == 'POST' and path.endswith('/dispatches'):
                if 'staging-linux-appimage-stage.yml' in path and 30 in self.runs:
                    self.add_bundle(False)
                elif 'sign-linux-appimage.yml' in path and 120 in self.runs:
                    self.add_bundle(True)

    def linux_bundle(self, record, name):
        self.bundle_reads.append(name)
        self.payloads.runs.update(copy.deepcopy(self.runs))
        step = record['steps'][name]
        artifact = step['artifacts'][0]
        bundle = linux.read(REPOSITORY, step['run_id'], artifact['name'], '0.2.6', self.payloads,
            signed=name == 'linux_signing', artifact_id=artifact['id'], workflow_commit=step['workflow_commit'], clock=NOW)
        tool.require(bundle['attempt'] == step['attempt'] and bundle['artifact'] == artifact,
                     'Accepted Linux run/artifact changed during verification')
        linux.bind(record, bundle)
        return bundle


class LinuxSigningTests(unittest.TestCase):
    reload = preparation.CoordinatorTests.reload
    reach_staging = preparation.CoordinatorTests.reach_staging

    def setUp(self):
        preparation.CoordinatorTests.setUp(self)
        self.github = SigningGitHub(self.directory.name)
        self.coordinator = tool.Coordinator(self.path, self.record, self.github)
        self.reach_staging()
        self.github.finish('linux')
        clock = patch.object(linux, 'datetime', wraps=datetime)
        clock.start().now.return_value = NOW
        self.addCleanup(clock.stop)

    def sign(self):
        self.coordinator.advance_signing()
        self.github.finish('linux_signing')
        self.coordinator.advance_signing()

    def test_complete_signing_pins_exact_input_and_retains_signed_hashes(self):
        self.sign()
        self.assertEqual(self.record['status'], 'linux_signed')
        step = self.record['steps']['linux_signing']
        self.assertEqual(step['ref'], 'main')
        self.assertEqual(step['inputs']['staging_artifact_id'], str(self.record['linux_signing_input']['artifact']['id']))
        self.assertEqual(step['inputs']['staging_artifact_sha256'], self.record['linux_signing_input']['archive_sha256'])
        self.assertEqual(len(self.record['linux_signed']['files']), 7)
        self.assertNotEqual(self.record['linux_signed']['files'], self.record['linux_signing_input']['files'])
        self.reload()
        self.coordinator.advance_signing()
        self.assertEqual(len(self.github.dispatches()), 6)
        self.assertEqual(self.github.bundle_reads, ['linux', 'linux_signing'])
        self.assertIn('Signed Linux evidence SHA256', tool.summary(self.record))
        for route, method, data in self.github.mutations:
            self.assertNotIn('/releases', route)
            self.assertNotIn('pending_deployments', route)
            self.assertNotIn('refs/tags/', str(data))

    def test_approval_wait_is_reported_and_never_approved_by_the_coordinator(self):
        self.github.waiting = True
        self.coordinator.advance_signing()
        self.assertEqual(self.record['status'], 'awaiting_linux_signing_approval')
        self.assertIn('Inspect the workflow verification summary', tool.summary(self.record))
        self.reload()
        self.coordinator.advance_signing()
        self.assertEqual(len(self.github.dispatches()), 6)
        with self.assertRaisesRegex(ValueError, 'not a failed completed run'):
            self.coordinator.retry('linux_signing')
        self.github.waiting = False
        self.github.finish('linux_signing')
        self.coordinator.advance_signing()
        self.assertEqual(self.record['status'], 'linux_signed')

    def test_lost_dispatch_recovers_and_failed_jobs_retry_without_rebuilding(self):
        self.github.lost_dispatch = 'linux_signing'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance_signing()
        self.reload()
        self.coordinator.advance_signing()
        self.assertEqual(self.record['steps']['linux_signing']['run_id'], '120')
        self.github.finish('linux_signing', 'failure')
        with self.assertRaisesRegex(ValueError, 'ended failure'):
            self.coordinator.advance_signing()
        self.coordinator.retry('linux_signing')
        self.github.finish('linux_signing')
        self.coordinator.advance_signing()
        self.assertEqual(self.record['linux_signed']['attempt'], 2)
        self.assertEqual(len(self.github.dispatches()), 6)
        self.assertEqual(self.github.bundle_reads.count('linux'), 1)

    def test_accepted_input_and_signed_outputs_cannot_be_replaced(self):
        self.sign()
        for name in ('linux', 'linux_signing'):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, 'already accepted|pinned for signing'):
                self.coordinator.retry(name)
        self.record['linux_signing_input']['archive_sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'Saved Linux signing input changed'):
            self.coordinator.advance_signing()

    def test_input_expiry_stops_pending_signing_but_not_accepted_signed_output(self):
        self.coordinator.advance_signing()
        later = datetime(2026, 11, 16, tzinfo=timezone.utc)
        with patch.object(tool, 'now', return_value=later), self.assertRaisesRegex(ValueError, 'expired before signing completed'):
            self.coordinator.advance_signing()
        self.github.finish('linux_signing')
        with patch.object(tool, 'now', return_value=later):
            self.coordinator.advance_signing()
        self.assertEqual(self.record['status'], 'linux_signed')

    def test_wrong_component_or_sdk_provenance_stops_before_signing_dispatch(self):
        item = self.github.artifacts[30][0]
        file = self.github.payloads.files[item['id']]
        self.github.provenance['backend']['commit'] = 'b' * 40
        file['Holder-0.2.6-provenance.json'] = json.dumps(self.github.provenance)
        self.github.payloads.read(REPOSITORY, item, [])
        with self.assertRaisesRegex(ValueError, 'backend differs from candidate pins'):
            self.coordinator.advance_signing()
        self.assertEqual(len(self.github.dispatches()), 5)

    def test_wrong_signed_handoff_metadata_is_rejected(self):
        self.coordinator.advance_signing()
        item = self.github.artifacts[120][0]
        files = self.github.payloads.files[item['id']]
        metadata = json.loads(files['Holder-0.2.6-signing-metadata.json'])
        metadata['staging']['artifact_id'] = '99999'
        files['Holder-0.2.6-signing-metadata.json'] = json.dumps(metadata)
        self.github.payloads.read(REPOSITORY, item, [])
        self.github.finish('linux_signing')
        with self.assertRaisesRegex(ValueError, 'metadata differs from the selected handoff'):
            self.coordinator.advance_signing()
        self.assertNotIn('linux_signed', self.record)

    def test_wrong_sdk_archive_provenance_stops_before_signing(self):
        item = self.github.artifacts[30][0]
        self.github.provenance['core']['sdk_asset']['sha256'] = '0' * 64
        self.github.payloads.files[item['id']]['Holder-0.2.6-provenance.json'] = json.dumps(self.github.provenance)
        self.github.payloads.read(REPOSITORY, item, [])
        with self.assertRaisesRegex(ValueError, 'SDK provenance differs'):
            self.coordinator.advance_signing()
        self.assertEqual(len(self.github.dispatches()), 5)

    def test_cli_wait_uses_signing_completion_and_does_not_advance_other_staging(self):
        self.coordinator.advance_signing()
        self.github.finish('linux_signing')
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', [
                'release.py', 'sign-linux', '--record', str(self.path), '--wait']), patch('builtins.print'), \
                patch.object(tool.time, 'sleep', side_effect=AssertionError('already ready')):
            tool.main()
        self.reload()
        self.assertEqual(self.record['status'], 'linux_signed')
        self.assertEqual(self.github.runs[40]['status'], 'queued')

    def test_worker_fetches_exact_pinned_bytes_without_executing_payloads(self):
        self.coordinator.advance_signing()
        bundle = self.record['linux_signing_input']
        output = Path(self.directory.name) / 'worker'
        with patch.object(linux.manifest_tool, 'GitHub', return_value=self.github.payloads), \
                patch.object(sys, 'argv', ['linux-candidate.py', '--repository', REPOSITORY, '--run-id', '30',
                    '--artifact-id', str(bundle['artifact']['id']), '--archive-sha256', bundle['archive_sha256'],
                    '--version', '0.2.6', '--output', str(output)]):
            linux.main()
        self.assertEqual({path.name for path in output.iterdir()}, linux.files('0.2.6'))
        self.assertEqual((output / 'Holder-0.2.6-x86_64.AppImage').read_bytes(), b'staged output')
        self.assertTrue((output / 'Holder-0.2.6-x86_64.AppImage').stat().st_mode & 0o111)

    def test_worker_rejects_wrong_archive_id_checksum_owner_and_payload_checksums(self):
        self.coordinator.advance_signing()
        bundle = self.record['linux_signing_input']
        arguments = dict(repository=REPOSITORY, identifier='30', artifact_name='Holder-linux-appimage-staged',
            version='0.2.6', github=self.github.payloads, clock=NOW)
        for changes in ({'artifact_id': '999'}, {'archive_sha256': '0' * 64}):
            with self.subTest(changes=changes), self.assertRaisesRegex(ValueError, 'changed'):
                linux.read(**arguments, **changes)
        item = self.github.artifacts[30][0]
        item['workflow_run']['head_sha'] = CANDIDATE
        with self.assertRaisesRegex(ValueError, 'does not belong'):
            linux.read(**arguments)
        item['workflow_run']['head_sha'] = MAIN
        self.github.payloads.files[item['id']]['Holder-0.2.6-x86_64.AppImage'] = b'changed'
        with self.assertRaisesRegex(ValueError, 'payload checksums differ'):
            linux.read(**arguments)


if __name__ == '__main__':
    unittest.main()
