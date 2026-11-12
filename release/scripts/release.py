#!/usr/bin/env python3
"""Resume candidate preparation and per-destination delivery with one release record."""
import argparse
import base64
from contextlib import contextmanager
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import tarfile
import time
import uuid
import zipfile
from urllib.parse import urlencode

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('release_manifest',
    ROOT / 'release/staging/scripts/generate-release-manifest.py')
manifest_tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(manifest_tool)
PACKAGE_SPEC = importlib.util.spec_from_file_location('release_packages', Path(__file__).with_name('python-distributions.py'))
package_tool = importlib.util.module_from_spec(PACKAGE_SPEC)
PACKAGE_SPEC.loader.exec_module(package_tool)
PUBLISH_SPEC = importlib.util.spec_from_file_location('release_publish', Path(__file__).with_name('publish-python.py'))
publisher_tool = importlib.util.module_from_spec(PUBLISH_SPEC)
PUBLISH_SPEC.loader.exec_module(publisher_tool)
LINUX_SPEC = importlib.util.spec_from_file_location('release_linux', Path(__file__).with_name('linux-candidate.py'))
linux_tool = importlib.util.module_from_spec(LINUX_SPEC)
LINUX_SPEC.loader.exec_module(linux_tool)
PROMOTION_SPEC = importlib.util.spec_from_file_location('release_linux_promotion', Path(__file__).with_name('promote-linux.py'))
promotion_tool = importlib.util.module_from_spec(PROMOTION_SPEC)
PROMOTION_SPEC.loader.exec_module(promotion_tool)
SDK_SPEC = importlib.util.spec_from_file_location('release_sdk', Path(__file__).with_name('publish-core-sdk.py'))
sdk_release = importlib.util.module_from_spec(SDK_SPEC)
SDK_SPEC.loader.exec_module(sdk_release)
WINDOWS_SPEC = importlib.util.spec_from_file_location('release_windows', Path(__file__).with_name('windows-candidate.py'))
windows_tool = importlib.util.module_from_spec(WINDOWS_SPEC)
WINDOWS_SPEC.loader.exec_module(windows_tool)
WINDOWS_PROMOTION_SPEC = importlib.util.spec_from_file_location('release_windows_promotion', Path(__file__).with_name('promote-windows.py'))
windows_promotion = importlib.util.module_from_spec(WINDOWS_PROMOTION_SPEC)
WINDOWS_PROMOTION_SPEC.loader.exec_module(windows_promotion)
PPA_SPEC = importlib.util.spec_from_file_location('release_launchpad', Path(__file__).with_name('launchpad-release.py'))
ppa_tool = importlib.util.module_from_spec(PPA_SPEC)
PPA_SPEC.loader.exec_module(ppa_tool)
WEBSITE_SPEC = importlib.util.spec_from_file_location('release_website', Path(__file__).with_name('website-observation.py'))
website_tool = importlib.util.module_from_spec(WEBSITE_SPEC)
WEBSITE_SPEC.loader.exec_module(website_tool)
MACOS_SPEC = importlib.util.spec_from_file_location('release_macos', Path(__file__).with_name('macos-delivery.py'))
macos_tool = importlib.util.module_from_spec(MACOS_SPEC)
MACOS_SPEC.loader.exec_module(macos_tool)
require = manifest_tool.require
WORKFLOWS = {'backend': 'daemon-ci.yml', 'desktop': 'desktop-ci.yml',
             'linux': 'staging-linux-appimage-stage.yml', 'windows': 'staging-windows-stage.yml',
             'macos': 'staging-macos-stage.yml', 'kit': 'release-kit-pypi.yml',
             'python': 'release-holder-python-pypi.yml'}
BUILDS = ('backend', 'desktop')
PACKAGES = ('kit', 'python')
PUBLICATIONS = {f'{package}_{destination}': (package, destination)
                for package in PACKAGES for destination in ('pypi', 'testpypi')}
WORKFLOWS.update({name: WORKFLOWS[package] for name, (package, _) in PUBLICATIONS.items()})
WORKFLOWS['linux_signing'] = 'sign-linux-appimage.yml'
WORKFLOWS['windows_promotion'] = 'promote-windows-dev-build.yml'
WORKFLOWS['linux_promotion'] = 'promote-linux-appimage.yml'
WORKFLOWS['core_sdk_publication'] = 'promote-core-sdk.yml'
PLATFORMS = ('linux', 'windows', 'macos')
STAGED = {'linux': ['Holder-linux-appimage-staged'], 'windows': ['Holder-windows-staged'],
          'macos': ['Holder-macos-staged']}


def encode(value):
    return json.dumps(value, indent=2) + '\n'


def digest(value):
    return hashlib.sha256(encode(value).encode()).hexdigest()


def now():
    return datetime.now(timezone.utc)


def linux_release_files(record):
    files = [file for file in record['linux_signed']['files'] if file['name'] in promotion_tool.names(record['version'])]
    require(len(files) == 4 and {file['name'] for file in files} == promotion_tool.names(record['version']),
            'Accepted signed Linux payload is incomplete')
    return sorted(files, key=lambda file: file['name'])


class APIError(RuntimeError):
    def __init__(self, status, message):
        super().__init__(f'GitHub HTTP {status or "unknown"}: {message}')
        self.status = status


class GitHub:
    asset_fingerprint = staticmethod(promotion_tool.asset_fingerprint)
    asset_contents = staticmethod(promotion_tool.assets.asset_contents)
    def api(self, route, method='GET', data=None, missing_ok=False, **query):
        if query:
            route += ('&' if '?' in route else '?') + urlencode(query)
        command = ['gh', 'api', '--method', method, '-H', 'X-GitHub-Api-Version: 2026-03-10', route]
        if data is not None:
            command += ['--input', '-']
        result = subprocess.run(command, input=json.dumps(data) if data is not None else None,
                                capture_output=True, text=True, encoding='utf-8')
        try:
            body = json.loads(result.stdout) if result.stdout.strip() else {}
        except ValueError:
            body = {}
        if result.returncode:
            status = str(body.get('status', ''))
            if missing_ok and status == '404':
                return None
            raise APIError(status, body.get('message', 'Request failed; inspect GitHub and resume the record'))
        return body

    def manifest(self, record):
        with tempfile.TemporaryDirectory(prefix='holder-candidate-inputs-') as directory:
            return manifest_tool.generate(record['repository'], record['candidate'], record['version'],
                record['steps']['desktop']['run_id'], record['steps']['backend']['run_id'],
                manifest_tool.GitHub(directory))

    def packages(self, record):
        with tempfile.TemporaryDirectory(prefix='holder-python-inputs-') as directory:
            return package_tool.collect(record, manifest_tool.GitHub(directory))

    def publication(self, record, name):
        step = record['steps'][name]
        with tempfile.TemporaryDirectory(prefix='holder-python-publication-') as directory:
            values, _ = manifest_tool.GitHub(directory).read(record['repository'],
                step['artifacts'][0], ['publication.json'])
            return json.loads(values['publication.json'])

    def publication_status(self, receipt):
        publisher_tool.verify(receipt, attempts=1)

    def linux_bundle(self, record, name):
        step = record['steps'][name]
        artifact = step['artifacts'][0]
        with tempfile.TemporaryDirectory(prefix='holder-linux-evidence-') as directory:
            bundle = linux_tool.read(record['repository'], step['run_id'], artifact['name'], record['version'],
                manifest_tool.GitHub(directory), signed=name == 'linux_signing', artifact_id=artifact['id'],
                workflow_commit=step['workflow_commit'])
        require(bundle['attempt'] == step['attempt'] and bundle['artifact'] == artifact,
                'Accepted Linux run/artifact changed during verification')
        linux_tool.bind(record, bundle)
        return bundle

    def linux_promotion_status(self, record, release_id=None):
        return promotion_tool.observe(self, record['repository'], 'holder-v' + record['version'],
            linux_release_files(record), record['candidate'], release_id=release_id)

    def macos_bundle(self, record):
        with tempfile.TemporaryDirectory(prefix='holder-macos-evidence-') as directory:
            return macos_tool.staging(record, manifest_tool.GitHub(directory))

    def upload(self, repository, tag, path):
        promotion_tool.GitHub().upload(repository, tag, path)

    def windows_bundle(self, record):
        step = record['steps']['windows']
        artifact = next(item for item in step['artifacts'] if item['name'] == 'Holder-windows-test-signed-installer')
        with tempfile.TemporaryDirectory(prefix='holder-windows-evidence-') as directory:
            bundle = windows_tool.read(record['repository'], step['run_id'], artifact['name'], record['version'],
                manifest_tool.GitHub(directory), artifact_id=artifact['id'], workflow_commit=step['workflow_commit'])
        require(bundle['attempt'] == step['attempt'] and bundle['artifact'] == artifact,
                'Accepted Windows run/artifact changed during verification')
        windows_tool.bind(record, bundle)
        return bundle

    def windows_promotion_status(self, record, release_id=None):
        return windows_promotion.observe(self, record['repository'], 'holder-v' + record['version'],
            windows_tool.payload_files(record['windows_delivery_input'], record['version']), record['candidate'], release_id=release_id)

    def sdk_publication_status(self, record, release_id=None):
        document = record['sdk_publication_inputs']['document']
        expected = sdk_release.make_index(document['core_selection'], record['repository'], document['backend']['run_id'])
        return sdk_release.observe(self, expected, release_id)


def save(path, record):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode='w', dir=path.parent, prefix=path.name + '.', delete=False) as output:
        temporary = Path(output.name)
        output.write(encode(record))
        output.flush()
        os.fsync(output.fileno())
    os.replace(temporary, path)


@contextmanager
def lock(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    marker = path.with_name(path.name + '.lock')
    try:
        descriptor = os.open(marker, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    except FileExistsError:
        raise ValueError(f'Release record is locked: {marker}; confirm no other process is using it before removing the lock')
    os.close(descriptor)
    try:
        yield
    finally:
        marker.unlink()


class Coordinator:
    def __init__(self, path, record, github):
        self.path, self.record, self.github = path, record, github
        self.prefix = 'repos/' + record['repository']

    def persist(self):
        save(self.path, self.record)

    def api(self, route, **options):
        return self.github.api(self.prefix + '/' + route, **options)

    def ensure_branch(self, name, commit):
        current = self.api('git/ref/heads/' + name, missing_ok=True)
        if current is None:
            try:
                self.api('git/refs', method='POST', data={'ref': 'refs/heads/' + name, 'sha': commit})
            except APIError:
                current = self.api('git/ref/heads/' + name, missing_ok=True)
                if current is None:
                    raise
        if current is not None:
            require(current['object']['sha'] == commit, f'Branch {name} moved; choose a new candidate number')

    def expected(self, name):
        if name == 'core_sdk_publication':
            snapshot = self.record['sdk_publication_inputs']
            return 'main', {'expected_version': self.record['version'], 'daemon_ci_run_id': self.record['steps']['backend']['run_id'],
                'release_tag': 'holder-v' + self.record['version'], 'prepared_sdk_path': snapshot['path'],
                'prepared_sdk_ref': snapshot['commit']}
        if name == 'windows_promotion':
            bundle = self.record['windows_delivery_input']
            return 'main', {'expected_version': self.record['version'], 'staging_repository': self.record['repository'],
                'staging_run_id': bundle['run_id'], 'staging_artifact_name': bundle['artifact']['name'],
                'staging_artifact_id': str(bundle['artifact']['id']), 'staging_artifact_sha256': bundle['archive_sha256'],
                'candidate_commit': self.record['candidate'], 'release_tag': 'holder-v' + self.record['version'],
                'replace_draft_assets': 'false'}
        if name == 'linux_promotion':
            signed = self.record['linux_signed']
            return 'main', {'expected_version': self.record['version'], 'signing_run_id': signed['run_id'],
                'signing_artifact_id': str(signed['artifact']['id']), 'signing_artifact_sha256': signed['archive_sha256'],
                'candidate_commit': self.record['candidate'], 'release_tag': 'holder-v' + self.record['version'],
                'allow_published_prerelease': 'false', 'replace_draft_assets': 'false'}
        if name == 'linux_signing':
            bundle = self.record['linux_signing_input']
            return 'main', {'expected_version': self.record['version'], 'staging_repository': self.record['repository'],
                'staging_run_id': bundle['run_id'], 'staging_artifact_name': bundle['artifact']['name'],
                'staging_artifact_id': str(bundle['artifact']['id']), 'staging_artifact_sha256': bundle['archive_sha256']}
        if name == 'backend':
            return self.record['build_ref'], {'core_build_type': 'Release', 'core_packages': 'source'}
        if name == 'desktop':
            return self.record['build_ref'], {'build_type': 'release'}
        if name in PACKAGES:
            inputs = {'destination': 'none'}
            if name == 'kit':
                inputs.update(release_manifest=self.record['manifest']['path'],
                              release_manifest_ref=self.record['manifest']['commit'])
            else:
                inputs['preview_number'] = '0'
            return self.record['build_ref'], inputs
        if name in PUBLICATIONS:
            _, destination = PUBLICATIONS[name]
            snapshot = self.record['python_publication_inputs']
            return f'holder-v{self.record["version"]}', {'destination': destination,
                'prepared_release_path': snapshot['path'], 'prepared_release_ref': snapshot['commit']}
        inputs = {'backend_repository': self.record['repository'],
                  'release_manifest': self.record['manifest']['path'],
                  'release_manifest_ref': self.record['manifest']['commit']}
        if name == 'windows':
            inputs['sign_test_installer'] = str(self.record['windows_sign_test_installer']).lower()
        return 'main', inputs

    def validate_run(self, name, step, run):
        require(run['repository']['full_name'] == self.record['repository'] and
                run['path'] == '.github/workflows/' + WORKFLOWS[name] and
                run['event'] == 'workflow_dispatch' and run['head_branch'] == step['ref'] and
                run['display_title'] == step['request'], f'{name} run identity differs from the release request')
        manifest_tool.provenance.commit(run['head_sha'])
        if name in BUILDS + PACKAGES or name in PUBLICATIONS:
            require(run['head_sha'] == self.record['candidate'], f'{name} run differs from candidate source')
        if step.get('workflow_commit'):
            require(step['workflow_commit'] == run['head_sha'], f'{name} workflow source changed')
        step['workflow_commit'] = run['head_sha']

    def reconcile(self, name, step):
        filters = {'event': 'workflow_dispatch', 'per_page': 100}
        if name not in PUBLICATIONS:
            filters['branch'] = step['ref']
        query = urlencode(filters)
        matches = []
        page = 1
        while True:
            runs = self.api(f'actions/workflows/{WORKFLOWS[name]}/runs?{query}&page={page}')['workflow_runs']
            matches.extend(run for run in runs if run['display_title'] == step['request'])
            if len(runs) < 100:
                break
            page += 1
        require(len(matches) <= 1, f'Multiple {name} runs match this request; inspect them before continuing')
        if matches:
            self.validate_run(name, step, matches[0])
            step['run_id'] = str(matches[0]['id'])
            self.persist()

    def start(self, name):
        reference, inputs = self.expected(name)
        request = f'holder-{self.record["version"]}-candidate-{self.record["number"]}-{name}-{uuid.uuid4().hex}'
        inputs['release_request'] = request
        step = {'workflow': WORKFLOWS[name], 'ref': reference, 'inputs': inputs, 'request': request,
                'requested_at': now().isoformat(timespec='seconds'), 'status': 'dispatch_pending', 'history': []}
        self.record['steps'][name] = step
        self.persist()  # Persist intent before a dispatch can create a remote run.
        try:
            response = self.api(f'actions/workflows/{WORKFLOWS[name]}/dispatches', method='POST',
                                data={'ref': reference, 'inputs': inputs})
        except APIError as error:
            if error.status.startswith('4'):
                step.update(status='dispatch_rejected', error=str(error))
                self.persist()
            raise
        if response.get('workflow_run_id'):
            step['run_id'] = str(response['workflow_run_id'])
            self.persist()

    def required_artifacts(self, name):
        if name in ('linux_promotion', 'windows_promotion', 'core_sdk_publication'):
            return []  # Destination assets are verified independently of run cleanup/receipt retention.
        if name == 'linux_signing':
            return ['Holder-linux-appimage-signed-' + self.record['version']]
        if name == 'backend':
            return ['core-selection'] + [f'core-sdk-{platform}-Release' for platform in PLATFORMS] + [
                pair[1] + '-release' for pair in manifest_tool.components.ARTIFACTS.values()]
        if name == 'desktop':
            return [pair[0] + '-release' for pair in manifest_tool.components.ARTIFACTS.values()]
        if name in PACKAGES:
            return list(package_tool.ARTIFACTS[name])
        if name in PUBLICATIONS:
            package, destination = PUBLICATIONS[name]
            return [f'{package}-{destination}-publication']
        return STAGED[name] + (['Holder-windows-test-signed-installer']
            if name == 'windows' and self.record['windows_sign_test_installer'] else [])

    def inspect(self, name, allow_failure=False):
        step = self.record['steps'][name]
        reference, inputs = self.expected(name)
        require(step['workflow'] == WORKFLOWS[name] and step['ref'] == reference and
                step['inputs'] == {**inputs, 'release_request': step['request']},
                f'{name} recorded dispatch inputs changed')
        if step['status'] == 'dispatch_rejected':
            raise ValueError(f'{name} dispatch was rejected; correct the cause and use --retry {name}')
        if 'run_id' not in step:
            self.reconcile(name, step)
            if 'run_id' not in step:
                return False  # A lost/legacy dispatch response never causes another POST.
        run = self.api('actions/runs/' + step['run_id'])
        self.validate_run(name, step, run)
        attempt = run['run_attempt']
        if step.get('retry_attempt'):
            if attempt < step['retry_attempt']:
                return False
            require(attempt == step['retry_attempt'], f'{name} retry attempt changed unexpectedly')
            step.pop('retry_attempt')
            step.pop('attempt', None)
        if 'attempt' in step:
            require(step['attempt'] == attempt, f'{name} run was retried outside this record; preserve the candidate pins')
        else:
            recorded = {item['attempt'] for item in step['history']}
            for previous in range(1, attempt):
                if previous not in recorded:
                    step['history'].append({'run_id': step['run_id'], 'attempt': previous,
                        'conclusion': 'not_observed',
                        'url': f'https://github.com/{self.record["repository"]}/actions/runs/{step["run_id"]}/attempts/{previous}'})
        step.update(attempt=attempt, status=run['status'], conclusion=run['conclusion'])
        if run['status'] != 'completed':
            self.persist()
            return False
        if run['conclusion'] != 'success':
            self.persist()
            if allow_failure:
                return False
            raise ValueError(f'{name} attempt {attempt} ended {run["conclusion"]}; use --retry {name}')
        select = manifest_tool.artifacts(self.record['repository'], run, self.github.api, now())
        artifacts = []
        for required in self.required_artifacts(name):
            item = select(required)
            artifacts.append({key: item.get(key) for key in ('id', 'name', 'digest', 'size_in_bytes', 'expires_at')})
        if 'artifacts' in step:
            require(step['artifacts'] == artifacts, f'{name} accepted artifacts changed; select a new candidate')
        step.update(status='verified', artifacts=artifacts)
        self.persist()
        return True

    def retry(self, name):
        require(name in self.record['steps'], f'{name} has not been started')
        step = self.record['steps'][name]
        if name == 'core_sdk_publication':
            require(not self.record.get('core_sdk_published'), 'SDK publication is already accepted; inspect its recorded assets instead of retrying')
            self.prepare_sdk_publication()
            self.require_sdk_inputs_available()
            self.ensure_sdk_metadata()
        require(not (name == 'macos' and self.record.get('macos_handoff')),
                'macOS staging is pinned for delivery; select a new candidate instead of replacing inputs')
        require(not (name == 'windows' and self.record.get('windows_delivery_input')),
                'Windows staging is pinned for delivery; select a new candidate instead of replacing inputs')
        if name == 'windows_promotion':
            require(not self.record.get('windows_promoted'), 'Windows delivery is already accepted; inspect its recorded assets instead of retrying')
            self.prepare_windows_promotion()
            require(datetime.fromisoformat(self.record['windows_delivery_input']['artifact']['expires_at'].replace('Z', '+00:00')) > now(),
                    'Windows tester artifact expired before promotion could be retried')
        if name == 'linux_promotion':
            require(not self.record.get('linux_promoted'), 'Linux delivery is already accepted; inspect its recorded assets instead of retrying')
            self.prepare_linux_promotion()
            require(datetime.fromisoformat(self.record['linux_signed']['artifact']['expires_at'].replace('Z', '+00:00')) > now(),
                    'Signed Linux artifact expired before promotion could be retried')
        require(not (name == 'linux' and self.record.get('linux_signing_input')),
                'Linux staging is pinned for signing; select a new candidate instead of replacing inputs')
        require(not (name == 'linux_signing' and self.record.get('linux_signed')),
                'Signed Linux bytes are already accepted; select a new candidate instead of signing again')
        if name == 'linux_signing':
            require(digest(self.record['linux_signing_input']) == self.record['linux_signing_input_sha256'],
                    'Saved Linux signing input changed')
            linux_tool.bind(self.record, self.record['linux_signing_input'])
            require(datetime.fromisoformat(self.record['linux_signing_input']['artifact']['expires_at'].replace('Z', '+00:00')) > now(),
                    'Pinned Linux staging artifact expired before signing completed')
        require(not (name in BUILDS and self.record.get('manifest')),
                'Builds already belong to a verified manifest; select a new candidate instead of replacing inputs')
        require(not (name in PACKAGES and self.record.get('python_packages')),
                'Packages already belong to a verified release set; select a new candidate instead of replacing inputs')
        require(not step.get('retry_attempt'), f'{name} retry is already requested; resume to observe it')
        if step['status'] == 'dispatch_rejected':
            self.record.setdefault('rejected_dispatches', []).append({'step': name, **step})
            del self.record['steps'][name]
            self.persist()
            return
        require('run_id' in step, f'{name} dispatch is uncertain; resume to reconcile it before retrying')
        run = self.api('actions/runs/' + step['run_id'])
        self.validate_run(name, step, run)
        require(run['status'] == 'completed' and run['conclusion'] in ('failure', 'cancelled', 'timed_out'),
                f'{name} is not a failed completed run')
        require(step.get('attempt') == run['run_attempt'], f'{name} attempt changed; inspect it before retrying')
        step['history'].append({'run_id': step['run_id'], 'attempt': run['run_attempt'],
                                'conclusion': run['conclusion'], 'workflow_commit': run['head_sha']})
        step.update(status='retry_pending', retry_attempt=run['run_attempt'] + 1)
        self.persist()
        try:
            self.api('actions/runs/' + step['run_id'] + '/rerun-failed-jobs', method='POST')
        except APIError as error:
            if error.status.startswith('4'):
                step.pop('retry_attempt')
                step.update(status='completed', error=str(error))
                self.persist()
            raise

    def validate_manifest(self, document, evidence):
        require(document['version'] == self.record['version'] and document['core']['build_type'] == 'Release' and
                document['core']['commit'] == self.record['candidate'] and
                evidence['candidate'] == {'repository': self.record['repository'],
                    'commit': self.record['candidate'], 'version': self.record['version']},
                'Saved manifest belongs to another candidate')
        for name in BUILDS:
            step = self.record['steps'][name]
            require(str(evidence['runs'][name]['id']) == step['run_id'] and
                    evidence['runs'][name]['attempt'] == step['attempt'],
                    'Build attempt changed while generating the candidate manifest')
        selection = next(item for item in self.record['steps']['backend']['artifacts'] if item['name'] == 'core-selection')
        require(str(selection['id']) == document['core']['selection_artifact_id'],
                'Core selection artifact changed while generating the manifest')
        for platform, names in manifest_tool.components.ARTIFACTS.items():
            for name, artifact_name in zip(('desktop', 'backend'), names):
                pin = document['platforms'][platform][name]
                step = self.record['steps'][name]
                artifact = next(item for item in step['artifacts'] if item['name'] == artifact_name + '-release')
                require(pin['commit'] == self.record['candidate'] and pin['repository'] == self.record['repository'] and
                        str(pin['run_id']) == step['run_id'] and str(pin['artifact_id']) == str(artifact['id']),
                        'Component artifacts changed while generating the manifest')

    def ensure_manifest(self):
        if 'manifest' not in self.record:
            document, evidence = self.github.manifest(self.record)
            self.validate_manifest(document, evidence)
            name = f'{self.record["version"]}-candidate-{self.record["number"]}'
            self.record['manifest'] = {'document': document, 'evidence': evidence,
                'path': f'release-manifests/{name}.json', 'sha256': digest(document),
                'branch': f'release/candidates/{name}'}
            self.persist()
        manifest = self.record['manifest']
        require(digest(manifest['document']) == manifest['sha256'], 'Saved manifest changed')
        self.validate_manifest(manifest['document'], manifest['evidence'])
        require(datetime.fromisoformat(manifest['evidence']['inputs_expire_at'].replace('Z', '+00:00')) > now(),
                'Candidate inputs expired; select a new candidate without replacing its record')
        paths = {'release/staging/' + manifest['path']: encode(manifest['document']),
                 'release/staging/' + manifest['path'].removesuffix('.json') + '.evidence.json': encode(manifest['evidence'])}
        self.ensure_metadata(manifest, paths)

    def ensure_metadata(self, manifest, paths, label='manifest'):
        existing = self.api('git/ref/heads/' + manifest['branch'], missing_ok=True)
        if existing:
            commit = existing['object']['sha']
            for path, value in paths.items():
                remote = self.api('contents/' + path + '?ref=' + commit)
                require(base64.b64decode(remote['content']).decode() == value,
                        f'Existing {label} branch contains different candidate evidence')
            require(not manifest.get('commit') or manifest['commit'] == commit, label.capitalize() + ' branch moved')
            manifest['commit'] = commit
            self.persist()
            return
        if not manifest.get('commit'):
            base = self.api('git/commits/' + self.record['candidate'])
            tree = self.api('git/trees', method='POST', data={'base_tree': base['tree']['sha'],
                'tree': [{'path': path, 'mode': '100644', 'type': 'blob', 'content': value}
                         for path, value in paths.items()]})
            commit = self.api('git/commits', method='POST', data={
                'message': 'Record verified release candidate ' + self.record['version'],
                'tree': tree['sha'], 'parents': [self.record['candidate']]})
            manifest['commit'] = commit['sha']
            self.persist()
        self.ensure_branch(manifest['branch'], manifest['commit'])

    def observe(self, names):
        ready, errors = [], []
        for name in names:
            try:
                ready.append(self.inspect(name))
            except (ValueError, APIError) as error:
                errors.append(str(error))
        require(not errors, '; '.join(errors))
        return all(ready)

    def advance(self):
        validate_record(self.record)
        self.ensure_branch(self.record['build_ref'], self.record['candidate'])
        for name in BUILDS:
            if name not in self.record['steps']:
                self.start(name)
        if not self.observe(BUILDS):
            self.record['status'] = 'building'
            self.persist()
            return
        self.ensure_manifest()
        for name in PLATFORMS:
            if name not in self.record['steps']:
                self.start(name)
        self.record['status'] = 'candidate_staged' if self.observe(PLATFORMS) else 'staging'
        self.persist()

    def advance_packages(self):
        self.advance()
        if self.record['status'] != 'candidate_staged':
            return
        for name in PACKAGES:
            if name not in self.record['steps']:
                self.start(name)
        if not self.observe(PACKAGES):
            self.record['status'] = 'packages_building'
            self.persist()
            return
        if 'python_packages' not in self.record:
            packages = self.github.packages(self.record)
            self.record.update(python_packages=packages, python_packages_sha256=digest(packages))
            self.persist()
        packages = self.record['python_packages']
        require(digest(packages) == self.record['python_packages_sha256'], 'Saved Python release set changed')
        require(set(packages) == set(PACKAGES), 'Python release set is incomplete')
        for name, evidence in packages.items():
            step = self.record['steps'][name]
            artifacts = {item['name']: item for item in step['artifacts']}
            dist, manifest = package_tool.ARTIFACTS[name]
            require(evidence['source'] == self.record['candidate'] and evidence['version'] == self.record['version'] and
                    evidence['run_id'] == step['run_id'] and evidence['attempt'] == step['attempt'] and
                    evidence['distribution_artifact_id'] == artifacts[dist]['id'] and
                    evidence['manifest_artifact_id'] == artifacts[manifest]['id'], 'Saved Python artifacts changed')
            require(datetime.fromisoformat(evidence['inputs_expire_at'].replace('Z', '+00:00')) > now(),
                    'Python release artifacts expired; select a new candidate')
        self.record['status'] = 'packages_prepared'
        self.persist()

    def require_release_tag(self):
        tag = f'holder-v{self.record["version"]}'
        ref = self.api('git/ref/tags/' + tag, missing_ok=True)
        require(ref is not None, f'Create the reviewed release tag {tag} through the release procedure before publication; this command does not create tags')
        target = ref['object']
        for _ in range(5):
            if target['type'] != 'tag':
                break
            target = self.api('git/tags/' + target['sha'])['object']
        require(target['type'] == 'commit' and target['sha'] == self.record['candidate'],
                'Release tag differs from prepared candidate source')

    def prepare_publication(self):
        validate_record(self.record)
        require(self.record.get('python_packages') and
                digest(self.record['python_packages']) == self.record['python_packages_sha256'],
                'Publication requires an unchanged prepared Python release set')
        self.require_release_tag()
        document = {'schema': 1, **{key: self.record[key] for key in ('repository', 'candidate', 'version', 'python_packages')},
            'steps': {name: self.record['steps'][name] for name in PACKAGES},
            'manifest': {'evidence': {'core_selection': self.record['manifest']['evidence']['core_selection']}}}
        if 'python_publication_inputs' not in self.record:
            name = f'{self.record["version"]}-candidate-{self.record["number"]}'
            self.record['python_publication_inputs'] = {'document': json.loads(encode(document)),
                'path': f'python-manifests/{name}.json', 'branch': f'release/python/{name}', 'sha256': digest(document)}
            self.persist()
        snapshot = self.record['python_publication_inputs']
        require(snapshot['sha256'] == digest(snapshot['document']) == digest(document), 'Saved publication inputs changed')
        self.ensure_metadata(snapshot, {'release/' + snapshot['path']: encode(document)}, label='publication')

    def validate_publication(self, name, receipt):
        package, destination = PUBLICATIONS[name]
        source = self.record['python_packages'][package]
        expected = {'repository': self.record['repository'], 'candidate': self.record['candidate'],
            'version': self.record['version'], 'package': package, 'destination': destination,
            'prepared_ref': self.record['python_publication_inputs']['commit'], 'source_run_id': source['run_id'],
            'source_artifact_id': source['distribution_artifact_id'], 'files': source['files'],
            'index_url': publisher_tool.index(package, self.record['version'], destination)}
        require(all(receipt[key] == value for key, value in expected.items()) and receipt.get('verified_at'),
                'Publication receipt differs from prepared release')

    def advance_publication(self, package, destination):
        self.prepare_publication()
        name = f'{package}_{destination}'
        completed = self.record.setdefault('publications', {}).get(name)
        if completed:
            require(digest(completed['receipt']) == completed['sha256'], 'Saved publication receipt changed')
            self.validate_publication(name, completed['receipt'])
            self.github.publication_status(completed['receipt'])
        else:
            source = self.record['python_packages'][package]
            require(datetime.fromisoformat(source['inputs_expire_at'].replace('Z', '+00:00')) > now(),
                    'Prepared Python artifacts expired before publication; select a new candidate')
            if name not in self.record['steps']:
                self.start(name)
            if not self.inspect(name):
                self.record['status'] = 'publishing_python'
                self.persist()
                return
            receipt = self.github.publication(self.record, name)
            self.validate_publication(name, receipt)
            self.github.publication_status(receipt)
            step = self.record['steps'][name]
            self.record['publications'][name] = {'receipt': receipt, 'sha256': digest(receipt),
                'run_id': step['run_id'], 'attempt': step['attempt']}
        self.record['status'] = 'python_publication_completed'
        self.persist()

    def advance_signing(self):
        validate_record(self.record)
        require(self.record.get('manifest') and self.record['manifest'].get('commit') and
                digest(self.record['manifest']['document']) == self.record['manifest']['sha256'],
                'Linux signing requires the unchanged candidate manifest')
        self.validate_manifest(self.record['manifest']['document'], self.record['manifest']['evidence'])
        if 'linux_signing_input' not in self.record:
            require('linux' in self.record['steps'], 'Prepare Linux staging before requesting signing')
            require(self.inspect('linux'), 'Linux staging is not ready for signing')
            bundle = self.github.linux_bundle(self.record, 'linux')
            self.record.update(linux_signing_input=bundle, linux_signing_input_sha256=digest(bundle))
            self.persist()
        bundle = self.record['linux_signing_input']
        require(digest(bundle) == self.record['linux_signing_input_sha256'], 'Saved Linux signing input changed')
        linux_tool.bind(self.record, bundle)
        input_available = datetime.fromisoformat(bundle['artifact']['expires_at'].replace('Z', '+00:00')) > now()
        if 'linux_signing' not in self.record['steps']:
            require(input_available, 'Pinned Linux staging artifact expired before signing completed')
            self.start('linux_signing')
        if not self.inspect('linux_signing'):
            require(input_available, 'Pinned Linux staging artifact expired before signing completed')
            step = self.record['steps']['linux_signing']
            pending = self.api('actions/runs/' + step['run_id'] + '/pending_deployments') if step.get('run_id') else []
            waiting = any(item['environment']['name'] == 'linux-release-signing' for item in pending)
            self.record['status'] = 'awaiting_linux_signing_approval' if waiting else 'signing_linux'
            self.persist()
            return
        if 'linux_signed' not in self.record:
            signed = self.github.linux_bundle(self.record, 'linux_signing')
            require(signed['provenance'] == bundle['provenance'], 'Signed Linux provenance differs from staged input')
            metadata = signed['signing_metadata']
            require(metadata['appimage_version'] == self.record['version'] and
                    metadata['staging'] == {'repository': self.record['repository'], 'run_id': bundle['run_id'],
                        'artifact_id': str(bundle['artifact']['id']), 'archive_sha256': bundle['archive_sha256']} and
                    metadata['signing_workflow'] == {'repository': self.record['repository'],
                        'run_id': self.record['steps']['linux_signing']['run_id']}, 'Signed Linux metadata differs from the selected handoff')
            self.record.update(linux_signed=signed, linux_signed_sha256=digest(signed))
            self.persist()
        require(digest(self.record['linux_signed']) == self.record['linux_signed_sha256'], 'Saved signed Linux evidence changed')
        self.record['status'] = 'linux_signed'
        self.persist()

    def prepare_linux_promotion(self):
        validate_record(self.record)
        require(self.record.get('linux_signed') and digest(self.record['linux_signed']) == self.record['linux_signed_sha256'],
                'Linux promotion requires unchanged accepted signed evidence')
        signed, step = self.record['linux_signed'], self.record['steps']['linux_signing']
        require(signed['run_id'] == step['run_id'] and signed['attempt'] == step['attempt'] and
                signed['artifact'] == step['artifacts'][0], 'Signed Linux input differs from the accepted signing run')
        linux_tool.bind(self.record, signed)
        linux_release_files(self.record)
        self.require_release_tag()

    def advance_linux_promotion(self):
        self.prepare_linux_promotion()
        completed = self.record.get('linux_promoted')
        if completed:
            require(digest(completed['receipt']) == completed['sha256'], 'Saved Linux promotion receipt changed')
        observed = self.github.linux_promotion_status(self.record,
            release_id=completed['receipt']['release']['id'] if completed else None)
        if observed is not None:
            if ('linux_promotion' in self.record['steps'] and
                    self.record['steps']['linux_promotion']['status'] != 'dispatch_rejected'):
                self.inspect('linux_promotion', allow_failure=True)
            source = {'signing_run_id': self.record['linux_signed']['run_id'],
                'artifact_id': self.record['linux_signed']['artifact']['id'],
                'archive_sha256': self.record['linux_signed']['archive_sha256']}
            if completed:
                receipt = completed['receipt']
                require(receipt['source'] == source and receipt['repository'] == self.record['repository'] and
                        receipt['candidate'] == self.record['candidate'] and receipt['files'] == observed['files'] and
                        receipt['release']['tag'] == observed['release']['tag'],
                        'Accepted Linux release assets or source changed')
            else:
                receipt = {**observed, 'source': source,
                    'workflow_run_id': self.record['steps'].get('linux_promotion', {}).get('run_id')}
                completed = {'receipt': receipt, 'sha256': digest(receipt)}
                self.record['linux_promoted'] = completed
            completed['last_check'] = {**observed['release'], 'verified_at': observed['verified_at']}
            self.record['status'] = 'linux_promoted'
            self.persist()
            return
        require(not completed, 'Accepted Linux release assets are now incomplete')
        if 'linux_promotion' not in self.record['steps']:
            require(datetime.fromisoformat(self.record['linux_signed']['artifact']['expires_at'].replace('Z', '+00:00')) > now(),
                    'Signed Linux artifact expired before promotion; recover the exact bytes without rebuilding')
            self.start('linux_promotion')
        ready = self.inspect('linux_promotion')
        require(not ready, 'Promotion workflow succeeded but destination assets are incomplete; inspect and re-observe')
        self.record['status'] = 'promoting_linux'
        self.persist()

    def prepare_windows_promotion(self):
        validate_record(self.record)
        require(self.record['windows_sign_test_installer'], 'Windows delivery requires a candidate staged with tester signing enabled')
        require(self.record.get('manifest') and digest(self.record['manifest']['document']) == self.record['manifest']['sha256'],
                'Windows delivery requires the unchanged candidate manifest')
        self.validate_manifest(self.record['manifest']['document'], self.record['manifest']['evidence'])
        self.require_release_tag()
        if 'windows_delivery_input' not in self.record:
            require(self.inspect('windows'), 'Windows staging has not succeeded')
            bundle = self.github.windows_bundle(self.record)
            windows_tool.bind(self.record, bundle)
            self.record.update(windows_delivery_input=bundle, windows_delivery_input_sha256=digest(bundle))
            self.persist()
        bundle, step = self.record['windows_delivery_input'], self.record['steps']['windows']
        require(digest(bundle) == self.record['windows_delivery_input_sha256'], 'Saved Windows delivery evidence changed')
        artifact = next(item for item in step['artifacts'] if item['name'] == 'Holder-windows-test-signed-installer')
        require(bundle['run_id'] == step['run_id'] and bundle['attempt'] == step['attempt'] and bundle['artifact'] == artifact,
                'Windows input differs from the accepted staging run')
        windows_tool.bind(self.record, bundle)
        files = windows_tool.payload_files(bundle, self.record['version'])
        require({file['name'] for file in files} == windows_promotion.names(self.record['version']), 'Incomplete Windows release payload')

    def advance_windows_promotion(self):
        self.prepare_windows_promotion()
        completed = self.record.get('windows_promoted')
        if completed:
            require(digest(completed['receipt']) == completed['sha256'], 'Saved Windows promotion receipt changed')
        observed = self.github.windows_promotion_status(self.record,
            release_id=completed['receipt']['release']['id'] if completed else None)
        if observed is not None:
            if ('windows_promotion' in self.record['steps'] and
                    self.record['steps']['windows_promotion']['status'] != 'dispatch_rejected'):
                self.inspect('windows_promotion', allow_failure=True)
            source = {'staging_run_id': self.record['windows_delivery_input']['run_id'],
                'artifact_id': self.record['windows_delivery_input']['artifact']['id'],
                'archive_sha256': self.record['windows_delivery_input']['archive_sha256']}
            if completed:
                receipt = completed['receipt']
                require(receipt['source'] == source and receipt['repository'] == self.record['repository'] and
                        receipt['candidate'] == self.record['candidate'] and receipt['files'] == observed['files'] and
                        receipt['release']['tag'] == observed['release']['tag'],
                        'Accepted Windows release assets or source changed')
            else:
                receipt = {**observed, 'source': source,
                    'workflow_run_id': self.record['steps'].get('windows_promotion', {}).get('run_id')}
                completed = {'receipt': receipt, 'sha256': digest(receipt)}
                self.record['windows_promoted'] = completed
            completed['last_check'] = {**observed['release'], 'verified_at': observed['verified_at']}
            self.record['status'] = 'windows_promoted'
            self.persist()
            return
        require(not completed, 'Accepted Windows release assets are now incomplete')
        if 'windows_promotion' not in self.record['steps']:
            destination = promotion_tool.release_for(self.github, self.record['repository'], 'holder-v' + self.record['version'])
            require(destination is not None, 'Create and review the shared product draft before Windows delivery')
            require(destination['draft'] and not destination.get('immutable'),
                    'Published or immutable product release cannot receive missing Windows assets')
            require(datetime.fromisoformat(self.record['windows_delivery_input']['artifact']['expires_at'].replace('Z', '+00:00')) > now(),
                    'Windows tester artifact expired before promotion; recover the exact bytes without rebuilding')
            self.start('windows_promotion')
        ready = self.inspect('windows_promotion')
        require(not ready, 'Promotion workflow succeeded but destination assets are incomplete; inspect and re-observe')
        self.record['status'] = 'promoting_windows'
        self.persist()

    def prepare_macos_handoff(self):
        validate_record(self.record)
        require(self.record.get('manifest') and digest(self.record['manifest']['document']) == self.record['manifest']['sha256'],
                'macOS handoff requires the unchanged candidate manifest')
        self.validate_manifest(self.record['manifest']['document'], self.record['manifest']['evidence'])
        if 'macos_handoff' not in self.record:
            if not self.inspect('macos'):
                self.record['status'] = 'preparing_macos_handoff'
                self.persist()
                return False
            bundle = self.github.macos_bundle(self.record)
            macos_tool.bind(self.record, bundle)
            self.record.update(macos_handoff=bundle, macos_handoff_sha256=digest(bundle))
            self.persist()
        bundle, step = self.record['macos_handoff'], self.record['steps']['macos']
        require(digest(bundle) == self.record['macos_handoff_sha256'], 'Saved macOS handoff evidence changed')
        require(bundle['run_id'] == step['run_id'] and bundle['attempt'] == step['attempt'] and
                bundle['workflow_commit'] == step['workflow_commit'] and bundle['artifact'] == step['artifacts'][0],
                'macOS handoff differs from the accepted staging run')
        macos_tool.bind(self.record, bundle)
        return True

    def macos_source(self):
        bundle = self.record['macos_handoff']
        return {'staging_run_id': bundle['run_id'], 'artifact_id': bundle['artifact']['id'],
                'archive_sha256': bundle['archive_sha256']}

    def accept_macos(self, directory):
        require(self.prepare_macos_handoff(), 'Wait for the selected macOS staging run before accepting the disk image')
        files = macos_tool.files(directory, self.record['version'])
        completed = self.record.get('macos_verified')
        if completed:
            require(digest(completed['receipt']) == completed['sha256'], 'Saved macOS verification receipt changed')
            require(completed['receipt']['files'] == files and completed['receipt']['source'] == self.macos_source(),
                    'Final macOS files differ from the accepted handoff')
        else:
            result = macos_tool.verify(directory, self.record, self.record['macos_handoff'])
            require(result['files'] == files, 'macOS files changed before verification')
            receipt = {**result, 'source': self.macos_source(), 'candidate': self.record['candidate'],
                       'version': self.record['version'], 'verified_at': now().isoformat(timespec='seconds')}
            self.record['macos_verified'] = {'receipt': receipt, 'sha256': digest(receipt)}
        self.record['status'] = 'macos_verified'
        self.persist()

    def advance_macos_promotion(self, directory=None):
        require(self.prepare_macos_handoff(), 'Wait for the macOS staging handoff')
        verified = self.record.get('macos_verified')
        require(verified and digest(verified['receipt']) == verified['sha256'], 'macOS delivery requires accepted final verification')
        receipt = verified['receipt']
        require(receipt['source'] == self.macos_source() and receipt['candidate'] == self.record['candidate'] and
                receipt['version'] == self.record['version'], 'macOS verification belongs to a different handoff')
        self.require_release_tag()
        completed = self.record.get('macos_promoted')
        if completed:
            require(digest(completed['receipt']) == completed['sha256'], 'Saved macOS delivery receipt changed')
        observed = promotion_tool.observe(self.github, self.record['repository'], 'holder-v' + self.record['version'],
            receipt['files'], self.record['candidate'], release_id=completed['receipt']['release']['id'] if completed else None)
        if observed is None:
            require(not completed, 'Accepted macOS release assets are now incomplete')
            require(directory is not None, 'Supply --macos-directory with the accepted final files to resume missing uploads')
            macos_tool.promote(self.github, self.record['repository'], self.record['candidate'], self.record['version'],
                               directory, receipt['files'])
            observed = promotion_tool.observe(self.github, self.record['repository'], 'holder-v' + self.record['version'],
                receipt['files'], self.record['candidate'])
            require(observed is not None, 'macOS delivery is incomplete; resume with the accepted files')
        result = {**observed, 'source': self.macos_source(), 'verification_sha256': verified['sha256']}
        if completed:
            previous = completed['receipt']
            require(previous['files'] == result['files'] and previous['source'] == result['source'] and
                    previous['verification_sha256'] == result['verification_sha256'], 'Accepted macOS delivery identities changed')
        else:
            completed = {'receipt': result, 'sha256': digest(result)}
            self.record['macos_promoted'] = completed
        completed['last_check'] = {**observed['release'], 'verified_at': observed['verified_at']}
        self.record['status'] = 'macos_promoted'
        self.persist()

    def prepare_ppa_observation(self, revisions=None, architectures=None):
        validate_record(self.record)
        require(self.record['repository'] == 'HolderTeam/holder', 'The Holder PPA is associated with HolderTeam/holder releases')
        self.require_release_tag()
        keys = {package + ':' + series for series in ppa_tool.SERIES for package in ppa_tool.BINARIES}
        ppa = self.record.get('ppa')
        if ppa is None:
            plan = {'schema': 1, 'archive': ppa_tool.ARCHIVE,
                **{key: self.record[key] for key in ('repository', 'candidate', 'version')},
                'architectures': sorted(set(architectures or ['amd64'])), 'revisions': dict.fromkeys(sorted(keys), 1)}
            ppa = {'plan': plan, 'sha256': digest(plan), 'targets': {}, 'history': []}
            self.record['ppa'] = ppa
        plan = ppa['plan']
        require(ppa['sha256'] == digest(plan) and plan['schema'] == 1 and plan['archive'] == ppa_tool.ARCHIVE and
                all(plan[key] == self.record[key] for key in ('repository', 'candidate', 'version')) and
                set(ppa['targets']) <= keys and set(plan['revisions']) == keys and all(type(value) is int and value > 0 for value in plan['revisions'].values()),
                'Saved PPA observation plan changed')
        require(plan['architectures'] and plan['architectures'] == sorted(set(plan['architectures'])) and
                set(plan['architectures']) <= set(ppa_tool.ARCHITECTURES), 'Invalid PPA architecture coverage')
        require(architectures is None or sorted(set(architectures)) == plan['architectures'],
                'PPA architecture coverage differs from the saved plan')
        require(set(revisions or {}) <= keys, 'Invalid PPA revision target')
        for target in ppa['targets'].values():
            if target.get('completed'):
                require(digest(target['completed']['receipt']) == target['completed']['sha256'], 'Saved PPA completion receipt changed')
        changed = {}
        for key, revision in (revisions or {}).items():
            require(type(revision) is int and revision >= plan['revisions'][key], 'PPA revisions cannot move backwards')
            if revision != plan['revisions'][key]:
                changed[key] = revision
        if changed:
            ppa['history'].append({'plan': json.loads(encode(plan)), 'sha256': ppa['sha256'],
                'changed_at': now().isoformat(timespec='seconds'),
                'targets': {key: ppa['targets'].pop(key) for key in changed if key in ppa['targets']}})
            plan['revisions'].update(changed)
            ppa['sha256'] = digest(plan)
        self.persist()
        return ppa

    def advance_ppa_observation(self, revisions=None, architectures=None):
        ppa = self.prepare_ppa_observation(revisions, architectures)
        client = ppa_tool.Client()
        self.record['status'] = 'observing_ppa'
        self.persist()
        failed = []
        for key, revision in ppa['plan']['revisions'].items():
            package, series = key.split(':')
            version = ppa_tool.package_version(self.record['version'], revision, series)
            observation = client.observe(package, series, version, ppa['plan']['architectures'])
            target = ppa['targets'].setdefault(key, {})
            target.update(observation=observation, checked_at=now().isoformat(timespec='seconds'))
            self.persist()
            completed = target.get('completed')
            if completed:
                require(observation['status'] == 'available', 'Accepted PPA packages are no longer available: ' + key)
                require(completed['receipt'] == observation['receipt'], 'Accepted PPA publication/build IDs or file hashes changed: ' + key)
            elif observation['status'] == 'available':
                receipt = observation['receipt']
                target['completed'] = {'receipt': receipt, 'sha256': digest(receipt)}
            if observation['status'] == 'build_failed':
                failed.append(key)
            self.persist()
        require(not failed, 'Launchpad build failures require attention: ' + ', '.join(failed) + '; inspect logs and resume observation after repair')
        self.record['status'] = 'ppa_available' if all(target['observation']['status'] == 'available'
            for target in ppa['targets'].values()) else 'awaiting_ppa'
        self.persist()

    def advance_website_observation(self):
        validate_record(self.record)
        repository = self.record['repository']
        require(repository == website_tool.generator.REPOSITORY, 'The public website serves HolderTeam/holder releases')
        self.require_release_tag()
        completed = self.record.get('website_verified')
        if completed:
            require(digest(completed['receipt']) == completed['sha256'], 'Saved website receipt changed')
        releases = promotion_tool.pages(self.github, self.prefix + '/releases')
        result = website_tool.observe(releases, self.record['version'],
            assets=lambda release: promotion_tool.assets_for(self.github, repository, release),
            contents=lambda asset: self.github.asset_contents(repository, asset))
        if 'pending' in result:
            self.record['website_pending'] = result['pending']
            self.persist()
            require(not completed, 'Previously verified website is no longer serving the recorded release')
            self.record['status'] = 'awaiting_website'
        else:
            receipt = {**result['receipt'], 'candidate': self.record['candidate']}
            if completed:
                require(receipt == completed['receipt'], 'Accepted website release assets or feed changed')
            else:
                completed = {'receipt': receipt, 'sha256': digest(receipt)}
                self.record['website_verified'] = completed
            completed['checked_at'] = now().isoformat(timespec='seconds')
            self.record.pop('website_pending', None)
            self.record['status'] = 'website_verified'
        self.persist()

    def prepare_sdk_publication(self):
        validate_record(self.record)
        require(self.record.get('manifest') and digest(self.record['manifest']['document']) == self.record['manifest']['sha256'],
                'SDK publication requires the unchanged candidate manifest')
        manifest = self.record['manifest']
        self.validate_manifest(manifest['document'], manifest['evidence'])
        selection = manifest['evidence']['core_selection']
        sdk_release.validate_selection(selection, self.record['candidate'], self.record['version'])
        self.require_release_tag()
        backend = self.record['steps']['backend']
        names = {'core-selection', 'core-sdk-linux-Release', 'core-sdk-macos-Release', 'core-sdk-windows-Release'}
        pins = [item for item in backend['artifacts'] if item['name'] in names]
        require(len(pins) == 4 and {item['name'] for item in pins} == names, 'SDK artifact pins are incomplete')
        document = {'schema': 1, **{key: self.record[key] for key in ('repository', 'candidate', 'version')},
            'backend': {**{key: backend[key] for key in ('workflow', 'workflow_commit', 'run_id', 'attempt')}, 'artifacts': pins},
            'core_selection': selection, 'core_pin': manifest['document']['core']}
        if 'sdk_publication_inputs' not in self.record:
            name = f'{self.record["version"]}-candidate-{self.record["number"]}'
            self.record['sdk_publication_inputs'] = {'document': json.loads(encode(document)),
                'path': f'sdk-manifests/{name}.json', 'branch': f'release/sdk/{name}', 'sha256': digest(document)}
            self.persist()
        snapshot = self.record['sdk_publication_inputs']
        require(snapshot['sha256'] == digest(snapshot['document']) == digest(document), 'Saved SDK publication inputs changed')

    def require_sdk_inputs_available(self):
        require(all(datetime.fromisoformat(item['expires_at'].replace('Z', '+00:00')) > now()
                    for item in self.record['sdk_publication_inputs']['document']['backend']['artifacts']),
                'SDK publication inputs expired; recover the exact archives without rebuilding')

    def ensure_sdk_metadata(self):
        snapshot = self.record['sdk_publication_inputs']
        self.ensure_metadata(snapshot, {'release/' + snapshot['path']: encode(snapshot['document'])}, label='SDK publication')

    def advance_sdk_publication(self):
        self.prepare_sdk_publication()
        completed = self.record.get('core_sdk_published')
        if completed:
            require(digest(completed['receipt']) == completed['sha256'], 'Saved SDK publication receipt changed')
        observed = self.github.sdk_publication_status(self.record, completed['receipt']['release']['id'] if completed else None)
        if observed is not None:
            if ('core_sdk_publication' in self.record['steps'] and
                    self.record['steps']['core_sdk_publication']['status'] != 'dispatch_rejected'):
                self.inspect('core_sdk_publication', allow_failure=True)
            if completed:
                receipt = completed['receipt']
                require(receipt['files'] == observed['files'] and receipt['candidate'] == observed['candidate'] and
                        receipt['sdk_repository_tag_commit'] == observed['sdk_repository_tag_commit'] and
                        receipt['input_sha256'] == self.record['sdk_publication_inputs']['sha256'], 'Accepted SDK destination or input changed')
            else:
                receipt = {**observed, 'input_sha256': self.record['sdk_publication_inputs']['sha256'],
                    'workflow_run_id': self.record['steps'].get('core_sdk_publication', {}).get('run_id')}
                completed = {'receipt': receipt, 'sha256': digest(receipt)}
                self.record['core_sdk_published'] = completed
            completed['last_verified_at'] = observed['verified_at']
            self.record['status'] = 'core_sdk_published'
            self.persist()
            return
        require(not completed, 'Accepted SDK release is no longer published and complete')
        if 'core_sdk_publication' not in self.record['steps']:
            self.require_sdk_inputs_available()
            self.ensure_sdk_metadata()
            self.start('core_sdk_publication')
        ready = self.inspect('core_sdk_publication')
        require(not ready, 'SDK workflow succeeded but publication is incomplete; inspect and re-observe')
        self.record['status'] = 'publishing_core_sdk'
        self.persist()


def summary(record):
    lines = [f'# Holder {record["version"]} candidate {record["number"]}', '',
             f'Source: `{record["candidate"]}`', f'Status: **{record["status"]}**',
             'Windows staging: ' + ('tester-signed' if record['windows_sign_test_installer'] else 'unsigned'), '']
    for name, step in record['steps'].items():
        link = (f'https://github.com/{record["repository"]}/actions/runs/{step["run_id"]}'
                if step.get('run_id') else 'waiting for the dispatch result')
        lines.append(f'- {name}: {step["status"]} / {step.get("conclusion") or "pending"}; {link}; '
                     f'attempt {step.get("attempt", "pending")}; workflow `{step.get("workflow_commit", "pending")}`')
    if record.get('manifest'):
        manifest = record['manifest']
        lines += ['', f'Manifest SHA256: `{manifest["sha256"]}`',
                  f'Component inputs expire: {manifest["evidence"]["inputs_expire_at"]}']
        if manifest.get('commit'):
            lines.append(f'Manifest: https://github.com/{record["repository"]}/blob/{manifest["commit"]}/release/staging/{manifest["path"]}')
    lines += ['', '| Step | Artifact | ID | Expiry | Digest |', '| --- | --- | --- | --- | --- |']
    for name, step in record['steps'].items():
        for artifact in step.get('artifacts', []):
            lines.append(f'| {name} | {artifact["name"]} | {artifact["id"]} | '
                         f'{artifact["expires_at"]} | {artifact.get("digest") or "not supplied by GitHub"} |')
    if record.get('error'):
        lines += ['', 'Stopped: ' + record['error']]
    if record.get('python_packages'):
        lines += ['', f'Python release set SHA256: `{record["python_packages_sha256"]}`',
                  '| Package | File | SHA256 |', '| --- | --- | --- |']
        for name, evidence in record['python_packages'].items():
            for file in evidence['files']:
                lines.append(f'| {name} | {file["name"]} | {file["sha256"]} |')
    for name, item in record.get('publications', {}).items():
        lines += ['', f'{name}: published files verified at {item["receipt"]["verified_at"]}; '
                  f'receipt SHA256 `{item["sha256"]}`']
    if record.get('linux_signed'):
        lines += ['', f'Signed Linux evidence SHA256: `{record["linux_signed_sha256"]}`',
                  '| Signed file | SHA256 |', '| --- | --- |']
        lines += [f'| {file["name"]} | {file["sha256"]} |' for file in record['linux_signed']['files']]
    if record['status'] == 'awaiting_linux_signing_approval':
        lines += ['', 'Waiting for review in linux-release-signing. Inspect the workflow verification summary before approval.']
    if record.get('linux_promoted'):
        delivery = record['linux_promoted']
        current = delivery['last_check']
        lines += ['', f'Linux release assets: {current["url"]}; draft={current["draft"]}; prerelease={current["prerelease"]}',
                  f'Linux promotion receipt SHA256: `{delivery["sha256"]}`']
    if record.get('windows_promoted'):
        delivery = record['windows_promoted']
        current = delivery['last_check']
        lines += ['', f'Windows development assets: {current["url"]}; draft={current["draft"]}',
                  f'Windows promotion receipt SHA256: `{delivery["sha256"]}`']
    if record.get('macos_handoff'):
        handoff = record['macos_handoff']
        lines += ['', f'macOS handoff staging run: {handoff["run_id"]}; artifact ID: {handoff["artifact"]["id"]}',
                  f'macOS staging archive SHA256: `{handoff["archive_sha256"]}`']
    if record.get('macos_verified'):
        verification = record['macos_verified']
        lines += ['', f'macOS verification receipt SHA256: `{verification["sha256"]}`']
        lines += [f'- macOS {name}: {outcome}' for name, outcome in verification['receipt']['checks'].items()]
        lines += [f'- {file["name"]}: SHA256 `{file["sha256"]}`' for file in verification['receipt']['files']]
    if record.get('macos_promoted'):
        delivery = record['macos_promoted']
        lines += ['', f'macOS release assets: {delivery["last_check"]["url"]}',
                  f'macOS delivery receipt SHA256: `{delivery["sha256"]}`']
    if record.get('website_pending'):
        lines += ['', record['website_pending']]
    if record.get('website_verified'):
        website = record['website_verified']
        receipt = website['receipt']
        lines += ['', f'Website last verified at {website["checked_at"]} for {receipt["tag"]}',
                  f'Feed: {receipt["site"]}version.json',
                  f'Website receipt SHA256: `{website["sha256"]}`']
    if record.get('ppa'):
        ppa = record['ppa']
        lines += ['', 'PPA architecture coverage: ' + ', '.join(ppa['plan']['architectures']),
            f'PPA observation plan SHA256: `{ppa["sha256"]}`',
            '| Source / series | Expected version | State | Remote builds | Missing publication/index entries |',
            '| --- | --- | --- | --- | --- |']
        for key, revision in ppa['plan']['revisions'].items():
            package, series = key.split(':')
            observation = ppa['targets'].get(key, {}).get('observation', {})
            builds = '; '.join(f'{item["architecture"]}: {item["status"]}' +
                (f' ([log]({item["log_url"]}))' if item.get('log_url') else '') for item in observation.get('builds', []))
            lines.append(f'| {key} | {ppa_tool.package_version(record["version"], revision, series)} | '
                f'{observation.get("status", "not observed")} | {builds} | {", ".join(observation.get("missing", []))} |')
        lines += ['', 'PPA availability records public package metadata; source-content provenance, signed APT installation and upgrade checks remain separate.']
    if record.get('core_sdk_published'):
        delivery = record['core_sdk_published']
        receipt = delivery['receipt']
        lines += ['', f'Core SDK published: {receipt["release"]["url"]}',
                  f'SDK repository tag target: `{receipt["sdk_repository_tag_commit"]}`',
                  f'SDK publication receipt SHA256: `{delivery["sha256"]}`',
                  '| SDK archive | Producer commit | SHA256 |', '| --- | --- | --- |']
        lines += [f'| {item["name"]} | {item["commit"]} | {item["sha256"]} |' for item in receipt['index']['assets']]
    package_work = ('Python distributions prepared; publish-python selects one package and destination explicitly.'
                    if record.get('python_packages') else 'Python distribution preparation: run packages with this record.')
    lines += ['', package_work,
              'Remaining release work: platform/upgrade validation and signing handoffs;',
              'the maintainer publication decision; per-destination promotion/publication',
              'and package availability checks; website completion. Follow release/docs/runbook.md.',
              'prepare/packages dispatch preparation workflows; sign-linux requests the protected signing workflow;',
              'promote-linux/promote-windows deliver accepted platform assets; publish-sdk publishes the verified SDK set;',
              'publish-python uploads to the explicit destination; observe-ppa checks public build and package availability.']
    return '\n'.join(lines) + '\n'


def initialize(github, repository, candidate, version, number, windows_signed):
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository), 'Invalid repository')
    manifest_tool.provenance.commit(candidate)
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', version) and number > 0,
            'Use a numeric product version and positive candidate number')
    content = github.api(f'repos/{repository}/contents/VERSION?ref={candidate}')
    require(base64.b64decode(content['content']).decode().strip() == version,
            'Candidate root VERSION differs from expected product version')
    return {'schema': 1, 'repository': repository, 'candidate': candidate, 'version': version,
            'number': number, 'windows_sign_test_installer': windows_signed,
            'build_ref': f'rc/{version}-candidate-{number}', 'status': 'new', 'steps': {}}


def validate_record(record):
    require(record['schema'] == 1, 'Unsupported release record schema')
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', record['repository']), 'Invalid repository')
    manifest_tool.provenance.commit(record['candidate'])
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', record['version']) and
            isinstance(record['number'], int) and record['number'] > 0, 'Invalid candidate version/number')
    require(record['build_ref'] == f'rc/{record["version"]}-candidate-{record["number"]}',
            'Saved build branch differs from candidate identity')
    require(isinstance(record['windows_sign_test_installer'], bool), 'Invalid Windows staging policy')
    require(set(record['steps']) <= set(WORKFLOWS), 'Unsupported preparation workflow in release record')
    if record.get('python_publication_inputs'):
        snapshot = record['python_publication_inputs']
        name = f'{record["version"]}-candidate-{record["number"]}'
        require(snapshot['path'] == f'python-manifests/{name}.json' and snapshot['branch'] == f'release/python/{name}',
                'Saved publication path/branch differs from candidate identity')
        if snapshot.get('commit'):
            manifest_tool.provenance.commit(snapshot['commit'])
    require(set(record.get('publications', {})) <= set(PUBLICATIONS), 'Unsupported publication destination')
    if record.get('sdk_publication_inputs'):
        snapshot = record['sdk_publication_inputs']
        name = f'{record["version"]}-candidate-{record["number"]}'
        require(snapshot['path'] == f'sdk-manifests/{name}.json' and snapshot['branch'] == f'release/sdk/{name}',
                'Saved SDK input path/branch differs from candidate identity')
        if snapshot.get('commit'):
            manifest_tool.provenance.commit(snapshot['commit'])
    if record.get('manifest'):
        name = f'{record["version"]}-candidate-{record["number"]}'
        manifest = record['manifest']
        require(manifest['path'] == f'release-manifests/{name}.json' and
                manifest['branch'] == f'release/candidates/{name}', 'Saved manifest path/branch differs from candidate identity')
        if manifest.get('commit'):
            manifest_tool.provenance.commit(manifest['commit'])
    for name, step in record['steps'].items():
        require(step['workflow'] == WORKFLOWS[name], 'Saved workflow differs from the preparation allowlist')
        if 'run_id' in step:
            require(re.fullmatch(r'[0-9]+', str(step['run_id'])), 'Invalid saved run ID')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=(
        'prepare', 'packages', 'publish-python', 'sign-linux', 'promote-linux', 'promote-windows',
        'publish-sdk', 'observe-ppa', 'observe-website', 'prepare-macos', 'accept-macos', 'promote-macos', 'status'))
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--repository', default=None)
    parser.add_argument('--candidate')
    parser.add_argument('--version')
    parser.add_argument('--number', type=int)
    parser.add_argument('--package', choices=PACKAGES)
    parser.add_argument('--destination', choices=('pypi', 'testpypi'))
    parser.add_argument('--windows-sign-test-installer', choices=('true', 'false'))
    parser.add_argument('--ppa-revision', action='append', default=[], help='explicit packaging revision: package:series=N')
    parser.add_argument('--ppa-architecture', action='append', choices=ppa_tool.ARCHITECTURES)
    parser.add_argument('--macos-directory', type=Path, help='directory containing the final versioned DMG and checksum')
    parser.add_argument('--retry', choices=tuple(WORKFLOWS))
    parser.add_argument('--wait', action='store_true', help='keep observing until completed or stopped')
    parser.add_argument('--poll-seconds', type=int, default=30)
    args = parser.parse_args()
    if args.command == 'accept-macos' and args.macos_directory is None:
        parser.error('accept-macos requires --macos-directory')
    if args.macos_directory is not None and args.command not in ('accept-macos', 'promote-macos'):
        parser.error('--macos-directory requires accept-macos or promote-macos')
    if args.command in ('prepare-macos', 'accept-macos', 'promote-macos') and args.retry:
        parser.error('Resume the macOS handoff with the same record and files')
    if (args.ppa_revision or args.ppa_architecture) and args.command != 'observe-ppa':
        parser.error('PPA selections require observe-ppa')
    if args.command in ('observe-ppa', 'observe-website') and args.retry:
        parser.error('Observation commands do not retry workflows. Use the existing workflow entry point.')
    try:
        selected_ppa_revisions = ppa_tool.revisions(args.ppa_revision)
    except ValueError as error:
        parser.error(str(error))
    if args.record.suffix != '.json':
        parser.error('--record must name a .json file')
    if args.command == 'publish-python':
        if not args.package or not args.destination:
            parser.error('publish-python requires explicit --package and --destination')
        if args.retry and args.retry != f'{args.package}_{args.destination}':
            parser.error('--retry must match the selected package and destination')
    elif args.package or args.destination or args.retry in PUBLICATIONS:
        parser.error('Publication options require publish-python')
    if (args.retry == 'linux_signing' and args.command != 'sign-linux') or (args.command == 'sign-linux' and args.retry not in (None, 'linux_signing')):
        parser.error('Linux signing retries require sign-linux --retry linux_signing')
    if (args.retry == 'linux_promotion' and args.command != 'promote-linux') or (args.command == 'promote-linux' and args.retry not in (None, 'linux_promotion')):
        parser.error('Linux promotion retries require promote-linux --retry linux_promotion')
    if (args.retry == 'windows_promotion' and args.command != 'promote-windows') or (args.command == 'promote-windows' and args.retry not in (None, 'windows_promotion')):
        parser.error('Windows promotion retries require promote-windows --retry windows_promotion')
    if (args.retry == 'core_sdk_publication' and args.command != 'publish-sdk') or (args.command == 'publish-sdk' and args.retry not in (None, 'core_sdk_publication')):
        parser.error('SDK publication retries require publish-sdk --retry core_sdk_publication')
    if args.retry and args.command == 'status':
        parser.error('--retry requires a preparation or publication command')
    if args.retry in PACKAGES and args.command != 'packages':
        parser.error('Python package retries require packages')
    if args.wait and args.command == 'status':
        parser.error('--wait requires a preparation or publication command')
    if not 1 <= args.poll_seconds <= 60:
        parser.error('--poll-seconds must be between 1 and 60')
    github = GitHub()
    try:
        if args.command == 'status':
            require(args.record.is_file(), 'Release record does not exist')
            record = json.loads(args.record.read_text())
            validate_record(record)
            print(summary(record))
            return
        with lock(args.record):
            if args.record.exists():
                record = json.loads(args.record.read_text())
                validate_record(record)
                for key in ('repository', 'candidate', 'version', 'number'):
                    value = getattr(args, key)
                    require(value is None or value == record[key], f'{key} differs from the saved candidate')
                require(args.windows_sign_test_installer is None or
                        (args.windows_sign_test_installer == 'true') == record['windows_sign_test_installer'],
                        'Windows staging policy differs from the saved candidate')
            else:
                require(args.command == 'prepare' and args.candidate and args.version and args.number,
                        'New records need prepare --candidate <SHA> --version <version> --number <N>')
                record = initialize(github, args.repository or 'HolderTeam/holder', args.candidate,
                                    args.version, args.number, args.windows_sign_test_installer != 'false')
                save(args.record, record)
            if args.command in ('prepare', 'packages', 'publish-python', 'sign-linux', 'promote-linux', 'promote-windows', 'publish-sdk', 'observe-ppa', 'observe-website', 'prepare-macos', 'accept-macos', 'promote-macos'):
                coordinator = Coordinator(args.record, record, github)
                try:
                    record.pop('error', None)
                    if args.retry:
                        if args.command == 'publish-python':
                            coordinator.prepare_publication()
                        coordinator.retry(args.retry)
                    last_report = None
                    while True:
                        if args.command == 'prepare-macos':
                            if coordinator.prepare_macos_handoff():
                                record['status'] = 'macos_handoff_ready'
                                coordinator.persist()
                        elif args.command == 'accept-macos':
                            coordinator.accept_macos(args.macos_directory)
                        elif args.command == 'promote-macos':
                            coordinator.advance_macos_promotion(args.macos_directory)
                        elif args.command == 'observe-website':
                            coordinator.advance_website_observation()
                        elif args.command == 'observe-ppa':
                            coordinator.advance_ppa_observation(selected_ppa_revisions, args.ppa_architecture)
                        elif args.command == 'publish-sdk':
                            coordinator.advance_sdk_publication()
                        elif args.command == 'promote-windows':
                            coordinator.advance_windows_promotion()
                        elif args.command == 'promote-linux':
                            coordinator.advance_linux_promotion()
                        elif args.command == 'sign-linux':
                            coordinator.advance_signing()
                        elif args.command == 'publish-python':
                            coordinator.advance_publication(args.package, args.destination)
                        elif args.command == 'packages':
                            coordinator.advance_packages()
                        else:
                            coordinator.advance()
                        report = summary(record)
                        args.record.with_suffix('.summary.md').write_text(report)
                        if report != last_report:
                            print(report, flush=True)
                            last_report = report
                        target = {'packages': 'packages_prepared', 'prepare': 'candidate_staged',
                                  'publish-python': 'python_publication_completed', 'sign-linux': 'linux_signed',
                                  'promote-linux': 'linux_promoted', 'promote-windows': 'windows_promoted', 'publish-sdk': 'core_sdk_published', 'observe-ppa': 'ppa_available',
                                  'observe-website': 'website_verified', 'prepare-macos': 'macos_handoff_ready',
                                  'accept-macos': 'macos_verified', 'promote-macos': 'macos_promoted'}[args.command]
                        if not args.wait or record['status'] == target:
                            break
                        time.sleep(args.poll_seconds)
                except (ValueError, APIError, KeyError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile, tarfile.TarError) as error:
                    record.update(status='stopped', error=str(error))
                    coordinator.persist()
                    raise
                finally:
                    args.record.with_suffix('.summary.md').write_text(summary(record))
    except (ValueError, APIError, KeyError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile, tarfile.TarError) as error:
        parser.exit(1, f'Release operation stopped: {error}\n')
    except KeyboardInterrupt:
        parser.exit(130, 'Interrupted; resume the command with the same release record\n')


if __name__ == '__main__':
    main()
