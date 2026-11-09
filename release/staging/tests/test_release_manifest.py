import base64
import copy
from datetime import datetime, timezone
import hashlib
import gzip
import importlib.util
import io
import json
import os
from pathlib import Path
import tempfile
import tarfile
import unittest
import sys
import contextlib
from unittest.mock import patch
import zipfile

SPEC = importlib.util.spec_from_file_location(
    'release_manifest', Path(__file__).parents[1] / 'scripts/generate-release-manifest.py')
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)
REPOSITORY = 'HolderTeam/holder'
CANDIDATE = 'a' * 40
SDK_COMMIT = 'b' * 40
CORE_TREE = 'c' * 40
NOW = datetime(2026, 11, 8, tzinfo=timezone.utc)


class Fixture(tool.GitHub):
    def __init__(self, directory):
        super().__init__(directory)
        self.runs = {identifier: {'id': identifier, 'repository': {'full_name': REPOSITORY},
                                 'path': '.github/workflows/' + workflow, 'head_sha': CANDIDATE,
                                 'status': 'completed', 'conclusion': 'success', 'run_attempt': 1}
                     for identifier, workflow in ((10, 'desktop-ci.yml'), (20, 'daemon-ci.yml'))}
        self.items = {10: [], 20: []}
        self.files = {}
        self.tree = {CANDIDATE: CORE_TREE, SDK_COMMIT: CORE_TREE}
        self.versions = {CANDIDATE: 'version-blob', SDK_COMMIT: 'version-blob'}
        self.selection = {'source': 'artifact', 'commit': CANDIDATE, 'version': '0.2.6',
                          'core_tree': CORE_TREE, 'assets': []}
        for platform, (desktop, backend) in tool.components.ARTIFACTS.items():
            core = {'commit': SDK_COMMIT, 'version': '0.2.6', 'build_type': 'Release',
                    'platform': platform, 'architecture': 'arm64' if platform == 'macos' else 'x86_64'}
            self.selection['assets'].append({**core, 'name': platform + '.tar.gz', 'sha256': 'd' * 64})
            desktop_binary = {'linux': 'usr/bin/holder-desktop', 'windows': 'bin/holder-desktop.exe',
                              'macos': 'bin/holder-desktop'}[platform]
            self.add(10, desktop + '-release', {
                'release/holder-desktop-build.json': json.dumps(
                    {'commit': CANDIDATE, 'version': '0.2.6', 'build_type': 'release'}),
                desktop_binary: 'binary fixture'})
            core_path = 'usr/share/holder-daemon/core-build.json' if platform == 'linux' else 'core-build.json'
            files = {core_path: json.dumps(core), 'release/holder-daemon-commit.txt': CANDIDATE,
                     'release/holder-daemon-version.txt': '0.2.6'}
            binaries = {'linux': ['usr/bin/holderd', 'usr/bin/holderctl'],
                        'windows': ['holderd.exe', 'holderctl.exe'],
                        'macos': ['bin/holderd', 'bin/holderctl']}[platform]
            files.update({name: 'binary fixture' for name in binaries})
            self.add(20, backend + '-release', files)
        self.add(20, 'core-selection', {'core-selection.json': json.dumps(self.selection)})
        self.corrupt_checksum = False
        self.corrupt_digest = False

    def add(self, run, name, files):
        identifier = len(self.files) + 1
        item = {'id': identifier, 'name': name, 'expired': False,
                'expires_at': '2026-11-15T00:00:00Z', 'digest': 'sha256:' + 'e' * 64,
                'workflow_run': {'id': run, 'head_sha': CANDIDATE}}
        self.items[run].append(item)
        self.files[identifier] = files

    def item(self, name):
        return next(item for items in self.items.values() for item in items if item['name'] == name)

    def request(self, path, **query):
        relative = path.removeprefix(f'repos/{REPOSITORY}/')
        if relative == 'contents/VERSION':
            return {'content': base64.b64encode(b'0.2.6\n').decode()}
        if relative.startswith('commits/'):
            return {'commit': {'tree': {'sha': relative.removeprefix('commits/')}}}
        if relative.startswith('git/trees/'):
            sha = relative.removeprefix('git/trees/')
            return {'tree': [{'path': 'core', 'type': 'tree', 'sha': self.tree[sha]},
                             {'path': 'VERSION', 'type': 'blob', 'sha': self.versions[sha]}]}
        parts = relative.split('/')
        if parts[:2] == ['actions', 'runs']:
            identifier = int(parts[2])
            if len(parts) == 4:
                return {'artifacts': copy.deepcopy(self.items[identifier]) if query['page'] == '1' else []}
            return copy.deepcopy(self.runs[identifier])
        raise AssertionError(path)

    def read(self, repository, artifact, text_files, required_files=()):
        # Exercise real ZIP metadata reads/checksums, with deterministic fixture payloads.
        path = self.directory / f"{artifact['id']}.zip"
        files = self.files[artifact['id']]
        if '-linux-x86_64-release' in artifact['name']:
            payload = io.BytesIO()
            with tarfile.open(fileobj=payload, mode='w') as archive:
                for name, data in files.items():
                    entry = tarfile.TarInfo('./' + name)
                    content = data.encode()
                    entry.size = len(content)
                    archive.addfile(entry, io.BytesIO(content))
            data = gzip.compress(payload.getvalue(), mtime=0)
            filename = artifact['name'].removesuffix('-release') + '.tar.gz'
            checksum = '0' * 64 if self.corrupt_checksum else hashlib.sha256(data).hexdigest()
            files = {filename: data, filename + '.sha256': f'{checksum}  {filename}\n'}
        with zipfile.ZipFile(path, 'w') as archive:
            for name, data in files.items():
                entry = zipfile.ZipInfo(name, date_time=(2026, 11, 8, 0, 0, 0))
                archive.writestr(entry, data)
        self.cache[artifact['id']] = hashlib.sha256(path.read_bytes()).hexdigest()
        artifact['digest'] = 'sha256:' + ('0' * 64 if self.corrupt_digest else self.cache[artifact['id']])
        return super().read(repository, artifact, text_files, required_files)


class ManifestTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.github = Fixture(self.directory.name)

    def generate(self, **options):
        arguments = dict(repository=REPOSITORY, candidate=CANDIDATE, version='0.2.6',
                         desktop_run='10', backend_run='20', github=self.github, now=NOW)
        arguments.update(options)
        return tool.generate(**arguments)

    def change_json(self, artifact_name, path, **changes):
        files = self.github.files[self.github.item(artifact_name)['id']]
        data = json.loads(files[path])
        data.update(changes)
        files[path] = json.dumps(data)

    def test_deterministic_manifest_preserves_cached_sdk_producer_and_expiry(self):
        manifest, evidence = self.generate()
        self.assertEqual((manifest, evidence), self.generate())
        self.assertEqual(manifest['core']['commit'], CANDIDATE)
        self.assertEqual(len(evidence['artifacts']), 7)
        self.assertEqual(evidence['inputs_expire_at'], '2026-11-15T00:00:00Z')
        self.assertTrue(all(asset['commit'] == SDK_COMMIT for asset in evidence['core_selection']['assets']))
        self.assertTrue(all(item['archive_sha256'] for item in evidence['artifacts']))

    def test_generated_document_is_consumed_by_all_existing_platform_readers(self):
        manifest, _ = self.generate()
        path = Path(self.directory.name) / 'candidate.json'
        path.write_text(json.dumps(manifest))
        for platform in ('linux', 'windows', 'macos'):
            pin = tool.provenance.load_pin(path, platform)
            self.assertEqual(pin['appimage_version'], '0.2.6')
            self.assertEqual(pin['desktop']['run_id'], '10')
            self.assertEqual(pin['backend']['run_id'], '20')
            self.assertEqual(pin['core']['build_type'], 'Release')
            output = Path(self.directory.name) / (platform + '.outputs')
            with patch.dict(os.environ, {'GITHUB_OUTPUT': str(output),
                                          'BACKEND_REPOSITORY_INPUT': REPOSITORY}, clear=True), \
                    patch.object(sys, 'argv', ['core-provenance.py', 'prepare', '--platform', platform,
                                              '--release-manifest', str(path)]):
                tool.provenance.main()
            values = dict(line.split('=', 1) for line in output.read_text().splitlines())
            self.assertEqual(values['desktop_artifact_id'], pin['desktop']['artifact_id'])
            self.assertEqual(values['backend_artifact_id'], pin['backend']['artifact_id'])
            self.assertEqual(values['core_selection_artifact_id'], manifest['core']['selection_artifact_id'])

    def test_partial_artifact_pins_are_rejected(self):
        manifest, _ = self.generate()
        del manifest['platforms']['linux']['backend']['artifact_id']
        path = Path(self.directory.name) / 'partial.json'
        path.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'Artifact pins require'):
            tool.provenance.load_pin(path)

    def test_cli_writes_both_records_and_refuses_existing_candidates(self):
        manifest, evidence = self.generate()
        output = Path(self.directory.name) / 'candidate.json'
        args = ['generate-release-manifest.py', '--candidate', CANDIDATE, '--version', '0.2.6',
                '--desktop-run', '10', '--backend-run', '20', '--output', str(output)]
        with patch.object(sys, 'argv', args), patch.object(tool, 'generate', return_value=(manifest, evidence)), \
                contextlib.redirect_stdout(io.StringIO()):
            tool.main()
        self.assertEqual(json.loads(output.read_text()), manifest)
        self.assertEqual(json.loads(output.with_suffix('.evidence.json').read_text()), evidence)
        with patch.object(sys, 'argv', args), patch.object(tool, 'generate') as generate, \
                contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            tool.main()
        generate.assert_not_called()
        self.assertEqual(json.loads(output.read_text()), manifest)

    def test_artifact_listing_is_paginated(self):
        required = self.github.items[10][0]
        def request(path, **query):
            return {'artifacts': [{'name': 'unrelated'}] * 100 if query['page'] == '1' else [required]}
        select = tool.artifacts(REPOSITORY, self.github.runs[10], request, NOW)
        self.assertEqual(select(required['name']), required)

    def test_wrong_candidate_version_or_moving_revision_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'root VERSION'):
            self.generate(version='0.2.7')
        with self.assertRaisesRegex(ValueError, 'full lowercase'):
            self.generate(candidate='main')

    def test_failed_wrong_workflow_wrong_repository_and_mixed_runs_are_rejected(self):
        for changes in ({'conclusion': 'failure'}, {'status': 'in_progress'},
                        {'path': '.github/workflows/kit-ci.yml'},
                        {'repository': {'full_name': 'Someone/else'}}, {'head_sha': SDK_COMMIT}):
            with self.subTest(changes=changes):
                original = copy.deepcopy(self.github.runs[10])
                self.github.runs[10].update(changes)
                with self.assertRaises(ValueError):
                    self.generate()
                self.github.runs[10] = original

    def test_missing_ambiguous_expired_and_wrong_run_artifacts_are_rejected(self):
        original = copy.deepcopy(self.github.items[10])
        for mode in ('missing', 'duplicate', 'expired', 'past_expiry', 'wrong_owner'):
            with self.subTest(mode=mode):
                self.github.items[10] = copy.deepcopy(original)
                item = self.github.items[10][0]
                if mode == 'missing':
                    self.github.items[10].pop(0)
                elif mode == 'duplicate':
                    self.github.items[10].append(copy.deepcopy(item))
                elif mode == 'expired':
                    item['expired'] = True
                elif mode == 'past_expiry':
                    item['expires_at'] = '2026-11-01T00:00:00Z'
                else:
                    item['workflow_run']['id'] = 99
                with self.assertRaises(ValueError):
                    self.generate()

    def test_missing_payload_is_rejected(self):
        del self.github.files[self.github.item('holder-desktop-windows-release')['id']]['bin/holder-desktop.exe']
        with self.assertRaisesRegex(ValueError, 'holder-desktop.exe'):
            self.generate()

    def test_linux_inner_archive_checksum_is_verified(self):
        self.github.corrupt_checksum = True
        with self.assertRaisesRegex(ValueError, 'tarball checksum differs'):
            self.generate()

    def test_downloaded_zip_must_match_github_digest(self):
        self.github.corrupt_digest = True
        with self.assertRaisesRegex(ValueError, "ZIP checksum differs from GitHub"):
            self.generate()

    def test_metadata_rejects_development_or_wrong_version_and_commits(self):
        name = 'holder-desktop-linux-x86_64-release'
        path = 'release/holder-desktop-build.json'
        for changes in ({'build_type': 'debug'}, {'version': '0.2.5'}, {'commit': SDK_COMMIT}):
            with self.subTest(changes=changes):
                original = self.github.files[self.github.item(name)['id']][path]
                self.change_json(name, path, **changes)
                with self.assertRaises(ValueError):
                    self.generate()
                self.github.files[self.github.item(name)['id']][path] = original

    def test_mixed_daemon_source_and_core_platform_are_rejected(self):
        item = self.github.item('holder-daemon-windows-backend-release')
        files = self.github.files[item['id']]
        files['release/holder-daemon-commit.txt'] = SDK_COMMIT
        with self.assertRaisesRegex(ValueError, 'Backend does not match release pin'):
            self.generate()
        files['release/holder-daemon-commit.txt'] = CANDIDATE
        self.change_json(item['name'], 'core-build.json', architecture='arm64')
        with self.assertRaisesRegex(ValueError, 'windows x86_64'):
            self.generate()

    def test_cached_sdk_with_changed_core_tree_or_root_version_is_rejected(self):
        for mapping in (self.github.tree, self.github.versions):
            original = mapping[SDK_COMMIT]
            mapping[SDK_COMMIT] = 'different'
            with self.assertRaisesRegex(ValueError, 'cached SDK source/version'):
                self.generate()
            mapping[SDK_COMMIT] = original

    def test_core_selection_must_belong_to_candidate(self):
        for changes in ({'commit': SDK_COMMIT}, {'version': '0.2.5'}, {'core_tree': 'wrong'}):
            with self.subTest(changes=changes):
                original = copy.deepcopy(self.github.files)
                self.change_json('core-selection', 'core-selection.json', **changes)
                with self.assertRaises(ValueError):
                    self.generate()
                self.github.files = original

    def test_run_retried_during_generation_is_rejected(self):
        original = self.github.read

        def retry(*args, **kwargs):
            result = original(*args, **kwargs)
            self.github.runs[10]['run_attempt'] = 2
            return result

        with patch.object(self.github, 'read', side_effect=retry):
            with self.assertRaisesRegex(ValueError, 'retried during generation'):
                self.generate()


if __name__ == '__main__':
    unittest.main()
