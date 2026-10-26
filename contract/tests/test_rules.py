"""Each rule a contract must follow, broken on purpose."""

import itertools
import sys
import textwrap

import pytest

from holder_contract.extract import ContractError, extract

PREAMBLE = """\
from enum import StrEnum
from typing import Annotated, Literal, Protocol

from pydantic import BaseModel, field_validator

from holder_contract import (MISSING, Field, HolderError, Int32, Record, Stream, host_interface,
                             http, http_error, operation)
"""
_counter = itertools.count()


def problems(tmp_path, *modules: str) -> str:
    """The problems found in a contract package made of these modules, joined by newlines."""
    package = f"contract_{next(_counter)}"
    directory = tmp_path / package
    directory.mkdir()
    (directory / "__init__.py").write_text("")
    for number, body in enumerate(modules):
        (directory / f"m{number}.py").write_text(PREAMBLE + textwrap.dedent(body))
    try:
        extract(tmp_path, package)
    except ContractError as error:
        return str(error)
    return ""


VALID_RECORD = '''
class Thing(Record):
    """A thing."""

    name: str
    """Its name."""
'''


def test_a_valid_contract_has_no_problems(tmp_path):
    assert problems(tmp_path, VALID_RECORD) == ""


@pytest.mark.parametrize("type_name, message", [
    ("int", "a bare int has no size in C"),
    ("float", "a bare float; use Float64"),
    ("set[str]", "is not a type the contract supports"),
    ("dict[Int32, str]", "a map's keys are strings"),
    ("Stream[str]", "only an operation's result can be a Stream"),
])
def test_unsupported_field_types_are_rejected(tmp_path, type_name, message):
    found = problems(tmp_path, f'''
class Thing(Record):
    """A thing."""

    value: {type_name}
    """Its value."""
''')
    assert "Thing.value" in found and message in found


def test_validators_are_rejected(tmp_path):
    found = problems(tmp_path, VALID_RECORD + '''
    @field_validator("name")
    def check(cls, value):
        return value
''')
    assert "m0.py:16: Thing.check: records and error kinds hold only fields" in found


def test_record_settings_are_rejected(tmp_path):
    found = problems(tmp_path, VALID_RECORD + '''
    model_config = {"extra": "forbid"}
''')
    assert "assignments such as model_config are not allowed" in found


def test_assignments_are_rejected_in_favour_of_type_statements(tmp_path):
    found = problems(tmp_path, "CardId = Annotated[str, Field(min_length=1)]\n")
    assert "name a type with a `type` statement" in found


def test_other_imports_are_rejected(tmp_path):
    assert "may import only" in problems(tmp_path, "import os\n")


def test_a_file_that_breaks_the_rules_is_never_run(tmp_path):
    found = problems(tmp_path, 'print("ran")\n')
    assert "contract files hold only declarations" in found
    assert not any(name.endswith(".m0") and name.startswith("contract_") and "print" in str(module)
                   for name, module in sys.modules.items())


def test_operation_bodies_hold_only_a_docstring(tmp_path):
    found = problems(tmp_path, '''
@operation
def count() -> Int32:
    """Count things."""
    return 1
''')
    assert "count: an operation's body may hold only its docstring" in found


def test_plain_functions_are_rejected(tmp_path):
    found = problems(tmp_path, '''
def helper() -> None:
    """Help."""
''')
    assert "a function in the contract must be an @operation" in found


def test_everything_is_documented(tmp_path):
    found = problems(tmp_path, '''
class Thing(Record):
    name: str

class Missing(HolderError):
    pass

@host_interface
class Provider(Protocol):
    """Provides."""

    def fetch(self, key: str) -> str: ...

@operation
def get_thing() -> Thing: ...
''')
    assert "Thing has no docstring" in found
    assert "Thing.name has no documentation" in found
    assert "Missing has no docstring" in found
    assert "Provider.fetch has no docstring" in found
    assert "get_thing has no docstring" in found


def test_only_record_fields_may_be_omittable(tmp_path):
    found = problems(tmp_path, '''
@operation
def touch(name: str | MISSING = MISSING) -> None:
    """Touch."""
''')
    assert "only a record's fields may be omittable" in found


def test_classes_must_be_contract_kinds(tmp_path):
    found = problems(tmp_path, '''
class Loose(BaseModel):
    """Not a Record."""
''')
    assert "Loose is not a Record, StrEnum, HolderError" in found


def test_names_are_unique_across_modules(tmp_path):
    found = problems(tmp_path, VALID_RECORD, VALID_RECORD)
    assert "m1.py:" in found and "Thing is already declared at" in found


OPERATIONS = '''
@operation
def get_thing(thing_id: str) -> None:
    """Get a thing."""

@operation
def put_thing(thing_id: str, name: str, colour: str) -> None:
    """Put a thing."""
'''


@pytest.mark.parametrize("binding, message", [
    ('http(get_thing, "GET", "/things/{id}")', "{id} in the path is not a parameter"),
    ('http(get_thing, "GET", "/things")', "a GET has no body, but thing_id is left for it"),
    ('http(put_thing, "PUT", "/things/{thing_id}")', "name, colour are left for the body"),
    ('http(get_thing, "GET", "/things/{thing_id}", query=["size"])', "query parameter size is not a parameter"),
    ('http(get_thing, "FETCH", "/things/{thing_id}")', "the method must be one of"),
    ('http(get_thing, "GET", "/things/{thing_id}", status=204)', "the status must be a 2xx with a body"),
    ('http(get_thing, "GET", "/things/{thing_id}", status=404)', "the status must be a 2xx with a body"),
])
def test_http_bindings_are_checked(tmp_path, binding, message):
    found = problems(tmp_path, OPERATIONS + binding + "\n")
    assert message in found


ERRORS = '''
class Missing(HolderError):
    """Not there."""

@operation(errors=[Missing])
def find_thing(thing_id: str) -> None:
    """Find a thing."""
'''


@pytest.mark.parametrize("bindings, message", [
    ('http(find_thing, "GET", "/things/{thing_id}")\n',
     "find_thing is bound to HTTP, so its error kind Missing needs an http_error() status"),
    ('http_error(Missing, 200)\n', "the status must be a 4xx or 5xx"),
    ('http_error(Missing, 404)\nhttp_error(Missing, 410)\n', "the error kind already has a status"),
    ('http_error(ValueError, 400)\n', "ValueError is not an error kind declared in the contract"),
])
def test_http_error_statuses_are_checked(tmp_path, bindings, message):
    assert message in problems(tmp_path, ERRORS + bindings)


def test_error_kinds_with_statuses_bind_cleanly(tmp_path):
    binding = 'http_error(Missing, 404)\nhttp(find_thing, "GET", "/things/{thing_id}")\n'
    assert problems(tmp_path, ERRORS + binding) == ""


def test_an_operation_is_bound_once(tmp_path):
    found = problems(tmp_path, OPERATIONS + 'http(get_thing, "GET", "/a/{thing_id}")\n'
                                           'http(get_thing, "GET", "/b/{thing_id}")\n')
    assert "the operation is already bound" in found


def test_every_problem_is_reported_at_once(tmp_path):
    found = problems(tmp_path, '''
class One(Record):
    """One."""

    a: int
    """A."""
    b: float
    """B."""
''')
    assert len(found.splitlines()) == 2
