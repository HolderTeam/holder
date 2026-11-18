"""Report schema and behavioral evidence per contract operation and transport."""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from xml.etree import ElementTree

from compiler.pipeline import Compiler, ContractSource

from .coverage_catalog import BEHAVIOR_LIMITS, GAPS, NATIVE, OTHER_TRANSPORT_GAPS, STATE_MACHINE
from .schema_scope import SSE_SERVICE_ERRORS, STREAM_EXCLUSIONS, positive_acceptance_limits

ROOT = Path(__file__).resolve().parents[2]
LOCATION_TEST = "test_location_binding"
LOCATION_OPERATION = "put_location_binding"
SCHEMA_TEST = "test_the_daemon_answers_as_its_document_says"


@dataclass(frozen=True)
class Evidence:
    source: str
    description: str
    suite: str = ""
    test: str = ""
    observation: str = ""

    def display(self, results: list[TestResult], relative: str) -> str:
        label = f"[{self.description}]({relative}{self.source})"
        if not self.suite:
            return f"{label} (native evidence, execution unreported)"
        matches = [
            result
            for result in results
            if result.suite == self.suite and (result.name == self.test or result.name.startswith(self.test + "["))
        ]
        if not matches:
            status = "unreported"
        else:
            counts = Counter(result.status for result in matches)
            status = ", ".join(f"{count} {name}" for name, count in sorted(counts.items()))
            if self.observation and counts["passed"]:
                observed = any(
                    result.status == "passed" and self.observation in result.observations for result in matches
                )
                status += ", check observed" if observed else ", check not observed in a passing test"
        return f"{label} ({status})"


@dataclass(frozen=True)
class TestResult:
    suite: str
    name: str
    status: str
    observations: frozenset[str]


def read_results(paths: list[Path]) -> list[TestResult]:
    results = []
    for path in paths:
        for case in ElementTree.parse(path).iter("testcase"):
            status = next((kind for kind in ("error", "failure", "skipped") if case.find(kind) is not None), "passed")
            name = case.find("./properties/property[@name='contract-case']")
            results.append(
                TestResult(
                    case.attrib["classname"].rsplit(".", 1)[-1],
                    name.attrib["value"] if name is not None else case.attrib["name"],
                    status,
                    frozenset(
                        item.attrib["value"]
                        for item in case.findall("./properties/property")
                        if item.get("name") == "contract-check"
                    ),
                )
            )
    return results


@dataclass(frozen=True)
class Row:
    operation: str
    transport: str
    schema: tuple[Evidence, ...]
    behavior: tuple[Evidence, ...]
    schema_gap: str = ""
    behavior_gap: str = ""


def inventory(ir) -> list[Row]:
    operations = {op.name: op for op in ir.operations}
    http = {b.operation: b for b in ir.http}
    groups = [*STATE_MACHINE, *NATIVE, LOCATION_OPERATION, *(name for names in GAPS.values() for name in names.split())]
    duplicates = sorted(name for name, count in Counter(groups).items() if count > 1)
    missing = sorted(operations.keys() - set(groups))
    stale = sorted(set(groups) - operations.keys())
    if missing or stale or duplicates:
        raise ValueError(f"coverage dispositions differ: missing={missing}, stale={stale}, duplicates={duplicates}")
    gap_by_name = {name: reason for reason, names in GAPS.items() for name in names.split()}
    extra = set(OTHER_TRANSPORT_GAPS) | set(BEHAVIOR_LIMITS) | STREAM_EXCLUSIONS.keys() | SSE_SERVICE_ERRORS
    if stale_notes := sorted(extra - operations.keys()):
        raise ValueError(f"stale coverage notes: {stale_notes}")

    rows = []
    used_other_gaps = set()
    for name, op in operations.items():
        transports = ([] if op.daemon_only else ["C"]) + (["HTTP"] if name in http else [])
        if not transports:
            raise ValueError(f"{name}: no public transport")
        if name in NATIVE and NATIVE[name][0] not in transports:
            raise ValueError(f"{name}: native evidence names an unavailable transport")
        if (name in {LOCATION_OPERATION, *STREAM_EXCLUSIONS, *SSE_SERVICE_ERRORS}) and "HTTP" not in transports:
            raise ValueError(f"{name}: HTTP evidence names an unavailable transport")
        for transport in transports:
            schema, behavior = [], []
            schema_notes = []
            if name in STATE_MACHINE:
                test = "runTest"
                suite = "TestCardsThroughTheCApi" if transport == "C" else "TestCardsOverHttp"
                source = "contract/tests/conformance/test_cards.py"
                schema.append(Evidence(source, "State machine result shape", suite, test, f"model-result:{name}"))
                behavior.append(
                    Evidence(source, "Independent state machine rules", suite, test, f"model-result:{name}")
                )
            if transport == "HTTP":
                binding = http[name]
                if name in STREAM_EXCLUSIONS:
                    schema_notes.append(STREAM_EXCLUSIONS[name])
                else:
                    test = f"{SCHEMA_TEST}[{binding.method} {binding.path}]"
                    schema.append(
                        Evidence(
                            "contract/tests/conformance/test_schemathesis.py",
                            "OpenAPI cases",
                            "test_schemathesis",
                            test,
                            f"http-schema:{name}",
                        )
                    )
                    body_fields = set()
                    if binding.body:
                        ref, _ = ir.resolve_type(op.parameter(binding.body).type)
                        records = [r for r in ir.records if r.name == getattr(ref, "name", None)]
                        if records:
                            body_fields = {field.name for field in records[0].fields}
                    schema_notes.extend(
                        positive_acceptance_limits(
                            name, binding.path, set(binding.query), set(binding.path_parameters), body_fields
                        )
                    )
                    if name in SSE_SERVICE_ERRORS:
                        schema_notes.append("The suite permits documented 503 responses when a service is unavailable.")
                if name == LOCATION_OPERATION:
                    test = "test_real_daemon_binding_preserves_preview_and_keeps_secrets_private"
                    evidence = Evidence(
                        f"contract/tests/conformance/{LOCATION_TEST}.py",
                        "Live preview, persistence and secret non-disclosure",
                        LOCATION_TEST,
                        test,
                    )
                    behavior.append(evidence)
                    behavior.append(Evidence("daemon/tests/AiResourceRoutes_test.cpp",
                                             "Unavailable and failing secret stores"))
                    for label, case_name in (
                        ("Declared success model", test),
                        (
                            "Declared invalid request error model",
                            "test_real_daemon_rejects_invalid_requests_without_changing_the_binding",
                        ),
                        (
                            "Declared missing location error model",
                            "test_real_daemon_missing_location_authentication_and_method_rules",
                        ),
                    ):
                        schema.append(Evidence(evidence.source, label, LOCATION_TEST, case_name))
            if name in NATIVE and NATIVE[name][0] == transport:
                _, source, scenario = NATIVE[name]
                behavior.append(Evidence(source, scenario))
            if not schema and transport == "C":
                schema_notes.append("No systematic live C result/argument shape check recorded for this operation.")
            if behavior:
                gap = BEHAVIOR_LIMITS.get(name, "")
            elif name in OTHER_TRANSPORT_GAPS:
                used_other_gaps.add(name)
                gap = OTHER_TRANSPORT_GAPS[name]
            elif name in gap_by_name:
                gap = gap_by_name[name]
            else:
                raise ValueError(f"{name} ({transport}): missing behavioral disposition")
            rows.append(Row(name, transport, tuple(schema), tuple(behavior), " ".join(schema_notes), gap))
    if unused := OTHER_TRANSPORT_GAPS.keys() - used_other_gaps:
        raise ValueError(f"unused transport gap notes: {sorted(unused)}")
    return rows


def render(rows: list[Row], results: list[TestResult], relative: str) -> str:
    counts = Counter(row.transport for row in rows)
    outcomes = Counter(result.status for result in results)
    lines = [
        "# Contract coverage",
        "",
        f"{len({row.operation for row in rows})} operations across {counts['C']} C and {counts['HTTP']} HTTP entries.",
        "",
        (
            "Conformance results supplied: "
            + ", ".join(f"{count} {status}" for status, count in sorted(outcomes.items()))
            + "."
            if results
            else "No conformance results supplied. All execution is unreported."
        ),
        (
            "A state-machine observation means the operation reached a result-shape assertion in that run. "
            "The linked rules specify the behavioral checks. Counts are pytest results, including repeated runs if multiple reports are supplied."
        ),
        "",
        "Priority gaps for Desktop and phone sync:",
        "",
        "- Exercise recovery import and retry with missing keys or interrupted work.",
        "- Check successful and conflicting two-device sync with a disposable Git remote. The shared model uses an unset remote.",
        "- Check named change-feed payloads against their schemas across reconnect and restart. Native tests cover replay and resync.",
        "- Exercise attachment job outcomes and streamed bytes with a configured storage provider when attachment sync enters scope.",
        "- Check runner-pull SSE terminal delivery and resume when that daemon-only workflow is next in scope.",
    ]
    lines += [
        "",
        (
            "Schemathesis still validates response shape when positive acceptance is relaxed. It skips overlong targets "
            "(the daemon drops them) and generated root paths (the daemon owns the directory). Negative mutations lost during "
            "query serialization or authorization injection are excluded from negative rejection."
        ),
        "",
        "| Operation | Transport | Schema checks and limits | Behavior checks and gaps |",
        "| --- | --- | --- | --- |",
    ]
    for row in rows:
        schema = [item.display(results, relative) for item in row.schema]
        behavior = [item.display(results, relative) for item in row.behavior]
        if row.schema_gap:
            schema.append(row.schema_gap)
        if row.behavior_gap:
            behavior.append(row.behavior_gap)
        lines.append(f"| `{row.operation}` | {row.transport} | {'<br>'.join(schema)} | {'<br>'.join(behavior)} |")
    return "\n".join([*lines, ""])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--junit", action="append", type=Path, default=[], help="pytest JUnit report (repeatable)")
    parser.add_argument("--link-prefix", default="../", help="prefix for repo-relative evidence links")
    args = parser.parse_args()
    ir = Compiler().compile(ContractSource(ROOT / "contract", "interface"))
    print(render(inventory(ir), read_results(args.junit), args.link_prefix), end="")


if __name__ == "__main__":
    main()
