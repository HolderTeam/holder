"""What every generator shares: the error for an unsupported contract shape, and the generated-file header."""

from __future__ import annotations

from collections.abc import Iterable

from compiler.ir import ContractView


class Unsupported(Exception):
    """The contract uses something a generator does not support yet."""


def contract_sources(contract: ContractView) -> list[str]:
    """The declaring modules of the contract's records, operations, errors and aliases."""
    views = [*contract.records, *contract.operations, *contract.errors, *contract.aliases]
    return sorted({view.source for view in views if view.source is not None})


def header_lines(sources: Iterable[str], what: str, style: str) -> list[str]:
    """A generated file's opening comment: where it comes from, not to edit it, and what it is."""
    text = [f"Generated from contract/{', contract/'.join(sources)} by compiler.",
            "Do not edit; change the contract, then run make -C contract update.", "", what]
    return [("// " if style == "cpp" else " * ") + line if line else ("//" if style == "cpp" else " *") for line in text]
