"""Validated runtime bindings and supported shapes for complete daemon boundaries.

Only the explicitly selected operations migrate. Public metadata comes from the
compiler result; C++ handler names, services and exception types belong here.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Annotated, Literal

from pydantic import BaseModel, ConfigDict, StringConstraints

from compiler.ir import (
    ContractView,
    FieldView,
    HttpBindingView,
    MapType,
    NamedType,
    NullableType,
    OperationView,
    RecordView,
    ScalarType,
    TypeRef,
)

from ..common import Unsupported

CppName = Annotated[str, StringConstraints(pattern=r"^[A-Za-z_][A-Za-z_0-9]*(::[A-Za-z_][A-Za-z_0-9]*)*$")]
Header = Annotated[str, StringConstraints(pattern=r"^api/[A-Za-z_0-9/]+\.h$")]
Service = Literal["db", "secret_store"]
SERVICE_TYPES = {"db": "holder::platform::Db&", "secret_store": "holder::privacy::SecretStore*"}
MIGRATED = frozenset({"put_location_binding"})
CONSTRAINTS = ("min_length", "max_length", "pattern", "ge", "le", "gt", "lt", "format")


class BoundaryBinding(BaseModel):
    model_config = ConfigDict(extra="forbid", frozen=True)

    handler: CppName
    header: Header
    services: tuple[Service, ...]
    arguments: tuple[str, ...]
    exceptions: dict[CppName, int]
    # Diagnostic wording is a backend convention; the field is supplied by the IR.
    map_value_error: str = "{field} values must be strings"


def bindings() -> dict[str, BoundaryBinding]:
    return {
        "put_location_binding": BoundaryBinding(
            handler="holder::api::put_locations_location_id_binding",
            header="api/LocationWrite.h",
            services=("db", "secret_store"),
            arguments=("location_id", "request"),
            exceptions={"holder::api::LocationNotFound": 404, "holder::api::SecretStoreUnavailable": 503},
            map_value_error="binding {field} must be strings",
        )
    }


@dataclass(frozen=True)
class BoundaryRoute:
    operation: OperationView
    http: HttpBindingView
    runtime: BoundaryBinding


class BoundaryPlan:
    """Validate selected routes once before emitting types, codecs or dispatch."""

    def __init__(self, contract: ContractView, runtime: dict[str, BoundaryBinding] | None = None):
        self.contract = contract
        runtime = bindings() if runtime is None else runtime
        if runtime.keys() != MIGRATED:
            raise Unsupported(
                f"daemon boundary bindings differ: missing={sorted(MIGRATED - runtime.keys())}, "
                f"unexpected={sorted(runtime.keys() - MIGRATED)}"
            )
        self.routes: list[BoundaryRoute] = []
        self.records: list[RecordView] = []
        self._done: set[str] = set()
        self._active: set[str] = set()
        for name, wiring in runtime.items():
            operations = [operation for operation in contract.operations if operation.name == name]
            http = [binding for binding in contract.http if binding.operation == name]
            if len(operations) != 1 or not operations[0].daemon_only or len(http) != 1:
                raise Unsupported(f"{name}: complete daemon boundary requires one daemon-only HTTP operation")
            operation, binding = operations[0], http[0]
            if binding.response_mode != "raw" or binding.response_media_type != "application/json":
                raise Unsupported(f"{name}: complete daemon boundary requires a raw JSON response")
            if (
                binding.query
                or binding.headers
                or binding.sse_events
                or binding.created_when
                or binding.raw_schema is not None
            ):
                raise Unsupported(f"{name}: unsupported daemon boundary HTTP metadata")
            parameter_names = [parameter.name for parameter in operation.parameters]
            if len(wiring.arguments) != len(set(wiring.arguments)) or set(wiring.arguments) != set(parameter_names):
                raise Unsupported(f"{name}: runtime argument inventory differs from the declaration")
            if len(wiring.services) != len(set(wiring.services)):
                raise Unsupported(f"{name}: duplicate runtime service")
            if set(parameter_names) != set(binding.path_parameters) | {binding.body}:
                raise Unsupported(f"{name}: every parameter must be a path parameter or the body")
            for parameter in operation.parameters:
                ref = self.resolve(parameter.type)
                if parameter.name == binding.body:
                    if not isinstance(ref, NamedType) or "default" in parameter.model_fields_set:
                        raise Unsupported(f"{name}: body must be a required named record")
                elif ref.kind != "string" or "default" in parameter.model_fields_set:
                    raise Unsupported(f"{name}: path parameters must be required strings")
                self.reach(parameter.type)
            if not isinstance(self.resolve(operation.returns), NamedType):
                raise Unsupported(f"{name}: response must be a named record")
            self.reach(operation.returns)
            if contract.openapi is None:
                raise Unsupported(f"{name}: the compiler's OpenAPI artifact is required")
            responses = contract.openapi["paths"][binding.path][binding.method.lower()]["responses"]
            success = responses.get(str(binding.status), {}).get("content", {})
            schema = dict(success.get("application/json", {}).get("schema", {}))
            if schema.get("type") == "string" and "$ref" in schema:
                schema.pop("type")  # FastAPI inherits the raw response class's schema hint.
            expected = {"$ref": f"#/components/schemas/{self.resolve(operation.returns).name}"}
            if success.keys() != {"application/json"} or schema != expected:
                raise Unsupported(f"{name}: success schema differs from the declared response record")
            supported = {str(status) for status in (binding.status, 400, 500, *wiring.exceptions.values())}
            for status, response in responses.items():
                schema = response.get("content", {}).get("application/json", {}).get("schema", {})
                if status == "422" and schema == {"$ref": "#/components/schemas/HTTPValidationError"}:
                    continue  # The daemon answers malformed requests with its declared 400.
                if status not in supported:
                    raise Unsupported(f"{name}: response status {status} has no runtime binding")
            for status in (400, 500, *wiring.exceptions.values()):
                if str(status) not in responses:
                    raise Unsupported(f"{name}: exception status {status} is not declared")
                schema = responses[str(status)].get("content", {}).get("application/json", {}).get("schema", {})
                if schema != {"$ref": "#/components/schemas/ErrorResponse"}:
                    raise Unsupported(f"{name}: status {status} must use the declared ErrorResponse")
            self.routes.append(BoundaryRoute(operation, binding, wiring))
        self.errors, self.error_constants = self._error_records()

    def resolve(self, ref: TypeRef) -> TypeRef:
        seen = set()
        while isinstance(ref, NamedType):
            aliases = [alias for alias in self.contract.aliases if alias.name == ref.name]
            if not aliases:
                break
            if ref.name in seen or ref.arguments or any(getattr(ref, key) is not None for key in CONSTRAINTS):
                raise Unsupported(f"{ref.name}: unsupported daemon boundary alias")
            seen.add(ref.name)
            ref = aliases[0].type
        return ref

    def reach(self, ref: TypeRef) -> None:
        ref = self.resolve(ref)
        allowed = {"min_length", "max_length"} if ref.kind == "string" else set()
        if any(getattr(ref, key) is not None for key in set(CONSTRAINTS) - allowed):
            raise Unsupported(f"{ref.kind}: unsupported daemon boundary constraints")
        if isinstance(ref, NamedType):
            if ref.arguments or ref.name in self._active:
                raise Unsupported(f"{ref.name}: generic or recursive daemon boundary record")
            if ref.name in self._done:
                return
            try:
                record = self.contract.record(ref.name)
            except ValueError as error:
                raise Unsupported(f"{ref.name}: unknown daemon boundary record") from error
            if record.base or record.open or record.type_parameters:
                raise Unsupported(f"{record.name}: inherited, open or generic daemon boundary record")
            self._active.add(ref.name)
            for field in record.fields:
                self.reach(field.type)
                if "default" in field.model_fields_set:
                    if field.omittable:
                        raise Unsupported(f"{record.name}.{field.name}: default and omission cannot be combined")
                    default_ref = self.resolve(field.type)
                    if isinstance(default_ref, NullableType):
                        if field.default is None:
                            continue
                        default_ref = self.resolve(default_ref.value)
                    if not (
                        (default_ref.kind == "string" and isinstance(field.default, str))
                        or (default_ref.kind == "bool" and type(field.default) is bool)
                    ):
                        raise Unsupported(f"{record.name}.{field.name}: unsupported daemon boundary default")
                    if default_ref.kind == "string":
                        try:
                            field.default.encode("utf-8")
                        except UnicodeError as error:
                            raise Unsupported(f"{record.name}.{field.name}: invalid text default") from error
                        if (
                            "\0" in field.default
                            or len(field.default) < (default_ref.min_length or 0)
                            or (default_ref.max_length is not None and len(field.default) > default_ref.max_length)
                        ):
                            raise Unsupported(f"{record.name}.{field.name}: invalid text default")
            self._active.remove(ref.name)
            self._done.add(ref.name)
            self.records.append(record)
        elif isinstance(ref, NullableType):
            if isinstance(self.resolve(ref.value), NullableType):
                raise Unsupported("nested nullable aliases are not supported in daemon boundaries")
            self.reach(ref.value)
        elif isinstance(ref, MapType):
            if self.resolve(ref.values).kind != "string":
                raise Unsupported("daemon boundary maps require string values")
            self.reach(ref.values)
        elif ref.kind not in {"string", "bool"}:
            raise Unsupported(f"{ref.kind}: unsupported daemon boundary type")

    def _error_records(self) -> tuple[list[RecordView], dict[tuple[str, str], bool]]:
        """The standard runtime error objects' public fields come from FastAPI's artifact."""
        schemas = self.contract.openapi["components"]["schemas"]
        records = []
        constants = {}
        for name in ("Error", "ErrorResponse"):
            schema = schemas.get(name, {})
            properties = schema.get("properties", {})
            if schema.get("type") != "object" or set(schema.get("required", [])) != properties.keys():
                raise Unsupported(f"{name}: unsupported daemon error schema")
            fields = []
            for key, value in properties.items():
                if set(value) - {"type", "title", "description", "const", "$ref"}:
                    raise Unsupported(f"{name}.{key}: unsupported daemon error constraints")
                if value.get("type") in {"string", "boolean"}:
                    ref = ScalarType(kind="string" if value["type"] == "string" else "bool")
                    if "const" in value:
                        if type(value["const"]) is not bool:
                            raise Unsupported(f"{name}.{key}: unsupported error constant")
                        constants[(name, key)] = value["const"]
                elif value == {"$ref": "#/components/schemas/Error"}:
                    ref = NamedType(name="Error")
                else:
                    raise Unsupported(f"{name}.{key}: unsupported daemon error field")
                fields.append(FieldView(name=key, type=ref))
            records.append(RecordView(name=name, fields=fields))
        return records, constants
