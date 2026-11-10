#!/usr/bin/env python3
"""Promote verified Linux files without replacing an existing asset on a retry."""
import argparse
from datetime import datetime, timezone
import importlib.util
import json
from pathlib import Path
import re


SPEC = importlib.util.spec_from_file_location('github_release', Path(__file__).with_name('github-release.py'))
assets = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(assets)
require = assets.require
GitHub = assets.GitHub
fingerprint = assets.fingerprint
asset_fingerprint = assets.asset_fingerprint
pages = assets.pages
tag_commit = assets.tag_commit
release_for = assets.release_for
assets_for = assets.assets_for
classify = assets.classify


def names(version):
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:[-+][0-9A-Za-z.+-]+)?', version), 'Invalid Linux release version')
    return {f'Holder-{version}-x86_64.AppImage', f'Holder-{version}-SHA256SUMS',
            f'Holder-{version}-SHA256SUMS.asc', 'Holder-linux-release-key.asc'}


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

    assets.upload_missing(github, repository, tag, release, directory, files, writable, replace_draft_assets)
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
