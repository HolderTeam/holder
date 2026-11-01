"""Typed, target-independent intermediate representation (IR) of Holder's contract.

Describes the IR grammar as defined in contract/IR.md. The declarations in
contract/interface remain the source of every field and operation.
"""

from __future__ import annotations

from typing import Annotated, Any, Literal

from pydantic import BaseModel, ConfigDict, Field


class View(BaseModel):
    model_config = ConfigDict(extra="forbid", frozen=True)


class TypeView(View):
    model_config = ConfigDict(extra="forbid", frozen=True)
    kind: str
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
    discriminator: str | None = None


class StreamType(TypeView):
    kind: Literal["stream"]
    items: TypeRef


TypeRef = Annotated[
    ScalarType | NamedType | ParameterType | LiteralType | NullableType | ListType | MapType | UnionType | StreamType,
    Field(discriminator="kind"),
]


class FieldView(View):
    name: str
    doc: str | None = None
    type: TypeRef
    omittable: bool = False
    default: Any = None


class RecordView(View):
    name: str
    source: str | None = None
    doc: str | None = None
    base: str | None = None
    open: bool = False
    type_parameters: list[str] = Field(default_factory=list)
    fields: list[FieldView]

    def field(self, name: str) -> FieldView:
        try:
            return next(field for field in self.fields if field.name == name)
        except StopIteration as error:
            raise ValueError(f"{self.name} has no field {name}") from error


class AliasView(View):
    name: str
    source: str | None = None
    doc: str | None = None
    type: TypeRef


class EnumMemberView(View):
    name: str
    value: str


class EnumView(View):
    name: str
    source: str | None = None
    doc: str | None = None
    values: list[EnumMemberView]


class ErrorFieldView(View):
    name: str
    doc: str | None = None
    type: TypeRef


class ErrorView(View):
    name: str
    source: str | None = None
    doc: str | None = None
    fields: list[ErrorFieldView]
    code: int | None = None


class ParameterView(View):
    name: str
    type: TypeRef
    doc: str | None = None
    default: Any = None


class InterfaceMethodView(View):
    name: str
    summary: str | None = None
    description: str | None = None
    parameters: list[ParameterView] = Field(default_factory=list)
    returns: TypeRef


class InterfaceView(View):
    name: str
    source: str | None = None
    doc: str | None = None
    methods: list[InterfaceMethodView]


class OperationView(View):
    name: str
    source: str | None = None
    summary: str | None = None
    description: str | None = None
    parameters: list[ParameterView]
    returns: TypeRef
    errors: list[str]
    daemon_only: bool

    def parameter(self, name: str) -> ParameterView:
        try:
            return next(parameter for parameter in self.parameters if parameter.name == name)
        except StopIteration as error:
            raise ValueError(f"{self.name} has no parameter {name}") from error


class HttpBindingView(View):
    operation: str
    method: str
    path: str
    path_parameters: list[str]
    query: list[str]
    status: int
    headers: list[str] = Field(default_factory=list)
    public: bool = False
    created_when: str | None = None
    body: str | None = None


class HttpErrorView(View):
    error: str
    status: int


class ContractView(View):
    ir_version: Literal[1]
    aliases: list[AliasView]
    enums: list[EnumView]
    records: list[RecordView]
    errors: list[ErrorView]
    interfaces: list[InterfaceView]
    operations: list[OperationView]
    http: list[HttpBindingView]
    http_errors: list[HttpErrorView]

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

    def error(self, name: str) -> ErrorView:
        try:
            return next(error for error in self.errors if error.name == name)
        except StopIteration as exc:
            raise ValueError(f"Contract has no error {name}") from exc

    def enum(self, name: str) -> EnumView:
        try:
            return next(enum for enum in self.enums if enum.name == name)
        except StopIteration as exc:
            raise ValueError(f"Contract has no enum {name}") from exc

    def interface(self, name: str) -> InterfaceView:
        try:
            return next(iface for iface in self.interfaces if iface.name == name)
        except StopIteration as exc:
            raise ValueError(f"Contract has no interface {name}") from exc

    def resolve_type(self, ref: TypeRef) -> tuple[TypeRef, bool]:
        """Follow aliases and nullability while retaining the field's separate omission rule."""
        nullable = False
        while isinstance(ref, NamedType | NullableType):
            if isinstance(ref, NamedType):
                try:
                    ref = self.alias(ref.name)
                except ValueError:
                    # Named type is a Record or Enum, not an alias
                    break
            else:
                nullable = True
                ref = ref.value
        return ref, nullable
