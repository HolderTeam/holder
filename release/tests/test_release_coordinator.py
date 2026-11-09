import base64
import contextlib
import copy
from datetime import datetime, timezone
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[2]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


tool = load('release_coordinator', 'release/scripts/release.py')
fixtures = load('manifest_fixtures', 'release/staging/tests/test_release_manifest.py')
fetcher = load('fetch_manifest', 'release/staging/scripts/fetch-release-manifest.py')
CANDIDATE = 'a' * 40
MAIN = 'f' * 40
MANIFEST = 'd' * 40
NOW = datetime(2026, 11, 9, tzinfo=timezone.utc)
REPOSITORY = 'HolderTeam/holder'


class FakeGitHub:
    def __init__(self, directory):
        self.payloads = fixtures.Fixture(directory)
        for platform in tool.PLATFORMS:
            self.payloads.add(20, f'core-sdk-{platform}-Release', {})
        self.artifacts = self.payloads.items
        self.runs = {}
        self.refs = {'main': MAIN}
        self.mutations = []
        self.files = {}
        self.tree_files = {}
        self.lost_dispatch = None
        self.rejected_dispatch = None
        self.lost_retry = False
        self.duplicate_reconciliation = False

    def manifest(self, record):
        return tool.manifest_tool.generate(REPOSITORY, CANDIDATE, '0.2.6',
            record['steps']['desktop']['run_id'], record['steps']['backend']['run_id'],
            self.payloads, now=NOW)

    def api(self, route, method='GET', data=None, missing_ok=False, **query):
        if method != 'GET':
            self.mutations.append((route, method, copy.deepcopy(data)))
        path = urlsplit(route).path.removeprefix('repos/' + REPOSITORY + '/')
        if path == 'contents/VERSION':
            return {'content': base64.b64encode(b'0.2.6\n').decode()}
        if path.startswith('git/ref/heads/'):
            name = path.removeprefix('git/ref/heads/')
            if name in self.refs:
                return {'object': {'sha': self.refs[name]}}
            if missing_ok:
                return None
            raise tool.APIError('404', 'Not Found')
        if path == 'git/refs':
            name = data['ref'].removeprefix('refs/heads/')
            assert data['ref'].startswith('refs/heads/')
            self.refs[name] = data['sha']
            return {'object': {'sha': data['sha']}}
        if path == 'git/commits/' + CANDIDATE:
            return {'tree': {'sha': 'c' * 40}}
        if path == 'git/trees':
            self.tree_files = {item['path']: item['content'] for item in data['tree']}
            return {'sha': 'e' * 40}
        if path == 'git/commits':
            assert data['parents'] == [CANDIDATE]
            self.files[MANIFEST] = copy.deepcopy(self.tree_files)
            return {'sha': MANIFEST}
        if path.startswith('contents/'):
            name = path.removeprefix('contents/')
            return {'content': base64.b64encode(self.files[MANIFEST][name].encode()).decode()}
        if path.startswith('actions/workflows/'):
            workflow, action = path.removeprefix('actions/workflows/').split('/')
            name = next(name for name, value in tool.WORKFLOWS.items() if value == workflow)
            if action == 'dispatches':
                if name in tool.PACKAGES and data['inputs'].get('destination', 'none') != 'none':
                    name += '_' + data['inputs']['destination']
                if self.rejected_dispatch == name:
                    raise tool.APIError('403', 'Request rejected')
                identifier = {'backend': 20, 'desktop': 10, 'linux': 30, 'windows': 40, 'macos': 50,
                              'kit': 60, 'python': 70, 'kit_pypi': 80, 'python_pypi': 90,
                              'kit_testpypi': 100, 'python_testpypi': 110}[name]
                head = CANDIDATE if name in tool.BUILDS + tool.PACKAGES or name in tool.PUBLICATIONS else MAIN
                self.runs[identifier] = {'id': identifier, 'repository': {'full_name': REPOSITORY},
                    'path': '.github/workflows/' + workflow, 'event': 'workflow_dispatch',
                    'head_branch': data['ref'], 'head_sha': head,
                    'display_title': data['inputs']['release_request'], 'status': 'queued',
                    'conclusion': None, 'run_attempt': 1}
                if name in tool.PLATFORMS:
                    self.artifacts[identifier] = []
                    required = tool.STAGED[name] + (['Holder-windows-test-signed-installer']
                        if name == 'windows' and data['inputs']['sign_test_installer'] == 'true' else [])
                    for offset, artifact in enumerate(required):
                        self.artifacts[identifier].append({'id': identifier * 100 + offset,
                            'name': artifact, 'digest': 'sha256:' + 'c' * 64, 'size_in_bytes': 123,
                            'expired': False, 'expires_at': '2026-11-15T00:00:00Z',
                            'workflow_run': {'id': identifier, 'head_sha': head}})
                if self.lost_dispatch == name:
                    self.lost_dispatch = None
                    raise tool.APIError('', 'Response lost after dispatch')
                return {'workflow_run_id': identifier}
            runs = [copy.deepcopy(run) for run in self.runs.values() if run['path'].endswith(workflow)]
            if self.duplicate_reconciliation:
                runs += [{**run, 'id': 999} for run in runs]
            return {'workflow_runs': runs}
        if path.startswith('actions/runs/'):
            parts = path.split('/')
            identifier = int(parts[2])
            if len(parts) == 3:
                return copy.deepcopy(self.runs[identifier])
            if parts[3] == 'artifacts':
                return {'artifacts': copy.deepcopy(self.artifacts[identifier])}
            if parts[3] == 'rerun-failed-jobs':
                self.runs[identifier].update(run_attempt=self.runs[identifier]['run_attempt'] + 1,
                                             status='queued', conclusion=None)
                if self.lost_retry:
                    self.lost_retry = False
                    raise tool.APIError('', 'Retry response lost')
                return {}
        raise AssertionError((route, method, data, query))

    def finish(self, name, conclusion='success'):
        workflow = '.github/workflows/' + tool.WORKFLOWS[name]
        next(run for run in self.runs.values() if run['path'] == workflow).update(
            status='completed', conclusion=conclusion)

    def dispatches(self, workflow=None):
        return [item for item in self.mutations if item[0].endswith('/dispatches') and
                (workflow is None or workflow in item[0])]


class CoordinatorTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / 'candidate.json'
        self.github = FakeGitHub(self.directory.name)
        clock = patch.object(tool, 'now', return_value=NOW)
        clock.start()
        self.addCleanup(clock.stop)
        self.record = tool.initialize(self.github, REPOSITORY, CANDIDATE, '0.2.6', 1, True)
        tool.save(self.path, self.record)
        self.coordinator = tool.Coordinator(self.path, self.record, self.github)

    def reload(self):
        self.record = json.loads(self.path.read_text())
        self.coordinator = tool.Coordinator(self.path, self.record, self.github)

    def reach_staging(self):
        self.coordinator.advance()
        for name in tool.BUILDS:
            self.github.finish(name)
        self.reload()
        self.coordinator.advance()

    def test_complete_preparation_resumes_without_repeating_successes(self):
        self.coordinator.advance()
        self.assertEqual(self.record['status'], 'building')
        self.assertEqual(len(self.github.dispatches()), 2)
        self.reload()
        self.coordinator.advance()
        self.assertEqual(len(self.github.dispatches()), 2)
        for name in tool.BUILDS:
            self.github.finish(name)
        self.reload()
        self.coordinator.advance()
        self.assertEqual(self.record['status'], 'staging')
        self.assertEqual(len(self.github.dispatches()), 5)
        self.assertEqual(self.record['manifest']['commit'], MANIFEST)
        self.assertEqual(self.github.refs[self.record['build_ref']], CANDIDATE)
        for name in tool.PLATFORMS:
            step = self.record['steps'][name]
            self.assertEqual(step['ref'], 'main')
            self.assertEqual(step['inputs']['release_manifest_ref'], MANIFEST)
            self.github.finish(name)
        self.reload()
        self.coordinator.advance()
        self.assertEqual(self.record['status'], 'candidate_staged')
        self.assertEqual(len(self.github.dispatches()), 5)
        self.assertIn('Holder-windows-test-signed-installer', tool.summary(self.record))
        self.assertIn('Remaining release work', tool.summary(self.record))
        for route, method, body in self.github.mutations:
            self.assertNotIn('/releases', route)
            if route.endswith('/dispatches'):
                self.assertTrue(any(value in route for value in tool.WORKFLOWS.values()))

    def test_lost_dispatch_response_is_reconciled_without_another_post(self):
        self.github.lost_dispatch = 'backend'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance()
        self.reload()
        self.assertNotIn('run_id', self.record['steps']['backend'])
        self.coordinator.advance()
        self.assertEqual(self.record['steps']['backend']['run_id'], '20')
        self.assertEqual(len(self.github.dispatches('daemon-ci.yml')), 1)

    def test_previously_unobserved_attempts_remain_visible_after_recovery(self):
        self.github.lost_dispatch = 'backend'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance()
        self.github.runs[20]['run_attempt'] = 2
        self.reload()
        self.coordinator.advance()
        history = self.record['steps']['backend']['history']
        self.assertEqual((history[0]['attempt'], history[0]['conclusion']), (1, 'not_observed'))

    def test_pending_dispatch_with_no_visible_run_is_never_reposted(self):
        self.github.lost_dispatch = 'backend'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance()
        saved = self.github.runs.pop(20)
        self.reload()
        self.coordinator.advance()
        self.assertNotIn('run_id', self.record['steps']['backend'])
        self.assertEqual(len(self.github.dispatches('daemon-ci.yml')), 1)
        self.github.runs[20] = saved
        self.coordinator.advance()
        self.assertEqual(self.record['steps']['backend']['run_id'], '20')

    def test_ambiguous_reconciliation_is_rejected(self):
        self.github.lost_dispatch = 'backend'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance()
        self.github.duplicate_reconciliation = True
        self.reload()
        with self.assertRaisesRegex(ValueError, 'Multiple backend runs'):
            self.coordinator.advance()
        self.assertEqual(len(self.github.dispatches('daemon-ci.yml')), 1)

    def test_rejected_dispatch_requires_explicit_retry_and_keeps_its_request(self):
        self.github.rejected_dispatch = 'backend'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance()
        self.reload()
        self.assertEqual(self.record['steps']['backend']['status'], 'dispatch_rejected')
        self.github.rejected_dispatch = None
        self.coordinator.retry('backend')
        self.coordinator.advance()
        self.assertEqual(self.record['rejected_dispatches'][0]['step'], 'backend')
        self.assertEqual(len(self.github.runs), 2)

    def test_moved_frozen_build_branch_is_rejected(self):
        self.coordinator.advance()
        self.github.refs[self.record['build_ref']] = MAIN
        self.reload()
        with self.assertRaisesRegex(ValueError, 'Branch .* moved'):
            self.coordinator.advance()

    def test_remote_run_must_match_request_source_workflow_and_repository(self):
        self.coordinator.advance()
        for changes in ({'repository': {'full_name': 'Someone/else'}}, {'event': 'push'},
                        {'path': '.github/workflows/release-launchpad.yml'},
                        {'head_branch': 'main'}, {'head_sha': MAIN}, {'display_title': 'unrelated'}):
            with self.subTest(changes=changes):
                original = copy.deepcopy(self.github.runs[20])
                self.github.runs[20].update(changes)
                with self.assertRaises(ValueError):
                    self.coordinator.advance()
                self.github.runs[20] = original
        self.assertNotIn('manifest', self.record)
        self.assertEqual(len(self.github.dispatches()), 2)

    def test_dispatch_reconciliation_searches_multiple_pages(self):
        self.github.lost_dispatch = 'backend'
        with self.assertRaises(tool.APIError):
            self.coordinator.advance()
        original = self.github.api
        def pages(route, **options):
            if 'workflows/daemon-ci.yml/runs?' in route and route.endswith('page=1'):
                return {'workflow_runs': [{'display_title': 'other'}] * 100}
            return original(route, **options)
        self.reload()
        with patch.object(self.github, 'api', side_effect=pages):
            self.coordinator.advance()
        self.assertEqual(self.record['steps']['backend']['run_id'], '20')
        self.assertEqual(len(self.github.dispatches('daemon-ci.yml')), 1)

    def test_failed_stage_retries_only_that_step_and_retains_the_failed_attempt(self):
        self.reach_staging()
        self.github.finish('linux', 'failure')
        self.github.finish('windows')
        self.github.finish('macos')
        with self.assertRaisesRegex(ValueError, 'linux attempt 1 ended failure'):
            self.coordinator.advance()
        self.assertEqual(self.record['steps']['macos']['status'], 'verified')
        original_manifest = copy.deepcopy(self.record['manifest'])
        self.coordinator.retry('linux')
        self.coordinator.advance()
        self.github.finish('linux')
        self.reload()
        self.coordinator.advance()
        self.assertEqual(self.record['status'], 'candidate_staged')
        self.assertEqual(self.record['manifest'], original_manifest)
        self.assertEqual(len(self.github.dispatches()), 5)
        self.assertEqual(self.record['steps']['linux']['attempt'], 2)
        self.assertEqual(self.record['steps']['linux']['history'][0]['conclusion'], 'failure')

    def test_build_retry_is_rejected_after_artifacts_have_been_pinned(self):
        self.reach_staging()
        with self.assertRaisesRegex(ValueError, 'verified manifest'):
            self.coordinator.retry('backend')

    def test_uncertain_retry_response_is_observed_without_reposting(self):
        self.reach_staging()
        self.github.finish('linux', 'failure')
        with self.assertRaises(ValueError):
            self.coordinator.advance()
        self.github.lost_retry = True
        with self.assertRaises(tool.APIError):
            self.coordinator.retry('linux')
        self.reload()
        self.coordinator.advance()
        retries = [route for route, method, data in self.github.mutations if route.endswith('/rerun-failed-jobs')]
        self.assertEqual(len(retries), 1)
        self.assertEqual(self.record['steps']['linux']['attempt'], 2)

    def test_manifest_generation_cannot_silently_adopt_a_new_build_attempt(self):
        self.coordinator.advance()
        for name in tool.BUILDS:
            self.github.finish(name)
        original = self.github.manifest
        def changed(record):
            document, evidence = original(record)
            evidence['runs']['backend']['attempt'] = 2
            return document, evidence
        with patch.object(self.github, 'manifest', side_effect=changed):
            with self.assertRaisesRegex(ValueError, 'Build attempt changed'):
                self.coordinator.advance()
        self.assertNotIn('manifest', self.record)
        self.assertEqual(len(self.github.dispatches()), 2)

    def test_external_attempt_change_and_missing_successful_artifacts_are_rejected(self):
        self.coordinator.advance()
        self.github.finish('backend')
        self.coordinator.advance()
        self.github.runs[20]['run_attempt'] = 2
        with self.assertRaisesRegex(ValueError, 'retried outside this record'):
            self.coordinator.advance()
        self.github.runs[20]['run_attempt'] = 1
        self.github.artifacts[20] = []
        with self.assertRaisesRegex(ValueError, 'needs exactly one'):
            self.coordinator.advance()

    def test_expired_inputs_stop_without_replacing_manifest_or_dispatching(self):
        self.reach_staging()
        original = copy.deepcopy(self.record['manifest'])
        with patch.object(tool, 'now', return_value=datetime(2026, 11, 16, tzinfo=timezone.utc)):
            with self.assertRaisesRegex(ValueError, 'expired'):
                self.coordinator.advance()
        self.assertEqual(self.record['manifest'], original)
        self.assertEqual(len(self.github.dispatches()), 5)

    def test_changed_manifest_branch_is_rejected(self):
        self.reach_staging()
        path = 'release/staging/' + self.record['manifest']['path']
        self.github.files[MANIFEST][path] = '{}'
        with self.assertRaisesRegex(ValueError, 'different candidate evidence'):
            self.coordinator.advance()

    def test_saved_manifest_path_cannot_escape_the_candidate_location(self):
        self.reach_staging()
        self.record['manifest']['path'] = '../../unexpected.json'
        before = len(self.github.mutations)
        with self.assertRaisesRegex(ValueError, 'Saved manifest path/branch'):
            self.coordinator.advance()
        self.assertEqual(len(self.github.mutations), before)

    def test_publication_workflows_cannot_be_injected_through_a_record(self):
        self.coordinator.advance()
        self.record['steps']['backend']['workflow'] = 'release-launchpad.yml'
        before = len(self.github.mutations)
        with self.assertRaisesRegex(ValueError, 'allowlist'):
            self.coordinator.advance()
        self.assertEqual(len(self.github.mutations), before)

    def test_unsigned_windows_mode_is_recorded_and_does_not_require_signed_artifact(self):
        self.record['windows_sign_test_installer'] = False
        self.reach_staging()
        self.assertEqual(self.record['steps']['windows']['inputs']['sign_test_installer'], 'false')
        for name in tool.PLATFORMS:
            self.github.finish(name)
        self.coordinator.advance()
        self.assertIn('Windows staging: unsigned', tool.summary(self.record))
        self.assertEqual(len(self.record['steps']['windows']['artifacts']), 1)

    def test_cli_resume_and_status_and_record_lock(self):
        args = ['release.py', 'prepare', '--record', str(self.path)]
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', args), \
                contextlib.redirect_stdout(io.StringIO()):
            tool.main()
        self.assertTrue(self.path.with_suffix('.summary.md').is_file())
        before = len(self.github.mutations)
        with patch.object(sys, 'argv', ['release.py', 'status', '--record', str(self.path)]), \
                contextlib.redirect_stdout(io.StringIO()):
            tool.main()
        self.assertEqual(len(self.github.mutations), before)
        self.path.with_name(self.path.name + '.lock').write_text('')
        # Read-only status works while another preparation process holds the lock.
        with patch.object(sys, 'argv', ['release.py', 'status', '--record', str(self.path)]), \
                contextlib.redirect_stdout(io.StringIO()):
            tool.main()
        with patch.object(sys, 'argv', args), contextlib.redirect_stderr(io.StringIO()), \
                self.assertRaises(SystemExit):
            tool.main()
        self.assertEqual(len(self.github.mutations), before)

    def test_wait_observes_until_staged_without_extra_dispatches(self):
        polls = []
        def progress(seconds):
            polls.append(seconds)
            names = tool.BUILDS if len(polls) == 1 else tool.PLATFORMS
            for name in names:
                self.github.finish(name)
        args = ['release.py', 'prepare', '--record', str(self.path), '--wait']
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', args), \
                patch.object(tool.time, 'sleep', side_effect=progress), contextlib.redirect_stdout(io.StringIO()):
            tool.main()
        self.assertEqual(len(polls), 2)
        self.assertEqual(len(self.github.dispatches()), 5)
        self.assertEqual(json.loads(self.path.read_text())['status'], 'candidate_staged')

    def test_cli_rejects_changed_candidate_parameters(self):
        args = ['release.py', 'prepare', '--record', str(self.path), '--version', '0.2.7']
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', args), \
                contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            tool.main()
        self.assertFalse(self.github.mutations)

    def test_cli_can_create_a_new_record_and_check_source_version_before_dispatch(self):
        path = self.path.with_name('new.json')
        args = ['release.py', 'prepare', '--record', str(path), '--candidate', CANDIDATE,
                '--version', '0.2.6', '--number', '1']
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', args), \
                contextlib.redirect_stdout(io.StringIO()):
            tool.main()
        self.assertEqual(json.loads(path.read_text())['status'], 'building')
        self.assertEqual(len(self.github.dispatches()), 2)
        before = len(self.github.mutations)
        bad_path = path.with_name('wrong-version.json')
        bad_args = ['release.py', 'prepare', '--record', str(bad_path), '--candidate', CANDIDATE,
                    '--version', '0.2.7', '--number', '2']
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', bad_args), \
                contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            tool.main()
        self.assertFalse(bad_path.exists())
        self.assertEqual(len(self.github.mutations), before)


class ManifestFetchTests(unittest.TestCase):
    def test_remote_manifest_is_validated_before_replacing_a_local_file(self):
        with tempfile.TemporaryDirectory() as directory:
            github = FakeGitHub(directory)
            record = tool.initialize(github, REPOSITORY, CANDIDATE, '0.2.6', 1, True)
            coordinator = tool.Coordinator(Path(directory) / 'record.json', record, github)
            with patch.object(tool, 'now', return_value=NOW):
                coordinator.advance()
                for name in tool.BUILDS:
                    github.finish(name)
                coordinator.advance()
            document = record['manifest']['document']
            response = {'type': 'file', 'encoding': 'base64',
                        'content': base64.b64encode(tool.encode(document).encode()).decode()}
            root = Path(directory) / 'staging'
            path = 'release-manifests/0.2.6-candidate-1.json'
            fetcher.fetch(root, path, MANIFEST, REPOSITORY, request=lambda route: response)
            self.assertEqual(json.loads((root / path).read_text()), document)
            response['content'] = base64.b64encode(b'{}').decode()
            with self.assertRaisesRegex(ValueError, 'all three'):
                fetcher.fetch(root, path, MANIFEST, REPOSITORY, request=lambda route: response)
            self.assertEqual(json.loads((root / path).read_text()), document)

    def test_branch_references_and_path_escape_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            for path, reference in (('release-manifests/example.json', 'main'),
                                    ('../outside.json', MANIFEST),
                                    ('scripts/overwrite.py', MANIFEST),
                                    ('release-manifests/../../outside.json', MANIFEST)):
                with self.subTest(path=path, reference=reference), self.assertRaises(ValueError):
                    fetcher.fetch(directory, path, reference, REPOSITORY)

    def test_legacy_checked_in_inputs_need_no_remote_read(self):
        fetcher.fetch('/unused', '', '', REPOSITORY,
                      request=lambda route: self.fail('unexpected remote read'))


if __name__ == '__main__':
    unittest.main()
