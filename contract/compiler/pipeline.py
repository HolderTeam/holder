"""Compile a declaration package into the canonical, target-independent IR."""

from __future__ import annotations

from .builder import build_ir
from .ir import ContractView
from .source import ContractSource, load_source


class Compiler:
    """Load, extract, and validate a declaration package's complete typed result."""

    def compile(self, source: ContractSource) -> ContractView:
        loaded = load_source(source)
        return build_ir(loaded)
