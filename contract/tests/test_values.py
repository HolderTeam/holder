"""The strategies and the conformance check in holder_contract.values, checked against each other
and against Pydantic, which reads the contract the same way the extractor does."""

import math
from pathlib import Path

import pydantic
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from holder_contract.extract import extract
from holder_contract.values import Values

CONTRACT = Path(__file__).resolve().parents[1]
HOLDER = extract(CONTRACT, "interface")
SAMPLE = extract(CONTRACT / "tests" / "fixtures", "sample")


def pydantic_type(name: str):
    import interface.cards
    import interface.links
    import interface.milestones

    if name == "Page":
        return interface.cards.Page[interface.cards.CardListItem]
    for module in (interface.cards, interface.links, interface.milestones):
        if hasattr(module, name):
            return getattr(module, name)
    raise AttributeError(name)


def record_type(ir, record):
    if record.get("type_parameters"):
        # Page[CardListItem], as the contract uses it; the sample's generic records take its first record.
        first = "CardListItem" if ir is HOLDER else ir["records"][0]["name"]
        argument = {"kind": "named", "name": first}
        return {"kind": "named", "name": record["name"], "arguments": [argument] * len(record["type_parameters"])}
    return {"kind": "named", "name": record["name"]}


RECORDS = [(ir, record_type(ir, r)) for ir in (HOLDER, SAMPLE) for r in ir["records"]]
IDS = [t["name"] for _, t in RECORDS]


@pytest.mark.parametrize("ir, t", RECORDS, ids=IDS)
@given(data=st.data())
@settings(max_examples=50)
def test_valid_values_conform(ir, t, data):
    values = Values(ir)
    assert values.problems(t, data.draw(values.valid(t))) == []


@pytest.mark.parametrize("ir, t", RECORDS, ids=IDS)
@given(data=st.data())
@settings(max_examples=50)
def test_invalid_values_do_not_conform(ir, t, data):
    values = Values(ir)
    strategy = values.invalid(t)
    if strategy is None:
        return
    assert values.problems(t, data.draw(strategy)) != []


@pytest.mark.parametrize("name", [r["name"] for r in HOLDER["records"]])
@given(data=st.data())
@settings(max_examples=50)
def test_pydantic_accepts_valid_values_and_rejects_invalid_ones(name, data):
    values = Values(HOLDER)
    t = record_type(HOLDER, next(r for r in HOLDER["records"] if r["name"] == name))
    adapter = pydantic.TypeAdapter(pydantic_type(name))
    adapter.validate_python(data.draw(values.valid(t)))
    strategy = values.invalid(t)
    if strategy is not None:
        with pytest.raises(pydantic.ValidationError):
            adapter.validate_python(data.draw(strategy))


@pytest.mark.parametrize("operation", [op["name"] for op in HOLDER["operations"]])
@given(data=st.data())
@settings(max_examples=50)
def test_arguments_conform_and_invalid_arguments_break_one(operation, data):
    values = Values(HOLDER)
    parameters = {p["name"]: p for p in values.operations[operation]["parameters"]}
    arguments = data.draw(values.arguments(operation))
    assert {name for name, p in parameters.items() if "default" not in p} <= arguments.keys()
    for name, value in arguments.items():
        assert values.problems(parameters[name]["type"], value) == []
    broken, name = data.draw(values.invalid_arguments(operation))
    assert [n for n, v in broken.items() if values.problems(parameters[n]["type"], v)] == [name]


@given(data=st.data())
def test_overrides_replace_a_named_type(data):
    values = Values(HOLDER, overrides={"CardId": st.just("c1")})
    assert data.draw(values.valid({"kind": "named", "name": "CardSummary"}))["card_id"] == "c1"


@pytest.mark.parametrize("value, problem", [
    ({"card_id": "", "title": "t", "deleted_at": None}, "card_id is shorter than 1"),
    ({"card_id": "c", "title": "t"}, "value is missing deleted_at"),
    ({"card_id": "c", "title": "t", "deleted_at": None, "extra": 1}, "value has the unknown field extra"),
    ({"card_id": "c", "title": 5, "deleted_at": None}, "title is not a string"),
    ({"card_id": "c", "title": "a\x00b", "deleted_at": None}, "title contains U+0000"),
    ({"card_id": "c", "title": "t", "deleted_at": True}, "deleted_at is not an integer"),
    ({"card_id": "c", "title": "t", "deleted_at": 2**63}, "deleted_at is out of range for int64"),
])
def test_problems_name_what_is_wrong(value, problem):
    assert problem in Values(HOLDER).problems({"kind": "named", "name": "CardSummary"}, value)


def test_float64_is_finite():
    values = Values(HOLDER)
    assert values.problems({"kind": "float64"}, math.nan) == ["value is not finite"]
    assert values.problems({"kind": "float64"}, 1) == []
