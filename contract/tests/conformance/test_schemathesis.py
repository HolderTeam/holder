"""Schemathesis against the daemon, on the routes the contract describes.

Schemathesis reads api/openapi.json, the daemon's whole document, and tests the contract's
operations in it: those that have moved from RIG to the contract. It sends each one requests both
valid and invalid by the document, and checks that the daemon accepts the valid ones, rejects the
invalid ones with a 4xx, asks for its token, never fails with a 500, and answers only with
statuses, media types, headers and bodies the document declares. The Hypothesis state machine in
test_cards checks what the answers mean; this checks their edges, such as malformed bodies,
out-of-range numbers and other methods.

Runs with HOLDER_DAEMON set to a holderd built from the same commit, on the daemon test_cards
uses. HOLDER_CONFORMANCE_EXAMPLES sets how many requests Hypothesis makes for each route.
"""

from __future__ import annotations

import os
from pathlib import Path

import pytest
import requests
import schemathesis
from hypothesis import HealthCheck, settings
from schemathesis.config import SchemathesisConfig
from schemathesis.specs.openapi.checks import negative_data_rejection, positive_data_acceptance

from holder_contract.extract import extract
from holder_http import MAX_TARGET, shared_daemon

ROOT = Path(__file__).resolve().parents[3]
IR = extract(ROOT / "contract", "interface")
# No string holds U+0000, a rule of every string in the contract (IR.md) that the document does not state.
CONFIG = SchemathesisConfig.from_dict({"generation": {"allow-x00": False}})
SCHEMA = schemathesis.openapi.from_path(ROOT / "api/openapi.json", config=CONFIG).include(
    name=[f"{b['method']} {b['path']}" for b in IR["http"]])
DAEMON = os.environ.get("HOLDER_DAEMON")


def stated_in_words(case) -> bool:
    """Whether the request uses a rule the contract states in words rather than in its schema.

    The daemon may reject such a request although the document allows it: a cursor is valid only
    when the daemon handed it out, a tag has a form (see Tag in interface/cards.py), whether in the
    query or the path, a page with content holds at most 1000 cards, and a day and an instant have
    forms, and a milestone's date holds exactly one of them (see interface/milestones.py), and a
    resource's ID, in the path or the body, and metadata names have forms (see interface/resources.py).
    Search q must also be valid SQLite FTS5 syntax (see interface/app.py). An asset import's
    source_path must name a readable regular file on the daemon's host.
    """
    query = case.query or {}
    path = case.path_parameters or {}
    limit = str(query.get("limit", 200))
    body = case.body if isinstance(case.body, dict) else {}
    return "cursor" in query or "tag" in query or "tag" in path or "resource_id" in path or (
        query.get("include_content") in (True, "true", "1") and limit.isdigit() and int(limit) > 1000) or (
        "first_day" in query or "last_day" in query or "dates" in body or "resource_id" in body
        or "metadata" in body or "recovery_token" in body) or (
        case.operation.path in ("/search/cards", "/search/ai") and "q" in query) or (
        case.operation.path == "/imports" and case.method.upper() == "POST")


def invalid_only_unsent(case) -> bool:
    """Whether every way the request breaks the document is lost before it reaches the daemon.

    The test sends its own token in place of the Authorization header Schemathesis makes, and an
    extra query parameter whose value is an empty list is not sent at all, so such a request
    reaches the daemon valid.
    """
    mutations = getattr(case.meta.phase.data, "mutations", None) if case.meta else None
    if not case.meta or not case.meta.generation.mode.is_negative or not mutations:
        return False
    declared = {parameter.name for parameter in case.operation.query}
    extras = [value for name, value in (case.query or {}).items() if name not in declared]
    return all(m.parameter == "Authorization" or (
        m.parameter_location is not None and m.parameter_location.name == "QUERY"
        and m.keywords == ("additionalProperties",) and all(value in ([], None) for value in extras))
        for m in mutations)


@pytest.mark.skipif(not DAEMON, reason="set HOLDER_DAEMON to a holderd to run these")
@SCHEMA.parametrize()
@settings(max_examples=int(os.environ.get("HOLDER_CONFORMANCE_EXAMPLES", "25")), deadline=None,
          suppress_health_check=list(HealthCheck))
def test_the_daemon_answers_as_its_document_says(case):
    daemon = shared_daemon(DAEMON)
    target = requests.Request(case.method, daemon.url + case.formatted_path, params=case.query).prepare().path_url
    if len(target) > MAX_TARGET:  # the daemon drops a request longer than it reads (#300)
        return
    # A project's root path is a directory the daemon makes: a random one could be anywhere.
    if isinstance(case.body, dict) and "root_path" in case.body:
        return
    excluded = [positive_data_acceptance] if stated_in_words(case) else []
    if invalid_only_unsent(case):
        excluded.append(negative_data_rejection)
    case.call_and_validate(base_url=daemon.url, headers={"Authorization": f"Bearer {daemon.token}"},
                           excluded_checks=excluded)
