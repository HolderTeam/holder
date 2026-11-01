"""The typed IR preserves the extractor's complete contract, across generator targets."""

from copy import deepcopy
from pathlib import Path

import pytest
from pydantic import ValidationError

from compiler.ir import ContractView
from compiler.pipeline import Compiler, ContractSource
from generators.android.page import JNI_PATH as OPERATION_JNI_PATH
from generators.android.page import KOTLIN_PATH as OPERATION_KOTLIN_PATH
from generators.android.page import generate_page_operation
from generators.core.abi import CAbi
from holder_contract.android import generate_android
from holder_contract.daemon import generate_daemon
from holder_contract.daemon_bindings import RuntimeBinding, bindings
from holder_contract.extract import extract, load_contract
from holder_contract.generate import generate

CONTRACT = Path(__file__).resolve().parents[1]


def test_typed_ir_preserves_every_extracted_field():
    raw = extract(CONTRACT, "interface")
    typed = Compiler().compile(ContractSource(CONTRACT, "interface"))
    assert typed.model_dump(exclude_unset=True) == raw
    assert typed.record("AiStatusData").open
    assert any(binding.headers for binding in typed.http)
    assert any(binding.public for binding in typed.http)


def test_typed_ir_rejects_unknown_metadata():
    raw = extract(CONTRACT, "interface")
    raw["records"][0]["unexpected"] = True
    with pytest.raises(ValidationError, match="extra_forbidden"):
        ContractView.model_validate(raw)


def test_one_card_field_reaches_c_daemon_and_android_outputs():
    raw = deepcopy(extract(CONTRACT, "interface"))
    item = next(record for record in raw["records"] if record["name"] == "CardListItem")
    item["fields"].append({"name": "proof_count", "type": {"kind": "int32"}})
    typed = ContractView.model_validate(raw)
    common = typed.model_dump(exclude_unset=True)
    c = generate(common)
    daemon = generate_daemon(common, load_contract(CONTRACT, "interface"))
    android = generate_android(common)
    assert "proof_count" in c["holder_api.h"]
    assert "proof_count" in c["Contract.h"]
    assert "proof_count" in daemon["ContractJson.h"]
    assert "proof_count" in android[OPERATION_JNI_PATH]
    assert "val proofCount: Int" in android[OPERATION_KOTLIN_PATH]


def test_new_response_option_reaches_all_three_targets():
    raw = deepcopy(extract(CONTRACT, "interface"))
    operation = next(op for op in raw["operations"] if op["name"] == "list_cards")
    operation["parameters"].append({"name": "include_proof", "type": {"kind": "bool"}, "default": False})
    binding = next(item for item in raw["http"] if item["operation"] == "list_cards")
    binding["query"].append("include_proof")
    common = ContractView.model_validate(raw).model_dump(exclude_unset=True)
    c = generate(common)
    daemon = generate_daemon(common, load_contract(CONTRACT, "interface"))
    android = generate_android(common)
    assert "bool include_proof" in c["holder_api.h"]
    assert '"include_proof"' in daemon["ContractRoutes.h"]
    assert "includeProof: Boolean = false" in android[OPERATION_KOTLIN_PATH]
    assert "include_proof == JNI_TRUE" in android[OPERATION_JNI_PATH]


def test_page_lowering_uses_operation_data_for_a_second_operation():
    raw = deepcopy(extract(CONTRACT, "interface"))
    operation = deepcopy(next(op for op in raw["operations"] if op["name"] == "list_cards"))
    operation["name"] = "find_cards"
    raw["operations"].append(operation)
    generated = generate_page_operation(ContractView.model_validate(raw), "find_cards")
    assert "nativeFindCards(" in generated[OPERATION_KOTLIN_PATH]
    assert "holder_find_cards(" in generated[OPERATION_JNI_PATH]


def test_c_enum_choice_is_shared_with_android_jni():
    raw = deepcopy(extract(CONTRACT, "interface"))
    operation = next(op for op in raw["operations"] if op["name"] == "list_cards")
    order = next(parameter for parameter in operation["parameters"] if parameter["name"] == "order")
    order["type"]["values"].append("proof_order")
    contract = ContractView.model_validate(raw)
    symbol = CAbi(contract).operation("list_cards").parameter("order").choices[-1].symbol

    assert symbol == "HOLDER_LIST_CARDS_ORDER_PROOF_ORDER"
    assert symbol in generate(contract.model_dump(exclude_unset=True))["holder_api.h"]
    assert symbol in generate_android(contract.model_dump(exclude_unset=True))[OPERATION_JNI_PATH]


def test_daemon_only_record_flows_through_same_ir():
    raw = deepcopy(extract(CONTRACT, "interface"))
    status = next(record for record in raw["records"] if record["name"] == "AiStatusData")
    status["fields"].append({"name": "proof_text", "type": {"kind": "string"}})
    common = ContractView.model_validate(raw).model_dump(exclude_unset=True)
    daemon = generate_daemon(common, load_contract(CONTRACT, "interface"))
    assert "proof_text" in daemon["AiJson.h"]


def test_daemon_runtime_binding_is_validated_data():
    assert bindings()["get_ai_status"].services == ("db", "registry")
    with pytest.raises(ValidationError):
        RuntimeBinding(services=("unknown_service",))
