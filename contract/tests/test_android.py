"""The Android listing is generated from the same fields as the C API."""

from copy import deepcopy
from pathlib import Path

import pytest

from holder_contract.android import JNI_PATH, KOTLIN_PATH, PROJECT_JNI_PATH, PROJECT_KOTLIN_PATH, generate_android
from holder_contract.extract import extract


def test_card_field_change_updates_kotlin_and_jni():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    changed = deepcopy(ir)
    item = next(record for record in changed["records"] if record["name"] == "CardListItem")
    item["fields"].append({"name": "new_count", "type": {"kind": "int32"}})
    item["fields"].append({"name": "new_score", "type": {"kind": "nullable", "value": {"kind": "float64"}}})

    before = generate_android(ir)
    assert "HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC" in before[JNI_PATH]
    after = generate_android(changed)
    assert before != after
    assert "val newCount: Int" in after[KOTLIN_PATH]
    assert "card.new_count" in after[JNI_PATH]
    assert "val newScore: Double?" in after[KOTLIN_PATH]
    assert "boxed_double(env, *card.new_score)" in after[JNI_PATH]


def test_card_list_return_change_requires_generator_update():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    changed = deepcopy(ir)
    operation = next(op for op in changed["operations"] if op["name"] == "list_cards")
    operation["returns"] = {"kind": "string"}
    with pytest.raises(ValueError, match="Page\\[CardListItem\\]"):
        generate_android(changed)


def test_card_metadata_change_updates_kotlin_and_jni():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    changed = deepcopy(ir)
    metadata = next(record for record in changed["records"] if record["name"] == "CardMetadata")
    metadata["fields"].append({"name": "new_rank", "type": {"kind": "int32"}})

    after = generate_android(changed)
    assert "val newRank: Int" in after[KOTLIN_PATH]
    assert "card.new_rank" in after[JNI_PATH]
    assert "holder_get_card_metadata" in after[JNI_PATH]


def test_create_card_uses_new_card_and_rejects_unmapped_inputs():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    assert "nativeCreateCard" in generated[KOTLIN_PATH]
    assert "holder_new_card request{}" in generated[JNI_PATH]
    assert "holder_create_card(" in generated[JNI_PATH]
    assert "GetStringChars" in generated[JNI_PATH]

    changed = deepcopy(ir)
    new_card = next(record for record in changed["records"] if record["name"] == "NewCard")
    new_card["fields"].append({"name": "new_option", "type": {"kind": "string"}})
    with pytest.raises(ValueError, match="NewCard fields need a generator update"):
        generate_android(changed)


def test_project_fields_and_privacy_literals_update_kotlin_and_jni():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    changed = deepcopy(ir)
    item = next(record for record in changed["records"] if record["name"] == "ProjectListItem")
    item["fields"].append({"name": "new_count", "type": {"kind": "int32"}, "omittable": True})
    mode = next(alias for alias in changed["aliases"] if alias["name"] == "PrivacyMode")
    mode["type"]["values"].append("new_mode")

    before = generate_android(ir)
    assert "HOLDER_LIST_PROJECTS_ORDER_UPDATED_AT_DESC" in before[PROJECT_JNI_PATH]
    after = generate_android(changed)
    assert "val newCount: Int?" in after[PROJECT_KOTLIN_PATH]
    assert "boxed_int(env, *project.new_count)" in after[PROJECT_JNI_PATH]
    assert 'HOLDER_PROJECT_PRIVACY_MODE_NEW_MODE: return "new_mode"' in after[PROJECT_JNI_PATH]


def test_project_list_return_change_requires_generator_update():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    changed = deepcopy(ir)
    operation = next(op for op in changed["operations"] if op["name"] == "list_projects")
    operation["returns"] = {"kind": "string"}
    with pytest.raises(ValueError, match="list\\[ProjectListItem\\]"):
        generate_android(changed)


def test_unmapped_literal_fails_before_emitting_invalid_jni():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    changed = deepcopy(ir)
    item = next(record for record in changed["records"] if record["name"] == "CardListItem")
    item["fields"].append({"name": "new_state", "type": {"kind": "literal", "values": ["ready"]}})
    with pytest.raises(ValueError, match="C enum mapping for new_state"):
        generate_android(changed)
