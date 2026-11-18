"""Schemathesis against the daemon, on the routes the contract describes.

Schemathesis reads api/openapi.json, the daemon's whole document, and tests the contract's
operations in it. It sends each one requests both
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
from types import SimpleNamespace
from urllib.parse import parse_qs, urlsplit

import jsonschema_rs
import pytest
import requests
import schemathesis
from hypothesis import HealthCheck, settings
from schemathesis.checks import not_a_server_error
from schemathesis.config import SchemathesisConfig
from schemathesis.specs.openapi.checks import negative_data_rejection, positive_data_acceptance

from holder_http import MAX_TARGET, shared_daemon
from test_support.contract import extract
from test_support.coverage_evidence import observe
from test_support.schema_scope import SSE_SERVICE_ERRORS, positive_acceptance_limits, schema_routes

ROOT = Path(__file__).resolve().parents[3]
IR = extract(ROOT / "contract", "interface")
# No string holds U+0000, a rule of every string in the contract (IR.md) that the document does not state.
CONFIG = SchemathesisConfig.from_dict({"generation": {"allow-x00": False}})
ROUTES = {(b["method"], b["path"]): b["operation"] for b in schema_routes(IR["http"])}
SCHEMA = schemathesis.openapi.from_path(ROOT / "api/openapi.json", config=CONFIG).include(
    name=[f"{method} {path}" for method, path in ROUTES])
DAEMON = os.environ.get("HOLDER_DAEMON")


def stated_in_words(case) -> bool:
    """Whether this request has prerequisites or rules beyond its schema."""
    query = set(case.query or {})
    limit = str((case.query or {}).get("limit", 200))
    if not ((case.query or {}).get("include_content") in (True, "true", "1")
            and limit.isdigit() and int(limit) > 1000):
        query.discard("include_content")
    method, path = case.operation.method.upper(), case.operation.path
    return any(positive_acceptance_limits(ROUTES[method, path], path, query,
                                          set(case.path_parameters or {}),
                                          set(case.body) if isinstance(case.body, dict) else set()))


def invalid_mutation_not_observed(case) -> bool:
    """Whether every schema violation is lost or ignored before response validation.

    The test sends its own token in place of the Authorization header Schemathesis makes. Requests
    also turns a non-string query value into URL text, or omits an empty list, before the daemon can
    validate it. Only ignore a mutation when that wire value satisfies the parameter's schema.
    """
    mutations = getattr(case.meta.phase.data, "mutations", None) if case.meta else None
    if not case.meta or not case.meta.generation.mode.is_negative or not mutations:
        return False
    declared = {parameter.name for parameter in case.operation.query}
    extras = [value for name, value in (case.query or {}).items() if name not in declared]

    def query_mutation_lost(mutation) -> bool:
        if mutation.parameter_location is None or mutation.parameter_location.name != "QUERY":
            return False
        parameter = next((p for p in case.operation.query if p.name == mutation.parameter), None)
        if parameter is None or mutation.parameter not in (case.query or {}):
            return False
        schema = parameter.definition["schema"]
        validator = jsonschema_rs.Draft202012Validator(schema)
        original = case.query[mutation.parameter]
        if validator.is_valid(original):
            return False
        request = requests.Request("GET", "http://holder.invalid/", params={mutation.parameter: original}).prepare()
        wire_values = parse_qs(urlsplit(request.url).query, keep_blank_values=True).get(mutation.parameter, [])
        if not wire_values:
            return not parameter.definition.get("required", False)
        return len(wire_values) == 1 and validator.is_valid(wire_values[0])

    return all(m.parameter == "Authorization" or (
        m.parameter_location is not None and m.parameter_location.name == "QUERY"
        and "additionalProperties" in m.keywords and all(value in ([], None) for value in extras))
        or query_mutation_lost(m)
        for m in mutations)


def test_negative_query_mutation_that_becomes_a_valid_string_is_not_observable():
    mutation = SimpleNamespace(parameter="runner_id", parameter_location=SimpleNamespace(name="QUERY"),
                               keywords=["type"])
    authorization = SimpleNamespace(parameter="Authorization", parameter_location=SimpleNamespace(name="HEADER"),
                                    keywords=["required"])
    case = SimpleNamespace(
        operation=SCHEMA["/ai/capabilities"]["GET"], query={"runner_id": ["false"]},
        meta=SimpleNamespace(generation=SimpleNamespace(mode=SimpleNamespace(is_negative=True)),
                             phase=SimpleNamespace(data=SimpleNamespace(mutations=[mutation, authorization]))))
    assert invalid_mutation_not_observed(case)
    case.query = {"runner_id": ["one", "two"]}
    assert not invalid_mutation_not_observed(case)


def test_unsupported_method_uses_the_declared_operation_for_scope():
    case = SimpleNamespace(method="TRACE", operation=SCHEMA["/imports"]["POST"], query={},
                           path_parameters={}, body={})
    assert stated_in_words(case)


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
    method = case.operation.method.upper()
    operation = ROUTES[method, case.operation.path]
    if operation in SSE_SERVICE_ERRORS:
        # These routes document 503 for an unavailable runner, provider or observer.
        excluded.append(not_a_server_error)
    if invalid_mutation_not_observed(case):
        excluded.append(negative_data_rejection)
    case.call_and_validate(base_url=daemon.url, headers={"Authorization": f"Bearer {daemon.token}"},
                           excluded_checks=excluded)
    if case.method.upper() == method:
        observe("http-schema", operation)
