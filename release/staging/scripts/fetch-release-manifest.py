#!/usr/bin/env python3
"""Read a candidate manifest from an immutable commit while staging runs on main."""
import argparse
import base64
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile


def fetch(root, path, reference, repository, request=None):
    if not reference:
        return  # Existing checked-in manifest inputs keep their original behaviour.
    if not re.fullmatch(r'[0-9a-f]{40}', reference):
        raise ValueError('release_manifest_ref must be a full lowercase commit SHA')
    if not re.fullmatch(r'release-manifests/[A-Za-z0-9._-]+\.json', path):
        raise ValueError('Remote manifests must use release-manifests/<candidate>.json')
    if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository):
        raise ValueError('Invalid manifest repository')
    target = Path(root) / path
    if not target.resolve().is_relative_to(Path(root).resolve()):
        raise ValueError('Manifest destination escapes the staging directory')
    route = f'repos/{repository}/contents/release/staging/{path}?ref={reference}'
    response = (request(route) if request else
                json.loads(subprocess.check_output(['gh', 'api', route], text=True, encoding='utf-8')))
    if response['type'] != 'file' or response['encoding'] != 'base64':
        raise ValueError('Manifest source must be a Git file')
    data = base64.b64decode(response['content'], validate=False)
    document = json.loads(data)
    platforms = document.get('platforms') if isinstance(document, dict) else None
    if not isinstance(platforms, dict) or set(platforms) != {'linux', 'windows', 'macos'}:
        raise ValueError('Remote manifest must select all three staging platforms')
    spec = importlib.util.spec_from_file_location('core_provenance', Path(__file__).with_name('core-provenance.py'))
    provenance = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(provenance)
    # Validate the complete document before replacing any local manifest.
    with tempfile.TemporaryDirectory() as directory:
        probe = Path(directory) / 'manifest.json'
        probe.write_bytes(data)
        for platform in document['platforms']:
            provenance.load_pin(probe, platform)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--path', default='')
    parser.add_argument('--ref', default='')
    parser.add_argument('--repository', required=True)
    args = parser.parse_args()
    fetch(args.root, args.path, args.ref, args.repository)


if __name__ == '__main__':
    main()
