"""Spellings the C header, the C++ declarations and the C glue share."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from compiler.ir import ListType, LiteralType, MapType, NamedType, OperationView, ParameterType, ParameterView, TypeRef

from ..common import Unsupported
from .abi import SCALARS_CPP, CAbiError
from .types import CoreTypes, Slot


def comment(text: str | None, indent: str = "", style: str = "c") -> list[str]:
    if not text:
        return []
    lines = text.splitlines()
    if style == "cpp":
        return [f"{indent}/// {line}".rstrip() for line in lines]
    if len(lines) == 1:
        return [f"{indent}/* {lines[0]} */"]
    return [f"{indent}/* {lines[0]}", *[f"{indent} * {line}".rstrip() for line in lines[1:]], f"{indent} */"]


def cpp_literal(value: Any) -> str:
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, str):
        return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'
    return str(value)


def cpp_base(types: CoreTypes, ref: TypeRef, generic: dict[str, str], enum: str | None = None) -> str:
    """The C++ type of a value in holder::contract; enum names a record's choice of literals."""
    ref = types.resolve(ref).type
    if isinstance(ref, LiteralType) and enum is not None:
        return enum
    if ref.kind == "string":
        return "std::string"
    if ref.kind in SCALARS_CPP:
        return SCALARS_CPP[ref.kind]
    if isinstance(ref, ListType):
        return f"std::vector<{cpp_base(types, ref.items, generic)}>"
    if isinstance(ref, MapType):
        return f"std::map<std::string, {cpp_base(types, ref.values, generic)}>"
    if isinstance(ref, ParameterType):
        return generic[ref.name]
    if isinstance(ref, NamedType):
        return types.instance(ref, {}).cpp_name
    raise Unsupported(ref.kind)


def cpp_type(types: CoreTypes, slot: Slot, generic: dict[str, str] | None = None) -> str:
    base = cpp_base(types, slot.type, generic or {}, slot.enum)
    if slot.change:
        return f"Change<{base}>"
    return f"std::optional<{base}>" if slot.nullable or (slot.optional and not slot.has_default) else base


@dataclass
class Param:
    """How one operation parameter is passed in C."""

    slot: Slot
    c_decl: str
    note: str
    enum_lines: list[str]


def c_param(types: CoreTypes, op: OperationView, parameter: ParameterView) -> Param:
    slot = types.parameter_slot(parameter)
    try:
        planned = types.abi.operation(op.name).parameter(parameter.name)
    except CAbiError as error:
        raise Unsupported(str(error)) from error
    return Param(slot, planned.c_decl, planned.note, list(planned.enum_lines))


def c_out(types: CoreTypes, op: OperationView) -> tuple[str, str] | None:
    """The result's C type and its out parameter's name; a list is a {items, count} struct."""
    try:
        result = types.abi.operation(op.name).result
    except CAbiError as error:
        raise Unsupported(str(error)) from error
    return (result.type_name, result.out_name) if result else None
