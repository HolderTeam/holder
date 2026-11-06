"""The generated AI dispatcher serves exactly the AI routes declared by the Python contract."""

import json
import re
from pathlib import Path

from generators.daemon.paths import route_pattern
from test_support.contract import extract

ROOT = Path(__file__).resolve().parents[2]


def test_ai_dispatcher_routes_match_contract():
    source = (ROOT / "daemon/src/api/generated/AiRoutes.h").read_text(encoding="utf-8")
    patterns = {number: json.loads(literal) for number, literal in re.findall(
        r'static const std::regex route_(\d+)\(("(?:\\.|[^"\\])*")\);', source)}
    actual = {(method, patterns[number]) for method, number in re.findall(
        r'if \(method == "([A-Z]+)" && std::regex_match\(route_path, match, route_(\d+)\)\)', source)}
    bindings = extract(ROOT / "contract", "interface")["http"]
    expected = {(binding["method"], route_pattern(binding["path"])[0])
                for binding in bindings if binding["path"].startswith("/ai/")}
    assert actual == expected
