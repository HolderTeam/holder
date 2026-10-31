"""Generate the Android adapters for scoped Card update, trash, and restore."""

from __future__ import annotations

from .android import _field_type, _record_jni_lines
from .android_ir import ContractView, NamedType
from .android_templates import TEMPLATES


def _jni_write(name: str, parameters: str, required: str, decode: str, request: str, call: str,
               descriptor: str, metadata: list[tuple[str, str, str, bool]], *, returns_card: bool) -> str:
    lines, arguments, refs = _record_jni_lines(metadata, "card", indent="  ")
    return TEMPLATES.get_template("android_card_write.cpp.j2").render(
        name=name, result_type="jobject" if returns_card else "void",
        early_return="return nullptr;" if returns_card else "return;",
        strings="std::string project, id, card_title, body;" if name == "nativeUpdateCard"
        else "std::string project, id;",
        parameters=parameters, required=required, decode=decode, request=request, call=call,
        returns_card=returns_card, descriptor=descriptor, record_lines="".join(lines),
        arguments=", ".join(arguments),
        release_refs="".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs),
    )


def generate_card_writes(contract: ContractView, metadata: list[tuple[str, str, str, bool]],
                         descriptor: str) -> tuple[str, str]:
    expected = {
        "update_card": ["project_id", "card_id", "update"],
        "trash_card": ["project_id", "card_id", "children"],
        "restore_card": ["project_id", "card_id"],
    }
    for name, parameters in expected.items():
        operation = contract.operation(name)
        if [p.name for p in operation.parameters] != parameters or operation.returns != NamedType(name="Card"):
            raise ValueError(f"Android {name} parameters or return need a generator update")
    update = contract.operation("update_card").parameter("update")
    if update.type != NamedType(name="CardUpdate"):
        raise ValueError("Android update_card needs CardUpdate")
    fields = contract.record("CardUpdate").fields
    if [field.name for field in fields] != [
        "title", "content", "parent_card_id", "sort_key", "updated_at"
    ] or [_field_type(field, contract) for field in fields[:2]] != [
        ("String", "Ljava/lang/String;", True), ("String", "Ljava/lang/String;", True)
    ]:
        raise ValueError("Android CardUpdate fields need a generator update")
    children = contract.operation("trash_card").parameter("children")
    if children.default != "promote" or children.type.kind != "literal" or \
            children.type.values != ["promote", "trash_subtree"]:
        raise ValueError("Android trash_card needs promote as its default child policy")

    kotlin = TEMPLATES.get_template("android_card_writes.kt.j2").render()
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
