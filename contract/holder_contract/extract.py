"""Read the contract into its intermediate representation (IR), the input of every generator.

Each file is first checked against holder_contract.rules, and only then imported. Problems are
collected rather than raised one at a time, so a single run reports everything wrong with the
contract. IR.md describes the result.
"""

from __future__ import annotations

import enum
import importlib
import inspect
import re
import sys
import types
import typing
from pathlib import Path
from typing import Any

from fastapi import params
from fastapi.routing import APIRoute
from pydantic import MISSING, JsonValue, SecretStr
from pydantic.fields import FieldInfo
from pydantic_core import PydanticUndefined

from .routes import Contract
from .rules import Declarations, Problem, check_file
from .vocabulary import Bytes, HolderError, Native, Record, Stream

IR_VERSION = 1
CONSTRAINTS = ("min_length", "max_length", "pattern", "ge", "le", "gt", "lt")
METHODS = ("GET", "POST", "PUT", "PATCH", "DELETE")
# The module of a contract package that declares its operations, as FastAPI routes.
APP_FILE = "app.py"
PLACEHOLDER = re.compile(r"\{([A-Za-z_][A-Za-z0-9_]*)\}")


class ContractError(Exception):
    def __init__(self, problems: list[Problem]):
        self.problems = problems
        super().__init__("\n".join(str(p) for p in problems))


def extract(root: Path, package: str) -> dict[str, Any]:
    """The IR of the contract package `package` in directory `root`."""
    root = Path(root).resolve()
    app_path = root / package / APP_FILE
    files = [f for f in sorted((root / package).rglob("*.py")) if f != app_path]
    problems: list[Problem] = []
    declared: dict[str, tuple[str, Declarations]] = {}
    for path in files:
        display = path.relative_to(root).as_posix()
        module = ".".join(path.relative_to(root).with_suffix("").parts)
        if module.endswith(".__init__"):
            module = module.removesuffix(".__init__")
        found_problems, declarations = check_file(path, display, package)
        problems += found_problems
        declared[module] = (display, declarations)
    if problems:
        raise ContractError(problems)

    sys.path.insert(0, str(root))
    try:
        modules = {}
        for name in declared:
            try:
                modules[name] = importlib.import_module(name)
            except Exception as error:  # noqa: BLE001 - any failure to import is a problem in the contract
                problems.append(Problem(declared[name][0], 1, f"cannot be imported: {error}"))
        app = None
        if app_path.exists():
            try:
                app = importlib.import_module(f"{package}.{app_path.stem}")
            except Exception as error:  # noqa: BLE001 - any failure to import is a problem in the contract
                problems.append(Problem(app_path.relative_to(root).as_posix(), 1, f"cannot be imported: {error}"))
    finally:
        sys.path.remove(str(root))
    if problems:
        raise ContractError(problems)
    return _Builder(modules, declared, app, app_path.relative_to(root).as_posix()).build()


def load_contract(root: Path, package: str) -> Contract | None:
    """The Contract the package's app.py makes, for FastAPI's document of its routes; None without one.

    Call it after extract(), which checks the package and reports any problem in it.
    """
    root = Path(root).resolve()
    if not (root / package / APP_FILE).exists():
        return None
    sys.path.insert(0, str(root))
    try:
        app = importlib.import_module(f"{package}.{Path(APP_FILE).stem}")
    finally:
        sys.path.remove(str(root))
    return next(v for v in vars(app).values() if isinstance(v, Contract))


def _doc(obj: Any) -> str | None:
    text = obj.__dict__.get("__doc__") if isinstance(obj, type) else getattr(obj, "__doc__", None)
    return inspect.cleandoc(text) if text else None


class _Builder:
    def __init__(self, modules: dict[str, types.ModuleType], declared: dict[str, tuple[str, Declarations]],
                 app: types.ModuleType | None = None, app_source: str = APP_FILE):
        self.modules = modules
        self.declared = declared
        self.app = app
        self.app_source = app_source
        self.problems: list[Problem] = []
        self.ir: dict[str, Any] = {"ir_version": IR_VERSION, "aliases": [], "enums": [], "records": [],
                                   "errors": [], "interfaces": [], "operations": [], "http": [],
                                   "http_errors": []}
        self.where = ("", 0)
        self.names: dict[str, tuple[str, int]] = {}

    # ---- top level -------------------------------------------------------------------------

    def build(self) -> dict[str, Any]:
        for module_name, module in self.modules.items():
            display, declarations = self.declared[module_name]
            for name, line in declarations.names:
                self.where = (display, line)
                self.declare(module, name, declarations)
        if self.app is not None:
            self.operations()
        self.check_error_statuses()
        if self.problems:
            raise ContractError(self.problems)
        return self.ir

    def problem(self, message: str) -> None:
        self.problems.append(Problem(self.where[0], self.where[1], message))

    def unique(self, name: str) -> None:
        if name in self.names:
            first = self.names[name]
            self.problem(f"{name} is already declared at {first[0]}:{first[1]}")
        self.names[name] = self.where

    def in_contract(self, obj: Any) -> bool:
        return getattr(obj, "__module__", None) in self.modules

    def declare(self, module: types.ModuleType, name: str, declarations: Declarations) -> None:
        obj = module.__dict__[name]
        source = self.where[0]
        if isinstance(obj, typing.TypeAliasType):
            self.unique(name)
            self.ir["aliases"].append({"name": name, "source": source, "doc": declarations.alias_docs.get(name),
                                       "type": self.ref(obj.__value__, name)})
        elif inspect.isclass(obj) and issubclass(obj, Record):
            self.unique(name)
            self.record(obj, source)
        elif inspect.isclass(obj) and issubclass(obj, enum.StrEnum):
            self.unique(name)
            self.enum(obj, source)
        elif inspect.isclass(obj) and issubclass(obj, HolderError):
            self.unique(name)
            self.error_kind(obj, source, declarations.attribute_docs.get(name, {}))
        elif inspect.isclass(obj) and getattr(obj, "__holder_interface__", False):
            self.unique(name)
            self.interface(obj, source, declarations.class_members.get(name, []))
        else:
            self.problem(f"{name} is not a Record, StrEnum, HolderError or @host_interface")

    # ---- declarations ------------------------------------------------------------------------

    def record(self, cls: type[Record], source: str) -> None:
        if not _doc(cls):
            self.problem(f"{cls.__name__} has no docstring")
        bases = [b for b in cls.__bases__ if issubclass(b, Record) and b is not Record and not _is_generic_base(b)]
        if len(bases) > 1:
            self.problem(f"{cls.__name__} extends more than one record")
        entry: dict[str, Any] = {"name": cls.__name__, "source": source, "doc": _doc(cls)}
        if bases:
            if not self.in_contract(bases[0]):
                self.problem(f"{cls.__name__} extends {bases[0].__name__}, which is not declared in the contract")
            entry["base"] = bases[0].__name__
        parameters = cls.__pydantic_generic_metadata__.get("parameters", ())
        if parameters:
            entry["type_parameters"] = [p.__name__ for p in parameters]
        own = inspect.get_annotations(cls)
        entry["fields"] = [self.record_field(cls, name) for name in cls.model_fields if name in own]
        self.ir["records"].append(entry)

    def record_field(self, cls: type[Record], name: str) -> dict[str, Any]:
        info = cls.model_fields[name]
        where = f"{cls.__name__}.{name}"
        if not info.description:
            self.problem(f"{where} has no documentation; write a string under it")
        out: dict[str, Any] = {"name": name, "doc": info.description,
                               "type": self.ref(info.rebuild_annotation(), where, allow_missing=True)}
        if info.default is MISSING:
            out["omittable"] = True
        elif info.default_factory is not None:
            self.problem(f"{where}: use a plain default, not a default factory")
        elif info.default is not PydanticUndefined:
            out["default"] = self.constant(info.default, where)
        return out

    def enum(self, cls: type[enum.StrEnum], source: str) -> None:
        if not _doc(cls):
            self.problem(f"{cls.__name__} has no docstring")
        self.ir["enums"].append({"name": cls.__name__, "source": source, "doc": _doc(cls),
                                 "values": [{"name": m.name, "value": m.value} for m in cls]})

    def error_kind(self, cls: type[HolderError], source: str, docs: dict[str, str]) -> None:
        if not _doc(cls):
            self.problem(f"{cls.__name__} has no docstring")
        if cls.__bases__ != (HolderError,):
            self.problem(f"{cls.__name__}: an error kind extends HolderError directly")
        hints = typing.get_type_hints(cls, include_extras=True)
        fields = []
        for name in inspect.get_annotations(cls):
            where = f"{cls.__name__}.{name}"
            if name not in docs:
                self.problem(f"{where} has no documentation; write a string under it")
            if name in cls.__dict__:
                self.problem(f"{where}: an error kind's fields have no defaults")
            fields.append({"name": name, "doc": docs.get(name), "type": self.ref(hints[name], where)})
        self.ir["errors"].append({"name": cls.__name__, "source": source, "doc": _doc(cls), "fields": fields})

    def interface(self, cls: type, source: str, members: list[str]) -> None:
        if not _doc(cls):
            self.problem(f"{cls.__name__} has no docstring")
        if typing.Protocol not in cls.__bases__:
            self.problem(f"{cls.__name__}: a host interface is a typing.Protocol")
        methods = [self.callable(cls.__dict__[name], f"{cls.__name__}.{name}", skip_self=True) for name in members]
        self.ir["interfaces"].append({"name": cls.__name__, "source": source, "doc": _doc(cls), "methods": methods})

    def operations(self) -> None:
        """The operations app.py declares through its Contract, in the order it declares them."""
        contracts = [v for v in vars(self.app).values() if isinstance(v, Contract)]
        if len(contracts) != 1:
            self.where = (self.app_source, 1)
            self.problem(f"{APP_FILE} must make exactly one Contract, not {len(contracts)}")
            return
        contract = contracts[0]
        routes = {r.endpoint: r for r in contract.router.routes if isinstance(r, APIRoute)}
        for operation in contract.operations:
            function = operation.function
            self.where = (self.app_source, function.__code__.co_firstlineno)
            self.unique(function.__name__)
            signature = self.callable(function, function.__name__, allow_stream=True)
            entry = {"name": signature.pop("name"), "source": self.app_source, **signature}
            errors = []
            for error in operation.errors:
                if not (inspect.isclass(error) and issubclass(error, HolderError) and self.in_contract(error)):
                    self.problem(f"{function.__name__}: {error!r} is not an error kind declared in the contract")
                else:
                    errors.append(error.__name__)
            entry["errors"] = errors
            entry["daemon_only"] = not operation.core
            self.ir["operations"].append(entry)
            if operation.route:
                self.bind(function, routes[function])
        self.where = (self.app_source, 1)
        for error, status in contract.statuses.items():
            self.bind_error(error, status)

    def callable(self, function: Any, where: str, skip_self: bool = False, allow_stream: bool = False) -> dict[str, Any]:
        doc = _doc(function)
        if not doc:
            self.problem(f"{where} has no docstring")
        summary, _, description = (doc or "").partition("\n\n")
        hints = typing.get_type_hints(function, include_extras=True)
        parameters = []
        for parameter in inspect.signature(function).parameters.values():
            if skip_self and parameter.name == "self":
                continue
            name = f"{where}({parameter.name})"
            if parameter.kind not in (parameter.POSITIONAL_OR_KEYWORD, parameter.KEYWORD_ONLY):
                self.problem(f"{name}: parameters are plain named parameters")
                continue
            if parameter.name not in hints:
                self.problem(f"{name} has no type")
                continue
            out: dict[str, Any] = {"name": parameter.name, "type": self.ref(hints[parameter.name], name)}
            doc_text = _annotated_description(hints[parameter.name])
            if doc_text:
                out["doc"] = doc_text
            if parameter.default is not parameter.empty:
                out["default"] = self.constant(parameter.default, name)
            parameters.append(out)
        if "return" not in hints:
            self.problem(f"{where} has no return type; write -> None for none")
        returns = self.ref(hints.get("return", type(None)), f"{where} result", allow_stream=allow_stream)
        return {"name": where.rpartition(".")[2], "summary": summary.strip() or None,
                "description": description.strip() or None, "parameters": parameters, "returns": returns}

    def bind(self, function: Any, route: APIRoute) -> None:
        name = function.__name__
        method = next(iter(route.methods))
        if method not in METHODS:
            self.problem(f"{name}: the method must be one of {', '.join(METHODS)}, not {method}")
        hints = typing.get_type_hints(function, include_extras=True)
        markers = {}
        for parameter in inspect.signature(function).parameters:
            marker = next((m for m in typing.get_args(hints.get(parameter))[1:]
                           if isinstance(m, params.Param | params.Body)), None)
            if marker is None:
                self.problem(f"{name}({parameter}): say where it comes from, with Path(), Query() or Body()")
            elif isinstance(marker, params.Header | params.Cookie):
                self.problem(f"{name}({parameter}): {type(marker).__name__.lower()} parameters are not supported yet")
            markers[parameter] = marker
        in_path = PLACEHOLDER.findall(route.path)
        for placeholder in in_path:
            if not isinstance(markers.get(placeholder), params.Path):
                self.problem(f"{name}: {{{placeholder}}} in the path is not a Path() parameter")
        for parameter, marker in markers.items():
            if isinstance(marker, params.Path) and parameter not in in_path:
                self.problem(f"{name}({parameter}): a Path() parameter must be in the path")
        body = [p for p, m in markers.items() if isinstance(m, params.Body)]
        if len(body) > 1:
            self.problem(f"{name}: {', '.join(body)} are all Body(); a route has at most one body")
        if body and method in ("GET", "DELETE"):
            self.problem(f"{name}: a {method} has no body, but {body[0]} is Body()")
        for existing in self.ir["http"]:
            if (existing["method"], existing["path"]) == (method, route.path):
                self.problem(f"{name}: {method} {route.path} is already the route of {existing['operation']}")
        if not 200 <= route.status_code <= 299 or route.status_code == 204:
            self.problem(f"{name}: the status must be a 2xx with a body, not {route.status_code}")
        entry = {"operation": name, "method": method, "path": route.path, "path_parameters": in_path,
                 "query": [p for p, m in markers.items() if isinstance(m, params.Query)],
                 "status": route.status_code}
        if body:
            entry["body"] = body[0]
        self.ir["http"].append(entry)

    def bind_error(self, error: Any, status: Any) -> None:
        name = getattr(error, "__name__", repr(error))
        declared = {e["name"] for e in self.ir["errors"]}
        if not (isinstance(error, type) and name in declared and self.in_contract(error)):
            self.problem(f"Contract statuses: {name} is not an error kind declared in the contract")
            return
        if not (isinstance(status, int) and 400 <= status <= 599):
            self.problem(f"Contract statuses: {name}'s status must be a 4xx or 5xx, not {status!r}")
        self.ir["http_errors"].append({"error": name, "status": status})

    def check_error_statuses(self) -> None:
        """Every error kind an HTTP-bound operation may fail with needs a status."""
        statuses = {e["error"] for e in self.ir["http_errors"]}
        operations = {op["name"]: op for op in self.ir["operations"]}
        for binding in self.ir["http"]:
            for error in operations[binding["operation"]]["errors"]:
                if error not in statuses:
                    self.problem(f"{binding['operation']} is a route, so its error kind {error} "
                                 "needs a status in the Contract's statuses")
                    statuses.add(error)

    # ---- types -------------------------------------------------------------------------------

    def ref(self, tp: Any, where: str, allow_missing: bool = False, allow_stream: bool = False) -> dict[str, Any]:
        origin, args = typing.get_origin(tp), typing.get_args(tp)
        if tp is JsonValue:
            return {"kind": "json"}
        if origin is typing.Annotated:
            native = next((item for item in args[1:] if isinstance(item, Native)), None)
            if native:
                out = {"kind": native.kind, **({"format": native.format} if native.format else {})}
            else:
                out = dict(self.ref(args[0], where, allow_missing, allow_stream))
            for item in args[1:]:
                out.update(_constraints(item))
                if isinstance(item, FieldInfo) and item.discriminator:
                    out["discriminator"] = item.discriminator if isinstance(item.discriminator, str) else None
            if native:
                # The type's own bounds are what its kind says already.
                for key in ("ge", "le"):
                    if key in out and out[key] == getattr(native, key):
                        del out[key]
            return out
        if isinstance(tp, typing.TypeAliasType):
            if not self.in_contract(tp):
                self.problem(f"{where}: {tp.__name__} is not declared in the contract")
            return {"kind": "named", "name": tp.__name__}
        if origin in (typing.Union, types.UnionType):
            if MISSING in args and not allow_missing:
                self.problem(f"{where}: only a record's fields may be omittable (MISSING)")
            present = [a for a in args if a is not MISSING]
            nullable = type(None) in present
            alternatives = [self.ref(a, where) for a in present if a is not type(None)]
            if not alternatives:
                self.problem(f"{where}: a union needs a type besides None")
                return {"kind": "none"}
            out = alternatives[0] if len(alternatives) == 1 else {"kind": "union", "alternatives": alternatives}
            return {"kind": "nullable", "value": out} if nullable else out
        if origin is typing.Literal:
            if not (all(isinstance(a, str) for a in args) or all(type(a) is int for a in args)):
                self.problem(f"{where}: a Literal's values are all strings or all integers")
            return {"kind": "literal", "values": list(args)}
        if origin is list:
            return {"kind": "list", "items": self.ref(args[0], where)}
        if origin is dict:
            if args[0] is not str:
                self.problem(f"{where}: a map's keys are strings")
            return {"kind": "map", "values": self.ref(args[1], where)}
        if origin is Stream:
            if not allow_stream:
                self.problem(f"{where}: only an operation's result can be a Stream")
            return {"kind": "stream", "items": self.ref(args[0], where)}
        if tp is Bytes:
            if not allow_stream:
                self.problem(f"{where}: only an operation's result can be Bytes")
            return {"kind": "bytes"}
        if tp is str:
            return {"kind": "string"}
        if tp is bool:
            return {"kind": "bool"}
        if tp is SecretStr:
            return {"kind": "secret"}
        if tp is type(None):
            return {"kind": "none"}
        if tp is int:
            self.problem(f"{where}: a bare int has no size in C; use Int32, Int64 or Timestamp")
            return {"kind": "int64"}
        if tp is float:
            self.problem(f"{where}: a bare float; use Float64")
            return {"kind": "float64"}
        if isinstance(tp, typing.TypeVar):
            return {"kind": "parameter", "name": tp.__name__}
        if inspect.isclass(tp) and issubclass(tp, Record):
            generic = tp.__pydantic_generic_metadata__
            target = generic["origin"] or tp
            if not self.in_contract(target):
                self.problem(f"{where}: {target.__name__} is not declared in the contract")
            out = {"kind": "named", "name": target.__name__}
            if generic["origin"]:
                out["arguments"] = [self.ref(a, where) for a in generic["args"]]
            return out
        if inspect.isclass(tp) and issubclass(tp, enum.StrEnum):
            if not self.in_contract(tp):
                self.problem(f"{where}: {tp.__name__} is not declared in the contract")
            return {"kind": "named", "name": tp.__name__}
        self.problem(f"{where}: {tp!r} is not a type the contract supports")
        return {"kind": "none"}

    def constant(self, value: Any, where: str) -> Any:
        if value is None or isinstance(value, (str, bool, int, float)):
            return value.value if isinstance(value, enum.Enum) else value
        self.problem(f"{where}: a default is None, a string, a number or a boolean")
        return None


def _is_generic_base(cls: type) -> bool:
    return cls.__pydantic_generic_metadata__.get("origin") is not None


def _constraints(item: Any) -> dict[str, Any]:
    if isinstance(item, FieldInfo):
        out: dict[str, Any] = {}
        for meta in item.metadata:
            out.update(_constraints(meta))
        return out
    return {name: getattr(item, name) for name in CONSTRAINTS if getattr(item, name, None) is not None}


def _annotated_description(tp: Any) -> str | None:
    if typing.get_origin(tp) is typing.Annotated:
        for item in typing.get_args(tp)[1:]:
            if isinstance(item, FieldInfo) and item.description:
                return item.description
    return None
