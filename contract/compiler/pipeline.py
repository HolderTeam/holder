"""Compile a declaration package into the canonical, target-independent IR."""

from __future__ import annotations

from holder_contract.extract import build_ir

from .ir import ContractView
from .source import ContractSource, load_source


class Compiler:
    """Load, extract, and validate a declaration package's complete typed result."""

    def compile(self, source: ContractSource) -> ContractView:
        loaded = load_source(source)
        return ContractView.model_validate(build_ir(loaded))
