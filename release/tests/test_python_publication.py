import base64
import copy
from datetime import datetime, timezone
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from urllib.parse import parse_qs, urlsplit

import test_python_release_sets as preparation
from test_release_coordinator import CANDIDATE, NOW, REPOSITORY, tool

publisher = tool.publisher_tool
METADATA = '9' * 40


def index_response(files):
    return {'urls': [{'filename': file['name'], 'size': file['size'], 'digests': {'sha256': file['sha256']},
                      'yanked': False} for file in files]}


class PublicationGitHub(preparation.PackageGitHub):
    def __init__(self, directory):
        super().__init__(directory)
        self.tag = {'type': 'commit', 'sha': CANDIDATE}
        self.indexes = {}
        self.receipt_override = None

    def api(self, route, method='GET', data=None, missing_ok=False, **query):
        path = urlsplit(route).path.removeprefix('repos/' + REPOSITORY + '/')
        if path.startswith('git/ref/tags/'):
            return {'object': self.tag} if self.tag else None
        if path == 'git/tags/' + '8' * 40:
            return {'object': {'type': 'commit', 'sha': CANDIDATE}}
        if path == 'git/commits' and any(name.startswith('release/python-manifests/') for name in self.tree_files):
            self.mutations.append((route, method, copy.deepcopy(data)))
            self.files[METADATA] = copy.deepcopy(self.tree_files)
            return {'sha': METADATA}
        if path.startswith('contents/release/python-manifests/'):
            reference = parse_qs(urlsplit(route).query).get('ref', [METADATA])[0]
            return {'content': base64.b64encode(self.files[reference][path.removeprefix('contents/')].encode()).decode()}
        result = super().api(route, method, data, missing_ok, **query)
        if method == 'POST' and path.endswith('/dispatches') and data['inputs'].get('destination') in publisher.INDEXES:
            identifier = result['workflow_run_id']
            package = 'kit' if 'release-kit-' in path else 'python'
            destination = data['inputs']['destination']
            self.artifacts[identifier] = [{'id': identifier * 100, 'name': f'{package}-{destination}-publication',
                'digest': 'sha256:' + 'c' * 64, 'size_in_bytes': 100, 'expired': False,
                'expires_at': '2026-11-15T00:00:00Z', 'workflow_run': {'id': identifier, 'head_sha': CANDIDATE}}]
        return result

    def finish_publication(self, name, conclusion='success'):
        request = next(step for step in self.runs.values() if name in step['display_title'])
        request.update(status='completed', conclusion=conclusion)

    def publication(self, record, name):
        package, destination = tool.PUBLICATIONS[name]
        source = record['python_packages'][package]
        return self.receipt_override or {'repository': REPOSITORY, 'candidate': CANDIDATE, 'version': '0.2.6',
            'package': package, 'destination': destination, 'prepared_ref': METADATA,
            'source_run_id': source['run_id'], 'source_artifact_id': source['distribution_artifact_id'],
            'files': source['files'], 'index_url': publisher.index(package, '0.2.6', destination),
            'verified_at': NOW.isoformat()}

    def publication_status(self, receipt):
        publisher.verify(receipt, request=lambda url: self.indexes.get(url), attempts=1)


class PublicationCoordinatorTests(unittest.TestCase):
    reload = preparation.PackageCoordinatorTests.reload
    reach_staging = preparation.PackageCoordinatorTests.reach_staging
    reach_packages = preparation.PackageCoordinatorTests.reach_packages
    finish_packages = preparation.PackageCoordinatorTests.finish_packages

    def setUp(self):
        preparation.PackageCoordinatorTests.setUp(self)
        clock = patch.object(publisher.packages.manifest_tool, 'datetime', wraps=datetime)
        clock.start().now.return_value = NOW
        self.addCleanup(clock.stop)
        self.github = PublicationGitHub(self.directory.name)
        self.coordinator = tool.Coordinator(self.path, self.record, self.github)
        self.finish_packages()

    def publish(self, package='kit', destination='pypi'):
        self.coordinator.advance_publication(package, destination)
        self.github.finish_publication(f'{package}_{destination}')
        self.github.indexes[publisher.index(package, '0.2.6', destination)] = index_response(
            self.record['python_packages'][package]['files'])
        self.coordinator.advance_publication(package, destination)

    def test_publish_and_resume_record_exact_inputs_and_keep_builds(self):
        self.publish()
        self.assertEqual(self.record['status'], 'python_publication_completed')
        step = self.record['steps']['kit_pypi']
        self.assertEqual(step['ref'], 'holder-v0.2.6')
        self.assertEqual(step['inputs']['prepared_release_ref'], METADATA)
        self.assertNotIn('core_build_type', step['inputs'])
        self.assertEqual(len(self.github.dispatches()), 8)
        self.reload()
        self.coordinator.advance_publication('kit', 'pypi')
        self.assertEqual(len(self.github.dispatches()), 8)
        self.assertEqual(self.record['publications']['kit_pypi']['receipt']['source_run_id'], '60')
        self.assertFalse(any('/git/ref/tags' in route and method != 'GET' for route, method, _ in self.github.mutations))

    def test_destinations_and_packages_complete_independently(self):
        self.publish('kit', 'testpypi')
        self.publish('kit', 'pypi')
        self.publish('python', 'pypi')
        self.assertEqual(set(self.record['publications']), {'kit_testpypi', 'kit_pypi', 'python_pypi'})
        self.assertEqual(len(self.github.dispatches()), 10)

    def test_tag_must_exist_and_point_to_candidate(self):
        before = len(self.github.mutations)
        self.github.tag = None
        with self.assertRaisesRegex(ValueError, 'reviewed release tag'):
            self.coordinator.advance_publication('kit', 'pypi')
        self.github.tag = {'type': 'commit', 'sha': 'b' * 40}
        with self.assertRaisesRegex(ValueError, 'differs from prepared candidate'):
            self.coordinator.advance_publication('kit', 'pypi')
        self.assertEqual(len(self.github.mutations), before)
        self.github.tag = {'type': 'tag', 'sha': '8' * 40}
        self.coordinator.advance_publication('kit', 'pypi')
        self.assertEqual(self.record['status'], 'publishing_python')

    def test_changed_prepared_set_or_metadata_branch_is_rejected(self):
        self.coordinator.advance_publication('kit', 'pypi')
        self.github.refs[self.record['python_publication_inputs']['branch']] = '7' * 40
        self.github.files['7' * 40] = self.github.files[METADATA]
        with self.assertRaisesRegex(ValueError, 'Publication branch moved'):
            self.coordinator.advance_publication('kit', 'pypi')
        self.record['python_packages']['kit']['files'][0]['sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'unchanged prepared'):
            self.coordinator.advance_publication('kit', 'pypi')

    def test_failed_publication_retry_and_index_completion(self):
        self.coordinator.advance_publication('kit', 'pypi')
        self.github.finish_publication('kit_pypi', 'failure')
        with self.assertRaisesRegex(ValueError, 'ended failure'):
            self.coordinator.advance_publication('kit', 'pypi')
        self.coordinator.retry('kit_pypi')
        self.github.finish_publication('kit_pypi')
        with self.assertRaisesRegex(ValueError, 'index is incomplete'):
            self.coordinator.advance_publication('kit', 'pypi')
        self.github.indexes[publisher.index('kit', '0.2.6', 'pypi')] = index_response(self.record['python_packages']['kit']['files'])
        self.coordinator.advance_publication('kit', 'pypi')
        self.assertEqual(len(self.github.dispatches()), 8)
        self.assertEqual(self.record['publications']['kit_pypi']['attempt'], 2)

    def test_receipt_mismatch_and_later_index_conflict_are_rejected(self):
        self.coordinator.advance_publication('kit', 'pypi')
        self.github.finish_publication('kit_pypi')
        receipt = self.github.publication(self.record, 'kit_pypi')
        self.github.receipt_override = {**receipt, 'source_artifact_id': 123456}
        with self.assertRaisesRegex(ValueError, 'receipt differs'):
            self.coordinator.advance_publication('kit', 'pypi')
        self.github.receipt_override = None
        url = publisher.index('kit', '0.2.6', 'pypi')
        self.github.indexes[url] = index_response(self.record['python_packages']['kit']['files'])
        self.coordinator.advance_publication('kit', 'pypi')
        self.github.indexes[url]['urls'][0]['digests']['sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'differs from prepared bytes'):
            self.coordinator.advance_publication('kit', 'pypi')

    def test_completed_destination_survives_artifact_expiry_but_unpublished_does_not(self):
        self.publish()
        with patch.object(tool, 'now', return_value=datetime(2026, 11, 16, tzinfo=timezone.utc)):
            self.coordinator.advance_publication('kit', 'pypi')
            with self.assertRaisesRegex(ValueError, 'expired before publication'):
                self.coordinator.advance_publication('python', 'pypi')

    def test_uncertain_publication_dispatch_is_reconciled_without_another_upload_run(self):
        self.github.lost_dispatch = 'kit_pypi'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance_publication('kit', 'pypi')
        self.reload()
        self.coordinator.advance_publication('kit', 'pypi')
        self.assertEqual(self.record['steps']['kit_pypi']['run_id'], '80')
        self.assertEqual(len(self.github.dispatches()), 8)

    def test_cli_requires_explicit_publication_target(self):
        before = len(self.github.mutations)
        for arguments in ([], ['--package', 'kit'], ['--destination', 'pypi']):
            with patch.object(sys, 'argv', ['release.py', 'publish-python', '--record', str(self.path), *arguments]), \
                    patch('sys.stderr', io.StringIO()), self.assertRaises(SystemExit):
                tool.main()
        self.assertEqual(len(self.github.mutations), before)

    def test_publication_cli_waits_for_its_own_completion_without_rebuilding(self):
        self.coordinator.advance_publication('python', 'pypi')
        self.github.finish_publication('python_pypi')
        self.github.indexes[publisher.index('python', '0.2.6', 'pypi')] = index_response(self.record['python_packages']['python']['files'])
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', [
                'release.py', 'publish-python', '--record', str(self.path), '--package', 'python', '--destination', 'pypi', '--wait']), \
                patch('builtins.print'), patch.object(tool.time, 'sleep', side_effect=AssertionError('already complete')):
            tool.main()
        self.reload()
        self.assertEqual(self.record['status'], 'python_publication_completed')
        self.assertEqual(len(self.github.dispatches()), 8)

    def fetch_inputs(self, package='kit', **overrides):
        self.coordinator.prepare_publication()
        directory = Path(self.directory.name) / 'download'
        directory.mkdir(exist_ok=True)
        github = preparation.fixtures.Fixture(directory)
        github.runs = copy.deepcopy(self.github.runs)
        github.items = copy.deepcopy(self.github.artifacts)
        github.files = copy.deepcopy(self.github.payloads.files)
        original = github.request
        snapshot = self.record['python_publication_inputs']

        def request(route, **query):
            if '/contents/release/python-manifests/' in route:
                return {'type': 'file', 'encoding': 'base64',
                    'content': base64.b64encode(tool.encode(snapshot['document']).encode()).decode()}
            return original(route, **query)
        github.request = request
        arguments = dict(path=snapshot['path'], reference=METADATA, repository=REPOSITORY,
            candidate=CANDIDATE, ref='refs/tags/holder-v0.2.6', version='0.2.6', package=package,
            destination='pypi', output=Path(self.directory.name) / 'publication-dist', github=github)
        arguments.update(overrides)
        return publisher.fetch(**arguments)

    def test_publisher_fetches_the_original_bytes_without_building(self):
        with patch.object(publisher.subprocess, 'run', side_effect=AssertionError('No build or external command expected')):
            receipt = self.fetch_inputs()
        self.assertEqual(receipt['files'], self.record['python_packages']['kit']['files'])
        directory = Path(self.directory.name) / 'publication-dist'
        self.assertEqual(len(list(directory.iterdir())), 19)
        for file in receipt['files']:
            self.assertEqual(publisher.packages.manifest_tool.file_sha256(directory / file['name']), file['sha256'])

    def test_publisher_rejects_wrong_source_tag_ref_and_paths(self):
        for overrides in ({'candidate': 'b' * 40}, {'ref': 'refs/heads/main'}, {'reference': 'main'},
                          {'path': '../outside.json'}, {'version': '0.2.5'}, {'destination': 'unknown'}):
            with self.subTest(overrides=overrides), self.assertRaises(ValueError):
                self.fetch_inputs(**overrides)


class IndexPublicationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.dist = self.root / 'dist'
        self.dist.mkdir()
        for name, data in (('one.whl', b'one'), ('two.tar.gz', b'two')):
            (self.dist / name).write_bytes(data)
        self.files = [{'name': path.name, 'size': path.stat().st_size,
                       'sha256': publisher.packages.manifest_tool.file_sha256(path)} for path in sorted(self.dist.iterdir())]
        self.receipt = {'files': self.files, 'index_url': publisher.index('kit', '0.2.6', 'pypi')}

    def test_empty_partial_and_complete_index_upload_only_missing_bytes(self):
        for number in (0, 1, 2):
            pending = self.root / str(number)
            response = index_response(self.files[:number]) if number else None
            self.assertEqual(publisher.plan(self.dist, self.receipt, response, pending), 2 - number)
            self.assertEqual({path.name for path in pending.iterdir()}, {file['name'] for file in self.files[number:]})
            for path in pending.iterdir():
                self.assertEqual(path.read_bytes(), (self.dist / path.name).read_bytes())

    def test_conflicts_unexpected_duplicate_and_yanked_files_fail_before_upload_selection(self):
        for change in ('hash', 'size', 'extra', 'duplicate', 'yanked'):
            response = index_response(self.files)
            if change == 'hash':
                response['urls'][0]['digests']['sha256'] = '0' * 64
            elif change == 'size':
                response['urls'][0]['size'] += 1
            elif change == 'extra':
                response['urls'][0]['filename'] = 'unknown.whl'
            elif change == 'duplicate':
                response['urls'].append(response['urls'][0])
            else:
                response['urls'][0]['yanked'] = True
            with self.subTest(change=change), self.assertRaises(ValueError):
                publisher.plan(self.dist, self.receipt, response, self.root / change)
            self.assertFalse((self.root / change).exists())

    def test_modified_local_bytes_are_rejected(self):
        (self.dist / 'one.whl').write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'bytes changed'):
            publisher.plan(self.dist, self.receipt, None, self.root / 'pending')

    def test_index_completion_wait_and_incomplete_failure(self):
        responses = iter([None, index_response(self.files[:1]), index_response(self.files)])
        sleeps = []
        receipt = publisher.verify(self.receipt, request=lambda url: next(responses), attempts=3, sleep=sleeps.append)
        self.assertIn('verified_at', receipt)
        self.assertEqual(sleeps, [5, 5])
        with self.assertRaisesRegex(ValueError, 'index is incomplete'):
            publisher.verify(self.receipt, request=lambda url: None, attempts=1)


if __name__ == '__main__':
    unittest.main()
