"""The generated C ABI, derived from typed contract operations."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any

from compiler.ir import (
    ContractView,
    ListType,
    LiteralType,
    NamedType,
    NullableType,
    OperationView,
    ParameterView,
    ScalarType,
    TypeRef,
    UnionType,
)

SCALARS_C = {"int32": "int32_t", "int64": "int64_t", "float64": "double", "bool": "bool"}
# Core uses long long internally for int64; its C boundary converts to int64_t.
SCALARS_CPP = {"int32": "std::int32_t", "int64": "long long", "float64": "double", "bool": "bool"}


class CAbiError(ValueError):
    """A contract shape cannot be represented by the current C ABI."""


def snake(name: str) -> str:
    return re.sub(r"(?<=[a-z0-9])([A-Z])", r"_\1", name).lower()


def literal_member(value: str | int) -> str:
    return re.sub(r"\W", "_", str(value)).upper()


def format_default(value: Any) -> str:
    return '""' if value == "" else ("null" if value is None else str(value).lower() if isinstance(value, bool) else str(value))


@dataclass(frozen=True)
class CChoice:
    value: str | int
    symbol: str


@dataclass(frozen=True)
class CParameter:
    name: str
    ref: TypeRef
    nullable: bool
    c_decl: str
    note: str = ""
    enum_lines: tuple[str, ...] = ()
    choices: tuple[CChoice, ...] = ()
    union_value_field: str | None = None
    union_value_kind: str | None = None

    @property
    def c_type(self) -> str:
        return self.c_decl.rsplit(" ", 1)[0].removeprefix("const ").removesuffix("*").strip()

    @property
    def pointer(self) -> bool:
        return "*" in self.c_decl


@dataclass(frozen=True)
class CResult:
    type_name: str
    out_name: str


@dataclass(frozen=True)
class COperation:
    name: str
    parameters: tuple[CParameter, ...]
    result: CResult | None

    def parameter(self, name: str) -> CParameter:
        try:
            return next(param for param in self.parameters if param.name == name)
        except StopIteration as error:
            raise ValueError(f"{self.name} has no parameter {name}") from error


class CAbi:
    """Plan the core C signature once for both C and JNI renderers."""

    def __init__(self, contract: ContractView):
        self.contract = contract
        self.aliases = {alias.name: alias.type for alias in contract.aliases}
        self._operations: dict[str, COperation] = {}

    def operation(self, name: str) -> COperation:
        if name not in self._operations:
            operation = self.contract.operation(name)
            if operation.daemon_only:
                raise CAbiError(f"{name}: daemon-only operations have no C ABI")
            self._operations[name] = COperation(name, tuple(self._parameter(operation, p)
                                                             for p in operation.parameters), self._result(operation))
        return self._operations[name]

    def _resolve(self, ref: TypeRef) -> tuple[TypeRef, bool, str | None]:
        nullable = False
        alias_name = None
        while isinstance(ref, NamedType | NullableType):
            if isinstance(ref, NullableType):
                nullable = True
                ref = ref.value
            elif ref.name in self.aliases:
                alias_name = ref.name
                ref = self.aliases[ref.name]
            else:
                break
        return ref, nullable, alias_name

    def _record_name(self, ref: NamedType) -> str:
        arguments = []
        for arg in ref.arguments:
            resolved, _, _ = self._resolve(arg)
            if not isinstance(resolved, NamedType) or resolved.arguments:
                raise CAbiError("generic records as type arguments")
            arguments.append(snake(resolved.name))
        return "holder_" + "_".join([*arguments, snake(ref.name)])

    def _parameter(self, operation: OperationView, value: ParameterView) -> CParameter:
        ref, nullable, _ = self._resolve(value.type)
        kind = ref.kind
        name = value.name
        prefix = f"holder_{operation.name}_{name}"
        upper = prefix.upper()
        has_default = "default" in value.model_fields_set
        default = value.default if has_default else None
        if kind == "string":
            note = "May be NULL." if nullable else (f"NULL for {format_default(default)}." if has_default else "")
            return CParameter(name, ref, nullable, f"const char* {name}", note)
        if kind in SCALARS_C:
            zero_default = has_default and not nullable and default in (0, False, 0.0)
            if nullable or (has_default and not zero_default):
                note = "May be NULL." if nullable else f"NULL for {format_default(default)}."
                return CParameter(name, ref, nullable, f"const {SCALARS_C[kind]}* {name}", note)
            return CParameter(name, ref, nullable, f"{SCALARS_C[kind]} {name}")
        if isinstance(ref, LiteralType):
            members = []
            if nullable:
                members.append(f"    {upper}_NONE = 0, /* null */")
            elif has_default:
                members.append(f"    {upper}_DEFAULT = 0, /* {format_default(default)} */")
            choices = tuple(CChoice(item, f"{upper}_{literal_member(item)}") for item in ref.values)
            members += [f"    {choice.symbol} = {index}," for index, choice in enumerate(choices, 1)]
            members.append(f"    {upper}_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */")
            lines = (f"typedef enum {prefix} {{", *members, f"}} {prefix};")
            return CParameter(name, ref, nullable, f"{prefix} {name}",
                              "The zero value is null." if nullable else "", lines, choices)
        if isinstance(ref, UnionType) and nullable:
            literals = [alternative for alternative in ref.alternatives if isinstance(alternative, LiteralType)]
            others = [self._resolve(alternative) for alternative in ref.alternatives
                      if not isinstance(alternative, LiteralType)]
            if len(others) != 1 or not isinstance(others[0][0], ScalarType) or others[0][0].kind != "string":
                raise CAbiError(f"{name}: unions other than literals plus one string")
            value_name = snake(others[0][2] or "value")
            choices = tuple(CChoice(item, f"{upper}_{literal_member(item)}")
                            for literal in literals for item in literal.values)
            members = [f"    {upper}_NONE = 0,", *[f"    {choice.symbol}," for choice in choices],
                       f"    {upper}_{value_name.upper()},",
                       f"    {upper}_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */"]
            lines = (f"typedef enum {prefix}_kind {{", *members, f"}} {prefix}_kind;", "",
                     f"typedef struct {prefix} {{", f"    {prefix}_kind kind;",
                     f"    const char* {value_name}; /* Set when kind is {upper}_{value_name.upper()}. */",
                     f"}} {prefix};")
            return CParameter(name, ref, nullable, f"{prefix} {name}", "The zero value is null.",
                              lines, choices, value_name, f"{upper}_{value_name.upper()}")
        if isinstance(ref, NamedType) and not nullable:
            return CParameter(name, ref, nullable, f"const {self._record_name(ref)}* {name}")
        raise CAbiError(f"{operation.name}({name}): {kind} parameters")

    def _result(self, operation: OperationView) -> CResult | None:
        ref, _, _ = self._resolve(operation.returns)
        if ref.kind == "none":
            return None
        if isinstance(ref, ListType):
            items, _, _ = self._resolve(ref.items)
            if items.kind == "string":
                return CResult("holder_string_list", "out_strings")
            if isinstance(items, NamedType):
                return CResult(self._record_name(items) + "_list", "out_" + snake(items.name) + "_list")
            raise CAbiError(f"{operation.name}: lists of {items.kind} as results")
        if not isinstance(ref, NamedType):
            raise CAbiError(f"{operation.name}: {ref.kind} results")
        return CResult(self._record_name(ref), "out_" + snake(ref.name))
