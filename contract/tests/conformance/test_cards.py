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
from datetime import date
from pathlib import Path

import pytest
from hypothesis import HealthCheck, event, settings
from hypothesis import strategies as st
from hypothesis.stateful import RuleBasedStateMachine, initialize, invariant, rule

from holder_capi import Core, OperationError, Unrepresentable
from holder_contract.extract import extract, load_contract
from holder_contract.values import CHARACTERS, Values
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


# Rules the contract states in words rather than types. A metadata name is not empty, too.
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
DATETIME = re.compile(r"([0-9]{4}-[0-9]{2}-[0-9]{2})T([0-9]{2}):([0-9]{2}):([0-9]{2})(Z|([+-])([0-9]{2}):([0-9]{2}))")
ROLE_POOL = ["start", "end", "due"]
RESOURCE_ID = re.compile(r"[A-Za-z0-9_-]{4,40}")
REMOTES = [None, "https://example.com/notes.git", "git@example.com:team/notes.git"]
PROJECT_ORDERS = {
    "updated_at_desc": (lambda p: p["updated_at"], True),
    "updated_at_asc": (lambda p: p["updated_at"], False),
    "created_at_desc": (lambda p: p["created_at"], True),
    "created_at_asc": (lambda p: p["created_at"], False),
    "name_desc": (lambda p: p["name"], True),
    "name_asc": (lambda p: p["name"], False),
}
EPOCH = date(1970, 1, 1).toordinal()


def valid_tag(tag: str) -> bool:
    return bool(TAG.fullmatch(tag)) and not HEX_COLOUR.fullmatch(tag)


def calendar_day(text: str) -> date | None:
    """The day a Day names, or None when it names none: YYYY-MM-DD, from year 1."""
    if not re.fullmatch(r"[0-9]{4}-[0-9]{2}-[0-9]{2}", text):
        return None
    try:
        return date.fromisoformat(text)
    except ValueError:
        return None


def epoch_of(text: str) -> int | None:
    """The epoch second a DateTime names, or None when it names none."""
    match = DATETIME.fullmatch(text)
    day = calendar_day(match[1]) if match else None
    if day is None:
        return None
    hour, minute, second = int(match[2]), int(match[3]), int(match[4])
    offset = 0 if match[5] == "Z" else (int(match[7]) * 60 + int(match[8])) * (1 if match[6] == "+" else -1)
    if hour > 23 or minute > 59 or second > 59 or (match[5] != "Z" and (int(match[7]) > 23 or int(match[8]) > 59)):
        return None
    return (day.toordinal() - EPOCH) * 86400 + hour * 3600 + minute * 60 + second - offset * 60


def valid_milestone_date(value: dict) -> bool:
    """Exactly one of a real day or a real instant."""
    if ("date" in value) == ("datetime" in value):
        return False
    return calendar_day(value["date"]) is not None if "date" in value else epoch_of(value["datetime"]) is not None


class Cards(RuleBasedStateMachine):
    """The model and its rules; a subclass gives the transport."""

    def __init__(self):
        super().__init__()
        self.core = self.open()
        # Two projects, so that a card is sometimes asked for in a project it is not in.
        # Resource IDs are unique on a device, and the HTTP runs share one, so a run's new IDs carry its number.
        self.run = next(_runs)
        # Every project a run makes has its prefix in its name, so that a listing can find the run's own.
        self.prefix = f"Conformance {self.run}-"
        self.project_records: dict[str, dict] = {}  # every project the run made, by ID, as the model expects it
        for i in range(2):
            made = self.core.call("create_project", project={"name": f"{self.prefix}{i}", "privacy_mode": "plain"})
            self.project_records[made["project_id"]] = made
        self.projects = list(self.project_records)
        self.cards: dict[str, dict] = {}  # every card created, by ID, as the model expects it
        self.deleted: set[str] = set()
        self.cursors: dict[str, tuple] = {}  # each cursor handed out: its query and the key it stops at
        self.links: dict[str, list[dict]] = {}  # each card's links, by its ID
        self.milestones: dict[str, list[dict]] = {}  # each card's milestones, by its ID, in the order made
        self.resources: dict[str, dict] = {}  # every resource that exists, by ID

    def open(self):
        raise NotImplementedError

    def teardown(self):
        self.core.close()

    # ---- the model ----------------------------------------------------------------------------

    # Cards and cursors are sampled in the order they were made, never sorted by ID: IDs differ
    # from run to run, and a replayed run must draw the same choices.
    def live(self, project: str | None = None) -> list[dict]:
        """The live cards, of one project or of every project."""
        return [c for c in self.cards.values()
                if c["card_id"] not in self.deleted and project in (None, c["project_id"])]

    def in_project(self, card_id: str | None, project: str) -> bool:
        """Whether the card is in the project, live or in the trash."""
        return card_id in self.cards and self.cards[card_id]["project_id"] == project

    def is_live(self, card_id: str | None, project: str) -> bool:
        return self.in_project(card_id, project) and card_id not in self.deleted

    def values(self) -> Values:
        """Strategies whose IDs are mostly of things that exist."""
        def mostly(known: st.SearchStrategy, name: str) -> st.SearchStrategy:
            return st.one_of(known, known, known, Values(IR).valid({"kind": "named", "name": name}))

        overrides = {"ProjectId": mostly(st.sampled_from(self.projects), "ProjectId")}
        if self.cards:
            overrides["CardId"] = mostly(st.sampled_from(list(self.cards)), "CardId")
        if self.resources:
            overrides["ResourceId"] = mostly(st.sampled_from(list(self.resources)), "ResourceId")
        milestone_ids = [m["milestone_id"] for made in self.milestones.values() for m in made]
        if milestone_ids:
            overrides["MilestoneId"] = mostly(st.sampled_from(milestone_ids), "MilestoneId")
        # Days and instants are nearly always real ones: the contract states their form in words.
        def nearly_always(valid: st.SearchStrategy, name: str) -> st.SearchStrategy:
            return st.one_of(*[valid] * 7, Values(IR).valid({"kind": "named", "name": name}))

        days = st.one_of(st.dates(min_value=date(1, 1, 1)),
                         st.dates(min_value=date(2026, 8, 1), max_value=date(2027, 12, 31))).map(date.isoformat)
        offsets = st.one_of(st.just("Z"), st.builds(
            lambda sign, minutes: f"{sign}{minutes // 60:02d}:{minutes % 60:02d}", st.sampled_from("+-"),
            st.integers(0, 23 * 60 + 59)))
        instants = st.builds(lambda day, seconds, offset: f"{day}T{seconds // 3600:02d}:{seconds // 60 % 60:02d}:"
                             f"{seconds % 60:02d}{offset}", days, st.integers(0, 86399), offsets)
        overrides["Day"] = nearly_always(days, "Day")
        overrides["DateTime"] = nearly_always(instants, "DateTime")
        role = st.one_of(st.sampled_from(ROLE_POOL), Values(IR).valid({"kind": "named", "name": "MilestoneRole"}))
        one = st.one_of(st.builds(lambda r, d: {"role": r, "date": d}, role, days),
                        st.builds(lambda r, d: {"role": r, "datetime": d}, role, instants))
        overrides["MilestoneDate"] = nearly_always(one, "MilestoneDate")
        names = st.one_of(st.sampled_from(["identifier", "author", "description"]),
                          st.text(CHARACTERS, min_size=1, max_size=8))
        overrides["ResourceMetadata"] = nearly_always(
            st.dictionaries(names, st.lists(st.text(CHARACTERS, max_size=12), max_size=3), max_size=4),
            "ResourceMetadata")
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
        """Start with a few cards in a hierarchy, so that listings have something to page through, and some
        milestones on them, so that there are some to change."""
        for _ in range(count):
            card = data.draw(self.values().record("NewCard"))
            project = data.draw(st.sampled_from(self.projects))
            live = [c["card_id"] for c in self.live(project)]
            card["parent_card_id"] = data.draw(st.none() | st.sampled_from(live)) if live else None
            self.create(project, card, data)
        for card in data.draw(st.lists(st.sampled_from(self.live()), max_size=4)) if self.live() else []:
            self.add_milestone(card["card_id"], card["project_id"], data.draw(self.values().record("NewMilestone")))
        for _ in range(data.draw(st.integers(0, 3))):
            resource = data.draw(self.values().record("NewResource"))
            resource.pop("resource_id", None)
            self.make_resource(data.draw(st.sampled_from(self.projects)), resource)

    @rule(data=st.data())
    def create_card(self, data):
        project = data.draw(self.values().valid({"kind": "named", "name": "ProjectId"}))
        self.create(project, data.draw(self.values().record("NewCard")), data)

    def create(self, project: str, card: dict, data) -> None:
        if data.draw(st.booleans()):
            card["content"] = card.get("content", "") + " #" + data.draw(st.sampled_from(TAG_POOL))
        parent = card.get("parent_card_id")
        kinds = set()
        if project not in self.projects or (parent is not None and not self.is_live(parent, project)):
            kinds.add("NotFound")
        before = int(time.time())
        created = self.expect("create_card", kinds, {"project_id": project, "card": card})
        after = int(time.time())
        if created is None:
            return
        self.conforms("create_card", created)
        assert created["project_id"] == project
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
        self.check_tags(created)
        self.cards[created["card_id"]] = created

    def check_tags(self, card: dict) -> None:
        """The card's tags are the ones written in its content."""
        content = card["content"].encode()
        for occurrence in card["tag_occurrences"]:
            written = content[occurrence["byte_start"]:occurrence["byte_end"]].decode()
            assert written[:1] == "#" and written[1:].lower() == occurrence["tag"], occurrence
            assert valid_tag(occurrence["tag"])
        assert sorted(card["tags"]) == sorted({o["tag"] for o in card["tag_occurrences"]})

    @rule(data=st.data())
    def get_card(self, data):
        card_id, project = self.a_card(data)
        live = self.is_live(card_id, project)
        card = self.expect("get_card", set() if live else {"NotFound"}, {"project_id": project, "card_id": card_id})
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
        project = data.draw(st.sampled_from(self.projects))
        cursor, seen = None, []
        for _ in range(len(self.cards) + 2):
            page = self.check_page({"project_id": project, "order": order, "limit": limit,
                                    **({"cursor": cursor} if cursor else {})})
            seen += [item["card_id"] for item in page["items"]]
            cursor = page["next_cursor"]
            if cursor is None:
                break
        assert seen == [c["card_id"] for c in self.ordered(self.live(project), order)]
        event(f"paged through {min(len(seen), 5)}{'+' if len(seen) >= 5 else ''} cards")

    # ---- the hierarchy ------------------------------------------------------------------------

    def under(self, card_id: str | None, ancestor: str) -> bool:
        """Whether card_id is ancestor or one of its descendants."""
        seen = set()
        while card_id is not None and card_id not in seen:
            if card_id == ancestor:
                return True
            seen.add(card_id)
            card_id = self.cards[card_id]["parent_card_id"] if card_id in self.cards else None
        return False

    def reachable(self, card_id: str | None) -> bool:
        """Whether browsing from the roots reaches the card: it and each of its ancestors are live."""
        while card_id is not None:
            if card_id not in self.cards or card_id in self.deleted:
                return False
            card_id = self.cards[card_id]["parent_card_id"]
        return True

    def siblings(self, parent: str | None, project: str) -> list[str]:
        """The live children of parent, or the project's live root cards, in their order."""
        return [c["card_id"] for c in self.ordered([c for c in self.live(project) if c["parent_card_id"] == parent],
                                                   "sort_key_asc")]

    def refresh(self, card_id: str, *fields: str) -> None:
        """Take fields a write may change from Holder; the invariant checks the others."""
        current = self.core.call("get_card", project_id=self.cards[card_id]["project_id"], card_id=card_id)
        for name in fields:
            self.cards[card_id][name] = current[name]

    def a_card(self, data) -> tuple[str, str]:
        """A card's ID, mostly one the model knows, live or in the trash, and a project to ask for it in: mostly
        its own."""
        values = self.values()
        card_id = data.draw(values.valid({"kind": "named", "name": "CardId"}))
        own = st.just(self.cards[card_id]["project_id"]) if card_id in self.cards else st.nothing()
        project = data.draw(st.one_of(own, own, own, values.valid({"kind": "named", "name": "ProjectId"})))
        if card_id in self.cards and project != self.cards[card_id]["project_id"]:
            event("a card asked for in another project")
        return card_id, project

    @rule(data=st.data())
    def update_card(self, data):
        card_id, project = self.a_card(data)
        update = data.draw(self.values().record("CardUpdate"))
        parent = update.get("parent_card_id", ...)
        kinds = set()
        new_parent = parent not in (..., None)
        if not self.is_live(card_id, project) or (new_parent and not self.is_live(parent, project)):
            kinds.add("NotFound")
        elif parent not in (..., None) and self.under(parent, card_id):
            kinds.add("InvalidMove")
        before = int(time.time())
        updated = self.expect("update_card", kinds, {"project_id": project, "card_id": card_id, "update": update})
        after = int(time.time())
        if updated is None:
            return
        self.conforms("update_card", updated)
        old = self.cards[card_id]
        expected = {**old, **{k: update[k] for k in ("title", "content", "sort_key", "parent_card_id") if k in update}}
        for name in ("card_id", "project_id", "rel_path", "title", "content", "parent_card_id", "created_at",
                     "deleted_at"):
            assert updated[name] == expected[name], name
        if "sort_key" in update or parent in (..., old["parent_card_id"]):
            assert updated["sort_key"] == expected["sort_key"]
        # A field set to its current value changes nothing.
        if any(expected[k] != old[k] for k in ("title", "content", "sort_key", "parent_card_id")):
            if "updated_at" in update:
                assert updated["updated_at"] == update["updated_at"]
            else:
                assert before <= updated["updated_at"] <= after
        else:
            assert updated["updated_at"] == old["updated_at"], "an update with nothing to change wrote"
        self.check_tags(updated)
        self.cards[card_id] = updated

    @rule(data=st.data(), children=st.sampled_from(["promote", "trash_subtree"]))
    def trash_card(self, data, children):
        card_id, project = self.a_card(data)
        live = self.is_live(card_id, project)
        before = int(time.time())
        trashed = self.expect("trash_card", set() if live else {"NotFound"},
                              {"project_id": project, "card_id": card_id, "children": children})
        after = int(time.time())
        if trashed is None:
            return
        self.conforms("trash_card", trashed)
        assert before <= trashed["deleted_at"] <= after
        for name in ("title", "content", "parent_card_id", "project_id", "tags"):
            assert trashed[name] == self.cards[card_id][name], name
        children_ids = [c["card_id"] for c in self.live() if c["parent_card_id"] == card_id]
        order = self.siblings(card_id, project)
        self.cards[card_id] = trashed
        self.deleted.add(card_id)
        if children == "trash_subtree":
            pending = list(children_ids)
            while pending:
                child = pending.pop()
                self.deleted.add(child)
                pending += [c["card_id"] for c in self.live() if c["parent_card_id"] == child]
        else:
            # Its children take its place, keeping their own order.
            for child in children_ids:
                self.refresh(child, "parent_card_id")
                assert self.cards[child]["parent_card_id"] == trashed["parent_card_id"]
            # Cards already there may be re-spaced to make room.
            for sibling in self.siblings(trashed["parent_card_id"], project):
                self.refresh(sibling, "sort_key", "updated_at")
            assert [c for c in self.siblings(trashed["parent_card_id"], project) if c in children_ids] == order

    @rule(data=st.data())
    def restore_card(self, data):
        card_id, project = self.a_card(data)
        kinds = set()
        if not self.in_project(card_id, project):
            kinds.add("NotFound")
        elif card_id not in self.deleted:
            kinds.add("Conflict")
        before = int(time.time())
        restored = self.expect("restore_card", kinds, {"project_id": project, "card_id": card_id})
        after = int(time.time())
        if restored is None:
            return
        self.conforms("restore_card", restored)
        # Its parent when that is reachable, otherwise its nearest reachable ancestor, or the top level.
        parent = self.cards[card_id]["parent_card_id"]
        while parent is not None and not self.reachable(parent):
            parent = self.cards[parent]["parent_card_id"] if parent in self.cards else None
        assert restored["parent_card_id"] == parent
        assert restored["deleted_at"] is None
        assert before <= restored["updated_at"] <= after
        for name in ("title", "content", "project_id", "tags"):
            assert restored[name] == self.cards[card_id][name], name
        # It goes back to its place among its new siblings, by its old sort key.
        self.deleted.discard(card_id)
        self.cards[card_id] = {**self.cards[card_id], "parent_card_id": parent}
        expected = self.siblings(parent, project)
        self.cards[card_id] = restored
        # Siblings may be re-spaced to make room.
        for sibling in self.siblings(parent, project):
            if sibling != card_id:
                self.refresh(sibling, "sort_key", "updated_at")
        assert self.siblings(parent, project) == expected

    @rule(data=st.data())
    def move_card(self, data):
        card_id, project = self.a_card(data)
        move = data.draw(self.values().record("CardMove"))
        intent, target = move["intent"], move.get("target_card_id")
        scoped = intent in ("left", "right", "to_start", "to_end")
        live = self.is_live(card_id, project)

        def known(other):
            return self.is_live(other, project)

        kinds = set()
        parent = None
        if not live:
            kinds.add("NotFound")
        elif intent in ("into", "before", "after"):
            if target is None:
                kinds.add("InvalidArgument")
            elif not known(target):
                kinds.add("NotFound")
            else:
                parent = target if intent == "into" else self.cards[target]["parent_card_id"]
                if self.under(parent, card_id) or target == card_id:
                    kinds.add("InvalidMove")
        elif scoped:
            parent = move.get("parent_card_id", self.cards[card_id]["parent_card_id"])
            if parent is not None and not known(parent):
                kinds.add("NotFound")
            elif self.under(parent, card_id):
                kinds.add("InvalidMove")
        else:
            up = self.cards[card_id]["parent_card_id"]
            if up is None:
                kinds.add("InvalidMove")
            else:
                parent = self.cards[up]["parent_card_id"]
        expected = None
        if not kinds:
            destination = [c for c in self.siblings(parent, project) if c != card_id]
            current = self.siblings(self.cards[card_id]["parent_card_id"], project)
            if intent in ("into", "to_end", "up_level"):
                expected = destination + [card_id]
            elif intent == "to_start":
                expected = [card_id] + destination
            elif intent in ("before", "after"):
                at = destination.index(target) + (intent == "after")
                expected = destination[:at] + [card_id] + destination[at:]
            elif parent != self.cards[card_id]["parent_card_id"]:
                parent, expected = self.cards[card_id]["parent_card_id"], current  # nothing to swap with
            else:
                expected, i = list(current), current.index(card_id)
                j = i - 1 if intent == "left" else i + 1
                if 0 <= j < len(expected):
                    expected[i], expected[j] = expected[j], expected[i]
        moved = self.expect("move_card", kinds, {"project_id": project, "card_id": card_id, "move": move})
        if moved is None:
            return
        self.conforms("move_card", moved)
        assert moved["parent_card_id"] == parent
        for name in ("title", "content", "project_id", "created_at", "deleted_at", "tags"):
            assert moved[name] == self.cards[card_id][name], name
        self.cards[card_id] = moved
        # Siblings may be re-spaced to make room.
        for sibling in self.siblings(parent, project):
            if sibling != card_id:
                self.refresh(sibling, "sort_key", "updated_at")
        assert self.siblings(parent, project) == expected

    # ---- links ------------------------------------------------------------------------------------

    @staticmethod
    def link_identity(link: dict) -> tuple:
        return link["target_type"], link["target_id"], link["kind"], link["target_project_id"]

    def visible(self, link: dict) -> bool:
        """Whether a link is listed without include_deleted: its target is not a card in the trash here."""
        if link["target_type"] != "card" or link["target_project_id"] != link["source_project_id"]:
            return True
        return link["target_id"] in self.cards and link["target_id"] not in self.deleted

    def links_of(self, card_id: str, include_deleted: bool) -> list[dict]:
        return [link for link in self.links.get(card_id, []) if include_deleted or self.visible(link)]

    @staticmethod
    def newest_first(expected: list[dict], links: list[dict]) -> None:
        """Links come newest first; those made in the same second may come in any order."""
        assert [link["created_at"] for link in links] == sorted((link["created_at"] for link in links), reverse=True)
        def key(link):
            return link["created_at"], *Cards.link_identity(link)

        assert sorted(links, key=key) == sorted(expected, key=key)

    @rule(data=st.data())
    def add_card_link(self, data):
        card_id, project = self.a_card(data)
        link = data.draw(self.values().record("NewLink"))
        if self.resources and data.draw(st.integers(0, 3)) == 0:
            link = {**link, "target_type": "resource", "target_id": data.draw(st.sampled_from(list(self.resources)))}
            link.pop("target_project_id", None)
        elif data.draw(st.booleans()):
            # Mostly a card the model knows, in its own project or another.
            link = {**link, "target_type": "card", "target_id": data.draw(st.sampled_from(list(self.cards) or ["x"]))}
            link.pop("target_project_id", None)
            if link["target_id"] in self.cards and data.draw(st.booleans()):
                link["target_project_id"] = self.cards[link["target_id"]]["project_id"]
        target_project = link.get("target_project_id", project)
        target_id = link["target_id"]
        kinds = set()
        if not self.is_live(card_id, project):
            kinds.add("NotFound")
        elif link["target_type"] != "card" and target_project != project:
            kinds.add("InvalidArgument")
        elif link["target_type"] == "resource":
            if self.resources.get(target_id, {}).get("project_id") != project:
                kinds.add("NotFound")
        elif link["target_type"] != "card":
            # No AI threads or messages are made here, so they are missing.
            kinds.add("NotFound")
        elif target_project == project:
            if not self.is_live(target_id, project):
                kinds.add("NotFound")
        elif self.in_project(target_id, target_project) and target_id in self.deleted:
            kinds.add("NotFound")
        before = int(time.time())
        made = self.expect("add_card_link", kinds, {"project_id": project, "card_id": card_id, "link": link})
        after = int(time.time())
        if made is None:
            return
        self.conforms("add_card_link", made)
        expected = {"source_project_id": project, "source_type": "card", "source_id": card_id,
                    "target_project_id": target_project, "target_type": link["target_type"], "target_id": target_id,
                    "kind": link.get("kind", "ref"), "label": link.get("label")}
        assert {k: made[k] for k in expected} == expected
        links = self.links.setdefault(card_id, [])
        existing = next((old for old in links if self.link_identity(old) == self.link_identity(made)), None)
        # A link is written in its card's file, which changes unless the link is there already, as it is.
        if existing is None or existing["label"] != made["label"]:
            self.refresh(card_id, "updated_at")
            assert before <= self.cards[card_id]["updated_at"] <= after
        if existing is not None:
            # A link that already exists keeps when it was made.
            assert made["created_at"] == existing["created_at"]
            links.remove(existing)
        elif "created_at" in link:
            assert made["created_at"] == link["created_at"]
        else:
            assert before <= made["created_at"] <= after
        links.append(made)

    @rule(data=st.data(), include_deleted=st.booleans())
    def list_card_links(self, data, include_deleted):
        card_id, project = self.a_card(data)
        links = self.expect("list_card_links", set() if self.is_live(card_id, project) else {"NotFound"},
                            {"project_id": project, "card_id": card_id, "include_deleted": include_deleted})
        if links is None:
            return
        self.conforms("list_card_links", links)
        self.newest_first(self.links_of(card_id, include_deleted), links)

    @rule(data=st.data())
    def remove_card_link(self, data):
        card_id, project = self.a_card(data)
        known = self.links.get(card_id, [])
        if known and data.draw(st.booleans()):
            link = data.draw(st.sampled_from(known))
            arguments = {"target_type": link["target_type"], "target_id": link["target_id"], "kind": link["kind"]}
            if link["target_project_id"] != link["source_project_id"] or data.draw(st.booleans()):
                arguments["target_project_id"] = link["target_project_id"]
        else:
            arguments = data.draw(self.values().arguments("remove_card_link"))
            arguments.pop("project_id"), arguments.pop("card_id")
        remaining = self.expect("remove_card_link", set() if self.is_live(card_id, project) else {"NotFound"},
                                {"project_id": project, "card_id": card_id, **arguments})
        if remaining is None:
            return
        self.conforms("remove_card_link", remaining)
        gone = (arguments["target_type"], arguments["target_id"], arguments.get("kind", "ref"),
                arguments.get("target_project_id", project))
        if any(self.link_identity(link) == gone for link in known):
            self.refresh(card_id, "updated_at")
        self.links[card_id] = [link for link in known if self.link_identity(link) != gone]
        self.newest_first(self.links_of(card_id, False), remaining)

    @rule(data=st.data(), scope=st.sampled_from(["local", "external", "all_available"]), include_deleted=st.booleans())
    def list_card_backlinks(self, data, scope, include_deleted):
        card_id, project = self.a_card(data)
        links = self.expect("list_card_backlinks", set() if self.is_live(card_id, project) else {"NotFound"},
                            {"project_id": project, "card_id": card_id, "scope": scope,
                             "include_deleted": include_deleted})
        if links is None:
            return
        self.conforms("list_card_backlinks", links)
        expected = []
        for source, made in self.links.items():
            local = self.cards[source]["project_id"] == project
            if (local and scope == "external") or (not local and scope == "local"):
                continue
            if source in self.deleted and not include_deleted:
                continue
            expected += [link for link in made if link["target_type"] == "card" and link["target_id"] == card_id
                         and link["target_project_id"] == project]
        def order(link):
            return -link["created_at"], link["source_project_id"], link["source_id"], link["kind"]

        assert links == sorted(expected, key=order)

    # ---- milestones -------------------------------------------------------------------------------

    @staticmethod
    def made_order(milestones: list[dict]) -> list[dict]:
        return sorted(milestones, key=lambda m: (m["created_at"], m["milestone_id"]))

    def live_milestones(self, project: str) -> list[dict]:
        """The milestones on the project's live cards."""
        return [m for card in self.live(project) for m in self.milestones.get(card["card_id"], [])]

    def a_milestone(self, data) -> tuple[str, str, str]:
        """A card, a project to ask for it in and a milestone ID: mostly a milestone the model knows, on its card
        in its project, and otherwise any of each."""
        known = [(card_id, self.cards[card_id]["project_id"], m["milestone_id"])
                 for card_id, made in self.milestones.items() for m in made]
        if known and data.draw(st.integers(0, 3)):
            return data.draw(st.sampled_from(known))
        card_id, project = self.a_card(data)
        return card_id, project, data.draw(self.values().valid({"kind": "named", "name": "MilestoneId"}))

    @rule(data=st.data())
    def add_card_milestone(self, data):
        card_id, project = self.a_card(data)
        self.add_milestone(card_id, project, data.draw(self.values().record("NewMilestone")))

    def add_milestone(self, card_id: str, project: str, milestone: dict) -> None:
        kinds = set()
        if not all(valid_milestone_date(d) for d in milestone["dates"]):
            kinds.add("InvalidArgument")
        elif not self.is_live(card_id, project):
            kinds.add("NotFound")
        before = int(time.time())
        made = self.expect("add_card_milestone", kinds, {"project_id": project, "card_id": card_id,
                                                         "milestone": milestone})
        after = int(time.time())
        if made is None:
            return
        self.conforms("add_card_milestone", made)
        expected = {"project_id": project, "card_id": card_id, "dates": milestone["dates"],
                    "kind": milestone.get("kind"), "description": milestone.get("description")}
        assert {k: made[k] for k in expected} == expected
        assert before <= made["created_at"] == made["updated_at"] <= after
        assert all(made["milestone_id"] != m["milestone_id"] for ms in self.milestones.values() for m in ms)
        self.milestones.setdefault(card_id, []).append(made)
        self.refresh(card_id, "updated_at")
        assert before <= self.cards[card_id]["updated_at"] <= after

    @rule(data=st.data())
    def list_card_milestones(self, data):
        card_id, project = self.a_card(data)
        milestones = self.expect("list_card_milestones", set() if self.is_live(card_id, project) else {"NotFound"},
                                 {"project_id": project, "card_id": card_id})
        if milestones is None:
            return
        self.conforms("list_card_milestones", milestones)
        assert milestones == self.made_order(self.milestones.get(card_id, []))

    @rule(data=st.data())
    def update_card_milestone(self, data):
        card_id, project, milestone_id = self.a_milestone(data)
        update = data.draw(self.values().record("MilestoneUpdate"))
        own = self.milestones.get(card_id, [])
        old = next((m for m in own if m["milestone_id"] == milestone_id), None)
        kinds = set()
        if not all(valid_milestone_date(d) for d in update.get("dates", [])):
            kinds.add("InvalidArgument")
        elif not self.is_live(card_id, project) or old is None:
            kinds.add("NotFound")
        before = int(time.time())
        updated = self.expect("update_card_milestone", kinds, {"project_id": project, "card_id": card_id,
                                                               "milestone_id": milestone_id, "update": update})
        after = int(time.time())
        if updated is None:
            return
        self.conforms("update_card_milestone", updated)
        expected = {**old, **{k: update[k] for k in ("dates", "kind", "description") if k in update}}
        if all(expected[k] == old[k] for k in ("dates", "kind", "description")):
            # An update that changes nothing writes nothing.
            assert updated == old
        else:
            assert before <= updated["updated_at"] <= after
            assert updated == {**expected, "updated_at": updated["updated_at"]}
            self.refresh(card_id, "updated_at")
        own[own.index(old)] = updated

    @rule(data=st.data())
    def remove_card_milestone(self, data):
        card_id, project, milestone_id = self.a_milestone(data)
        remaining = self.expect("remove_card_milestone", set() if self.is_live(card_id, project) else {"NotFound"},
                                {"project_id": project, "card_id": card_id, "milestone_id": milestone_id})
        if remaining is None:
            return
        self.conforms("remove_card_milestone", remaining)
        own = self.milestones.get(card_id, [])
        if any(m["milestone_id"] == milestone_id for m in own):
            self.refresh(card_id, "updated_at")
        self.milestones[card_id] = [m for m in own if m["milestone_id"] != milestone_id]
        assert remaining == self.made_order(self.milestones[card_id])

    def matching(self, project: str, role: str | None, match) -> list[dict]:
        """The live milestones with a date of the role that match says is in a range, by the first such date and
        then by ID."""
        found = []
        for milestone in self.live_milestones(project):
            keys = [key for d in milestone["dates"] if role in (None, d["role"]) and (key := match(d)) is not None]
            if keys:
                found.append((min(keys), milestone["milestone_id"], milestone))
        return [m for *_, m in sorted(found, key=lambda f: f[:2])]

    @rule(data=st.data())
    def list_milestones_by_day(self, data):
        arguments = data.draw(self.values().arguments("list_milestones_by_day"))
        if data.draw(st.booleans()):
            arguments["role"] = data.draw(st.sampled_from(ROLE_POOL))
        first, last = arguments["first_day"], arguments["last_day"]
        kinds = set()
        if calendar_day(first) is None or calendar_day(last) is None:
            kinds.add("InvalidArgument")
        elif arguments["project_id"] not in self.projects:
            kinds.add("NotFound")
        milestones = self.expect("list_milestones_by_day", kinds, arguments)
        if milestones is None:
            return
        self.conforms("list_milestones_by_day", milestones)
        assert milestones == self.matching(arguments["project_id"], arguments.get("role"),
                                           lambda d: d["date"] if first <= d.get("date", "") <= last else None)

    def instants(self, data) -> tuple[int, int]:
        """A range of instants, mostly around the dates the model's milestones and cards have."""
        stamp = st.integers(-(2**63), 2**63 - 1)
        # From 2026-08-01 to 2028-01-01 UTC: fixed, so that a replayed run draws the same.
        around = st.integers(1785542400, 1830297600)
        return data.draw(st.one_of(around, around, stamp)), data.draw(st.one_of(around, around, stamp))

    @rule(data=st.data())
    def list_milestones_by_instant(self, data):
        arguments = data.draw(self.values().arguments("list_milestones_by_instant"))
        arguments["first_instant"], arguments["last_instant"] = self.instants(data)
        if data.draw(st.booleans()):
            arguments["role"] = data.draw(st.sampled_from(ROLE_POOL))
        first, last = arguments["first_instant"], arguments["last_instant"]
        milestones = self.expect("list_milestones_by_instant",
                                 set() if arguments["project_id"] in self.projects else {"NotFound"}, arguments)
        if milestones is None:
            return
        self.conforms("list_milestones_by_instant", milestones)

        def match(d):
            at = epoch_of(d["datetime"]) if "datetime" in d else None
            return at if at is not None and first <= at <= last else None

        assert milestones == self.matching(arguments["project_id"], arguments.get("role"), match)

    @rule(data=st.data())
    def get_calendar(self, data):
        arguments = data.draw(self.values().arguments("get_calendar"))
        arguments["first_instant"], arguments["last_instant"] = self.instants(data)
        first, last = arguments["first_instant"], arguments["last_instant"]
        first_day, last_day = arguments["first_day"], arguments["last_day"]
        project = arguments["project_id"]
        kinds = set()
        if calendar_day(first_day) is None or calendar_day(last_day) is None:
            kinds.add("InvalidArgument")
        elif project not in self.projects:
            kinds.add("NotFound")
        calendar = self.expect("get_calendar", kinds, arguments)
        if calendar is None:
            return
        self.conforms("get_calendar", calendar)

        def in_window(d):
            if "date" in d:
                return first_day <= d["date"] <= last_day
            return first <= epoch_of(d["datetime"]) <= last

        expected = [{**m, "card_title": self.cards[m["card_id"]]["title"]}
                    for m in self.made_order(self.live_milestones(project)) if any(map(in_window, m["dates"]))]
        assert calendar["milestones"] == expected
        cards = self.live(project)
        assert calendar["created_cards"] == [
            {name: c[name] for name in METADATA}
            for c in sorted(cards, key=lambda c: (c["created_at"], c["card_id"])) if first <= c["created_at"] <= last]
        assert calendar["updated_cards"] == [
            {name: c[name] for name in METADATA} for c in sorted(cards, key=lambda c: (c["updated_at"], c["card_id"]))
            if c["updated_at"] != c["created_at"] and first <= c["updated_at"] <= last]

    # ---- resources --------------------------------------------------------------------------------

    @staticmethod
    def by_change(resources) -> list[dict]:
        return sorted(resources, key=lambda r: (-r["updated_at"], r["resource_id"]))

    def a_resource(self, data) -> tuple[str, str]:
        """A resource's ID, mostly one the model knows, and a project to ask for it in: mostly its own."""
        values = self.values()
        resource_id = data.draw(values.valid({"kind": "named", "name": "ResourceId"}))
        own = st.just(self.resources[resource_id]["project_id"]) if resource_id in self.resources else st.nothing()
        return resource_id, data.draw(st.one_of(own, own, own, values.valid({"kind": "named", "name": "ProjectId"})))

    def resource_here(self, resource_id: str, project: str) -> bool:
        return self.resources.get(resource_id, {}).get("project_id") == project

    @rule(data=st.data())
    def create_resource(self, data):
        project = data.draw(self.values().valid({"kind": "named", "name": "ProjectId"}))
        resource = data.draw(self.values().record("NewResource"))
        # Mostly a new ID, or none; sometimes one in use, or one with characters an ID may not hold.
        choice = data.draw(st.sampled_from(["none", "new", "new", "used", "bad"]))
        resource.pop("resource_id", None)
        if choice == "new":
            resource["resource_id"] = f"run{self.run}-" + data.draw(st.from_regex(RESOURCE_ID, fullmatch=True))
        elif choice == "used" and self.resources:
            resource["resource_id"] = data.draw(st.sampled_from(list(self.resources)))
        elif choice == "bad":
            resource["resource_id"] = data.draw(st.sampled_from(["../../etc", "a/b/c/d", "res.json", "res 1"]))
        self.make_resource(project, resource)

    def make_resource(self, project: str, resource: dict) -> None:
        given = resource.get("resource_id")
        kinds = set()
        if (given is not None and not RESOURCE_ID.fullmatch(given)) or "" in resource.get("metadata", {}):
            kinds.add("InvalidArgument")
        elif project not in self.projects:
            kinds.add("NotFound")
        elif given in self.resources:
            kinds.add("Conflict")
        before = int(time.time())
        made = self.expect("create_resource", kinds, {"project_id": project, "resource": resource})
        after = int(time.time())
        if made is None:
            return
        self.conforms("create_resource", made)
        assert made["resource_id"] not in self.resources
        if given is not None:
            assert made["resource_id"] == given
        # A name without values is no name.
        expected = {"project_id": project, "type": resource["type"], "label": resource["label"],
                    "metadata": {k: v for k, v in resource.get("metadata", {}).items() if v}, "assets": []}
        assert {k: made[k] for k in expected} == expected
        if "created_at" in resource:
            assert made["created_at"] == resource["created_at"]
        else:
            assert before <= made["created_at"] <= after
        assert made["updated_at"] == resource.get("updated_at", made["created_at"])
        self.resources[made["resource_id"]] = made

    @rule(data=st.data())
    def get_resource(self, data):
        resource_id, project = self.a_resource(data)
        here = self.resource_here(resource_id, project)
        got = self.expect("get_resource", set() if here else {"NotFound"},
                          {"project_id": project, "resource_id": resource_id})
        if got is not None:
            self.conforms("get_resource", got)
            assert got == self.resources[resource_id]

    @rule(data=st.data())
    def list_resources(self, data):
        project = data.draw(self.values().valid({"kind": "named", "name": "ProjectId"}))
        listed = self.expect("list_resources", set() if project in self.projects else {"NotFound"},
                             {"project_id": project})
        if listed is not None:
            self.conforms("list_resources", listed)
            assert listed == self.by_change(r for r in self.resources.values() if r["project_id"] == project)

    @rule(data=st.data())
    def update_resource(self, data):
        resource_id, project = self.a_resource(data)
        update = data.draw(self.values().record("ResourceUpdate"))
        here = self.resource_here(resource_id, project)
        kinds = {"InvalidArgument"} if "" in update.get("metadata", {}) else set() if here else {"NotFound"}
        before = int(time.time())
        updated = self.expect("update_resource", kinds,
                              {"project_id": project, "resource_id": resource_id, "update": update})
        after = int(time.time())
        if updated is None:
            return
        self.conforms("update_resource", updated)
        old = self.resources[resource_id]
        expected = {**old, **{k: update[k] for k in ("type", "label") if k in update}}
        metadata = dict(old["metadata"])
        for name, values in update.get("metadata", {}).items():
            if values:
                metadata[name] = values
            else:
                metadata.pop(name, None)
        expected["metadata"] = metadata
        changed = any(expected[k] != old[k] for k in ("type", "label", "metadata"))
        if "updated_at" in update:
            expected["updated_at"] = update["updated_at"]
        elif changed:
            assert before <= updated["updated_at"] <= after
            expected["updated_at"] = updated["updated_at"]
        # Otherwise nothing changes, and nothing is written.
        assert updated == expected
        self.resources[resource_id] = updated

    @rule(data=st.data())
    def delete_resource(self, data):
        resource_id, project = self.a_resource(data)
        here = self.resource_here(resource_id, project)
        deleted = self.expect("delete_resource", set() if here else {"NotFound"},
                              {"project_id": project, "resource_id": resource_id})
        if deleted is None:
            return
        self.conforms("delete_resource", deleted)
        assert deleted == self.resources.pop(resource_id)
        # The links to it from live cards go with it, changing those cards.
        for card in self.live(project):
            links = self.links.get(card["card_id"], [])
            kept = [link for link in links if not (link["target_type"] == "resource" and link["target_id"] == resource_id)]
            if len(kept) != len(links):
                self.links[card["card_id"]] = kept
                self.refresh(card["card_id"], "updated_at")

    @rule(data=st.data(), include_deleted=st.booleans())
    def list_resource_backlinks(self, data, include_deleted):
        resource_id, project = self.a_resource(data)
        here = self.resource_here(resource_id, project)
        links = self.expect("list_resource_backlinks", set() if here else {"NotFound"},
                            {"project_id": project, "resource_id": resource_id, "include_deleted": include_deleted})
        if links is None:
            return
        self.conforms("list_resource_backlinks", links)
        expected = [link for source, made in self.links.items()
                    if self.cards[source]["project_id"] == project and (include_deleted or source not in self.deleted)
                    for link in made if link["target_type"] == "resource" and link["target_id"] == resource_id]

        def order(link):
            return -link["created_at"], link["source_project_id"], link["source_id"], link["kind"]

        assert links == sorted(expected, key=order)

    def attached(self, card_id: str) -> list[dict]:
        ids = {link["target_id"] for link in self.links.get(card_id, [])
               if link["target_type"] == "resource" and link["kind"] == "attachment"}
        return self.by_change(self.resources[i] for i in ids if i in self.resources)

    def a_card_and_resource(self, data) -> tuple[str, str, str]:
        """A card, a project to ask for it in, and mostly a resource in that project, or one attached."""
        card_id, project = self.a_card(data)
        near = sorted({r for r, resource in self.resources.items() if resource["project_id"] == project}
                      | {link["target_id"] for link in self.links.get(card_id, []) if link["target_type"] == "resource"})
        mine = st.sampled_from(near) if near else st.nothing()
        resource_id = data.draw(st.one_of(mine, mine, mine, self.values().valid({"kind": "named", "name": "ResourceId"})))
        return card_id, project, resource_id

    @rule(data=st.data())
    def list_card_resources(self, data):
        card_id, project = self.a_card(data)
        listed = self.expect("list_card_resources", set() if self.is_live(card_id, project) else {"NotFound"},
                             {"project_id": project, "card_id": card_id})
        if listed is not None:
            self.conforms("list_card_resources", listed)
            assert listed == self.attached(card_id)

    def reread_links(self, card_id: str) -> None:
        """Take the card's links from Holder after an attachment changes them, and its updated_at."""
        project = self.cards[card_id]["project_id"]
        self.links[card_id] = self.core.call("list_card_links", project_id=project, card_id=card_id,
                                             include_deleted=True)
        self.refresh(card_id, "updated_at")

    @rule(data=st.data())
    def attach_card_resource(self, data):
        card_id, project, resource_id = self.a_card_and_resource(data)
        kinds = set()
        if not self.is_live(card_id, project) or not self.resource_here(resource_id, project):
            kinds.add("NotFound")
        had = [dict(link) for link in self.links.get(card_id, [])]
        attached = self.expect("attach_card_resource", kinds,
                               {"project_id": project, "card_id": card_id, "resource_id": resource_id})
        if attached is None:
            return
        self.conforms("attach_card_resource", attached)
        self.reread_links(card_id)
        made = [link for link in self.links[card_id] if link not in had]
        already = any(link["target_type"] == "resource" and link["target_id"] == resource_id
                      and link["kind"] == "attachment" for link in had)
        # Attaching once makes one link; again, none.
        assert len(made) == (0 if already else 1)
        assert all(link["target_id"] == resource_id and link["kind"] == "attachment" for link in made)
        assert attached == self.attached(card_id)

    @rule(data=st.data())
    def detach_card_resource(self, data):
        card_id, project, resource_id = self.a_card_and_resource(data)
        had = list(self.links.get(card_id, []))
        detached = self.expect("detach_card_resource", set() if self.is_live(card_id, project) else {"NotFound"},
                               {"project_id": project, "card_id": card_id, "resource_id": resource_id})
        if detached is None:
            return
        self.conforms("detach_card_resource", detached)
        self.links[card_id] = [link for link in had if not (
            link["target_type"] == "resource" and link["target_id"] == resource_id and link["kind"] == "attachment")]
        if len(self.links[card_id]) != len(had):
            self.refresh(card_id, "updated_at")
        assert detached == self.attached(card_id)

    # ---- projects ---------------------------------------------------------------------------------

    def a_project_id(self, data) -> str:
        """Mostly a project the run made, and sometimes any."""
        known = st.sampled_from(list(self.project_records))
        return data.draw(st.one_of(known, known, known, Values(IR).valid({"kind": "named", "name": "ProjectId"})))

    def project_name(self, data) -> str:
        return self.prefix + data.draw(st.text(CHARACTERS, max_size=12))

    @rule(data=st.data())
    def create_project(self, data):
        project = data.draw(self.values().record("NewProject"))
        # A new ID or none, a plain project in this device's directory, and a remote Git accepts.
        project.pop("project_id", None)
        project.pop("root_path", None)
        project["name"] = self.project_name(data)
        project["privacy_mode"] = "plain"
        project["git_remote_url"] = data.draw(st.sampled_from(REMOTES))
        if data.draw(st.booleans()):
            project["project_id"] = f"run{self.run}-" + data.draw(st.from_regex(r"[a-z0-9]{1,12}", fullmatch=True))
        if data.draw(st.integers(0, 4)) == 0 and self.project_records:
            project["project_id"] = data.draw(st.sampled_from(list(self.project_records)))
        given = project.get("project_id")
        before = int(time.time())
        made = self.expect("create_project", {"Conflict"} if given in self.project_records else set(),
                           {"project": project})
        after = int(time.time())
        if made is None:
            return
        self.conforms("create_project", made)
        expected = {"name": project["name"], "privacy_mode": "plain", "project_key_id": None,
                    "git_remote_url": project["git_remote_url"], "git_provider": project.get("git_provider")}
        assert {k: made[k] for k in expected} == expected
        if given is not None:
            assert made["project_id"] == given
        assert made["project_id"] not in self.project_records
        if "created_at" in project:
            assert made["created_at"] == project["created_at"]
        else:
            assert before <= made["created_at"] <= after
        assert made["updated_at"] == project.get("updated_at", made["created_at"])
        self.project_records[made["project_id"]] = made

    @rule(data=st.data())
    def get_project(self, data):
        project_id = self.a_project_id(data)
        got = self.expect("get_project", set() if project_id in self.project_records else {"NotFound"},
                          {"project_id": project_id})
        if got is not None:
            self.conforms("get_project", got)
            assert got == self.project_records[project_id]

    @rule(data=st.data())
    def update_project(self, data):
        project_id = self.a_project_id(data)
        update = data.draw(self.values().record("ProjectUpdate"))
        if "name" in update:
            update["name"] = self.project_name(data)
        if "git_remote_url" in update:
            update["git_remote_url"] = data.draw(st.sampled_from(REMOTES))
        before = int(time.time())
        updated = self.expect("update_project", set() if project_id in self.project_records else {"NotFound"},
                              {"project_id": project_id, "update": update})
        after = int(time.time())
        if updated is None:
            return
        self.conforms("update_project", updated)
        old = self.project_records[project_id]
        expected = {**old, **update}
        if any(expected[k] != old[k] for k in update):
            assert before <= updated["updated_at"] <= after
            expected["updated_at"] = updated["updated_at"]
        # An update that changes nothing writes nothing.
        assert updated == expected
        self.project_records[project_id] = updated

    @rule(data=st.data())
    def delete_project(self, data):
        # Only a project made by create_project, so that the cards' projects stay.
        removable = [p for p in self.project_records if p not in self.projects]
        known = st.sampled_from(removable) if removable else st.nothing()
        project_id = data.draw(st.one_of(known, known, Values(IR).valid({"kind": "named", "name": "ProjectId"})))
        if project_id in self.projects:
            return
        removed = self.expect("delete_project", set() if project_id in self.project_records else {"NotFound"},
                              {"project_id": project_id})
        if removed is not None:
            self.conforms("delete_project", removed)
            assert removed == self.project_records.pop(project_id)

    @rule(data=st.data(), counts=st.booleans())
    def list_projects(self, data, counts):
        order = data.draw(st.sampled_from([None, *PROJECT_ORDERS]))
        arguments = {"name": self.prefix, "limit": 1000, "include_card_counts": counts}
        if order is not None:
            arguments["order"] = order
        if data.draw(st.booleans()):
            # Fixed, so that a replayed run draws the same: up to 2028-01-01.
            arguments["updated_after"] = data.draw(st.integers(0, 1830297600))
        listed = self.expect("list_projects", set(), arguments)
        self.conforms("list_projects", listed)
        expected = [p for p in self.project_records.values()
                    if p["updated_at"] >= arguments.get("updated_after", -(2**63))]
        assert sorted(p["project_id"] for p in listed) == sorted(p["project_id"] for p in expected)
        key, descending = PROJECT_ORDERS[order or "updated_at_desc"]
        keys = [key(p) for p in listed]
        assert keys == sorted(keys, reverse=descending), "projects out of order"
        for item in listed:
            record = {k: v for k, v in item.items() if k not in ("card_count", "root_card_count")}
            assert record == self.project_records[item["project_id"]]
            if counts:
                live = self.live(item["project_id"])
                assert item["card_count"] == len(live)
                assert item["root_card_count"] == sum(1 for c in live if c["parent_card_id"] is None)
            else:
                assert "card_count" not in item
        # Pages of the same listing are its slices.
        if listed:
            size = data.draw(st.integers(1, len(listed)))
            offset = data.draw(st.integers(0, len(listed)))
            page = self.core.call("list_projects", **{**arguments, "limit": size, "offset": offset})
            assert page == listed[offset:offset + size]

    # ---- git sync ---------------------------------------------------------------------------------

    def a_project_without_remote(self, data) -> str:
        """Mostly a project the run made with no Git remote, so that nothing reaches the network, and sometimes
        one that does not exist."""
        local = [p for p, record in self.project_records.items() if record["git_remote_url"] is None]
        known = st.sampled_from(local) if local else st.nothing()
        return data.draw(st.one_of(known, known, known, Values(IR).valid({"kind": "named", "name": "ProjectId"})))

    @rule(data=st.data(), operation=st.sampled_from(["pull_project", "push_project", "sync_project"]))
    def sync_without_remote(self, data, operation):
        project_id = self.a_project_without_remote(data)
        arguments = {"project_id": project_id}
        if operation != "pull_project":
            arguments["options"] = data.draw(self.values().record("PushOptions"))
        result = self.expect(operation, set() if project_id in self.project_records else {"NotFound"}, arguments)
        if result is None:
            return
        self.conforms(operation, result)
        pull, push = operation != "push_project", operation == "push_project"
        assert result["project_id"] == project_id
        assert result["status"] == "failed"
        assert result["remote_url"] is None
        # A sync's push follows only a pull that succeeded.
        assert result["pull"]["attempted"] == pull
        assert result["pull"]["status"] == ("remote_unset" if pull else "not_attempted")
        assert result["push"]["attempted"] == push
        assert result["push"]["status"] == ("remote_unset" if push else "not_attempted")
        state = self.core.call("get_project_sync_state", project_id=project_id)
        self.conforms("get_project_sync_state", state)
        assert state["project_id"] == project_id
        if push:
            assert state["last_push_status"] == "remote_unset"

    @rule(data=st.data())
    def test_remote_without_remote(self, data):
        project_id = self.a_project_without_remote(data)
        result = self.expect("test_project_remote", set() if project_id in self.project_records else {"NotFound"},
                             {"project_id": project_id, "remote": {}})
        if result is not None:
            self.conforms("test_project_remote", result)
            assert result == {"project_id": project_id, "status": "remote_unset", "remote_url": None,
                              "remote_has_head": False, "error_message": result["error_message"]}

    # ---- tags -------------------------------------------------------------------------------------

    def a_tag(self, data, card_id: str) -> str:
        """A tag, mostly the card's own or one the model's cards use, in any case, and sometimes text that is
        not a tag."""
        own = self.cards[card_id]["tags"] if card_id in self.cards else []
        used = sorted({t for c in self.cards.values() for t in c["tags"]})
        mine = st.sampled_from(own + [t.upper() for t in own]) if own else st.nothing()
        known = st.sampled_from(used) if used else st.nothing()
        return data.draw(st.one_of(mine, mine, known, st.sampled_from(TAG_POOL + ["#idea", "1st", "ab c"]),
                                   self.values().valid({"kind": "named", "name": "Tag"})))

    @rule(data=st.data())
    def add_card_tag(self, data):
        card_id, project = self.a_card(data)
        tag = self.a_tag(data, card_id)
        kinds = set()
        if not valid_tag(tag):
            kinds.add("InvalidArgument")
        elif not self.is_live(card_id, project):
            kinds.add("NotFound")
        before = int(time.time())
        card = self.expect("add_card_tag", kinds, {"project_id": project, "card_id": card_id, "tag": tag})
        after = int(time.time())
        if card is None:
            return
        self.conforms("add_card_tag", card)
        old = self.cards[card_id]
        assert tag.lower() in card["tags"]
        assert set(old["tags"]) <= set(card["tags"])
        self.check_tags(card)
        for name in ("card_id", "project_id", "title", "parent_card_id", "sort_key", "created_at", "deleted_at"):
            assert card[name] == old[name], name
        if tag.lower() in old["tags"]:
            # A card that already carries the tag, anywhere in its content, is left as it is.
            assert card == old
        else:
            assert before <= card["updated_at"] <= after
        self.cards[card_id] = card

    @rule(data=st.data())
    def remove_card_tag(self, data):
        card_id, project = self.a_card(data)
        tag = self.a_tag(data, card_id)
        kinds = set()
        if not valid_tag(tag):
            kinds.add("InvalidArgument")
        elif not self.is_live(card_id, project):
            kinds.add("NotFound")
        elif tag.lower() in self.cards[card_id]["tags"]:
            # Whether the trailing tag line alone carries it is core's to read from the content.
            try:
                card = self.core.call("remove_card_tag", project_id=project, card_id=card_id, tag=tag)
            except OperationError as error:
                event(f"remove_card_tag: {error.kind}")
                assert error.kind == "Conflict", f"unexpected {error}"
                return  # left as it is, which the invariant checks
            event("remove_card_tag: removed")
            self.conforms("remove_card_tag", card)
            assert tag.lower() not in card["tags"]
            self.check_tags(card)
            old = self.cards[card_id]
            for name in ("card_id", "project_id", "title", "parent_card_id", "sort_key", "created_at", "deleted_at"):
                assert card[name] == old[name], name
            assert set(card["tags"]) == set(old["tags"]) - {tag.lower()}
            self.cards[card_id] = card
            return
        card = self.expect("remove_card_tag", kinds, {"project_id": project, "card_id": card_id, "tag": tag})
        if card is not None:
            # A card that does not carry the tag is left as it is.
            self.conforms("remove_card_tag", card)
            assert card == self.cards[card_id]

    @rule(data=st.data())
    def list_project_tags(self, data):
        project = data.draw(self.values().valid({"kind": "named", "name": "ProjectId"}))
        tags = self.expect("list_project_tags", set() if project in self.projects else {"NotFound"},
                           {"project_id": project})
        if tags is None:
            return
        self.conforms("list_project_tags", tags)
        counts: dict[str, int] = {}
        for card in self.live(project):
            for tag in card["tags"]:
                counts[tag] = counts.get(tag, 0) + 1
        expected = [{"tag": tag, "card_count": count}
                    for tag, count in sorted(counts.items(), key=lambda item: (-item[1], item[0]))]
        assert tags == expected

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
            assert self.core.call("get_card", project_id=card["project_id"], card_id=card["card_id"]) == card

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
        if arguments["project_id"] not in self.projects:
            kinds.add("NotFound")
        page = self.expect("list_cards", kinds, arguments)
        if page is None:
            return None
        self.conforms("list_cards", page)

        matching = [c for c in self.live(arguments["project_id"])
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
            ("get_card", {"project_id": project, "card_id": text}),
            ("create_card", {"project_id": project, "card": {"title": text}}),
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
        ("get_card", {"project_id": project, "card_id": text}),
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
