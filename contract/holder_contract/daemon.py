"""The daemon's generated files: its HTTP routes, their JSON codec, and their OpenAPI paths."""

from __future__ import annotations

from typing import Any

from .generate import Model
from .http_routes import generate_routes
from .json_codec import generate_codec
from .openapi import convert, dumps

# Included as the daemon includes them: core's C++ declarations from its SDK, the codec beside the routes.
API_HEADER = "api/Contract.h"
CODEC_HEADER = "api/generated/ContractJson.h"


def generate_daemon(ir: dict[str, Any]) -> dict[str, str]:
    """The generated files, by name."""
    model = Model(ir)
    return {"ContractJson.h": generate_codec(model, API_HEADER),
            "ContractRoutes.h": generate_routes(model, CODEC_HEADER),
            "openapi.json": dumps(convert(model))}
