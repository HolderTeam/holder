"""Generate the daemon's HTTP dispatcher for the contract's operations.

Ported from RIG's http_routes.py and the route rules in its ir.py (https://github.com/zeth/rig,
src/rig/http_routes.py and src/rig/ir.py at e32ea68), which read declarations reflected from C++;
this reads the contract's IR and its HTTP bindings (the routes in interface/app.py).

contract_http::dispatch matches a request to an operation, decodes and checks its arguments, calls
core's C++ function for it (core/src/api/Contract.h) with the caller's holder_context, and encodes
the result. A success is {"ok": true, "data": ...} with the binding's status. A failure is
{"ok": false, "error": {"code", "message"}}: an invalid argument is 400, each error kind has the
status the Contract gives it, and anything else is a 500. The code is the error kind's name in
snake case.

dispatch answers only requests it has an operation for. For a path it knows with another method it
returns nothing, so that a host with other routes on the same path can answer; allowed_methods
lists the methods it serves on a path, for the host's 405.
"""

from __future__ import annotations

import json
import re
from typing import Any

from .generate import SCALARS_CPP, Model, Unsupported, c_param, camel, cpp_literal, cpp_param, header_lines, snake

NAMESPACE = "contract_http"
PARAMETER = re.compile(r"\{([A-Za-z_][A-Za-z_0-9]*)\}\Z")
LITERAL = re.compile(r"[A-Za-z0-9._~-]+\Z")
INVALID_ARGUMENT = (400, "invalid_argument")
UNEXPECTED = (500, "internal_error")
INT_RANGES = {"int32": ("std::numeric_limits<std::int32_t>::min()", "std::numeric_limits<std::int32_t>::max()"),
              "int64": ("std::numeric_limits<long long>::min()", "std::numeric_limits<long long>::max()")}

# ---- routes (from RIG's ir.py) -----------------------------------------------------------------


def route_pattern(path: str) -> tuple[str, dict[str, int]]:
    """The regex for a route and the capture position of each path parameter."""
    if not isinstance(path, str) or not path.startswith("/") or "//" in path or (path != "/" and path.endswith("/")):
        raise ValueError(f"unsupported route path: {path!r}")
    if path == "/":
        return "^/$", {}
    regex, captures = ["^"], {}
    for segment in path.split("/")[1:]:
        regex.append("/")
        placeholder = PARAMETER.fullmatch(segment)
        if placeholder:
            name = placeholder[1]
            if name in captures:
                raise ValueError(f"duplicate route parameter: {name}")
            regex.append("([^/]+)")
            captures[name] = len(captures) + 1
        elif LITERAL.fullmatch(segment):
            regex.append(re.escape(segment))
        else:
            raise ValueError(f"unsupported route segment: {segment}")
    regex.append("$")
    return "".join(regex), captures


def route_segments(path: str) -> tuple[tuple[int, str], ...]:
    """The route's segments as (specificity, literal): 0 for a literal, 1 for a parameter."""
    route_pattern(path)
    return tuple((1, "") if PARAMETER.fullmatch(segment) else (0, segment)
                 for segment in path.split("/")[1:] if segment)


def route_priority(binding: dict[str, Any]) -> tuple[int, ...]:
    return tuple(kind for kind, _ in route_segments(binding["path"]))


def _segments_overlap(left, right) -> bool:
    if len(left) != len(right):
        return False
    return not any(a_kind == b_kind == 0 and a_value != b_value
                   for (a_kind, a_value), (b_kind, b_value) in zip(left, right))


def _same_template(left, right) -> bool:
    return all((a_kind == b_kind == 0 and a_value == b_value) or (a_kind != 0 and b_kind != 0)
               for (a_kind, a_value), (b_kind, b_value) in zip(left, right))


def _more_specific(left, right) -> bool:
    kinds = [(a[0], b[0]) for a, b in zip(left, right)]
    return all(a <= b for a, b in kinds) and any(a < b for a, b in kinds)


def validate_routes(bindings: list[dict[str, Any]]) -> None:
    """Every request must match at most one route, or one route that is strictly more specific."""
    routes: dict[tuple[str, str], tuple] = {}
    for binding in bindings:
        method, path = binding["method"], binding["path"]
        segments = route_segments(path)
        for (existing_method, existing_path), existing in routes.items():
            if method != existing_method or not _segments_overlap(segments, existing):
                continue
            if _same_template(segments, existing) or not (
                    _more_specific(segments, existing) or _more_specific(existing, segments)):
                raise ValueError(f"ambiguous routes: {method} {existing_path} and {method} {path}")
        routes[(method, path)] = segments


# ---- dispatcher ---------------------------------------------------------------------------------

SUPPORT = r"""
namespace contract_http {
using json = nlohmann::json;
struct response {
    int status;
    json body;
    // The operation that answered, for logs.
    std::string_view operation;
};
inline response success(int status, json data, std::string_view operation) {
    return {status, json{{"ok", true}, {"data", std::move(data)}}, operation};
}
inline response failure(int status, std::string_view code, const std::string& message, std::string_view operation) {
    return {status, json{{"ok", false}, {"error", {{"code", code}, {"message", message}}}}, operation};
}
inline int hex_digit(char character) {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}
inline std::string decode_uri_component(std::string_view input, bool query) {
    std::string result;
    result.reserve(input.size());
    for (std::size_t index = 0; index < input.size(); ++index) {
        if (query && input[index] == '+') {
            result.push_back(' ');
            continue;
        }
        if (input[index] != '%') {
            result.push_back(input[index]);
            continue;
        }
        if (input.size() - index < 3)
            throw std::invalid_argument("invalid percent escape in URL parameter");
        const int high = hex_digit(input[index + 1]);
        const int low = hex_digit(input[index + 2]);
        if (high < 0 || low < 0)
            throw std::invalid_argument("invalid percent escape in URL parameter");
        result.push_back(static_cast<char>((high << 4) | low));
        index += 2;
    }
    return result;
}
inline std::map<std::string, std::string> decode_query(std::string_view input) {
    std::map<std::string, std::string> values;
    while (!input.empty()) {
        const auto separator = input.find('&');
        const auto entry = input.substr(0, separator);
        const auto equals = entry.find('=');
        auto key = decode_uri_component(entry.substr(0, equals), true);
        auto value = equals == std::string_view::npos ? std::string{} :
            decode_uri_component(entry.substr(equals + 1), true);
        if (!values.emplace(std::move(key), std::move(value)).second)
            throw std::invalid_argument("duplicate query parameter");
        if (separator == std::string_view::npos) break;
        input.remove_prefix(separator + 1);
    }
    return values;
}
template <typename T>
T decode_integer(std::string_view input, const char* name) {
    T value;
    const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), value);
    if (error != std::errc{} || end != input.data() + input.size())
        throw std::invalid_argument(std::string(name) + " must be an integer in range");
    return value;
}
inline bool decode_bool(std::string_view input, const char* name) {
    if (input == "true" || input == "1") return true;
    if (input == "false" || input == "0") return false;
    throw std::invalid_argument(std::string(name) + " must be true or false");
}
// The body's JSON. A syntax error's message quotes the bytes the parser stopped at, which need not
// be UTF-8, so it says only where; any other quotes a number, such as one too large.
inline json decode_body(const std::string& body) {
    if (body.empty()) throw std::invalid_argument("a request body is required");
    try {
        return json::parse(body);
    } catch (const json::parse_error& error) {
        throw std::invalid_argument("the request body is not JSON, at byte " + std::to_string(error.byte));
    } catch (const json::exception& error) {
        throw std::invalid_argument(std::string("the request body is not JSON: ") + error.what());
    }
}
"""


def text_check(t: dict[str, Any], source: str, label: str) -> str:
    maximum = t.get("max_length", "std::numeric_limits<std::size_t>::max()")
    return f'contract_json::check_text({source}, "{label}", {t.get("min_length", 0)}, {maximum})'


def scalar_value(t: dict[str, Any], source: str, label: str) -> str:
    """An expression for a path or query value, from its decoded text."""
    kind = t["kind"]
    if kind == "string":
        return text_check(t, source, label)
    if kind in INT_RANGES:
        cpp = SCALARS_CPP[kind]
        value = f'decode_integer<{cpp}>({source}, "{label}")'
        if "ge" in t or "le" in t:
            low, high = INT_RANGES[kind]
            value = f'contract_json::check_range<{cpp}>({value}, "{label}", {t.get("ge", low)}, {t.get("le", high)})'
        return value
    if kind == "bool":
        return f'decode_bool({source}, "{label}")'
    raise Unsupported(f"{label}: {kind} in a path or query")


def query_lines(model: Model, op: dict[str, Any], parameter: dict[str, Any]) -> list[str]:
    """Statements that set the parameter's variable from the query string."""
    param = c_param(model, op, parameter)
    slot, t, name = param.slot, param.slot.type, param.slot.name
    type_name = "holder::contract::" + camel(op["name"]) + camel(name)
    found = f"query_{name}"
    lines = [f'            const auto {found} = query_values.find("{name}");',
             f"            if ({found} != query_values.end()) {{",
             f"                const auto& text = {found}->second;"]
    if t["kind"] == "literal":
        choices = ", ".join(t["values"])
        for i, value in enumerate(t["values"]):
            lines.append(f'                {"if" if i == 0 else "else if"} (text == {json.dumps(value)}) '
                         f"{name} = {type_name}::{camel(str(value))};")
        lines.append(f'                else throw std::invalid_argument("{name} must be one of {choices}");')
    elif t["kind"] == "union":
        literals = [v for a in t["alternatives"] if a["kind"] == "literal" for v in a["values"]]
        other = next(model.resolve(a) for a in t["alternatives"] if a["kind"] != "literal")
        if other["kind"] != "string":
            raise Unsupported(f"{op['name']}({name}): unions other than literals plus one string")
        for i, value in enumerate(literals):
            lines.append(f'                {"if" if i == 0 else "else if"} (text == {json.dumps(value)}) '
                         f"{name} = {type_name}{{{type_name}{camel(str(value))}{{}}}};")
        lines.append(f"                else {name} = {type_name}{{{text_check(other, 'text', name)}}};")
    else:
        lines.append(f"                {name} = {scalar_value(t, 'text', name)};")
    if slot.has_default and slot.default is not None:
        default = (f"{type_name}::{camel(str(slot.default))}" if t["kind"] == "literal"
                   else cpp_literal(slot.default))
        lines += ["            } else {", f"                {name} = {default};"]
    elif not (slot.nullable or slot.optional):
        lines += ["            } else {",
                  f'                throw std::invalid_argument("the query parameter {name} is required");']
    lines.append("            }")
    return lines


def operation_lines(model: Model, op: dict[str, Any], binding: dict[str, Any], index: int,
                    statuses: dict[str, int]) -> list[str]:
    name = op["name"]
    pattern, captures = route_pattern(binding["path"])
    lines = [f"    static const std::regex route_{index}({json.dumps(pattern)});",
             f'    if (method == "{binding["method"]}" && std::regex_match(route_path, match, route_{index})) {{']
    declarations, decodes, arguments = [], [], []
    if binding["query"]:
        decodes.append("            const auto query_values = decode_query(query_string);")
    for parameter in op["parameters"]:
        param = c_param(model, op, parameter)
        decl, _ = cpp_param(model, op, param)
        decl = re.sub(r"^const (.*)&", r"\1", decl)
        # Types core declares in holder::contract, such as ListCardsOrder or NewCard.
        decl = re.sub(r"(?<![\w:])([A-Z]\w*)", r"holder::contract::\1", decl)
        declarations.append(f"        {decl}{{}};")
        pname = param.slot.name
        arguments.append(f"std::move({pname})")
        if pname in captures:
            decodes.append(f"            {pname} = "
                           f"{scalar_value(param.slot.type, f'decode_uri_component(match[{captures[pname]}].str(), false)', pname)};")
        elif pname in binding["query"]:
            decodes += query_lines(model, op, parameter)
        else:
            if param.slot.type["kind"] != "named" or param.slot.nullable:
                raise Unsupported(f"{name}({pname}): a request body that is not a record")
            record = model.instance(param.slot.type, {}).qualified("holder::contract::")
            decodes.append(f"            {pname} = contract_json::decode<{record}>(decode_body(body));")
    status, code = INVALID_ARGUMENT
    lines += declarations
    if decodes:
        lines += ["        try {", *decodes,
                  "        } catch (const std::invalid_argument& error) {",
                  f'            return failure({status}, "{code}", error.what(), "{name}");',
                  "        }"]
    call = f"holder::contract::{name}(context{''.join(', ' + a for a in arguments)})"
    lines += ["        try {",
              f'            return success({binding["status"]}, contract_json::encode({call}), "{name}");']
    for error in op["errors"]:
        lines += [f"        }} catch (const holder::contract::{error}& error) {{",
                  f'            return failure({statuses[error]}, "{snake(error)}", error.what(), "{name}");']
    unexpected_status, unexpected_code = UNEXPECTED
    lines += ["        } catch (const holder::contract::InvalidArgument& error) {",
              f'            return failure({status}, "{code}", error.what(), "{name}");',
              "        } catch (const std::exception& error) {",
              f'            return failure({unexpected_status}, "{unexpected_code}", error.what(), "{name}");',
              "        }", "    }"]
    return lines


def generate_routes(model: Model, codec_header: str) -> str:
    if not re.fullmatch(r"[A-Za-z_0-9/.-]+", codec_header):
        raise ValueError("invalid codec header path")
    operations = {op["name"]: op for op in model.operations}
    bindings = [b for b in model.ir["http"] if b["operation"] in operations]
    validate_routes(bindings)
    statuses = {e["error"]: e["status"] for e in model.ir["http_errors"]}
    lines = [*header_lines(model.ir, "The daemon's HTTP routes for the contract's operations; "
                                     "contract/holder_contract/http_routes.py says how they behave.", "cpp"),
             "#pragma once",
             f'#include "{codec_header}"',
             "#include <charconv>",
             "#include <map>",
             "#include <optional>",
             "#include <regex>",
             "#include <set>",
             "#include <stdexcept>",
             "#include <string>",
             "#include <string_view>",
             "#include <system_error>",
             "#include <utility>",
             "#include <vector>",
             SUPPORT.rstrip(),
             "// The response for a request the contract has an operation for; nothing otherwise.",
             "inline std::optional<response> dispatch(const std::string& method, const std::string& path,",
             "                                        const std::string& body, holder_context& context) {",
             "    const auto query_start = path.find('?');",
             "    const auto route_path = path.substr(0, query_start);"]
    if any(b["query"] for b in bindings):
        lines += ["    const auto query_string = query_start == std::string::npos ? std::string_view{} :",
                  "        std::string_view(path).substr(query_start + 1);"]
    lines += ["    std::smatch match;", "    (void)body;", "    (void)context;"]
    # Routes are tried from the most specific: a literal segment before a parameter.
    ordered = sorted(enumerate(bindings), key=lambda pair: route_priority(pair[1]))
    for index, binding in ordered:
        lines += operation_lines(model, operations[binding["operation"]], binding, index, statuses)
    lines += ["    return std::nullopt;", "}", "",
              "// The methods dispatch answers on this path, for a 405's Allow header.",
              "inline std::set<std::string_view> allowed_methods(const std::string& path) {",
              "    const auto route_path = path.substr(0, path.find('?'));",
              "    std::set<std::string_view> methods;"]
    patterns: dict[str, set[str]] = {}
    for _, binding in ordered:
        patterns.setdefault(route_pattern(binding["path"])[0], set()).add(binding["method"])
    for position, (pattern, methods) in enumerate(patterns.items()):
        listed = ", ".join(json.dumps(m) for m in sorted(methods))
        lines += [f"    static const std::regex path_{position}({json.dumps(pattern)});",
                  f"    if (std::regex_match(route_path, path_{position})) methods.insert({{{listed}}});"]
    lines += ["    return methods;", "}", f"}}  // namespace {NAMESPACE}", ""]
    return "\n".join(lines)
