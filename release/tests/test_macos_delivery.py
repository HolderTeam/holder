import copy
import contextlib
import io
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import plistlib
import shutil
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from urllib.parse import urlsplit

import test_release_coordinator as preparation
import test_linux_promotion as linux
from test_release_coordinator import CANDIDATE, MAIN, NOW, REPOSITORY, tool

macos = tool.macos_tool


class MacGitHub(preparation.FakeGitHub):
    def __init__(self, directory):
        super().__init__(directory)
        self.store = linux.ReleaseStore()
        self.store.create()
        self.reads = 0

    def add_bundle(self, inputs):
        asset = next(item for item in self.payloads.selection['assets'] if item['platform'] == 'macos')
        core = {key: asset[key] for key in ('commit', 'version', 'build_type', 'platform', 'architecture')}
        core = macos.manifest.provenance.verify(self.payloads.selection, core, CANDIDATE, CANDIDATE,
            platform='macos', architecture='arm64', expected_build_type='Release',
            backend_repository=REPOSITORY, backend_run_id='20')
        self.metadata = {'version': '0.2.6', 'platform': 'macos', 'staging_repository': REPOSITORY,
            'staging_sha': MAIN, 'core': core, 'desktop_repository': REPOSITORY, 'desktop_run_id': '10',
            'desktop_sha': CANDIDATE, 'backend_repository': REPOSITORY, 'backend_run_id': '20',
            'backend_run_commit': CANDIDATE}
        self.framework = json.loads(self.files[preparation.MANIFEST]['release/staging/' + inputs['release_manifest']])
        self.artifacts[50] = []
        self.payloads.add(50, 'Holder-macos-staged', {
            macos.METADATA: json.dumps(self.metadata), macos.CORE: json.dumps(core),
            macos.FRAMEWORK: json.dumps(self.framework), 'Holder-macos-staged.dmg': b'preview'})
        item = self.artifacts[50][0]
        item['workflow_run']['head_sha'] = MAIN
        self.payloads.read(REPOSITORY, item, [])

    def api(self, route, method='GET', data=None, missing_ok=False, **query):
        path = urlsplit(route).path.removeprefix('repos/' + REPOSITORY + '/')
        if path.startswith(('releases', 'git/ref/tags/', 'git/tags/')):
            return self.store.api(route, method, data, missing_ok, **query)
        try:
            return super().api(route, method, data, missing_ok, **query)
        finally:
            if method == 'POST' and 'staging-macos-stage.yml/dispatches' in path and 50 in self.runs:
                self.add_bundle(data['inputs'])

    def macos_bundle(self, record):
        self.reads += 1
        self.payloads.runs.update(copy.deepcopy(self.runs))
        return macos.staging(record, self.payloads, NOW)

    def asset_fingerprint(self, repository, asset):
        return self.store.asset_fingerprint(repository, asset)

    def upload(self, repository, tag, path):
        return self.store.upload(repository, tag, path)


class MacosDeliveryTests(unittest.TestCase):
    reload = preparation.CoordinatorTests.reload
    reach_staging = preparation.CoordinatorTests.reach_staging

    def setUp(self):
        preparation.CoordinatorTests.setUp(self)
        self.github = MacGitHub(self.directory.name)
        self.coordinator = tool.Coordinator(self.path, self.record, self.github)
        self.reach_staging()
        self.github.finish('macos')
        self.payload = Path(self.directory.name) / 'final'
        self.payload.mkdir()
        self.image = self.payload / 'Holder-macos-0.2.6.dmg'
        self.image.write_bytes(b'final disk image')
        self.write_checksum()
        self.commands = []
        self.fail = None
        self.changed_metadata = False
        self.changed_image = False

    def write_checksum(self):
        self.checksum = self.image.with_suffix('.dmg.sha256')
        self.checksum.write_text(hashlib.sha256(self.image.read_bytes()).hexdigest() + '  ' + self.image.name + '\n')

    def native(self, command, **options):
        self.commands.append(command)
        self.assertTrue(options['capture_output'])
        if 'attach' in command:
            mount = Path(command[command.index('-mountpoint') + 1])
            item = self.record['macos_handoff']
            for relative, data in ((macos.METADATA, item['metadata']), (macos.CORE, item['core']), (macos.FRAMEWORK, item['framework'])):
                path = mount / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(json.dumps(data))
            info = mount / 'Holder.app/Contents/Info.plist'
            info.write_bytes(plistlib.dumps({'CFBundleIdentifier': 'team.holder.Holder',
                'CFBundleShortVersionString': '0.2.6', 'CFBundleVersion': '0.2.6'}))
            if self.changed_metadata:
                (mount / macos.METADATA).write_text('{}')
            if self.changed_image:
                self.image.write_bytes(b'changed while verifying')
        if 'detach' in command and self.fail != 'detach':
            shutil.rmtree(Path(command[-1]) / 'Holder.app')
        failed = self.fail and self.fail in command
        return SimpleNamespace(returncode=1 if failed else 0)

    def accept(self):
        with patch.object(macos.sys, 'platform', 'darwin'), patch.object(macos.subprocess, 'run', side_effect=self.native):
            self.coordinator.accept_macos(self.payload)

    def test_accept_and_promote_cli_write_the_record_and_summary(self):
        self.coordinator.prepare_macos_handoff()
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(macos.sys, 'platform', 'darwin'), \
             patch.object(macos.subprocess, 'run', side_effect=self.native), contextlib.redirect_stdout(io.StringIO()):
            for command in ('accept-macos', 'promote-macos'):
                with patch.object(sys, 'argv', ['release.py', command, '--record', str(self.path),
                        '--macos-directory', str(self.payload)]):
                    tool.main()
        self.reload()
        self.assertEqual(self.record['status'], 'macos_promoted')
        self.assertIn('macOS delivery receipt SHA256', self.path.with_suffix('.summary.md').read_text())

    def test_stage_pins_final_verification_and_full_delivery_survive_reload(self):
        self.assertTrue(self.coordinator.prepare_macos_handoff())
        self.assertEqual(self.record['macos_handoff']['artifact']['name'], 'Holder-macos-staged')
        self.accept()
        checks = self.record['macos_verified']['receipt']['checks']
        self.assertEqual(set(checks.values()), {'passed'})
        self.assertEqual(len(self.commands), 5)
        attach = next(command for command in self.commands if 'attach' in command)
        self.assertIn('-readonly', attach)
        self.assertIn('-nobrowse', attach)
        self.reload()
        self.coordinator.advance_macos_promotion(self.payload)
        self.assertEqual(self.record['status'], 'macos_promoted')
        self.assertEqual(len(self.github.store.uploads), 2)
        self.assertEqual(self.github.store.releases[0]['body'], 'Reviewed notes')
        self.github.store.releases[0]['draft'] = False
        shutil.rmtree(self.payload)
        self.reload()
        self.coordinator.advance_macos_promotion()
        self.assertEqual(self.record['status'], 'macos_promoted')
        self.assertFalse(self.record['macos_promoted']['last_check']['draft'])
        self.assertEqual(self.github.reads, 1)
        self.assertIn('macOS delivery receipt', tool.summary(self.record))
        public_record = json.dumps(self.record)
        self.assertNotIn(str(self.payload), public_record)
        self.assertNotIn('Authority=', public_record)

    def test_failed_signature_and_candidate_mismatch_detach_without_accepting(self):
        self.fail = '--verify'
        with self.assertRaisesRegex(ValueError, 'app signature'):
            self.accept()
        self.assertTrue(any('detach' in command for command in self.commands))
        self.assertNotIn('macos_verified', self.record)
        self.fail = None
        self.changed_metadata = True
        self.commands.clear()
        with self.assertRaisesRegex(ValueError, 'selected staged metadata'):
            self.accept()
        self.assertTrue(any('detach' in command for command in self.commands))
        self.assertFalse(any('codesign' == Path(command[0]).name for command in self.commands))
        self.assertNotIn('macos_verified', self.record)

    def test_failed_detach_does_not_accept_final_output(self):
        self.fail = 'detach'
        with self.assertRaisesRegex(ValueError, 'detach disk image'):
            self.accept()
        self.assertNotIn('macos_verified', self.record)
        attach = next(command for command in self.commands if 'attach' in command)
        remaining_mount = Path(attach[attach.index('-mountpoint') + 1])
        self.assertTrue((remaining_mount / 'Holder.app').is_dir())
        shutil.rmtree(remaining_mount.parent)  # Dispose of the simulated mounted volume.
        self.fail = None
        self.accept()
        self.assertEqual(self.record['status'], 'macos_verified')

    def test_handoff_can_be_pinned_before_the_publication_tag(self):
        self.github.store.tag = None
        self.assertTrue(self.coordinator.prepare_macos_handoff())
        self.accept()
        self.assertEqual(self.record['status'], 'macos_verified')
        with self.assertRaisesRegex(ValueError, 'Create the reviewed release tag'):
            self.coordinator.advance_macos_promotion(self.payload)
        self.assertFalse(self.github.store.uploads)

    def test_gatekeeper_failure_and_file_changes_stop_acceptance(self):
        self.fail = '--assess'
        with self.assertRaisesRegex(ValueError, 'Gatekeeper'):
            self.accept()
        self.assertNotIn('macos_verified', self.record)
        self.fail = None
        self.changed_image = True
        with self.assertRaisesRegex(ValueError, 'checksum|changed'):
            self.accept()
        self.assertNotIn('macos_verified', self.record)

    def test_verification_requires_a_mac_and_matching_checksum(self):
        with patch.object(macos.sys, 'platform', 'linux'), self.assertRaisesRegex(ValueError, 'on a Mac'):
            self.coordinator.accept_macos(self.payload)
        self.checksum.write_text('wrong checksum\n')
        with self.assertRaisesRegex(ValueError, 'checksum must match'):
            self.accept()
        self.assertFalse(self.commands)

    def test_changed_accepted_files_and_retried_stage_are_rejected(self):
        self.accept()
        with self.assertRaisesRegex(ValueError, 'pinned for delivery'):
            self.coordinator.retry('macos')
        self.image.write_bytes(b'changed')
        self.write_checksum()
        with self.assertRaisesRegex(ValueError, 'differ from the accepted handoff'):
            self.accept()
        with self.assertRaisesRegex(ValueError, 'differ from the accepted final bytes'):
            self.coordinator.advance_macos_promotion(self.payload)
        self.assertFalse(self.github.store.uploads)

    def test_partial_upload_response_loss_resumes_without_replacing_bytes(self):
        self.accept()
        self.github.store.fail_upload = True
        with self.assertRaisesRegex(RuntimeError, 'response lost'):
            self.coordinator.advance_macos_promotion(self.payload)
        self.reload()
        self.coordinator.advance_macos_promotion(self.payload)
        self.assertEqual(len(self.github.store.uploads), 2)
        self.assertEqual(self.record['status'], 'macos_promoted')

    def test_manual_complete_destination_is_adopted_after_staging_expires(self):
        self.accept()
        self.coordinator.advance_macos_promotion(self.payload)
        del self.record['macos_promoted']
        with patch.object(tool, 'now', return_value=datetime(2027, 1, 1, tzinfo=timezone.utc)):
            self.coordinator.advance_macos_promotion()
        self.assertEqual(len(self.github.store.uploads), 2)
        self.assertEqual(self.record['status'], 'macos_promoted')

    def test_destination_hash_id_and_source_tag_drift_are_rejected(self):
        self.accept()
        self.coordinator.advance_macos_promotion(self.payload)
        asset = self.github.store.assets[1][0]
        asset['id'] += 1000
        with self.assertRaisesRegex(ValueError, 'identities changed'):
            self.coordinator.advance_macos_promotion()
        asset['digest'] = 'sha256:' + 'f' * 64
        with self.assertRaisesRegex(ValueError, 'differs from accepted'):
            self.coordinator.advance_macos_promotion()
        self.github.store.tag['sha'] = 'c' * 40
        with self.assertRaisesRegex(ValueError, 'differs from prepared candidate'):
            self.coordinator.advance_macos_promotion()

    def test_conflict_published_release_and_missing_shared_draft_stop_writes(self):
        self.accept()
        self.github.store.add(self.github.store.releases[0], self.image.name, b'conflict')
        with self.assertRaisesRegex(ValueError, 'differs from accepted'):
            self.coordinator.advance_macos_promotion(self.payload)
        self.github.store.assets[1].clear()
        self.github.store.releases[0]['draft'] = False
        with self.assertRaisesRegex(ValueError, 'matching mutable draft'):
            self.coordinator.advance_macos_promotion(self.payload)
        self.github.store.releases.clear()
        with self.assertRaisesRegex(ValueError, 'shared product draft'):
            self.coordinator.advance_macos_promotion(self.payload)
        self.assertFalse(self.github.store.uploads or self.github.store.mutations)

    def test_first_handoff_rejects_expired_and_mismatched_staging_inputs(self):
        artifact = self.github.artifacts[50][0]
        artifact['expires_at'] = '2026-01-01T00:00:00Z'
        with self.assertRaisesRegex(ValueError, 'expired'):
            self.coordinator.prepare_macos_handoff()
        artifact['expires_at'] = '2026-11-15T00:00:00Z'
        self.github.payloads.files[artifact['id']][macos.CORE] = '{}'
        self.github.payloads.read(REPOSITORY, artifact, [])
        with self.assertRaisesRegex(ValueError, 'SDK evidence differs'):
            self.coordinator.prepare_macos_handoff()
        self.assertNotIn('macos_handoff', self.record)


if __name__ == '__main__':
    unittest.main()
