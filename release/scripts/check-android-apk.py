#!/usr/bin/env python3
"""Verify an Android APK's version and, with --certificate, that it is signed only by that certificate."""
import argparse
import base64
import hashlib
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def build_tool(name, explicit):
    """The newest SDK build-tools copy of `name`, unless one is given."""
    if explicit is not None:
        return explicit
    sdk = os.environ.get('ANDROID_HOME') or os.environ.get('ANDROID_SDK_ROOT')
    tools = sorted((Path(sdk) / 'build-tools').glob(f'*/{name}*'), reverse=True) if sdk else []
    return next((path for path in tools if path.name in (name, name + '.exe', name + '.bat')), None)


def certificate_sha256(pem_text):
    """SHA-256 of the one DER certificate in a PEM file, as lower-case hex."""
    blocks = re.findall(r'-----BEGIN CERTIFICATE-----(.*?)-----END CERTIFICATE-----', pem_text, re.DOTALL)
    if len(blocks) != 1:
        raise ValueError(f'expected one certificate in the PEM file, found {len(blocks)}')
    der = base64.b64decode(''.join(blocks[0].split()), validate=True)
    return hashlib.sha256(der).hexdigest()


def signer_sha256s(verify_output):
    """The certificate SHA-256 of every signer `apksigner verify --print-certs` reports."""
    return re.findall(r'^Signer #\d+ certificate SHA-256 digest: ([0-9a-f]{64})$', verify_output, re.MULTILINE)


def check_signer(apksigner, apk, pem):
    expected = certificate_sha256(pem.read_text())
    result = subprocess.run([str(apksigner), 'verify', '--print-certs', '-v', str(apk)],
                            capture_output=True, text=True)
    if result.returncode != 0:
        raise ValueError(f'apksigner could not verify {apk.name}: {result.stderr.strip() or result.stdout.strip()}')
    signers = signer_sha256s(result.stdout)
    if signers != [expected]:
        found = ', '.join(signers) or 'none'
        raise ValueError(f'{apk.name} is signed by {found}; expected only {expected} from {pem}')
    print(f'APK is signed only by {pem.name}: {expected}')


def check_version(aapt, apk, suffix):
    version = (ROOT / 'VERSION').read_text().strip()
    major, minor, patch = map(int, version.split('.'))
    code = major * 1_000_000 + minor * 1_000 + patch
    badging = subprocess.run([str(aapt), 'dump', 'badging', str(apk)],
                             check=True, capture_output=True, text=True).stdout
    metadata = re.search(r"^package: .*versionCode='(\d+)' versionName='([^']*)'", badging, re.MULTILINE)
    if metadata is None:
        raise ValueError('APK manifest has no readable version metadata')
    actual_code, actual_name = metadata.groups()
    if (actual_code, actual_name) != (str(code), version + suffix):
        raise ValueError(f'APK version is {actual_name} ({actual_code}); '
                         f'expected {version + suffix} ({code}) from root VERSION')
    print(f'APK manifest matches root VERSION: {actual_name} ({actual_code})')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk', type=Path)
    parser.add_argument('--suffix', default='', help='expected build-type versionName suffix')
    parser.add_argument('--aapt', type=Path, help='Android build-tools aapt executable')
    parser.add_argument('--certificate', type=Path,
                        help='PEM certificate the APK must be signed by, and only by')
    parser.add_argument('--apksigner', type=Path, help='Android build-tools apksigner executable')
    args = parser.parse_args()
    aapt = build_tool('aapt', args.aapt)
    if aapt is None:
        parser.error('set ANDROID_HOME/ANDROID_SDK_ROOT or pass --aapt')
    check_version(aapt, args.apk, args.suffix)
    if args.certificate is not None:
        apksigner = build_tool('apksigner', args.apksigner)
        if apksigner is None:
            parser.error('set ANDROID_HOME/ANDROID_SDK_ROOT or pass --apksigner')
        check_signer(apksigner, args.apk, args.certificate)


if __name__ == '__main__':
    try:
        main()
    except ValueError as error:
        raise SystemExit(f'check-android-apk: {error}')
