"""The Android listing is generated from the same fields as the C API."""

from copy import deepcopy
from pathlib import Path

import pytest
from pydantic import ValidationError

from holder_contract.android import JNI_PATH, KOTLIN_PATH, PROJECT_JNI_PATH, PROJECT_KOTLIN_PATH, generate_android
from holder_contract.android_ir import ContractView, NullableType, ScalarType
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
    new_project["fields"].append({"name": "new_option", "type": {"kind": "string"}, "omittable": True})
    expanded = generate_android(changed)
    assert "val newOption: String? = null" in expanded[PROJECT_KOTLIN_PATH]
    assert 'read_string_field(env, input, input_type, "newOption", false' in expanded[PROJECT_JNI_PATH]
    assert "request.new_option = v_new_option ? v_new_option->c_str() : nullptr" in expanded[PROJECT_JNI_PATH]

    new_project["fields"].append({"name": "required_nullable", "type": {
        "kind": "nullable", "value": {"kind": "string"}}})
    new_project["fields"].append({"name": "optional_with_value", "type": {
        "kind": "nullable", "value": {"kind": "string"}}, "default": "hello"})
    expanded = generate_android(changed)[PROJECT_KOTLIN_PATH]
    assert "val requiredNullable: String?," in expanded
    assert "val requiredNullable: String? = null" not in expanded
    assert 'val optionalWithValue: String? = "hello"' in expanded

    changed = deepcopy(ir)
    update = next(record for record in changed["records"] if record["name"] == "ProjectUpdate")
    update["fields"][1]["omittable"] = False
    with pytest.raises(ValueError, match="remote update needs nullable, omittable"):
        generate_android(changed)


def test_project_encryption_check_maps_nested_safety_fields():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    kotlin = generated[PROJECT_KOTLIN_PATH]
    jni = generated[PROJECT_JNI_PATH]
    assert "data class ContractEncryptionCheck(" in kotlin
    assert "val unsafePaths: List<String>" in kotlin
    assert "nativeCheckProjectEncryption" in kotlin
    assert "holder_check_project_encryption(" in jni
    assert "value.check.unsafe_paths.items[i]" in jni
    assert "HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_PLAIN" in jni

    changed = deepcopy(ir)
    safety = next(record for record in changed["records"] if record["name"] == "EncryptionSafetyCheck")
    safety["fields"].append({"name": "new_field", "type": {"kind": "string"}})
    with pytest.raises(ValueError, match="encryption check fields need a generator update"):
        generate_android(changed)


def test_project_recovery_export_and_import_follow_contract():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    kotlin = generated[PROJECT_KOTLIN_PATH]
    jni = generated[PROJECT_JNI_PATH]
    assert "data class ContractProjectRecoveryTokenExportResult(" in kotlin
    assert "val recoveryToken: String" in kotlin
    assert "nativeExportRecoveryToken" in kotlin
    assert "nativeImportRecoveryToken" in kotlin
    assert "data class ContractRecoveryTokenGlobalImportResult(" in kotlin
    assert "val projectCreated: Boolean" in kotlin
    assert "nativeImportRecoveryTokenGlobal" in kotlin
    assert "holder_export_project_recovery_token(" in jni
    assert "holder_import_project_recovery_token(" in jni
    assert "holder_import_recovery_token_global(" in jni
    assert "HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED" in jni

    changed = deepcopy(ir)
    result = next(record for record in changed["records"]
                  if record["name"] == "ProjectRecoveryTokenExportResult")
    result["fields"].append({"name": "new_detail", "type": {"kind": "string"}})
    expanded = generate_android(changed)
    assert "val newDetail: String" in expanded[PROJECT_KOTLIN_PATH]
    assert "value.new_detail" in expanded[PROJECT_JNI_PATH]


def test_project_sync_state_generates_nullable_fields_and_c_bridge():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    kotlin = generated[PROJECT_KOTLIN_PATH]
    jni = generated[PROJECT_JNI_PATH]
    assert "data class ContractSyncState(" in kotlin
    assert "val lastPushAt: Long?" in kotlin
    assert "val unpushedCommitsCount: Int" in kotlin
    assert "nativeGetProjectSyncState" in kotlin
    assert "holder_get_project_sync_state(" in jni
    assert "boxed_long(env, *value.last_push_at)" in jni

    changed = deepcopy(ir)
    sync = next(op for op in changed["operations"] if op["name"] == "get_project_sync_state")
    sync["returns"] = {"kind": "string"}
    with pytest.raises(ValueError, match="get_project_sync_state parameters or return"):
        generate_android(changed)


def test_project_pull_maps_contract_phase_and_guards_its_shape():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    kotlin = generated[PROJECT_KOTLIN_PATH]
    jni = generated[PROJECT_JNI_PATH]
    assert "data class ContractPullPhase(" in kotlin
    assert "nativePullProject" in kotlin
    assert "holder_pull_project(" in jni
    assert "HOLDER_PULL_PHASE_STATUS_REMOTE_UNSET" in jni
    assert "value->pull" in jni

    changed = deepcopy(ir)
    pull = next(record for record in changed["records"] if record["name"] == "PullPhase")
    pull["fields"].append({"name": "new_field", "type": {"kind": "bool"}})
    with pytest.raises(ValueError, match="PullPhase needs a generator update"):
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


def test_card_trash_purge_uses_scoped_contract_operations():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    assert "data class ContractTrashItem(" in generated[KOTLIN_PATH]
    assert "nativeListTrashedCards" in generated[KOTLIN_PATH]
    assert "holder_list_trash(" in generated[JNI_PATH]
    assert 'project.c_str(), "card"' in generated[JNI_PATH]
    assert "nativePurgeCard" in generated[KOTLIN_PATH]
    assert "holder_delete_project_trash_item(" in generated[JNI_PATH]
    assert "HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_CARD" in generated[JNI_PATH]
    assert "nativeEmptyCardTrash" in generated[KOTLIN_PATH]
    assert "holder_empty_trash(" in generated[JNI_PATH]
    assert "HOLDER_EMPTY_TRASH_TYPE_CARD" in generated[JNI_PATH]

    changed = deepcopy(ir)
    purge = next(op for op in changed["operations"] if op["name"] == "delete_project_trash_item")
    purge["returns"] = {"kind": "string"}
    with pytest.raises(ValueError, match="Android card purge needs"):
        generate_android(changed)

    changed = deepcopy(ir)
    item = next(record for record in changed["records"] if record["name"] == "TrashItem")
    item["fields"].append({"name": "new_detail", "type": {"kind": "string"}})
    expanded = generate_android(changed)
    assert "val newDetail: String" in expanded[KOTLIN_PATH]
    assert "value.new_detail" in expanded[JNI_PATH]


def test_card_reference_resolution_uses_contract_candidate_fields():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    assert "data class ContractCardReferenceCandidate(" in generated[KOTLIN_PATH]
    assert "nativeResolveCardReference" in generated[KOTLIN_PATH]
    assert "holder_resolve_card_reference(" in generated[JNI_PATH]
    assert "value.deleted_at" in generated[JNI_PATH]

    changed = deepcopy(ir)
    candidate = next(record for record in changed["records"] if record["name"] == "CardReferenceCandidate")
    candidate["fields"].append({"name": "new_detail", "type": {"kind": "string"}})
    expanded = generate_android(changed)
    assert "val newDetail: String" in expanded[KOTLIN_PATH]
    assert "value.new_detail" in expanded[JNI_PATH]


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


def test_project_sync_bridge_tracks_contract_operation():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    assert "nativeSyncProject" in generated[PROJECT_KOTLIN_PATH]
    assert "holder_sync_project(" in generated[PROJECT_JNI_PATH]
    assert "nativeSyncProjectIfDue" in generated[PROJECT_KOTLIN_PATH]
    assert "holder_sync_project_if_due(" in generated[PROJECT_JNI_PATH]
    assert "holder_pull_project(" in generated[PROJECT_JNI_PATH]

    changed = deepcopy(ir)
    operation = next(op for op in changed["operations"] if op["name"] == "sync_project")
    operation["returns"] = {"kind": "string"}
    with pytest.raises(ValueError, match="Android sync_project parameters or return"):
        generate_android(changed)

    changed = deepcopy(ir)
    operation = next(op for op in changed["operations"] if op["name"] == "sync_project_if_due")
    operation["returns"] = {"kind": "string"}
    with pytest.raises(ValueError, match="Android sync_project_if_due parameters or return"):
        generate_android(changed)


def test_android_contract_view_preserves_types_and_accepts_unmapped_result_fields():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    contract = ContractView.model_validate(ir)
    assert contract.record("PullPhase").field("error_message").type == NullableType(
        kind="nullable", value=ScalarType(kind="string"))
    assert contract.operation("list_projects").parameter("limit").type.le == 1000

    changed = deepcopy(ir)
    due = next(record for record in changed["records"] if record["name"] == "DueSyncResult")
    due["fields"].append({"name": "new_detail", "type": {"kind": "string"}})
    assert generate_android(changed) == generate_android(ir)

    due["fields"][-1]["type"] = {"kind": "unsupported_new_type"}
    with pytest.raises(ValidationError, match="unsupported_new_type"):
        generate_android(changed)


def test_card_content_bridge_tracks_get_card_contract():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    generated = generate_android(ir)
    assert "nativeGetCardContent" in generated[KOTLIN_PATH]
    assert "holder_get_card(" in generated[JNI_PATH]

    changed = deepcopy(ir)
    operation = next(op for op in changed["operations"] if op["name"] == "get_card")
    operation["returns"] = {"kind": "string"}
    with pytest.raises(ValueError, match="Android get_card needs Card content"):
        generate_android(changed)


def test_unmapped_literal_fails_before_emitting_invalid_jni():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    changed = deepcopy(ir)
    item = next(record for record in changed["records"] if record["name"] == "CardListItem")
    item["fields"].append({"name": "new_state", "type": {"kind": "literal", "values": ["ready"]}})
    with pytest.raises(ValueError, match="C enum mapping for new_state"):
        generate_android(changed)
