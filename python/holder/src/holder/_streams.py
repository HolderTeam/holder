"""Library errors for the generated client's streams."""

from __future__ import annotations

import json

from .exceptions import (
    APIError,
    AuthenticationError,
    HolderError,
    NotFoundError,
    ProtocolError,
)
from .generated.models import ErrorResponse
from .generated.streams import StreamError


def stream_failure(error: StreamError, unexpected: str) -> HolderError:
    """The library error for a stream the daemon refused, or answered with something else."""
    if 200 <= error.status_code <= 299:
        return ProtocolError(unexpected)
    error_type = {401: AuthenticationError, 404: NotFoundError}.get(
        error.status_code, APIError
    )
    if isinstance(error.parsed, ErrorResponse):
        detail = error.parsed.error
    else:
        # Undocumented statuses are not parsed by generated endpoints.
        try:
            detail = ErrorResponse.from_dict(json.loads(error.content)).error
        except (ValueError, KeyError, TypeError, AttributeError):
            return error_type(error.status_code, "http_error", "Daemon request failed")
    return error_type(error.status_code, detail.code, detail.message)
