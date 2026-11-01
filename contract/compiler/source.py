"""Check and import a supplied contract package before constructing its IR."""

from __future__ import annotations

import importlib
import sys
from dataclasses import dataclass
from pathlib import Path
from types import ModuleType

from .errors import ContractError
from .rules import Declarations, Problem, check_file

APP_FILE = "app.py"


@dataclass(frozen=True, slots=True)
class ContractSource:
    """A Python package containing contract declarations and an optional app.py."""

    root: Path
    package: str


@dataclass(frozen=True, slots=True)
class LoadedSource:
    modules: dict[str, ModuleType]
    declared: dict[str, tuple[str, Declarations]]
    app: ModuleType | None
    app_source: str


def load_source(source: ContractSource) -> LoadedSource:
    """Report declaration problems together, then import the checked package."""
    root = Path(source.root).resolve()
    package = source.package
    if not package.isidentifier():
        raise ContractError([Problem(package, 1, "package must be a Python identifier")])
    package_dir = root / package
    if not package_dir.is_dir():
        raise ContractError([Problem(package, 1, f"contract package does not exist under {root}")])
    app_path = package_dir / APP_FILE
    all_files = sorted(package_dir.rglob("*.py"))
    if not all_files:
        raise ContractError([Problem(package, 1, "contract package has no Python files")])
    files = [path for path in all_files if path != app_path]
    problems: list[Problem] = []
    declared: dict[str, tuple[str, Declarations]] = {}
    for path in files:
        display = path.relative_to(root).as_posix()
        module = ".".join(path.relative_to(root).with_suffix("").parts)
        if module.endswith(".__init__"):
            module = module.removesuffix(".__init__")
        found_problems, declarations = check_file(path, display, package)
        problems += found_problems
        declared[module] = (display, declarations)
    if problems:
        raise ContractError(problems)

    fingerprint = tuple((path.relative_to(root).as_posix(), path.stat().st_mtime_ns, path.stat().st_size)
                        for path in all_files)
    cached = sys.modules.get(package)
    if (cached is None or getattr(cached, "__contract_source_root__", None) != root or
            getattr(cached, "__contract_source_fingerprint__", None) != fingerprint):
        # Python's import cache is keyed by package name; a different source root or an
        # edited declaration must replace the package and all its submodules together.
        for name in tuple(sys.modules):
            if name == package or name.startswith(f"{package}."):
                del sys.modules[name]
        importlib.invalidate_caches()
    sys.path.insert(0, str(root))
    try:
        modules = {}
        for name in declared:
            try:
                modules[name] = importlib.import_module(name)
            except Exception as error:  # noqa: BLE001 - any import failure is a contract problem
                problems.append(Problem(declared[name][0], 1, f"cannot be imported: {error}"))
        app = None
        if app_path.exists():
            try:
                app = importlib.import_module(f"{package}.{app_path.stem}")
            except Exception as error:  # noqa: BLE001 - any import failure is a contract problem
                problems.append(Problem(app_path.relative_to(root).as_posix(), 1, f"cannot be imported: {error}"))
    finally:
        sys.path.remove(str(root))
    if problems:
        raise ContractError(problems)
    imported_package = sys.modules.get(package)
    if imported_package is not None:
        imported_package.__contract_source_root__ = root
        imported_package.__contract_source_fingerprint__ = fingerprint
    return LoadedSource(modules, declared, app, app_path.relative_to(root).as_posix())
