"""Bind packaged component/SDK provenance to an accepted framework candidate."""
import importlib.util
from pathlib import Path

SPEC = importlib.util.spec_from_file_location('candidate_core_provenance',
    Path(__file__).parents[1] / 'staging/scripts/core-provenance.py')
core = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(core)


def bind(record, provenance, platform):
    label = {'linux': 'Linux', 'windows': 'Windows', 'macos': 'macOS'}[platform]
    document = record['manifest']['document']
    selection = record['manifest']['evidence']['core_selection']
    pin = {**document['platforms'][platform], 'core': document['core'], 'appimage_version': document['version']}
    core.require(document['version'] == provenance['appimage_version'] == record['version'] and
                 document['core']['commit'] == selection['commit'] == record['candidate'] and
                 selection['version'] == record['version'], label + ' bundle differs from candidate version/source')
    for name in ('desktop', 'backend'):
        actual, expected = provenance[name], pin[name]
        core.require(all(actual[key] == expected[key] for key in ('repository', 'run_id', 'commit')) and
                     actual['version'] == record['version'] and actual['repository'] == record['repository'] and
                     actual['commit'] == record['candidate'], f'{label} {name} differs from candidate pins')
    result = core.verify(selection, provenance['core'], provenance['desktop']['commit'], provenance['backend']['commit'],
        pin, platform, 'arm64' if platform == 'macos' else 'x86_64', 'Release', record['repository'], pin['backend']['run_id'])
    core.require(result == provenance['core'], label + ' SDK provenance differs from the accepted selection')
