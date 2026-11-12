"""What every generator shares: the error for an unsupported contract shape, and the generated-file header."""

from __future__ import annotations

from collections.abc import Iterable

from compiler.ir import ContractView


class Unsupported(Exception):
    """The contract uses something a generator does not support yet."""


def contract_sources(contract: ContractView) -> list[str]:
    """The declaring modules of the contract's records, operations, errors and aliases."""
    views = [*contract.records, *contract.operations, *contract.errors, *contract.aliases,
             *contract.interfaces]
    return sorted({view.source for view in views if view.source is not None})


def source_summary(sources: Iterable[str]) -> str:
    """The contract's parts the sources come from, such as contract/interface (cards, errors.py): a
    list of every module would run to dozens of paths."""
    parts: dict[str, set[str]] = {}
    for source in sources:
        package, _, rest = source.partition("/")
        parts.setdefault(package, set()).add(rest.split("/", 1)[0])
    return "; ".join(f"contract/{package} ({', '.join(sorted(names))})" for package, names in sorted(parts.items()))


def header_lines(sources: Iterable[str], what: str, style: str) -> list[str]:
    """A generated file's opening comment: where it comes from, not to edit it, and what it is."""
    text = [f"Generated from {source_summary(sources)} by compiler.",
            "Do not edit; change the contract, then run make -C contract update.", "", what]
    return [("// " if style == "cpp" else " * ") + line if line else ("//" if style == "cpp" else " *") for line in text]
