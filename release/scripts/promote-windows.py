#!/usr/bin/env python3
"""Deliver Windows development assets, preserving accepted bytes on retries."""
import argparse
import importlib.util
import json
from pathlib import Path
import re

SPEC = importlib.util.spec_from_file_location('windows_release_assets', Path(__file__).with_name('promote-linux.py'))
common = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(common)
require = common.require
observe = common.observe


def names(version):
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:[-+][0-9A-Za-z.+-]+)?', version), 'Invalid Windows version')
    installer = f'Holder-windows-{version}-dev-Setup.exe'
    return {installer, installer + '.sha256', 'Holder-windows-dev-test-signing.cer',
            'Holder-windows-dev-test-signing-fingerprint.txt'}


def promote(github, repository, tag, version, directory, *, candidate=None, replace_draft_assets=False, dev_run=None):
    product = tag in (f'holder-v{version}', f'v{version}', version)
    require(product or (dev_run and str(dev_run).isdecimal() and tag == f'holder-windows-dev-{dev_run}'),
            'Use the matching product tag or the staging run development tag')
    require(candidate is None or product, 'Coordinated Windows delivery requires the product tag')
    require(not candidate or not replace_draft_assets, 'Coordinated delivery cannot replace assets')
    source = common.tag_commit(github, repository, tag, candidate) if product else None
    require({path.name for path in directory.iterdir()} == names(version), 'Unexpected Windows development payload')
    files = []
    for path in sorted(directory.iterdir()):
        require(path.is_file() and not path.is_symlink(), 'Windows payload must contain regular files')
        checksum, size = common.fingerprint(path)
        files.append({'name': path.name, 'size': size, 'sha256': checksum})
    installer = f'Holder-windows-{version}-dev-Setup.exe'
    digest = next(file['sha256'] for file in files if file['name'] == installer)
    require((directory / (installer + '.sha256')).read_bytes() == f'{digest}  {installer}\n'.encode(),
            'Windows installer checksum differs')
    release = common.release_for(github, repository, tag)
    if release is None:
        require(candidate is None, 'Create and review the shared product draft before Windows delivery')
        data = {'tag_name': tag, 'draft': True, 'prerelease': True,
                'name': f'Holder {version} Windows dev build',
                'body': 'Windows development installer, self-signed for testing. Windows may show SmartScreen or unknown-publisher warnings. Check the attached certificate fingerprint before trusting it.'}
        if source:
            data['target_commitish'] = source
        release = github.api(f'repos/{repository}/releases', method='POST', data=data)

    def writable(replacing=False):
        if product:
            common.tag_commit(github, repository, tag, source)
        current = github.api(f'repos/{repository}/releases/{release["id"]}')
        require(current['tag_name'] == tag and not current.get('immutable'), 'Release changed or is immutable')
        require(current['draft'] or (not product and current['prerelease'] and not replacing),
                'Published release cannot be modified by this promotion')
        return current

    common.assets.upload_missing(github, repository, tag, release, directory, files, writable, replace_draft_assets)
    missing, conflicts, complete = common.classify(github, repository, common.assets_for(github, repository, release), files)
    require(not missing and not conflicts, 'Windows delivery is incomplete; resume the same inputs')
    if not product and release['draft']:
        writable()
        github.api(f'repos/{repository}/releases/{release["id"]}', method='PATCH', data={'draft': False})
    if product:
        return observe(github, repository, tag, files, source, release_id=release['id'])
    return {'repository': repository, 'release_id': release['id'], 'tag': tag, 'files': complete}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repository', required=True)
    parser.add_argument('--tag')
    parser.add_argument('--run-id', required=True)
    parser.add_argument('--version')
    parser.add_argument('--candidate')
    parser.add_argument('--replace-draft-assets', action='store_true')
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    installers = list(args.directory.glob('Holder-windows-*-dev-Setup.exe'))
    require(len(installers) == 1, 'Expected exactly one Windows installer')
    version = installers[0].name.removeprefix('Holder-windows-').removesuffix('-dev-Setup.exe')
    require(args.version is None or args.version == version, 'Windows release version differs')
    result = promote(common.GitHub(), args.repository, args.tag or f'holder-windows-dev-{args.run_id}', version,
                     args.directory, candidate=args.candidate, replace_draft_assets=args.replace_draft_assets, dev_run=args.run_id)
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
