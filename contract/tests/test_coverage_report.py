"""The inventory stays aligned with the compiler and reports actual test outcomes."""

from pathlib import Path

import pytest

from compiler.ir import ContractView
from compiler.pipeline import Compiler, ContractSource
from test_support.coverage_report import Evidence, inventory, read_results, render

ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture(scope="module")
def contract():
    return Compiler().compile(ContractSource(ROOT / "contract", "interface"))


def test_every_public_transport_has_separate_schema_and_behavior_dispositions(contract):
    rows = {(row.operation, row.transport): row for row in inventory(contract)}
    assert set(rows) == ({(op.name, "C") for op in contract.operations if not op.daemon_only}
                         | {(binding.operation, "HTTP") for binding in contract.http})
    for row in rows.values():
        assert row.schema or row.schema_gap
        assert row.behavior or row.behavior_gap
        for item in (*row.schema, *row.behavior):
            assert (ROOT / item.source).is_file()
    assert ("register_ssh_signer", "HTTP") not in rows
    assert ("put_location_binding", "C") not in rows
    assert not rows["stream_changes", "HTTP"].schema
    assert "stays open" in rows["stream_changes", "HTTP"].schema_gap
    assert rows["stream_changes", "HTTP"].behavior
    assert "cursor must have been issued" in rows["list_cards", "HTTP"].schema_gap


def test_stale_and_new_operations_require_review(contract):
    raw = contract.model_dump(exclude_unset=True)
    raw["operations"][0]["name"] = "new_operation"
    with pytest.raises(ValueError, match=r"missing=\['new_operation'\], stale=\['version'\]"):
        inventory(ContractView.model_validate(raw))


def test_report_distinguishes_live_pass_skip_and_unobserved_checks(contract, tmp_path):
    junit = tmp_path / "report.xml"
    junit.write_text("""<testsuites><testsuite>
      <testcase classname="tests.conformance.test_cards.TestCardsThroughTheCApi" name="runTest">
        <properties><property name="contract-check" value="model-result:create_card"/></properties>
      </testcase>
      <testcase classname="tests.conformance.test_cards.TestCardsOverHttp" name="runTest"><skipped/></testcase>
      <testcase classname="tests.conformance.test_schemathesis" name="test_the_daemon_answers_as_its_document_says">
        <properties><property name="contract-case"
          value="test_the_daemon_answers_as_its_document_says[PUT /locations/{location_id}/binding]"/></properties>
        <failure/>
      </testcase>
    </testsuite></testsuites>""")
    results = read_results([junit])
    rows = {(row.operation, row.transport): row for row in inventory(contract)}
    assert "1 passed, check observed" in rows["create_card", "C"].schema[0].display(results, "")
    assert "check not observed" in rows["get_card", "C"].schema[0].display(results, "")
    assert "(1 skipped)" in rows["create_card", "HTTP"].behavior[0].display(results, "")
    assert "(1 failure)" in rows["put_location_binding", "HTTP"].schema[0].display(results, "")
    assert "execution unreported" in rows["stream_changes", "HTTP"].behavior[0].display(results, "")
    assert "(unreported)" in Evidence("test.py", "evidence", "absent", "test_case").display(results, "")
    assert "1 failure, 1 passed, 1 skipped" in render(list(rows.values()), results, "../")
