"""The contract's operations, written as FastAPI routes, and what C needs that HTTP does not say.

A contract's app.py makes one Contract and declares every operation through it:

    contract = Contract(statuses={NotFound: 404})

    @contract.route("GET", "/projects/{project_id}/cards/{card_id}", errors=[NotFound])
    def get_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card:
        \"\"\"Fetch a card.\"\"\"

A route is an ordinary FastAPI path operation on contract.router (contract.app is the FastAPI app
that includes it), whose body never runs: the daemon
serves the route, from code generated from the IR. Each parameter says where it comes from, with
FastAPI's Path(), Query(), Header() or Body(). On top of FastAPI, a route names the error kinds it
may fail with (two kinds can share a status, so the status alone does not say which), and whether
core has it (core=False for an operation only the daemon serves). An operation only core's C API
has is declared with @contract.core_operation, and is not a route. Each error kind's HTTP status is
given once, in statuses.

The compiler reads the routes from contract.router, FastAPI's analysis of their paths, methods and
statuses, and the rest from the Contract.
"""

from __future__ import annotations

import inspect
from collections.abc import Callable
from dataclasses import dataclass, field
from functools import cached_property, reduce
from operator import or_
from typing import Any, Literal, get_args, get_origin, get_type_hints

from fastapi import APIRouter, FastAPI
from fastapi.responses import Response, StreamingResponse
from fastapi.sse import EventSourceResponse
from pydantic import BaseModel

from .vocabulary import Bytes, HolderError, Record, Stream


class Envelope[T](BaseModel):
    """A success, as the daemon answers it."""

    ok: Literal[True]
    data: T

    @classmethod
    def model_parametrized_name(cls, params: tuple[type[Any], ...]) -> str:
        return "".join(_type_name(p) for p in params) + "Response"


def _type_name(value: Any) -> str:
    """A type's name in a schema's: a list of TagCount is TagCountList."""
    if get_origin(value) is list:
        return _type_name(get_args(value)[0]) + "List"
    return getattr(value, "__name__", str(value))


class Error(BaseModel):
    """What went wrong."""

    code: str
    message: str


class ErrorResponse(BaseModel):
    """A failure, as the daemon answers it."""

    ok: Literal[False]
    error: Error


class OctetStream(StreamingResponse):
    """A route whose result is bytes."""

    media_type = "application/octet-stream"


@dataclass
class Operation:
    """What the contract says about an operation beyond its signature."""

    function: Callable[..., Any]
    errors: list[type[HolderError]]
    core: bool = True  # whether core's C API has it
    route: bool = True  # whether the daemon serves it over HTTP
    created_when: str | None = None  # boolean result field selecting 201 instead of 200
    public: bool = False
    operation_id: str | None = None
    sse_events: dict[str, type] | None = None
    raw_schema: dict[str, Any] | None = None


@dataclass
class Contract:
    """A contract's app and its operations, in the order they are declared."""

    statuses: dict[type[HolderError], int] = field(default_factory=dict)
    title: str = "Holder API"

    def __post_init__(self) -> None:
        self.router = APIRouter()
        self.operations: list[Operation] = []

    @cached_property
    def app(self) -> FastAPI:
        """The FastAPI app of every route, made when first asked for, after they are declared."""
        app = FastAPI(title=self.title, separate_input_output_schemas=False)
        app.include_router(self.router)
        return app

    def route(self, method: str, path: str, *, errors: list[type[HolderError]] = (), status: int = 200,
              core: bool = True, created_when: str | None = None,
              responses: dict[int, dict[str, Any]] | None = None, public: bool = False,
              raw_media_type: str | None = None,
              raw_schema: dict[str, Any] | None = None,
              operation_id: str | None = None,
              sse_events: dict[str, type] | None = None) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
        """Declare an operation the daemon serves at method and path, answering status on success."""

        def register(function: Callable[..., Any]) -> Callable[..., Any]:
            summary, _, description = (inspect.getdoc(function) or "").partition("\n\n")
            returns = get_type_hints(function, include_extras=True).get("return")
            options: dict[str, Any] = {}
            origin = getattr(returns, "__origin__", None)
            if raw_media_type is not None:
                base = StreamingResponse if returns is Bytes else Response
                response_class = type("RawContractResponse", (base,), {"media_type": raw_media_type})
                options = {"response_class": response_class,
                           "response_model": None if returns is Bytes else returns}
            elif origin is Stream:
                options = {"response_class": EventSourceResponse, "response_model": None}
            elif returns is Bytes:
                options = {"response_class": OctetStream, "response_model": None}
            else:
                options = {"response_model": Envelope[returns]}
            options["responses"] = self.error_responses(errors)
            if responses:
                options["responses"].update(responses)
            if sse_events:
                options["responses"][status] = {"model": reduce(or_, sse_events.values())}
            if raw_media_type == "application/json" and isinstance(returns, type) and issubclass(returns, Record):
                options["responses"][status] = {"model": returns}
            elif raw_media_type and get_origin(returns) is Literal:
                options["responses"][status] = {
                    "content": {raw_media_type: {"schema": {"type": "string", "enum": list(get_args(returns))}}}
                }
            if created_when is not None:
                if status != 200:
                    raise ValueError("created_when requires status 200")
                options["responses"][201] = {"model": Envelope[returns], "description": "Created."}
            self.router.add_api_route(path, function, methods=[method], status_code=status,
                                      summary=summary.strip() or None, description=description.strip() or None,
                                      operation_id=operation_id or function.__name__, **options)
            self.operations.append(Operation(function, list(errors), core=core, created_when=created_when,
                                             public=public, operation_id=operation_id, sse_events=sse_events,
                                             raw_schema=raw_schema))
            return function

        return register

    def error_responses(self, errors: list[type[HolderError]]) -> dict[int | str, dict[str, Any]]:
        """A route's failures: an invalid argument, each of its error kinds by status, and the rest."""
        descriptions: dict[int, list[str]] = {400: ["Invalid argument."]}
        for error in errors:
            if error in self.statuses:  # the extractor reports a kind without one
                descriptions.setdefault(self.statuses[error], []).append(inspect.getdoc(error) or error.__name__)
        descriptions.setdefault(500, []).append("Unexpected failure.")
        return {status: {"model": ErrorResponse, "description": " ".join(texts)}
                for status, texts in sorted(descriptions.items())}

    def core_operation(self, *, errors: list[type[HolderError]] = ()) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
        """Declare an operation only core's C API has; the daemon does not serve it."""

        def register(function: Callable[..., Any]) -> Callable[..., Any]:
            self.operations.append(Operation(function, list(errors), route=False))
            return function

        return register
