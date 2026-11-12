import copy
import gzip
import json
import unittest
from unittest.mock import patch
from urllib.error import HTTPError
from urllib.parse import parse_qs, urlsplit

import test_release_coordinator as preparation
from test_release_coordinator import CANDIDATE, REPOSITORY, tool

ppa = tool.ppa_tool


class Archive:
    def __init__(self):
        self.records = {}
        self.indexes = {}
        self.requests = []
        self.pages = {}

    def add(self, package, series, revision=1, architectures=('amd64',)):
        version = ppa.package_version('0.2.6', revision, series)
        source = 'holder-' + package
        series_link = ppa.API + '/ubuntu/' + series
        key = (source, series_link, version)
        identifier = len(self.records) + 1
        common = {'archive_link': ppa.ARCHIVE, 'pocket': 'Release',
                  'source_package_name': source, 'source_package_version': version}
        source_link = f'{ppa.ARCHIVE}/+sourcepub/{identifier}'
        record = {'source': {**common, 'distro_series_link': series_link, 'component_name': 'main',
                             'self_link': source_link, 'status': 'Published'}, 'builds': [], 'binaries': []}
        for i, arch in enumerate(architectures):
            build = f'{ppa.ARCHIVE}/+build/{identifier * 10 + i}'
            record['builds'].append({**common, 'distro_series_link': series_link, 'arch_tag': arch,
                'self_link': build, 'current_source_publication_link': source_link,
                'buildstate': 'Successfully built', 'build_log_url': build + '/log'})
            for j, name in enumerate(ppa.BINARIES[package]):
                binary_arch = 'all' if name == 'holder' else arch
                record['binaries'].append({**common, 'component_name': 'main',
                    'distro_arch_series_link': series_link + '/' + arch, 'architecture_specific': binary_arch != 'all',
                    'self_link': f'{ppa.ARCHIVE}/+binarypub/{identifier * 100 + i * 10 + j}',
                    'binary_package_name': name, 'binary_package_version': version, 'status': 'Published',
                    'build_link': record['builds'][0]['self_link'] if binary_arch == 'all' else build})
                entry = {'Package': name, 'Source': source, 'Version': version, 'Architecture': binary_arch,
                         'Filename': f'pool/main/h/{source}/{name}_{version}_{binary_arch}.deb', 'Size': '123', 'SHA256': 'a' * 64}
                self.indexes.setdefault((series, arch), []).append(entry)
        self.records[key] = record
        return record

    def request(self, url):
        self.requests.append(url)
        parsed = urlsplit(url)
        if url in self.pages:
            return json.dumps(self.pages[url]).encode()
        if url.startswith(ppa.CONTENT):
            parts = parsed.path.split('/')
            key = (parts[-4], parts[-2].removeprefix('binary-'))
            if key not in self.indexes:
                raise HTTPError(url, 404, 'Not found', {}, None)
            return gzip.compress('\n\n'.join('\n'.join(f'{k}: {v}' for k, v in item.items()) + '\nDescription: fixture\n continued'
                for item in self.indexes[key]).encode(), mtime=0)
        values = {k: v[0] for k, v in parse_qs(parsed.query).items()}
        operation = values['ws.op']
        if operation == 'getPublishedSources':
            key = (values['source_name'], values['distro_series'], values['version'])
            record = self.records.get(key)
            items = [record['source']] if record and values['status'] == record['source']['status'] else []
        else:
            base = url.split('?')[0]
            record = next(item for item in self.records.values() if item['source']['self_link'] == base)
            items = record[{'getBuilds': 'builds', 'getPublishedBinaries': 'binaries'}[operation]]
        return json.dumps({'entries': items}).encode()


class ObservationTests(unittest.TestCase):
    def setUp(self):
        self.archive = Archive()
        self.record = self.archive.add('desktop', 'noble', architectures=('amd64', 'arm64'))

    def observe(self, version='0.2.6-1~noble1', architectures=('amd64',)):
        return ppa.Client(self.archive.request).observe('desktop', 'noble', version, architectures)

    def test_exact_publications_and_indexes_include_architecture_independent_package(self):
        observed = self.observe(architectures=('amd64', 'arm64'))
        self.assertEqual(observed['status'], 'available')
        receipt = observed['receipt']
        self.assertEqual(len(receipt['binaries']), 4)
        holder = [item for item in receipt['binaries'] if item['name'] == 'holder']
        self.assertEqual({item['package_architecture'] for item in holder}, {'all'})
        self.assertEqual(len({item['publication'] for item in holder}), 2)
        self.assertEqual(len({item['build'] for item in holder}), 1)
        self.assertEqual({item['sha256'] for item in receipt['binaries']}, {'a' * 64})

    def test_newer_revision_never_substitutes_for_exact_version(self):
        self.archive.add('desktop', 'noble', revision=2)
        self.assertEqual(self.observe('0.2.6-3~noble1')['status'], 'awaiting_source')
        self.assertEqual(self.observe('0.2.6-2~noble1')['receipt']['version'], '0.2.6-2~noble1')

    def test_source_pending_and_build_wait_or_failure_are_distinct(self):
        self.record['source']['status'] = 'Pending'
        self.assertEqual(self.observe()['status'], 'awaiting_publication')
        for state, expected in [('Dependency wait', 'awaiting_builds'), ('Needs building', 'awaiting_builds'),
                                ('Failed to build', 'build_failed'), ('Failed to upload', 'build_failed')]:
            with self.subTest(state=state):
                self.record['builds'][0]['buildstate'] = state
                observed = self.observe()
                self.assertEqual(observed['status'], expected)
                self.assertEqual(observed['builds'][0]['status'], state)
                self.assertIn('/log', observed['builds'][0]['log_url'])
                self.assertNotIn('receipt', observed)

    def test_binary_publication_without_apt_index_does_not_complete(self):
        del self.archive.indexes[('noble', 'amd64')]
        observed = self.observe()
        self.assertEqual(observed['status'], 'awaiting_publication')
        self.assertEqual(set(observed['missing']), {'holder:amd64: APT index', 'holder-desktop:amd64: APT index'})
        self.assertIsNone(observed['indexes'][0]['sha256'])

    def test_index_without_published_binary_does_not_complete(self):
        self.record['binaries'][0]['status'] = 'Pending'
        observed = self.observe()
        self.assertEqual(observed['status'], 'awaiting_publication')
        self.assertEqual(observed['missing'], ['holder:amd64: Pending'])

    def test_wrong_build_or_duplicate_publication_fails_closed(self):
        binary = self.record['binaries'][1]
        original = binary['build_link']
        binary['build_link'] = self.record['builds'][1]['self_link']
        with self.assertRaisesRegex(ValueError, 'different or unsuccessful build'):
            self.observe()
        binary['build_link'] = original
        self.record['binaries'].append(copy.deepcopy(binary))
        with self.assertRaisesRegex(ValueError, 'Ambiguous binary publication'):
            self.observe()

    def test_wrong_index_source_and_duplicate_entries_rejected(self):
        index = self.archive.indexes[('noble', 'amd64')]
        index[0]['Source'] = 'other-source'
        with self.assertRaisesRegex(ValueError, 'APT source differs'):
            self.observe()
        index[0]['Source'] = 'holder-desktop'
        index.append(index[0])
        with self.assertRaisesRegex(ValueError, 'Ambiguous APT index'):
            self.observe()

    def test_collection_pagination_checks_next_page_and_rejects_cycles(self):
        source = self.record['source']['self_link']
        first = ppa.query(source, **{'ws.op': 'getBuilds', 'ws.size': '100'})
        second = first + '&ws.start=1'
        self.archive.pages[first] = {'entries': [], 'next_collection_link': second}
        self.archive.pages[second] = {'entries': self.record['builds']}
        self.assertEqual(self.observe()['status'], 'available')
        self.archive.pages[second]['next_collection_link'] = first
        with self.assertRaisesRegex(ValueError, 'repeated a page'):
            self.observe()


class PpaCoordinatorTests(unittest.TestCase):
    reload = preparation.CoordinatorTests.reload

    def setUp(self):
        preparation.CoordinatorTests.setUp(self)
        self.archive = Archive()
        self.remote = {}
        for series in ppa.SERIES:
            for package in ppa.BINARIES:
                self.remote[package + ':' + series] = self.archive.add(package, series)
        self.tag = CANDIDATE
        api = self.github.api
        def tag_api(route, method='GET', **options):
            if route == f'repos/{REPOSITORY}/git/ref/tags/holder-v0.2.6':
                self.assertEqual(method, 'GET')
                return {'object': {'type': 'commit', 'sha': self.tag}} if self.tag else None
            return api(route, method=method, **options)
        self.github.api = tag_api
        self.client = ppa.Client
        patcher = patch.object(ppa, 'Client', side_effect=lambda: self.client(self.archive.request))
        patcher.start()
        self.addCleanup(patcher.stop)

    def advance(self, revisions=None, architectures=None):
        self.coordinator.advance_ppa_observation(revisions, architectures)
        self.assertFalse(self.github.mutations)

    def test_complete_both_series_and_recheck_after_reload(self):
        self.advance()
        self.assertEqual(self.record['status'], 'ppa_available')
        self.assertEqual(len(self.record['ppa']['targets']), 8)
        self.assertEqual(sum(url.startswith(ppa.CONTENT) for url in self.archive.requests), 2)
        saved = copy.deepcopy(self.record['ppa']['targets'])
        self.reload()
        self.advance()
        self.assertEqual(saved, self.record['ppa']['targets'])
        self.assertIn('PPA architecture coverage: amd64', tool.summary(self.record))

    def test_pending_source_and_failed_build_preserve_completed_siblings(self):
        missing = self.remote['kit:resolute']['source']
        missing['status'] = 'Superseded'
        self.advance()
        self.assertEqual(self.record['status'], 'awaiting_ppa')
        self.assertEqual(self.record['ppa']['targets']['kit:resolute']['observation']['status'], 'awaiting_source')
        missing['status'] = 'Published'
        build = self.remote['kit:resolute']['builds'][0]
        build['buildstate'] = 'Failed to build'
        with self.assertRaisesRegex(ValueError, 'build failures.*kit:resolute'):
            self.advance()
        self.reload()
        self.assertIn('Failed to build', tool.summary(self.record))
        self.assertIn('completed', self.record['ppa']['targets']['core:noble'])
        build['buildstate'] = 'Successfully built'
        self.advance()
        self.assertEqual(self.record['status'], 'ppa_available')

    def test_api_interruption_keeps_completed_targets_for_resume(self):
        request = self.archive.request
        def interrupted(url):
            if 'source_name=holder-kit' in url:
                raise TimeoutError('Launchpad temporarily unavailable')
            return request(url)
        with patch.object(self.archive, 'request', side_effect=interrupted):
            with self.assertRaisesRegex(TimeoutError, 'temporarily unavailable'):
                self.advance()
        self.reload()
        self.assertIn('completed', self.record['ppa']['targets']['core:noble'])
        self.assertNotIn('kit:noble', self.record['ppa']['targets'])
        self.advance()
        self.assertEqual(self.record['status'], 'ppa_available')

    def test_explicit_packaging_repair_keeps_history_and_other_receipts(self):
        self.advance()
        before = copy.deepcopy(self.record['ppa']['targets'])
        self.archive.add('daemon', 'noble', revision=2)
        self.advance({'daemon:noble': 2})
        repaired = self.record['ppa']
        self.assertEqual(repaired['history'][0]['targets'], {'daemon:noble': before['daemon:noble']})
        self.assertEqual(repaired['targets']['daemon:noble']['completed']['receipt']['version'], '0.2.6-2~noble1')
        self.assertEqual(repaired['targets']['daemon:resolute'], before['daemon:resolute'])
        self.reload()
        self.advance({'daemon:noble': 2})
        self.assertEqual(len(self.record['ppa']['history']), 1)
        with self.assertRaisesRegex(ValueError, 'cannot move backwards'):
            self.advance({'daemon:noble': 1})

    def test_index_hash_drift_preserves_the_accepted_receipt(self):
        self.advance()
        old = copy.deepcopy(self.record['ppa']['targets']['core:noble']['completed'])
        self.archive.indexes[('noble', 'amd64')][0]['SHA256'] = 'f' * 64
        with self.assertRaisesRegex(ValueError, 'file hashes changed'):
            self.advance()
        self.reload()
        self.assertEqual(self.record['ppa']['targets']['core:noble']['completed'], old)

    def test_disappearing_package_is_not_silently_marked_complete(self):
        self.advance()
        self.archive.indexes[('noble', 'amd64')].pop(0)
        with self.assertRaisesRegex(ValueError, 'no longer available: core:noble'):
            self.advance()

    def test_missing_or_moved_tag_stops_before_launchpad_reads(self):
        for tag in (None, 'b' * 40):
            self.tag = tag
            with self.assertRaisesRegex(ValueError, 'Create the reviewed release tag|differs from prepared candidate'):
                self.advance()
        self.assertEqual(self.archive.requests, [])

    def test_saved_architecture_plan_cannot_change_implicitly(self):
        self.advance()
        with self.assertRaisesRegex(ValueError, 'coverage differs'):
            self.advance(architectures=['amd64', 'arm64'])
        self.record['ppa']['plan']['revisions']['core:noble'] = 3
        with self.assertRaisesRegex(ValueError, 'Saved PPA observation plan changed'):
            self.advance()


if __name__ == '__main__':
    unittest.main()
