"""Verify the public website against the selected stable Holder release."""
import hashlib
from html.parser import HTMLParser
import importlib.util
import json
from pathlib import Path
import re
from urllib.error import HTTPError
from urllib.parse import urljoin
from urllib.request import Request, urlopen

SPEC = importlib.util.spec_from_file_location('website_generator', Path(__file__).with_name('website-release.py'))
generator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(generator)

SITE = 'https://www.holder.team/'
require = generator.require


def fetch(path):
    request = Request(urljoin(SITE, path), headers={'Cache-Control': 'no-cache'})
    try:
        with urlopen(request, timeout=20) as response:
            data = response.read(1024 * 1024 + 1)
    except HTTPError as error:
        error.close()
        if error.code == 404:
            return None
        raise
    require(len(data) <= 1024 * 1024, 'Oversized website response for ' + path)
    return data


class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.downloads = []

    def handle_starttag(self, tag, attrs):
        if tag == 'a':
            attributes = dict(attrs)
            if 'data-download-asset' in attributes:
                self.downloads.append((attributes['data-download-asset'], attributes.get('href')))


def observe(releases, version, assets, contents, request=fetch, generate=generator.generate):
    tag = 'holder-v' + version
    matching = [release for release in releases if release['tag_name'] == tag]
    require(len(matching) <= 1, 'Ambiguous candidate release')
    if not matching or matching[0]['draft'] or matching[0]['prerelease'] or not matching[0].get('published_at'):
        return {'pending': 'Waiting for the stable candidate release to be published'}
    release = generator.latest_stable(releases)
    require(release['tag_name'] == tag, 'A newer stable release is selected for the website')
    release = {**release, 'assets': assets(release)}
    files = generate(release, {'status': 'unconfirmed'}, fetch=contents)
    feed = request('version.json')
    if feed is None:
        return {'pending': 'Waiting for the public update feed'}
    actual = json.loads(feed)
    require(isinstance(actual, dict), 'Public update feed must be a JSON object')
    actual_version = actual.get('version')
    require(isinstance(actual_version, str) and re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', actual_version),
            'Public update feed has an invalid version')
    if tuple(map(int, actual_version.split('.'))) < tuple(map(int, version.split('.'))):
        return {'pending': f'Public update feed still serves {actual_version}'}
    require(actual == json.loads(files['version.json']), 'Public update feed differs from the candidate release')
    checksums = {}
    for path, expected in files.items():
        if not path.startswith('assets/checksums/'):
            continue
        data = request(path)
        if data is None:
            return {'pending': 'Waiting for public checksum ' + path}
        require(data == expected.encode(), 'Public checksum differs from the candidate release: ' + path)
        checksums[path] = hashlib.sha256(data).hexdigest()
    platforms = json.loads(files['_data/downloads.json'])['platforms']
    downloads = {}
    for platform in ('linux', 'macos', 'windows'):
        path = platform + '/'
        page = request(path)
        if page is None:
            return {'pending': 'Waiting for public download page ' + path}
        links = Links()
        links.feed(page.decode('utf-8'))
        item = platforms[platform]
        if [href for name, href in links.downloads if name == platform] != [item['url']]:
            return {'pending': f'Waiting for the {platform} download page to link to {tag}'}
        downloads[platform] = {'url': item['url'], 'channel': item['channel']}
    selected = {item['url'] for item in downloads.values()}
    selected.update(platforms['linux'][key] for key in ('signature_url', 'public_key_url'))
    names = {path.rsplit('/', 1)[-1] for path in checksums}
    return {'receipt': {'site': SITE, 'release_id': release['id'], 'tag': tag, 'feed': actual,
        'checksums': checksums, 'downloads': downloads,
        'assets': sorted(({key: item[key] for key in ('id', 'name', 'size', 'digest')}
            for item in release['assets'] if item['browser_download_url'] in selected or item['name'] in names),
            key=lambda item: item['name'])}}
