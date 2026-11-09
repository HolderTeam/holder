#!/usr/bin/env python3
"""Promote verified Linux files without replacing an existing asset on a retry."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
from urllib.parse import urlencode


def require(condition, message):
    if not condition:
        raise ValueError(message)


def names(version):
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:[-+][0-9A-Za-z.+-]+)?', version), 'Invalid Linux release version')
    return {f'Holder-{version}-x86_64.AppImage', f'Holder-{version}-SHA256SUMS',
            f'Holder-{version}-SHA256SUMS.asc', 'Holder-linux-release-key.asc'}


def fingerprint(path):
    checksum = hashlib.sha256()
    with path.open('rb') as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b''):
            checksum.update(chunk)
    return checksum.hexdigest(), path.stat().st_size


def asset_fingerprint(repository, asset):
    require(str(asset['id']).isdecimal(), 'Invalid release asset ID')
    with tempfile.TemporaryDirectory(prefix='holder-release-asset-') as directory:
        path = Path(directory) / 'asset'
        with path.open('wb') as output:
            subprocess.run(['gh', 'api', '-H', 'Accept: application/octet-stream',
                f'repos/{repository}/releases/assets/{asset["id"]}'], stdout=output, check=True)
        return fingerprint(path)


class GitHub:
    def api(self, route, method='GET', data=None, **query):
        if query:
            route += '?' + urlencode(query)
        command = ['gh', 'api', '--method', method, '-H', 'X-GitHub-Api-Version: 2026-03-10', route]
        if data is not None:
            command += ['--input', '-']
        result = subprocess.run(command, input=json.dumps(data) if data is not None else None,
                                text=True, capture_output=True, check=True)
        return json.loads(result.stdout) if result.stdout.strip() else {}

    asset_fingerprint = staticmethod(asset_fingerprint)

    def upload(self, repository, tag, path):
        subprocess.run(['gh', 'release', 'upload', tag, '--repo', repository, str(path)], check=True)


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


def observe(github, repository, tag, files, candidate, release_id=None):
    tag_commit(github, repository, tag, candidate)
    release = release_for(github, repository, tag)
    if release is None:
        require(release_id is None, 'Accepted release was removed')
        return None
    require(release_id is None or release_id == release['id'], 'Accepted release ID changed')
    missing, conflicts, complete = classify(github, repository, assets_for(github, repository, release), files)
    require(not conflicts, 'Release asset differs from accepted signed bytes: ' + ', '.join(item['name'] for item in conflicts))
    if missing:
        return None
    return {'repository': repository, 'candidate': candidate, 'release': {
        'id': release['id'], 'tag': tag, 'url': release['html_url'], 'draft': release['draft'], 'prerelease': release['prerelease']},
        'files': complete, 'verified_at': datetime.now(timezone.utc).isoformat(timespec='seconds')}


def promote(github, repository, tag, version, directory, *, candidate=None,
            allow_published_prerelease=False, replace_draft_assets=False):
    require(tag in (f'holder-v{version}', f'v{version}', version), 'Release tag differs from the expected version')
    candidate = tag_commit(github, repository, tag, candidate)
    require({path.name for path in directory.iterdir()} == names(version), 'Unexpected Linux release payload')
    files = []
    for path in sorted(directory.iterdir()):
        require(path.is_file() and not path.is_symlink(), 'Linux release payload must contain regular files')
        checksum, size = fingerprint(path)
        files.append({'name': path.name, 'size': size, 'sha256': checksum})
    release = release_for(github, repository, tag)
    if release is None:
        release = github.api(f'repos/{repository}/releases', method='POST', data={
            'tag_name': tag, 'target_commitish': candidate, 'draft': True, 'prerelease': '-' in version,
            'name': 'Holder ' + version, 'body': 'Release notes pending. Edit this draft before publishing.'})

    def writable(replacing=False):
        tag_commit(github, repository, tag, candidate)
        current = github.api(f'repos/{repository}/releases/{release["id"]}')
        require(current['tag_name'] == tag and not current.get('immutable'), 'Release changed or is immutable')
        require(current['draft'] or (allow_published_prerelease and current['prerelease'] and not replacing),
                'Published release cannot be modified by this promotion')
        return current

    missing, conflicts, _ = classify(github, repository, assets_for(github, repository, release), files)
    require(not conflicts or (release['draft'] and replace_draft_assets),
            'Release asset conflicts with signed bytes; replacement requires an explicit draft-only operation')
    if missing or conflicts:
        writable(bool(conflicts))  # Check every conflict before performing any mutation.
    for item in conflicts:
        writable(True)
        github.api(f'repos/{repository}/releases/assets/{item["id"]}', method='DELETE')
        missing.append(item['name'])
    for name in sorted(missing):
        writable()
        github.upload(repository, tag, directory / name)
    result = observe(github, repository, tag, files, candidate, release_id=release['id'])
    require(result is not None, 'Linux release assets are incomplete after promotion; resume the same inputs')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repository', required=True)
    parser.add_argument('--tag', required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--candidate')
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    parser.add_argument('--allow-published-prerelease', action='store_true')
    parser.add_argument('--replace-draft-assets', action='store_true')
    args = parser.parse_args()
    result = promote(GitHub(), args.repository, args.tag, args.version, args.directory, candidate=args.candidate,
        allow_published_prerelease=args.allow_published_prerelease, replace_draft_assets=args.replace_draft_assets)
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    print(result['release']['url'])


if __name__ == '__main__':
    main()
