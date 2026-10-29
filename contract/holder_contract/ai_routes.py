"""Generate daemon AI dispatch from the Python contract's HTTP bindings.

The service arguments below connect each declared operation to its daemon handler. The
contract supplies paths, methods, request parameters, status codes and record schemas.
"""

from __future__ import annotations

import json
from typing import Any

from .generate import Unsupported, header_lines
from .http_routes import route_pattern, route_priority, validate_routes


def _groups() -> dict[str, tuple[str, ...]]:
    groups = {
        "db, fts": "get_ai_messages get_ai_message get_ai_message_links get_ai_message_backlinks "
                   "patch_ai_messages_message_id delete_ai_messages_message_id "
                   "post_ai_messages_message_id_restore post_ai_messages_message_id_links "
                   "delete_ai_messages_message_id_links",
        "db, fts, uuid_v4": "post_ai_messages post_ai_messages_capture",
        "db, registry": "get_ai_nudges post_ai_nudges_evaluate get_ai_capabilities get_ai_status "
                        "patch_ai_runners_runner_id delete_ai_runners_runner_id "
                        "post_ai_nudges_nudge_id_dismiss",
        "registry": "get_ai_runners get_ai_runner post_ai_runner_pull get_pull_status "
                    "post_ai_runners_runner_id_retry",
        "db, registry, uuid_v4": "post_ai_runners",
        "db": "get_ai_runs get_ai_threads list_provider_credentials get_local_model_config "
              "put_local_model_config get_ai_provider_catalog list_provider_settings "
              "set_provider_setting delete_provider_setting get_ai_run get_ai_thread "
              "patch_ai_threads_thread_id delete_ai_threads_thread_id",
        "db, uuid_v4": "post_ai_threads",
        "db, secret_store": "upsert_provider_credential delete_provider_credential",
        "registry, events": "get_ai_runner_pull_job_id_events",
        "db, events": "get_ai_runs_run_id_events",
        "socket, db, fts, secret_store, registry, uuid_v4, events": "post_ai_runs",
    }
    return {name: tuple(f"context.{part.strip()}" for part in arguments.split(","))
            for arguments, names in groups.items() for name in names.split()}


INJECTIONS = _groups()
CALL_ORDER = {
    "get_ai_message_links": ("message_id", "include_deleted"),
    "get_ai_message_backlinks": ("message_id", "include_deleted"),
}

BODY_SUPPORT = r"""
// JSON's parser converts negative integers outside int64 to floating point. Check their raw
// tokens first, so an underflow cannot round back into the contract's accepted range.
inline void check_negative_integer_tokens(std::string_view body) {
    bool quoted = false, escaped = false;
    for (std::size_t i = 0; i < body.size(); ++i) {
        const char c = body[i];
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
            continue;
        }
        if (c == '"') { quoted = true; continue; }
        if (c != '-' || i + 1 >= body.size() || body[i + 1] < '0' || body[i + 1] > '9') continue;
        std::size_t end = i + 2;
        while (end < body.size() && body[end] >= '0' && body[end] <= '9') ++end;
        if (end < body.size() && (body[end] == '.' || body[end] == 'e' || body[end] == 'E')) {
            i = end - 1;
            continue;
        }
        long long number;
        const auto [last, error] = std::from_chars(body.data() + i, body.data() + end, number);
        if (error == std::errc::result_out_of_range)
            throw std::invalid_argument("integer outside int64 range");
        (void)last;
        i = end - 1;
    }
}
template <typename T> T decode_body(const std::string& body) {
    check_negative_integer_tokens(body);
    return ai_json::decode<T>(contract_http::decode_body(body));
}
"""


def _cpp_type(ref: dict[str, Any]) -> str:
    if ref["kind"] == "nullable":
        return f"std::optional<{_cpp_type(ref['value'])}>"
    if ref["kind"] in ("string", "literal"):
        return "int" if ref["kind"] == "literal" and isinstance(ref["values"][0], int) else "std::string"
    if ref["kind"] == "named":
        name = ref["name"]
        namespace = ("holder::api::routes" if name in {
            "CardLink", "CardLinkCreateRequest", "CardLinkCreateResponse", "CardLinkListResponse"
        } else "holder::api")
        return f"{namespace}::{name}"
    raise Unsupported(f"AI request parameter type {ref['kind']}")


def _field_lines(parameter: dict[str, Any], source: str) -> list[str]:
    """Decode a path or query value and apply its scalar contract constraints."""
    name, ref = parameter["name"], parameter["type"]
    inner = ref["value"] if ref["kind"] == "nullable" else ref
    if inner["kind"] == "literal":
        values = inner["values"]
        if isinstance(values[0], int):
            result = f'contract_http::decode_integer<int>({source}, "{name}")'
            condition = " && ".join(f"arg_{name} != {v}" for v in values)
        else:
            result = source
            condition = " && ".join(f"arg_{name} != {json.dumps(v)}" for v in values)
        return [f"auto arg_{name} = {result};",
                f'if ({condition}) throw std::invalid_argument("invalid {name}");']
    if inner["kind"] != "string":
        raise Unsupported(f"{name}: unsupported AI URL parameter {inner['kind']}")
    low = inner.get("min_length", 0)
    high = inner.get("max_length", "std::numeric_limits<std::size_t>::max()")
    return [f'auto arg_{name} = ai_json::check_text({source}, "{name}", {low}, {high});']


def _argument_lines(op: dict[str, Any], binding: dict[str, Any], captures: dict[str, int]) -> tuple[list[str], list[str]]:
    parameters = {p["name"]: p for p in op["parameters"]}
    body = binding.get("body")
    queries = binding["query"]
    paths = binding["path_parameters"]
    headers = [name for name in parameters if name not in {*queries, *paths, body}]
    order = [*queries, *headers, *paths, *([body] if body else [])]
    if set(order) != set(parameters):
        raise Unsupported(f"{op['name']}: route parameter inventory differs from operation")
    lines = ["const auto query_values = contract_http::decode_query(query_string);",
             "contract_http::check_query_names(query_values, {" + ", ".join(json.dumps(n) for n in queries) + "});"]
    for name in order:
        parameter = parameters[name]
        if name in queries:
            lines += [f'const auto query_{name} = query_values.find("{name}");']
            if "default" not in parameter:
                lines += [f'if (query_{name} == query_values.end()) throw std::invalid_argument("missing {name}");']
                lines += _field_lines(parameter, f"query_{name}->second")
            else:
                cpp = _cpp_type(parameter["type"])
                if cpp.startswith("std::optional<"):
                    cpp = cpp[len("std::optional<"):-1]
                lines += [f"std::optional<{cpp}> arg_{name};",
                          f"if (query_{name} != query_values.end()) {{"]
                decoded = _field_lines(parameter, f"query_{name}->second")
                # The decoded local has a distinct name so it does not shadow the optional.
                lines += [f"    {line.replace(f'arg_{name}', f'decoded_{name}')}" for line in decoded]
                lines += [f"    arg_{name} = std::move(decoded_{name});", "}"]
        elif name in paths:
            source = f"contract_http::decode_uri_component(match[{captures[name]}].str(), false)"
            lines += _field_lines(parameter, source)
        elif name == body:
            cpp = _cpp_type(parameter["type"])
            if "default" in parameter:
                lines += [f"std::optional<{cpp}> arg_{name};",
                          f"if (!body.empty()) arg_{name} = decode_body<{cpp}>(body);"]
            else:
                lines += [f"auto arg_{name} = decode_body<{cpp}>(body);"]
        else:
            if name != "last_event_id":
                raise Unsupported(f"{op['name']}: unsupported header {name}")
            lines += [f'auto arg_{name} = header ? header("Last-Event-ID") : std::nullopt;']
    call_order = CALL_ORDER.get(op["name"], order)
    if set(call_order) != set(order):
        raise Unsupported(f"{op['name']}: handler argument inventory differs from contract")
    return lines, [f"std::move(arg_{name})" for name in call_order]


def generate_ai_routes(ir: dict[str, Any]) -> str:
    operations = {op["name"]: op for op in ir["operations"] if op["daemon_only"]}
    bindings = [b for b in ir["http"] if b["operation"] in operations and b["path"].startswith("/ai/")]
    validate_routes(ir["http"])
    declared = {b["operation"] for b in bindings}
    if declared != INJECTIONS.keys():
        raise Unsupported(f"AI handler wiring differs from contract: missing={sorted(declared - INJECTIONS.keys())}, "
                          f"unused={sorted(INJECTIONS.keys() - declared)}")
    lines = [*header_lines(ir, "AI HTTP routes generated from contract/interface/app.py.", "cpp"),
             "#pragma once", '#include "api/generated/AiJson.h"',
             '#include "api/generated/ContractRoutes.h"',
             "#include <any>", "#include <charconv>", "#include <functional>", "#include <optional>",
             "#include <regex>", "#include <string>", "#include <string_view>",
             "#include <system_error>", "#include <type_traits>", "#include <utility>",
             "namespace ai_http {",
             "using json = nlohmann::json;",
             "struct response { int status; json body; std::string_view operation;",
             "    std::any host = {};",
             '    std::string_view media_type = "application/json"; };',
             "using header_lookup = std::function<std::optional<std::string>(std::string_view)>;",
             BODY_SUPPORT.rstrip(),
             "inline int error_status(const std::exception& error) {",
             "    if (dynamic_cast<const std::invalid_argument*>(&error)) return 400;",
             "    if (dynamic_cast<const holder::model::ConflictError*>(&error)) return 409;"]
    for status, classes in [
        (400, "AiProviderCatalogMissing AiRunsMissingScope AiMessageWrongState"),
        (404, ("AiCaptureNotFound AiMessageNotFound AiNudgeNotFound AiRunNotFound "
               "AiRunnerNotConfigured AiRunnerNotFound AiThreadNotFound PullJobNotFound RunnerNotConfigured")),
        (503, "AiRunUnavailable ProviderCredentialSecretFailure RunnerUnavailable SecretStoreUnavailable")]:
        for name in classes.split():
            lines.append(f"    if (dynamic_cast<const holder::api::{name}*>(&error)) return {status};")
    lines += ["    return 500;", "}",
              "inline response failure(int status, const std::exception& error, std::string_view operation) {",
              "    const auto type = std::type_identity<holder::api::ErrorResponse>{};",
              "    if (const auto* privacy = dynamic_cast<const holder::privacy::PrivacyError*>(&error))",
              "        return {status, ai_json::encode(holder::api::api_error_body(type, status, *privacy)), operation};",
              "    if (const auto* coded = dynamic_cast<const holder::api::CodedError*>(&error))",
              "        return {status, ai_json::encode(holder::api::api_error_body(type, status, *coded)), operation};",
              "    return {status, ai_json::encode(holder::api::api_error_body(type, status, error)), operation};",
              "}",
              "template <typename Context>",
              "inline std::optional<response> dispatch(const std::string& method, const std::string& path,",
              "    const std::string& body, Context& context, const header_lookup& header = {}) {",
              "    const auto separator = path.find('?');",
              "    const auto route_path = path.substr(0, separator);",
              "    const auto query_string = separator == std::string::npos ? std::string_view{} :",
              "        std::string_view(path).substr(separator + 1);",
              "    std::smatch match;", "    (void)body; (void)context; (void)header;"]
    for index, binding in sorted(enumerate(bindings), key=lambda pair: route_priority(pair[1])):
        op = operations[binding["operation"]]
        name = op["name"]
        pattern, captures = route_pattern(binding["path"])
        decodes, arguments = _argument_lines(op, binding, captures)
        injections = INJECTIONS[name]
        call = f"holder::api::{name}({', '.join([*injections, *arguments])})"
        lines += [f"    static const std::regex route_{index}({json.dumps(pattern)});",
                  f'    if (method == "{binding["method"]}" && std::regex_match(route_path, match, route_{index})) {{',
                  "        try {"]
        lines += [f"            {line}" for line in decodes]
        if op["returns"]["kind"] == "stream":
            lines += [f"            auto outcome = {call};",
                      "            if (outcome.status != 200 && outcome.status != 204)",
                      f'                throw std::logic_error("{name} chose an undeclared success status");',
                      f'            return response{{outcome.status, json(), "{name}", std::move(outcome.value)}};']
        else:
            lines += [f'            return response{{{binding["status"]}, ai_json::encode({call}), "{name}"}};']
        lines += ["        } catch (const std::exception& error) {",
                  f'            return failure(error_status(error), error, "{name}");',
                  "        }", "    }"]
    lines += ["    return std::nullopt;", "}", "} // namespace ai_http", ""]
    return "\n".join(lines)
