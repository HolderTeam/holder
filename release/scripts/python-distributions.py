#!/usr/bin/env python3
"""Record and verify Python release sets without rebuilding their distributions."""
import argparse
from email.parser import Parser
import importlib.util
import json
from pathlib import Path
import shutil
import re
import tarfile
import zipfile

SPEC = importlib.util.spec_from_file_location('python_manifest',
    Path(__file__).parents[1] / 'staging/scripts/generate-release-manifest.py')
manifest_tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(manifest_tool)
require = manifest_tool.require
ARTIFACTS = {'kit': ('kit-release-distributions', 'kit-release-manifest'),
             'python': ('holder-python-distributions', 'holder-python-release-manifest')}
WORKFLOWS = {'kit': 'release-kit-pypi.yml', 'python': 'release-holder-python-pypi.yml'}
PLATFORMS = {'manylinux_2_39_x86_64': 'linux', 'win_amd64': 'windows', 'macosx_15_0_arm64': 'macos'}
HOLDER_PYTHON_ABIS = ('cp311', 'cp312', 'cp313', 'cp314')
HOLDER_PYTHON_DESKTOP_ABIS = (*HOLDER_PYTHON_ABIS, 'cp315')
HOLDER_PYTHON_LINUX_TAG = 'manylinux_2_39_x86_64'
SDK_ARCHITECTURES = {'linux': 'x86_64', 'windows': 'x86_64', 'macos': 'arm64'}


def filenames(package, version, *, desktop_wheels=False, python_abis=HOLDER_PYTHON_ABIS):
    if package == 'python':
        tags = PLATFORMS if desktop_wheels else (HOLDER_PYTHON_LINUX_TAG,)
        return {f'holder_python-{version}-{abi}-{abi}-{tag}.whl'
                for abi in python_abis for tag in tags} | {
                    f'holder_python-{version}-py3-none-any.whl', f'holder_python-{version}.tar.gz'}
    return {f'holder_kit-{version}-{python}-{python}-{platform}.whl'
            for python in ('cp310', 'cp311', 'cp312', 'cp313', 'cp314', 'cp315')
            for platform in PLATFORMS} | {f'holder_kit-{version}.tar.gz'}


def metadata(data, name, version):
    parsed = Parser().parsestr(data.decode())
    require(parsed['Name'] == name and parsed['Version'] == version,
            f'Distribution metadata differs from {name} {version}')


def python_sdk(selection, version, platform):
    require(selection is not None, 'Holder Python verification requires the pinned core selection')
    require(selection['version'] == version.split('.post', 1)[0], 'Holder Python SDK version differs')
    assets = [asset for asset in selection['assets']
              if asset['platform'] == platform and asset['build_type'] == 'Release']
    require(len(assets) == 1 and assets[0]['architecture'] == SDK_ARCHITECTURES[platform],
            f'Holder Python {platform} SDK configuration is missing or ambiguous')
    return assets[0]


def python_core_evidence(selection, version, candidate, *, desktop_wheels=False):
    platforms = PLATFORMS.values() if desktop_wheels else ('linux',)
    assets = {platform: python_sdk(selection, version, platform) for platform in platforms}
    require(selection['commit'] == candidate, 'Pinned Holder Python core identity differs')
    record = {'commit': candidate, 'version': selection['version'], 'core_tree': selection['core_tree']}
    if desktop_wheels:
        record['sdk_assets'] = assets
    else:
        record['sdk_asset'] = assets['linux']
    return record


def check(directory, package, version, selection=None, *, desktop_wheels=False,
          python_abis=HOLDER_PYTHON_ABIS):
    expected = filenames(package, version, desktop_wheels=desktop_wheels, python_abis=python_abis)
    require({file.name for file in directory.iterdir()} == expected, 'Incomplete or unexpected Python distribution set')
    name = 'holder-kit' if package == 'kit' else 'holder-python'
    stem = name.replace('-', '_') + '-' + version
    for filename in sorted(expected):
        path = directory / filename
        if filename.endswith('.whl'):
            with zipfile.ZipFile(path) as wheel:
                entry = stem + '.dist-info/METADATA'
                require(wheel.namelist().count(entry) == 1, 'Wheel metadata is missing or ambiguous')
                metadata(wheel.read(entry), name, version)
                if package == 'python':
                    prefix = 'holder/_embedded/'
                    files = wheel.namelist()
                    extensions = [path for path in files if path.startswith(prefix + '_native')]
                    core_entry = prefix + '_core_build.json'
                    schema_entry = prefix + '_schema.sql'
                    if filename.endswith('-none-any.whl'):
                        require(not extensions and core_entry not in files and schema_entry not in files,
                                'Universal Holder Python wheel contains native build files')
                    else:
                        platform = PLATFORMS[filename.removesuffix('.whl').split('-')[-1]]
                        suffix = '.pyd' if platform == 'windows' else '.so'
                        require(len(extensions) == 1 and extensions[0].endswith(suffix) and
                                files.count(core_entry) == files.count(schema_entry) == 1,
                                'Native Holder Python wheel is missing its extension, schema or SDK metadata')
                        if platform == 'windows':
                            notices = stem + '.dist-info/licenses/third-party/'
                            require(stem + '.dist-info/DELVEWHEEL' in files and
                                    not any(path.startswith(prefix + '.libs/') for path in files) and
                                    any(path.startswith('holder_python.libs/') and path.endswith('.dll') for path in files) and
                                    notices + 'microsoft-runtime.txt' in files and
                                    any(path.startswith(notices + 'vcpkg/') and path.endswith('.txt') for path in files),
                                    'Windows Holder Python wheel is missing repaired DLLs or dependency licences')
                        elif platform == 'macos':
                            require(any(path.endswith('.dylib') for path in files) and
                                    any(path.startswith(stem + '.dist-info/licenses/third-party/') and
                                        not path.endswith('/') for path in files),
                                    'macOS Holder Python wheel is missing dylibs or dependency licences')
                        asset = python_sdk(selection, version, platform)
                        core = json.loads(wheel.read(core_entry))
                        require(all(core[key] == value for key, value in {
                            'commit': asset.get('commit', selection['commit']),
                            'version': selection['version'], 'platform': platform,
                            'architecture': asset['architecture'], 'build_type': 'Release'}.items()),
                            'Holder Python wheel differs from the pinned SDK')
                if package == 'kit':
                    require(selection is not None, 'Kit verification requires the pinned core selection')
                    platform = PLATFORMS[filename.removesuffix('.whl').split('-')[-1]]
                    assets = [asset for asset in selection['assets'] if asset['platform'] == platform
                              and asset['build_type'] == 'Release']
                    require(len(assets) == 1, 'Kit SDK configuration is missing or ambiguous')
                    require(wheel.namelist().count('holderkit/_core_build.json') == 1, 'Wheel core metadata is missing or ambiguous')
                    core = json.loads(wheel.read('holderkit/_core_build.json'))
                    asset = assets[0]
                    require(all(core[key] == value for key, value in {
                        'commit': asset.get('commit', selection['commit']), 'version': version,
                        'platform': platform, 'architecture': asset['architecture'], 'build_type': 'Release'}.items()),
                        'Kit wheel differs from the pinned SDK')
        else:
            with tarfile.open(path) as source:
                entries = [entry for entry in source.getmembers() if entry.name == stem + '/PKG-INFO' and entry.isfile()]
                require(len(entries) == 1, 'Source metadata is missing or ambiguous')
                metadata(source.extractfile(entries[0]).read(), name, version)
    return [{'name': filename, 'sha256': manifest_tool.file_sha256(directory / filename),
             'size': (directory / filename).stat().st_size} for filename in sorted(expected)]


def holder_wheel_targets(package, document):
    desktop_wheels = package == 'python' and 'sdk_assets' in document.get('core', {})
    if 'wheel_abis' not in document:
        return desktop_wheels, HOLDER_PYTHON_ABIS
    require(desktop_wheels and document['wheel_abis'] == list(HOLDER_PYTHON_DESKTOP_ABIS),
            'Unexpected Holder Python desktop wheel ABI set')
    return desktop_wheels, HOLDER_PYTHON_DESKTOP_ABIS


def verify(directory, package, version, candidate, document, selection=None):
    require(document['source'] == candidate and document['version'] == version,
            'Distribution manifest differs from candidate source/version')
    desktop_wheels, python_abis = holder_wheel_targets(package, document)
    files = check(directory, package, version, selection, desktop_wheels=desktop_wheels, python_abis=python_abis)
    require(document['files'] == files, 'Distribution file hashes differ from the manifest')
    if package == 'kit':
        require(selection['commit'] == candidate and selection['version'] == version, 'Pinned core identity differs')
        core = document['core']
        require(core['commit'] == candidate and core['version'] == version and
                core['core_tree'] == selection['core_tree'] and core['sdk_assets'] == selection['assets'] and
                core['sdk_commits'] == {asset['platform']: asset.get('commit', candidate)
                    for asset in selection['assets'] if asset['build_type'] == 'Release'},
                'Kit distribution manifest differs from the pinned SDK selection')
    else:
        require(document.get('core') == python_core_evidence(
            selection, version, candidate, desktop_wheels=desktop_wheels),
                'Holder Python distribution manifest differs from the pinned SDK selection')
    return files


def collect(record, github, names=None):
    """Download accepted artifact IDs, verify ZIPs/contents and return durable evidence."""
    result = {}
    for package in names or ARTIFACTS:
        dist_name, manifest_name = ARTIFACTS[package]
        step = record['steps'][package]
        require(step['workflow'] == WORKFLOWS[package], 'Unexpected Python producer workflow')
        accepted = {item['name']: item for item in step['artifacts']}
        # inspect() has already checked ownership, expiry and attempt. Recheck the
        # complete accepted artifact metadata before downloading immutable IDs.
        run = manifest_tool.run(record['repository'], step['run_id'], step['workflow'], record['candidate'], github.request)
        require(run['run_attempt'] == step['attempt'], 'Package run attempt changed')
        select = manifest_tool.artifacts(record['repository'], run, github.request,
                                         manifest_tool.datetime.now(manifest_tool.timezone.utc))
        for name in (dist_name, manifest_name):
            item = select(name)
            require(all(item.get(key) == value for key, value in accepted[name].items()), 'Accepted package artifact changed')
        manifest_item = select(manifest_name)
        values, manifest_hash = github.read(record['repository'], manifest_item, ['release-manifest.json'])
        document = json.loads(values['release-manifest.json'])
        item = select(dist_name)
        desktop_wheels, python_abis = holder_wheel_targets(package, document)
        expected = filenames(package, record['version'], desktop_wheels=desktop_wheels, python_abis=python_abis)
        _, archive_hash = github.read(record['repository'], item, [], expected)
        directory = github.directory / package
        directory.mkdir()
        with zipfile.ZipFile(github.directory / f"{item['id']}.zip") as archive:
            require({entry.filename for entry in archive.infolist() if not entry.is_dir()} == expected,
                    'Unexpected files in distribution artifact')
            for filename in expected:
                with archive.open(filename) as source, (directory / filename).open('wb') as output:
                    shutil.copyfileobj(source, output)
        files = verify(directory, package, record['version'], record['candidate'], document,
                       record['manifest']['evidence']['core_selection'])
        result[package] = {'source': record['candidate'], 'version': record['version'],
                          'run_id': step['run_id'], 'attempt': step['attempt'],
                          'distribution_artifact_id': item['id'], 'distribution_archive_sha256': archive_hash,
                          'manifest_artifact_id': manifest_item['id'], 'manifest_archive_sha256': manifest_hash,
                          'manifest': document, 'files': files,
                          'inputs_expire_at': min(entry['expires_at'] for entry in accepted.values())}
        current = manifest_tool.run(record['repository'], step['run_id'], step['workflow'], record['candidate'], github.request)
        require(current['run_attempt'] == step['attempt'], 'Package run was retried during verification')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--candidate', required=True)
    parser.add_argument('--selection', required=True, type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--desktop-wheels', action='store_true',
                        help='verify native wheels for Linux, macOS and Windows')
    args = parser.parse_args()
    manifest_tool.provenance.commit(args.candidate)
    require(re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:\.post[0-9]+)?', args.version), 'Invalid package version')
    selection = json.loads(args.selection.read_text())
    python_abis = HOLDER_PYTHON_DESKTOP_ABIS if args.desktop_wheels else HOLDER_PYTHON_ABIS
    document = {'source': args.candidate, 'version': args.version,
                'files': check(args.directory, 'python', args.version, selection,
                               desktop_wheels=args.desktop_wheels, python_abis=python_abis),
                'core': python_core_evidence(selection, args.version, args.candidate,
                                             desktop_wheels=args.desktop_wheels)}
    if args.desktop_wheels:
        document['wheel_abis'] = list(python_abis)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(document, indent=2) + '\n')


if __name__ == '__main__':
    main()
