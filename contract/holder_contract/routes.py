"""The contract's operations, written as FastAPI routes, and what C needs that HTTP does not say.

A contract's app.py makes one Contract and declares every operation through it:

    contract = Contract(statuses={NotFound: 404})

    @contract.route("GET", "/cards/{card_id}", errors=[NotFound])
    def get_card(card_id: Annotated[CardId, Path()]) -> Card:
        \"\"\"Fetch a card.\"\"\"

A route is an ordinary FastAPI path operation on contract.router (contract.app is the FastAPI app
that includes it), whose body never runs: the daemon
serves the route, from code generated from the IR. Each parameter says where it comes from, with
FastAPI's Path(), Query(), Header() or Body(). On top of FastAPI, a route names the error kinds it
may fail with (two kinds can share a status, so the status alone does not say which), and whether
core has it (core=False for an operation only the daemon serves). An operation only core's C API
has is declared with @contract.core_operation, and is not a route. Each error kind's HTTP status is
given once, in statuses.

The extractor reads the routes from contract.router, FastAPI's analysis of their paths, methods and
statuses, and the rest from the Contract.
"""

from __future__ import annotations

import inspect
from collections.abc import Callable
from dataclasses import dataclass, field
from functools import cached_property
from typing import Any, Literal, get_type_hints

from fastapi import APIRouter, FastAPI
from fastapi.responses import StreamingResponse
from fastapi.sse import EventSourceResponse
from pydantic import BaseModel

from .vocabulary import Bytes, HolderError, Stream


class Envelope[T](BaseModel):
    """A success, as the daemon answers it."""

    ok: Literal[True]
    data: T

    @classmethod
    def model_parametrized_name(cls, params: tuple[type[Any], ...]) -> str:
        return "".join(getattr(p, "__name__", str(p)) for p in params) + "Response"


class ErrorBody(BaseModel):
    """What went wrong."""

    code: str
    message: str


class ErrorResponse(BaseModel):
    """A failure, as the daemon answers it."""

    ok: Literal[False]
    error: ErrorBody


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
              core: bool = True) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
        """Declare an operation the daemon serves at method and path, answering status on success."""

        def register(function: Callable[..., Any]) -> Callable[..., Any]:
            summary, _, description = (inspect.getdoc(function) or "").partition("\n\n")
            returns = get_type_hints(function, include_extras=True).get("return")
            options: dict[str, Any] = {}
            origin = getattr(returns, "__origin__", None)
            if origin is Stream:
                options = {"response_class": EventSourceResponse, "response_model": None}
            elif returns is Bytes:
                options = {"response_class": OctetStream, "response_model": None}
            else:
                options = {"response_model": Envelope[returns]}
            self.router.add_api_route(path, function, methods=[method], status_code=status,
                                      summary=summary.strip() or None, description=description.strip() or None,
                                      operation_id=function.__name__, **options)
            self.operations.append(Operation(function, list(errors), core=core))
            return function

        return register

    def core_operation(self, *, errors: list[type[HolderError]] = ()) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
        """Declare an operation only core's C API has; the daemon does not serve it."""

        def register(function: Callable[..., Any]) -> Callable[..., Any]:
            self.operations.append(Operation(function, list(errors), route=False))
            return function

        return register
