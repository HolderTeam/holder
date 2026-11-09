#!/usr/bin/env python3
"""Resume candidate preparation and Python publication using one local release record."""
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
PLATFORMS = ('linux', 'windows', 'macos')
STAGED = {'linux': ['Holder-linux-appimage-staged'], 'windows': ['Holder-windows-staged'],
          'macos': ['Holder-macos-staged']}


def encode(value):
    return json.dumps(value, indent=2) + '\n'


def digest(value):
    return hashlib.sha256(encode(value).encode()).hexdigest()


def now():
    return datetime.now(timezone.utc)


class APIError(RuntimeError):
    def __init__(self, status, message):
        super().__init__(f'GitHub HTTP {status or "unknown"}: {message}')
        self.status = status


class GitHub:
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

    def inspect(self, name):
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
    package_work = ('Python distributions prepared; publish-python selects one package and destination explicitly.'
                    if record.get('python_packages') else 'Python distribution preparation: run packages with this record.')
    lines += ['', package_work,
              'Remaining release work: platform/upgrade validation and signing handoffs;',
              'the maintainer publication decision; per-destination promotion/publication',
              'and package availability checks; website completion. Follow release/docs/runbook.md.',
              'prepare/packages dispatch preparation workflows; publish-python uploads to the explicit destination.']
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
    parser.add_argument('command', choices=('prepare', 'packages', 'publish-python', 'status'))
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--repository', default=None)
    parser.add_argument('--candidate')
    parser.add_argument('--version')
    parser.add_argument('--number', type=int)
    parser.add_argument('--package', choices=PACKAGES)
    parser.add_argument('--destination', choices=('pypi', 'testpypi'))
    parser.add_argument('--windows-sign-test-installer', choices=('true', 'false'))
    parser.add_argument('--retry', choices=tuple(WORKFLOWS))
    parser.add_argument('--wait', action='store_true', help='keep observing until completed or stopped')
    parser.add_argument('--poll-seconds', type=int, default=30)
    args = parser.parse_args()
    if args.record.suffix != '.json':
        parser.error('--record must name a .json file')
    if args.command == 'publish-python':
        if not args.package or not args.destination:
            parser.error('publish-python requires explicit --package and --destination')
        if args.retry and args.retry != f'{args.package}_{args.destination}':
            parser.error('--retry must match the selected package and destination')
    elif args.package or args.destination or args.retry in PUBLICATIONS:
        parser.error('Publication options require publish-python')
    if args.retry and args.command == 'status':
        parser.error('--retry requires prepare, packages or publish-python')
    if args.retry in PACKAGES and args.command != 'packages':
        parser.error('Python package retries require packages')
    if args.wait and args.command == 'status':
        parser.error('--wait requires prepare, packages or publish-python')
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
            if args.command in ('prepare', 'packages', 'publish-python'):
                coordinator = Coordinator(args.record, record, github)
                try:
                    record.pop('error', None)
                    if args.retry:
                        if args.command == 'publish-python':
                            coordinator.prepare_publication()
                        coordinator.retry(args.retry)
                    last_report = None
                    while True:
                        if args.command == 'publish-python':
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
                                  'publish-python': 'python_publication_completed'}[args.command]
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
