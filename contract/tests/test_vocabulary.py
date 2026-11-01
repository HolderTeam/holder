"""The vocabulary as Pydantic reads it: native types, closed records and generic names.

FastAPI writes the contract's OpenAPI from Pydantic's reading (#303), so these hold for that as
well as for the extractor.
"""

from pathlib import Path
from typing import Annotated

import pydantic
import pytest

from authoring import Field, Float64, Int32, Int64, Record, Timestamp
from authoring.vocabulary import Native
from holder_contract.extract import extract

INT32 = {"type": "integer", "format": "int32", "minimum": -(2**31), "maximum": 2**31 - 1}
INT64 = {"type": "integer", "format": "int64", "minimum": -(2**63), "maximum": 2**63 - 1}


@pytest.mark.parametrize("native, schema", [
    (Int32, INT32),
    (Int64, INT64),
    (Timestamp, INT64),
    (Float64, {"type": "number", "format": "double"}),
])
def test_native_types_have_formats_and_exact_bounds(native, schema):
    assert pydantic.TypeAdapter(native).json_schema() == schema


def test_a_constraint_on_a_native_type_stays_where_it_is_written():
    assert pydantic.TypeAdapter(Annotated[Int32, Field(ge=1, le=5000)]).json_schema() == {
        **INT32, "minimum": 1, "maximum": 5000}
    assert pydantic.TypeAdapter(Int32).json_schema() == INT32


def test_native_types_check_their_ranges():
    with pytest.raises(pydantic.ValidationError):
        pydantic.TypeAdapter(Int32).validate_python(2**31)
    with pytest.raises(pydantic.ValidationError):
        pydantic.TypeAdapter(Float64).validate_python(float("nan"))


def test_the_extractor_reads_a_native_type_by_its_marker():
    ir = extract(Path(__file__).resolve().parents[1], "interface")
    limit = next(p for op in ir["operations"] if op["name"] == "list_cards"
                 for p in op["parameters"] if p["name"] == "limit")
    assert limit["type"] == {"kind": "int32", "ge": 1, "le": 5000}
    card = next(r for r in ir["records"] if r["name"] == "CardMetadata")
    created_at = next(f for f in card["fields"] if f["name"] == "created_at")
    assert created_at["type"] == {"kind": "int64", "format": "unix-seconds"}
    assert Native("int64", "unix-seconds", -(2**63), 2**63 - 1) in Timestamp.__metadata__


class Thing(Record):
    """A thing."""

    name: str
    """Its name."""


class Box[T](Record):
    """A box of something."""

    item: T
    """What is in it."""


def test_records_reject_fields_they_do_not_have():
    with pytest.raises(pydantic.ValidationError, match="Extra inputs are not permitted"):
        Thing.model_validate({"name": "a", "colour": "red"})
    assert Thing.model_json_schema()["additionalProperties"] is False


def test_a_generic_records_use_is_named_for_its_arguments():
    assert Box[Thing].__name__ == "ThingBox"
    assert Box[Thing].model_json_schema()["title"] == "ThingBox"
