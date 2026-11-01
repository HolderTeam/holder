"""Generate Android's typed Card bridge over the contract C ABI."""

from __future__ import annotations

from typing import Any

from generators.android.page import generate_page_operation

from .android_ir import ContractView, FieldView, NamedType, ScalarType
from .android_templates import TEMPLATES

KOTLIN_PATH = "apps/mobile/android/app/src/main/java/team/holder/android/generated/ContractCards.kt"
JNI_PATH = "apps/mobile/android/app/src/main/cpp/generated/contract_cards.cpp"
PROJECT_KOTLIN_PATH = "apps/mobile/android/app/src/main/java/team/holder/android/generated/ContractProjects.kt"
PROJECT_JNI_PATH = "apps/mobile/android/app/src/main/cpp/generated/contract_projects.cpp"


def _field_type(field: FieldView, contract: ContractView, *,
                allow_literal: bool = False) -> tuple[str, str, bool]:
    value, nullable = contract.resolve_type(field.type)
    optional = field.omittable or nullable
    kind = value.kind
    if kind == "literal" and not allow_literal:
        raise ValueError(f"Android record conversion needs a C enum mapping for {field.name}")
    if kind == "bool" and optional:
        raise ValueError(f"Android record conversion does not support optional bool for {field.name}")
    if kind not in {"string", "literal", "bool", "int32", "int64", "float64"}:
        raise ValueError(f"Android record conversion does not support {kind} for {field.name}")
    return ({"string": "String", "literal": "String", "bool": "Boolean", "int32": "Int", "int64": "Long", "float64": "Double"}[kind],
            {"string": "Ljava/lang/String;", "literal": "Ljava/lang/String;", "int32": "Ljava/lang/Integer;",
             "int64": "Ljava/lang/Long;", "float64": "Ljava/lang/Double;", "bool": "Ljava/lang/Boolean;"}[kind] if optional else
            {"string": "Ljava/lang/String;", "literal": "Ljava/lang/String;", "bool": "Z", "int32": "I", "int64": "J", "float64": "D"}[kind],
            optional)


def _camel(name: str) -> str:
    first, *rest = name.split("_")
    return first + "".join(word.title() for word in rest)


def _record_kotlin(name: str, typed: list[tuple[str, str, str, bool]]) -> str:
    return f"data class {name}(\n" + "".join(
        f"    val {_camel(field)}: {kind}{'?' if optional else ''},\n"
        for field, kind, _, optional in typed) + ")\n"


def _record_jni_lines(typed: list[tuple[str, str, str, bool]], source: str,
                      overrides: dict[str, str] | None = None, indent: str = "    "
                      ) -> tuple[list[str], list[str], list[str]]:
    lines: list[str] = []
    arguments: list[str] = []
    refs: list[str] = []
    for name, kind, _, optional in typed:
        var = f"v_{name}"
        field = f"{source}.{name}"
        if overrides and name in overrides:
            lines.append(f"{indent}jstring {var} = string(env, {overrides[name]});\n")
            refs.append(var)
        elif kind == "String":
            lines.append(f"{indent}jstring {var} = string(env, {field});\n")
            refs.append(var)
        elif optional:
            box = {"Long": "boxed_long", "Int": "boxed_int", "Double": "boxed_double"}[kind]
            lines.append(f"{indent}jobject {var} = {field} ? {box}(env, *{field}) : nullptr;\n")
            refs.append(var)
        else:
            cast = {"Int": "jint", "Long": "jlong", "Double": "jdouble", "Boolean": "jboolean"}[kind]
            lines.append(f"{indent}{cast} {var} = static_cast<{cast}>({field});\n")
        arguments.append(var)
    return lines, arguments, refs


def generate_android(ir: dict[str, Any]) -> dict[str, str]:
    contract = ContractView.model_validate(ir)
    lookup = contract.operation("get_card_metadata")
    if [p.name for p in lookup.parameters] != ["project_id", "card_id"] or \
            lookup.returns != NamedType(name="CardMetadata"):
        raise ValueError("Android get_card_metadata needs CardMetadata and project/card IDs")
    content_lookup = contract.operation("get_card")
    if [p.name for p in content_lookup.parameters] != ["project_id", "card_id"] or \
            content_lookup.returns != NamedType(name="Card") or \
            contract.record("Card").field("content").type != ScalarType(kind="string"):
        raise ValueError("Android get_card needs Card content and project/card IDs")
    create = contract.operation("create_card")
    if [p.name for p in create.parameters] != ["project_id", "card"] or \
            create.parameter("card").type != NamedType(name="NewCard") or \
            create.returns != NamedType(name="Card"):
        raise ValueError("Android create_card needs NewCard and Card")
    new_card_fields = contract.record("NewCard").fields
    if [field.name for field in new_card_fields] != [
        "title", "content", "parent_card_id", "sort_key", "created_at", "updated_at"
    ] or [_field_type(field, contract) for field in new_card_fields] != [
        ("String", "Ljava/lang/String;", False),
        ("String", "Ljava/lang/String;", False),
        ("String", "Ljava/lang/String;", True),
        ("Double", "Ljava/lang/Double;", True),
        ("Long", "Ljava/lang/Long;", True),
        ("Long", "Ljava/lang/Long;", True),
    ]:
        raise ValueError("Android NewCard fields need a generator update")
    metadata = [(field.name, *_field_type(field, contract)) for field in contract.record("CardMetadata").fields]
    header = "// Generated from contract/interface/cards.py and contract/interface/app.py by holder_contract.\n" \
             "// Do not edit; run make -C contract update.\n"
    metadata_descriptor = "(" + "".join(signature for _, _, signature, _ in metadata) + ")V"
    from .android_card_writes import generate_card_writes
    from .android_lists import generate_record_lists
    from .android_references import generate_references
    from .android_trash import generate_trash

    write_kotlin, write_jni = generate_card_writes(contract, metadata, metadata_descriptor)
    list_records, list_methods, list_jni = generate_record_lists(contract)
    trash_record, trash_methods, trash_jni = generate_trash(contract)
    reference_record, reference_method, reference_jni = generate_references(contract)
    kotlin = header + TEMPLATES.get_template("android_cards.kt.j2").render(
        card_metadata_record=_record_kotlin("ContractCardMetadata", metadata),
        list_records=list_records + trash_record + reference_record,
        write_methods=write_kotlin, list_methods=list_methods,
        trash_methods=trash_methods + reference_method,
    )
    meta_lines, meta_arguments, meta_refs = _record_jni_lines(metadata, "card", indent="  ")
    card_metadata_conversion = (
        f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{metadata_descriptor}");\n'
        + "".join(meta_lines)
        + f"  jobject result = env->NewObject(item_type, item_ctor, {', '.join(meta_arguments)});\n"
        + "".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in meta_refs)
    )
    jni = header + TEMPLATES.get_template("android_cards.cpp.j2").render(
        include_boxed_double=any(kind == "Double" and optional for _, kind, _, optional in metadata),
        card_metadata_conversion=card_metadata_conversion,
        write_jni=write_jni, list_jni=list_jni, trash_jni=trash_jni + reference_jni,
    )
    from .android_history import generate_history
    from .android_projects import generate_projects
    from .android_resources import generate_resources

    return {KOTLIN_PATH: kotlin, JNI_PATH: jni,
            **generate_page_operation(contract, "list_cards"),
            **generate_projects(contract), **generate_history(contract), **generate_resources(contract)}
