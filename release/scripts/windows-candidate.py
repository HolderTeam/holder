#!/usr/bin/env python3
"""Record and verify the final Windows tester installer without executing it."""
import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import stat
import tempfile
import zipfile

SPEC = importlib.util.spec_from_file_location('windows_manifest', Path(__file__).parents[1] / 'staging/scripts/generate-release-manifest.py')
manifest = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(manifest)
require = manifest.require


def names(version):
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:[-+][0-9A-Za-z.+-]+)?', version), 'Invalid Windows version')
    return [f'installer/Holder-{version}-Setup-test-signed.exe',
            'certificate/Holder-windows-test-signing.cer',
            'certificate/Holder-windows-test-signing-fingerprint.txt']


def fingerprint(data):
    return hashlib.sha256(data).hexdigest()


def certificate(files):
    values = re.findall(r'^SHA256: ([0-9A-Fa-f]{64})\s*$', files[names('0.0.0')[2]].decode('utf-8-sig'), re.MULTILINE)
    require(len(values) == 1 and values[0].lower() == fingerprint(files[names('0.0.0')[1]]),
            'Windows certificate differs from its public fingerprint')


def record(directory, metadata, repository, run_id, workflow_commit):
    version = metadata['version']
    files = {}
    for name in names(version):
        path = directory / name
        require(path.is_file() and not path.is_symlink(), 'Missing Windows tester file: ' + name)
        files[name] = path.read_bytes()
    certificate(files)
    require(metadata['platform'] == 'windows' and metadata['staging_repository'] == repository and
            str(metadata['staging_run_id']) == str(run_id), 'Windows staging identity differs')
    manifest.provenance.commit(workflow_commit)
    provenance = {'appimage_version': version, 'core': metadata['core']}
    for name in ('desktop', 'backend'):
        provenance[name] = {'repository': metadata[name + '_repository'], 'run_id': str(metadata[name + '_run_id']),
            'commit': metadata['desktop_commit' if name == 'desktop' else 'backend_run_commit'], 'version': version}
    result = {'schema': 1, 'channel': 'windows-dev', 'version': version,
        'staging': {'repository': repository, 'run_id': str(run_id), 'workflow_commit': workflow_commit},
        'provenance': provenance,
        'files': [{'name': name, 'size': len(data), 'sha256': fingerprint(data)} for name, data in sorted(files.items())]}
    (directory / 'release-evidence.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


def read(repository, identifier, artifact_name, version, github, *, artifact_id=None,
         archive_sha256=None, workflow_commit=None, clock=None):
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository), 'Invalid repository')
    require(str(identifier).isdecimal(), 'Invalid Windows staging run ID')
    workflow = 'staging-windows-stage.yml'
    run = github.request(f'repos/{repository}/actions/runs/{identifier}')
    manifest.provenance.commit(run['head_sha'])
    require(workflow_commit is None or workflow_commit == run['head_sha'], 'Windows workflow source changed')
    manifest.run(repository, identifier, workflow, run['head_sha'], github.request)
    item = manifest.artifacts(repository, run, github.request, clock or datetime.now(timezone.utc))(artifact_name)
    require(artifact_id is None or str(item['id']) == str(artifact_id), 'Windows artifact ID changed')
    metadata, checksum = github.read(repository, item, ['release-evidence.json'])
    require(archive_sha256 is None or checksum == archive_sha256, 'Windows archive checksum changed')
    evidence = json.loads(metadata['release-evidence.json'])
    version = version or evidence['version']
    expected = set(names(version)) | {'release-evidence.json'}
    require(evidence['schema'] == 1 and evidence['channel'] == 'windows-dev' and evidence['version'] == version,
            'Unexpected Windows tester evidence/version')
    require(evidence['staging'] == {'repository': repository, 'run_id': str(identifier), 'workflow_commit': run['head_sha']},
            'Windows evidence belongs to another staging run')
    require(evidence['provenance']['appimage_version'] == version and
            all(evidence['provenance'][name]['version'] == version for name in ('desktop', 'backend')),
            'Windows component versions differ from the tester version')
    records, certificate_files = [], {}
    with zipfile.ZipFile(github.directory / f"{item['id']}.zip") as archive:
        entries = [entry for entry in archive.infolist() if not entry.is_dir()]
        require(len(entries) == len(expected) and {entry.filename for entry in entries} == expected,
                'Unexpected or duplicate Windows tester files')
        require(all(entry.filename in ('installer/', 'certificate/') for entry in archive.infolist() if entry.is_dir()),
                'Unexpected Windows bundle directory')
        for entry in sorted(entries, key=lambda value: value.filename):
            require(stat.S_IFMT(entry.external_attr >> 16) in (0, stat.S_IFREG), 'Windows bundle contains a link or special file')
            digest = hashlib.sha256()
            with archive.open(entry) as source:
                for chunk in iter(lambda: source.read(1024 * 1024), b''):
                    digest.update(chunk)
            records.append({'name': entry.filename, 'size': entry.file_size, 'sha256': digest.hexdigest()})
            if entry.filename.startswith('certificate/'):
                require(entry.file_size < 1024 * 1024, 'Oversized Windows certificate metadata')
                certificate_files[entry.filename] = archive.read(entry)
    require(evidence['files'] == [file for file in records if file['name'] != 'release-evidence.json'],
            'Windows final bytes differ from staging evidence')
    certificate(certificate_files)
    current = manifest.run(repository, identifier, workflow, run['head_sha'], github.request)
    require(current['run_attempt'] == run['run_attempt'], 'Windows staging was retried during verification')
    return {'run_id': str(identifier), 'attempt': run['run_attempt'], 'workflow_commit': run['head_sha'],
        'artifact': {key: item.get(key) for key in ('id', 'name', 'digest', 'size_in_bytes', 'expires_at')},
        'archive_sha256': checksum, 'files': records, 'provenance': evidence['provenance']}


def bind(record, bundle):
    spec = importlib.util.spec_from_file_location('windows_candidate_policy', Path(__file__).with_name('candidate-provenance.py'))
    policy = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(policy)
    policy.bind(record, bundle['provenance'], 'windows')


def payload_files(bundle, version):
    source = {file['name']: file for file in bundle['files']}
    result = []
    target = [f'Holder-windows-{version}-dev-Setup.exe', 'Holder-windows-dev-test-signing.cer',
              'Holder-windows-dev-test-signing-fingerprint.txt']
    for old, new in zip(names(version), target):
        result.append({**source[old], 'name': new})
    data = f'{result[0]["sha256"]}  {target[0]}\n'.encode()
    result.append({'name': target[0] + '.sha256', 'size': len(data), 'sha256': fingerprint(data)})
    return sorted(result, key=lambda value: value['name'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('record', 'fetch'))
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--metadata', type=Path)
    parser.add_argument('--repository', required=True)
    parser.add_argument('--run-id', required=True)
    parser.add_argument('--workflow-commit')
    parser.add_argument('--version')
    parser.add_argument('--artifact-name', default='Holder-windows-test-signed-installer')
    parser.add_argument('--artifact-id')
    parser.add_argument('--archive-sha256')
    parser.add_argument('--candidate')
    args = parser.parse_args()
    if args.command == 'record':
        require(args.metadata and args.workflow_commit, 'record requires metadata and workflow commit')
        record(args.directory, json.loads(args.metadata.read_text(encoding='utf-8-sig')),
               args.repository, args.run_id, args.workflow_commit)
        return
    require(not args.directory.exists(), 'fetch requires a new output directory')
    require(bool(args.artifact_id) == bool(args.archive_sha256), 'Supply both Windows artifact pins together')
    if args.artifact_id:
        require(args.artifact_id.isdecimal() and re.fullmatch(r'[0-9a-f]{64}', args.archive_sha256), 'Invalid Windows artifact pins')
    with tempfile.TemporaryDirectory(prefix='holder-windows-candidate-') as directory:
        github = manifest.GitHub(directory)
        bundle = read(args.repository, args.run_id, args.artifact_name, args.version, github,
                      artifact_id=args.artifact_id, archive_sha256=args.archive_sha256)
        args.version = args.version or bundle['provenance']['appimage_version']
        if args.candidate:
            manifest.provenance.commit(args.candidate)
            require(all(bundle['provenance'][name]['commit'] == args.candidate and
                        bundle['provenance'][name]['version'] == args.version for name in ('desktop', 'backend')),
                    'Windows tester differs from candidate source/version')
        if args.candidate:
            require(args.artifact_id and args.archive_sha256, 'Candidate delivery requires immutable artifact pins')
        args.directory.mkdir(parents=True)
        targets = [f'Holder-windows-{args.version}-dev-Setup.exe', 'Holder-windows-dev-test-signing.cer',
                   'Holder-windows-dev-test-signing-fingerprint.txt']
        with zipfile.ZipFile(github.directory / f"{bundle['artifact']['id']}.zip") as archive:
            for old, new in zip(names(args.version), targets):
                with archive.open(old) as source, (args.directory / new).open('wb') as output:
                    shutil.copyfileobj(source, output)
        digest = next(file['sha256'] for file in bundle['files'] if file['name'] == names(args.version)[0])
        (args.directory / (targets[0] + '.sha256')).write_bytes(f'{digest}  {targets[0]}\n'.encode())


if __name__ == '__main__':
    main()
