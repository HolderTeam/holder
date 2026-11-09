import copy
import contextlib
import hashlib
import importlib.util
import json
import io
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('website_release', ROOT / 'release/scripts/website-release.py')
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)


def fixture(version='0.2.6', development=True):
    tag = 'holder-v' + version
    release = {'id': 123, 'tag_name': tag, 'draft': False, 'prerelease': False,
               'published_at': '2026-11-09T00:00:00Z', 'assets': [],
               'html_url': f'https://github.com/HolderTeam/holder/releases/tag/{tag}'}
    files = {}

    def add(name, data):
        digest = hashlib.sha256(data).hexdigest()
        release['assets'].append({'id': len(release['assets']) + 1, 'name': name,
                                   'state': 'uploaded', 'size': len(data), 'digest': 'sha256:' + digest,
                                   'browser_download_url': f'https://github.com/HolderTeam/holder/releases/download/{tag}/{name}'})
        files[name] = data
        return digest

    for name, checksum_name in (
        (f'Holder-{version}-x86_64.AppImage', f'Holder-{version}-SHA256SUMS'),
        (f'Holder-macos-{version}.dmg', f'Holder-macos-{version}.dmg.sha256'),
        (f'Holder-windows-{version}{"-dev" if development else ""}-Setup.exe',
         f'Holder-windows-{version}{"-dev" if development else ""}-Setup.exe.sha256'),
    ):
        digest = add(name, b'Fixture binary ' + name.encode())
        add(checksum_name, f'{digest}  {name}\n'.encode())
    add(f'Holder-{version}-SHA256SUMS.asc', b'Fixture detached signature')
    add('Holder-linux-release-key.asc', b'Fixture public key')
    return release, files


class WebsiteReleaseTests(unittest.TestCase):
    def setUp(self):
        self.release, self.files = fixture()
        self.ubuntu = {'status': 'available', 'version': '0.2.5'}
        self.verify = Mock()

    def generate(self):
        def fetch(asset):
            self.assertFalse(asset['name'].endswith(('.AppImage', '.dmg', '.exe')))
            return self.files[asset['name']]
        return tool.generate(self.release, self.ubuntu, fetch=fetch, verify=self.verify,
                             public_key=b'Fixture public key')

    def test_complete_release_generates_compatible_feed_and_independent_ppa_status(self):
        files = self.generate()
        self.assertEqual(files, self.generate())
        feed = json.loads(files['version.json'])
        catalog = json.loads(files['_data/downloads.json'])
        self.assertEqual((feed['schema'], feed['product'], feed['channel'], feed['version']),
                         (1, 'Holder', 'stable', '0.2.6'))
        self.assertEqual(set(feed['downloads']), {'linux', 'macos', 'windows'})
        self.assertEqual(feed['downloads']['linux'], 'https://holder.team/linux/')
        self.assertEqual(catalog['platforms']['windows']['channel'], 'development')
        self.assertEqual(catalog['platforms']['ubuntu']['version'], '0.2.5')
        self.assertNotIn('0.2.6', catalog['platforms']['ubuntu']['detail'])
        self.verify.assert_called_with(self.files['Holder-0.2.6-SHA256SUMS'],
                                       self.files['Holder-0.2.6-SHA256SUMS.asc'], b'Fixture public key')
        for platform in ('linux', 'windows', 'macos'):
            path = catalog['platforms'][platform]['checksum_url'].removeprefix('/')
            self.assertIn(path, files)

    def test_production_windows_pattern_is_distinct_from_development(self):
        self.release, self.files = fixture(development=False)
        catalog = json.loads(self.generate()['_data/downloads.json'])
        self.assertEqual(catalog['platforms']['windows']['channel'], 'stable')
        self.assertEqual(catalog['platforms']['windows']['version'], '0.2.6')

    def test_unconfirmed_ppa_does_not_inherit_github_release_version(self):
        self.ubuntu = {'status': 'unconfirmed', 'version': None}
        data = json.loads(self.generate()['_data/downloads.json'])['platforms']['ubuntu']
        self.assertIsNone(data['version'])
        self.assertEqual(data['channel'], 'unconfirmed')
        self.assertNotIn('0.2.6', data['detail'])

    def test_latest_stable_uses_version_order_and_excludes_drafts_prereleases_and_other_tags(self):
        older, _ = fixture('0.2.5')
        excluded = []
        for changes in ({'draft': True}, {'prerelease': True}, {'published_at': None},
                        {'tag_name': 'holder-v0.3.0-rc.1'}, {'tag_name': 'holder-python-v8.0.0'}):
            excluded.append({**self.release, **changes})
        self.assertEqual(tool.latest_stable([older, *excluded]), older)
        self.assertEqual(tool.latest_stable([self.release, older]), self.release)
        with self.assertRaisesRegex(ValueError, 'No published stable'):
            tool.latest_stable(excluded)

    def test_duplicate_highest_version_is_ambiguous(self):
        with self.assertRaisesRegex(ValueError, 'ambiguous'):
            tool.latest_stable([self.release, copy.deepcopy(self.release)])

    def test_newest_incomplete_release_does_not_fall_back_to_an_older_complete_one(self):
        older, _ = fixture('0.2.5')
        self.release['assets'].pop()
        chosen = tool.latest_stable([older, self.release])
        self.assertIs(chosen, self.release)
        with self.assertRaisesRegex(ValueError, 'needs exactly one'):
            self.generate()

    def test_missing_duplicate_incomplete_and_untrusted_assets_are_rejected(self):
        for mode in ('missing', 'duplicate', 'uploading', 'empty', 'no_digest', 'wrong_url'):
            with self.subTest(mode=mode):
                original = copy.deepcopy(self.release)
                item = self.release['assets'][0]
                if mode == 'missing':
                    self.release['assets'].pop(0)
                elif mode == 'duplicate':
                    self.release['assets'].append(copy.deepcopy(item))
                elif mode == 'uploading':
                    item['state'] = 'new'
                elif mode == 'empty':
                    item['size'] = 0
                elif mode == 'no_digest':
                    item['digest'] = None
                else:
                    item['browser_download_url'] = 'https://example.invalid/file'
                with self.assertRaises(ValueError):
                    self.generate()
                self.release = original

    def test_both_windows_installer_variants_are_ambiguous(self):
        other, _ = fixture(development=False)
        self.release['assets'].append(next(item for item in other['assets'] if item['name'].endswith('.exe')))
        with self.assertRaisesRegex(ValueError, 'found 2'):
            self.generate()

    def test_corrupt_download_and_mismatched_checksum_are_rejected(self):
        name = 'Holder-0.2.6-SHA256SUMS'
        self.files[name] = b'bad data'
        with self.assertRaisesRegex(ValueError, 'differs from its GitHub digest'):
            self.generate()
        item = next(item for item in self.release['assets'] if item['name'] == name)
        item['digest'] = 'sha256:' + hashlib.sha256(self.files[name]).hexdigest()
        with self.assertRaisesRegex(ValueError, 'names the wrong asset'):
            self.generate()

    def test_checksum_must_match_the_binary_digest(self):
        self.release['assets'][0]['digest'] = 'sha256:' + '0' * 64
        with self.assertRaisesRegex(ValueError, 'checksum differs from GitHub'):
            self.generate()

    def test_incomplete_release_leaves_existing_generated_data_untouched(self):
        with tempfile.TemporaryDirectory() as directory:
            site = Path(directory)
            (site / '_data').mkdir()
            (site / '_data/ubuntu.json').write_text(json.dumps(self.ubuntu))
            (site / 'version.json').write_text('previous feed\n')
            (site / '_data/downloads.json').write_text('previous catalog\n')
            release = copy.deepcopy(self.release)
            assets = release.pop('assets')[:-1]
            with patch.object(tool, 'pages', side_effect=[[release], assets]), \
                    patch.object(sys, 'argv', ['website-release.py', '--site', str(site)]), \
                    contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
                tool.main()
            self.assertEqual((site / 'version.json').read_text(), 'previous feed\n')
            self.assertEqual((site / '_data/downloads.json').read_text(), 'previous catalog\n')

    def test_wrong_key_or_signature_is_rejected(self):
        self.verify.side_effect = subprocess.CalledProcessError(1, ['gpgv'])
        with self.assertRaisesRegex(ValueError, 'signature does not verify'):
            self.generate()
        self.verify.side_effect = None
        name = 'Holder-linux-release-key.asc'
        self.files[name] = b'Unexpected release public key'
        next(item for item in self.release['assets'] if item['name'] == name)['digest'] = \
            'sha256:' + hashlib.sha256(self.files[name]).hexdigest()
        with self.assertRaisesRegex(ValueError, 'differs from the committed'):
            self.generate()

    def test_release_and_asset_pages_are_all_read(self):
        requests = []
        def request(path):
            requests.append(path)
            return [self.release] * 100 if path.endswith('&page=1') else [self.release]
        self.assertEqual(len(tool.pages('releases', request)), 101)
        self.assertEqual(len(requests), 2)

    @unittest.skipUnless(shutil.which('gpg') and shutil.which('gpgv'), 'GnuPG is needed for signature verification')
    def test_public_release_signature_verifies_and_modified_checksum_fails(self):
        fixture_root = ROOT / 'release/tests/fixtures/website'
        sums = (fixture_root / 'Holder-0.2.5-SHA256SUMS').read_bytes()
        signature = (fixture_root / 'Holder-0.2.5-SHA256SUMS.asc').read_bytes()
        key = (ROOT / 'release/keys/holder-linux-release.asc').read_bytes()
        tool.verify_signature(sums, signature, key)
        with self.assertRaises(subprocess.CalledProcessError):
            tool.verify_signature(sums.replace(b'dbcbe', b'abcbe', 1), signature, key)


if __name__ == '__main__':
    unittest.main()
