#!/usr/bin/env python3
"""Fetch a Release SDK using the immutable candidate's core selection."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


manifest_tool = load('candidate_manifest', 'release/staging/scripts/generate-release-manifest.py')
manifest_fetch = load('candidate_fetch', 'release/staging/scripts/fetch-release-manifest.py')
sdk_tool = load('candidate_sdk', 'core/scripts/core-sdk.py')
require = manifest_tool.require


def fetch(root, path, reference, repository, candidate, platform, architecture, github):
    require(reference, 'Pinned SDK reuse requires an immutable manifest ref')
    manifest_fetch.fetch(root, path, reference, repository, request=github.request)
    pin = manifest_tool.provenance.load_pin(root / path, platform)
    require(pin['core']['build_type'] == 'Release' and pin['core']['commit'] == candidate and
            all(pin[name]['commit'] == candidate and pin[name]['repository'] == repository
                for name in ('desktop', 'backend')), 'SDK manifest differs from candidate source/repository')
    run = manifest_tool.run(repository, pin['backend']['run_id'], 'daemon-ci.yml', candidate, github.request)
    select = manifest_tool.artifacts(repository, run, github.request, manifest_tool.datetime.now(manifest_tool.timezone.utc))
    item = select('core-selection')
    require(str(item['id']) == str(pin['core']['selection_artifact_id']), 'Core selection artifact ID changed')
    values, checksum = github.read(repository, item, ['core-selection.json'])
    require(checksum == pin['core']['selection_artifact_sha256'], 'Core selection archive checksum changed')
    selection = json.loads(values['core-selection.json'])
    require(selection['source'] == 'artifact' and selection['commit'] == candidate and
            selection['version'] == pin['appimage_version'] and
            selection['version'] == (ROOT / 'VERSION').read_text().strip(), 'Core selection source/version changed')
    asset = sdk_tool.selected_asset(selection, platform, architecture, 'Release')
    require('\\' not in asset['name'], 'Invalid SDK asset name')
    # Read only the selected archive from the wrapper ZIP. core-sdk.py subsequently
    # verifies its checksum, size, manifest and installed interface before using it.
    sdk_item = select(f'core-sdk-{platform}-Release')
    github.read(repository, sdk_item, [], [asset['name']])
    output = root / 'core-sdk-assets'
    output.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(github.directory / f"{sdk_item['id']}.zip") as archive:
        data = archive.read(asset['name'])
    require(len(data) == asset['size'] and hashlib.sha256(data).hexdigest() == asset['sha256'],
            'SDK archive differs from the pinned core selection')
    (output / asset['name']).write_bytes(data)
    (root / 'core-selection.json').write_text(json.dumps(selection, indent=2) + '\n')
    current = manifest_tool.run(repository, pin['backend']['run_id'], 'daemon-ci.yml', candidate, github.request)
    require(current['run_attempt'] == run['run_attempt'], 'Backend run was retried while fetching the SDK')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', required=True)
    parser.add_argument('--ref', required=True)
    parser.add_argument('--repository', required=True)
    parser.add_argument('--candidate', required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'out')
    args = parser.parse_args()
    require(subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip() == args.candidate,
            'SDK consumer checkout differs from candidate')
    with tempfile.TemporaryDirectory(prefix='holder-candidate-sdk-') as directory:
        fetch(args.output, args.manifest, args.ref, args.repository, args.candidate,
              *sdk_tool.host(), manifest_tool.GitHub(directory))


if __name__ == '__main__':
    main()
