"""Generate the daemon's HTTP dispatcher for the contract's operations.

Ported from RIG's http_routes.py and the route rules in its ir.py (https://github.com/zeth/rig,
src/rig/http_routes.py and src/rig/ir.py at e32ea68), which read declarations reflected from C++;
this reads the contract's IR and its HTTP bindings (the routes in each domain's operations.py).

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

from compiler.ir import ContractView, LiteralType, ScalarType, TypeRef, UnionType
from generators.daemon.http import HttpArgument, HttpRoute, plan_http
from generators.daemon.paths import route_pattern, route_priority, route_segments, validate_routes

from .generate import SCALARS_CPP, Model, Unsupported, camel, cpp_literal, header_lines, snake

NAMESPACE = "contract_http"
INVALID_ARGUMENT = (400, "invalid_argument")
UNEXPECTED = (500, "internal_error")
INT_RANGES = {"int32": ("std::numeric_limits<std::int32_t>::min()", "std::numeric_limits<std::int32_t>::max()"),
              "int64": ("std::numeric_limits<long long>::min()", "std::numeric_limits<long long>::max()")}

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
// A query parameter the operation does not have is refused, so that a caller who sends one learns it
// was not used, as the JSON codec does for a record's fields.
inline void check_query_names(const std::map<std::string, std::string>& values,
                              std::initializer_list<std::string_view> names) {
    for (const auto& [key, value] : values) {
        if (std::find(names.begin(), names.end(), key) == names.end())
            throw std::invalid_argument("there is no query parameter " + key);
    }
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
// A body whose record has only fields that may be left out may itself be left out, as {}.
inline json decode_body(const std::string& body, bool optional = false) {
    if (body.empty() && optional) return json::object();
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


def text_check(t: TypeRef, source: str, label: str) -> str:
    maximum = t.max_length if t.max_length is not None else "std::numeric_limits<std::size_t>::max()"
    return f'contract_json::check_text({source}, "{label}", {t.min_length or 0}, {maximum})'


def scalar_value(t: TypeRef, source: str, label: str) -> str:
    """An expression for a path or query value, from its decoded text."""
    kind = t.kind
    if kind == "string":
        return text_check(t, source, label)
    if kind in INT_RANGES:
        cpp = SCALARS_CPP[kind]
        value = f'decode_integer<{cpp}>({source}, "{label}")'
        if t.ge is not None or t.le is not None:
            low, high = INT_RANGES[kind]
            value = f'contract_json::check_range<{cpp}>({value}, "{label}", {t.ge if t.ge is not None else low}, {t.le if t.le is not None else high})'
        return value
    if kind == "bool":
        return f'decode_bool({source}, "{label}")'
    raise Unsupported(f"{label}: {kind} in a path or query")


def query_lines(contract: ContractView, route: HttpRoute, argument: HttpArgument) -> list[str]:
    """Statements that set the parameter's variable from the query string."""
    op = route.operation
    t, name = argument.ref, argument.parameter.name
    type_name = "holder::contract::" + camel(op.name) + camel(name)
    found = f"query_{name}"
    lines = [f'            const auto {found} = query_values.find("{name}");',
             f"            if ({found} != query_values.end()) {{",
             f"                const auto& text = {found}->second;"]
    if isinstance(t, LiteralType):
        choices = ", ".join(str(value) for value in t.values)
        for i, value in enumerate(t.values):
            lines.append(f'                {"if" if i == 0 else "else if"} (text == {json.dumps(value)}) '
                         f"{name} = {type_name}::{camel(str(value))};")
        lines.append(f'                else throw std::invalid_argument("{name} must be one of {choices}");')
    elif isinstance(t, UnionType):
        literals = [v for a in t.alternatives if isinstance(a, LiteralType) for v in a.values]
        other = next(contract.resolve_type(a)[0] for a in t.alternatives if not isinstance(a, LiteralType))
        if not isinstance(other, ScalarType) or other.kind != "string":
            raise Unsupported(f"{op.name}({name}): unions other than literals plus one string")
        for i, value in enumerate(literals):
            lines.append(f'                {"if" if i == 0 else "else if"} (text == {json.dumps(value)}) '
                         f"{name} = {type_name}{{{type_name}{camel(str(value))}{{}}}};")
        lines.append(f"                else {name} = {type_name}{{{text_check(other, 'text', name)}}};")
    else:
        lines.append(f"                {name} = {scalar_value(t, 'text', name)};")
    default_value = argument.parameter.default
    if argument.has_default and default_value is not None:
        default = (f"{type_name}::{camel(str(default_value))}" if isinstance(t, LiteralType)
                   else cpp_literal(default_value))
        lines += ["            } else {", f"                {name} = {default};"]
    elif not (argument.nullable or argument.has_default):
        lines += ["            } else {",
                  f'                throw std::invalid_argument("the query parameter {name} is required");']
    lines.append("            }")
    return lines


def operation_lines(contract: ContractView, route: HttpRoute, index: int,
                    statuses: dict[str, int]) -> list[str]:
    op, binding = route.operation, route.binding
    name = op.name
    pattern, captures = route_pattern(binding.path)
    lines = [f"    static const std::regex route_{index}({json.dumps(pattern)});",
             f'    if (method == "{binding.method}" && std::regex_match(route_path, match, route_{index})) {{']
    declarations, decodes, arguments = [], [], []
    names = ", ".join(json.dumps(n) for n in binding.query)
    if binding.query:
        decodes += ["            const auto query_values = decode_query(query_string);",
                    f"            check_query_names(query_values, {{{names}}});"]
    else:
        decodes.append("            check_query_names(decode_query(query_string), {});")
    for argument in route.arguments:
        decl = argument.cpp_declaration
        decl = re.sub(r"^const (.*)&", r"\1", decl)
        # Types core declares in holder::contract, such as ListCardsOrder or NewCard.
        decl = re.sub(r"(?<![\w:])([A-Z]\w*)", r"holder::contract::\1", decl)
        declarations.append(f"        {decl}{{}};")
        pname = argument.parameter.name
        arguments.append(f"std::move({pname})")
        if argument.location == "path":
            source = f"decode_uri_component(match[{captures[pname]}].str(), false)"
            if isinstance(argument.ref, LiteralType):
                choices = argument.ref.values
                type_name = "holder::contract::" + camel(name) + camel(pname)
                decodes.append(f"            const auto {pname}_text = {source};")
                for i, value in enumerate(choices):
                    decodes.append(f'            {"if" if i == 0 else "else if"} ({pname}_text == {json.dumps(value)}) '
                                   f"{pname} = {type_name}::{camel(str(value))};")
                decodes.append(f'            else throw std::invalid_argument("{pname} must be one of {", ".join(choices)}");')
            else:
                decodes.append(f"            {pname} = {scalar_value(argument.ref, source, pname)};")
        elif argument.location == "query":
            decodes += query_lines(contract, route, argument)
        else:
            if argument.body_record is None:
                raise Unsupported(f"{name}({pname}): a request body that is not a record")
            record = "holder::contract::" + argument.body_record
            optional = argument.body_optional
            decodes.append(f"            {pname} = contract_json::decode<{record}>("
                           f"decode_body(body{', true' if optional else ''}));")
    status, code = INVALID_ARGUMENT
    lines += declarations
    if decodes:
        lines += ["        try {", *decodes,
                  "        } catch (const std::invalid_argument& error) {",
                  f'            return failure({status}, "{code}", error.what(), "{name}");',
                  "        }"]
    call = f"holder::contract::{name}(context{''.join(', ' + a for a in arguments)})"
    lines.append("        try {")
    if created_when := binding.created_when:
        lines += [f"            auto result = {call};",
                  (f'            return success(result.{created_when} ? 201 : {binding.status}, '
                   f'contract_json::encode(result), "{name}");')]
    else:
        lines.append(f'            return success({binding.status}, contract_json::encode({call}), "{name}");')
    for error in op.errors:
        lines += [f"        }} catch (const holder::contract::{error}& error) {{",
                  f'            return failure({statuses[error]}, "{snake(error)}", error.what(), "{name}");']
    unexpected_status, unexpected_code = UNEXPECTED
    lines += ["        } catch (const holder::contract::InvalidArgument& error) {",
              f'            return failure({status}, "{code}", error.what(), "{name}");',
              "        } catch (const std::exception& error) {",
              f'            return failure({unexpected_status}, "{unexpected_code}", error.what(), "{name}");',
              "        }", "    }"]
    return lines


def generate_routes(contract: ContractView | Model, codec_header: str) -> str:
    if not re.fullmatch(r"[A-Za-z_0-9/.-]+", codec_header):
        raise ValueError("invalid codec header path")
    typed = contract if isinstance(contract, ContractView) else ContractView.model_validate(contract.ir)
    try:
        plan = plan_http(typed)
    except ValueError as error:
        raise Unsupported(str(error)) from error
    validate_routes(plan.bindings)
    bindings = plan.routes
    all_bindings = plan.bindings
    statuses = plan.error_statuses
    lines = [*header_lines(typed.model_dump(exclude_unset=True), "The daemon's HTTP routes for the contract's operations; "
                                     "contract/holder_contract/http_routes.py says how they behave.", "cpp"),
             "#pragma once",
             f'#include "{codec_header}"',
             "#include <charconv>",
             "#include <algorithm>",
             "#include <initializer_list>",
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
    if any(route.binding.query for route in bindings):
        lines += ["    const auto query_string = query_start == std::string::npos ? std::string_view{} :",
                  "        std::string_view(path).substr(query_start + 1);"]
    lines += ["    std::smatch match;", "    (void)body;", "    (void)context;"]
    # Routes are tried from the most specific: a literal segment before a parameter.
    ordered = sorted(enumerate(bindings), key=lambda pair: route_priority(pair[1].binding))
    for index, route in ordered:
        lines += operation_lines(typed, route, index, statuses)
    lines += ["    return std::nullopt;", "}", "",
              "// The methods dispatch answers on this path, for a 405's Allow header.",
              "inline std::set<std::string_view> allowed_methods(const std::string& path) {",
              "    const auto route_path = path.substr(0, path.find('?'));",
              "    std::set<std::string_view> methods;"]
    patterns: dict[str, tuple[str, set[str]]] = {}
    for binding in all_bindings:
        pattern = route_pattern(binding.path)[0]
        if pattern not in patterns:
            patterns[pattern] = (binding.path, set())
        patterns[pattern][1].add(binding.method)
    for position, (pattern, (_, methods)) in enumerate(
            sorted(patterns.items(), key=lambda item: tuple(kind for kind, _ in route_segments(item[1][0])))):
        listed = ", ".join(json.dumps(m) for m in sorted(methods))
        lines += [f"    static const std::regex path_{position}({json.dumps(pattern)});",
                  f"    if (std::regex_match(route_path, path_{position})) return {{{listed}}};"]
    lines += ["    return methods;", "}",
              "// Public routes do not require the daemon's bearer token.",
              "inline bool is_public(const std::string& method, const std::string& path) {",
              "    const auto route_path = path.substr(0, path.find('?'));"]
    for position, binding in enumerate(b for b in all_bindings if b.public):
        lines += [f'    static const std::regex public_{position}({json.dumps(route_pattern(binding.path)[0])});',
                  f'    if (method == "{binding.method}" && std::regex_match(route_path, public_{position})) return true;']
    lines += ["    return false;", "}", f"}}  // namespace {NAMESPACE}", ""]
    return "\n".join(lines)
