"""Exercise the real selection/result shell scripts and the compiled CI dependency graph."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location('shared_ci', ROOT / '.github/scripts/generate-shared-ci.py')
CI = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CI)
GRAPH = yaml.safe_load(CI.generate())
JOBS = GRAPH['jobs']


class SharedCITests(unittest.TestCase):
    def test_launchpad_default_leaves_kit_for_explicit_repairs(self):
        workflow = yaml.safe_load((CI.WORKFLOWS / 'release-launchpad.yml').read_text())
        source = workflow[True]['workflow_dispatch']['inputs']['packages']
        self.assertEqual(source['default'], 'all')
        self.assertIn('kit', source['options'])
        matrix = workflow['jobs']['upload']['strategy']['matrix']['package']
        self.assertTrue(matrix.endswith("|| '[\"core\", \"daemon\", \"desktop\"]') }}"))

    def test_standalone_kit_builds_manually_and_publishes_only_prepared_files(self):
        workflow = yaml.safe_load((CI.WORKFLOWS / 'release-kit-pypi.yml').read_text())
        self.assertEqual(set(workflow[True]), {'workflow_dispatch'})
        jobs = workflow['jobs']
        self.assertEqual(jobs['publish']['needs'], 'prepared')
        self.assertIn("inputs.prepared_release_ref != ''", jobs['prepared']['if'])
        publish_dirs = [step['with']['packages-dir'] for step in jobs['publish']['steps']
                        if step.get('uses', '').startswith('pypa/gh-action-pypi-publish@')]
        self.assertEqual(publish_dirs, ['pending/', 'pending/'])

        prepare = next(step['run'] for step in jobs['prepare']['steps'] if step.get('id') == 'config')
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            directory = root / 'python' / 'kit'
            directory.mkdir(parents=True)
            (root / 'VERSION').write_text('0.2.7\n')
            for destination, prepared_ref, valid in (('none', '', True), ('pypi', '', False),
                                                     ('testpypi', '', False), ('none', 'sha', False)):
                with self.subTest(destination=destination, prepared_ref=prepared_ref):
                    env = {**os.environ, 'DESTINATION': destination,
                           'PREPARED_PATH': 'saved.json' if prepared_ref else '', 'PREPARED_REF': prepared_ref,
                           'RELEASE_MANIFEST': '', 'RELEASE_MANIFEST_REF': '', 'GITHUB_OUTPUT': str(root / 'output')}
                    result = subprocess.run(['bash', '-e', '-c', prepare], cwd=directory, env=env, capture_output=True)
                    self.assertEqual(result.returncode == 0, valid, result.stderr)

    def selected(self, paths, event='pull_request', before='a' * 40):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            git = directory / 'git'
            git.write_text('#!/bin/bash\nprintf "%s\\n" "$CHANGED_PATHS"\n')
            git.chmod(0o755)
            selected = {}
            for suite in CI.SOURCES:
                output = directory / suite
                script = next(step['run'] for step in JOBS[f'{suite}-changes']['steps'] if step.get('id') == 'select')
                env = {**os.environ, 'PATH': str(directory) + os.pathsep + os.environ['PATH'],
                       'CHANGED_PATHS': '\n'.join(paths), 'GITHUB_OUTPUT': str(output),
                       'EVENT': event, 'BEFORE': before,
                       'SCOPED': 'true' if event in ('pull_request', 'push') else 'false'}
                subprocess.run(['bash', '-e', '-o', 'pipefail', '-c', script], env=env, check=True, capture_output=True)
                selected[suite] = output.read_text().strip().split('=')[1] == 'true'
            return {suite for suite, active in selected.items() if active}

    def result(self, suite, selected=True, overrides=None, skip_packages=False):
        job = JOBS[f'{suite}-result']
        results = {key: {'result': 'success' if selected else 'skipped'} for key in job['needs']}
        results[f'{suite}-changes']['result'] = 'success'
        if skip_packages:
            for key in results:
                if key.endswith(('ubuntu-packages', 'ubuntu-runtime')):
                    results[key]['result'] = 'skipped'
        for key, status in (overrides or {}).items():
            results[key]['result'] = status
        env = {**os.environ, 'RESULTS': json.dumps(results), 'SELECTED': str(selected).lower(),
               'SKIP_LAUNCHPAD': str(skip_packages).lower(), 'GITHUB_STEP_SUMMARY': os.devnull}
        return subprocess.run(['bash', '-e', '-o', 'pipefail', '-c', job['steps'][0]['run']],
                              env=env, capture_output=True, text=True)

    def test_generated_workflow_is_current(self):
        self.assertEqual((ROOT / '.github/workflows/shared-ci.yml').read_text(), CI.generate())

    def test_path_selection_on_pull_requests_and_main_pushes(self):
        cases = [(['README.md'], set()), (['core/src/models/Card.cpp'], {'core', 'daemon', 'kit'}),
                 (['core/tests/example.cpp'], {'core', 'kit'}),
                 (['core/src/capi/holder.cpp'], {'core', 'kit'}),
                 (['daemon/src/main.cpp'], {'daemon'}), (['python/kit/README.md'], {'kit'}),
                 (['python/holder/src/holder/client.py'], {'python'}),
                 (['VERSION'], set(CI.SOURCES)),
                 (['.github/workflows/shared-ci.yml'], set(CI.SOURCES)),
                 (['.github/scripts/generate-shared-ci.py'], set(CI.SOURCES))]
        for event in ('pull_request', 'push'):
            for paths, expected in cases:
                with self.subTest(event=event, paths=paths):
                    self.assertEqual(self.selected(paths, event), expected)
        self.assertEqual(self.selected([], 'workflow_dispatch'), set(CI.SOURCES))
        self.assertEqual(self.selected(['VERSION'], 'push', '0' * 40), set(CI.SOURCES))

    def test_selected_skipped_failed_and_cancelled_results(self):
        for suite in CI.SOURCES:
            for selected in (True, False):
                with self.subTest(suite=suite, selected=selected):
                    result = self.result(suite, selected)
                    self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            for key in JOBS[f'{suite}-result']['needs']:
                for status in ('failure', 'cancelled'):
                    with self.subTest(suite=suite, key=key, status=status):
                        self.assertNotEqual(self.result(suite, overrides={key: status}).returncode, 0)
            for key in JOBS[f'{suite}-result']['needs']:
                if key != f'{suite}-changes':
                    self.assertNotEqual(self.result(suite, False, {key: 'success'}).returncode, 0)

    def test_labelled_package_skips(self):
        for suite in ('kit', 'daemon'):
            result = self.result(suite, skip_packages=True)
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)

    def test_graph_is_acyclic_and_all_dependencies_exist(self):
        visited, visiting = set(), set()

        def visit(key):
            self.assertIn(key, JOBS)
            self.assertNotIn(key, visiting)
            if key in visited:
                return
            visiting.add(key)
            for parent in JOBS[key].get('needs', []):
                visit(parent)
            visiting.remove(key)
            visited.add(key)

        for key in JOBS:
            visit(key)

    def test_one_sdk_producer_per_platform_and_configuration(self):
        producers = {key: job for key, job in JOBS.items() if job.get('uses') == './.github/workflows/core-sdk.yml'}
        self.assertEqual(set(producers), {'sdk-linux', 'sdk-macos', 'sdk-windows'})
        for platform in ('linux', 'macos', 'windows'):
            self.assertEqual(json.loads(producers[f'sdk-{platform}']['with']['platforms']), [platform])
            self.assertFalse(producers[f'sdk-{platform}']['with']['aggregate'])
            if platform != 'linux':
                self.assertIn('sdk-linux', producers[f'sdk-{platform}']['needs'])
        self.assertNotIn('sdk-macos', JOBS['sdk-windows']['needs'])
        self.assertNotIn('sdk-windows', JOBS['sdk-macos']['needs'])

    def test_daemon_only_runs_keep_the_aggregate_staging_selection(self):
        self.assertIn("needs.daemon-changes.outputs.daemon == 'true'", JOBS['sdk-index']['if'])
        self.assertIn('sdk-index', JOBS['daemon-sdk-ready']['needs'])
        self.assertIn('core-sdk-ready', JOBS['core-result']['needs'])
        self.assertNotIn('sdk-index', JOBS['core-result']['needs'])

    def test_consumers_wait_only_for_their_platform(self):
        for suite in ('daemon', 'kit'):
            for platform in ('linux', 'macos', 'windows'):
                needs = JOBS[f'{suite}-{platform}']['needs']
                self.assertIn(f'sdk-{platform}', needs)
                for other in {'linux', 'macos', 'windows'} - {platform}:
                    self.assertNotIn(f'sdk-{other}', needs)
        self.assertIn('sdk-linux', JOBS['python-live']['needs'])
        for key in ('core-coverage', 'core-sanitizers', 'core-linux', 'core-macos', 'core-windows'):
            self.assertIn('sdk-linux', JOBS[key]['needs'])
            self.assertNotIn('sdk-macos', JOBS[key]['needs'])
            self.assertNotIn('sdk-windows', JOBS[key]['needs'])
        # Independent source/format/type checks still run if the Linux producer fails.
        for key in ('core-format', 'daemon-checks', 'kit-checks', 'python-check'):
            self.assertFalse(any(parent.startswith('sdk-') for parent in JOBS[key]['needs']))

    def test_platform_artifacts_and_tree_guard(self):
        action = yaml.safe_load((ROOT / '.github/actions/core-sdk/action.yml').read_text())
        downloads = [step for step in action['runs']['steps'] if 'download-artifact' in step.get('uses', '')]
        self.assertIn('core-selection-${{ runner.os', downloads[0]['with']['name'])
        producer = yaml.safe_load((CI.WORKFLOWS / 'core-sdk.yml').read_text())
        self.assertTrue(producer[True]['workflow_call']['inputs']['aggregate']['default'])
        self.assertEqual(producer['jobs']['sdk']['strategy']['fail-fast'], False)
        guard = next(step['run'] for step in action['runs']['steps'] if step.get('name') == "Verify the run selection's core tree")
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / 'out').mkdir()
            git = directory / 'git'
            git.write_text('#!/bin/bash\nprintf "%s\\n" "' + 'a' * 40 + '"\n')
            git.chmod(0o755)
            env = {**os.environ, 'PATH': str(directory) + os.pathsep + os.environ['PATH']}
            for tree, valid in [('a' * 40, True), ('b' * 40, False), (None, False)]:
                (directory / 'out/core-selection.json').write_text(json.dumps({'core_tree': tree}))
                result = subprocess.run(['bash', '-e', '-c', guard], cwd=directory, env=env, capture_output=True)
                self.assertEqual(result.returncode == 0, valid, result.stderr)

    def test_result_names_manual_release_and_android_paths(self):
        self.assertEqual({job.get('name') for key, job in JOBS.items() if key.endswith('-result')},
                         {'Core CI result', 'Daemon CI result', 'Kit CI result', 'Python check result'})
        for name in CI.SOURCES.values():
            triggers = yaml.safe_load((CI.WORKFLOWS / f'{name}.yml').read_text())[True]
            self.assertIn('workflow_dispatch', triggers)
            self.assertNotIn('pull_request', triggers)
            self.assertNotIn('push', triggers)
        daemon = yaml.safe_load((CI.WORKFLOWS / 'daemon-ci.yml').read_text())
        self.assertEqual(daemon['jobs']['core-sdk']['with']['build_type'], "${{ inputs.core_build_type || 'RelWithDebInfo' }}")
        self.assertEqual(daemon['jobs']['linux']['with']['daemon_commit'], '${{ needs.resolve.outputs.daemon_commit }}')
        self.assertNotIn('android', ' '.join(JOBS))
        self.assertIn("github.event.pull_request.number || github.run_id", GRAPH['concurrency']['group'])
        self.assertEqual(GRAPH['concurrency']['cancel-in-progress'], "${{ github.event_name == 'pull_request' }}")


if __name__ == '__main__':
    unittest.main()
