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
    typed = [(field["name"], *_field_type(field, aliases, allow_literal=field["name"] == "privacy_mode"))
             for field in fields]
    header = "// Generated from contract/interface/projects.py, maintenance.py and app.py by holder_contract.\n" \
             "// Do not edit; run make -C contract update.\n"
    kotlin = header + "package team.holder.android.generated\n\n" + \
        _record_kotlin("ContractProjectListItem", typed) + "\n" + \
        _record_kotlin("ContractProject", project_typed) + \
        "\ndata class ContractEncryptionCheck(\n" \
        "    val projectId: String,\n    val privacyMode: String,\n    val ok: Boolean,\n" \
        "    val checkedFiles: Int,\n    val unsafeFiles: Int,\n" \
        "    val unsafePaths: List<String>,\n    val message: String,\n)\n" + \
        "\nobject ContractProjects {\n" \
        "    external fun nativeListProjectsPage(contextHandle: Long, offset: Int): List<ContractProjectListItem>\n" \
        "    external fun nativeCreateProject(contextHandle: Long, name: String, privacyMode: String?): ContractProject\n" \
        "    external fun nativeRenameProject(contextHandle: Long, projectId: String, name: String): ContractProject\n" \
        "    external fun nativeUpdateProjectGitRemote(contextHandle: Long, projectId: String, remoteUrl: String?): ContractProject\n" \
        "    external fun nativeCheckProjectEncryption(contextHandle: Long, projectId: String): ContractEncryptionCheck\n" \
        "    external fun nativeDeleteProject(contextHandle: Long, projectId: String)\n" \
        "}\n"
    descriptor = "(" + "".join(signature for _, _, signature, _ in typed) + ")V"
    record_lines, arguments, refs = _record_jni_lines(typed, "project", {"privacy_mode": "mode"})
    project_descriptor = "(" + "".join(signature for _, _, signature, _ in project_typed) + ")V"
    project_lines, project_args, project_refs = _record_jni_lines(project_typed, "project", {"privacy_mode": "mode"}, indent="  ")
    mode_lines = "".join(
        f"      case HOLDER_PROJECT_PRIVACY_MODE_{value.upper()}: return \"{value}\";\n"
        for value in mode["values"])
    template = Template(Path(__file__).with_name("android_projects.cpp.in").read_text())
    jni = header + template.substitute(
        mode_cases=mode_lines,
        check_mode_cases="".join(
            f"      case HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_{value.upper()}: return \"{value}\";\n"
            for value in mode["values"]),
        item_constructor=f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{descriptor}");\n',
        record_lines="".join(record_lines),
        item_construction=f"    jobject item = env->NewObject(item_type, item_ctor, {', '.join(arguments)});\n",
        release_refs="".join(f"    if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs),
        project_constructor=f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{project_descriptor}");\n',
        project_record_lines="".join(project_lines),
        project_construction=f"  jobject item = env->NewObject(item_type, item_ctor, {', '.join(project_args)});\n",
        project_release_refs="".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in project_refs),
    )
    return {PROJECT_KOTLIN_PATH: kotlin, PROJECT_JNI_PATH: jni}
