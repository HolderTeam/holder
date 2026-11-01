"""The daemon's generated files from the dictionary IR: an adapter for callers that still hold one.

The generators live in generators/daemon/ and read the typed ContractView.
"""

from __future__ import annotations

from typing import Any

from authoring.routes import Contract
from compiler.ir import ContractView
from generators import daemon
from generators.daemon import DAEMON_PATHS, OPENAPI_PATHS, api_documents

__all__ = ["DAEMON_PATHS", "OPENAPI_PATHS", "api_documents", "generate_daemon"]


def generate_daemon(ir: dict[str, Any], contract: Contract) -> dict[str, str]:
    """The generated files, by name, with the contract's OpenAPI paths on their own."""
    return daemon.generate_daemon(ContractView.model_validate(ir), contract)
