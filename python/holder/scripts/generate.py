"""Regenerate the low-level package from the checked-in daemon contract."""

import argparse
import hashlib
import importlib.metadata
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT.parents[1] / "api/openapi.yaml"
TARGET = ROOT / "src/holder/generated"
VERSIONS = {"openapi-python-client": "0.29.1", "ruff": "0.16.10"}


def files(directory: Path) -> dict[str, bytes]:
    return {
        str(path.relative_to(directory)): path.read_bytes()
        for path in directory.rglob("*")
        if path.is_file() and "__pycache__" not in path.parts
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check", action="store_true", help="Compare without writing files"
    )
    args = parser.parse_args()
    try:
        data = SCHEMA.read_bytes()
    except FileNotFoundError:
        parser.error(
            f"Owning contract not found at {SCHEMA}; run from a Holder repository checkout"
        )

    provenance_path = ROOT / "openapi/provenance.json"
    provenance = {
        "source": "api/openapi.yaml",
        "sha256": hashlib.sha256(data).hexdigest(),
        "generator": VERSIONS,
    }
    if args.check and (
        not provenance_path.exists()
        or json.loads(provenance_path.read_text()) != provenance
    ):
        parser.error(
            "The owning api/openapi.yaml or generator versions changed; "
            "regenerate the Python client"
        )

    for package, version in VERSIONS.items():
        if importlib.metadata.version(package) != version:
            parser.error(f"Install {package}=={version} before regenerating")

    # Use the generator and Ruff installed beside this Python, independent of shell activation.
    env = os.environ.copy()
    env["PATH"] = str(Path(sys.executable).parent) + os.pathsep + env.get("PATH", "")
    with tempfile.TemporaryDirectory(prefix="holder-python-generate-") as temporary:
        output = Path(temporary) / "generated"
        subprocess.run(
            [
                sys.executable,
                "-m",
                "openapi_python_client",
                "generate",
                "--path",
                str(SCHEMA),
                "--config",
                str(ROOT / "openapi/generator.yaml"),
                "--meta",
                "none",
                "--output-path",
                str(output),
                "--fail-on-warning",
            ],
            env=env,
            check=True,
            cwd=temporary,
        )
        # Generator's metadata-free output still includes a Ruff cache.
        generated = {
            name: content
            for name, content in files(output).items()
            if Path(name).suffix == ".py" or name == "py.typed"
        }
        if args.check:
            existing = files(TARGET)
            changed = sorted(
                name
                for name in existing.keys() | generated.keys()
                if existing.get(name) != generated.get(name)
            )
            if changed:
                print(
                    "Generated client differs:\n" + "\n".join(changed), file=sys.stderr
                )
                return 1
            print("Generated client is up to date.")
            return 0

        # Only replace the dedicated generated directory after successful generation.
        if TARGET.exists():
            shutil.rmtree(TARGET)
        for name, content in generated.items():
            path = TARGET / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
        provenance_path.write_text(json.dumps(provenance, indent=2) + "\n")
        print(f"Generated {len(generated)} files in {TARGET.relative_to(ROOT)}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
