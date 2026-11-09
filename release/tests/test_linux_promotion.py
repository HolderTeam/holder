import copy
from datetime import datetime, timezone
import hashlib
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from urllib.parse import urlsplit

import test_linux_signing as signing
import test_release_coordinator as preparation
from test_release_coordinator import CANDIDATE, NOW, REPOSITORY, tool

promotion = tool.promotion_tool
TAG = 'holder-v0.2.6'


class ReleaseStore:
    def __init__(self):
        self.tag = {'type': 'commit', 'sha': CANDIDATE}
        self.releases, self.assets, self.blobs = [], {}, {}
        self.mutations, self.uploads, self.downloads = [], [], []
        self.next_id = 100
        self.fail_upload = False
        self.before_release_get = None

    def create(self, draft=True, prerelease=False):
        release = {'id': len(self.releases) + 1, 'tag_name': TAG, 'draft': draft, 'prerelease': prerelease,
                   'immutable': False, 'html_url': f'https://github.com/{REPOSITORY}/releases/tag/{TAG}',
                   'name': 'Reviewed title', 'body': 'Reviewed notes'}
        self.releases.append(release)
        self.assets[release['id']] = []
        return release

    def add(self, release, name, data, digest=True):
        self.next_id += 1
        asset = {'id': self.next_id, 'name': name, 'state': 'uploaded', 'size': len(data),
                 'digest': 'sha256:' + hashlib.sha256(data).hexdigest() if digest else None}
        self.assets[release['id']].append(asset)
        self.blobs[asset['id']] = data
        return asset

    def api(self, route, method='GET', data=None, missing_ok=False, **query):
        path = urlsplit(route).path.removeprefix('repos/' + REPOSITORY + '/')
        if method != 'GET':
            self.mutations.append((path, method, copy.deepcopy(data)))
        if path == 'git/ref/tags/' + TAG:
            if self.tag:
                return {'object': self.tag}
            if missing_ok:
                return None
            raise tool.APIError('404', 'Release tag does not exist')
        if path == 'git/tags/' + '8' * 40:
            return {'object': {'type': 'commit', 'sha': CANDIDATE}}
        if path == 'releases':
            if method == 'POST':
                release = self.create(data['draft'], data['prerelease'])
                release.update(name=data['name'], body=data['body'])
                return copy.deepcopy(release)
            start = (int(query.get('page', 1)) - 1) * 100
            return copy.deepcopy(self.releases[start:start + 100])
        if path.startswith('releases/assets/') and method == 'DELETE':
            identifier = int(path.split('/')[-1])
            for assets in self.assets.values():
                assets[:] = [item for item in assets if item['id'] != identifier]
            return {}
        if path.startswith('releases/'):
            parts = path.split('/')
            identifier = int(parts[1])
            if len(parts) == 3:
                start = (int(query.get('page', 1)) - 1) * 100
                return copy.deepcopy(self.assets[identifier][start:start + 100])
            if self.before_release_get:
                self.before_release_get()
            return copy.deepcopy(next(item for item in self.releases if item['id'] == identifier))
        raise AssertionError((route, method, data, query))

    def asset_fingerprint(self, repository, asset):
        self.downloads.append(asset['id'])
        data = self.blobs[asset['id']]
        return hashlib.sha256(data).hexdigest(), len(data)

    def upload(self, repository, tag, path):
        release = next(item for item in self.releases if item['tag_name'] == tag)
        assert not any(item['name'] == path.name for item in self.assets[release['id']]), 'Never clobber on upload'
        self.uploads.append(path.name)
        self.add(release, path.name, path.read_bytes())
        if self.fail_upload:
            self.fail_upload = False
            raise RuntimeError('Upload response lost after storing bytes')


class PromotionPolicyTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        for name in promotion.names('0.2.6'):
            (self.directory / name).write_bytes(('verified bytes ' + name).encode())
        self.github = ReleaseStore()

    def promote(self, **options):
        return promotion.promote(self.github, REPOSITORY, TAG, '0.2.6', self.directory, candidate=CANDIDATE, **options)

    def fill(self, release, digest=True):
        for path in sorted(self.directory.iterdir()):
            self.github.add(release, path.name, path.read_bytes(), digest)

    def test_create_draft_without_creating_tag_and_repeat_without_writes(self):
        result = self.promote()
        self.assertTrue(result['release']['draft'])
        self.assertEqual(len(result['files']), 4)
        self.assertEqual(len(self.github.uploads), 4)
        self.assertEqual([entry[0] for entry in self.github.mutations], ['releases'])
        self.promote()
        self.assertEqual(len(self.github.uploads), 4)
        self.assertEqual(len(self.github.mutations), 1)

    def test_partial_upload_resume_preserves_existing_assets_notes_and_other_platforms(self):
        release = self.github.create()
        foreign = self.github.add(release, 'Holder-macos-0.2.6.dmg', b'other platform')
        self.github.fail_upload = True
        with self.assertRaisesRegex(RuntimeError, 'response lost'):
            self.promote()
        first = self.github.assets[release['id']][-1]['id']
        self.promote()
        self.assertEqual(len(self.github.uploads), 4)
        self.assertIn(first, [item['id'] for item in self.github.assets[release['id']]])
        self.assertIn(foreign, self.github.assets[release['id']])
        self.assertEqual((release['name'], release['body']), ('Reviewed title', 'Reviewed notes'))
        self.assertEqual(self.github.mutations, [])

    def test_conflicting_draft_fails_before_any_write_and_explicit_replacement_is_separate(self):
        release = self.github.create()
        file = next(self.directory.iterdir())
        old = self.github.add(release, file.name, b'conflicting bytes')
        with self.assertRaisesRegex(ValueError, 'explicit draft-only'):
            self.promote()
        self.assertFalse(self.github.uploads or self.github.mutations)
        self.promote(replace_draft_assets=True)
        self.assertEqual(self.github.mutations, [(f'releases/assets/{old["id"]}', 'DELETE', None)])
        self.assertEqual(len(self.github.uploads), 4)

    def test_published_stable_is_read_only_even_when_replacement_is_requested(self):
        release = self.github.create(draft=False)
        with self.assertRaisesRegex(ValueError, 'Published release cannot be modified'):
            self.promote(replace_draft_assets=True, allow_published_prerelease=True)
        self.fill(release)
        release['immutable'] = True
        self.promote()
        self.assertFalse(self.github.uploads or self.github.mutations)

    def test_published_prerelease_requires_opt_in_for_missing_files_and_never_replaces_conflicts(self):
        release = self.github.create(draft=False, prerelease=True)
        with self.assertRaisesRegex(ValueError, 'cannot be modified'):
            self.promote()
        self.promote(allow_published_prerelease=True)
        self.github.assets[release['id']][0]['digest'] = 'sha256:' + '0' * 64
        before = len(self.github.uploads)
        with self.assertRaisesRegex(ValueError, 'explicit draft-only'):
            self.promote(allow_published_prerelease=True, replace_draft_assets=True)
        self.assertEqual(len(self.github.uploads), before)

    def test_tag_must_exist_and_match_before_draft_creation(self):
        self.github.tag = None
        with self.assertRaises(tool.APIError):
            self.promote()
        self.github.tag = {'type': 'commit', 'sha': 'b' * 40}
        with self.assertRaisesRegex(ValueError, 'differs from accepted'):
            self.promote()
        self.assertFalse(self.github.mutations)
        self.github.tag = {'type': 'tag', 'sha': '8' * 40}
        self.promote()

    def test_release_publication_race_is_rechecked_before_writing(self):
        release = self.github.create()
        self.github.before_release_get = lambda: release.update(draft=False)
        with self.assertRaisesRegex(ValueError, 'cannot be modified'):
            self.promote()
        self.assertFalse(self.github.uploads or self.github.mutations)

    def test_asset_without_digest_is_checked_from_downloaded_bytes(self):
        release = self.github.create()
        self.fill(release, digest=False)
        self.promote()
        self.assertTrue(self.github.downloads)
        self.assertFalse(self.github.uploads)
        self.github.blobs[self.github.assets[release['id']][0]['id']] = b'changed'
        with self.assertRaisesRegex(ValueError, 'explicit draft-only'):
            self.promote()

    def test_paginated_releases_and_assets_and_duplicate_names(self):
        for number in range(101):
            self.github.create()['tag_name'] = 'unrelated-' + str(number)
        release = self.github.create()
        for number in range(101):
            self.github.add(release, 'unrelated-' + str(number), b'other')
        self.fill(release)
        result = self.promote()
        self.assertEqual(result['release']['id'], release['id'])
        self.assertFalse(self.github.uploads)
        self.github.assets[release['id']].append(copy.deepcopy(self.github.assets[release['id']][-1]))
        with self.assertRaisesRegex(ValueError, 'Duplicate release asset'):
            self.promote()


class PromotionGitHub(signing.SigningGitHub):
    def __init__(self, directory):
        super().__init__(directory)
        self.store = ReleaseStore()

    def api(self, route, method='GET', data=None, missing_ok=False, **query):
        path = urlsplit(route).path.removeprefix('repos/' + REPOSITORY + '/')
        if path.startswith(('releases', 'git/ref/tags/', 'git/tags/')):
            return self.store.api(route, method, data, missing_ok, **query)
        try:
            return super().api(route, method, data, missing_ok, **query)
        finally:
            if 'promote-linux-appimage.yml/dispatches' in path and 130 in self.runs:
                self.artifacts[130] = []

    def linux_promotion_status(self, record, release_id=None):
        return promotion.observe(self.store, REPOSITORY, TAG, tool.linux_release_files(record), CANDIDATE, release_id)


class PromotionCoordinatorTests(unittest.TestCase):
    reload = preparation.CoordinatorTests.reload
    reach_staging = preparation.CoordinatorTests.reach_staging

    def setUp(self):
        preparation.CoordinatorTests.setUp(self)
        self.github = PromotionGitHub(self.directory.name)
        self.coordinator = tool.Coordinator(self.path, self.record, self.github)
        self.reach_staging()
        self.github.finish('linux')
        self.coordinator.advance_signing()
        self.github.finish('linux_signing')
        self.coordinator.advance_signing()
        self.payload = Path(self.directory.name) / 'release-payload'
        self.payload.mkdir()
        item = self.record['linux_signed']['artifact']
        for name in promotion.names('0.2.6'):
            data = self.github.payloads.files[item['id']][name]
            (self.payload / name).write_bytes(data.encode() if isinstance(data, str) else data)

    def deliver(self):
        return promotion.promote(self.github.store, REPOSITORY, TAG, '0.2.6', self.payload, candidate=CANDIDATE)

    def test_full_promotion_and_recheck_preserve_signed_pins(self):
        self.coordinator.advance_linux_promotion()
        step = self.record['steps']['linux_promotion']
        self.assertEqual(step['inputs']['candidate_commit'], CANDIDATE)
        self.assertEqual(step['inputs']['replace_draft_assets'], 'false')
        self.assertEqual(step['inputs']['signing_artifact_sha256'], self.record['linux_signed']['archive_sha256'])
        self.deliver()
        self.github.finish('linux_promotion')
        self.coordinator.advance_linux_promotion()
        self.assertEqual(self.record['status'], 'linux_promoted')
        self.assertEqual(len(self.record['linux_promoted']['receipt']['files']), 4)
        self.github.store.releases[0]['draft'] = False
        self.reload()
        self.coordinator.advance_linux_promotion()
        self.assertFalse(self.record['linux_promoted']['last_check']['draft'])
        self.assertEqual(len(self.github.dispatches()), 7)
        self.assertEqual(len(self.github.store.uploads), 4)

    def test_partial_delivery_failed_job_retry_and_completion_after_cleanup_failure(self):
        self.coordinator.advance_linux_promotion()
        self.github.store.fail_upload = True
        with self.assertRaises(RuntimeError):
            self.deliver()
        self.github.finish('linux_promotion', 'failure')
        with self.assertRaisesRegex(ValueError, 'ended failure'):
            self.coordinator.advance_linux_promotion()
        self.coordinator.retry('linux_promotion')
        self.deliver()
        self.github.finish('linux_promotion', 'failure')  # Assets succeeded; later cleanup failed.
        self.coordinator.advance_linux_promotion()
        self.assertEqual(self.record['status'], 'linux_promoted')
        self.assertEqual(self.record['steps']['linux_promotion']['conclusion'], 'failure')
        self.assertEqual(len(self.github.store.uploads), 4)
        self.assertEqual(len(self.github.dispatches()), 7)

    def test_existing_exact_assets_can_be_adopted_without_a_run_even_after_input_expiry(self):
        self.deliver()
        with patch.object(tool, 'now', return_value=datetime(2027, 1, 1, tzinfo=timezone.utc)):
            self.coordinator.advance_linux_promotion()
        self.assertIsNone(self.record['linux_promoted']['receipt']['workflow_run_id'])
        self.assertEqual(len(self.github.dispatches()), 6)

    def test_lost_dispatch_does_not_create_another_promotion_run(self):
        self.github.lost_dispatch = 'linux_promotion'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance_linux_promotion()
        self.reload()
        self.coordinator.advance_linux_promotion()
        self.assertEqual(self.record['steps']['linux_promotion']['run_id'], '130')
        self.assertEqual(len(self.github.dispatches()), 7)

    def test_expired_missing_inputs_and_wrong_tag_fail_before_dispatch(self):
        before = len(self.github.dispatches())
        with patch.object(tool, 'now', return_value=datetime(2027, 1, 1, tzinfo=timezone.utc)), \
                self.assertRaisesRegex(ValueError, 'expired before promotion'):
            self.coordinator.advance_linux_promotion()
        self.github.store.tag = {'type': 'commit', 'sha': 'b' * 40}
        with self.assertRaisesRegex(ValueError, 'differs from prepared candidate'):
            self.coordinator.advance_linux_promotion()
        self.assertEqual(len(self.github.dispatches()), before)

    def test_accepted_asset_replacement_and_receipt_edits_are_rejected(self):
        self.coordinator.advance_linux_promotion()
        self.deliver()
        self.github.finish('linux_promotion')
        self.coordinator.advance_linux_promotion()
        self.github.store.assets[1][0]['id'] += 1000
        with self.assertRaisesRegex(ValueError, 'assets or source changed'):
            self.coordinator.advance_linux_promotion()
        self.github.store.assets[1][0]['id'] -= 1000
        self.record['linux_promoted']['receipt']['source']['artifact_id'] = 999
        with self.assertRaisesRegex(ValueError, 'receipt changed'):
            self.coordinator.advance_linux_promotion()
        with self.assertRaisesRegex(ValueError, 'already accepted'):
            self.coordinator.retry('linux_promotion')

    def test_cli_wait_reaches_destination_state_without_advancing_siblings(self):
        self.deliver()
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', [
                'release.py', 'promote-linux', '--record', str(self.path), '--wait']), patch('builtins.print'), \
                patch.object(tool.time, 'sleep', side_effect=AssertionError('already delivered')):
            tool.main()
        self.reload()
        self.assertEqual(self.record['status'], 'linux_promoted')
        self.assertEqual(self.github.runs[40]['status'], 'queued')

    def test_worker_downloads_exact_signed_source_and_rejects_a_different_candidate(self):
        linux = tool.linux_tool
        bundle = self.record['linux_signed']
        output = Path(self.directory.name) / 'worker-signed'
        arguments = ['linux-candidate.py', '--signed', '--repository', REPOSITORY, '--run-id', bundle['run_id'],
            '--artifact-id', str(bundle['artifact']['id']), '--archive-sha256', bundle['archive_sha256'],
            '--version', '0.2.6', '--output', str(output), '--candidate', CANDIDATE]
        with patch.object(linux.manifest_tool, 'GitHub', return_value=self.github.payloads), \
                patch.object(linux, 'datetime', wraps=datetime) as clock:
            clock.now.return_value = NOW
            with patch.object(sys, 'argv', arguments):
                linux.main()
            self.assertEqual((output / 'Holder-0.2.6-x86_64.AppImage').read_bytes(), b'signed output')
            self.assertEqual(len(list(output.iterdir())), 7)
            arguments[arguments.index('--output') + 1] = str(output.with_name('wrong-source'))
            arguments[-1] = 'b' * 40
            with patch.object(sys, 'argv', arguments), self.assertRaisesRegex(ValueError, 'expected candidate source'):
                linux.main()
        self.assertFalse(output.with_name('wrong-source').exists())


if __name__ == '__main__':
    unittest.main()
