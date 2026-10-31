"""Generate the Android adapters for scoped Card update, trash, and restore."""

from __future__ import annotations

from typing import Any

from .android import _field_type, _record_jni_lines


def _jni_write(name: str, parameters: str, required: str, decode: str, request: str, call: str,
               descriptor: str, metadata: list[tuple[str, str, str, bool]], *, returns_card: bool) -> str:
    lines, arguments, refs = _record_jni_lines(metadata, "card", indent="  ")
    result_type = "jobject" if returns_card else "void"
    early_return = "return nullptr;" if returns_card else "return;"
    strings = "std::string project, id, card_title, body;" if name == "nativeUpdateCard" else "std::string project, id;"
    result = ""
    if returns_card:
        result = (
            '  const auto& card = *value;\n'
            '  jclass item_type = env->FindClass("team/holder/android/generated/ContractCardMetadata");\n'
            f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{descriptor}");\n'
            + "".join(lines)
            + f'  jobject result = env->NewObject(item_type, item_ctor, {", ".join(arguments)});\n'
            + "".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs)
            + '  return result;\n'
        )
    return (
        f'\nextern "C" JNIEXPORT {result_type} JNICALL\n'
        f'Java_team_holder_android_generated_ContractCards_{name}(\n'
        f'    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring card_id{parameters}) {{\n'
        f'  if (!context_handle || !project_id || !card_id{required}) {{\n'
        f'    fail(env, "context, project ID, card ID and required fields are needed"); {early_return}\n  }}\n'
        f'  {strings}\n'
        '  if (!utf8(env, project_id, project) || !utf8(env, card_id, id)'
        f'{decode}) {early_return}\n'
        f'{request}'
        '  Arena arena;\n  const holder_card* value = nullptr;\n'
        '  holder_error* error = nullptr;\n'
        f'  int status = arena.value ? {call} : HOLDER_ERROR_ALLOCATION;\n'
        '  if (status != HOLDER_OK) {\n'
        f'    std::string message = error ? holder_error_message(error) : "cannot allocate card for {name}";\n'
        '    if (error) holder_error_destroy(error);\n'
        f'    fail(env, message.c_str()); {early_return}\n  }}\n'
        f'{result}'
        '}\n'
    )


def generate_card_writes(ir: dict[str, Any], records: dict[str, dict[str, Any]],
                         aliases: dict[str, dict[str, Any]], metadata: list[tuple[str, str, str, bool]],
                         descriptor: str) -> tuple[str, str]:
    operations = {op["name"]: op for op in ir["operations"]}
    expected = {
        "update_card": (["project_id", "card_id", "update"], {"kind": "named", "name": "Card"}),
        "trash_card": (["project_id", "card_id", "children"], {"kind": "named", "name": "Card"}),
        "restore_card": (["project_id", "card_id"], {"kind": "named", "name": "Card"}),
    }
    for name, (parameters, result) in expected.items():
        operation = operations[name]
        if [p["name"] for p in operation["parameters"]] != parameters or operation["returns"] != result:
            raise ValueError(f"Android {name} parameters or return need a generator update")
    update = operations["update_card"]["parameters"][2]
    if update["type"] != {"kind": "named", "name": "CardUpdate"}:
        raise ValueError("Android update_card needs CardUpdate")
    fields = records["CardUpdate"]["fields"]
    if [field["name"] for field in fields] != [
        "title", "content", "parent_card_id", "sort_key", "updated_at"
    ] or [_field_type(field, aliases) for field in fields[:2]] != [
        ("String", "Ljava/lang/String;", True), ("String", "Ljava/lang/String;", True)
    ]:
        raise ValueError("Android CardUpdate fields need a generator update")
    children = operations["trash_card"]["parameters"][2]
    if children.get("default") != "promote" or children["type"] != {
        "kind": "literal", "values": ["promote", "trash_subtree"]
    }:
        raise ValueError("Android trash_card needs promote as its default child policy")

    kotlin = (
        "    external fun nativeUpdateCard(contextHandle: Long, projectId: String, cardId: String, "
        "title: String, content: String): ContractCardMetadata\n"
        "    external fun nativeTrashCard(contextHandle: Long, projectId: String, cardId: String)\n"
        "    external fun nativeRestoreCard(contextHandle: Long, projectId: String, cardId: String): "
        "ContractCardMetadata\n"
    )
    context = 'reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle))'
    prefix = f'{context}, project.c_str(), id.c_str(), '
    jni = _jni_write(
        "nativeUpdateCard", ", jstring title, jstring content", " || !title || !content",
        " ||\n      !utf8(env, title, card_title) || !utf8(env, content, body)",
        '  holder_card_update request{};\n'
        '  request.title = card_title.empty() ? nullptr : card_title.c_str();\n'
        '  request.content = body.c_str();\n',
        f'holder_update_card({prefix}&request, arena.value, &value, &error)',
        descriptor, metadata, returns_card=True,
    )
    jni += _jni_write(
        "nativeTrashCard", "", "", "", "",
        f'holder_trash_card({prefix}HOLDER_TRASH_CARD_CHILDREN_PROMOTE, arena.value, &value, &error)',
        descriptor, metadata, returns_card=False,
    )
    jni += _jni_write(
        "nativeRestoreCard", "", "", "", "",
        f'holder_restore_card({prefix}arena.value, &value, &error)',
        descriptor, metadata, returns_card=True,
    )
    return kotlin, jni
