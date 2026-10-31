"""Typed, read-only view of the extracted contract for Android generation.

These models describe the IR grammar, not Holder's domain records. The declarations in
contract/interface remain the source of every field and operation.
"""

from __future__ import annotations

from typing import Annotated, Any, Literal

from pydantic import BaseModel, ConfigDict, Field


class View(BaseModel):
    model_config = ConfigDict(extra="ignore", frozen=True)


class TypeView(View):
    model_config = ConfigDict(extra="forbid", frozen=True)
    min_length: int | None = None
    max_length: int | None = None
    pattern: str | None = None
    ge: int | float | None = None
    le: int | float | None = None
    gt: int | float | None = None
    lt: int | float | None = None
    format: str | None = None


class ScalarType(TypeView):
    kind: Literal["string", "bool", "int32", "int64", "float64", "json", "secret", "none", "bytes"]


class NamedType(TypeView):
    kind: Literal["named"] = "named"
    name: str
    arguments: list[TypeRef] = Field(default_factory=list)


class ParameterType(TypeView):
    kind: Literal["parameter"]
    name: str


class LiteralType(TypeView):
    kind: Literal["literal"]
    values: list[str | int]


class NullableType(TypeView):
    kind: Literal["nullable"]
    value: TypeRef


class ListType(TypeView):
    kind: Literal["list"]
    items: TypeRef


class MapType(TypeView):
    kind: Literal["map"]
    values: TypeRef


class UnionType(TypeView):
    kind: Literal["union"]
    alternatives: list[TypeRef]


class StreamType(TypeView):
    kind: Literal["stream"]
    items: TypeRef


TypeRef = Annotated[
    ScalarType | NamedType | ParameterType | LiteralType | NullableType | ListType | MapType | UnionType | StreamType,
    Field(discriminator="kind"),
]


class FieldView(View):
    name: str
    type: TypeRef
    omittable: bool = False
    default: Any = None


class RecordView(View):
    name: str
    fields: list[FieldView]

    def field(self, name: str) -> FieldView:
        try:
            return next(field for field in self.fields if field.name == name)
        except StopIteration as error:
            raise ValueError(f"{self.name} has no field {name}") from error


class AliasView(View):
    name: str
    type: TypeRef


class ParameterView(View):
    name: str
    type: TypeRef
    default: Any = None


class OperationView(View):
    name: str
    parameters: list[ParameterView]
    returns: TypeRef

    def parameter(self, name: str) -> ParameterView:
        return next(parameter for parameter in self.parameters if parameter.name == name)


class ContractView(View):
    ir_version: Literal[1]
    records: list[RecordView]
    aliases: list[AliasView]
    operations: list[OperationView]

    def record(self, name: str) -> RecordView:
        try:
            return next(record for record in self.records if record.name == name)
        except StopIteration as error:
            raise ValueError(f"Contract has no record {name}") from error

    def alias(self, name: str) -> TypeRef:
        try:
            return next(alias.type for alias in self.aliases if alias.name == name)
        except StopIteration as error:
            raise ValueError(f"Contract has no alias {name}") from error

    def operation(self, name: str) -> OperationView:
        try:
            return next(operation for operation in self.operations if operation.name == name)
        except StopIteration as error:
            raise ValueError(f"Contract has no operation {name}") from error

    def resolve_type(self, ref: TypeRef) -> tuple[TypeRef, bool]:
        """Follow aliases and nullability while retaining the field's separate omission rule."""
        nullable = False
        while isinstance(ref, NamedType | NullableType):
            if isinstance(ref, NamedType):
                ref = self.alias(ref.name)
            else:
                nullable = True
                ref = ref.value
        return ref, nullable
