"""Values of the contract's types, read from the IR: Hypothesis strategies that make them, and a
check that a value conforms.

Values are what JSON decodes to: str, int, float, bool, None, list and dict. A record is a dict in
which an omitted field is a missing key. Hypothesis's own Pydantic support is not used, because it
loses the constraints on nullable fields and does not understand MISSING.

- Values(ir).valid(type) makes values the contract accepts, and arguments(operation) makes an
  operation's arguments.
- invalid(type) and invalid_arguments(operation) make values that break exactly one rule, which
  every transport must reject as an invalid argument. Null is not generated where a value may be
  left out, because in C null is how a value is left out.
- problems(type, value) says how a value fails to conform, and is empty when it conforms.

A string is Unicode text without U+0000, so that it is valid UTF-8 and passes through C. A float64
is finite, because JSON has no NaN or infinity. Lengths count characters, as Pydantic does.
"""

from __future__ import annotations

import math
import re
from typing import Any

from hypothesis import strategies as st

INT_RANGES = {"int32": (-(2**31), 2**31 - 1), "int64": (-(2**63), 2**63 - 1)}
# Unicode text that is valid UTF-8 and holds no NUL, so that it survives a C string.
CHARACTERS = st.characters(exclude_categories=("Cs",), exclude_characters="\x00")


class Values:
    def __init__(self, ir: dict[str, Any], overrides: dict[str, st.SearchStrategy] | None = None):
        """overrides replaces the values of named types, such as the IDs of things that exist."""
        self.aliases = {a["name"]: a for a in ir["aliases"]}
        self.enums = {e["name"]: e for e in ir["enums"]}
        self.records = {r["name"]: r for r in ir["records"]}
        self.operations = {op["name"]: op for op in ir["operations"]}
        self.overrides = overrides or {}

    # ---- types --------------------------------------------------------------------------------

    def resolve(self, t: dict[str, Any], bindings: dict[str, dict[str, Any]] | None = None) -> dict[str, Any]:
        """The type with aliases and type parameters replaced, keeping constraints written on top."""
        if t["kind"] == "parameter":
            return self.resolve(bindings[t["name"]], bindings)
        if t["kind"] == "named" and t["name"] in self.aliases:
            inner = dict(self.resolve(self.aliases[t["name"]]["type"], bindings))
            inner.update({k: v for k, v in t.items() if k not in ("kind", "name")})
            return inner
        return t

    def fields(self, record: str, arguments: list[dict[str, Any]], bindings=None) -> list[dict[str, Any]]:
        """A record's fields, its base's first, with its type parameters bound."""
        declaration = self.records[record]
        inner = dict(zip(declaration.get("type_parameters", []),
                         [self.bound(a, bindings) for a in arguments]))
        out = self.fields(declaration["base"], []) if "base" in declaration else []
        return out + [{**f, "type": self.bound(f["type"], inner)} for f in declaration["fields"]]

    def bound(self, t: dict[str, Any], bindings) -> dict[str, Any]:
        """The type with its type parameters replaced, for use outside the record that names them."""
        if t["kind"] == "parameter":
            return bindings[t["name"]]
        out = dict(t)
        for key in ("value", "items", "values"):
            if isinstance(out.get(key), dict):
                out[key] = self.bound(out[key], bindings)
        for key in ("alternatives", "arguments"):
            if key in out:
                out[key] = [self.bound(a, bindings) for a in out[key]]
        return out

    # ---- valid values -------------------------------------------------------------------------

    def valid(self, t: dict[str, Any]) -> st.SearchStrategy:
        if t["kind"] == "named" and t["name"] in self.overrides:
            return self.overrides[t["name"]]
        if t["kind"] == "named" and t["name"] in self.aliases:
            return self.valid(self.resolve(t))
        kind = t["kind"]
        if kind in ("string", "secret"):
            if "pattern" in t:
                return st.from_regex(t["pattern"], fullmatch=True, alphabet=CHARACTERS).filter(
                    lambda s: t.get("min_length", 0) <= len(s) <= t.get("max_length", math.inf))
            return st.text(CHARACTERS, min_size=t.get("min_length", 0), max_size=t.get("max_length"))
        if kind in INT_RANGES:
            low, high = INT_RANGES[kind]
            return st.integers(max(low, t.get("ge", low), t.get("gt", low - 1) + 1),
                               min(high, t.get("le", high), t.get("lt", high + 1) - 1))
        if kind == "float64":
            return st.floats(t.get("ge", t.get("gt")), t.get("le", t.get("lt")), allow_nan=False,
                             allow_infinity=False, exclude_min="gt" in t, exclude_max="lt" in t)
        if kind == "bool":
            return st.booleans()
        if kind == "literal":
            return st.sampled_from(t["values"])
        if kind == "nullable":
            return st.none() | self.valid({**self.resolve(t["value"]),
                                           **{k: v for k, v in t.items() if k not in ("kind", "value")}})
        if kind == "list":
            return st.lists(self.valid(t["items"]), max_size=5)
        if kind == "map":
            return st.dictionaries(st.text(CHARACTERS, max_size=10), self.valid(t["values"]), max_size=5)
        if kind == "json":
            return st.recursive(st.none() | st.booleans() | st.integers(INT_RANGES["int64"][0], INT_RANGES["int64"][1])
                                | st.floats(allow_nan=False, allow_infinity=False) | st.text(CHARACTERS),
                                lambda inner: st.lists(inner, max_size=3)
                                | st.dictionaries(st.text(CHARACTERS, max_size=10), inner, max_size=3),
                                max_leaves=10)
        if kind == "union":
            return st.one_of([self.valid(a) for a in t["alternatives"]])
        if kind == "named" and t["name"] in self.enums:
            return st.sampled_from([v["value"] for v in self.enums[t["name"]]["values"]])
        if kind == "named":
            return self.record(t["name"], t.get("arguments", []))
        raise ValueError(f"no values for a {kind} type outside a result")

    def record(self, name: str, arguments: list[dict[str, Any]] | None = None) -> st.SearchStrategy:
        required, optional = {}, {}
        for field in self.fields(name, arguments or []):
            (optional if field.get("omittable") or "default" in field else required)[field["name"]] = \
                self.valid(field["type"])
        return st.fixed_dictionaries(required, optional=optional)

    def arguments(self, operation: str) -> st.SearchStrategy:
        """An operation's arguments by name; a parameter with a default may be left out."""
        required, optional = {}, {}
        for parameter in self.operations[operation]["parameters"]:
            (optional if "default" in parameter else required)[parameter["name"]] = self.valid(parameter["type"])
        return st.fixed_dictionaries(required, optional=optional)

    # ---- invalid values -----------------------------------------------------------------------

    def invalid(self, t: dict[str, Any], nullable: bool = False, may_omit: bool = False) -> st.SearchStrategy | None:
        """Values that break one of the type's rules, or None when there are none to make."""
        t = self.resolve(t)
        kind = t["kind"]
        if kind == "nullable":
            return self.invalid({**self.resolve(t["value"]),
                                 **{k: v for k, v in t.items() if k not in ("kind", "value")}}, nullable=True)
        choices = [] if nullable or may_omit else [st.none()]
        if kind in ("string", "secret"):
            if t.get("min_length", 0) > 0:
                choices.append(st.text(CHARACTERS, max_size=t["min_length"] - 1))
            if "max_length" in t:
                # Hypothesis cannot make long text directly, so repeat a character.
                choices.append(st.tuples(CHARACTERS, st.integers(1, 3)).map(
                    lambda pair: pair[0] * (t["max_length"] + pair[1])))
            if "pattern" in t:
                choices.append(st.text(CHARACTERS).filter(lambda s: not re.fullmatch(t["pattern"], s)))
        elif kind in INT_RANGES:
            low, high = INT_RANGES[kind]
            ge = max(t.get("ge", low), t.get("gt", low - 1) + 1)
            le = min(t.get("le", high), t.get("lt", high + 1) - 1)
            if ge > low:
                choices.append(st.integers(max(low, ge - 1000), ge - 1))
            if le < high:
                choices.append(st.integers(le + 1, min(high, le + 1000)))
        elif kind == "float64":
            choices.append(st.sampled_from([math.nan, math.inf, -math.inf]))
        elif kind == "literal":
            choices.append(st.text(CHARACTERS, max_size=12).filter(lambda s: s not in t["values"]))
        elif kind == "named" and t["name"] in self.enums:
            values = [v["value"] for v in self.enums[t["name"]]["values"]]
            choices.append(st.text(CHARACTERS, max_size=12).filter(lambda s: s not in values))
        elif kind == "named" and t["name"] in self.records:
            fields = self.fields(t["name"], t.get("arguments", []))
            broken = [self.invalid_field(t, fields, f) for f in fields]
            choices += [s for s in broken if s is not None]
        elif kind == "union":
            # Text that is none of the literals and breaks the string alternative's rules.
            strings = [self.resolve(a) for a in t["alternatives"] if self.resolve(a)["kind"] == "string"]
            literals = {v for a in t["alternatives"] if a["kind"] == "literal" for v in a["values"]}
            if len(strings) == 1:
                inner = self.invalid(strings[0], nullable=True)
                if inner is not None:
                    choices.append(inner.filter(lambda s: s not in literals))
        return st.one_of(choices) if choices else None

    def invalid_field(self, t, fields, field) -> st.SearchStrategy | None:
        bad = self.invalid(field["type"], may_omit=field.get("omittable", False) or "default" in field)
        if bad is None:
            return None
        others = self.record(t["name"], t.get("arguments", []))
        return st.tuples(others, bad).map(lambda pair: {**pair[0], field["name"]: pair[1]})

    def invalid_arguments(self, operation: str) -> st.SearchStrategy:
        """(arguments, name): valid arguments except for the one named, which breaks a rule."""
        choices = []
        for parameter in self.operations[operation]["parameters"]:
            bad = self.invalid(parameter["type"], may_omit="default" in parameter)
            if bad is not None:
                name = parameter["name"]
                choices.append(st.tuples(self.arguments(operation), bad).map(
                    lambda pair, name=name: ({**pair[0], name: pair[1]}, name)))
        return st.one_of(choices)

    # ---- conformance --------------------------------------------------------------------------

    def problems(self, t: dict[str, Any], value: Any, path: str = "") -> list[str]:
        """How value fails to be a value of type t; empty when it conforms."""
        where = path or "value"
        t = self.resolve(t)
        kind = t["kind"]
        if kind == "nullable":
            if value is None:
                return []
            return self.problems({**self.resolve(t["value"]),
                                  **{k: v for k, v in t.items() if k not in ("kind", "value")}}, value, path)
        if value is None and kind != "json":
            return [f"{where} is null"]
        if kind in ("string", "secret"):
            if not isinstance(value, str):
                return [f"{where} is not a string"]
            out = []
            if "\x00" in value:
                out.append(f"{where} contains U+0000")
            if len(value) < t.get("min_length", 0):
                out.append(f"{where} is shorter than {t['min_length']}")
            if len(value) > t.get("max_length", math.inf):
                out.append(f"{where} is longer than {t['max_length']}")
            if "pattern" in t and not re.fullmatch(t["pattern"], value):
                out.append(f"{where} does not match {t['pattern']}")
            return out
        if kind in INT_RANGES:
            if not isinstance(value, int) or isinstance(value, bool):
                return [f"{where} is not an integer"]
            low, high = INT_RANGES[kind]
            if not max(low, t.get("ge", low), t.get("gt", low - 1) + 1) <= value <= \
                    min(high, t.get("le", high), t.get("lt", high + 1) - 1):
                return [f"{where} is out of range for {kind}" + (f" {t.get('ge')}..{t.get('le')}" if "ge" in t else "")]
            return []
        if kind == "float64":
            if not isinstance(value, (int, float)) or isinstance(value, bool):
                return [f"{where} is not a number"]
            if not math.isfinite(value):
                return [f"{where} is not finite"]
            out = []
            if ("ge" in t and value < t["ge"]) or ("gt" in t and value <= t["gt"]):
                out.append(f"{where} is below its minimum")
            if ("le" in t and value > t["le"]) or ("lt" in t and value >= t["lt"]):
                out.append(f"{where} is above its maximum")
            return out
        if kind == "bool":
            return [] if isinstance(value, bool) else [f"{where} is not a boolean"]
        if kind == "literal":
            return [] if value in t["values"] and type(value) is type(t["values"][0]) \
                else [f"{where} is not one of {t['values']}"]
        if kind == "list":
            if not isinstance(value, list):
                return [f"{where} is not a list"]
            return [p for i, item in enumerate(value) for p in self.problems(t["items"], item, f"{where}[{i}]")]
        if kind == "map":
            if not isinstance(value, dict):
                return [f"{where} is not an object"]
            return [p for k, item in value.items() for p in self.problems(t["values"], item, f"{where}[{k!r}]")]
        if kind == "json":
            return []
        if kind == "union":
            alternatives = t["alternatives"]
            if "discriminator" in t and isinstance(value, dict):
                alternatives = [a for a in alternatives if not self.problems(
                    self.discriminator_type(a, t["discriminator"]), value.get(t["discriminator"]))]
            found = [self.problems(a, value, path) for a in alternatives]
            if any(not f for f in found):
                return []
            return min(found, key=len) if found else [f"{where} matches no alternative"]
        if kind == "named" and t["name"] in self.enums:
            values = [v["value"] for v in self.enums[t["name"]]["values"]]
            return [] if value in values else [f"{where} is not a {t['name']}"]
        if kind == "named":
            if not isinstance(value, dict):
                return [f"{where} is not a {t['name']} object"]
            fields = self.fields(t["name"], t.get("arguments", []))
            names = {f["name"] for f in fields}
            out = [] if self.records[t["name"]].get("open") else [
                f"{where} has the unknown field {key}" for key in value if key not in names]
            for field in fields:
                if field["name"] in value:
                    out += self.problems(field["type"], value[field["name"]], f"{path}.{field['name']}".lstrip("."))
                elif not field.get("omittable") and "default" not in field:
                    out.append(f"{where} is missing {field['name']}")
            return out
        return [f"{where}: cannot check a {kind}"]

    def discriminator_type(self, alternative: dict[str, Any], name: str) -> dict[str, Any]:
        fields = self.fields(alternative["name"], alternative.get("arguments", []))
        return next(f["type"] for f in fields if f["name"] == name)
