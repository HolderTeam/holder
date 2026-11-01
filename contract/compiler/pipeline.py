"""Compile a declaration package into the canonical, target-independent IR."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from holder_contract.extract import extract

from .ir import ContractView


@dataclass(frozen=True, slots=True)
class ContractSource:
    """A Python package containing contract declarations and an optional app.py."""

    root: Path
    package: str


class Compiler:
    """Run declaration extraction and validate its complete typed result."""

    def compile(self, source: ContractSource) -> ContractView:
        return ContractView.model_validate(extract(source.root, source.package))
