import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    "component_runs", Path(__file__).parents[1] / "scripts/resolve-component-runs.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)

A, B, C = "a" * 40, "b" * 40, "c" * 40


class RunSelectionTests(unittest.TestCase):
    def request(self, path, **query):
        if path.endswith("/runs"):
            return {"workflow_runs": [self.make_run(2), self.make_run(1)]}
        if path.endswith("/artifacts"):
            return {"artifacts": [{"name": "backend", "expired": "/2/" in path}]}
        return self.make_run(2)

    @staticmethod
    def make_run(identifier, sha=A, conclusion="success"):
        return dict(id=identifier, status="completed", conclusion=conclusion, head_sha=sha)

    def test_auto_selection_skips_expired_artifacts(self):
        result = tool.resolve("owner/repo", "ci.yml", "main", "backend", request=self.request)
        self.assertEqual(result, ("1", A))

    def test_explicit_run_must_have_unexpired_artifact(self):
        with self.assertRaises(ValueError):
            tool.resolve("owner/repo", "ci.yml", "main", "backend", "2", self.request)

    def test_backend_workflow_follows_repository(self):
        self.assertEqual(tool.backend_workflow("HolderTeam/holder-core"), "daemon-integration.yml")
        self.assertEqual(tool.backend_workflow("HolderTeam/holder"), "daemon-ci.yml")

    def test_explicit_failed_run_is_rejected(self):
        def request(path, **query):
            return {**self.make_run(1), "conclusion": "failure"}
        with self.assertRaises(ValueError):
            tool.resolve("owner/repo", "ci.yml", "main", "backend", "1", request)


class FakeRepository:
    """Daemon CI and Desktop / CI runs over commits whose apps/desktop trees are given."""

    def __init__(self, desktop_trees, daemon_runs, desktop_runs):
        self.desktop_trees = desktop_trees
        self.runs = {"daemon-ci.yml": daemon_runs, "desktop-ci.yml": desktop_runs}

    def __call__(self, path, **query):
        parts = path.split("/")
        if path.endswith("/runs") and "/workflows/" in path:
            return {"workflow_runs": [RunSelectionTests.make_run(*run) for run in self.runs[parts[-2]]]}
        if path.endswith("/artifacts"):
            run = int(parts[-2])
            name = ("holder-daemon-linux-x86_64" if any(r[0] == run for r in self.runs["daemon-ci.yml"])
                    else "holder-desktop-linux-x86_64")
            return {"artifacts": [{"name": name, "expired": False}]}
        if "/actions/runs/" in path:
            run = int(parts[-1])
            return next(RunSelectionTests.make_run(*r) for runs in self.runs.values() for r in runs if r[0] == run)
        if "/commits/" in path:
            return {"commit": {"tree": {"sha": "root-" + parts[-1]}}}
        tree = parts[-1]
        if tree.startswith("root-"):
            return {"tree": [{"path": "apps", "type": "tree", "sha": "apps-" + tree[5:]}]}
        if tree.startswith("apps-"):
            return {"tree": [{"path": "desktop", "type": "tree", "sha": self.desktop_trees[tree[5:]]}]}
        raise AssertionError(path)


class DesktopSelectionTests(unittest.TestCase):
    def test_newest_desktop_run_with_the_backends_desktop_tree(self):
        # The desktop changed in C after the backend's commit B; A has the same desktop as B.
        repo = FakeRepository({A: "t1", B: "t1", C: "t2"}, daemon_runs=[(10, B)], desktop_runs=[(21, C), (20, A)])
        selected = tool.select("linux", "RelWithDebInfo", "HolderTeam/holder", "main", request=repo)
        self.assertEqual((selected["backend_run_id"], selected["desktop_run_id"]), ("10", "20"))
        self.assertEqual((selected["desktop_commit"], selected["desktop_tree"]), (A, "t1"))

    def test_explicit_desktop_run_from_other_desktop_source_is_rejected(self):
        repo = FakeRepository({B: "t1", C: "t2"}, daemon_runs=[(10, B)], desktop_runs=[(21, C)])
        with self.assertRaisesRegex(ValueError, "desktop source"):
            tool.select("linux", "RelWithDebInfo", "HolderTeam/holder", "main", desktop_run="21", request=repo)

    def test_no_desktop_run_for_the_backends_desktop_source(self):
        repo = FakeRepository({B: "t1", C: "t2"}, daemon_runs=[(10, B)], desktop_runs=[(21, C)])
        with self.assertRaisesRegex(ValueError, "No successful desktop-ci.yml run"):
            tool.select("linux", "RelWithDebInfo", "HolderTeam/holder", "main", request=repo)

    def test_same_commit_needs_no_tree_lookup_for_the_desktop(self):
        repo = FakeRepository({B: "t1"}, daemon_runs=[(10, B)], desktop_runs=[(20, B)])
        selected = tool.select("linux", "RelWithDebInfo", "HolderTeam/holder", "main", "20", "10", request=repo)
        self.assertEqual(selected["desktop_commit"], B)

    def test_release_artifact_names(self):
        self.assertEqual(tool.ARTIFACTS["windows"], ("holder-desktop-windows", "holder-daemon-windows-backend"))


if __name__ == "__main__":
    unittest.main()
