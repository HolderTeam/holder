"""Dictionary IR adapter for generators awaiting migration to the typed compiler."""

from __future__ import annotations

import importlib
import sys
from pathlib import Path
from typing import Any

from authoring.routes import Contract
from compiler.builder import build_ir
from compiler.source import APP_FILE, ContractSource, load_source


def extract(root: Path, package: str) -> dict[str, Any]:
    """Return the typed compiler's IR as the legacy dictionary format."""
    return build_ir(load_source(ContractSource(root, package))).model_dump(exclude_unset=True)


def load_contract(root: Path, package: str) -> Contract | None:
    """The source's FastAPI contract, used to produce its OpenAPI document.

    Call after compilation, which checks and imports the package.
    """
    root = Path(root).resolve()
    if not (root / package / APP_FILE).exists():
        return None
    sys.path.insert(0, str(root))
    try:
        app = importlib.import_module(f"{package}.{Path(APP_FILE).stem}")
    finally:
        sys.path.remove(str(root))
    return next(value for value in vars(app).values() if isinstance(value, Contract))
