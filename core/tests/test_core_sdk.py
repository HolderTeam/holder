"""SDK selection, cache integrity and provenance checks without network access."""

import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path, PureWindowsPath
import tarfile
import tempfile
import unittest
from unittest.mock import patch
from typing import Any, cast

spec = importlib.util.spec_from_file_location("core_sdk", Path(__file__).parents[1] / "scripts/core-sdk.py")
assert spec is not None and spec.loader is not None
sdk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sdk)
COMMIT = "a" * 40


class SDKTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.manifest = {"commit": COMMIT, "version": "0.2.0", "platform": "linux",
                         "architecture": "x86_64", "build_type": "RelWithDebInfo"}
        self.selection: dict[str, Any] = {"commit": COMMIT, "version": "0.2.0", "release_tag": f"sdk-{COMMIT}",
                          "repository": sdk.REPOSITORY, "assets": []}

    def archive(self, manifest: dict[str, str] | None = None, extra: dict[str, str] | None = None) -> bytes:
        stream = io.BytesIO()
        with tarfile.open(fileobj=stream, mode="w:gz") as package:
            files = {"libholder-sdk/libholder-manifest.json": json.dumps(manifest or self.manifest),
                     "libholder-sdk/share/holder/schema/schema.sql": "schema",
                     "libholder-sdk/share/holder/resources/WELCOME.md": "welcome",
                     "libholder-sdk/lib/cmake/holder/holderConfig.cmake": "cmake"}
            if extra:
                files.update(extra)
            for name, contents in files.items():
                data = contents.encode()
                entry = tarfile.TarInfo(name)
                entry.size = len(data)
                package.addfile(entry, io.BytesIO(data))
        data = stream.getvalue()
        name = "libholder-0.2.0-linux-x86_64-relwithdebinfo.tar.gz"
        asset = {"name": name, "platform": "linux", "architecture": "x86_64",
                 "build_type": "RelWithDebInfo", "size": len(data), "sha256": hashlib.sha256(data).hexdigest(),
                 "url": f"https://github.com/{sdk.REPOSITORY}/releases/download/sdk-{COMMIT}/{name}"}
        self.selection["assets"] = [asset]
        return data

    def fetch(self) -> Path:
        return cast(Path, sdk.fetch(self.selection, self.root, "linux", "x86_64", "RelWithDebInfo"))

    def test_cached_archive_is_rechecked_and_extracted_manifest_is_restored(self) -> None:
        data = self.archive()
        with patch.object(sdk, "read_url", return_value=data) as download:
            installed = self.fetch()
            (installed / "libholder-manifest.json").write_text("damaged")
            self.fetch()
            self.assertEqual(download.call_count, 1)
            self.assertEqual(json.loads((installed / "libholder-manifest.json").read_text()), self.manifest)

    def test_install_dir_is_a_fixed_path_that_is_replaced(self) -> None:
        data = self.archive()
        target = self.root / "fixed" / "libholder-sdk"
        with patch.object(sdk, "read_url", return_value=data):
            installed = sdk.fetch(self.selection, self.root, "linux", "x86_64", "RelWithDebInfo", install_dir=target)
            self.assertEqual(installed, target.resolve())
            (installed / "stale").write_text("from an earlier SDK")
            sdk.fetch(self.selection, self.root, "linux", "x86_64", "RelWithDebInfo", install_dir=target)
        self.assertFalse((target / "stale").exists())
        self.assertEqual(json.loads((target / "libholder-manifest.json").read_text()), self.manifest)
        self.assertNotIn(COMMIT, installed.as_posix())

    def test_corrupt_download_is_not_cached(self) -> None:
        self.archive()
        with patch.object(sdk, "read_url", return_value=b"damaged"):
            with self.assertRaisesRegex(ValueError, "checksum"):
                self.fetch()
        self.assertEqual(list(self.root.rglob("*.tar.gz")), [])

    def test_corrupt_cache_is_rejected_without_network_fallback(self) -> None:
        data = self.archive()
        with patch.object(sdk, "read_url", return_value=data):
            self.fetch()
        next(self.root.rglob("*.tar.gz")).write_bytes(b"damaged")
        with patch.object(sdk, "read_url", side_effect=AssertionError("unexpected download")):
            with self.assertRaisesRegex(ValueError, "Cached SDK checksum"):
                self.fetch()

    def test_mismatched_commit_and_configuration_are_rejected(self) -> None:
        for key, value in [("commit", "b" * 40), ("build_type", "Release")]:
            with self.subTest(key=key):
                data = self.archive({**self.manifest, key: value})
                with patch.object(sdk, "read_url", return_value=data):
                    with self.assertRaisesRegex(ValueError, f"manifest {key}"):
                        self.fetch()
                next(self.root.rglob("*.tar.gz")).unlink()

    def test_archive_cannot_escape_destination(self) -> None:
        data = self.archive(extra={"libholder-sdk/../../escaped": "unsafe"})
        with patch.object(sdk, "read_url", return_value=data):
            with self.assertRaisesRegex(ValueError, "Unsafe SDK archive"):
                self.fetch()
        self.assertFalse((self.root / "escaped").exists())

    def test_unknown_host_configuration_fails(self) -> None:
        self.archive()
        with self.assertRaisesRegex(ValueError, "No unique SDK"):
            sdk.selected_asset(self.selection, "linux", "arm64", "RelWithDebInfo")

    def test_unexpected_download_host_fails(self) -> None:
        self.archive()
        self.selection["assets"][0]["url"] = "https://example.com/archive"
        with self.assertRaisesRegex(ValueError, "Unexpected SDK download URL"):
            self.fetch()

    def test_latest_green_is_resolved_once_into_immutable_release(self) -> None:
        assets = [{"platform": system, "architecture": arch, "build_type": config}
                  for system, arch in [("linux", "x86_64"), ("macos", "arm64"), ("windows", "x86_64")]
                  for config in ["Release", "RelWithDebInfo"]]
        index = {"schema_version": 1, "repository": sdk.REPOSITORY, "commit": COMMIT,
                 "snapshot_tag": f"sdk-{COMMIT}", "version": "0.2.0", "assets": assets}
        release = {"draft": False, "assets": [{"name": "sdk-index.json", "browser_download_url": "https://example.com/index"}]}
        with patch.object(sdk, "api", side_effect=[{"object": {"sha": COMMIT, "type": "commit"}}, release]) as api:
            with patch.object(sdk, "read_url", return_value=json.dumps(index).encode()):
                result = sdk.resolve("latest-green")
        self.assertEqual(result["commit"], COMMIT)
        self.assertEqual(api.call_args_list[1].args, (f"releases/tags/sdk-{COMMIT}",))

    def test_arbitrary_branch_is_not_a_release_pin(self) -> None:
        with self.assertRaisesRegex(ValueError, "full core commit SHA"):
            sdk.resolve("main")

    def test_version_tag_is_resolved_to_commit_and_checksums(self) -> None:
        assets = [{"name": f"libholder-0.2.0-{system}-{arch}-{config}.tar.gz", "digest": "sha256:" + "a" * 64,
                   "size": 1, "browser_download_url": "https://example.com/archive"}
                  for system, arch in [("linux", "x86_64"), ("macos", "arm64"), ("windows", "x86_64")]
                  for config in ["release", "relwithdebinfo"]]
        with patch.object(sdk, "api", side_effect=[{"sha": COMMIT}, {"draft": False, "assets": assets}]):
            result = sdk.resolve("v0.2.0")
        self.assertEqual(result["release_tag"], "v0.2.0")
        self.assertEqual(result["assets"][0]["sha256"], "a" * 64)

    def test_windows_environment_uses_cmake_safe_sdk_paths(self) -> None:
        selection = self.root / "selection.json"
        selection.write_text(json.dumps(self.selection))
        environment = self.root / "github-env"
        windows_sdk = PureWindowsPath("D:/a/holder-kit/.core-sdk/libholder-sdk")
        with patch("sys.argv", ["core-sdk.py", "fetch", "--selection", str(selection), "--github-env"]):
            with patch.object(sdk, "fetch", return_value=windows_sdk), patch.object(sdk, "host", return_value=("windows", "x86_64")):
                with patch.object(sdk.platform, "system", return_value="Windows"), patch.dict(os.environ, {"GITHUB_ENV": str(environment)}):
                    sdk.main()
        lines = environment.read_text().splitlines()
        self.assertEqual(lines[0], "HOLDER_CORE_SDK=D:/a/holder-kit/.core-sdk/libholder-sdk")
        self.assertNotIn("\\", lines[1])
        self.assertIn("-DVCPKG_APPLOCAL_DEPS=OFF", lines[1])

    def test_fetch_rejects_selection_from_another_run_before_downloading(self) -> None:
        selection = self.root / "selection.json"
        selection.write_text(json.dumps(self.selection))
        with patch("sys.argv", ["core-sdk.py", "fetch", "--selection", str(selection), "--expected-commit", "b" * 40]):
            with patch.object(sdk, "fetch", side_effect=AssertionError("unexpected download")):
                with self.assertRaisesRegex(ValueError, "run-wide core commit"):
                    sdk.main()

    def write_assets(self, *manifests: dict[str, str]) -> Path:
        assets = self.root / "assets"
        assets.mkdir(exist_ok=True)
        for number, manifest in enumerate(manifests or (self.manifest,)):
            (assets / f"libholder-{number}.tar.gz").write_bytes(self.archive(manifest))
        return assets

    def test_artifact_index_describes_archives_from_this_run(self) -> None:
        assets = self.write_assets()
        selection = sdk.index(assets)
        self.assertEqual((selection["source"], selection["commit"], selection["version"]), ("artifact", COMMIT, "0.2.0"))
        installed = sdk.fetch(selection, self.root / "cache", "linux", "x86_64", "RelWithDebInfo", assets)
        self.assertEqual(json.loads((installed / "libholder-manifest.json").read_text()), self.manifest)

    def test_artifact_fetch_never_downloads(self) -> None:
        assets = self.write_assets()
        with patch.object(sdk, "read_url", side_effect=AssertionError("unexpected download")):
            sdk.fetch(sdk.index(assets), self.root / "cache", "linux", "x86_64", "RelWithDebInfo", assets)

    def test_artifact_changed_after_indexing_is_rejected(self) -> None:
        assets = self.write_assets()
        selection = sdk.index(assets)
        next(assets.iterdir()).write_bytes(b"damaged")
        with self.assertRaisesRegex(ValueError, "checksum"):
            sdk.fetch(selection, self.root / "cache", "linux", "x86_64", "RelWithDebInfo", assets)

    def test_cached_archives_of_one_tree_may_record_different_commits(self) -> None:
        # Each platform's SDK is cached by core/'s tree on its own, so a run can restore the Linux
        # SDK first built at one commit and the macOS SDK first built at another.
        macos = {**self.manifest, "commit": "b" * 40, "platform": "macos", "architecture": "arm64"}
        assets = self.write_assets(self.manifest, macos)
        selection = sdk.index(assets, "c" * 40, "d" * 40)
        self.assertEqual((selection["commit"], selection["core_tree"]), ("c" * 40, "d" * 40))
        self.assertEqual({a["platform"]: a["commit"] for a in selection["assets"]},
                         {"linux": COMMIT, "macos": "b" * 40})
        installed = sdk.fetch(selection, self.root / "cache", "linux", "x86_64", "RelWithDebInfo", assets)
        self.assertEqual(json.loads((installed / "libholder-manifest.json").read_text())["commit"], COMMIT)
        selection["assets"][0]["commit"] = "e" * 40
        with self.assertRaisesRegex(ValueError, "manifest commit"):
            sdk.fetch(selection, self.root / "other cache", "linux", "x86_64", "RelWithDebInfo", assets)

    def test_artifact_index_rejects_mixed_commits_and_duplicates(self) -> None:
        for manifests, message in [((self.manifest, {**self.manifest, "commit": "b" * 40, "platform": "macos"}), "commit"),
                                   ((self.manifest, self.manifest), "Duplicate")]:
            with self.subTest(message=message):
                with self.assertRaisesRegex(ValueError, message):
                    sdk.index(self.write_assets(*manifests))
                for archive in (self.root / "assets").iterdir():
                    archive.unlink()

    def test_windows_package_configs_use_their_own_installation(self) -> None:
        triplet = self.root / "sdk/vcpkg/installed/x64-windows"
        config = triplet / "share/unofficial-sodium/unofficial-sodiumConfig.cmake"
        config.parent.mkdir(parents=True)
        config.write_text('set(library "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/sodium.lib")\n')
        sdk.relocate_windows_packages(triplet)
        self.assertEqual(config.read_text(), 'set(library "${CMAKE_CURRENT_LIST_DIR}/../../lib/sodium.lib")\n')
        first = config.read_text()
        sdk.relocate_windows_packages(triplet)
        self.assertEqual(config.read_text(), first)


if __name__ == "__main__":
    unittest.main()
