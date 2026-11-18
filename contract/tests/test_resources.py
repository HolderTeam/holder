"""Resource authoring expands to the same declarations, IR and public outputs."""

import sys
import textwrap
from pathlib import Path

import pytest
from pydantic import ValidationError

from compiler.errors import ContractError
from compiler.pipeline import Compiler, ContractSource
from generators.core import render_core
from generators.core.types import CoreTypes
from generators.daemon.contract_json import generate_codec
from generators.daemon.contract_routes import generate_routes
from generators.daemon.openapi import document

CONTRACT = Path(__file__).resolve().parents[1]
IMPORTS = '''
from typing import Annotated
from authoring import MISSING, Field, Input, Record, Resource, ResourceField, Timestamp

type ThingId = Annotated[str, Field(min_length=1)]
"""The identity."""

type ProjectId = Annotated[str, Field(min_length=1)]
"""The owning project."""
'''
RESOURCE_MODELS = IMPORTS + '''
class Thing(Resource, collection="/projects/{project_id}/things", identity="thing_id"):
    """A thing."""

    thing_id: ThingId
    """Its identity."""
    project_id: ProjectId
    """Its project."""
    title: Annotated[str, ResourceField(input_type=Annotated[str, Field(min_length=1)],
                                      create=Input(), update=Input(doc="Replacement title."))]
    """Its title."""
    note: Annotated[str | None, ResourceField(create=Input(default=None), update=Input())]
    """Its note; null clears it."""
    created_at: Annotated[Timestamp, ResourceField(create=Input(default=MISSING, doc="Imported creation time."))]
    """When it was created."""
    computed: bool
    """Computed by the implementation."""

NewThing = Thing.create_record("NewThing", doc="A thing to create.")
ThingUpdate = Thing.update_record("ThingUpdate", doc="Changes to a thing.")
ThingKey = Thing.identity_record("ThingKey", doc="A deleted thing.", field_doc="The deleted identity.")
ThingList = Thing.list_record("ThingList", doc="Things and a cursor.", items="data", items_doc="The things.",
                             extra={"cursor": (str | None, Field(description="The next cursor."))})
'''
MANUAL_MODELS = IMPORTS + '''
class Thing(Record):
    """A thing."""

    thing_id: ThingId
    """Its identity."""
    project_id: ProjectId
    """Its project."""
    title: str
    """Its title."""
    note: str | None
    """Its note; null clears it."""
    created_at: Timestamp
    """When it was created."""
    computed: bool
    """Computed by the implementation."""

class NewThing(Record):
    """A thing to create."""

    title: Annotated[str, Field(min_length=1)]
    """Its title."""
    note: str | None = None
    """Its note; null clears it."""
    created_at: Timestamp | MISSING = MISSING
    """Imported creation time."""

class ThingUpdate(Record):
    """Changes to a thing."""

    title: Annotated[str, Field(min_length=1)] | MISSING = MISSING
    """Replacement title."""
    note: str | None | MISSING = MISSING
    """Its note; null clears it."""

class ThingKey(Record):
    """A deleted thing."""

    thing_id: ThingId
    """The deleted identity."""

class ThingList(Record):
    """Things and a cursor."""

    data: list[Thing]
    """The things."""
    cursor: str | None
    """The next cursor."""
'''
OPERATION_IMPORTS = '''
from typing import Annotated
from fastapi import Body, Path, Query
from authoring import Contract, Int32, Parameter
from .models import Thing, ThingId, ProjectId, NewThing, ThingUpdate, ThingKey, ThingList

def register(contract: Contract) -> None:
'''
RESOURCE_OPERATIONS = OPERATION_IMPORTS + '''
    contract.resource(Thing, "read", name="get_thing", doc="Get a thing.")
    contract.resource(Thing, "create", name="create_thing", doc="Create a thing.")
    contract.resource(Thing, "update", name="update_thing", doc="Update a thing.")
    contract.resource(Thing, "delete", name="delete_thing", doc="Delete a thing.", returns=ThingKey)
    contract.resource(Thing, "list", name="list_things", doc="List things.", returns=ThingList,
                      parameters=(Parameter("limit", Annotated[Int32, Query(ge=1)], default=3),))
    contract.resource(Thing, "list", name="list_things_local", doc="List locally.", core_only=True,
                      returns=ThingList)
    contract.resource(Thing, "read", name="get_thing_local", doc="Get locally.", core_only=True)
'''
MANUAL_OPERATIONS = OPERATION_IMPORTS + '''
    @contract.route("GET", "/projects/{project_id}/things/{thing_id}")
    def get_thing(project_id: Annotated[ProjectId, Path()], thing_id: Annotated[ThingId, Path()]) -> Thing:
        """Get a thing."""

    @contract.route("POST", "/projects/{project_id}/things", status=201)
    def create_thing(project_id: Annotated[ProjectId, Path()], request: Annotated[NewThing, Body()]) -> Thing:
        """Create a thing."""

    @contract.route("PATCH", "/projects/{project_id}/things/{thing_id}")
    def update_thing(project_id: Annotated[ProjectId, Path()], thing_id: Annotated[ThingId, Path()],
                     update: Annotated[ThingUpdate, Body()]) -> Thing:
        """Update a thing."""

    @contract.route("DELETE", "/projects/{project_id}/things/{thing_id}")
    def delete_thing(project_id: Annotated[ProjectId, Path()], thing_id: Annotated[ThingId, Path()]) -> ThingKey:
        """Delete a thing."""

    @contract.route("GET", "/projects/{project_id}/things")
    def list_things(project_id: Annotated[ProjectId, Path()], limit: Annotated[Int32, Query(ge=1)] = 3) -> ThingList:
        """List things."""

    @contract.core_operation()
    def list_things_local(project_id: ProjectId) -> ThingList:
        """List locally."""

    @contract.core_operation()
    def get_thing_local(project_id: ProjectId, thing_id: ThingId) -> Thing:
        """Get locally."""
'''


def compile_package(root, models=RESOURCE_MODELS, operations=RESOURCE_OPERATIONS):
    package = root / "resource_example"
    package.mkdir(parents=True)
    (package / "__init__.py").write_text("")
    (package / "models.py").write_text(textwrap.dedent(models))
    (package / "operations.py").write_text(textwrap.dedent(operations))
    (package / "app.py").write_text('''
from authoring import Contract
from .operations import register
contract = Contract()
register(contract)
''')
    return Compiler().compile(ContractSource(root, package.name))


def test_resource_sugar_preserves_the_entire_ir_and_generated_interfaces(tmp_path):
    before = compile_package(tmp_path / "before", MANUAL_MODELS, MANUAL_OPERATIONS)
    after = compile_package(tmp_path / "after")
    assert after.model_dump(exclude_unset=True) == before.model_dump(exclude_unset=True)
    assert render_core(after) == render_core(before)
    assert generate_codec(CoreTypes(after), "Contract.h") == generate_codec(CoreTypes(before), "Contract.h")
    assert generate_routes(after, "ContractJson.h") == generate_routes(before, "ContractJson.h")
    assert document(after) == document(before)


def test_input_constraints_defaults_and_omit_versus_null(tmp_path):
    compile_package(tmp_path)
    module = sys.modules["resource_example.models"]
    assert module.NewThing(title="New").model_dump() == {"title": "New", "note": None}
    assert module.ThingUpdate().model_dump() == {}
    assert module.ThingUpdate(note=None).model_dump() == {"note": None}
    assert module.ThingUpdate(title="Changed").model_dump() == {"title": "Changed"}
    assert module.NewThing(title="New", created_at=0).created_at == 0
    for model, values in [(module.NewThing, {}), (module.NewThing, {"title": ""}),
                          (module.ThingUpdate, {"title": None}), (module.ThingUpdate, {"title": ""}),
                          (module.ThingUpdate, {"computed": True}), (module.ThingUpdate, {"created_at": 0})]:
        with pytest.raises(ValidationError):
            model(**values)


def test_real_card_and_location_inputs_preserve_existing_semantics():
    Compiler().compile(ContractSource(CONTRACT, "interface"))
    cards = sys.modules["interface.cards.models"]
    locations = sys.modules["interface.locations.models"]
    assert cards.CardUpdate().model_dump() == {}
    assert cards.CardUpdate(parent_card_id=None).model_dump() == {"parent_card_id": None}
    assert cards.NewCard(title="Card").model_dump() == {"title": "Card", "content": "", "parent_card_id": None}
    assert locations.NewStorageLocation(project_id="p", name="Assets", provider="local_directory",
                                        location_id="").location_id == ""
    assert locations.StorageLocationUpdate(configuration={}).model_dump() == {"configuration": {}}
    for model, values in [(cards.CardUpdate, {"content": None}), (cards.NewCard, {"title": ""}),
                          (cards.CardUpdate, {"tags": []}), (locations.StorageLocationUpdate, {"name": None}),
                          (locations.StorageLocationUpdate, {"configuration": None}),
                          (locations.StorageLocationUpdate, {"provider": "local_directory"}),
                          (locations.StorageLocationUpdate, {"bound": True})]:
        with pytest.raises(ValidationError):
            model(**values)


@pytest.mark.parametrize("old, new, message", [
    ('identity="thing_id"', 'identity="missing"', "identity must name a resource field"),
    ('/projects/{project_id}/things', '/projects/{missing}/things', "collection scopes must name"),
    ('/projects/{project_id}/things', 'things', "collection must be an absolute route path"),
    ('create=Input()', 'create=True', "Thing.title: create must be an Input"),
    ('update=Input()', 'update=Input(default=None)', "updates omit to keep"),
    ('ResourceField(create=Input(default=None), update=Input())',
     'ResourceField(create=Input()), ResourceField(update=Input())', "only one ResourceField"),
    ('doc="A thing to create.")', 'doc="A thing to create.", fields=("title",))', "each selected field exactly once"),
    ('"NewThing", doc=', '"WrongName", doc=', "must declare its exported name"),
    ('items="data"', 'items="cursor"', "list metadata duplicates"),
    ('update=Input(doc="Replacement title.")', 'update=Input(doc="")', "ThingUpdate.title has no documentation"),
])
def test_resource_declaration_diagnostics(tmp_path, old, new, message):
    assert old in RESOURCE_MODELS
    with pytest.raises(ContractError, match=message) as caught:
        compile_package(tmp_path, RESOURCE_MODELS.replace(old, new))
    assert "resource_example/models.py:" in str(caught.value)


@pytest.mark.parametrize("old, new, message", [
    ('Thing, "read"', 'Thing, "explode"', "unknown standard operation"),
    ('Thing, "read"', 'Thing, "read", body="body"', "read has no input body"),
    ('Thing, "read"', 'Thing, "read", parameters=(Parameter("project_id", str),)', "duplicate operation parameter"),
    ('Thing, "read"', 'NewThing, "read"', "requires a Resource record"),
    ('name="get_thing"', 'name="class"', "standard operation name must be a Python identifier"),
    ('Thing, "read"', 'Thing, "read", parameters=("project_id",)', "must be Parameter declarations"),
])
def test_standard_operation_diagnostics(tmp_path, old, new, message):
    with pytest.raises(ContractError, match=message) as caught:
        compile_package(tmp_path, operations=RESOURCE_OPERATIONS.replace(old, new))
    assert "resource_example/app.py:" in str(caught.value)
