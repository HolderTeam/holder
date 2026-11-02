"""Mutable dictionary IR fixtures around the typed compiler and generators."""

from pathlib import Path
from typing import Any

from authoring.routes import Contract
from compiler.ir import ContractView
from compiler.pipeline import Compiler, ContractSource
from compiler.source import load_contract
from generators.core import generate_core
from generators.daemon import generate_daemon as generate_typed_daemon


def extract(root: Path, package: str) -> dict[str, Any]:
    """Compile a package, then give legacy test fixtures its dictionary view."""
    return Compiler().compile(ContractSource(root, package)).model_dump(exclude_unset=True)


def generate(ir: dict[str, Any]) -> dict[str, str]:
    """Generate core files from a mutable dictionary test fixture."""
    return generate_core(ContractView.model_validate(ir))


def generate_daemon(ir: dict[str, Any], contract: Contract) -> dict[str, str]:
    """Generate daemon files from a mutable dictionary test fixture."""
    return generate_typed_daemon(ContractView.model_validate(ir), contract)


__all__ = ["extract", "generate", "generate_daemon", "load_contract"]
