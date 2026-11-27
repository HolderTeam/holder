"""Check an installed Holder Python release distribution in isolation."""

import argparse
import importlib.util
import json
import sys
from importlib.metadata import distribution
from pathlib import Path
from tempfile import TemporaryDirectory


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--embedded", action="store_true")
    parser.add_argument("--core-build-type", default="Release")
    args = parser.parse_args()

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

        assert _native.WORKSPACE_BRIDGE_VERSION == 1
        manifest = installed.locate_file("holder/_embedded/_core_build.json")
        core = json.loads(manifest.read_text(encoding="utf-8"))
        assert core["version"] == args.version.split(".post", 1)[0], core
        platform = {"linux": "linux", "win32": "windows"}[sys.platform]
        assert core["platform"] == platform and core["build_type"] == args.core_build_type, core
        if platform == "windows":
            libraries = installed.locate_file("holder/_embedded/.libs")
            licenses = installed.locate_file("holder/_embedded/_licenses")
            assert any(libraries.glob("*.dll"))
            assert any(path.is_file() for path in licenses.rglob("*"))
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
