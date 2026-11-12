"""Mutable dictionary IR fixtures around the typed compiler and generators."""

from pathlib import Path
from typing import Any

from authoring.routes import Contract
from compiler.ir import ContractView
from compiler.pipeline import Compiler, ContractSource
from compiler.source import load_contract
from generators.core import generate_core, render_core
from generators.daemon import generate_daemon as generate_typed_daemon
from generators.daemon import render_daemon


def extract(root: Path, package: str) -> dict[str, Any]:
    """Compile a package, then give legacy test fixtures its dictionary view."""
    return Compiler().compile(ContractSource(root, package)).model_dump(exclude_unset=True)


def generate(ir: dict[str, Any]) -> dict[str, str]:
    """Core's files from a mutable dictionary test fixture, as the generators write them (before
    clang-format), so tests can look for what was emitted."""
    return render_core(ContractView.model_validate(ir))


def generate_formatted(ir: dict[str, Any]) -> dict[str, str]:
    """Core's files from a mutable dictionary test fixture, laid out as core holds them."""
    return generate_core(ContractView.model_validate(ir))


def generate_daemon(ir: dict[str, Any], contract: Contract) -> dict[str, str]:
    """The daemon's files from a mutable dictionary test fixture, as the generators write them
    (before clang-format), so tests can look for what was emitted."""
    return render_daemon(ContractView.model_validate(ir), contract)


def generate_daemon_formatted(ir: dict[str, Any], contract: Contract) -> dict[str, str]:
    """The daemon's files from a mutable dictionary test fixture, laid out as the daemon holds them."""
    return generate_typed_daemon(ContractView.model_validate(ir), contract)


__all__ = ["extract", "generate", "generate_daemon", "generate_daemon_formatted", "generate_formatted", "load_contract"]
