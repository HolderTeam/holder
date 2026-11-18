"""Authoring sugar that expands resources to ordinary records and route declarations.

There is deliberately no resource IR or runtime implementation here. Existing
Pydantic/FastAPI declarations remain the input to the compiler and all backends.
"""

from __future__ import annotations

import inspect
import keyword
import re
from copy import deepcopy
from dataclasses import dataclass
from typing import Annotated, Any, ClassVar, Literal, get_args, get_origin

from fastapi import Body, Path
from pydantic import Field, create_model
from pydantic.fields import FieldInfo
from pydantic_core import MISSING, PydanticUndefined

from .vocabulary import Record


@dataclass(frozen=True)
class Input:
    """Include a resource field in an input, optionally overriding its default or docs.

    Create inputs are required by default. Update inputs always distinguish omission
    from null, retaining the source field's nullability.
    """

    default: Any = PydanticUndefined
    doc: str | None = None


@dataclass(frozen=True)
class ResourceField:
    """Fields without input roles are output-only. Input types can differ from reads."""

    create: Input | None = None
    update: Input | None = None
    input_type: Any = PydanticUndefined


@dataclass(frozen=True)
class Parameter:
    """An extra operation parameter, such as a list filter or trash policy."""

    name: str
    type: Any
    default: Any = PydanticUndefined


class Resource(Record):
    """A read record with input roles and explicit standard-operation placement."""

    _collection: ClassVar[str | None] = None
    _identity: ClassVar[str | None] = None
    _identity_type: ClassVar[Any] = PydanticUndefined
    _input_records: ClassVar[dict[str, type[Record]]]

    def __init_subclass__(
        cls,
        *,
        collection: str | None = None,
        identity: str | None = None,
        identity_type: Any = PydanticUndefined,
        **kwargs,
    ):
        super().__init_subclass__(**kwargs)
        cls._collection = collection if collection is not None else cls._collection
        cls._identity = identity if identity is not None else cls._identity
        cls._identity_type = identity_type if identity_type is not PydanticUndefined else cls._identity_type
        cls._input_records = {}

    @classmethod
    def __pydantic_init_subclass__(cls, **kwargs):
        super().__pydantic_init_subclass__(**kwargs)
        for name, info in cls.model_fields.items():
            markers = [item for item in info.metadata if isinstance(item, ResourceField)]
            if len(markers) > 1:
                raise ValueError(f"{cls.__name__}.{name}: only one ResourceField is allowed")
            for marker in markers:
                for role in ("create", "update"):
                    selected = getattr(marker, role)
                    if selected is not None and not isinstance(selected, Input):
                        raise TypeError(f"{cls.__name__}.{name}: {role} must be an Input")
                if marker.update is not None and marker.update.default is not PydanticUndefined:
                    raise ValueError(f"{cls.__name__}.{name}: updates omit to keep; no default is allowed")
        if cls._identity is not None and cls._identity not in cls.model_fields:
            raise ValueError(f"{cls.__name__}: identity must name a resource field")
        if cls._collection is not None:
            if not isinstance(cls._collection, str) or not cls._collection.startswith("/"):
                raise ValueError(f"{cls.__name__}: collection must be an absolute route path")
            for name in re.findall(r"\{([A-Za-z_][A-Za-z0-9_]*)\}", cls._collection):
                if name not in cls.model_fields or name == cls._identity:
                    raise ValueError(f"{cls.__name__}: collection scopes must name non-identity resource fields")

    @classmethod
    def _input_record(
        cls, role: Literal["create", "update"], name: str, *, doc: str, fields: tuple[str, ...] | None = None
    ) -> type[Record]:
        definitions = {}
        for field_name, info in cls.model_fields.items():
            marker = next((item for item in info.metadata if isinstance(item, ResourceField)), None)
            if marker is None:
                continue
            selected = getattr(marker, role)
            if selected is None:
                continue
            annotation = marker.input_type
            if annotation is PydanticUndefined:
                annotation = info.rebuild_annotation()
                if get_origin(annotation) is Annotated:
                    base, *metadata = get_args(annotation)
                    metadata = [item for item in metadata if not isinstance(item, ResourceField)]
                    annotation = Annotated[base, *metadata] if metadata else base
            default = selected.default
            if role == "update":
                default = MISSING
            if default is MISSING:
                annotation = annotation | MISSING
            definitions[field_name] = (
                annotation,
                Field(
                    default=default,
                    description=selected.doc if selected.doc is not None else info.description,
                    json_schema_extra=deepcopy(info.json_schema_extra)
                    if marker.input_type is PydanticUndefined
                    else None,
                ),
            )
        if fields is not None:
            if len(fields) != len(set(fields)) or set(fields) != set(definitions):
                raise ValueError(f"{cls.__name__}: {role} fields must list each selected field exactly once")
            definitions = {key: definitions[key] for key in fields}
        if role in cls._input_records:
            raise ValueError(f"{cls.__name__}: {role} record already declared")
        result = create_model(name, __base__=Record, __module__=cls.__module__, __doc__=doc, **definitions)
        cls._input_records[role] = result
        return result

    @classmethod
    def create_record(cls, name: str, *, doc: str, fields: tuple[str, ...] | None = None) -> type[Record]:
        return cls._input_record("create", name, doc=doc, fields=fields)

    @classmethod
    def update_record(cls, name: str, *, doc: str, fields: tuple[str, ...] | None = None) -> type[Record]:
        return cls._input_record("update", name, doc=doc, fields=fields)

    @classmethod
    def identity_record(cls, name: str, *, doc: str, field_doc: str) -> type[Record]:
        if cls._identity not in cls.model_fields:
            raise ValueError(f"{cls.__name__}: identity must name a resource field")
        info = deepcopy(cls.model_fields[cls._identity])
        info.description = field_doc
        return create_model(
            name,
            __base__=Record,
            __module__=cls.__module__,
            __doc__=doc,
            **{cls._identity: (info.rebuild_annotation(), Field(description=field_doc))},
        )

    @classmethod
    def list_record(
        cls, name: str, *, doc: str, items: str, items_doc: str, extra: dict[str, tuple[Any, FieldInfo]] | None = None
    ) -> type[Record]:
        """A collection result, with explicit resource-specific metadata after its items."""
        if items in (extra or {}):
            raise ValueError(f"{cls.__name__}: list metadata duplicates the items field {items}")
        return create_model(
            name,
            __base__=Record,
            __module__=cls.__module__,
            __doc__=doc,
            **{items: (list[cls], Field(description=items_doc)), **(extra or {})},
        )


def register_resource_operation(
    contract,
    resource: type[Resource],
    action: str,
    *,
    name: str,
    doc: str,
    parameters: tuple[Parameter, ...] = (),
    returns: Any = PydanticUndefined,
    body: str | None = None,
    core_only: bool = False,
    source_module: str,
    source_file: str,
    source_line: int,
    **options,
) -> None:
    """Expand one standard operation at its declaration's position in registration order."""
    if not isinstance(resource, type) or not issubclass(resource, Resource):
        raise TypeError("resource operation requires a Resource record")
    if not isinstance(name, str) or not name.isidentifier() or keyword.iskeyword(name):
        raise ValueError("standard operation name must be a Python identifier")
    if any(not isinstance(parameter, Parameter) for parameter in parameters):
        raise TypeError("resource operation parameters must be Parameter declarations")
    methods = {"read": "GET", "list": "GET", "create": "POST", "update": "PATCH", "delete": "DELETE"}
    if action not in methods:
        raise ValueError(f"{resource.__name__}: unknown standard operation {action!r}")
    collection, identity = resource._collection, resource._identity
    if not collection or not collection.startswith("/") or identity not in resource.model_fields:
        raise ValueError(f"{resource.__name__}: standard operations require a collection and a field identity")
    path = collection if action in {"list", "create"} else f"{collection}/{{{identity}}}"
    arguments = []
    for field_name in re.findall(r"\{([A-Za-z_][A-Za-z0-9_]*)\}", path):
        if field_name not in resource.model_fields:
            raise ValueError(f"{resource.__name__}: path parameter {field_name} is not a resource field")
        annotation = resource.model_fields[field_name].rebuild_annotation()
        if field_name == identity and resource._identity_type is not PydanticUndefined:
            annotation = resource._identity_type
        arguments.append(Parameter(field_name, annotation if core_only else Annotated[annotation, Path()]))
    if action in {"create", "update"}:
        record = resource._input_records.get(action)
        if record is None:
            raise ValueError(f"{resource.__name__}: declare the {action} record before its operation")
        arguments.append(
            Parameter(
                body or ("request" if action == "create" else "update"),
                record if core_only else Annotated[record, Body()],
            )
        )
    elif body is not None:
        raise ValueError(f"{resource.__name__}: {action} has no input body")
    arguments.extend(parameters)
    if len({argument.name for argument in arguments}) != len(arguments):
        raise ValueError(f"{resource.__name__}: duplicate operation parameter")
    if returns is PydanticUndefined:
        returns = list[resource] if action == "list" else resource

    def declaration():
        raise RuntimeError("contract declarations are never executed")

    declaration.__name__ = name
    declaration.__qualname__ = name
    declaration.__doc__ = inspect.cleandoc(doc)
    declaration.__module__ = source_module
    declaration.__code__ = declaration.__code__.replace(co_filename=source_file, co_firstlineno=source_line)
    declaration.__annotations__ = {argument.name: argument.type for argument in arguments} | {"return": returns}
    declaration.__signature__ = inspect.Signature(
        [
            inspect.Parameter(
                argument.name,
                inspect.Parameter.POSITIONAL_OR_KEYWORD,
                annotation=argument.type,
                default=inspect.Parameter.empty if argument.default is PydanticUndefined else argument.default,
            )
            for argument in arguments
        ],
        return_annotation=returns,
    )
    if core_only:
        contract.core_operation(**options)(declaration)
    else:
        if action == "create":
            options.setdefault("status", 201)
        contract.route(methods[action], path, **options)(declaration)
