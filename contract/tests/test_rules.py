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

contract = Contract(errors=ERROR_TABLE)
"""
_counter = itertools.count()


def problems(tmp_path, *modules: str, app: str | None = None, errors: str = "{}") -> str:
    """The problems found in a contract package made of these modules and, if given, app.py with
    these routes and error table, joined by newlines."""
    package = f"contract_{next(_counter)}"
    directory = tmp_path / package
    directory.mkdir()
    (directory / "__init__.py").write_text("")
    for number, body in enumerate(modules or ("",)):
        (directory / f"m{number}.py").write_text(PREAMBLE + textwrap.dedent(body))
    if app is not None:
        (directory / "app.py").write_text(APP_PREAMBLE.replace("ERROR_TABLE", errors) + textwrap.dedent(app))
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
    assert "helper: operations are routes in a domain's operations.py, not functions in a model module" in found


def test_operation_modules_allow_routes_but_reject_executable_bodies(tmp_path):
    directory = tmp_path / "sample" / "cards"
    directory.mkdir(parents=True)
    module = directory / "operations.py"
    module.write_text('''\
from fastapi import Path
from authoring import Contract

def register(contract: Contract) -> None:
    @contract.route("GET", "/cards")
    def list_cards() -> list[str]:
        """List cards."""
''')
    assert check_file(module, "sample/cards/operations.py", "sample")[0] == []
    module.write_text(module.read_text() + '        print("ran")\n')
    problems, _ = check_file(module, "sample/cards/operations.py", "sample")
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


@pytest.mark.parametrize("errors, message", [
    ("{}", "find_thing is a route, so its error kind Missing needs a status in the Contract's errors"),
    ("{}", "Missing needs a status and a code, such as (404, 4040)"),
    ("{Missing: 404}", "Missing needs (status, code), such as (404, 4040), not 404"),
    ("{Missing: (200, 2000)}", "Missing's status must be a 4xx or 5xx, not 200"),
    ("{Missing: (404, 404)}", "Missing's code must have four digits, not 404"),
    ("{Missing: (404, 4090)}", "Missing's code 4090 must begin with its status 404"),
    ("{Missing: (400, 4000)}", "Missing's code 4000 is the C API's code for an invalid argument"),
    ("{Missing: (404, 4040), ValueError: (400, 4000)}", "ValueError is not an error kind declared in the contract"),
])
def test_error_table_is_checked(tmp_path, errors, message):
    assert message in problems(tmp_path, ERRORS, app=FIND, errors=errors)


def test_error_codes_are_unique(tmp_path):
    found = problems(tmp_path, ERRORS + '''
class Gone(HolderError):
    """Gone for good."""
''', app=FIND, errors="{Missing: (404, 4040), Gone: (404, 4040)}")
    assert "Gone and Missing both have code 4040" in found


def test_routes_with_an_error_table_are_clean(tmp_path):
    assert problems(tmp_path, ERRORS, app=FIND, errors="{Missing: (404, 4040)}") == ""


def test_core_operations_need_no_route(tmp_path):
    assert problems(tmp_path, ERRORS, app='''
@contract.core_operation(errors=[Missing])
def rebuild(name: str) -> None:
    """Rebuild."""
''', errors="{Missing: (404, 4040)}") == ""


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


def test_base_error_codes_match_holder_h():
    """The codes the compiler reserves are the ones holder.h declares."""
    import re
    from pathlib import Path

    from compiler.builder import BASE_ERROR_CODES

    header = (Path(__file__).resolve().parents[2] / "core" / "include" / "holder" / "holder.h").read_text()
    declared = {int(code) for code in re.findall(r"^#define HOLDER_ERROR_\w+ (\d+)$", header, flags=re.MULTILINE)}
    assert declared == set(BASE_ERROR_CODES)


def test_a_documented_string_constant_is_declared(tmp_path):
    package = "contract_constant"
    directory = tmp_path / package
    directory.mkdir()
    (directory / "__init__.py").write_text("")
    (directory / "m0.py").write_text('from typing import Final\n\nNAME: Final = "Home"\n"""The name."""\n')
    ir = extract(tmp_path, package)
    assert ir["constants"] == [{"name": "NAME", "source": f"{package}/m0.py", "doc": "The name.", "value": "Home"}]


@pytest.mark.parametrize("body, message", [
    ('NAME: Final = "Home"\n', "NAME has no docstring"),
    ('name: Final = "Home"\n"""The name."""\n', "contract files hold no assignments"),
    ('NAME: Final = 3\n"""The name."""\n', "contract files hold no assignments"),
    ('NAME: str = "Home"\n"""The name."""\n', "contract files hold no assignments"),
])
def test_only_documented_capitalised_final_strings_are_constants(tmp_path, body, message):
    assert message in problems(tmp_path, "from typing import Final\n\n" + body)


@pytest.mark.parametrize("decorator, signature, message", [
    ('context="opens"', "def open_it(path: str) -> str:", "open_it: an operation that opens a context returns None"),
    ('context="closes"', "def close_it(path: str) -> None:", "close_it: an operation that closes a context takes only"),
    ('context="closes"', "def close_it() -> str:", "close_it: an operation that closes a context returns None"),
])
def test_runtime_operations_are_checked(tmp_path, decorator, signature, message):
    found = problems(tmp_path, app=f'''
@contract.runtime_operation({decorator})
{signature}
    """Do it."""
''')
    assert message in found


def test_runtime_operations_carry_their_context_mode(tmp_path):
    assert problems(tmp_path, app='''
@contract.runtime_operation(context="none")
def check(path: str) -> None:
    """Check."""


@contract.runtime_operation(context="opens")
def open_it(path: str) -> None:
    """Open."""


@contract.runtime_operation(context="closes")
def close_it() -> None:
    """Close."""
''') == ""
