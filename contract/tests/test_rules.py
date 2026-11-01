"""Each rule a contract must follow, broken on purpose: the model modules' rules, and the routes'."""

import itertools
import sys
import textwrap

import pytest

from compiler.errors import ContractError
from compiler.rules import check_file
from holder_contract.extract import extract

PREAMBLE = """\
from enum import StrEnum
from typing import Annotated, Literal, Protocol

from pydantic import BaseModel, field_validator

from authoring import MISSING, Field, HolderError, Int32, Record, Stream, host_interface
"""
APP_PREAMBLE = """\
from typing import Annotated

from fastapi import Body, Header, Path, Query

from authoring import MISSING, Contract, Int32

from .m0 import *  # noqa: F403

contract = Contract(statuses=STATUSES)
"""
_counter = itertools.count()


def problems(tmp_path, *modules: str, app: str | None = None, statuses: str = "{}") -> str:
    """The problems found in a contract package made of these modules and, if given, app.py with
    these routes and error statuses, joined by newlines."""
    package = f"contract_{next(_counter)}"
    directory = tmp_path / package
    directory.mkdir()
    (directory / "__init__.py").write_text("")
    for number, body in enumerate(modules or ("",)):
        (directory / f"m{number}.py").write_text(PREAMBLE + textwrap.dedent(body))
    if app is not None:
        (directory / "app.py").write_text(APP_PREAMBLE.replace("STATUSES", statuses) + textwrap.dedent(app))
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
    assert "m0.py:15: Thing.check: records and error kinds hold only fields" in found


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


def test_functions_are_rejected_in_model_modules(tmp_path):
    found = problems(tmp_path, '''
def helper() -> None:
    """Help."""
''')
    assert "helper: operations are routes in app.py, not functions in a model module" in found


def test_operation_modules_allow_routes_but_reject_executable_bodies(tmp_path):
    directory = tmp_path / "sample" / "operations"
    directory.mkdir(parents=True)
    module = directory / "cards.py"
    module.write_text('''\
from fastapi import Path
from authoring import Contract

def register(contract: Contract) -> None:
    @contract.route("GET", "/cards")
    def list_cards() -> list[str]:
        """List cards."""
''')
    assert check_file(module, "sample/operations/cards.py", "sample")[0] == []
    module.write_text(module.read_text() + '        print("ran")\n')
    problems, _ = check_file(module, "sample/operations/cards.py", "sample")
    assert "an operation body may hold only its docstring" in str(problems[0])


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
''', app='''
@contract.route("GET", "/thing")
def get_thing() -> Int32: ...
''')
    assert "Thing has no docstring" in found
    assert "Thing.name has no documentation" in found
    assert "Missing has no docstring" in found
    assert "Provider.fetch has no docstring" in found
    assert "get_thing has no docstring" in found


def test_only_record_fields_may_be_omittable(tmp_path):
    found = problems(tmp_path, app='''
@contract.core_operation()
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


ROUTE = '''
@contract.route({route})
def get_thing({parameters}) -> None:
    """Get a thing."""
'''


@pytest.mark.parametrize("route, parameters, message", [
    ('"GET", "/things/{id}"', "thing_id: Annotated[str, Path()]", "{id} in the path is not a Path() parameter"),
    ('"GET", "/things"', "thing_id: Annotated[str, Path()]", "get_thing(thing_id): a Path() parameter must be in the path"),
    ('"GET", "/things"', "thing_id: str", "get_thing(thing_id): say where it comes from"),
    ('"GET", "/things"', "thing: Annotated[str, Body()]", "a GET has no body, but thing is Body()"),
    ('"PUT", "/things"', "a: Annotated[str, Body()], b: Annotated[str, Body()]", "a, b are all Body()"),
    ('"FETCH", "/things"', "", "the method must be one of"),
    # FastAPI refuses this one itself, when the app is made.
    ('"GET", "/things", status=204', "", "Status code 204 must not have a response body"),
    ('"GET", "/things", status=404', "", "the status must be a 2xx with a body, not 404"),
])
def test_routes_are_checked(tmp_path, route, parameters, message):
    assert message in problems(tmp_path, app=ROUTE.format(route=route, parameters=parameters))


def test_a_route_is_taken_once(tmp_path):
    found = problems(tmp_path, app=ROUTE.format(route='"GET", "/things"', parameters="") + '''
@contract.route("GET", "/things")
def list_things() -> None:
    """List things."""
''')
    assert "list_things: GET /things is already the route of get_thing" in found


ERRORS = '''
class Missing(HolderError):
    """Not there."""
'''
FIND = '''
@contract.route("GET", "/things/{thing_id}", errors=[Missing])
def find_thing(thing_id: Annotated[str, Path()]) -> None:
    """Find a thing."""
'''


@pytest.mark.parametrize("statuses, message", [
    ("{}", "find_thing is a route, so its error kind Missing needs a status in the Contract's statuses"),
    ("{Missing: 200}", "Missing's status must be a 4xx or 5xx, not 200"),
    ("{Missing: 404, ValueError: 400}", "ValueError is not an error kind declared in the contract"),
])
def test_error_statuses_are_checked(tmp_path, statuses, message):
    assert message in problems(tmp_path, ERRORS, app=FIND, statuses=statuses)


def test_routes_with_error_statuses_are_clean(tmp_path):
    assert problems(tmp_path, ERRORS, app=FIND, statuses="{Missing: 404}") == ""


def test_core_operations_need_no_route(tmp_path):
    assert problems(tmp_path, ERRORS, app='''
@contract.core_operation(errors=[Missing])
def rebuild(name: str) -> None:
    """Rebuild."""
''') == ""


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
