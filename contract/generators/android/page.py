"""Lower a contract page operation into typed Kotlin/JNI rendering data.

This pass knows supported type shapes and the generated C ABI naming rules. It does not
know operation names or assemble C++ statements.
"""

from __future__ import annotations

import json
from dataclasses import dataclass

from compiler.ir import (
    ContractView,
    ListType,
    LiteralType,
    NamedType,
    NullableType,
    ParameterType,
    ParameterView,
    ScalarType,
    UnionType,
)
from generators.core.abi import CAbi, CParameter

from .jinja import TEMPLATES

KOTLIN_PATH = "apps/mobile/android/app/src/main/java/team/holder/android/generated/ContractOperations.kt"
JNI_PATH = "apps/mobile/android/app/src/main/cpp/generated/contract_operations.cpp"


def camel(name: str) -> str:
    first, *rest = name.split("_")
    return first + "".join(part.title() for part in rest)


def pascal(name: str) -> str:
    return "".join(part.title() for part in name.split("_"))


@dataclass(frozen=True)
class Choice:
    value: str
    c_name: str


@dataclass(frozen=True)
class Argument:
    name: str
    kotlin_name: str
    kotlin_type: str
    jni_type: str
    kind: str
    nullable: bool
    default: str | None
    c_type: str
    c_pointer: bool
    choices: tuple[Choice, ...] = ()
    union_value_field: str | None = None
    union_value_kind: str | None = None


@dataclass(frozen=True)
class ResultField:
    name: str
    kotlin_name: str
    kotlin_type: str
    kind: str
    optional: bool
    jni_type: str
    descriptor: str


@dataclass(frozen=True)
class PageOperation:
    name: str
    kotlin_name: str
    arguments: tuple[Argument, ...]
    c_result_type: str
    items_field: str
    items_kotlin_name: str
    cursor_field: str
    cursor_kotlin_name: str
    item_name: str
    item_descriptor: str
    fields: tuple[ResultField, ...]


def _argument(operation_name: str, value: ParameterView, param: CParameter) -> Argument:
    ref, nullable = param.ref, param.nullable
    kind = ref.kind
    default = json.dumps(value.default) if "default" in value.model_fields_set else None
    if default == "null" and not nullable:
        raise ValueError(f"{operation_name}({value.name}): null default needs a nullable type")
    if nullable and kind in {"literal", "int32", "bool"}:
        raise ValueError(f"{operation_name}({value.name}): nullable {kind} needs a JNI conversion rule")
    choices: tuple[Choice, ...] = ()
    union_value_field = union_value_kind = None
    if kind in {"string", "literal", "union"}:
        kotlin_type, jni_type = "String" + ("?" if nullable else ""), "jstring"
    elif kind == "int32":
        kotlin_type, jni_type = "Int", "jint"
    elif kind == "bool":
        kotlin_type, jni_type = "Boolean", "jboolean"
    else:
        raise ValueError(f"{operation_name}({value.name}): Android page calls do not support {kind}")
    if kind == "literal":
        if not isinstance(ref, LiteralType) or not all(isinstance(v, str) for v in ref.values):
            raise ValueError(f"{operation_name}({value.name}): expected string literals")
        choices = tuple(Choice(choice.value, choice.symbol) for choice in param.choices)
    if kind == "union":
        if not isinstance(ref, UnionType) or not nullable:
            raise ValueError(f"{operation_name}({value.name}): expected a nullable literal/string union")
        if not all(isinstance(choice.value, str) for choice in param.choices):
            raise ValueError(f"{operation_name}({value.name}): expected string literals and one string")
        choices = tuple(Choice(choice.value, choice.symbol) for choice in param.choices)
        union_value_field, union_value_kind = param.union_value_field, param.union_value_kind
    return Argument(value.name, camel(value.name), kotlin_type, jni_type, kind, nullable,
                    default, param.c_type, param.pointer, choices, union_value_field, union_value_kind)


def lower_page_operation(contract: ContractView, operation_name: str) -> PageOperation:
    """Describe any supported Page[Record] operation over the generated C API."""
    operation = contract.operation(operation_name)
    result = operation.returns
    if not isinstance(result, NamedType) or len(result.arguments) != 1:
        raise ValueError(f"{operation_name}: expected a generic page result")
    page = contract.record(result.name)
    item_ref = result.arguments[0]
    if not isinstance(item_ref, NamedType):
        raise TypeError(f"{operation_name}: page item must be a record")
    if len(page.type_parameters) != 1 or len(page.fields) != 2:
        raise ValueError(f"{operation_name}: unsupported page shape")
    items, cursor = page.fields
    if (not isinstance(items.type, ListType) or not isinstance(items.type.items, ParameterType) or
            items.type.items.name != page.type_parameters[0] or
            not isinstance(cursor.type, NullableType) or
            not isinstance(cursor.type.value, ScalarType) or cursor.type.value.kind != "string"):
        raise ValueError(f"{operation_name}: unsupported page fields")
    item = contract.record(item_ref.name)
    abi = CAbi(contract).operation(operation_name)
    if abi.result is None:
        raise ValueError(f"{operation_name}: page has no C result")
    fields = []
    chain = []
    current = item
    while current:
        chain.append(current)
        current = contract.record(current.base) if current.base else None
    for record in reversed(chain):
        for field in record.fields:
            ref, nullable = contract.resolve_type(field.type)
            kind = ref.kind
            optional = field.omittable or nullable
            primitive = {"int32": ("jint", "I"), "int64": ("jlong", "J"),
                         "float64": ("jdouble", "D"), "bool": ("jboolean", "Z")}
            boxed = {"int32": "Ljava/lang/Integer;", "int64": "Ljava/lang/Long;",
                     "float64": "Ljava/lang/Double;", "bool": "Ljava/lang/Boolean;"}
            if kind == "string":
                jni_type, descriptor = "jstring", "Ljava/lang/String;"
            elif kind in primitive:
                jni_type, descriptor = (("jobject", boxed[kind]) if optional else primitive[kind])
            else:
                raise ValueError(f"{operation_name}: unsupported result field {field.name}: {kind}")
            if kind == "bool" and optional:
                raise ValueError(f"{operation_name}: optional boolean result needs a JNI boxing rule")
            kotlin_type = {"string": "String", "int32": "Int", "int64": "Long",
                           "float64": "Double", "bool": "Boolean"}[kind]
            fields.append(ResultField(field.name, camel(field.name), kotlin_type,
                                      kind, optional, jni_type, descriptor))
    arguments = tuple(_argument(operation_name, value, abi.parameter(value.name)) for value in operation.parameters)
    return PageOperation(operation.name, "native" + pascal(operation.name), arguments,
                         abi.result.type_name, items.name, camel(items.name), cursor.name, camel(cursor.name), item.name,
                         "(" + "".join(f.descriptor for f in fields) + ")V", tuple(fields))


def generate_page_operation(contract: ContractView, operation_name: str) -> dict[str, str]:
    plan = lower_page_operation(contract, operation_name)
    header = "// Generated from contract/interface by holder_contract. Do not edit.\n"
    return {
        KOTLIN_PATH: header + TEMPLATES.get_template("page_operation.kt.j2").render(plan=plan),
        JNI_PATH: header + TEMPLATES.get_template("page_operation.cpp.j2").render(
            plan=plan, include_boxed_double=any(f.kind == "float64" and f.optional for f in plan.fields)),
    }
