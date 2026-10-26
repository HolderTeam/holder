"""The extractor on the sample contract, which uses every part of the vocabulary.

tests/fixtures/sample.ir.json is the expected IR. After an intended change to the IR, rewrite it with
    python -m holder_contract ir --root tests/fixtures --package sample --output tests/fixtures/sample.ir.json
and review the difference.
"""

import json
from pathlib import Path

from holder_contract.__main__ import main
from holder_contract.extract import extract

CONTRACT = Path(__file__).resolve().parents[1]
FIXTURES = CONTRACT / "tests" / "fixtures"


def test_sample_contract_gives_the_expected_ir():
    expected = json.loads((FIXTURES / "sample.ir.json").read_text(encoding="utf-8"))
    assert extract(FIXTURES, "sample") == expected


def test_the_ir_is_the_same_on_every_run():
    first = json.dumps(extract(FIXTURES, "sample"))
    assert json.dumps(extract(FIXTURES, "sample")) == first


def test_holders_contract_is_valid():
    ir = extract(CONTRACT, "interface")
    assert ir["ir_version"] == 1


def test_check_command_reports_success(capsys):
    assert main(["check"]) == 0
    assert capsys.readouterr().err == ""


def test_ir_command_writes_json(tmp_path):
    output = tmp_path / "ir.json"
    assert main(["ir", "--root", str(FIXTURES), "--package", "sample", "--output", str(output)]) == 0
    assert json.loads(output.read_text(encoding="utf-8")) == extract(FIXTURES, "sample")


def test_partial_update_fields_keep_omitted_and_null_apart():
    patch = next(r for r in extract(FIXTURES, "sample")["records"] if r["name"] == "MessagePatch")
    fields = {f["name"]: f for f in patch["fields"]}
    assert fields["role"] == {"name": "role", "doc": "Omit to leave it unchanged.",
                              "type": {"kind": "named", "name": "Role"}, "omittable": True}
    assert fields["provider"]["type"] == {"kind": "nullable", "value": {"kind": "string"}}
    assert fields["provider"]["omittable"] is True


def test_http_bindings_place_each_parameter():
    bindings = {b["operation"]: b for b in extract(FIXTURES, "sample")["http"]}
    assert bindings["list_cards"]["path_parameters"] == ["project_id"]
    assert bindings["list_cards"]["query"] == ["parent", "limit", "cursor"]
    assert "body" not in bindings["list_cards"]
    assert bindings["update_message"]["body"] == "patch"
    assert bindings["list_cards"]["status"] == 200
    assert bindings["run_ai"]["status"] == 201


def test_error_kinds_have_http_statuses():
    assert extract(FIXTURES, "sample")["http_errors"] == [{"error": "NotFound", "status": 404},
                                                          {"error": "Conflict", "status": 409}]
