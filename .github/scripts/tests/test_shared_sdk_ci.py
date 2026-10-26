"""Selection, dependency failures and SDK provenance for the shared CI entrypoint."""

import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def load(name):
    spec = importlib.util.spec_from_file_location(
        name.replace("-", "_"), ROOT / ".github/scripts" / (name + ".py")
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


selector = load("select-sdk-ci")
results = load("sdk-ci-result")
shared = load("verify-shared-sdk")


class SelectionTests(unittest.TestCase):
    def test_core_changes_select_sdk_consumers_but_not_unrelated_python(self):
        chosen = selector.select(["core/src/card/CardRepo.cpp"])
        self.assertTrue(
            all(chosen[c] for c in ["core", "daemon", "kit", "sdk", "all_platforms"])
        )
        self.assertFalse(chosen["python"])

    def test_core_c_api_and_tests_do_not_select_daemon(self):
        for path in [
            "core/include/holder/holder.h",
            "core/src/capi/context.cpp",
            "core/tests/context_test.cpp",
        ]:
            with self.subTest(path=path):
                chosen = selector.select([path])
                self.assertTrue(chosen["core"] and chosen["kit"])
                self.assertFalse(chosen["daemon"])

    def test_python_only_requests_linux_and_no_core_test_build(self):
        chosen = selector.select(["python/holder/src/holder/events.py"])
        self.assertTrue(chosen["python"] and chosen["sdk"])
        self.assertFalse(
            any(chosen[c] for c in ["core", "daemon", "kit", "all_platforms"])
        )

    def test_daemon_only_does_not_wait_for_core_test_builds(self):
        chosen = selector.select(["daemon/src/api/CardRead.cpp"])
        self.assertTrue(chosen["daemon"] and chosen["all_platforms"])
        self.assertFalse(chosen["core"] or chosen["kit"] or chosen["python"])

    def test_shared_workflow_and_action_changes_select_all_consumers(self):
        for path in [
            ".github/workflows/daemon-ci.yml",
            ".github/workflows/core-sdk.yml",
            ".github/actions/core-sdk/action.yml",
            ".github/scripts/verify-shared-sdk.py",
        ]:
            with self.subTest(path=path):
                self.assertTrue(all(selector.select([path]).values()))

    def test_unrelated_changes_skip_all_work(self):
        self.assertFalse(
            any(selector.select(["README.md", "apps/desktop/gtk/main.vala"]).values())
        )

    def test_manual_scopes(self):
        for scope in ["core", "daemon", "kit", "python"]:
            chosen = selector.select([], scope)
            self.assertEqual([c for c in selector.PATTERNS if chosen[c]], [scope])
        self.assertTrue(all(selector.select([], "all").values()))
        self.assertFalse(any(selector.select(["core/CMakeLists.txt"], "none").values()))
        with self.assertRaises(ValueError):
            selector.select([], "unknown")


class ResultTests(unittest.TestCase):
    def status(self, worker="success", sdk="success", select="success"):
        return {
            "select": {"result": select},
            "core-sdk": {"result": sdk},
            "core": {"result": worker},
            "daemon": {"result": worker},
        }

    def test_success_and_legitimately_skipped_work(self):
        results.check("core", True, True, self.status())
        results.check("core", False, False, self.status("skipped", "skipped"))
        results.check("daemon", False, True, self.status("skipped", "success"))

    def test_failed_selection_cannot_turn_skips_into_success(self):
        with self.assertRaisesRegex(ValueError, "selection failed"):
            results.check(
                "core", False, False, self.status("skipped", "skipped", "failure")
            )

    def test_shared_failure_is_reported_at_core_origin(self):
        for status in ["failure", "cancelled", "skipped"]:
            with (
                self.subTest(status=status),
                self.assertRaisesRegex(ValueError, "Shared core SDK"),
            ):
                results.check("core", True, True, self.status("success", status))

    def test_consumer_cannot_pass_after_failed_prerequisite(self):
        with self.assertRaisesRegex(ValueError, "Blocked by"):
            results.check("daemon", True, True, self.status("skipped", "failure"))

    def test_selected_failure_and_unexpected_work_fail(self):
        for selected, status in [
            (True, "failure"),
            (True, "skipped"),
            (False, "success"),
        ]:
            with (
                self.subTest(selected=selected, status=status),
                self.assertRaises(ValueError),
            ):
                results.check("daemon", selected, True, self.status(status))

    def test_python_requires_only_its_linux_sdk(self):
        values = {
            "select": {"result": "success"},
            "sdk-linux": {"result": "success"},
            "python": {"result": "success"},
        }
        results.check("python", True, True, values, "sdk-linux")


class ProvenanceTests(unittest.TestCase):
    def selection(self):
        return {
            "source": "artifact",
            "commit": "commit",
            "core_tree": "tree",
            "assets": [
                {
                    "platform": platform,
                    "architecture": arch,
                    "build_type": "RelWithDebInfo",
                }
                for platform, arch in shared.ARCHITECTURES.items()
            ],
        }

    def test_valid_platform_subset(self):
        shared.verify(self.selection(), "commit", "tree", "RelWithDebInfo", ["linux"])
        shared.verify(
            self.selection(),
            "commit",
            "tree",
            "RelWithDebInfo",
            list(shared.ARCHITECTURES),
        )

    def test_wrong_source_revision_tree_and_configuration_are_rejected(self):
        for field, value in [
            ("source", "release"),
            ("commit", "other"),
            ("core_tree", "other"),
        ]:
            selection = self.selection()
            selection[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                shared.verify(selection, "commit", "tree", "RelWithDebInfo", ["linux"])
        with self.assertRaisesRegex(ValueError, "No unique"):
            shared.verify(self.selection(), "commit", "tree", "Release", ["linux"])

    def test_missing_duplicate_and_wrong_architecture_are_rejected(self):
        for kind in ["missing", "duplicate", "architecture"]:
            selection = self.selection()
            if kind == "missing":
                selection["assets"] = []
            elif kind == "duplicate":
                selection["assets"] += [selection["assets"][0]]
            else:
                selection["assets"][0]["architecture"] = "arm64"
            with (
                self.subTest(kind=kind),
                self.assertRaisesRegex(ValueError, "No unique"),
            ):
                shared.verify(selection, "commit", "tree", "RelWithDebInfo", ["linux"])


if __name__ == "__main__":
    unittest.main()

# Failure probe selects every consumer so dependency skipping is visible.
