"""Rules on what a contract file may contain, checked on its source before it is imported.

A model module holds declarations only: imports of the vocabulary, records, enums, error kinds,
host interfaces and `type` statements. Anything that could run logic, such as a validator, an
assignment or a function, is rejected with the line it is on. Operation modules under
operations/ may contain one register function holding only route declarations.
"""

from __future__ import annotations

import ast
import itertools
from dataclasses import dataclass, field
from pathlib import Path

ALLOWED_IMPORTS = {"__future__", "authoring", "enum", "typing", "pydantic"}
ENUM_BASES = {"StrEnum"}


@dataclass(frozen=True)
class Problem:
    source: str
    line: int
    message: str

    def __str__(self) -> str:
        return f"{self.source}:{self.line}: {self.message}"


@dataclass
class Declarations:
    """What a file declares, in source order, with the documentation the AST holds."""

    names: list[tuple[str, int]] = field(default_factory=list)
    attribute_docs: dict[str, dict[str, str]] = field(default_factory=dict)
    alias_docs: dict[str, str] = field(default_factory=dict)
    class_members: dict[str, list[str]] = field(default_factory=dict)
    module_order: tuple[str, ...] | None = None


def _is_docstring(node: ast.stmt) -> bool:
    return isinstance(node, ast.Expr) and isinstance(node.value, ast.Constant) and isinstance(node.value.value, str)


def _is_placeholder(node: ast.stmt) -> bool:
    return isinstance(node, ast.Pass) or (
        isinstance(node, ast.Expr) and isinstance(node.value, ast.Constant) and node.value.value is Ellipsis)


def _declaration_body(body: list[ast.stmt]) -> bool:
    return all(_is_docstring(node) or _is_placeholder(node) for node in body)


def _decorator_name(node: ast.expr) -> str | None:
    target = node.func if isinstance(node, ast.Call) else node
    return target.id if isinstance(target, ast.Name) else None


def _base_name(node: ast.expr) -> str | None:
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        return node.attr
    return None


def _following_docs(body: list[ast.stmt]) -> dict[str, str]:
    """The string written directly under each annotated name: its documentation."""
    docs = {}
    for node, following in itertools.pairwise(body):
        if isinstance(node, (ast.AnnAssign, ast.TypeAlias)) and _is_docstring(following):
            target = node.target if isinstance(node, ast.AnnAssign) else node.name
            if isinstance(target, ast.Name):
                docs[target.id] = following.value.value
    return docs


def check_file(path: Path, display: str, package: str) -> tuple[list[Problem], Declarations]:
    problems: list[Problem] = []
    found = Declarations()

    def problem(node: ast.AST, message: str) -> None:
        problems.append(Problem(display, getattr(node, "lineno", 1), message))

    try:
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=display)
    except SyntaxError as error:
        return [Problem(display, error.lineno or 1, f"syntax error: {error.msg}")], found

    if "operations" in path.parts:
        return _check_operation_file(tree, display, package), found

    found.alias_docs = _following_docs(tree.body)
    for node in tree.body:
        if _is_docstring(node):
            continue
        if isinstance(node, ast.Import):
            for alias in node.names:
                if alias.name.split(".")[0] not in ALLOWED_IMPORTS:
                    problem(node, f"contract files may import only {', '.join(sorted(ALLOWED_IMPORTS))} "
                                  f"and other contract modules, not {alias.name}")
        elif isinstance(node, ast.ImportFrom):
            module = node.module or ""
            if node.level == 0 and module.split(".")[0] not in ALLOWED_IMPORTS | {package}:
                problem(node, f"contract files may import only {', '.join(sorted(ALLOWED_IMPORTS))} "
                              f"and other contract modules, not {module}")
        elif isinstance(node, ast.TypeAlias):
            found.names.append((node.name.id, node.lineno))
        elif (path.parent.name == package and path.name == "__init__.py" and
              isinstance(node, ast.Assign) and len(node.targets) == 1 and
              isinstance(node.targets[0], ast.Name) and node.targets[0].id == "DECLARATION_ORDER"):
            if (not isinstance(node.value, ast.Tuple) or
                    not all(isinstance(value, ast.Constant) and isinstance(value.value, str)
                            for value in node.value.elts)):
                problem(node, "DECLARATION_ORDER must be a tuple of module names")
            else:
                found.module_order = tuple(value.value for value in node.value.elts)
        elif isinstance(node, ast.ClassDef):
            found.names.append((node.name, node.lineno))
            _check_class(node, found, problem)
        elif isinstance(node, ast.FunctionDef):
            problem(node, f"{node.name}: operations are routes in app.py, not functions in a model module")
        elif isinstance(node, (ast.Assign, ast.AnnAssign)):
            problem(node, "contract files hold no assignments; name a type with a `type` statement")
        else:
            problem(node, f"contract files hold only declarations, not {type(node).__name__}")
    return problems, found


def _check_operation_file(tree: ast.Module, display: str, package: str) -> list[Problem]:
    """Keep public operation modules declarative despite their registration functions."""
    problems: list[Problem] = []

    def problem(node: ast.AST, message: str) -> None:
        problems.append(Problem(display, getattr(node, "lineno", 1), message))

    for node in tree.body:
        if _is_docstring(node):
            continue
        if isinstance(node, ast.Import):
            for alias in node.names:
                if alias.name.split(".")[0] not in ALLOWED_IMPORTS | {package, "fastapi"}:
                    problem(node, f"operation modules may not import {alias.name}")
        elif isinstance(node, ast.ImportFrom):
            module = node.module or ""
            if node.level == 0 and module.split(".")[0] not in ALLOWED_IMPORTS | {package, "fastapi"}:
                problem(node, f"operation modules may not import {module}")
        elif isinstance(node, ast.FunctionDef) and node.name == "register":
            if node.decorator_list:
                problem(node, "register takes no decorators")
            for operation in node.body:
                if _is_docstring(operation):
                    continue
                if not isinstance(operation, ast.FunctionDef) or len(operation.decorator_list) != 1 or not (
                        isinstance(operation.decorator_list[0], ast.Call) and
                        isinstance(operation.decorator_list[0].func, ast.Attribute) and
                        isinstance(operation.decorator_list[0].func.value, ast.Name) and
                        operation.decorator_list[0].func.value.id == "contract" and
                        operation.decorator_list[0].func.attr in {"route", "core_operation"}):
                    problem(operation, "register holds only @contract.route or @contract.core_operation declarations")
                elif not _declaration_body(operation.body):
                    problem(operation, f"{operation.name}: an operation body may hold only its docstring")
        elif (display.endswith("/operations/common.py") and isinstance(node, ast.Assign) and
              len(node.targets) == 1 and isinstance(node.targets[0], ast.Name) and node.targets[0].id == "KEYS" and
              isinstance(node.value, ast.List) and all(isinstance(value, ast.Name) for value in node.value.elts)):
            continue
        else:
            problem(node, "operation modules hold imports and one register function only")
    return problems


def _check_class(node: ast.ClassDef, found: Declarations, problem) -> None:
    decorators = [_decorator_name(d) for d in node.decorator_list]
    interface = decorators == ["host_interface"]
    if decorators and not interface:
        problem(node, f"{node.name}: the only class decorator allowed is @host_interface")
    if node.keywords:
        problem(node, f"{node.name}: class keywords such as settings are not allowed; Record sets them")
    enum = any(_base_name(base) in ENUM_BASES for base in node.bases)
    found.attribute_docs[node.name] = _following_docs(node.body)
    members = []
    for item in node.body:
        if _is_docstring(item) or _is_placeholder(item):
            continue
        if isinstance(item, ast.AnnAssign) and isinstance(item.target, ast.Name) and not enum and not interface:
            continue
        if enum and isinstance(item, ast.Assign) and len(item.targets) == 1 and isinstance(item.targets[0], ast.Name) \
                and isinstance(item.value, ast.Constant) and isinstance(item.value.value, str):
            continue
        if interface and isinstance(item, ast.FunctionDef):
            members.append(item.name)
            if item.decorator_list:
                problem(item, f"{node.name}.{item.name}: interface methods take no decorators")
            if not _declaration_body(item.body):
                problem(item, f"{node.name}.{item.name}: an interface method's body may hold only its docstring and `...`")
            continue
        if isinstance(item, (ast.FunctionDef, ast.AsyncFunctionDef)):
            problem(item, f"{node.name}.{item.name}: records and error kinds hold only fields; "
                          "methods and validators are not allowed")
        elif isinstance(item, ast.Assign):
            problem(item, f"{node.name}: assignments such as model_config are not allowed in a class; "
                          "an enum's members are string constants")
        else:
            problem(item, f"{node.name}: classes hold only fields, docstrings and, in an enum, string members")
    found.class_members[node.name] = members
