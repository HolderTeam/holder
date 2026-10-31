"""Generic model generation for Android Kotlin data classes and JNI converters."""

from __future__ import annotations

import json
from dataclasses import dataclass
from typing import Any

from .generate import snake
from .ir import (
    ContractView,
    FieldView,
    ListType,
    LiteralType,
    NamedType,
    NullableType,
    RecordView,
    ScalarType,
    TypeRef,
)


def camel_case(name: str) -> str:
    """Convert snake_case to camelCase."""
    first, *rest = name.split("_")
    return first + "".join(word.title() for word in rest)


def pascal_case(name: str) -> str:
    """Convert snake_case to PascalCase."""
    return "".join(word.title() for word in name.split("_"))


@dataclass(frozen=True)
class KotlinField:
    name: str
    kotlin_name: str
    kotlin_type: str
    jni_signature: str
    optional: bool
    default: str | None
    kind: str
    enum_cases: tuple[tuple[str, str], ...] = ()


@dataclass(frozen=True)
class KotlinRecord:
    name: str
    fields: tuple[KotlinField, ...]
    descriptor: str

    def render(self, *, include_defaults: bool = False) -> str:
        lines = [f"data class {self.name}("]
        for f in self.fields:
            opt = "?" if f.optional else ""
            default_val = f" = {f.default}" if include_defaults and f.default is not None else ""
            lines.append(f"    val {f.kotlin_name}: {f.kotlin_type}{opt}{default_val},")
        lines.append(")\n")
        return "\n".join(lines[:1] + lines[1:-1] + [lines[-1]])


def map_field(field: FieldView, contract: ContractView, *,
              record_name: str = "", allow_literal: bool = False) -> KotlinField:
    """Map a contract IR field to its Kotlin/JNI representation."""
    resolved, nullable = contract.resolve_type(field.type)
    optional = field.omittable or nullable
    kind = resolved.kind

    if kind == "literal" and not allow_literal:
        raise ValueError(f"Android record conversion needs a C enum mapping for {field.name}")
    if kind == "bool" and optional:
        raise ValueError(f"Android record conversion does not support optional bool for {field.name}")

    scalars = {
        "string": ("String", "Ljava/lang/String;"),
        "literal": ("String", "Ljava/lang/String;"),
        "bool": ("Boolean", "Ljava/lang/Boolean;" if optional else "Z"),
        "int32": ("Int", "Ljava/lang/Integer;" if optional else "I"),
        "int64": ("Long", "Ljava/lang/Long;" if optional else "J"),
        "float64": ("Double", "Ljava/lang/Double;" if optional else "D"),
    }

    enum_cases: tuple[tuple[str, str], ...] = ()
    if kind in scalars:
        kt_type, jni_sig = scalars[kind]
        if kind == "literal" and isinstance(resolved, LiteralType):
            if not all(isinstance(v, str) for v in resolved.values):
                raise ValueError(f"Android input enum needs string values for {record_name}.{field.name}")
            prefix = f"HOLDER_{snake(record_name).upper()}_{field.name.upper()}"
            enum_cases = tuple((v, f"{prefix}_{v.upper()}") for v in resolved.values)
    elif isinstance(resolved, ListType):
        item_type = resolved.items
        if isinstance(item_type, NamedType):
            kt_item = f"Contract{item_type.name}"
        elif isinstance(item_type, ScalarType) and item_type.kind == "string":
            kt_item = "String"
        else:
            raise ValueError(f"Unsupported list item type in Android mapping: {item_type}")
        kt_type = f"List<{kt_item}>"
        jni_sig = "Ljava/util/List;"
    elif isinstance(resolved, NamedType):
        kt_type = f"Contract{resolved.name}"
        jni_sig = f"Lteam/holder/android/generated/Contract{resolved.name};"
    else:
        raise ValueError(f"Android record conversion does not support {kind} for {field.name}")

    if "default" in field.model_fields_set:
        default = json.dumps(field.default)
    elif field.omittable:
        default = "null"
    else:
        default = None

    return KotlinField(
        name=field.name,
        kotlin_name=camel_case(field.name),
        kotlin_type=kt_type,
        jni_signature=jni_sig,
        optional=optional,
        default=default,
        kind=kind,
        enum_cases=enum_cases,
    )


def map_record(record: RecordView, contract: ContractView, *,
               name_prefix: str = "Contract",
               allow_literal_fields: set[str] | None = None,
               include_fields: set[str] | None = None,
               exclude_fields: set[str] | None = None,
               extra_fields: list[FieldView] | None = None) -> KotlinRecord:
    """Map a full RecordView to a KotlinRecord."""
    allow_literal = allow_literal_fields or set()
    fields = list(record.fields)
    if extra_fields:
        fields.extend(extra_fields)
    if include_fields is not None:
        fields = [f for f in fields if f.name in include_fields]
    if exclude_fields is not None:
        fields = [f for f in fields if f.name not in exclude_fields]
    mapped_fields = tuple(
        map_field(f, contract, record_name=record.name,
                  allow_literal=(f.name in allow_literal))
        for f in fields
    )
    descriptor = "(" + "".join(f.jni_signature for f in mapped_fields) + ")V"
    return KotlinRecord(
        name=f"{name_prefix}{record.name}",
        fields=mapped_fields,
        descriptor=descriptor,
    )


def record_jni_lines(fields: Any, source: str,
                     overrides: dict[str, str] | None = None,
                     indent: str = "    ") -> tuple[list[str], list[str], list[str]]:
    """Generate JNI conversion statements, constructor arguments, and local refs to release."""
    lines: list[str] = []
    arguments: list[str] = []
    refs: list[str] = []
    for item in fields:
        if isinstance(item, KotlinField):
            name, kind, optional = item.name, item.kotlin_type, item.optional
        else:
            name, kind, _, optional = item
        var = f"v_{name}"
        expr = f"{source}.{name}"
        if overrides and name in overrides:
            lines.append(f"{indent}jstring {var} = string(env, {overrides[name]});\n")
            refs.append(var)
        elif kind == "String":
            lines.append(f"{indent}jstring {var} = string(env, {expr});\n")
            refs.append(var)
        elif optional:
            box = {"Long": "boxed_long", "Int": "boxed_int", "Double": "boxed_double"}[kind]
            lines.append(f"{indent}jobject {var} = {expr} ? {box}(env, *{expr}) : nullptr;\n")
            refs.append(var)
        else:
            cast = {"Int": "jint", "Long": "jlong", "Double": "jdouble", "Boolean": "jboolean"}[kind]
            lines.append(f"{indent}{cast} {var} = static_cast<{cast}>({expr});\n")
        arguments.append(var)
    return lines, arguments, refs

