"""Pydantic and FastAPI vocabulary for declaring Holder's contract."""

try:
    from pydantic import MISSING, Field, JsonValue, SecretStr
except ImportError:
    from pydantic import Field, JsonValue, SecretStr
    from pydantic_core import MISSING

from .resources import Input, Parameter, Resource, ResourceField
from .routes import Contract
from .vocabulary import (
    Bytes,
    Float64,
    HolderError,
    Int32,
    Int64,
    OpenRecord,
    Record,
    Stream,
    Timestamp,
    host_interface,
)

__all__ = [
    "MISSING",
    "Bytes",
    "Contract",
    "Field",
    "Float64",
    "HolderError",
    "Input",
    "Int32",
    "Int64",
    "JsonValue",
    "OpenRecord",
    "Parameter",
    "Record",
    "Resource",
    "ResourceField",
    "SecretStr",
    "Stream",
    "Timestamp",
    "host_interface",
]
