import contextlib
import copy
import io
import sys
import unittest
from unittest.mock import patch

import test_release_coordinator as preparation
from test_release_coordinator import tool


class ReleaseSummaryTests(unittest.TestCase):
    setUp = preparation.CoordinatorTests.setUp
    reload = preparation.CoordinatorTests.reload

    def overview(self):
        return '\n'.join(tool.destination_summary(self.record))

    def ledger(self, **values):
        receipt = {'verified_at': '2026-11-12T12:00:00+00:00', **values}
        return {'receipt': receipt, 'sha256': tool.digest(receipt)}

    def test_legacy_record_shows_pending_destinations_without_inventing_scope(self):
        overview = self.overview()
        self.assertIn('scope is unrecorded', overview)
        self.assertIn('Linux AppImage | Preparation pending | `prepare`', overview)
        self.assertIn('Kit on PyPI | Pending | `packages`', overview)
        self.assertIn('Ubuntu PPA', overview)
        self.assertNotIn('TestPyPI', overview.split('| Destination |')[1])
        self.assertNotIn('release_scope', self.record)

    def test_scope_command_changes_only_local_selection_and_preserves_receipts(self):
        self.record['publications'] = {'kit_testpypi': self.ledger()}
        tool.save(self.path, self.record)
        before = copy.deepcopy(self.record)
        with patch.object(tool, 'GitHub', return_value=self.github), patch.object(sys, 'argv', [
            'release.py', 'scope', '--record', str(self.path), '--include', 'ppa', 'website', 'ppa'
        ]), contextlib.redirect_stdout(io.StringIO()):
            tool.main()
        self.reload()
        self.assertEqual(self.record['release_scope'], ['ppa', 'website'])
        self.assertEqual({key: value for key, value in self.record.items() if key != 'release_scope'}, before)
        self.assertFalse(self.github.mutations)
        table = self.overview()
        self.assertIn('Ubuntu PPA', table)
        self.assertNotIn('Linux AppImage', table)
        self.assertTrue(self.path.with_suffix('.summary.md').is_file())

    def test_draft_delivery_and_published_destinations_are_distinct(self):
        self.record['release_scope'] = ['linux', 'sdk', 'website']
        self.record['linux_promoted'] = self.ledger(release={'draft': True})
        self.record['core_sdk_published'] = self.ledger(release={'draft': False})
        self.record['website_verified'] = self.ledger()
        table = self.overview()
        self.assertIn('Linux AppImage | Delivered to draft', table)
        self.assertIn('Core SDKs | Published', table)
        self.assertIn('Website and update feed | Verified', table)
        self.record['website_pending'] = 'Waiting for the website'
        self.assertIn('Website and update feed | Live check pending', self.overview())
        self.record['linux_promoted']['receipt']['release']['draft'] = False
        self.assertIn('Linux AppImage | Evidence checksum mismatch', self.overview())

    def test_next_command_tracks_saved_handoff_and_package_inputs(self):
        self.record['manifest'] = {}
        self.record['release_scope'] = ['linux', 'macos', 'kit-testpypi']
        self.record['status'] = 'awaiting_linux_signing_approval'
        self.record['macos_handoff'] = {'retained': True}
        table = self.overview()
        self.assertIn('Linux AppImage | Awaiting signing approval | `sign-linux`', table)
        self.assertIn('accept-macos --macos-directory', table)
        self.record['macos_verified'] = {'retained': True}
        self.record['python_packages'] = {'prepared': True}
        table = self.overview()
        self.assertIn('macOS disk image | Pending | `promote-macos`', table)
        self.assertIn('publish-python --package kit --destination testpypi', table)

    def test_ppa_counts_only_current_available_matching_receipts(self):
        key = 'core:noble'
        receipt = {'version': '0.2.6-1~noble1'}
        completed = {'receipt': receipt, 'sha256': tool.digest(receipt)}
        plan = {'revisions': {key: 1, 'daemon:noble': 1}}
        self.record['release_scope'] = ['ppa']
        self.record['ppa'] = {'plan': plan, 'sha256': tool.digest(plan), 'targets': {
            key: {'completed': completed, 'observation': {'status': 'available', 'receipt': receipt},
                  'checked_at': '2026-11-12T12:00:00+00:00'}}}
        self.assertIn('1/2 source/series targets available', self.overview())
        self.record['ppa']['targets'][key]['observation']['status'] = 'awaiting_publication'
        self.assertIn('0/2 source/series targets available', self.overview())
        self.record['ppa']['plan']['revisions'][key] = 2
        self.assertIn('Evidence checksum mismatch', self.overview())

    def test_read_only_status_uses_saved_scope_while_the_record_is_locked(self):
        self.record['release_scope'] = ['website']
        tool.save(self.path, self.record)
        original = self.path.read_bytes()
        self.path.with_name(self.path.name + '.lock').write_text('')
        output = io.StringIO()
        with patch.object(sys, 'argv', ['release.py', 'status', '--record', str(self.path)]), \
                patch.object(tool.GitHub, 'api', side_effect=AssertionError('Status must use saved evidence')), \
                contextlib.redirect_stdout(output):
            tool.main()
        self.assertIn('Selected release destinations: website', output.getvalue())
        self.assertEqual(self.path.read_bytes(), original)

    def test_invalid_saved_scope_is_rejected(self):
        for scope in ([], ['unsupported'], ['linux', 'linux'], 'linux'):
            with self.subTest(scope=scope), self.assertRaisesRegex(ValueError, 'destination scope'):
                tool.validate_record({**self.record, 'release_scope': scope})
