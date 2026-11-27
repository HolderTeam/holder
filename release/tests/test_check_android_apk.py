import base64
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    'check_android_apk', Path(__file__).parents[1] / 'scripts/check-android-apk.py')
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)

DER = b'not a real certificate, but any DER bytes hash the same way'
EXPECTED = hashlib.sha256(DER).hexdigest()
OTHER = hashlib.sha256(b'another key').hexdigest()


def pem(der=DER):
    body = base64.b64encode(der).decode()
    lines = '\n'.join(body[i:i + 64] for i in range(0, len(body), 64))
    return f'-----BEGIN CERTIFICATE-----\n{lines}\n-----END CERTIFICATE-----\n'


def verify_output(*digests):
    lines = ['Verifies', 'Verified using v3 scheme (APK Signature Scheme v3): true']
    for number, digest in enumerate(digests, 1):
        lines.append(f'Signer #{number} certificate DN: CN=Holder')
        lines.append(f'Signer #{number} certificate SHA-256 digest: {digest}')
        lines.append(f'Signer #{number} public key SHA-256 digest: {"0" * 64}')
    return '\n'.join(lines) + '\n'


class CertificateTests(unittest.TestCase):
    def test_digest_is_of_the_der_certificate(self):
        self.assertEqual(tool.certificate_sha256(pem()), EXPECTED)

    def test_pem_must_hold_exactly_one_certificate(self):
        with self.assertRaisesRegex(ValueError, 'found 2'):
            tool.certificate_sha256(pem() + pem(b'second'))
        with self.assertRaisesRegex(ValueError, 'found 0'):
            tool.certificate_sha256('no certificate here')

    def test_public_key_digests_are_not_signers(self):
        self.assertEqual(tool.signer_sha256s(verify_output(EXPECTED)), [EXPECTED])


class SignerTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.pem = Path(directory.name) / 'app-signing.pem'
        self.pem.write_text(pem())
        self.apk = Path(directory.name) / 'Holder.apk'

    def check(self, returncode, stdout, stderr=''):
        result = subprocess.CompletedProcess([], returncode, stdout, stderr)
        with patch.object(tool.subprocess, 'run', return_value=result) as run:
            tool.check_signer(Path('apksigner'), self.apk, self.pem)
        self.assertEqual(run.call_args.args[0][:3], ['apksigner', 'verify', '--print-certs'])

    def test_expected_signer_passes(self):
        self.check(0, verify_output(EXPECTED))

    def test_other_signer_fails(self):
        with self.assertRaisesRegex(ValueError, f'signed by {OTHER}'):
            self.check(0, verify_output(OTHER))

    def test_extra_signer_fails(self):
        with self.assertRaisesRegex(ValueError, 'expected only'):
            self.check(0, verify_output(EXPECTED, OTHER))

    def test_unsigned_or_broken_apk_fails(self):
        with self.assertRaisesRegex(ValueError, 'could not verify.*DOES NOT VERIFY'):
            self.check(1, '', 'DOES NOT VERIFY')

    def test_no_signer_lines_fail(self):
        with self.assertRaisesRegex(ValueError, 'signed by none'):
            self.check(0, 'Verifies\n')


if __name__ == '__main__':
    unittest.main()
