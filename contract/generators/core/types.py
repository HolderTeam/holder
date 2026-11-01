"""The contract's types as core's C API and C++ declarations use them.

Lowering resolves aliases, instantiates generic records, strips nullability into flags, and names a
record's choices of literals as enums. The C header, the C++ declarations, the C glue, the daemon's
JSON codec and the conformance tests' C driver all read the same lowering.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any

from compiler.ir import (
    ContractView,
    FieldView,
    ListType,
    LiteralType,
    MapType,
    NamedType,
    NullableType,
    OperationView,
    ParameterType,
    RecordView,
    StreamType,
    TypeRef,
    UnionType,
)

from ..common import Unsupported
from .abi import SCALARS_C, CAbi, snake

# A type's constraints, which an alias's use may add to what the alias declares.
CONSTRAINTS = ("min_length", "max_length", "pattern", "ge", "le", "gt", "lt", "format")


def camel(name: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in re.split(r"[_\W]+", name) if part)


def constraint(ref: TypeRef, name: str) -> Any:
    """A constraint the type states, or None."""
    return getattr(ref, name, None)


def constrained(ref: TypeRef, *names: str) -> bool:
    return any(constraint(ref, name) is not None for name in names)


@dataclass(frozen=True)
class Instance:
    """A record as used: its name and, for a generic record, its type arguments."""

    record: str
    arguments: tuple[str, ...] = ()

    @property
    def c_name(self) -> str:
        return "holder_" + "_".join([snake(a) for a in self.arguments] + [snake(self.record)])

    @property
    def cpp_name(self) -> str:
        return self.qualified("")

    def qualified(self, namespace: str) -> str:
        """The C++ name, with namespace before the record and each type argument."""
        arguments = f"<{', '.join(namespace + a for a in self.arguments)}>" if self.arguments else ""
        return namespace + self.record + arguments


@dataclass(frozen=True)
class Resolved:
    """A type with its aliases replaced by what they name, and the alias it was named with."""

    type: TypeRef
    alias: str | None = None


@dataclass
class Slot:
    """A field or parameter after aliases are resolved."""

    name: str
    type: TypeRef  # resolved, and not nullable: nullable says that
    doc: str | None
    nullable: bool
    optional: bool  # omittable, or has a default
    default: Any = None
    has_default: bool = False
    change: bool = False  # nullable and omittable: a partial update keeps, sets or clears it
    alias: str | None = None  # the alias the type is named with; a map is named for it
    enum: str | None = None  # a record's choice of literals: its enum's name


def _with_constraints(inner: TypeRef, outer: TypeRef, skip: tuple[str, ...]) -> TypeRef:
    """inner, with the constraints outer states on top."""
    update = {name: getattr(outer, name) for name in outer.model_fields_set
              if name not in skip and name in CONSTRAINTS}
    return inner.model_copy(update=update) if update else inner


class CoreTypes:
    """The contract's operations and records as core's C API sees them."""

    def __init__(self, contract: ContractView):
        self.contract = contract
        self.aliases = {alias.name: alias for alias in contract.aliases}
        self.records: dict[str, RecordView] = {record.name: record for record in contract.records}
        self.errors = contract.errors
        self.operations: list[OperationView] = [op for op in contract.operations if not op.daemon_only]
        if contract.enums:
            raise Unsupported("enums")
        if contract.interfaces:
            raise Unsupported("host interfaces")
        for error in self.errors:
            if error.fields:
                raise Unsupported(f"error kinds with fields ({error.name})")
        for error in self.errors:
            if error.code is None:
                raise Unsupported(f"error kinds without a code ({error.name})")
        self.codes = {error.name: error.code for error in self.errors}
        self.abi = CAbi(contract)
        self.outputs: list[Instance] = []
        self.inputs: list[Instance] = []
        self.listed: set[Instance] = set()
        # Each map's type, by its alias: a map is named for the alias it is declared with.
        self.maps: dict[str, MapType] = {}
        for op in self.operations:
            for parameter in op.parameters:
                self.collect(parameter.type, self.inputs, {})
            self.collect(op.returns, self.outputs, {})
        for instance in self.outputs:
            for slot in self.slots(instance):
                if slot.change:
                    raise Unsupported(f"{instance.record}.{slot.name}: a partial update in a result")

    def resolve(self, ref: TypeRef) -> Resolved:
        """The type with aliases replaced by what they name, keeping any constraints on top."""
        if isinstance(ref, NamedType) and ref.name in self.aliases:
            inner = self.resolve(self.aliases[ref.name].type)
            return Resolved(_with_constraints(inner.type, ref, ("kind", "name")), inner.alias or ref.name)
        return Resolved(ref)

    def kind(self, ref: TypeRef) -> str:
        """The kind of the type an alias names."""
        return self.resolve(ref).type.kind

    def instance(self, ref: TypeRef, bindings: dict[str, Instance]) -> Instance:
        if isinstance(ref, ParameterType):
            return bindings[ref.name]
        arguments = tuple(self.instance(a, bindings).record for a in ref.arguments)
        for argument in ref.arguments:
            if self.instance(argument, bindings).arguments:
                raise Unsupported("generic records as type arguments")
        return Instance(ref.name, arguments)

    def collect(self, ref: TypeRef, into: list[Instance], bindings: dict[str, Instance],
                alias: str | None = None) -> None:
        resolved = self.resolve(ref)
        ref, alias = resolved.type, resolved.alias or alias
        if isinstance(ref, NullableType):
            self.collect(ref.value, into, bindings)
        elif isinstance(ref, MapType):
            if alias is None:
                raise Unsupported("a map without a name of its own")
            self.maps[alias] = ref
        elif isinstance(ref, ListType):
            items = self.resolve(ref.items).type
            if isinstance(items, NamedType | ParameterType):
                self.listed.add(self.instance(items, bindings))
            self.collect(ref.items, into, bindings)
        elif isinstance(ref, UnionType):
            for alternative in ref.alternatives:
                self.collect(alternative, into, bindings)
        elif isinstance(ref, NamedType | ParameterType):
            instance = self.instance(ref, bindings)
            if instance in into:
                return
            record = self.records[instance.record]
            inner = dict(zip(record.type_parameters, [Instance(a) for a in instance.arguments]))
            for slot in self.slots(instance):
                self.collect(slot.type, into, inner, slot.alias)
            into.append(instance)

    def slots(self, instance: Instance) -> list[Slot]:
        """A record's fields, its base's first, with type parameters replaced by the instance's arguments."""
        record = self.records[instance.record]
        out = self.slots(Instance(record.base)) if record.base else []
        bindings = dict(zip(record.type_parameters, instance.arguments))
        for field in record.fields:
            out.append(self.field_slot(instance.record, field, bindings))
        return out

    def field_slot(self, record: str, field: FieldView, bindings: dict[str, str] | None = None) -> Slot:
        """A record's field. A choice of literals is an enum named for the record and the field."""
        slot = self.slot(field.name, self.substitute(field.type, bindings or {}), field.doc,
                         omittable=field.omittable, default=field.default,
                         has_default="default" in field.model_fields_set)
        if isinstance(slot.type, LiteralType):
            if slot.nullable or (slot.optional and not slot.has_default):
                raise Unsupported(f"{record}.{slot.name}: an optional choice of literals in a record")
            if not all(isinstance(v, str) for v in slot.type.values):
                raise Unsupported(f"{record}.{slot.name}: a choice of integers in a record")
            slot.enum = camel(record) + camel(slot.name)
        return slot

    def own_enums(self, record: str) -> list[Slot]:
        """The record's own fields, not its base's, that are a choice of literals."""
        slots = [self.field_slot(record, field) for field in self.records[record].fields]
        return [slot for slot in slots if isinstance(slot.type, LiteralType)]

    def substitute(self, ref: TypeRef, bindings: dict[str, str]) -> TypeRef:
        if isinstance(ref, ParameterType):
            return NamedType(name=bindings[ref.name])
        if isinstance(ref, NullableType):
            return ref.model_copy(update={"value": self.substitute(ref.value, bindings)})
        if isinstance(ref, ListType | StreamType):
            return ref.model_copy(update={"items": self.substitute(ref.items, bindings)})
        if isinstance(ref, UnionType):
            return ref.model_copy(update={"alternatives": [self.substitute(a, bindings) for a in ref.alternatives]})
        return ref

    def parameter_slot(self, parameter: Any) -> Slot:
        """An operation parameter as a slot."""
        return self.slot(parameter.name, parameter.type, parameter.doc, default=parameter.default,
                         has_default="default" in parameter.model_fields_set)

    def slot(self, name: str, ref: TypeRef, doc: str | None, omittable: bool = False, default: Any = None,
             has_default: bool = False) -> Slot:
        resolved = self.resolve(ref)
        ref, alias = resolved.type, resolved.alias
        nullable = isinstance(ref, NullableType)
        if nullable:
            inner = self.resolve(ref.value)
            ref, alias = _with_constraints(inner.type, ref, ("kind", "value")), alias or inner.alias
        if nullable and omittable and ref.kind != "string":
            raise Unsupported(f"{name}: a partial update of a {ref.kind}, which only strings have so far")
        if isinstance(ref, MapType):
            values = self.resolve(ref.values).type
            if alias is None:
                raise Unsupported(f"{name}: a map without a name of its own")
            string_list = isinstance(values, ListType) and self.kind(values.items) == "string"
            if values.kind != "string" and not string_list:
                raise Unsupported(f"{name}: maps to anything but strings or lists of strings")
        if ref.kind in ("json", "secret", "stream", "bytes", "none"):
            raise Unsupported(f"{name}: {ref.kind}")
        if constraint(ref, "pattern") is not None:
            raise Unsupported(f"{name}: patterns, which the glue would not check")
        # A field's or parameter's doc ends up in a one-line comment.
        doc = " ".join(doc.split()) if doc else doc
        return Slot(name, ref, doc, nullable, omittable or has_default, default, has_default, nullable and omittable,
                    alias)

    def changes(self) -> bool:
        """Whether an input record has a field a partial update keeps, sets or clears."""
        return any(slot.change for instance in self.inputs for slot in self.slots(instance))


def unique(items: list[Instance]) -> list[Instance]:
    seen, out = set(), []
    for item in items:
        if item not in seen:
            seen.add(item)
            out.append(item)
    return out


def list_c_name(types: CoreTypes, items: TypeRef, bindings: dict[str, Instance] | None = None) -> str:
    items = types.resolve(items).type
    if items.kind == "string":
        return "holder_string_list"
    if items.kind in SCALARS_C:
        return f"holder_{items.kind}_list"
    return types.instance(items, bindings or {}).c_name + "_list"


def map_c_name(alias: str) -> str:
    return "holder_" + snake(alias)


def c_enum_name(slot: Slot) -> str:
    return "holder_" + snake(slot.enum)
