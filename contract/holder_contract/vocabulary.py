"""The words Holder's contract is written in.

Contract files import these to declare records, operations, error kinds, host interfaces and HTTP
bindings. Nothing here does any work at run time: decorators only mark what they decorate, and the
extractor (holder_contract.extract) reads the marks.
"""

from __future__ import annotations

import inspect
from collections.abc import Callable
from dataclasses import dataclass, field
from typing import Annotated, Any

from pydantic import BaseModel, ConfigDict, Field
from pydantic_core import core_schema


class Record(BaseModel):
    """Base for every record. Field documentation is the string written under each field.

    A record rejects fields it does not have, in requests and in results alike. A generic record's
    use is named for its arguments, as CardListItemPage for Page[CardListItem].
    """

    model_config = ConfigDict(use_attribute_docstrings=True, extra="forbid", frozen=True)

    @classmethod
    def model_parametrized_name(cls, params: tuple[type[Any], ...]) -> str:
        return "".join(getattr(p, "__name__", str(p)) for p in params) + cls.__name__


@dataclass(frozen=True)
class Native:
    """Marks a native type in its metadata: its kind and format in the IR, and its own bounds."""

    kind: str
    format: str | None = None
    ge: int | None = None
    le: int | None = None


def _integer(kind: str, bits: int, format: str | None = None) -> Any:
    low, high = -(2 ** (bits - 1)), 2 ** (bits - 1) - 1
    return Annotated[int, Native(kind, format, low, high), Field(ge=low, le=high, json_schema_extra={"format": kind})]


# Native types. A bare int or float has no size in C, so the extractor rejects it. Each is a plain
# Annotated value, not a `type` statement, so that a constraint written on one (Annotated[Int32,
# Field(ge=1)]) stays where it is written, and JSON Schema shows the type inline, with its format.
Int32 = _integer("int32", 32)
Int64 = _integer("int64", 64)
Float64 = Annotated[float, Native("float64"), Field(allow_inf_nan=False, json_schema_extra={"format": "double"})]
"""A finite number: JSON has no NaN or infinity."""
Timestamp = _integer("int64", 64, "unix-seconds")
"""Seconds since the Unix epoch."""


class HolderError(Exception):
    """Base for error kinds. An error kind may declare fields, with the same types as a record's."""


class _ResultOnly:
    """Accepted by Pydantic anywhere, so that the extractor, not Pydantic, explains where it may go."""

    @classmethod
    def __get_pydantic_core_schema__(cls, source: Any, handler: Any) -> core_schema.CoreSchema:
        return core_schema.any_schema()


class Stream[T](_ResultOnly):
    """An operation's result delivered as a sequence of items, such as an AI run's events."""


class Bytes(_ResultOnly):
    """An operation's result delivered as a stream of bytes, such as an asset's content."""


@dataclass(frozen=True)
class OperationMark:
    errors: tuple[type[HolderError], ...]
    daemon_only: bool


def operation(function: Callable[..., Any] | None = None, /, *,
              errors: list[type[HolderError]] | tuple[type[HolderError], ...] = (),
              daemon_only: bool = False):
    """Declare an operation: a function whose parameters, result and docstring are its contract.

    Its body is only a docstring or `...`. errors lists the error kinds it may fail with, besides
    an invalid argument, which every operation may report. A daemon-only operation is served by the
    daemon and is not part of core's C API.
    """

    def mark(target: Callable[..., Any]) -> Callable[..., Any]:
        target.__holder_operation__ = OperationMark(tuple(errors), daemon_only)
        return target

    return mark(function) if function is not None else mark


def host_interface(cls: type) -> type:
    """Declare an interface the host implements and core calls, such as a keyring provider."""
    cls.__holder_interface__ = True
    return cls


@dataclass(frozen=True)
class HttpBinding:
    operation: Callable[..., Any]
    method: str
    path: str
    query: tuple[str, ...]
    status: int
    line: int


@dataclass(frozen=True)
class HttpErrorBinding:
    error: Any
    status: int
    line: int


@dataclass
class _Bindings:
    by_module: dict[str, list[HttpBinding]] = field(default_factory=dict)
    errors_by_module: dict[str, list[HttpErrorBinding]] = field(default_factory=dict)


BINDINGS = _Bindings()


def http(target: Callable[..., Any], method: str, path: str, *, query: list[str] | tuple[str, ...] = (),
         status: int = 200) -> None:
    """Bind an operation to an HTTP method and path. Core ignores this; the daemon serves it.

    Parameters named in the path come from the path, those listed in query from the query string,
    and at most one remaining parameter is the request body. status is the success status.
    """
    caller = inspect.currentframe().f_back
    module = caller.f_globals["__name__"]
    BINDINGS.by_module.setdefault(module, []).append(
        HttpBinding(target, method, path, tuple(query), status, caller.f_lineno))


def http_error(error: type[HolderError], status: int) -> None:
    """Give an error kind its HTTP status. Core ignores this; the daemon answers with it.

    Every error kind an HTTP-bound operation may fail with needs one. An invalid argument is 400 and
    an unexpected failure 500, without a declaration.
    """
    caller = inspect.currentframe().f_back
    module = caller.f_globals["__name__"]
    BINDINGS.errors_by_module.setdefault(module, []).append(HttpErrorBinding(error, status, caller.f_lineno))
