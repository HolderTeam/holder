#!/usr/bin/env python3
"""Fetch prepared Python bytes and verify resumable PyPI publication."""
import argparse
import base64
from datetime import datetime, timezone
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from urllib.error import HTTPError
from urllib.request import Request, urlopen

SPEC = importlib.util.spec_from_file_location('prepared_python', Path(__file__).with_name('python-distributions.py'))
packages = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(packages)
require = packages.require
INDEXES = {'pypi': 'https://pypi.org', 'testpypi': 'https://test.pypi.org'}


def index(package, version, destination):
    require(destination in INDEXES and package in packages.ARTIFACTS, 'Invalid package publication target')
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', version), 'Prepared publication needs a stable product version')
    name = 'holder-kit' if package == 'kit' else 'holder-python'
    return f'{INDEXES[destination]}/pypi/{name}/{version}/json'


def request_index(url):
    try:
        with urlopen(Request(url, headers={'Accept': 'application/json', 'User-Agent': 'holder-release'}), timeout=20) as response:
            return json.load(response)
    except HTTPError as error:
        if error.code == 404:
            return None
        raise


def published(files, response):
    """Accept matching existing files, reject conflicts before uploading anything."""
    expected = {file['name']: file for file in files}
    entries = [] if response is None else response['urls']
    found = {}
    for item in entries:
        name = item['filename']
        require(name in expected and name not in found, 'Unexpected or duplicate published file: ' + name)
        require(not item.get('yanked'), 'Published file is yanked: ' + name)
        require(item['digests']['sha256'] == expected[name]['sha256'] and item['size'] == expected[name]['size'],
                'Published file differs from prepared bytes: ' + name)
        found[name] = expected[name]
    return found


def fetch(path, reference, repository, candidate, ref, version, package, destination, output, github):
    require(ref == f'refs/tags/holder-v{version}', 'Prepared publication requires the matching Holder release tag')
    require(re.fullmatch(r'[0-9a-f]{40}', reference), 'Prepared release ref must be an immutable commit SHA')
    require(re.fullmatch(r'python-manifests/[A-Za-z0-9._-]+\.json', path), 'Invalid prepared release path')
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository), 'Invalid repository')
    index(package, version, destination)
    response = github.request(f'repos/{repository}/contents/release/{path}', ref=reference)
    require(response['type'] == 'file' and response['encoding'] == 'base64', 'Prepared release must be a Git file')
    record = json.loads(base64.b64decode(response['content']))
    require(record['schema'] == 1 and record['repository'] == repository and record['candidate'] == candidate and
            record['version'] == version, 'Prepared release differs from tagged source/version')
    evidence = record['python_packages'][package]
    actual = packages.collect(record, github, [package])[package]
    require(actual == evidence, 'Downloaded package evidence differs from the prepared release')
    require(not output.exists(), 'Publication output already exists; use a fresh directory')
    shutil.copytree(github.directory / package, output)
    return {'repository': repository, 'candidate': candidate, 'version': version,
            'package': package, 'destination': destination, 'prepared_ref': reference,
            'source_run_id': evidence['run_id'], 'source_artifact_id': evidence['distribution_artifact_id'],
            'files': evidence['files'], 'index_url': index(package, version, destination)}


def plan(directory, receipt, response, pending):
    expected = {file['name']: file for file in receipt['files']}
    require({path.name for path in directory.iterdir()} == set(expected), 'Publication directory differs from prepared files')
    for name, file in expected.items():
        path = directory / name
        require(path.stat().st_size == file['size'] and packages.manifest_tool.file_sha256(path) == file['sha256'],
                'Publication bytes changed: ' + name)
    complete = published(receipt['files'], response)
    require(not pending.exists(), 'Upload directory already exists; use a fresh directory')
    pending.mkdir(parents=True)
    for name in expected.keys() - complete.keys():
        shutil.copyfile(directory / name, pending / name)
    return len(expected) - len(complete)


def verify(receipt, request=request_index, attempts=12, sleep=time.sleep):
    for attempt in range(attempts):
        found = published(receipt['files'], request(receipt['index_url']))
        if len(found) == len(receipt['files']):
            return {**receipt, 'verified_at': datetime.now(timezone.utc).isoformat(timespec='seconds')}
        if attempt + 1 < attempts:
            sleep(5)
    raise ValueError('Published index is incomplete; resume the same prepared release after inspecting the run')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('fetch', 'plan', 'verify'))
    parser.add_argument('--package', choices=tuple(packages.ARTIFACTS))
    parser.add_argument('--destination', choices=tuple(INDEXES))
    parser.add_argument('--prepared-path')
    parser.add_argument('--prepared-ref')
    parser.add_argument('--directory', type=Path, default=Path('dist'))
    parser.add_argument('--pending', type=Path, default=Path('pending'))
    parser.add_argument('--receipt', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.command == 'fetch':
        require(all((args.package, args.destination, args.prepared_path, args.prepared_ref)), 'Fetch requires package/destination/prepared inputs')
        candidate = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
        require(candidate == os.environ['GITHUB_SHA'], 'Publisher checkout differs from tagged source')
        version = Path('VERSION').read_text().strip()
        with tempfile.TemporaryDirectory(prefix='holder-python-publish-') as directory:
            receipt = fetch(args.prepared_path, args.prepared_ref, os.environ['GITHUB_REPOSITORY'], candidate,
                os.environ['GITHUB_REF'], version, args.package, args.destination, args.directory,
                packages.manifest_tool.GitHub(directory))
        args.receipt.parent.mkdir(parents=True, exist_ok=True)
        args.receipt.write_text(json.dumps(receipt, indent=2) + '\n')
    else:
        receipt = json.loads(args.receipt.read_text())
        require(receipt['index_url'] == index(receipt['package'], receipt['version'], receipt['destination']), 'Unexpected package index URL')
        if args.command == 'plan':
            remaining = plan(args.directory, receipt, request_index(receipt['index_url']), args.pending)
            with open(os.environ['GITHUB_OUTPUT'], 'a') as output:
                output.write(f'remaining={remaining}\n')
        else:
            require(args.output, 'Verification requires --output')
            result = verify(receipt)
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
