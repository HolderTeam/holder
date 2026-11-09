#!/usr/bin/env python3
"""Verify product version metadata in a built Android APK's binary manifest."""
import argparse
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk', type=Path)
    parser.add_argument('--suffix', default='', help='expected build-type versionName suffix')
    parser.add_argument('--aapt', type=Path, help='Android build-tools aapt executable')
    args = parser.parse_args()
    aapt = args.aapt
    if aapt is None:
        sdk = os.environ.get('ANDROID_HOME') or os.environ.get('ANDROID_SDK_ROOT')
        tools = sorted((Path(sdk) / 'build-tools').glob('*/aapt*')) if sdk else []
        aapt = next((path for path in tools if path.name in ('aapt', 'aapt.exe')), None)
    if aapt is None:
        parser.error('set ANDROID_HOME/ANDROID_SDK_ROOT or pass --aapt')
    version = (ROOT / 'VERSION').read_text().strip()
    major, minor, patch = map(int, version.split('.'))
    code = major * 1_000_000 + minor * 1_000 + patch
    badging = subprocess.run([str(aapt), 'dump', 'badging', str(args.apk)],
                             check=True, capture_output=True, text=True).stdout
    metadata = re.search(r"^package: .*versionCode='(\d+)' versionName='([^']*)'", badging, re.MULTILINE)
    if metadata is None:
        raise ValueError('APK manifest has no readable version metadata')
    actual_code, actual_name = metadata.groups()
    if (actual_code, actual_name) != (str(code), version + args.suffix):
        raise ValueError(f'APK version is {actual_name} ({actual_code}); '
                         f'expected {version + args.suffix} ({code}) from root VERSION')
    print(f'APK manifest matches root VERSION: {actual_name} ({actual_code})')


if __name__ == '__main__':
    main()
