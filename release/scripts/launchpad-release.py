#!/usr/bin/env python3
"""Observe exact Holder PPA versions, remote builds and public package indexes."""
import argparse
from datetime import datetime, timezone
import gzip
import hashlib
import json
from pathlib import PurePosixPath
import re
import urllib.error
import urllib.parse
import urllib.request

API = 'https://api.launchpad.net/1.0'
ARCHIVE = API + '/~holderteam/+archive/ubuntu/holder'
CONTENT = 'https://ppa.launchpadcontent.net/holderteam/holder/ubuntu/'
SERIES = ('noble', 'resolute')
ARCHITECTURES = ('amd64', 'arm64', 'armhf', 'ppc64el', 'riscv64', 's390x')
BINARIES = {'core': ('libholder0', 'libholder-dev'), 'daemon': ('holder-daemon',),
            'kit': ('python3-holder-kit',), 'desktop': ('holder', 'holder-desktop')}
WAITING = {'Needs building', 'Dependency wait', 'Currently building', 'Uploading build', 'Gathering build output'}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def package_version(version, revision, series):
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', version), 'Invalid PPA upstream version')
    require(type(revision) is int and revision > 0 and series in SERIES, 'Invalid PPA revision/series')
    return f'{version}-{revision}~{series}1'


def revisions(values):
    result = {}
    for value in values:
        match = re.fullmatch(r'(core|daemon|kit|desktop):(noble|resolute)=([1-9][0-9]*)', value)
        require(match is not None, 'Use --ppa-revision <core|daemon|kit|desktop>:<noble|resolute>=<positive revision>')
        key = ':'.join(match.group(1, 2))
        require(key not in result, 'Duplicate PPA revision selection: ' + key)
        result[key] = int(match[3])
    return result


def api_url(url):
    require(url.startswith(API + '/') and urllib.parse.urlsplit(url).netloc == 'api.launchpad.net',
            'Unexpected Launchpad API URL')
    return url


def query(url, **parameters):
    return url + '?' + urllib.parse.urlencode(parameters)


def fetch(url):
    with urllib.request.urlopen(url, timeout=20) as response:
        return response.read()


def packages(data):
    result = []
    for paragraph in data.decode('utf-8').strip().split('\n\n'):
        fields = {}
        for line in paragraph.splitlines():
            if line.startswith((' ', '\t')):
                continue
            key, separator, value = line.partition(':')
            require(separator and key not in fields, 'Malformed or duplicate APT package field')
            fields[key] = value.strip()
        if fields:
            result.append(fields)
    return result


class Client:
    def __init__(self, request=fetch):
        self.request = request
        self.indexes = {}

    def collection(self, url):
        result, visited = [], set()
        while url:
            api_url(url)
            require(url not in visited, 'Launchpad collection pagination repeated a page')
            visited.add(url)
            document = json.loads(self.request(url))
            result.extend(document['entries'])
            url = document.get('next_collection_link')
        return result

    def index(self, series, architecture):
        key = (series, architecture)
        if key not in self.indexes:
            url = CONTENT + f'dists/{series}/main/binary-{architecture}/Packages.gz'
            try:
                data = self.request(url)
            except urllib.error.HTTPError as error:
                error.close()
                if error.code != 404:
                    raise
                self.indexes[key] = (url, None, [])
            else:
                self.indexes[key] = (url, hashlib.sha256(data).hexdigest(), packages(gzip.decompress(data)))
        return self.indexes[key]

    def observe(self, package, series, version, architectures):
        require(package in BINARIES and series in SERIES and architectures and
                set(architectures) <= set(ARCHITECTURES), 'Unsupported PPA target')
        source_name = 'holder-' + package
        require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+-[1-9][0-9]*~' + series + '1', version), 'Invalid target package version')
        series_link = API + '/ubuntu/' + series
        expected = {'source_package_name': source_name, 'source_package_version': version,
                    'archive_link': ARCHIVE, 'pocket': 'Release', 'distro_series_link': series_link}
        result = {'source': source_name, 'series': series, 'version': version, 'architectures': list(architectures),
                  'status': 'awaiting_source', 'builds': [], 'binaries': [], 'missing': [], 'indexes': []}
        sources = []
        for status in ('Published', 'Pending'):
            entries = self.collection(query(ARCHIVE, **{'ws.op': 'getPublishedSources', 'source_name': source_name,
                'version': version, 'distro_series': series_link, 'exact_match': 'true', 'status': status, 'ws.size': '100'}))
            for item in entries:
                require(all(item[key] == value for key, value in expected.items()) and item['component_name'] == 'main' and
                        item['status'] == status, 'Launchpad source differs from the exact target')
                require(re.fullmatch(re.escape(ARCHIVE) + r'/\+sourcepub/[0-9]+', item['self_link']), 'Invalid source publication ID')
            sources.extend(entries)
        require(len(sources) <= 1, 'Ambiguous exact Launchpad source publication')
        if not sources:
            return result
        source = sources[0]
        result.update(source_publication=source['self_link'], source_status=source['status'])
        builds = self.collection(query(source['self_link'], **{'ws.op': 'getBuilds', 'ws.size': '100'}))
        by_architecture, by_link = {}, {}
        for build in builds:
            require(all(build[key] == value for key, value in expected.items()) and
                    build['current_source_publication_link'] == source['self_link'], 'Launchpad build differs from the selected source')
            link = build['self_link']
            require(re.fullmatch(re.escape(ARCHIVE) + r'/\+build/[0-9]+', link), 'Invalid Launchpad build ID')
            require(build['arch_tag'] not in by_architecture and link not in by_link, 'Ambiguous source build architecture/ID')
            by_architecture[build['arch_tag']] = build
            by_link[link] = build
        failed, waiting = False, False
        for architecture in architectures:
            build = by_architecture.get(architecture)
            state = build['buildstate'] if build else 'Not scheduled'
            result['builds'].append({'architecture': architecture, 'status': state,
                'url': build['self_link'] if build else None, 'log_url': build.get('build_log_url') if build else None})
            if state != 'Successfully built':
                waiting |= state in WAITING or state == 'Not scheduled'
                failed |= state not in WAITING and state != 'Not scheduled'
        result['status'] = 'build_failed' if failed else 'awaiting_builds'
        if failed or waiting:
            return result
        binaries = self.collection(query(source['self_link'], **{'ws.op': 'getPublishedBinaries',
            'active_binaries_only': 'true', 'ws.size': '100'}))
        for architecture in architectures:
            index_url, index_hash, index = self.index(series, architecture)
            result['indexes'].append({'architecture': architecture, 'url': index_url, 'sha256': index_hash})
            for name in BINARIES[package]:
                binary_arch = 'all' if name == 'holder' else architecture
                matches = [item for item in binaries if item['binary_package_name'] == name and
                    item['distro_arch_series_link'] == series_link + '/' + architecture and
                    item['architecture_specific'] == (binary_arch != 'all')]
                require(len(matches) <= 1, 'Ambiguous binary publication: ' + name)
                if not matches:
                    result['missing'].append(f'{name}:{architecture}: publication')
                    continue
                binary = matches[0]
                require(all(binary[key] == expected[key] for key in ('source_package_name', 'source_package_version', 'archive_link', 'pocket')) and
                        binary['distro_arch_series_link'].startswith(series_link + '/') and binary['component_name'] == 'main' and
                        binary['binary_package_version'] == version, 'Binary publication differs from the selected source/version')
                require(re.fullmatch(re.escape(ARCHIVE) + r'/\+binarypub/[0-9]+', binary['self_link']), 'Invalid binary publication ID')
                build = by_link.get(binary['build_link'])
                require(build and build['buildstate'] == 'Successfully built' and
                        (binary_arch == 'all' or build['arch_tag'] == architecture), 'Binary belongs to a different or unsuccessful build')
                if binary['status'] != 'Published':
                    result['missing'].append(f'{name}:{architecture}: {binary["status"]}')
                    continue
                matches = [item for item in index if item.get('Package') == name and item.get('Version') == version and
                           item.get('Architecture') == binary_arch]
                require(len(matches) <= 1, 'Ambiguous APT index entry: ' + name)
                if not matches:
                    result['missing'].append(f'{name}:{architecture}: APT index')
                    continue
                entry = matches[0]
                require(entry.get('Source', name) in (source_name, f'{source_name} ({version})'), 'APT source differs from selected publication')
                path = PurePosixPath(entry['Filename'])
                require(not path.is_absolute() and '..' not in path.parts and str(path).startswith('pool/main/') and
                        path.suffix == '.deb' and re.fullmatch(r'[0-9a-f]{64}', entry['SHA256']) and
                        entry['Size'].isdecimal() and int(entry['Size']) > 0, 'Invalid APT package file metadata')
                result['binaries'].append({'name': name, 'architecture': architecture, 'package_architecture': binary_arch,
                    'publication': binary['self_link'], 'build': binary['build_link'], 'version': version,
                    'filename': str(path), 'size': int(entry['Size']), 'sha256': entry['SHA256']})
        result['status'] = 'available' if source['status'] == 'Published' and not result['missing'] else 'awaiting_publication'
        if result['status'] == 'available':
            result['receipt'] = {key: result[key] for key in ('source', 'series', 'version', 'architectures', 'source_publication', 'binaries')}
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    parser.add_argument('--ppa-revision', action='append', default=[])
    parser.add_argument('--ppa-architecture', action='append', choices=ARCHITECTURES)
    args = parser.parse_args()
    selected = revisions(args.ppa_revision)
    client = Client()
    result = {}
    for series in SERIES:
        for package in BINARIES:
            key = package + ':' + series
            result[key] = client.observe(package, series, package_version(args.version, selected.get(key, 1), series),
                                         sorted(set(args.ppa_architecture or ['amd64'])))
    print(json.dumps({'archive': ARCHIVE, 'checked_at': datetime.now(timezone.utc).isoformat(timespec='seconds'),
                      'targets': result}, indent=2))


if __name__ == '__main__':
    main()
