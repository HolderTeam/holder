"""Core's C API from the dictionary IR: an adapter for callers that still hold one.

The generators live in generators/core/ and read the typed ContractView.
"""

from __future__ import annotations

from typing import Any

from compiler.ir import ContractView
from generators.core import CORE_PATHS, generate_core

__all__ = ["CORE_PATHS", "generate"]


def generate(ir: dict[str, Any]) -> dict[str, str]:
    """The generated files, by name."""
    return generate_core(ContractView.model_validate(ir))
