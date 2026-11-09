import base64
import copy
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from urllib.parse import urlsplit
import zipfile

import test_linux_promotion as linux_tests
import test_python_release_sets as package_tests
import test_release_coordinator as preparation
from test_release_coordinator import CANDIDATE, NOW, REPOSITORY, tool

sdk = tool.sdk_release
DESTINATION = sdk.SDK_REPOSITORY
TAG = 'holder-v0.2.6'
METADATA = '7' * 40
SDK_TAG = 'f' * 40


class SourceFixture(preparation.fixtures.Fixture):
    def __init__(self, directory):
        super().__init__(directory)
        self.tag = {'type': 'commit', 'sha': CANDIDATE}
        self.prepared_document = None
        for asset in self.selection['assets']:
            manifest = {key: asset[key] for key in ('commit', 'version', 'platform', 'architecture', 'build_type')}
            asset.pop('version')  # The production run selection keeps version at the top level.
            if asset['platform'] == 'windows':
                asset['name'] = 'windows.zip'
                content = io.BytesIO()
                with zipfile.ZipFile(content, 'w') as archive:
                    archive.writestr('libholder-sdk/libholder-manifest.json', json.dumps(manifest))
                data = content.getvalue()
            else:
                data = package_tests.tar_bytes({'libholder-sdk/libholder-manifest.json': json.dumps(manifest)})
            asset.update(size=len(data), sha256=hashlib.sha256(data).hexdigest())
            self.add(20, f'core-sdk-{asset["platform"]}-Release', {asset['name']: data})
        item = self.item('core-selection')
        self.files[item['id']]['core-selection.json'] = json.dumps(self.selection)
        for item in self.items[20]:
            if item['name'] == 'core-selection' or item['name'].startswith('core-sdk-'):
                self.read(REPOSITORY, item, [])

    def request(self, route, **query):
        path = urlsplit(route).path.removeprefix('repos/' + REPOSITORY + '/')
        if path == 'git/ref/tags/' + TAG:
            if not self.tag:
                raise tool.APIError('404', 'Missing reviewed source tag')
            return {'object': copy.deepcopy(self.tag)}
        if path.startswith('contents/release/sdk-manifests/'):
            return {'type': 'file', 'encoding': 'base64',
                    'content': base64.b64encode(sdk.encode(self.prepared_document).encode()).decode()}
        return super().request(route, **query)

    def document(self):
        names = {'core-selection', 'core-sdk-linux-Release', 'core-sdk-macos-Release', 'core-sdk-windows-Release'}
        selection = self.item('core-selection')
        return {'schema': 1, 'repository': REPOSITORY, 'candidate': CANDIDATE, 'version': '0.2.6',
            'backend': {'workflow': 'daemon-ci.yml', 'workflow_commit': CANDIDATE, 'run_id': '20', 'attempt': 1,
                'artifacts': [{key: item.get(key) for key in ('id', 'name', 'digest', 'size_in_bytes', 'expires_at')}
                    for item in self.items[20] if item['name'] in names]},
            'core_selection': copy.deepcopy(self.selection),
            'core_pin': {'selection_artifact_id': str(selection['id']), 'selection_artifact_sha256': selection['digest'][7:]}}


class SDKStore(linux_tests.ReleaseStore):
    def __init__(self):
        super().__init__()
        self.publish_response_lost = False
        self.sdk_tag = SDK_TAG

    def create(self, draft=True, prerelease=False):
        result = super().create(draft, prerelease)
        result['html_url'] = f'https://github.com/{DESTINATION}/releases/tag/{TAG}'
        return result

    def api(self, route, method='GET', data=None, **query):
        assert route.startswith('repos/' + DESTINATION + '/'), 'Destination token must only access SDK repository'
        path = urlsplit(route).path.removeprefix('repos/' + DESTINATION + '/')
        if path == 'git/ref/tags/' + TAG:
            return {'object': {'type': 'commit', 'sha': self.sdk_tag}}
        if method == 'PATCH':
            self.mutations.append((path, method, copy.deepcopy(data)))
            release = next(item for item in self.releases if item['id'] == int(path.split('/')[-1]))
            release.update({key: value for key, value in data.items() if key in ('draft', 'prerelease')})
            if self.publish_response_lost:
                self.publish_response_lost = False
                raise RuntimeError('Publication response lost after publishing')
            return copy.deepcopy(release)
        translated = route.replace('repos/' + DESTINATION + '/', 'repos/' + REPOSITORY + '/', 1)
        return super().api(translated, method, data, **query)

    def asset_contents(self, repository, asset):
        assert repository == DESTINATION
        data = self.blobs[asset['id']]
        sdk.require(len(data) == asset['size'] and
            (not asset.get('digest') or asset['digest'] == 'sha256:' + hashlib.sha256(data).hexdigest()), 'Metadata digest differs')
        return data


class SDKPublicationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = SourceFixture(self.root)
        self.store = SDKStore()
        self.inputs = sdk.fetch(REPOSITORY, '20', '0.2.6', self.root / 'prepared', self.source,
                                self.source.document(), clock=NOW)
        self.directory = self.root / 'prepared/core-sdk'

    def publish(self):
        return sdk.publish(self.store, sdk.ReadAPI(self.source), self.inputs, self.directory)

    def test_pinned_archives_and_original_schema_publish_without_building(self):
        self.assertEqual(len(self.inputs['artifacts']), 4)
        self.assertEqual(len(list(self.directory.iterdir())), 3)
        result = self.publish()
        self.assertFalse(result['release']['draft'])
        self.assertEqual(result['candidate'], CANDIDATE)
        self.assertEqual(result['sdk_repository_tag_commit'], SDK_TAG)
        self.assertNotEqual(result['candidate'], SDK_TAG)
        self.assertEqual(result['index']['schema_version'], 1)
        self.assertTrue(all(asset['commit'] == preparation.fixtures.SDK_COMMIT for asset in result['index']['assets']))
        self.assertEqual(len(self.store.uploads), 4)
        mutations = copy.deepcopy(self.store.mutations)
        self.publish()
        self.assertEqual(self.store.mutations, mutations)
        self.assertEqual(len(self.store.uploads), 4)

    def test_partial_draft_resume_preserves_matching_assets_and_notes(self):
        self.store.fail_upload = True
        with self.assertRaisesRegex(RuntimeError, 'Upload response lost'):
            self.publish()
        release = self.store.releases[0]
        original = self.store.assets[release['id']][0]['id']
        release.update(name='Reviewed SDK title', body='Reviewed SDK notes')
        self.publish()
        self.assertEqual(len(self.store.uploads), 4)
        self.assertIn(original, [item['id'] for item in self.store.assets[release['id']]])
        self.assertEqual((release['name'], release['body']), ('Reviewed SDK title', 'Reviewed SDK notes'))
        self.assertFalse(any(method == 'DELETE' for _, method, _ in self.store.mutations))

    def test_publish_response_loss_is_recovered_read_only(self):
        self.store.publish_response_lost = True
        with self.assertRaisesRegex(RuntimeError, 'response lost'):
            self.publish()
        before = copy.deepcopy(self.store.mutations)
        self.publish()
        self.assertEqual(self.store.mutations, before)

    def test_conflicting_index_or_archive_and_unexpected_files_fail_before_writes(self):
        release = self.store.create()
        expected = sdk.make_index(self.inputs['core_selection'], REPOSITORY, '20')
        index = {**expected, 'commit': 'b' * 40}
        self.store.add(release, 'sdk-index.json', sdk.encode(index).encode())
        with self.assertRaisesRegex(ValueError, 'index differs'):
            self.publish()
        self.store.assets[1].clear()
        asset = self.inputs['core_selection']['assets'][0]
        self.store.add(release, asset['name'], b'conflict')
        with self.assertRaisesRegex(ValueError, 'archive conflicts'):
            self.publish()
        self.store.assets[1].clear()
        self.store.add(release, 'other-version.zip', b'other')
        with self.assertRaisesRegex(ValueError, 'unexpected assets'):
            self.publish()
        self.assertFalse(self.store.mutations or self.store.uploads)

    def test_incomplete_published_release_is_not_repaired(self):
        self.store.create(draft=False)
        with self.assertRaisesRegex(ValueError, 'Published SDK release is incomplete'):
            self.publish()
        self.assertFalse(self.store.uploads or self.store.mutations)

    def test_legacy_index_format_is_retained_when_content_matches(self):
        expected = sdk.make_index(self.inputs['core_selection'], REPOSITORY, '20')
        release = self.store.create()
        original = self.store.add(release, 'sdk-index.json', json.dumps(expected, sort_keys=True).encode())
        result = self.publish()
        self.assertIn(original['id'], [item['asset_id'] for item in result['files']])
        self.assertNotIn('sdk-index.json', self.store.uploads)
        self.assertEqual(len(self.store.uploads), 3)

    def test_wrong_source_tag_and_changed_local_archive_prevent_publication(self):
        self.source.tag = {'type': 'commit', 'sha': 'b' * 40}
        with self.assertRaisesRegex(ValueError, 'differs from accepted candidate'):
            self.publish()
        self.source.tag = {'type': 'commit', 'sha': CANDIDATE}
        archive = self.directory / self.inputs['core_selection']['assets'][0]['name']
        archive.write_bytes(archive.read_bytes() + b'changed')
        with self.assertRaisesRegex(ValueError, 'differ from selection'):
            self.publish()
        self.assertFalse(self.store.uploads or self.store.mutations)

    def test_worker_rejects_pin_changes_and_source_attempt_drift(self):
        document = self.source.document()
        document['backend']['artifacts'][0]['id'] = 999
        with self.assertRaisesRegex(ValueError, 'Pinned SDK artifact changed'):
            sdk.fetch(REPOSITORY, '20', '0.2.6', self.root / 'wrong-pin', self.source, document, clock=NOW)
        original = self.source.read

        def read(*args, **kwargs):
            result = original(*args, **kwargs)
            self.source.runs[20]['run_attempt'] += 1
            return result
        self.source.read = read
        with self.assertRaisesRegex(ValueError, 'retried during download'):
            sdk.fetch(REPOSITORY, '20', '0.2.6', self.root / 'drift', self.source, clock=NOW)
        self.assertFalse((self.root / 'drift').exists())

    def test_prepared_metadata_path_and_ref_are_strict(self):
        for path, ref in (('../outside.json', METADATA), ('sdk-manifests/example.json', 'main')):
            with self.assertRaises(ValueError):
                sdk.prepared(self.source, REPOSITORY, path, ref)
        self.source.prepared_document = self.source.document()
        self.assertEqual(sdk.prepared(self.source, REPOSITORY, 'sdk-manifests/example.json', METADATA), self.source.document())

    def test_wrong_configuration_and_internal_sdk_producer_are_rejected(self):
        asset = self.source.selection['assets'][0]
        item = self.source.item('core-selection')
        asset['build_type'] = 'RelWithDebInfo'
        self.source.files[item['id']]['core-selection.json'] = json.dumps(self.source.selection)
        with self.assertRaisesRegex(ValueError, 'three Release platform configurations'):
            sdk.fetch(REPOSITORY, '20', '0.2.6', self.root / 'wrong-config', self.source, clock=NOW)
        asset['build_type'] = 'Release'
        manifest = {key: asset[key] for key in ('commit', 'platform', 'architecture', 'build_type')}
        manifest.update(commit='d' * 40, version='0.2.6')
        data = package_tests.tar_bytes({'libholder-sdk/libholder-manifest.json': json.dumps(manifest)})
        asset.update(size=len(data), sha256=hashlib.sha256(data).hexdigest())
        wrapper = self.source.item('core-sdk-linux-Release')
        self.source.files[wrapper['id']][asset['name']] = data
        self.source.files[item['id']]['core-selection.json'] = json.dumps(self.source.selection)
        with self.assertRaisesRegex(ValueError, 'SDK manifests, configurations or bytes differ'):
            sdk.fetch(REPOSITORY, '20', '0.2.6', self.root / 'wrong-producer', self.source, clock=NOW)
        self.assertFalse((self.root / 'wrong-producer').exists())

    def test_source_tag_is_rechecked_during_uploads_before_publication(self):
        upload = self.store.upload

        def move_source_tag(*args):
            upload(*args)
            self.source.tag = {'type': 'commit', 'sha': 'b' * 40}
        self.store.upload = move_source_tag
        with self.assertRaisesRegex(ValueError, 'differs from accepted candidate'):
            self.publish()
        self.assertTrue(self.store.releases[0]['draft'])
        self.assertEqual(len(self.store.uploads), 1)
        self.assertFalse(any(method == 'PATCH' for _, method, _ in self.store.mutations))

    def test_cached_producer_tree_or_version_input_must_match(self):
        producer = self.source.selection['assets'][0]['commit']
        tree = self.source.tree[producer]
        self.source.tree[producer] = 'd' * 40
        with self.assertRaisesRegex(ValueError, 'Cached SDK producer source/version differs'):
            sdk.fetch(REPOSITORY, '20', '0.2.6', self.root / 'wrong-tree', self.source, clock=NOW)
        self.source.tree[producer] = tree
        self.source.versions[producer] = 'different-version-input'
        with self.assertRaisesRegex(ValueError, 'Cached SDK producer source/version differs'):
            sdk.fetch(REPOSITORY, '20', '0.2.6', self.root / 'wrong-version', self.source, clock=NOW)


class SDKCoordinatorGitHub(preparation.FakeGitHub):
    def __init__(self, directory):
        super().__init__(directory)
        self.payloads = SourceFixture(directory)
        self.artifacts = self.payloads.items
        self.store = SDKStore()

    def api(self, route, method='GET', data=None, missing_ok=False, **query):
        if route.startswith(f'repos/{REPOSITORY}/git/ref/tags/'):
            if self.payloads.tag is None and missing_ok:
                return None
            return self.payloads.request(route)
        path = urlsplit(route).path.removeprefix('repos/' + REPOSITORY + '/')
        if path == 'git/commits' and any(name.startswith('release/sdk-manifests/') for name in self.tree_files):
            self.mutations.append((route, method, copy.deepcopy(data)))
            self.files[METADATA] = copy.deepcopy(self.tree_files)
            return {'sha': METADATA}
        if path.startswith('contents/release/sdk-manifests/'):
            return {'content': base64.b64encode(self.files[METADATA][path.removeprefix('contents/')].encode()).decode()}
        try:
            return super().api(route, method, data, missing_ok, **query)
        finally:
            if 'promote-core-sdk.yml/dispatches' in path and 140 in self.runs:
                self.artifacts[140] = []

    def sdk_publication_status(self, record, release_id=None):
        document = record['sdk_publication_inputs']['document']
        return sdk.observe(self.store, sdk.make_index(document['core_selection'], REPOSITORY, '20'), release_id)


class SDKCoordinatorTests(unittest.TestCase):
    reload = preparation.CoordinatorTests.reload
    reach_staging = preparation.CoordinatorTests.reach_staging

    def setUp(self):
        preparation.CoordinatorTests.setUp(self)
        self.github = SDKCoordinatorGitHub(self.directory.name)
        self.coordinator = tool.Coordinator(self.path, self.record, self.github)
        self.reach_staging()

    def deliver(self):
        self.github.payloads.runs.update(copy.deepcopy(self.github.runs))
        document = self.record['sdk_publication_inputs']['document']
        output = Path(self.directory.name) / ('fetch-' + str(len(self.github.store.uploads)))
        inputs = sdk.fetch(REPOSITORY, '20', '0.2.6', output, self.github.payloads, document, clock=NOW)
        return sdk.publish(self.github.store, sdk.ReadAPI(self.github.payloads), inputs, output / 'core-sdk')

    def test_complete_sdk_publication_and_resume_without_another_run(self):
        self.coordinator.advance_sdk_publication()
        step = self.record['steps']['core_sdk_publication']
        self.assertEqual(step['ref'], 'main')
        self.assertEqual(step['inputs']['prepared_sdk_ref'], METADATA)
        self.deliver()
        self.github.finish('core_sdk_publication')
        self.coordinator.advance_sdk_publication()
        self.assertEqual(self.record['status'], 'core_sdk_published')
        self.assertNotEqual(self.record['core_sdk_published']['receipt']['sdk_repository_tag_commit'], CANDIDATE)
        self.reload()
        self.coordinator.advance_sdk_publication()
        self.assertEqual(len(self.github.dispatches()), 6)
        self.assertEqual(len(self.github.store.uploads), 4)
        self.assertIn('Producer commit', tool.summary(self.record))

    def test_existing_publication_adoption_survives_input_expiry_without_metadata_commit(self):
        self.coordinator.prepare_sdk_publication()
        self.deliver()
        before = len(self.github.mutations)
        with patch.object(tool, 'now', return_value=datetime(2027, 1, 1, tzinfo=timezone.utc)):
            self.coordinator.advance_sdk_publication()
        self.assertEqual(len(self.github.mutations), before)
        self.assertNotIn('commit', self.record['sdk_publication_inputs'])
        self.assertIsNone(self.record['core_sdk_published']['receipt']['workflow_run_id'])

    def test_partial_upload_retry_and_later_workflow_failure_preserve_completion(self):
        self.coordinator.advance_sdk_publication()
        self.github.store.fail_upload = True
        with self.assertRaises(RuntimeError):
            self.deliver()
        self.github.finish('core_sdk_publication', 'failure')
        with self.assertRaisesRegex(ValueError, 'ended failure'):
            self.coordinator.advance_sdk_publication()
        self.coordinator.retry('core_sdk_publication')
        self.deliver()
        self.github.finish('core_sdk_publication', 'failure')
        self.coordinator.advance_sdk_publication()
        self.assertEqual(self.record['status'], 'core_sdk_published')
        self.assertEqual(self.record['steps']['core_sdk_publication']['conclusion'], 'failure')
        self.assertEqual(len(self.github.store.uploads), 4)

    def test_lost_dispatch_reconciles_without_duplicate(self):
        self.github.lost_dispatch = 'core_sdk_publication'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance_sdk_publication()
        self.reload()
        self.coordinator.advance_sdk_publication()
        self.assertEqual(self.record['steps']['core_sdk_publication']['run_id'], '140')
        self.assertEqual(len(self.github.dispatches()), 6)

    def test_wrong_tag_expired_input_and_changed_input_snapshot_stop(self):
        self.github.payloads.tag = None
        with self.assertRaisesRegex(ValueError, 'reviewed release tag'):
            self.coordinator.advance_sdk_publication()
        self.github.payloads.tag = {'type': 'commit', 'sha': CANDIDATE}
        with patch.object(tool, 'now', return_value=datetime(2027, 1, 1, tzinfo=timezone.utc)), \
                self.assertRaisesRegex(ValueError, 'inputs expired'):
            self.coordinator.advance_sdk_publication()
        self.record['sdk_publication_inputs']['document']['core_selection']['assets'][0]['sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'Saved SDK publication inputs changed'):
            self.coordinator.advance_sdk_publication()
        self.assertEqual(len(self.github.dispatches()), 5)

    def test_destination_id_or_sdk_tag_drift_and_receipt_corruption_are_rejected(self):
        self.coordinator.advance_sdk_publication()
        self.deliver()
        self.github.finish('core_sdk_publication')
        self.coordinator.advance_sdk_publication()
        self.github.store.assets[1][0]['id'] += 1000
        with self.assertRaisesRegex(ValueError, 'Accepted SDK destination'):
            self.coordinator.advance_sdk_publication()
        self.github.store.assets[1][0]['id'] -= 1000
        self.github.store.sdk_tag = 'e' * 40
        with self.assertRaisesRegex(ValueError, 'Accepted SDK destination'):
            self.coordinator.advance_sdk_publication()
        self.github.store.sdk_tag = SDK_TAG
        self.record['core_sdk_published']['receipt']['candidate'] = 'b' * 40
        with self.assertRaisesRegex(ValueError, 'receipt changed'):
            self.coordinator.advance_sdk_publication()

    def test_cli_wait_reaches_sdk_destination_state(self):
        self.coordinator.prepare_sdk_publication()
        self.deliver()
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', [
                'release.py', 'publish-sdk', '--record', str(self.path), '--wait']), patch('builtins.print'), \
                patch.object(tool.time, 'sleep', side_effect=AssertionError('already published')):
            tool.main()
        self.reload()
        self.assertEqual(self.record['status'], 'core_sdk_published')


if __name__ == '__main__':
    unittest.main()
