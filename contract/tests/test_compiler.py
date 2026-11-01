"""The compiler reads a supplied declaration package, independent of Holder's models."""

from compiler.ir import NamedType
from compiler.pipeline import Compiler, ContractSource


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
