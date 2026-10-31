"""Generate Android's typed project listing from the contract and its C ABI."""

from __future__ import annotations

from pathlib import Path
from string import Template
from typing import Any

from .android import PROJECT_JNI_PATH, PROJECT_KOTLIN_PATH, _field_type, _record_jni_lines, _record_kotlin


def generate_projects(ir: dict[str, Any], records: dict[str, dict[str, Any]],
                      aliases: dict[str, dict[str, Any]]) -> dict[str, str]:
    operation = next(op for op in ir["operations"] if op["name"] == "list_projects")
    if [p["name"] for p in operation["parameters"]] != [
        "name", "updated_after", "updated_before", "order", "limit", "offset", "include_card_counts"
    ] or operation["parameters"][4]["type"].get("le") != 1000 or \
            "updated_at_desc" not in operation["parameters"][3]["type"]["value"]["values"]:
        raise ValueError("Android list_projects parameters need a generator update")
    if operation["returns"] != {"kind": "list", "items": {"kind": "named", "name": "ProjectListItem"}}:
        raise ValueError("Android list_projects needs list[ProjectListItem]")
    mode = aliases["PrivacyMode"]
    if mode["kind"] != "literal" or not all(isinstance(v, str) for v in mode["values"]):
        raise ValueError("Android project privacy mode needs string literals")
    fields = records["Project"]["fields"] + records["ProjectListItem"]["fields"]
    project_fields = records["Project"]["fields"]
    operations = {op["name"]: op for op in ir["operations"]}
    if [p["name"] for p in operations["get_project"]["parameters"]] != ["project_id"] or \
            operations["get_project"]["returns"] != {"kind": "named", "name": "Project"}:
        raise ValueError("Android get_project needs a Project result")
    for name, parameters in {"create_project": ["project"], "update_project": ["project_id", "update"],
                             "delete_project": ["project_id"]}.items():
        op = operations[name]
        if [p["name"] for p in op["parameters"]] != parameters or op["returns"] != {"kind": "named", "name": "Project"}:
            raise ValueError(f"Android {name} parameters or return need a generator update")
    if [f["name"] for f in records["NewProject"]["fields"]] != [
        "project_id", "name", "root_path", "git_remote_url", "git_provider", "privacy_mode", "created_at", "updated_at"
    ] or operations["create_project"]["parameters"][0]["type"] != {"kind": "named", "name": "NewProject"} or \
            records["NewProject"]["fields"][5].get("default") != "encrypted_git":
        raise ValueError("Android NewProject fields need a generator update")
    project_typed = [(field["name"], *_field_type(field, aliases,
        allow_literal=field["name"] == "privacy_mode")) for field in project_fields]
    if [f["name"] for f in records["ProjectUpdate"]["fields"]] != ["name", "git_remote_url", "git_provider"] or \
            operations["update_project"]["parameters"][1]["type"] != {"kind": "named", "name": "ProjectUpdate"}:
        raise ValueError("Android ProjectUpdate fields need a generator update")
    remote_field = records["ProjectUpdate"]["fields"][1]
    if not remote_field.get("omittable") or remote_field["type"] != {
        "kind": "nullable", "value": {"kind": "named", "name": "GitRemoteUrl"}
    }:
        raise ValueError("Android project remote update needs nullable, omittable GitRemoteUrl")
    encryption = operations["check_project_encryption"]
    if [p["name"] for p in encryption["parameters"]] != ["project_id"] or encryption["returns"] != {
        "kind": "named", "name": "ProjectEncryptionCheck"
    }:
        raise ValueError("Android check_project_encryption parameters or return need a generator update")
    check_fields = records["ProjectEncryptionCheck"]["fields"]
    safety_fields = records["EncryptionSafetyCheck"]["fields"]
    if [(f["name"], f["type"]) for f in check_fields] != [
        ("project_id", {"kind": "named", "name": "ProjectId"}),
        ("privacy_mode", {"kind": "named", "name": "PrivacyMode"}),
        ("check", {"kind": "named", "name": "EncryptionSafetyCheck"}),
    ] or [(f["name"], f["type"]) for f in safety_fields] != [
        ("ok", {"kind": "bool"}),
        ("checked_files", {"kind": "int32", "ge": 0}),
        ("unsafe_files", {"kind": "int32", "ge": 0}),
        ("unsafe_paths", {"kind": "list", "items": {"kind": "string"}}),
        ("message", {"kind": "string"}),
    ]:
        raise ValueError("Android encryption check fields need a generator update")
    sync = operations["get_project_sync_state"]
    if [p["name"] for p in sync["parameters"]] != ["project_id"] or sync["returns"] != {
        "kind": "named", "name": "SyncState"
    }:
        raise ValueError("Android get_project_sync_state parameters or return need a generator update")
    sync_typed = [(field["name"], *_field_type(field, aliases)) for field in records["SyncState"]["fields"]]
    pull = operations["pull_project"]
    pull_fields = records["PullPhase"]["fields"]
    if [p["name"] for p in pull["parameters"]] != ["project_id"] or pull["returns"] != {
        "kind": "named", "name": "SyncResult"
    } or not any(f["name"] == "pull" and f["type"] == {"kind": "named", "name": "PullPhase"}
                 for f in records["SyncResult"]["fields"]) or [(f["name"], f["type"]) for f in pull_fields] != [
        ("attempted", {"kind": "bool"}),
        ("status", {"kind": "literal", "values": ["not_attempted", "succeeded", "remote_unset", "failed"]}),
        ("conflicts_resolved", {"kind": "int32"}),
        ("hidden_cards_repaired", {"kind": "int32"}),
        ("error_message", {"kind": "nullable", "value": {"kind": "string"}}),
    ]:
        raise ValueError("Android pull_project or PullPhase needs a generator update")
    for name, parameters in {
        "push_project": ["project_id", "options"],
        "sync_project": ["project_id", "options"],
        "test_project_remote": ["project_id", "remote"],
    }.items():
        op = operations[name]
        expected = "RemoteTestResult" if name == "test_project_remote" else "SyncResult"
        if [p["name"] for p in op["parameters"]] != parameters or op["returns"] != {
            "kind": "named", "name": expected
        }:
            raise ValueError(f"Android {name} parameters or return need a generator update")
    push_fields = records["PushPhase"]["fields"]
    remote_test_fields = records["RemoteTestResult"]["fields"]
    if [field["name"] for field in push_fields] != [
        "attempted", "status", "ahead_count", "behind_count", "local_head_commit", "error_message"
    ] or [field["name"] for field in remote_test_fields] != [
        "project_id", "status", "remote_url", "remote_has_head", "error_message"
    ] or [field["name"] for field in records["PushOptions"]["fields"]] != ["branch", "set_upstream"] or \
            [field["name"] for field in records["RemoteTest"]["fields"]] != ["remote_url"]:
        raise ValueError("Android project push or remote test fields need a generator update")
    if [field["name"] for field in records["SyncResult"]["fields"]] != [
        "project_id", "status", "remote_url", "pull", "push"
    ]:
        raise ValueError("Android SyncResult fields need a generator update")
    typed = [(field["name"], *_field_type(field, aliases, allow_literal=field["name"] == "privacy_mode"))
             for field in fields]
    header = "// Generated from contract/interface/projects.py, maintenance.py, git.py and app.py by holder_contract.\n" \
             "// Do not edit; run make -C contract update.\n"
    kotlin = header + "package team.holder.android.generated\n\n" + \
        _record_kotlin("ContractProjectListItem", typed) + "\n" + \
        _record_kotlin("ContractProject", project_typed) + \
        "\ndata class ContractEncryptionCheck(\n" \
        "    val projectId: String,\n    val privacyMode: String,\n    val ok: Boolean,\n" \
        "    val checkedFiles: Int,\n    val unsafeFiles: Int,\n" \
        "    val unsafePaths: List<String>,\n    val message: String,\n)\n" + \
        "\n" + _record_kotlin("ContractSyncState", sync_typed) + \
        "\ndata class ContractPullPhase(\n    val attempted: Boolean,\n    val status: String,\n" \
        "    val conflictsResolved: Int,\n    val hiddenCardsRepaired: Int,\n" \
        "    val errorMessage: String?,\n)\n" + \
        "\ndata class ContractPushPhase(\n    val status: String,\n    val aheadCount: Int,\n" \
        "    val behindCount: Int,\n    val errorMessage: String?,\n)\n" + \
        "\ndata class ContractRemoteTestResult(\n    val status: String,\n" \
        "    val remoteHasHead: Boolean,\n    val errorMessage: String?,\n)\n" + \
        "\ndata class ContractSyncNowResult(\n    val pullAttempted: Boolean,\n" \
        "    val pullStatus: String?,\n    val pullError: String?,\n    val pullConflictsResolved: Int,\n" \
        "    val pushAttempted: Boolean,\n    val pushStatus: String?,\n    val pushError: String?,\n" \
        "    val pushAheadCount: Int,\n    val pushBehindCount: Int,\n" \
        "    val pushLocalHeadCommit: String?,\n)\n" + \
        "\nobject ContractProjects {\n" \
        "    external fun nativeListProjectsPage(contextHandle: Long, offset: Int): List<ContractProjectListItem>\n" \
        "    external fun nativeGetProject(contextHandle: Long, projectId: String): ContractProject\n" \
        "    external fun nativeCreateProject(contextHandle: Long, name: String, privacyMode: String?): ContractProject\n" \
        "    external fun nativeRenameProject(contextHandle: Long, projectId: String, name: String): ContractProject\n" \
        "    external fun nativeUpdateProjectGitRemote(contextHandle: Long, projectId: String, remoteUrl: String?): ContractProject\n" \
        "    external fun nativeCheckProjectEncryption(contextHandle: Long, projectId: String): ContractEncryptionCheck\n" \
        "    external fun nativeGetProjectSyncState(contextHandle: Long, projectId: String): ContractSyncState\n" \
        "    external fun nativePullProject(contextHandle: Long, projectId: String): ContractPullPhase\n" \
        "    external fun nativePushProject(contextHandle: Long, projectId: String, branch: String?, setUpstream: Boolean): ContractPushPhase\n" \
        "    external fun nativeTestProjectRemote(contextHandle: Long, projectId: String, remoteUrl: String?): ContractRemoteTestResult\n" \
        "    external fun nativeSyncProject(contextHandle: Long, projectId: String, branch: String?, push: Boolean, setUpstream: Boolean): ContractSyncNowResult\n" \
        "    external fun nativeDeleteProject(contextHandle: Long, projectId: String)\n" \
        "}\n"
    descriptor = "(" + "".join(signature for _, _, signature, _ in typed) + ")V"
    record_lines, arguments, refs = _record_jni_lines(typed, "project", {"privacy_mode": "mode"})
    project_descriptor = "(" + "".join(signature for _, _, signature, _ in project_typed) + ")V"
    project_lines, project_args, project_refs = _record_jni_lines(project_typed, "project", {"privacy_mode": "mode"}, indent="  ")
    sync_descriptor = "(" + "".join(signature for _, _, signature, _ in sync_typed) + ")V"
    sync_lines, sync_args, sync_refs = _record_jni_lines(sync_typed, "value", indent="  ")
    mode_lines = "".join(
        f"      case HOLDER_PROJECT_PRIVACY_MODE_{value.upper()}: return \"{value}\";\n"
        for value in mode["values"])
    template = Template(Path(__file__).with_name("android_projects.cpp.in").read_text())
    jni = header + template.substitute(
        mode_cases=mode_lines,
        check_mode_cases="".join(
            f"      case HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_{value.upper()}: return \"{value}\";\n"
            for value in mode["values"]),
        pull_status_cases="".join(
            f"      case HOLDER_PULL_PHASE_STATUS_{value.upper()}: return \"{value}\";\n"
            for value in pull_fields[1]["type"]["values"]),
        push_status_cases="".join(
            f"      case HOLDER_PUSH_PHASE_STATUS_{value.upper()}: return \"{value}\";\n"
            for value in aliases["PushStatus"]["values"]),
        remote_test_status_cases="".join(
            f"      case HOLDER_REMOTE_TEST_RESULT_STATUS_{value.upper()}: return \"{value}\";\n"
            for value in remote_test_fields[1]["type"]["values"]),
        item_constructor=f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{descriptor}");\n',
        record_lines="".join(record_lines),
        item_construction=f"    jobject item = env->NewObject(item_type, item_ctor, {', '.join(arguments)});\n",
        release_refs="".join(f"    if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs),
        project_constructor=f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{project_descriptor}");\n',
        project_record_lines="".join(project_lines),
        project_construction=f"  jobject item = env->NewObject(item_type, item_ctor, {', '.join(project_args)});\n",
        project_release_refs="".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in project_refs),
        sync_constructor=f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{sync_descriptor}");\n',
        sync_record_lines="".join(sync_lines),
        sync_construction=f"  jobject item = env->NewObject(item_type, item_ctor, {', '.join(sync_args)});\n",
        sync_release_refs="".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in sync_refs),
    )
    return {PROJECT_KOTLIN_PATH: kotlin, PROJECT_JNI_PATH: jni}
