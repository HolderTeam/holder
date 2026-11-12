#!/usr/bin/env python3
"""Read and pin Linux staging/signing bundles without executing their contents."""
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

SPEC = importlib.util.spec_from_file_location('linux_manifest',
    Path(__file__).parents[1] / 'staging/scripts/generate-release-manifest.py')
manifest_tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(manifest_tool)
require = manifest_tool.require


def files(version, signed=False):
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:[-+][0-9A-Za-z.+-]+)?', version), 'Invalid candidate version')
    stem = f'Holder-{version}'
    expected = {stem + '-x86_64.AppImage', stem + '-provenance.json', stem + '-SHA256SUMS'}
    return expected | ({stem + '-signing-metadata.json', 'Holder-linux-release-key.asc',
                        stem + '-embedded-signature.asc', stem + '-SHA256SUMS.asc'} if signed else
                       {stem + '-x86_64.AppDir.tar.gz'})


def read(repository, identifier, artifact_name, version, github, *, signed=False,
         artifact_id=None, archive_sha256=None, workflow_commit=None, clock=None):
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository), 'Invalid repository')
    require(str(identifier).isdecimal(), 'Invalid Linux run ID')
    require(artifact_id is None or str(artifact_id).isdecimal(), 'Invalid artifact ID')
    require(archive_sha256 is None or re.fullmatch(r'[0-9a-f]{64}', archive_sha256), 'Invalid artifact checksum')
    expected = files(version, signed)
    workflow = 'sign-linux-appimage.yml' if signed else 'staging-linux-appimage-stage.yml'
    route = f'repos/{repository}/actions/runs/{identifier}'
    run = github.request(route)
    manifest_tool.provenance.commit(run['head_sha'])
    if workflow_commit:
        require(run['head_sha'] == workflow_commit, 'Linux workflow source changed')
    manifest_tool.run(repository, identifier, workflow, run['head_sha'], github.request)
    select = manifest_tool.artifacts(repository, run, github.request, clock or datetime.now(timezone.utc))
    item = select(artifact_name)
    require(artifact_id is None or str(item['id']) == str(artifact_id), 'Linux artifact ID changed')
    metadata_names = [f'Holder-{version}-provenance.json', f'Holder-{version}-SHA256SUMS']
    if signed:
        metadata_names.append(f'Holder-{version}-signing-metadata.json')
    metadata, checksum = github.read(repository, item, metadata_names, expected)
    require(archive_sha256 is None or checksum == archive_sha256, 'Linux artifact archive checksum changed')
    checksums = []
    records = []
    with zipfile.ZipFile(github.directory / f"{item['id']}.zip") as archive:
        require({entry.filename for entry in archive.infolist()} == expected and len(archive.infolist()) == len(expected),
                'Unexpected or duplicate Linux bundle files')
        for entry in sorted(archive.infolist(), key=lambda value: value.filename):
            require(stat.S_IFMT(entry.external_attr >> 16) in (0, stat.S_IFREG), 'Linux bundle contains a link or special file')
            digest = hashlib.sha256()
            with archive.open(entry) as source:
                for chunk in iter(lambda: source.read(1024 * 1024), b''):
                    digest.update(chunk)
            records.append({'name': entry.filename, 'size': entry.file_size, 'sha256': digest.hexdigest()})
        for name in [f'Holder-{version}-x86_64.AppImage'] + ([] if signed else [f'Holder-{version}-x86_64.AppDir.tar.gz']):
            file = next(value for value in records if value['name'] == name)
            checksums.append(f'{file["sha256"]}  {name}')
    require(metadata[f'Holder-{version}-SHA256SUMS'].splitlines() == checksums, 'Linux payload checksums differ')
    current = manifest_tool.run(repository, identifier, workflow, run['head_sha'], github.request)
    require(current['run_attempt'] == run['run_attempt'], 'Linux run was retried during bundle verification')
    result = {'run_id': str(identifier), 'attempt': run['run_attempt'], 'workflow_commit': run['head_sha'],
        'artifact': {key: item.get(key) for key in ('id', 'name', 'digest', 'size_in_bytes', 'expires_at')},
        'archive_sha256': checksum, 'files': records,
        'provenance': json.loads(metadata[f'Holder-{version}-provenance.json'])}
    if signed:
        result['signing_metadata'] = json.loads(metadata[f'Holder-{version}-signing-metadata.json'])
    return result


def bind(record, bundle):
    spec = importlib.util.spec_from_file_location('candidate_bundle_provenance', Path(__file__).with_name('candidate-provenance.py'))
    policy = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(policy)
    policy.bind(record, bundle['provenance'], 'linux')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repository', required=True)
    parser.add_argument('--run-id', required=True)
    parser.add_argument('--artifact-name')
    parser.add_argument('--artifact-id', required=True)
    parser.add_argument('--archive-sha256', required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--signed', action='store_true')
    parser.add_argument('--candidate', help='optional exact desktop/backend source SHA')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.exists(), 'Linux download output already exists')
    artifact_name = args.artifact_name or ('Holder-linux-appimage-signed-' + args.version if args.signed else 'Holder-linux-appimage-staged')
    with tempfile.TemporaryDirectory(prefix='holder-linux-candidate-') as directory:
        github = manifest_tool.GitHub(directory)
        bundle = read(args.repository, args.run_id, artifact_name, args.version, github, signed=args.signed,
                      artifact_id=args.artifact_id, archive_sha256=args.archive_sha256)
        if args.signed:
            metadata = bundle['signing_metadata']
            require(metadata['appimage_version'] == args.version and metadata['signing_workflow'] == {
                'repository': args.repository, 'run_id': args.run_id}, 'Signed bundle belongs to a different signing run/version')
        if args.candidate:
            manifest_tool.provenance.commit(args.candidate)
            require(all(bundle['provenance'][name]['commit'] == args.candidate and
                        bundle['provenance'][name]['version'] == args.version for name in ('desktop', 'backend')),
                    'Linux bundle differs from the expected candidate source/version')
        args.output.mkdir(parents=True)
        with zipfile.ZipFile(github.directory / f"{bundle['artifact']['id']}.zip") as archive:
            for file in bundle['files']:
                with archive.open(file['name']) as source, (args.output / file['name']).open('wb') as output:
                    shutil.copyfileobj(source, output)
        (args.output / f'Holder-{args.version}-x86_64.AppImage').chmod(0o755)


if __name__ == '__main__':
    main()
