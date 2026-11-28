"""Check an installed Holder Python release distribution in isolation."""

import argparse
import importlib.util
import json
import pickle
import sys
from importlib.metadata import distribution
from pathlib import Path
from tempfile import TemporaryDirectory


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--embedded", action="store_true")
    parser.add_argument("--core-build-type", default="Release")
    parser.add_argument("--repaired-windows", action="store_true")
    args = parser.parse_args()
    assert not args.repaired_windows or (args.embedded and sys.platform == "win32")

    from holder import Holder, open_workspace
    from holder.generated.api.default import get_projects

    installed = distribution("holder-python")
    assert installed.version == args.version, installed.version
    assert installed.metadata["Name"] == "holder-python"
    assert installed.metadata["License-Expression"] == "GPL-3.0-only"
    assert Holder is not None and get_projects is not None
    assert installed.locate_file("holder/py.typed").is_file()
    assert importlib.util.find_spec("holderkit") is None
    assert not {"pandas", "networkx"}.intersection(sys.modules)

    embedded = "holder._embedded._native"
    assert (importlib.util.find_spec(embedded) is not None) == args.embedded
    if args.embedded:
        from holder._embedded import _native

        assert _native.__name__ == embedded
        assert _native.Context.__module__ == _native.HolderError.__module__ == embedded
        error = pickle.loads(pickle.dumps(_native.HolderError("native error")))
        assert type(error) is _native.HolderError and str(error) == "native error"
        assert "holderkit" not in sys.modules
        assert _native.WORKSPACE_BRIDGE_VERSION == 1
        manifest = installed.locate_file("holder/_embedded/_core_build.json")
        core = json.loads(manifest.read_text(encoding="utf-8"))
        assert core["version"] == args.version.split(".post", 1)[0], core
        platform = {"linux": "linux", "darwin": "macos", "win32": "windows"}[sys.platform]
        assert core["platform"] == platform and core["build_type"] == args.core_build_type, core
        if platform == "windows":
            if args.repaired_windows:
                metadata = f"holder_python-{args.version}.dist-info"
                assert installed.locate_file(f"{metadata}/DELVEWHEEL").is_file()
                assert not installed.locate_file("holder/_embedded/.libs").exists()
                libraries = installed.locate_file("holder_python.libs")
                licenses = installed.locate_file(f"{metadata}/licenses/third-party")
                assert (licenses / "microsoft-runtime.txt").is_file()
                licenses /= "vcpkg"
            else:
                libraries = installed.locate_file("holder/_embedded/.libs")
                licenses = installed.locate_file("holder/_embedded/_licenses")
            assert any(libraries.glob("*.dll"))
            assert any(path.is_file() for path in licenses.rglob("*"))
        elif platform == "macos":
            files = installed.files or []
            assert any(path.suffix == ".dylib" for path in files)
            assert any("third-party" in path.parts for path in files)
        with TemporaryDirectory(prefix="holder-installed-wheel-") as temporary:
            private = Path(temporary) / "private"
            with open_workspace(private) as workspace:
                project = workspace.create_plain_project(name="Installed wheel")
                project_id = project["project_id"]
                card = workspace.card_operations.create_card(
                    project_id, {"title": "Release check", "content": "Saved body"}
                )
            with open_workspace(private) as reopened:
                assert reopened.get_project(project_id) == project
                assert reopened.card_operations.get_card(project_id, card["card_id"]) == card
    print(f"Installed holder-python {args.version} passed (embedded={args.embedded})")


if __name__ == "__main__":
    main()
