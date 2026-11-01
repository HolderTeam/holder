"""Plan HTTP dispatch from the same typed operations used by other targets."""

from __future__ import annotations

import re
from dataclasses import dataclass

from compiler.ir import (
    ContractView,
    HttpBindingView,
    ListType,
    LiteralType,
    MapType,
    NamedType,
    NullableType,
    OperationView,
    ParameterType,
    ParameterView,
    TypeRef,
    UnionType,
)
from generators.core.abi import SCALARS_CPP


def camel(name: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in re.split(r"[_\W]+", name) if part)


@dataclass(frozen=True)
class HttpArgument:
    parameter: ParameterView
    location: str
    ref: TypeRef
    nullable: bool
    has_default: bool
    cpp_declaration: str
    body_record: str | None = None
    body_optional: bool = False

@dataclass(frozen=True)
class HttpRoute:
    operation: OperationView
    binding: HttpBindingView
    arguments: tuple[HttpArgument, ...]


@dataclass(frozen=True)
class HttpPlan:
    routes: tuple[HttpRoute, ...]
    bindings: tuple[HttpBindingView, ...]
    error_statuses: dict[str, int]


def _resolve(contract: ContractView, ref: TypeRef) -> tuple[TypeRef, bool]:
    nullable = False
    while isinstance(ref, NamedType | NullableType):
        if isinstance(ref, NullableType):
            nullable = True
            ref = ref.value
            continue
        try:
            inner = contract.alias(ref.name)
        except ValueError:
            break
        constraints = {name: getattr(ref, name) for name in (
            "min_length", "max_length", "pattern", "ge", "le", "gt", "lt", "format"
        ) if name in ref.model_fields_set}
        ref = inner.model_copy(update=constraints)
    return ref, nullable


def _cpp_base(contract: ContractView, ref: TypeRef) -> str:
    ref, _ = _resolve(contract, ref)
    if ref.kind == "string":
        return "std::string"
    if ref.kind in SCALARS_CPP:
        return SCALARS_CPP[ref.kind]
    if isinstance(ref, ListType):
        return f"std::vector<{_cpp_base(contract, ref.items)}>"
    if isinstance(ref, MapType):
        return f"std::map<std::string, {_cpp_base(contract, ref.values)}>"
    if isinstance(ref, NamedType):
        args = f"<{', '.join(_cpp_base(contract, arg) for arg in ref.arguments)}>" if ref.arguments else ""
        return ref.name + args
    if isinstance(ref, ParameterType):
        return ref.name
    raise ValueError(f"unsupported HTTP C++ type: {ref.kind}")


def _cpp_declaration(contract: ContractView, operation: OperationView, parameter: ParameterView,
                     ref: TypeRef, nullable: bool) -> str:
    name = parameter.name
    type_name = camel(operation.name) + camel(name)
    if isinstance(ref, LiteralType):
        held = f"std::optional<{type_name}>" if nullable else type_name
        return f"{held} {name}"
    if isinstance(ref, UnionType):
        return f"std::optional<{type_name}> {name}"
    if isinstance(ref, NamedType):
        return f"const {_cpp_base(contract, ref)}& {name}"
    base = _cpp_base(contract, ref)
    held = f"std::optional<{base}>" if nullable else base
    return f"{held} {name}"


def _body_optional(contract: ContractView, record_name: str) -> bool:
    record = contract.record(record_name)
    fields = list(record.fields)
    while record.base:
        record = contract.record(record.base)
        fields = [*record.fields, *fields]
    return all(field.omittable or "default" in field.model_fields_set for field in fields)


def _argument(contract: ContractView, operation: OperationView, binding: HttpBindingView,
              parameter: ParameterView) -> HttpArgument:
    ref, nullable = _resolve(contract, parameter.type)
    name = parameter.name
    location = "path" if name in binding.path_parameters else "query" if name in binding.query else "body"
    has_default = "default" in parameter.model_fields_set
    if location == "body":
        if not isinstance(ref, NamedType) or nullable:
            raise ValueError(f"{operation.name}({name}): a request body that is not a record")
        body_record = _cpp_base(contract, ref)
        body_optional = _body_optional(contract, ref.name)
    else:
        body_record = None
        body_optional = False
    return HttpArgument(parameter, location, ref, nullable, has_default,
                        _cpp_declaration(contract, operation, parameter, ref, nullable),
                        body_record, body_optional)


def plan_http(contract: ContractView) -> HttpPlan:
    """Match declared bindings to operations and locate every argument."""
    operations = {operation.name: operation for operation in contract.operations}
    routes = []
    for binding in contract.http:
        operation = operations.get(binding.operation)
        if operation is None or operation.daemon_only:
            # The HTTP contract can also describe routes whose implementations are
            # outside the generated core dispatcher.
            continue
        path = set(binding.path_parameters)
        query = set(binding.query)
        if path & query:
            raise ValueError(f"{operation.name}: parameter appears in path and query: {sorted(path & query)}")
        declared = {parameter.name for parameter in operation.parameters}
        unknown = (path | query) - declared
        if unknown:
            raise ValueError(f"{operation.name}: HTTP binding names unknown parameters: {sorted(unknown)}")
        arguments = tuple(_argument(contract, operation, binding, parameter) for parameter in operation.parameters)
        routes.append(HttpRoute(operation, binding, arguments))
    return HttpPlan(tuple(routes), tuple(contract.http),
                    {error.error: error.status for error in contract.http_errors})
