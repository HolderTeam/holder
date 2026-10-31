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


def test_project_writes_follow_contract_and_preserve_android_plain_default():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    kotlin = generated[PROJECT_KOTLIN_PATH]
    jni = generated[PROJECT_JNI_PATH]
    for operation in ("Create", "Rename", "Delete"):
        assert f"native{operation}Project" in kotlin
    for operation in ("create", "update", "delete"):
        assert f"holder_{operation}_project(" in jni
    assert "HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN" in jni
    assert "GetStringChars" in jni
    assert "project.project_id" in jni
    assert "nativeUpdateProjectGitRemote" in kotlin
    assert "request.git_remote_url.change = !remote_url || url.empty() ? HOLDER_CLEAR : HOLDER_SET" in jni

    changed = deepcopy(ir)
    project = next(record for record in changed["records"] if record["name"] == "Project")
    project["fields"].append({"name": "new_count", "type": {"kind": "int32"}})
    generated = generate_android(changed)
    assert "val newCount: Int" in generated[PROJECT_KOTLIN_PATH]
    assert "project.new_count" in generated[PROJECT_JNI_PATH]

    changed = deepcopy(ir)
    new_project = next(record for record in changed["records"] if record["name"] == "NewProject")
    new_project["fields"].append({"name": "new_option", "type": {"kind": "string"}})
    with pytest.raises(ValueError, match="NewProject fields need a generator update"):
        generate_android(changed)

    changed = deepcopy(ir)
    update = next(record for record in changed["records"] if record["name"] == "ProjectUpdate")
    update["fields"][1]["omittable"] = False
    with pytest.raises(ValueError, match="remote update needs nullable, omittable"):
        generate_android(changed)


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


def test_card_writes_use_scoped_contract_operations_and_guard_input_shape():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    kotlin = generated[KOTLIN_PATH]
    jni = generated[JNI_PATH]
    for name in ("Update", "Trash", "Restore"):
        assert f"native{name}Card(contextHandle: Long, projectId: String, cardId: String" in kotlin
        assert f"holder_{name.lower()}_card(" in jni
    assert "HOLDER_TRASH_CARD_CHILDREN_PROMOTE" in jni
    assert "card_title.empty() ? nullptr" in jni
    assert "request.content = body.c_str()" in jni

    changed = deepcopy(ir)
    update = next(record for record in changed["records"] if record["name"] == "CardUpdate")
    update["fields"].append({"name": "new_option", "type": {"kind": "string"}})
    with pytest.raises(ValueError, match="CardUpdate fields need a generator update"):
        generate_android(changed)

    changed = deepcopy(ir)
    trash = next(op for op in changed["operations"] if op["name"] == "trash_card")
    trash["parameters"][2]["default"] = "trash_subtree"
    with pytest.raises(ValueError, match="promote as its default"):
        generate_android(changed)


def test_record_lists_follow_contract_fields_and_operation_shapes():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    assert "nativeListProjectTags" in generated[KOTLIN_PATH]
    assert "holder_list_project_tags(" in generated[JNI_PATH]
    assert "nativeSearchCards" in generated[KOTLIN_PATH]
    assert "holder_search_cards(" in generated[JNI_PATH]

    changed = deepcopy(ir)
    search = next(record for record in changed["records"] if record["name"] == "SearchCard")
    search["fields"].append({"name": "new_score", "type": {"kind": "float64"}})
    after = generate_android(changed)
    assert "val newScore: Double" in after[KOTLIN_PATH]
    assert "value.new_score" in after[JNI_PATH]

    changed = deepcopy(ir)
    tags = next(op for op in changed["operations"] if op["name"] == "list_project_tags")
    tags["returns"] = {"kind": "string"}
    with pytest.raises(ValueError, match="Android list_project_tags parameters or return"):
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
