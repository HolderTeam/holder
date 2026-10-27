"""Conformance tests for the Card slice: core through the generated C API, and the daemon over
HTTP, against the contract.

Hypothesis drives a model of a project with the contract's operations, drawing arguments from the
strategies in holder_contract.values, and checks every result two ways: it conforms to the
operation's declared result type, and it is what the model expects. Over HTTP, every response must
also be one its route in interface/app.py declares (see holder_http). The machine talks to Holder
through a transport's call method and a few set-up helpers, and runs once per transport:

- through the C API, with HOLDER_LIBRARY set to a shared libholder built from the same commit (for
  example core/build/libholder.so from cmake -DHOLDER_CORE_BUILD_SHARED=ON). Each run starts on a
  new data directory.
- over HTTP, with HOLDER_DAEMON set to a holderd built from the same commit. One daemon on
  temporary directories serves every run, each in a new project.

A transport without its variable is skipped. HOLDER_CONFORMANCE_EXAMPLES sets how many runs
Hypothesis makes for each (default 25).
"""

from __future__ import annotations

import itertools
import json
import os
import re
import shutil
import tempfile
import time
from pathlib import Path

import pytest
from hypothesis import HealthCheck, event, settings
from hypothesis import strategies as st
from hypothesis.stateful import RuleBasedStateMachine, initialize, invariant, precondition, rule

from holder_capi import Core, OperationError, Unrepresentable
from holder_contract.extract import extract, load_contract
from holder_contract.values import Values
from holder_http import Http, Nonconforming, shared_daemon

CONTRACT = Path(__file__).resolve().parents[2]
IR = extract(CONTRACT, "interface")
CONTRACT_APP = load_contract(CONTRACT, "interface")
LIBRARY = os.environ.get("HOLDER_LIBRARY")
DAEMON = os.environ.get("HOLDER_DAEMON")
needs_library = pytest.mark.skipif(not LIBRARY, reason="set HOLDER_LIBRARY to a shared libholder to run these")
needs_daemon = pytest.mark.skipif(not DAEMON, reason="set HOLDER_DAEMON to a holderd to run these")
# A project's directory comes from its name, and the HTTP runs share a daemon, so each run's is new.
_runs = itertools.count()


# Rules the contract states in words rather than types.
TAG = re.compile(r"[A-Za-z][A-Za-z0-9_/-]*")
HEX_COLOUR = re.compile(r"[0-9A-Fa-f]{3,4}|[0-9A-Fa-f]{6}|[0-9A-Fa-f]{8}")
MAX_CONTENT_PAGE = 1000
# Tags written into content, so that some cards share them; fff is a colour, not a tag.
TAG_POOL = ["draft", "Draft", "idea", "a/b", "x-1", "fff"]
ORDER_KEYS = {
    "card_id_asc": (lambda c: c["card_id"], False),
    "updated_desc": (lambda c: (c["updated_at"], c["card_id"]), True),
    "sort_key_asc": (lambda c: (c["sort_key"], c["card_id"]), False),
}
METADATA = [f["name"] for f in next(r for r in IR["records"] if r["name"] == "CardMetadata")["fields"]]


def valid_tag(tag: str) -> bool:
    return bool(TAG.fullmatch(tag)) and not HEX_COLOUR.fullmatch(tag)


class Cards(RuleBasedStateMachine):
    """The model and its rules; a subclass gives the transport."""

    def __init__(self):
        super().__init__()
        self.core = self.open()
        self.project = self.core.create_project(f"Conformance {next(_runs)}")
        self.cards: dict[str, dict] = {}  # every card created, by ID, as the model expects it
        self.deleted: set[str] = set()
        self.cursors: dict[str, tuple] = {}  # each cursor handed out: its query and the key it stops at

    def open(self):
        raise NotImplementedError

    def teardown(self):
        self.core.close()

    # ---- the model ----------------------------------------------------------------------------

    # Cards and cursors are sampled in the order they were made, never sorted by ID: IDs differ
    # from run to run, and a replayed run must draw the same choices.
    def live(self) -> list[dict]:
        return [c for c in self.cards.values() if c["card_id"] not in self.deleted]

    def values(self) -> Values:
        """Strategies whose IDs are mostly of things that exist."""
        def mostly(known: st.SearchStrategy, name: str) -> st.SearchStrategy:
            return st.one_of(known, known, known, Values(IR).valid({"kind": "named", "name": name}))

        overrides = {"ProjectId": mostly(st.just(self.project), "ProjectId")}
        if self.cards:
            overrides["CardId"] = mostly(st.sampled_from(list(self.cards)), "CardId")
        return Values(IR, overrides=overrides)

    def expect(self, operation: str, kinds: set[str], arguments: dict):
        """Call operation; it must fail with one of kinds, or succeed when kinds is empty.

        Nothing is checked when the transport cannot send the arguments, such as a cursor too long
        for a URL; that happens only for arguments the contract rejects anyway.
        """
        try:
            result = self.core.call(operation, **arguments)
        except Unrepresentable:
            event(f"{operation}: not sent")
            assert kinds, f"{operation} could not send arguments the contract accepts"
            return None
        except OperationError as error:
            event(f"{operation}: {error.kind}")
            assert error.kind in kinds, f"unexpected {error}"
            return None
        event(f"{operation}: succeeded")
        assert not kinds, f"expected {' or '.join(sorted(kinds))}, got {result!r}"
        return result

    def conforms(self, operation: str, result) -> None:
        returns = next(op for op in IR["operations"] if op["name"] == operation)["returns"]
        assert Values(IR).problems(returns, result) == []

    # ---- rules --------------------------------------------------------------------------------

    @initialize(data=st.data(), count=st.integers(0, 8))
    def some_cards(self, data, count):
        """Start with a few cards in a hierarchy, so that listings have something to page through."""
        for _ in range(count):
            card = data.draw(self.values().record("NewCard"))
            card["project_id"] = self.project
            live = [c["card_id"] for c in self.live()]
            card["parent_card_id"] = data.draw(st.none() | st.sampled_from(live)) if live else None
            self.create(card, data)

    @rule(data=st.data())
    def create_card(self, data):
        self.create(data.draw(self.values().record("NewCard")), data)

    def create(self, card: dict, data) -> None:
        if data.draw(st.booleans()):
            card["content"] = card.get("content", "") + " #" + data.draw(st.sampled_from(TAG_POOL))
        parent = card.get("parent_card_id")
        kinds = set()
        if card["project_id"] != self.project or (parent is not None and (
                parent not in self.cards or parent in self.deleted)):
            kinds.add("NotFound")
        before = int(time.time())
        created = self.expect("create_card", kinds, {"card": card})
        after = int(time.time())
        if created is None:
            return
        self.conforms("create_card", created)
        assert created["project_id"] == self.project
        assert created["title"] == card["title"]
        assert created["content"] == card.get("content", "")
        assert created["parent_card_id"] == parent
        assert created["deleted_at"] is None
        assert created["card_id"] not in self.cards
        if "sort_key" in card:
            assert created["sort_key"] == card["sort_key"]
        if "created_at" in card:
            assert created["created_at"] == card["created_at"]
        else:
            assert before <= created["created_at"] <= after
        assert created["updated_at"] == card.get("updated_at", created["created_at"])
        content = created["content"].encode()
        for occurrence in created["tag_occurrences"]:
            written = content[occurrence["byte_start"]:occurrence["byte_end"]].decode()
            assert written[:1] == "#" and written[1:].lower() == occurrence["tag"], occurrence
            assert valid_tag(occurrence["tag"])
        assert sorted(created["tags"]) == sorted({o["tag"] for o in created["tag_occurrences"]})
        self.cards[created["card_id"]] = created

    @rule(data=st.data())
    def get_card(self, data):
        card_id = data.draw(self.values().valid({"kind": "named", "name": "CardId"}))
        live = card_id in self.cards and card_id not in self.deleted
        card = self.expect("get_card", set() if live else {"NotFound"}, {"card_id": card_id})
        if card is not None:
            self.conforms("get_card", card)
            assert card == self.cards[card_id]

    @rule(data=st.data())
    def list_cards(self, data):
        arguments = data.draw(self.values().arguments("list_cards"))
        known_tags = sorted({t for c in self.live() for t in c["tags"]})
        if known_tags and data.draw(st.booleans()):
            arguments["tag"] = data.draw(st.sampled_from(known_tags + [t.upper() for t in known_tags]))
        if self.cursors and data.draw(st.booleans()):
            arguments["cursor"] = data.draw(st.sampled_from(list(self.cursors)))
        self.check_page(arguments)

    @rule(data=st.data(), limit=st.integers(1, 4), order=st.sampled_from(sorted(ORDER_KEYS)))
    def page_through_every_card(self, data, limit, order):
        cursor, seen = None, []
        for _ in range(len(self.cards) + 2):
            page = self.check_page({"project_id": self.project, "order": order, "limit": limit,
                                    **({"cursor": cursor} if cursor else {})})
            seen += [item["card_id"] for item in page["items"]]
            cursor = page["next_cursor"]
            if cursor is None:
                break
        assert seen == [c["card_id"] for c in self.ordered(self.live(), order)]
        event(f"paged through {min(len(seen), 5)}{'+' if len(seen) >= 5 else ''} cards")

    @precondition(lambda self: self.live())
    @rule(data=st.data())
    def delete_subtree(self, data):
        card_id = data.draw(st.sampled_from([c["card_id"] for c in self.live()]))
        self.core.delete_subtree(card_id)
        pending = [card_id]
        while pending:
            parent = pending.pop()
            self.deleted.add(parent)
            pending += [c["card_id"] for c in self.live() if c["parent_card_id"] == parent]

    @rule(data=st.data(), operation=st.sampled_from([op["name"] for op in IR["operations"]]))
    def invalid_arguments_are_rejected(self, data, operation):
        arguments, name = data.draw(self.values().invalid_arguments(operation))
        try:
            self.core.call(operation, **arguments)
        except OperationError as error:
            assert error.kind == "InvalidArgument", f"{name}={arguments[name]!r}: {error}"
        except Unrepresentable:
            return
        else:
            raise AssertionError(f"{operation} accepted {name}={arguments[name]!r}")

    @invariant()
    def cards_are_where_the_model_says(self):
        for card in self.live():
            assert self.core.call("get_card", card_id=card["card_id"]) == card

    # ---- list_cards ---------------------------------------------------------------------------

    def ordered(self, cards: list[dict], order: str) -> list[dict]:
        key, reverse = ORDER_KEYS[order]
        return sorted(cards, key=key, reverse=reverse)

    def check_page(self, arguments: dict) -> dict | None:
        order = arguments.get("order", "card_id_asc")
        limit = arguments.get("limit", 200)
        tag, parent, cursor = arguments.get("tag"), arguments.get("parent"), arguments.get("cursor")
        include_content = arguments.get("include_content", False)
        query = (arguments["project_id"], tag.lower() if tag else None, parent, order)
        kinds = set()
        if tag is not None and not valid_tag(tag):
            kinds.add("InvalidArgument")
        if include_content and limit > MAX_CONTENT_PAGE:
            kinds.add("InvalidArgument")
        if cursor is not None and (cursor not in self.cursors or self.cursors[cursor][0] != query):
            kinds.add("InvalidArgument")
        if arguments["project_id"] != self.project:
            kinds.add("NotFound")
        page = self.expect("list_cards", kinds, arguments)
        if page is None:
            return None
        self.conforms("list_cards", page)

        matching = [c for c in self.live()
                    if (tag is None or tag.lower() in c["tags"])
                    and (parent is None or c["parent_card_id"] == (None if parent == "roots" else parent))]
        matching = self.ordered(matching, order)
        if cursor is not None:
            key, reverse = ORDER_KEYS[order]
            stop = self.cursors[cursor][1]
            matching = [c for c in matching if (key(c) < stop if reverse else key(c) > stop)]
        expected = []
        for card in matching[:limit]:
            item = {name: card[name] for name in METADATA}
            if include_content:
                item["content"] = card["content"]
            if arguments.get("include_child_count", False):
                item["child_count"] = sum(1 for c in self.live() if c["parent_card_id"] == card["card_id"])
            expected.append(item)
        assert page["items"] == expected
        assert (page["next_cursor"] is not None) == (len(matching) > limit)
        if page["next_cursor"] is not None:
            self.cursors[page["next_cursor"]] = (query, ORDER_KEYS[order][0](matching[limit - 1]))
        return page


class CardsThroughTheCApi(Cards):
    def open(self):
        self.data_dir = Path(tempfile.mkdtemp(prefix="holder-conformance-"))
        return Core(IR, LIBRARY, self.data_dir)

    def teardown(self):
        super().teardown()
        shutil.rmtree(self.data_dir, ignore_errors=True)


class CardsOverHttp(Cards):
    def open(self):
        return Http(IR, CONTRACT_APP, shared_daemon(DAEMON))


NOT_UTF8 = pytest.mark.parametrize(
    "text", [b"\xff", b"\xc0\x80", b"\xed\xa0\x80", b"\xf4\x90\x80\x80", b"a\xe2\x82"],
    ids=["invalid byte", "overlong", "surrogate", "above U+10FFFF", "truncated"])


def rejects_text_that_is_not_utf8(transport, project, operations):
    for operation, arguments in operations:
        with pytest.raises(OperationError) as error:
            transport.call(operation, **arguments)
        assert error.value.kind == "InvalidArgument", operation
        assert "must be UTF-8" in error.value.message


@needs_library
@NOT_UTF8
def test_text_that_is_not_utf8_is_rejected_through_the_c_api(tmp_path, text):
    core = Core(IR, LIBRARY, tmp_path)
    try:
        project = core.create_project("UTF-8")
        rejects_text_that_is_not_utf8(core, project, [
            ("get_card", {"card_id": text}),
            ("create_card", {"card": {"project_id": project, "title": text}}),
            ("list_cards", {"project_id": project, "tag": text})])
    finally:
        core.close()


@needs_daemon
@NOT_UTF8
def test_text_that_is_not_utf8_is_rejected_over_http(text):
    # A JSON body is text, so only the path and the query can carry bytes that are not UTF-8.
    http = Http(IR, CONTRACT_APP, shared_daemon(DAEMON))
    project = http.create_project(f"UTF-8 {next(_runs)}")
    rejects_text_that_is_not_utf8(http, project, [
        ("get_card", {"card_id": text}),
        ("list_cards", {"project_id": project, "tag": text})])


CARD = {"card_id": "c1", "project_id": "p1", "title": "A card", "rel_path": "a-card.md", "parent_card_id": None,
        "sort_key": 1.5, "created_at": 1700000000, "updated_at": 1700000000, "deleted_at": None,
        "content": "#idea", "tags": ["idea"], "tag_occurrences": [{"tag": "idea", "byte_start": 0, "byte_end": 5}]}


def failure(code: str) -> dict:
    return {"ok": False, "error": {"code": code, "message": "No."}}


@pytest.mark.parametrize("operation, status, media_type, body", [
    ("get_card", 200, "application/json", {"ok": True, "data": CARD}),
    ("create_card", 201, "application/json", {"ok": True, "data": CARD}),
    ("list_cards", 200, "application/json", {"ok": True, "data": {"items": [], "next_cursor": None}}),
    ("get_card", 404, "application/json", failure("not_found")),
    ("get_card", 409, "application/json", failure("key_missing")),
    ("create_card", 409, "application/json", failure("conflict")),
    ("list_cards", 400, "application/json", failure("invalid_argument")),
    ("get_card", 500, "application/json", failure("internal_error")),
], ids=lambda value: value if isinstance(value, str) else None)
def test_a_response_its_route_declares_conforms(operation, status, media_type, body):
    http = Http(IR, CONTRACT_APP, None)
    assert http.conforming(operation, status, media_type, json.dumps(body).encode()) == body


@pytest.mark.parametrize("operation, status, media_type, body, problem", [
    ("get_card", 201, "application/json", {"ok": True, "data": CARD}, "a status its route does not declare"),
    ("get_card", 418, "application/json", failure("not_found"), "a status its route does not declare"),
    ("get_card", 200, "text/plain", {"ok": True, "data": CARD}, "not application/json"),
    ("get_card", 200, "application/json", CARD, "not its CardResponse"),
    ("get_card", 200, "application/json", {"ok": True, "data": {**CARD, "colour": "red"}}, "not its CardResponse"),
    ("get_card", 200, "application/json", {"ok": True, "data": {**CARD, "created_at": 1.5}}, "not its CardResponse"),
    ("get_card", 200, "application/json", {"ok": True, "data": {**CARD, "created_at": "1700000000"}},
     "not its CardResponse"),
    ("get_card", 200, "application/json", {"ok": True, "data": {k: v for k, v in CARD.items() if k != "tags"}},
     "not its CardResponse"),
    ("get_card", 404, "application/json", {"ok": False, "error": {"code": "not_found"}}, "not its ErrorResponse"),
    ("get_card", 409, "application/json", failure("conflict"), "not one of its error kinds"),
    ("get_card", 404, "application/json", failure("key_missing"), "whose status is 409"),
], ids=lambda value: value if isinstance(value, str) and " " in value else None)
def test_a_response_its_route_does_not_declare_is_nonconforming(operation, status, media_type, body, problem):
    http = Http(IR, CONTRACT_APP, None)
    with pytest.raises(Nonconforming, match=re.escape(problem)):
        http.conforming(operation, status, media_type, json.dumps(body).encode())


SETTINGS = settings(max_examples=int(os.environ.get("HOLDER_CONFORMANCE_EXAMPLES", "25")),
                    stateful_step_count=15, deadline=None,
                    suppress_health_check=[HealthCheck.too_slow, HealthCheck.filter_too_much])
CardsThroughTheCApi.TestCase.settings = SETTINGS
CardsOverHttp.TestCase.settings = SETTINGS
TestCardsThroughTheCApi = needs_library(CardsThroughTheCApi.TestCase)
TestCardsOverHttp = needs_daemon(CardsOverHttp.TestCase)
