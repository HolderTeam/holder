"""The compiler reads a supplied declaration package, independent of Holder's models."""

from pathlib import Path

import pytest

from compiler.builder import build_ir
from compiler.errors import ContractError
from compiler.ir import NamedType
from compiler.pipeline import Compiler, ContractSource
from compiler.source import load_source


def test_compiler_accepts_an_unrelated_contract_package(tmp_path):
    package = tmp_path / "proof_catalog"
    package.mkdir()
    (package / "__init__.py").write_text("")
    (package / "books.py").write_text('''\
"""A small catalog."""

from authoring import Field, Record
from typing import Annotated

type BookId = Annotated[str, Field(min_length=1)]
"""A book's ID."""

class Book(Record):
    """A catalog entry."""

    book_id: BookId
    """Its ID."""
    title: str
    """Its title."""
''')
    (package / "app.py").write_text('''\
"""Catalog operations."""

from typing import Annotated
from fastapi import Path
from authoring import Contract
from .books import Book, BookId

contract = Contract()

@contract.route("GET", "/books/{book_id}")
def get_book(book_id: Annotated[BookId, Path()]) -> Book:
    """Fetch a book."""
''')

    ir = Compiler().compile(ContractSource(tmp_path, "proof_catalog"))

    assert ir.record("Book").field("title").type.kind == "string"
    assert ir.operation("get_book").returns == NamedType(name="Book")
    assert ir.http[0].path == "/books/{book_id}"


def test_source_root_selects_the_package_when_names_match(tmp_path):
    def write_source(root, record_name):
        package = root / "proof_switch"
        package.mkdir(parents=True)
        (package / "__init__.py").write_text("")
        (package / "models.py").write_text(f'''\
from authoring import Record

class {record_name}(Record):
    """A record from this source root."""

    name: str
    """Its name."""
''')

    first = tmp_path / "first"
    second = tmp_path / "second"
    write_source(first, "FirstRecord")
    write_source(second, "SecondRecord")

    compiler = Compiler()
    assert compiler.compile(ContractSource(first, "proof_switch")).records[0].name == "FirstRecord"
    assert compiler.compile(ContractSource(second, "proof_switch")).records[0].name == "SecondRecord"
    model = second / "proof_switch" / "models.py"
    model.write_text(model.read_text().replace("SecondRecord", "EditedRecordLong"))
    assert compiler.compile(ContractSource(second, "proof_switch")).records[0].name == "EditedRecordLong"


def test_missing_source_package_is_a_compiler_error(tmp_path):
    with pytest.raises(ContractError, match="contract package does not exist"):
        Compiler().compile(ContractSource(tmp_path, "missing_catalog"))


def test_builder_produces_typed_ir_before_legacy_serialization():
    root = Path(__file__).resolve().parents[1]
    ir = build_ir(load_source(ContractSource(root, "interface")))
    assert isinstance(ir.operation("list_cards").returns, NamedType)


def test_declaration_order_is_explicit_and_checked(tmp_path):
    package = tmp_path / "proof_order"
    package.mkdir()
    for module, record in (("first", "First"), ("second", "Second")):
        (package / f"{module}.py").write_text(f'''\
from authoring import Record

class {record}(Record):
    """A catalog record."""

    name: str
    """Its name."""
''')
    manifest = package / "__init__.py"
    manifest.write_text('DECLARATION_ORDER = ("second", "first")\n')
    source = ContractSource(tmp_path, "proof_order")
    assert [record.name for record in Compiler().compile(source).records] == ["Second", "First"]

    manifest.write_text('DECLARATION_ORDER = ("second",)\n')
    with pytest.raises(ContractError, match="DECLARATION_ORDER missing"):
        Compiler().compile(source)
