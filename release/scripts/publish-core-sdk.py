#!/usr/bin/env python3
"""Fetch candidate SDKs and publish their verified bytes to the SDK release repository."""
import argparse
import base64
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
SDK_REPOSITORY = 'HolderTeam/holder-sdk'


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


assets = load('sdk_release_assets', 'release/scripts/github-release.py')
manifest_tool = load('sdk_release_manifest', 'release/staging/scripts/generate-release-manifest.py')
sdk_tool = load('sdk_release_archives', 'core/scripts/core-sdk.py')
require = assets.require


def encode(value):
    return json.dumps(value, indent=2) + '\n'


def validate_selection(selection, candidate, version):
    manifest_tool.provenance.commit(candidate)
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?', version), 'Invalid SDK release version')
    require(selection['source'] == 'artifact' and selection['commit'] == candidate and selection['version'] == version,
            'SDK selection differs from candidate source/version')
    manifest_tool.provenance.commit(selection['core_tree'])
    expected = {('linux', 'x86_64', 'Release'), ('windows', 'x86_64', 'Release'), ('macos', 'arm64', 'Release')}
    require(len(selection['assets']) == 3 and
            {(item['platform'], item['architecture'], item['build_type']) for item in selection['assets']} == expected,
            'SDK publication needs exactly three Release platform configurations')
    require(len({item['name'] for item in selection['assets']}) == 3, 'Duplicate SDK archive names')
    for item in selection['assets']:
        require(re.fullmatch(r'[A-Za-z0-9_.+-]+\.(?:tar\.gz|zip)', item['name']) and
                re.fullmatch(r'[0-9a-f]{64}', item['sha256']) and isinstance(item['size'], int) and item['size'] > 0,
                'Invalid SDK archive name/checksum/size')
        manifest_tool.provenance.commit(item['commit'])


def make_index(selection, repository, run_id):
    validate_selection(selection, selection['commit'], selection['version'])
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository) and str(run_id).isdecimal(), 'Invalid SDK source run')
    tag = 'holder-v' + selection['version']
    return {'schema_version': 1, 'repository': SDK_REPOSITORY, 'release_tag': tag,
        'core_tree': selection['core_tree'], 'commit': selection['commit'], 'version': selection['version'],
        'source': 'https://github.com/' + repository,
        'workflow_run': f'https://github.com/{repository}/actions/runs/{run_id}',
        'assets': [{**item, 'url': f'https://github.com/{SDK_REPOSITORY}/releases/download/{tag}/{item["name"]}'}
                   for item in selection['assets']]}


def prepared(github, repository, path, reference):
    require(re.fullmatch(r'sdk-manifests/[A-Za-z0-9._-]+\.json', path) and
            re.fullmatch(r'[0-9a-f]{40}', reference), 'SDK inputs require a safe path and immutable commit SHA')
    response = github.request(f'repos/{repository}/contents/release/{path}', ref=reference)
    require(response['type'] == 'file' and response['encoding'] == 'base64', 'SDK inputs must be a Git file')
    return json.loads(base64.b64decode(response['content']))


class ReadAPI:
    def __init__(self, github):
        self.github = github

    def api(self, route):
        return self.github.request(route)


def fetch(repository, run_id, version, output, github, expected=None, clock=None):
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository) and str(run_id).isdecimal(), 'Invalid SDK producer repository/run')
    require(not output.exists(), 'SDK preparation output already exists; use a fresh directory')
    run = github.request(f'repos/{repository}/actions/runs/{run_id}')
    candidate = run['head_sha']
    manifest_tool.run(repository, run_id, 'daemon-ci.yml', candidate, github.request)
    source = ReadAPI(github)
    assets.tag_commit(source, repository, 'holder-v' + version, candidate)
    root_version = github.request(f'repos/{repository}/contents/VERSION', ref=candidate)
    require(base64.b64decode(root_version['content']).decode().strip() == version, 'SDK candidate root VERSION differs')
    if expected:
        require(expected['schema'] == 1 and expected['repository'] == repository and expected['candidate'] == candidate and
                expected['version'] == version and expected['backend']['run_id'] == str(run_id) and
                expected['backend']['attempt'] == run['run_attempt'] and expected['backend']['workflow'] == 'daemon-ci.yml' and
                expected['backend']['workflow_commit'] == candidate, 'Prepared SDK source/run attempt differs')
    select = manifest_tool.artifacts(repository, run, github.request, clock or datetime.now(timezone.utc))
    evidence = []

    def read(item, metadata, required=()):
        if expected:
            pins = [value for value in expected['backend']['artifacts'] if value['name'] == item['name']]
            require(len(pins) == 1 and all(item.get(key) == value for key, value in pins[0].items()), 'Pinned SDK artifact changed')
        values, checksum = github.read(repository, item, metadata, required)
        evidence.append({'id': item['id'], 'name': item['name'], 'archive_sha256': checksum,
                         'expires_at': item['expires_at']})
        return values

    item = select('core-selection')
    selection = json.loads(read(item, ['core-selection.json'])['core-selection.json'])
    validate_selection(selection, candidate, version)
    if expected:
        require(selection == expected['core_selection'] and str(item['id']) == str(expected['core_pin']['selection_artifact_id']) and
                evidence[-1]['archive_sha256'] == expected['core_pin']['selection_artifact_sha256'], 'Pinned core selection differs')
    tree = manifest_tool.components.subtree(repository, candidate, ('core',), github.request)
    version_blob = manifest_tool.components.version_blob(repository, candidate, github.request)
    require(selection['core_tree'] == tree, 'SDK selection core tree differs from candidate')
    for commit in {item['commit'] for item in selection['assets']} - {candidate}:
        require(manifest_tool.components.subtree(repository, commit, ('core',), github.request) == tree and
                manifest_tool.components.version_blob(repository, commit, github.request) == version_blob,
                'Cached SDK producer source/version differs from candidate')
    with tempfile.TemporaryDirectory(dir=github.directory, prefix='sdk-verify-') as directory:
        payload = Path(directory)
        for asset in selection['assets']:
            item = select(f'core-sdk-{asset["platform"]}-Release')
            read(item, [], [asset['name']])
            with zipfile.ZipFile(github.directory / f"{item['id']}.zip") as archive:
                with archive.open(asset['name']) as source, (payload / asset['name']).open('wb') as destination:
                    shutil.copyfileobj(source, destination)
            require(assets.fingerprint(payload / asset['name']) == (asset['sha256'], asset['size']), 'SDK archive bytes differ from selection')
        verify_archives(payload, selection)
        current = manifest_tool.run(repository, run_id, 'daemon-ci.yml', candidate, github.request)
        require(current['run_attempt'] == run['run_attempt'], 'SDK producer run was retried during download')
        assets.tag_commit(ReadAPI(github), repository, 'holder-v' + version, candidate)
        output.mkdir(parents=True)
        shutil.copytree(payload, output / 'core-sdk')
    result = {'schema': 1, 'repository': repository, 'candidate': candidate, 'version': version,
        'run_id': str(run_id), 'attempt': run['run_attempt'], 'artifacts': evidence, 'core_selection': selection}
    (output / 'core-selection.json').write_text(encode(selection))
    (output / 'sdk-publication-input.json').write_text(encode(result))
    return result


def verify_archives(directory, selection):
    require({path.name for path in directory.iterdir()} <= {item['name'] for item in selection['assets']} | {'sdk-index.json'},
            'Unexpected SDK publication payload files')
    require(all(path.is_file() and not path.is_symlink() for path in directory.iterdir()), 'SDK publication needs regular files')
    actual = sdk_tool.index(directory, selection['commit'], selection['core_tree'])
    require(all(actual[key] == selection[key] for key in ('commit', 'core_tree', 'version')) and
            sorted(actual['assets'], key=lambda item: item['name']) == sorted(selection['assets'], key=lambda item: item['name']),
            'Downloaded SDK manifests, configurations or bytes differ from selection')


def inspect_release(github, expected, release_id=None):
    release = assets.release_for(github, SDK_REPOSITORY, expected['release_tag'])
    if release is None:
        require(release_id is None, 'Accepted SDK release was removed')
        return None, None
    require(release_id is None or release_id == release['id'], 'Accepted SDK release ID changed')
    actual = assets.assets_for(github, SDK_REPOSITORY, release)
    names = {item['name'] for item in expected['assets']} | {'sdk-index.json'}
    require(all(item['name'] in names for item in actual), 'SDK release contains unexpected assets')
    missing, conflicts, complete = assets.classify(github, SDK_REPOSITORY, actual, [
        {'name': item['name'], 'size': item['size'], 'sha256': item['sha256']} for item in expected['assets']])
    require(not conflicts, 'SDK release archive conflicts with the accepted selection')
    indexes = [item for item in actual if item['name'] == 'sdk-index.json']
    require(len(indexes) <= 1, 'Duplicate SDK release index')
    if indexes:
        item = indexes[0]
        require(item['state'] == 'uploaded', 'SDK index upload is incomplete')
        data = github.asset_contents(SDK_REPOSITORY, item)
        require(json.loads(data) == expected, 'Published SDK index differs from the accepted source/selection')
        complete.append({'name': item['name'], 'asset_id': item['id'], 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    if missing or not indexes:
        require(release['draft'], 'Published SDK release is incomplete; it cannot be repaired by replacing assets')
        return release, None
    return release, sorted(complete, key=lambda item: item['name'])


def observe(github, expected, release_id=None):
    release, complete = inspect_release(github, expected, release_id)
    if release is None or release['draft']:
        return None
    require(complete and release['prerelease'] == ('-' in expected['version']), 'SDK publication state differs from version')
    return {'repository': SDK_REPOSITORY, 'source_repository': expected['source'].removeprefix('https://github.com/'),
        'candidate': expected['commit'], 'version': expected['version'], 'index': expected,
        'release': {'id': release['id'], 'tag': release['tag_name'], 'url': release['html_url'],
                    'draft': release['draft'], 'prerelease': release['prerelease']},
        'sdk_repository_tag_commit': assets.tag_commit(github, SDK_REPOSITORY, expected['release_tag']),
        'files': complete, 'verified_at': datetime.now(timezone.utc).isoformat(timespec='seconds')}


def publish(github, source_github, inputs, directory):
    selection = inputs['core_selection']
    validate_selection(selection, inputs['candidate'], inputs['version'])
    require(inputs['candidate'] == selection['commit'], 'SDK input source differs')
    expected = make_index(selection, inputs['repository'], inputs['run_id'])
    tag = expected['release_tag']

    def source_tag():
        assets.tag_commit(source_github, inputs['repository'], tag, inputs['candidate'])

    source_tag()
    verify_archives(directory, selection)
    (directory / 'sdk-index.json').write_text(encode(expected))
    release, complete = inspect_release(github, expected)
    if release is not None and not release['draft']:
        return {**observe(github, expected), 'input_artifacts': inputs['artifacts'], 'source_run_attempt': inputs['attempt']}
    if release is None:
        source_tag()
        release = github.api(f'repos/{SDK_REPOSITORY}/releases', method='POST', data={'tag_name': tag,
            'target_commitish': 'main', 'draft': True, 'prerelease': '-' in inputs['version'],
            'name': 'Holder ' + inputs['version'] + ' core SDK',
            'body': f'The core SDK shipped with Holder {inputs["version"]}, from {expected["workflow_run"]}. See sdk-index.json for source and archive checksums.'})
    require(not release.get('immutable') and release['prerelease'] == ('-' in inputs['version']), 'SDK draft publication state differs')
    present = {item['name'] for item in complete or []}
    # For an incomplete draft, inspect_release deliberately withholds completion.
    if complete is None:
        present = {item['name'] for item in assets.assets_for(github, SDK_REPOSITORY, release)}
    for path in sorted(directory.iterdir()):
        if path.name in present:
            continue
        source_tag()
        current, _ = inspect_release(github, expected, release['id'])
        require(current['draft'] and not current.get('immutable'), 'SDK release was published while uploads were pending')
        github.upload(SDK_REPOSITORY, tag, path)
    source_tag()
    current, complete = inspect_release(github, expected, release['id'])
    require(complete, 'SDK draft is incomplete; resume the same inputs')
    if current['draft']:
        require(not current.get('immutable'), 'SDK release became immutable')
        github.api(f'repos/{SDK_REPOSITORY}/releases/{release["id"]}', method='PATCH', data={
            'draft': False, 'make_latest': 'false' if '-' in inputs['version'] else 'true'})
    result = observe(github, expected, release['id'])
    require(result, 'SDK release is not yet published; re-observe before retrying')
    return {**result, 'input_artifacts': inputs['artifacts'], 'source_run_attempt': inputs['attempt']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('fetch', 'publish'))
    parser.add_argument('--repository')
    parser.add_argument('--run-id')
    parser.add_argument('--version')
    parser.add_argument('--prepared-path', default='')
    parser.add_argument('--prepared-ref', default='')
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    if args.command == 'fetch':
        require(args.repository and args.run_id and args.version, 'SDK fetch requires source repository/run/version')
        require(bool(args.prepared_path) == bool(args.prepared_ref), 'Provide both prepared SDK inputs')
        with tempfile.TemporaryDirectory(prefix='holder-sdk-publication-') as directory:
            github = manifest_tool.GitHub(directory)
            expected = prepared(github, args.repository, args.prepared_path, args.prepared_ref) if args.prepared_ref else None
            fetch(args.repository, args.run_id, args.version, args.directory, github, expected)
    else:
        require(os.environ.get('SOURCE_GH_TOKEN'), 'SDK publication requires the separate source read token')
        inputs = json.loads((args.directory / 'sdk-publication-input.json').read_text())
        result = publish(assets.GitHub(), assets.GitHub(os.environ['SOURCE_GH_TOKEN']), inputs, args.directory / 'core-sdk')
        (args.directory / 'sdk-publication.json').write_text(encode(result))
        print(result['release']['url'])


if __name__ == '__main__':
    main()
