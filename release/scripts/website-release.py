#!/usr/bin/env python3
"""Generate website downloads and the compatible desktop feed from a published release."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
REPOSITORY = 'HolderTeam/holder'
TAG = re.compile(r'holder-v([0-9]+\.[0-9]+\.[0-9]+)')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def api(path):
    return json.loads(subprocess.check_output(['gh', 'api', '--method', 'GET', path], text=True))


def pages(path, request=api):
    result = []
    page = 1
    while True:
        batch = request(f'{path}?per_page=100&page={page}')
        result.extend(batch)
        if len(batch) < 100:
            return result
        page += 1


def latest_stable(releases):
    candidates = []
    for release in releases:
        match = TAG.fullmatch(release['tag_name'])
        if match and not release['draft'] and not release['prerelease'] and release.get('published_at'):
            candidates.append((tuple(map(int, match[1].split('.'))), release))
    require(candidates, 'No published stable holder-v<major.minor.patch> release exists')
    highest = max(version for version, release in candidates)
    matches = [release for version, release in candidates if version == highest]
    require(len(matches) == 1, 'Latest stable release version is ambiguous')
    return matches[0]


def download(asset):
    return subprocess.check_output(['gh', 'api', '-H', 'Accept: application/octet-stream',
                                    f'repos/{REPOSITORY}/releases/assets/{asset["id"]}'])


def verify_signature(checksums, signature, public_key):
    with tempfile.TemporaryDirectory(prefix='holder-website-verify-') as directory:
        root = Path(directory)
        (root / 'SHA256SUMS').write_bytes(checksums)
        (root / 'SHA256SUMS.asc').write_bytes(signature)
        (root / 'release.asc').write_bytes(public_key)
        # Use only the committed public key in an isolated home; no personal keyring.
        subprocess.run(['gpg', '--no-options', '--homedir', directory, '--batch', '--yes',
                        '--dearmor', '--output', str(root / 'release.gpg'), str(root / 'release.asc')],
                       check=True, capture_output=True)
        subprocess.run(['gpgv', '--homedir', directory, '--keyring', str(root / 'release.gpg'),
                        str(root / 'SHA256SUMS.asc'), str(root / 'SHA256SUMS')],
                       check=True, capture_output=True)


def generate(release, ubuntu, fetch=download, verify=verify_signature,
             public_key=None):
    require(latest_stable([release]) == release, 'Expected a published stable Holder release')
    version = TAG.fullmatch(release['tag_name'])[1]
    tag = release['tag_name']
    release_url = f'https://github.com/{REPOSITORY}/releases/tag/{tag}'
    require(release['html_url'] == release_url, 'Release URL does not belong to Holder')

    def asset(*names):
        found = [item for item in release['assets'] if item['name'] in names]
        require(len(found) == 1, f'Release {tag} needs exactly one of {names}; found {len(found)}')
        result = found[0]
        require(result['state'] == 'uploaded' and result['size'] > 0,
                f"Release asset {result['name']} is not completely uploaded")
        require(re.fullmatch(r'sha256:[0-9a-f]{64}', result.get('digest') or ''),
                f"Release asset {result['name']} has no SHA256 digest")
        require(result['browser_download_url'] ==
                f"https://github.com/{REPOSITORY}/releases/download/{tag}/{result['name']}",
                f"Release asset {result['name']} has an unexpected URL")
        return result

    def contents(item):
        data = fetch(item)
        require('sha256:' + hashlib.sha256(data).hexdigest() == item['digest'],
                f"Downloaded {item['name']} differs from its GitHub digest")
        return data

    def checksum(item, binary):
        data = contents(item)
        lines = data.decode('utf-8-sig').strip().splitlines()
        require(len(lines) == 1, f"{item['name']} must contain one checksum")
        match = re.fullmatch(r'([0-9a-f]{64}) [ *](.+)', lines[0])
        require(match and match[2] == binary['name'], f"{item['name']} names the wrong asset")
        require(binary['digest'] == 'sha256:' + match[1], f"{binary['name']} checksum differs from GitHub")
        return data

    linux = asset(f'Holder-{version}-x86_64.AppImage')
    macos = asset(f'Holder-macos-{version}.dmg')
    windows = asset(f'Holder-windows-{version}-dev-Setup.exe', f'Holder-windows-{version}-Setup.exe')
    linux_sums = asset(f'Holder-{version}-SHA256SUMS')
    linux_signature = asset(linux_sums['name'] + '.asc')
    key = asset('Holder-linux-release-key.asc')
    macos_sums = asset(macos['name'] + '.sha256')
    windows_sums = asset(windows['name'] + '.sha256')
    linux_data = checksum(linux_sums, linux)
    macos_data = checksum(macos_sums, macos)
    windows_data = checksum(windows_sums, windows)
    signature = contents(linux_signature)
    key_data = contents(key)
    public_key = public_key if public_key is not None else (ROOT / 'release/keys/holder-linux-release.asc').read_bytes()
    require(key_data == public_key, 'Published Linux key differs from the committed release public key')
    try:
        verify(linux_data, signature, public_key)
    except subprocess.CalledProcessError as error:
        raise ValueError('Linux checksum signature does not verify with the committed release public key') from error

    development = windows['name'].endswith('-dev-Setup.exe')
    platforms = {}
    for platform, label, item, sums, detail in (
        ('linux', 'Linux', linux, linux_sums, f'Signed {version} AppImage for x86_64 Linux.'),
        ('windows', 'Windows', windows, windows_sums,
         f'{"Development installer" if development else "Installer"} {version} for Windows 10 and 11.'),
        ('macos', 'macOS', macos, macos_sums, f'Holder {version} disk image for macOS.'),
    ):
        channel = 'development' if platform == 'windows' and development else 'stable'
        platforms[platform] = {'label': label, 'version': version + ('-dev' if channel == 'development' else ''),
                               'channel': channel, 'badge': f'{version} {"development" if channel == "development" else "release"}',
                               'asset_name': item['name'], 'url': item['browser_download_url'],
                               'checksum_local': True, 'checksum_url': '/assets/checksums/' + sums['name'],
                               'button_label': 'Download Linux AppImage' if platform == 'linux' else 'Download for ' + label,
                               'detail': detail}
    platforms['linux'].update(architecture='x86_64',
                              signature_url=linux_signature['browser_download_url'],
                              public_key_url=key['browser_download_url'])
    require(ubuntu['status'] in ('available', 'unconfirmed'), 'Unsupported declared PPA status')
    ppa_version = ubuntu.get('version')
    if ubuntu['status'] == 'available':
        require(isinstance(ppa_version, str) and re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', ppa_version),
                'Available PPA status needs an explicitly declared version')
    platforms['ubuntu'] = {'label': 'Ubuntu', 'version': ppa_version, 'status': ubuntu['status'],
                            'channel': 'stable' if ubuntu['status'] == 'available' else 'unconfirmed',
                            'badge': f'{ppa_version} PPA' if ubuntu['status'] == 'available' else 'PPA status unconfirmed',
                            'local': True, 'url': '/ubuntu/',
                            'button_label': 'Install on Ubuntu' if ubuntu['status'] == 'available' else 'Ubuntu installation notes',
                            'detail': f'Holder {ppa_version} is available from the HolderTeam PPA.'
                            if ubuntu['status'] == 'available' else 'See Ubuntu notes for current PPA availability.'}
    catalog = {'schema': 1, 'version': version, 'release_page': release_url, 'platforms': platforms}
    feed = {'schema': 1, 'product': 'Holder', 'channel': 'stable', 'version': version,
            'release_url': release_url, 'notes_url': release_url,
            'message': 'A new Holder release is available.',
            'downloads': {'linux': 'https://holder.team/linux/',
                          'windows': windows['browser_download_url'], 'macos': macos['browser_download_url']}}
    files = {'_data/downloads.json': json.dumps(catalog, indent=2) + '\n',
             'version.json': json.dumps(feed, indent=2) + '\n'}
    for item, data in ((linux_sums, linux_data), (macos_sums, macos_data), (windows_sums, windows_data)):
        files['assets/checksums/' + item['name']] = data.decode('utf-8-sig')
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--site', type=Path, default=ROOT / 'release/website')
    args = parser.parse_args()
    try:
        release = latest_stable(pages(f'repos/{REPOSITORY}/releases'))
        # List assets separately so releases with more than one API page remain complete.
        release['assets'] = pages(f'repos/{REPOSITORY}/releases/{release["id"]}/assets')
        ubuntu = json.loads((args.site / '_data/ubuntu.json').read_text())
        files = generate(release, ubuntu)
    except (ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'Cannot generate website release data: {error}\n')
    for name, contents in files.items():
        destination = args.site / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(contents)
    print(f'Generated website data for {release["tag_name"]}; PPA status remains independently declared')


if __name__ == '__main__':
    main()
