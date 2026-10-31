"""Android adapters for permanent deletion of trashed cards."""

from .android import _field_type, _record_jni_lines, _record_kotlin
from .android_ir import ContractView, ListType, LiteralType, NamedType
from .android_templates import TEMPLATES


def generate_trash(contract: ContractView) -> tuple[str, str, str]:
    listing = contract.operation("list_trash")
    purge = contract.operation("delete_project_trash_item")
    empty = contract.operation("empty_trash")
    if [p.name for p in listing.parameters] != ["project_id", "type"] or \
            listing.returns != ListType(kind="list", items=NamedType(name="TrashItem")):
        raise ValueError("Android card trash listing needs list_trash")
    if [p.name for p in purge.parameters] != ["project_id", "type", "id"] or \
            purge.returns != NamedType(name="TrashDeleteResult") or \
            purge.parameter("type").type != NamedType(name="TrashItemType"):
        raise ValueError("Android card purge needs delete_project_trash_item")
    if [p.name for p in empty.parameters] != ["project_id", "type"] or \
            empty.returns != NamedType(name="TrashEmptyResult") or \
            empty.parameter("type").type != NamedType(name="TrashFilter"):
        raise ValueError("Android empty card trash needs empty_trash")
    for alias in ("TrashItemType", "TrashFilter"):
        values = contract.alias(alias)
        if not isinstance(values, LiteralType) or "card" not in values.values:
            raise ValueError(f"Android card trash needs {alias} to include card")
    fields = [(field.name, *_field_type(field, contract, allow_literal=field.name == "type"))
              for field in contract.record("TrashItem").fields]
    record = _record_kotlin("ContractTrashItem", fields) + "\n"
    descriptor = "(" + "".join(signature for _, _, signature, _ in fields) + ")V"
    lines, arguments, refs = _record_jni_lines(fields, "value", {"type": "type_name"})
    list_jni = TEMPLATES.get_template("android_record_list.cpp.j2").render(
        method="nativeListTrashedCards", operation="list_trash", extra_parameters="",
        required_parameters="", extra_variables="", decode_parameters="",
        pre_call="", c_record="holder_trash_item",
        c_call=('holder_list_trash(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)), '
                'project.c_str(), "card", arena.value, &page, &error)'),
        kotlin_record="ContractTrashItem", descriptor=descriptor,
        record_lines=('    if (value.type != HOLDER_TRASH_ITEM_TYPE_CARD) { fail(env, "unexpected trash item"); '
                      'return nullptr; }\n    const char* type_name = "card";\n' + "".join(lines)),
        arguments=", ".join(arguments),
        release_refs="".join(f"    if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs),
    )
    return (record, TEMPLATES.get_template("android_trash.kt.j2").render(),
            list_jni + TEMPLATES.get_template("android_trash.cpp.j2").render())
