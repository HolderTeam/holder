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
    typed = [(field["name"], *_field_type(field, aliases, allow_literal=field["name"] == "privacy_mode"))
             for field in fields]
    header = "// Generated from contract/interface/projects.py and contract/interface/app.py by holder_contract.\n" \
             "// Do not edit; run make -C contract update.\n"
    kotlin = header + "package team.holder.android.generated\n\n" + \
        _record_kotlin("ContractProjectListItem", typed) + \
        "\nobject ContractProjects {\n" \
        "    external fun nativeListProjectsPage(contextHandle: Long, offset: Int): List<ContractProjectListItem>\n" \
        "}\n"
    descriptor = "(" + "".join(signature for _, _, signature, _ in typed) + ")V"
    record_lines, arguments, refs = _record_jni_lines(typed, "project", {"privacy_mode": "mode"})
    mode_lines = "".join(
        f"      case HOLDER_PROJECT_PRIVACY_MODE_{value.upper()}: return \"{value}\";\n"
        for value in mode["values"])
    template = Template(Path(__file__).with_name("android_projects.cpp.in").read_text())
    jni = header + template.substitute(
        mode_cases=mode_lines,
        item_constructor=f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{descriptor}");\n',
        record_lines="".join(record_lines),
        item_construction=f"    jobject item = env->NewObject(item_type, item_ctor, {', '.join(arguments)});\n",
        release_refs="".join(f"    if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs),
    )
    return {PROJECT_KOTLIN_PATH: kotlin, PROJECT_JNI_PATH: jni}
