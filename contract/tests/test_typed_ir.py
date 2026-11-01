"""The typed IR preserves the extractor's complete contract, across generator targets."""

from copy import deepcopy
from pathlib import Path

import pytest
from pydantic import ValidationError

from compiler.ir import ContractView
from compiler.pipeline import Compiler, ContractSource
from generators.android.client import JNI_PATH as CLIENT_JNI_PATH
from generators.android.client import KOTLIN_DIR, generate_android_client
from generators.core.abi import CAbi
from generators.daemon.bindings import RuntimeBinding, bindings
from holder_contract.daemon import generate_daemon
from holder_contract.extract import extract, load_contract
from holder_contract.generate import generate

CONTRACT = Path(__file__).resolve().parents[1]
CARDS_KOTLIN = f"{KOTLIN_DIR}/Cards.kt"


def generate_android(ir):
    return generate_android_client(ContractView.model_validate(ir))


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
    assert "value.proof_count" in android[CLIENT_JNI_PATH]
    assert "val proofCount: Int" in android[CARDS_KOTLIN]


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
    assert "includeProof: Boolean = false" in android[CARDS_KOTLIN]
    assert "a_include_proof == JNI_TRUE" in android[CLIENT_JNI_PATH]


def test_operation_source_names_its_domain_module():
    contract = ContractView.model_validate(extract(CONTRACT, "interface"))
    assert contract.operation("list_cards").source == "interface/cards/operations.py"
    assert contract.operation("list_projects").source == "interface/projects/operations.py"
    assert next(alias.source for alias in contract.aliases if alias.name == "ProjectId") == "interface/projects/ids.py"
    assert contract.operation("search_cards").source == "interface/search/operations.py"


def test_c_enum_choice_is_shared_with_android_jni():
    raw = deepcopy(extract(CONTRACT, "interface"))
    operation = next(op for op in raw["operations"] if op["name"] == "list_cards")
    order = next(parameter for parameter in operation["parameters"] if parameter["name"] == "order")
    order["type"]["values"].append("proof_order")
    contract = ContractView.model_validate(raw)
    symbol = CAbi(contract).operation("list_cards").parameter("order").choices[-1].symbol

    assert symbol == "HOLDER_LIST_CARDS_ORDER_PROOF_ORDER"
    assert symbol in generate(contract.model_dump(exclude_unset=True))["holder_api.h"]
    # Kotlin's entries follow C's order, so an entry's ordinal plus one is its C value.
    kotlin = generate_android(contract.model_dump(exclude_unset=True))[CARDS_KOTLIN]
    entries = kotlin[kotlin.index("enum class ListCardsOrder"):].split("}")[0]
    assert entries.strip().splitlines()[-1].strip() == 'PROOF_ORDER("proof_order"),'


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
