import json
import unittest
from unittest.mock import patch

import test_release_coordinator as preparation
from test_release_coordinator import CANDIDATE, tool
from test_website_release import fixture

website = tool.website_tool


class WebsiteObservationTests(unittest.TestCase):
    reload = preparation.CoordinatorTests.reload

    def setUp(self):
        preparation.CoordinatorTests.setUp(self)
        self.release, self.assets = fixture()
        self.stable = [self.release]
        generated = website.generator.generate(self.release, {'status': 'unconfirmed'},
            fetch=lambda item: self.assets[item['name']], verify=lambda *_: None,
            public_key=self.assets['Holder-linux-release-key.asc'])
        self.live = {path: data.encode() for path, data in generated.items()}
        catalog = json.loads(generated['_data/downloads.json'])
        for platform in ('linux', 'macos', 'windows'):
            self.live[platform + '/'] = f'<a data-download-asset="{platform}" href="{catalog["platforms"][platform]["url"]}">Download</a>'.encode()

    def advance(self):
        def api(path, method='GET', **options):
            self.assertEqual(method, 'GET')
            if '/git/ref/tags/' in path:
                return {'object': {'type': 'commit', 'sha': CANDIDATE}}
            if path.endswith('/assets'):
                return self.release['assets']
            self.assertTrue(path.endswith('/releases'))
            return self.stable
        observe = website.observe
        def observed(*args, **kwargs):
            return observe(*args, **kwargs, request=self.live.get, generate=lambda *args, **options:
                website.generator.generate(*args, **options, verify=lambda *_: None,
                    public_key=self.assets['Holder-linux-release-key.asc']))
        with patch.object(self.github, 'api', side_effect=api), patch.object(self.github, 'asset_contents',
                lambda repo, item: self.assets[item['name']], create=True), patch.object(website, 'observe', side_effect=observed):
            self.coordinator.advance_website_observation()

    def test_waits_for_deployment_and_saves_receipt_after_resume(self):
        feed = self.live['version.json']
        self.live['version.json'] = json.dumps({'version': '0.2.5'}).encode()
        self.advance()
        self.assertEqual(self.record['status'], 'awaiting_website')
        self.reload()
        self.live['version.json'] = feed
        self.advance()
        self.assertEqual(self.record['status'], 'website_verified')
        receipt = self.record['website_verified']['receipt']
        self.assertEqual(len(receipt['checksums']), 3)
        self.assertEqual(receipt['downloads']['windows']['channel'], 'development')
        self.reload()
        self.advance()
        self.assertEqual(self.record['website_verified']['receipt'], receipt)

    def test_matching_feed_alone_is_insufficient(self):
        self.live['windows/'] = b'<a href="/old-installer">Download</a>'
        self.advance()
        self.assertEqual(self.record['status'], 'awaiting_website')
        self.assertIn('windows download page', self.record['website_pending'])
        self.live['assets/checksums/Holder-0.2.6-SHA256SUMS'] = b'changed'
        with self.assertRaisesRegex(ValueError, 'Public checksum differs'):
            self.advance()

    def test_newer_release_and_changed_accepted_asset_are_rejected(self):
        self.advance()
        self.release['assets'][0]['id'] += 100
        with self.assertRaisesRegex(ValueError, 'Accepted website release assets'):
            self.advance()
        newer, _ = fixture('0.2.7')
        self.stable.append(newer)
        with self.assertRaisesRegex(ValueError, 'newer stable release'):
            self.advance()
