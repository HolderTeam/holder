"""Pin the macOS handoff, verify a final disk image, and deliver accepted files."""
from datetime import datetime, timezone
import importlib.util
import json
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import sys
import tempfile


def sibling(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


manifest = sibling('macos_manifest', Path(__file__).parents[1] / 'staging/scripts/generate-release-manifest.py')
policy = sibling('macos_provenance', Path(__file__).with_name('candidate-provenance.py'))
release_assets = sibling('macos_release_assets', Path(__file__).with_name('promote-linux.py'))
require = manifest.require
METADATA = 'Holder.app/Contents/Resources/release/release-candidate.json'
CORE = 'Holder.app/Contents/Resources/release/holder-core-provenance.json'
FRAMEWORK = 'Holder.app/Contents/Resources/release/holder-framework-release.json'


def staging(record, github, clock=None):
    step = record['steps']['macos']
    artifact = step['artifacts'][0]
    repository = record['repository']
    run = manifest.run(repository, step['run_id'], 'staging-macos-stage.yml', step['workflow_commit'], github.request)
    require(run['run_attempt'] == step['attempt'], 'Accepted macOS staging attempt changed')
    item = manifest.artifacts(repository, run, github.request, clock or datetime.now(timezone.utc))(artifact['name'])
    require(item['id'] == artifact['id'], 'Accepted macOS staging artifact ID changed')
    values, checksum = github.read(repository, item, [METADATA, CORE, FRAMEWORK])
    require({key: item.get(key) for key in artifact} == artifact, 'Accepted macOS staging artifact metadata changed')
    metadata = json.loads(values[METADATA])
    require(metadata['platform'] == 'macos' and metadata['staging_repository'] == repository and
            metadata['staging_sha'] == step['workflow_commit'], 'macOS staging metadata differs from the selected workflow')
    provenance = {'appimage_version': metadata['version'], 'core': metadata['core']}
    for name, field in (('desktop', 'desktop_sha'), ('backend', 'backend_run_commit')):
        provenance[name] = {'repository': metadata[name + '_repository'], 'run_id': str(metadata[name + '_run_id']),
                            'commit': metadata[field], 'version': metadata['version']}
    bundle = {'run_id': step['run_id'], 'attempt': step['attempt'], 'workflow_commit': step['workflow_commit'],
              'artifact': artifact, 'archive_sha256': checksum, 'metadata': metadata,
              'core': json.loads(values[CORE]), 'framework': json.loads(values[FRAMEWORK]), 'provenance': provenance}
    bind(record, bundle)
    current = manifest.run(repository, step['run_id'], 'staging-macos-stage.yml', step['workflow_commit'], github.request)
    require(current['run_attempt'] == run['run_attempt'], 'macOS staging was retried during verification')
    return bundle


def bind(record, bundle):
    policy.bind(record, bundle['provenance'], 'macos')
    require(bundle['framework'] == record['manifest']['document'] and bundle['core'] == bundle['provenance']['core'],
            'macOS packaged manifest or SDK evidence differs from the accepted candidate')


def files(directory, version):
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', version), 'Invalid macOS delivery version')
    name = f'Holder-macos-{version}.dmg'
    result = []
    for filename in (name, name + '.sha256'):
        path = directory / filename
        require(path.is_file() and not path.is_symlink(), 'Missing regular macOS release file: ' + filename)
        checksum, size = release_assets.fingerprint(path)
        require(size > 0, 'Empty macOS release file: ' + filename)
        result.append({'name': filename, 'sha256': checksum, 'size': size})
    require((directory / (name + '.sha256')).stat().st_size <= 1024, 'Oversized macOS checksum file')
    lines = (directory / (name + '.sha256')).read_text(encoding='utf-8-sig').splitlines()
    require(len(lines) == 1 and re.fullmatch(re.escape(result[0]['sha256']) + r' [ *]' + re.escape(name), lines[0]),
            'macOS checksum must match the final disk image and release filename')
    return result


def native(command, label):
    result = subprocess.run(command, capture_output=True)
    require(result.returncode == 0, 'macOS verification failed: ' + label)


def verify(directory, record, bundle):
    require(sys.platform == 'darwin', 'Verify the final macOS disk image on a Mac')
    before = files(directory, record['version'])
    image = (directory / before[0]['name']).resolve()
    native(['/usr/bin/hdiutil', 'verify', str(image)], 'disk image integrity')
    temporary = Path(tempfile.mkdtemp(prefix='holder-macos-verify-'))
    mount = temporary / 'mounted'
    mount.mkdir()
    attached = False
    try:
        native(['/usr/bin/hdiutil', 'attach', '-readonly', '-nobrowse', '-mountpoint', str(mount), str(image)], 'mount disk image')
        attached = True
        app = mount / 'Holder.app'
        require(app.is_dir() and not app.is_symlink(), 'Final disk image must contain Holder.app')
        for relative, expected in ((METADATA, bundle['metadata']), (CORE, bundle['core']), (FRAMEWORK, bundle['framework'])):
            path = mount / relative
            require(path.is_file() and path.resolve().is_relative_to(app.resolve()), 'Missing macOS handoff metadata')
            require(json.loads(path.read_text(encoding='utf-8-sig')) == expected,
                    'Final disk image differs from the selected staged metadata')
        info = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
        require(info['CFBundleIdentifier'] == 'team.holder.Holder' and
                info['CFBundleShortVersionString'] == info['CFBundleVersion'] == record['version'],
                'Final macOS app has a different identity/version')
        native(['/usr/bin/codesign', '--verify', '--deep', '--strict', str(app)], 'app signature')
        native(['/usr/sbin/spctl', '--assess', '--type', 'execute', str(app)], 'Gatekeeper assessment')
    finally:
        try:
            if attached:
                native(['/usr/bin/hdiutil', 'detach', str(mount)], 'detach disk image; inspect hdiutil info for a remaining mount')
        finally:
            # Only remove empty mount directories. A failed detach leaves the mount for recovery.
            try:
                mount.rmdir()
                temporary.rmdir()
            except OSError:
                pass
    require(files(directory, record['version']) == before, 'macOS release files changed during verification')
    return {'files': before, 'checks': {'disk_image': 'passed', 'candidate_metadata': 'passed',
            'app_signature': 'passed', 'gatekeeper': 'passed'}}


def promote(github, repository, candidate, version, directory, expected):
    tag = 'holder-v' + version
    release_assets.tag_commit(github, repository, tag, candidate)
    release = release_assets.release_for(github, repository, tag)
    require(release is not None, 'Create the shared product draft before macOS delivery')
    # Keep one verified local copy throughout uploads, including a retry after a lost response.
    with tempfile.TemporaryDirectory(prefix='holder-macos-payload-') as temporary:
        payload = Path(temporary)
        require(files(directory, version) == expected, 'Local macOS files differ from the accepted final bytes')
        for item in expected:
            shutil.copyfile(directory / item['name'], payload / item['name'])
        require(files(payload, version) == expected, 'macOS files changed while preparing delivery')

        def writable(replacing=False):
            release_assets.tag_commit(github, repository, tag, candidate)
            current = github.api(f'repos/{repository}/releases/{release["id"]}')
            require(current['tag_name'] == tag and current['draft'] and not current.get('immutable'),
                    'macOS delivery requires the matching mutable draft')
            require(files(payload, version) == expected, 'macOS delivery payload changed')

        release_assets.assets.upload_missing(github, repository, tag, release, payload, expected, writable)
    result = release_assets.observe(github, repository, tag, expected, candidate, release_id=release['id'])
    require(result is not None, 'macOS delivery is incomplete; resume with the accepted files')
    return result
