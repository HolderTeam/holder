#!/usr/bin/env python3
"""Generate a framework manifest from explicit, verified Release component runs."""
import argparse
import base64
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
import zipfile


def sibling(name):
    spec = importlib.util.spec_from_file_location(name.replace('-', '_'), Path(__file__).with_name(name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


components = sibling('resolve-component-runs')
provenance = sibling('core-provenance')
require = provenance.require


def file_sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


class GitHub:
    def __init__(self, directory):
        self.directory = Path(directory)
        self.cache = {}

    def request(self, path, **query):
        # Run state is deliberately not cached: detect a rerun during generation.
        return components.api(path, **query)

    def read(self, repository, artifact, text_files, required_files=()):
        path = self.directory / f"{artifact['id']}.zip"
        if artifact['id'] not in self.cache:
            with path.open('wb') as output:
                subprocess.run(['gh', 'api', f"repos/{repository}/actions/artifacts/{artifact['id']}/zip"],
                               stdout=output, check=True)
            self.cache[artifact['id']] = file_sha256(path)
        if artifact.get('digest'):
            require(artifact['digest'] == 'sha256:' + self.cache[artifact['id']],
                    f"Artifact {artifact['name']} ZIP checksum differs from GitHub's digest")
        with zipfile.ZipFile(path) as archive:
            if artifact['name'] in ('holder-desktop-linux-x86_64-release', 'holder-daemon-linux-x86_64-release'):
                filename = artifact['name'].removesuffix('-release') + '.tar.gz'
                for name in (filename, filename + '.sha256'):
                    require(archive.namelist().count(name) == 1, f"Artifact {artifact['name']} needs one {name}")
                nested = self.directory / f"{artifact['id']}.tar.gz"
                with archive.open(filename) as source, nested.open('wb') as output:
                    shutil.copyfileobj(source, output)
                checksum = file_sha256(nested)
                expected = archive.read(filename + '.sha256').decode().split()
                require(expected == [checksum, filename], f"Artifact {artifact['name']} tarball checksum differs")
                with tarfile.open(nested) as payload:
                    values = {}
                    for name in set(text_files) | set(required_files):
                        matches = [entry for entry in payload.getmembers()
                                   if entry.name.removeprefix('./') == name and entry.isfile()]
                        require(len(matches) == 1, f"Artifact {artifact['name']} must contain exactly one {name}")
                        if name in text_files:
                            require(matches[0].size <= 1024 * 1024, f'{name} metadata is unexpectedly large')
                            values[name] = payload.extractfile(matches[0]).read().decode('utf-8-sig').strip()
                return values, self.cache[artifact['id']]
            for name in set(text_files) | set(required_files):
                matches = [entry for entry in archive.infolist() if entry.filename == name and not entry.is_dir()]
                require(len(matches) == 1, f"Artifact {artifact['name']} must contain exactly one {name}")
                if name in text_files:
                    require(matches[0].file_size <= 1024 * 1024, f'{name} metadata is unexpectedly large')
            # Read only metadata; never extract or execute downloaded component code.
            values = {name: archive.read(name).decode('utf-8-sig').strip() for name in text_files}
        return values, self.cache[artifact['id']]


def run(repository, identifier, workflow, candidate, request):
    require(str(identifier).isdecimal(), 'Component run IDs must be numeric')
    result = request(f'repos/{repository}/actions/runs/{identifier}')
    require(str(result['id']) == str(identifier) and result['repository']['full_name'] == repository,
            f'Run {identifier} belongs to a different repository or run')
    require(result['path'] == f'.github/workflows/{workflow}', f'Run {identifier} is not {workflow}')
    require(result['status'] == 'completed' and result['conclusion'] == 'success',
            f'Run {identifier} must be completed successfully')
    require(result['head_sha'] == candidate, f'Run {identifier} was not built from candidate {candidate}')
    return result


def artifacts(repository, selected_run, request, now):
    found = []
    page = 1
    while True:
        result = request(f"repos/{repository}/actions/runs/{selected_run['id']}/artifacts",
                         per_page='100', page=str(page))['artifacts']
        found.extend(result)
        if len(result) < 100:
            break
        page += 1

    def select(name):
        matches = [item for item in found if item['name'] == name]
        require(len(matches) == 1, f"Run {selected_run['id']} needs exactly one {name} artifact; found {len(matches)}")
        item = matches[0]
        require(not item['expired'] and
                datetime.fromisoformat(item['expires_at'].replace('Z', '+00:00')) > now,
                f"Artifact {name} has expired; rebuild the candidate components")
        owner = item['workflow_run']
        require(owner['id'] == selected_run['id'] and owner['head_sha'] == selected_run['head_sha'],
                f'Artifact {name} does not belong to the selected candidate run')
        return item
    return select


def generate(repository, candidate, version, desktop_run, backend_run, github, now=None):
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository), 'Invalid repository')
    provenance.commit(candidate)
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', version), 'Expected a numeric product version')
    now = now or datetime.now(timezone.utc)
    request = github.request
    content = request(f'repos/{repository}/contents/VERSION', ref=candidate)
    actual_version = base64.b64decode(content['content']).decode().strip()
    require(actual_version == version, f'Candidate root VERSION is {actual_version}, expected {version}')
    desktop = run(repository, desktop_run, 'desktop-ci.yml', candidate, request)
    backend = run(repository, backend_run, 'daemon-ci.yml', candidate, request)
    desktop_artifact = artifacts(repository, desktop, request, now)
    backend_artifact = artifacts(repository, backend, request, now)
    evidence = {'candidate': {'repository': repository, 'commit': candidate, 'version': version},
                'runs': {name: {'id': value['id'], 'attempt': value['run_attempt'],
                                'commit': value['head_sha'], 'workflow': value['path']}
                         for name, value in (('desktop', desktop), ('backend', backend))},
                'artifacts': []}

    def read(item, selected_run, text_files, required_files=()):
        values, digest = github.read(repository, item, text_files, required_files)
        evidence['artifacts'].append({'id': item['id'], 'name': item['name'],
                                      'run_id': selected_run['id'], 'expires_at': item['expires_at'],
                                      'github_digest': item.get('digest'), 'archive_sha256': digest})
        return values

    selection_file = 'core-selection.json'
    selection_artifact = backend_artifact('core-selection')
    selection = json.loads(read(selection_artifact, backend, [selection_file])[selection_file])
    selection_digest = evidence['artifacts'][-1]['archive_sha256']
    core_tree = components.subtree(repository, candidate, ('core',), request)
    require(selection['source'] == 'artifact' and selection['commit'] == candidate,
            'Core selection is not the selected candidate run selection')
    require(selection['version'] == version and selection.get('core_tree') == core_tree,
            'Core selection version/tree differs from the candidate')
    manifest = {'version': version, 'core': {'commit': selection['commit'], 'build_type': 'Release',
                                           'selection_artifact_id': str(selection_artifact['id']),
                                           'selection_artifact_sha256': selection_digest},
                'platforms': {}}
    checked_sdk_commits = {candidate}
    for platform, (desktop_name, backend_name) in components.ARTIFACTS.items():
        pin = {'core': manifest['core'], 'appimage_version': version,
               'desktop': {'repository': repository, 'run_id': str(desktop['id']), 'commit': candidate},
               'backend': {'repository': repository, 'run_id': str(backend['id']), 'commit': candidate}}
        info_path = 'release/holder-desktop-build.json'
        desktop_binary = {'linux': 'usr/bin/holder-desktop', 'windows': 'bin/holder-desktop.exe',
                          'macos': 'bin/holder-desktop'}[platform]
        desktop_item = desktop_artifact(desktop_name + '-release')
        desktop_info = json.loads(read(desktop_item, desktop,
                                       [info_path], [desktop_binary])[info_path])
        pin['desktop'].update(artifact_id=str(desktop_item['id']),
                              artifact_sha256=evidence['artifacts'][-1]['archive_sha256'])
        provenance.verify_component_builds(pin, desktop_info)
        require(desktop_info['version'] == version, f'{platform} desktop version differs from candidate')
        core_path = 'usr/share/holder-daemon/core-build.json' if platform == 'linux' else 'core-build.json'
        commit_path = 'release/holder-daemon-commit.txt'
        version_path = 'release/holder-daemon-version.txt'
        binaries = {'linux': ['usr/bin/holderd', 'usr/bin/holderctl'],
                    'windows': ['holderd.exe', 'holderctl.exe'],
                    'macos': ['bin/holderd', 'bin/holderctl']}[platform]
        backend_item = backend_artifact(backend_name + '-release')
        metadata = read(backend_item, backend,
                        [core_path, commit_path, version_path], binaries)
        pin['backend'].update(artifact_id=str(backend_item['id']),
                              artifact_sha256=evidence['artifacts'][-1]['archive_sha256'])
        require(metadata[version_path] == version, f'{platform} daemon version differs from candidate')
        core = json.loads(metadata[core_path])
        result = provenance.verify(selection, core, desktop_info['commit'], metadata[commit_path], pin,
                                   platform, 'arm64' if platform == 'macos' else 'x86_64',
                                   'Release', repository, str(backend['id']))
        asset = result['sdk_asset']
        require(re.fullmatch(r'[0-9a-f]{64}', asset['sha256']), f'{platform} SDK has no valid checksum')
        sdk_commit = provenance.commit(core['commit'])
        if sdk_commit not in checked_sdk_commits:
            require(components.subtree(repository, sdk_commit, ('core',), request) == core_tree and
                    components.version_blob(repository, sdk_commit, request) ==
                    components.version_blob(repository, candidate, request),
                    f'{platform} cached SDK source/version differs from candidate')
            checked_sdk_commits.add(sdk_commit)
        manifest['platforms'][platform] = {name: pin[name] for name in ('desktop', 'backend')}
    # Preserve each SDK producer and digest, separately from the run/candidate SHA.
    evidence['core_selection'] = selection
    evidence['inputs_expire_at'] = min(item['expires_at'] for item in evidence['artifacts'])
    for identifier, workflow, selected in ((desktop_run, 'desktop-ci.yml', desktop),
                                            (backend_run, 'daemon-ci.yml', backend)):
        current = run(repository, identifier, workflow, candidate, request)
        require(current['run_attempt'] == selected['run_attempt'],
                f'Run {identifier} was retried during generation; select inputs again')
    return manifest, evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repository', default='HolderTeam/holder')
    parser.add_argument('--candidate', required=True, help='full candidate commit SHA')
    parser.add_argument('--version', required=True, help='expected root product version')
    parser.add_argument('--desktop-run', required=True)
    parser.add_argument('--backend-run', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    evidence_path = args.output.with_suffix('.evidence.json')
    if args.output.exists() or evidence_path.exists():
        parser.error('Candidate output already exists; choose a new candidate filename')
    try:
        with tempfile.TemporaryDirectory(prefix='holder-release-inputs-') as directory:
            manifest, evidence = generate(args.repository, args.candidate, args.version,
                                          args.desktop_run, args.backend_run, GitHub(directory))
    except (ValueError, KeyError, zipfile.BadZipFile, tarfile.TarError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'Cannot generate release manifest: {error}\n')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive creation preserves existing candidate records, including on retries.
    with args.output.open('x') as output:
        output.write(json.dumps(manifest, indent=2) + '\n')
    with evidence_path.open('x') as output:
        output.write(json.dumps(evidence, indent=2) + '\n')
    print(f'Wrote {args.output} and {evidence_path}; inputs expire at {evidence["inputs_expire_at"]}')


if __name__ == '__main__':
    main()
