#!/usr/bin/env python3
"""Shared GitHub release reads, asset fingerprints and upload primitives."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
from urllib.parse import urlencode


def require(condition, message):
    if not condition:
        raise ValueError(message)


def fingerprint(path):
    checksum = hashlib.sha256()
    with path.open('rb') as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b''):
            checksum.update(chunk)
    return checksum.hexdigest(), path.stat().st_size


def asset_fingerprint(repository, asset, env=None):
    require(str(asset['id']).isdecimal(), 'Invalid release asset ID')
    with tempfile.TemporaryDirectory(prefix='holder-release-asset-') as directory:
        path = Path(directory) / 'asset'
        with path.open('wb') as output:
            subprocess.run(['gh', 'api', '-H', 'Accept: application/octet-stream',
                f'repos/{repository}/releases/assets/{asset["id"]}'], stdout=output, check=True, env=env)
        return fingerprint(path)


def asset_contents(repository, asset, env=None):
    require(str(asset['id']).isdecimal() and 0 < asset['size'] <= 1024 * 1024, 'Invalid or oversized release metadata asset')
    data = subprocess.check_output(['gh', 'api', '-H', 'Accept: application/octet-stream',
        f'repos/{repository}/releases/assets/{asset["id"]}'], env=env)
    require(len(data) == asset['size'], 'Downloaded release metadata size differs')
    if asset.get('digest'):
        require(asset['digest'] == 'sha256:' + hashlib.sha256(data).hexdigest(), 'Downloaded release metadata digest differs')
    return data


class GitHub:
    def __init__(self, token=None):
        self.token = token

    def environment(self):
        if self.token is None:
            return None  # Preserve the caller's GitHub CLI authentication.
        return {**os.environ, 'GH_TOKEN': self.token}

    def api(self, route, method='GET', data=None, **query):
        if query:
            route += '?' + urlencode(query)
        command = ['gh', 'api', '--method', method, '-H', 'X-GitHub-Api-Version: 2026-03-10', route]
        if data is not None:
            command += ['--input', '-']
        result = subprocess.run(command, input=json.dumps(data) if data is not None else None,
                                text=True, capture_output=True, check=True, env=self.environment())
        return json.loads(result.stdout) if result.stdout.strip() else {}

    def asset_fingerprint(self, repository, asset):
        return asset_fingerprint(repository, asset, self.environment())

    def asset_contents(self, repository, asset):
        return asset_contents(repository, asset, self.environment())

    def upload(self, repository, tag, path):
        subprocess.run(['gh', 'release', 'upload', tag, '--repo', repository, str(path)], check=True, env=self.environment())


def pages(github, route):
    result, page = [], 1
    while True:
        batch = github.api(route, per_page=100, page=page)
        result.extend(batch)
        if len(batch) < 100:
            return result
        page += 1


def tag_commit(github, repository, tag, candidate=None):
    require(re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository), 'Invalid repository')
    require(re.fullmatch(r'(?:holder-v|v)?[0-9]+\.[0-9]+\.[0-9]+(?:[-+][0-9A-Za-z.+-]+)?', tag), 'Invalid release tag')
    target = github.api(f'repos/{repository}/git/ref/tags/{tag}')['object']
    for _ in range(5):
        if target['type'] != 'tag':
            break
        target = github.api(f'repos/{repository}/git/tags/{target["sha"]}')['object']
    require(target['type'] == 'commit' and re.fullmatch(r'[0-9a-f]{40}', target['sha']), 'Release tag does not resolve to a commit')
    require(candidate is None or candidate == target['sha'], 'Release tag differs from accepted candidate source')
    return target['sha']


def release_for(github, repository, tag):
    matches = [item for item in pages(github, f'repos/{repository}/releases') if item['tag_name'] == tag]
    require(len(matches) <= 1, 'Multiple releases match the exact tag')
    return matches[0] if matches else None


def assets_for(github, repository, release):
    return pages(github, f'repos/{repository}/releases/{release["id"]}/assets')


def classify(github, repository, assets, files):
    missing, conflicts, complete = [], [], []
    for file in files:
        found = [item for item in assets if item['name'] == file['name']]
        require(len(found) <= 1, 'Duplicate release asset: ' + file['name'])
        if not found:
            missing.append(file['name'])
            continue
        item = found[0]
        digest = item.get('digest')
        require(not digest or re.fullmatch(r'sha256:[0-9a-f]{64}', digest), 'Invalid release asset digest')
        matches = item['state'] == 'uploaded' and item['size'] == file['size']
        if matches:
            checksum, size = (digest.removeprefix('sha256:'), item['size']) if digest else github.asset_fingerprint(repository, item)
            matches = checksum == file['sha256'] and size == file['size']
        if matches:
            complete.append({**file, 'asset_id': item['id']})
        else:
            conflicts.append(item)
    return missing, conflicts, complete


