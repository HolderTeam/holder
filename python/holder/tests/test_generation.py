"""Generation checks must detect changes in the owning contract."""

import subprocess
import sys
from pathlib import Path

import pytest

from scripts import generate

ROOT = Path(__file__).resolve().parents[1]


def test_check_rejects_owning_contract_drift_without_writing(
    tmp_path, monkeypatch, capsys
):
    provenance = ROOT / "openapi/provenance.json"
    before = provenance.read_bytes()
    changed_contract = tmp_path / "openapi.yaml"
    changed_contract.write_bytes(
        generate.SCHEMA.read_bytes() + b"\n# Changed owning contract\n"
    )
    monkeypatch.setattr(generate, "SCHEMA", changed_contract)
    monkeypatch.setattr(sys, "argv", ["generate.py", "--check"])

    with pytest.raises(SystemExit) as error:
        generate.main()

    assert error.value.code == 2
    assert (
        "owning api/openapi.yaml or generator versions changed"
        in capsys.readouterr().err
    )
    assert provenance.read_bytes() == before


def test_check_resolves_owning_contract_from_script_location(tmp_path):
    result = subprocess.run(
        [sys.executable, str(ROOT / "scripts/generate.py"), "--check"],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        check=False,
    )

    assert result.returncode == 0, result.stderr
    assert "Generated client is up to date." in result.stdout
