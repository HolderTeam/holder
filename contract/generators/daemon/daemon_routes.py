"""Dispatch complete daemon boundaries through explicit runtime service bindings."""

from __future__ import annotations

import json

from ..common import contract_sources, header_lines
from .boundary import BoundaryPlan
from .daemon_json import cpp_type
from .paths import route_pattern, validate_routes


def generate_routes(plan: BoundaryPlan) -> str:
    validate_routes(plan.contract.http)
    lines = [
        *header_lines(contract_sources(plan.contract), "Complete daemon request/response boundaries.", "cpp"),
        "#pragma once",
        '#include "api/generated/DaemonJson.h"',
        '#include "api/generated/ContractRoutes.h"',
        *[f'#include "{header}"' for header in sorted({route.runtime.header for route in plan.routes})],
        "#include <optional>",
        "#include <regex>",
        "#include <string>",
        "#include <string_view>",
        "#include <type_traits>",
        "#include <utility>",
        "",
        "namespace daemon_http {",
        "struct response { int status; nlohmann::json body; };",
        "",
        "inline response failure(int status, const std::exception& error) {",
        "    const auto type = std::type_identity<holder::api::ErrorResponse>{};",
        "    if (const auto* coded = dynamic_cast<const holder::api::CodedError*>(&error))",
        "        return {status, daemon_json::encode(holder::api::api_error_body(type, status, *coded))};",
        # Preserve the existing resource-boundary diagnostic codes for generic failures.
        "    auto result = holder::api::api_error_body(type, status, error);",
        '    if (status == 400) result.error.code = "invalid_argument";',
        '    if (status == 500) result.error.code = "internal_error";',
        "    return {status, daemon_json::encode(result)};",
        "}",
        "",
        "template <typename Context>",
        "std::optional<response> dispatch(const std::string& method, const std::string& path,",
        "                                 const std::string& body, Context& context) {",
        "    const auto split = path.find('?');",
        "    const auto route_path = path.substr(0, split);",
        "    const auto query = split == std::string::npos ? std::string_view{} : std::string_view(path).substr(split + 1);",
        "    std::smatch match;",
    ]
    for index, route in enumerate(plan.routes):
        pattern, captures = route_pattern(route.http.path)
        lines += [
            f"    static const std::regex route_{index}({json.dumps(pattern)});",
            f"    if (method == {json.dumps(route.http.method)} && std::regex_match(route_path, match, route_{index})) {{",
            "        try {",
            "            contract_http::check_query_names(contract_http::decode_query(query), {});",
        ]
        for parameter in route.operation.parameters:
            if parameter.name == route.http.body:
                record = plan.contract.record(plan.resolve(parameter.type).name)
                optional = all(field.omittable or "default" in field.model_fields_set for field in record.fields)
                lines += [
                    f"            auto arg_{parameter.name} = daemon_json::decode<{cpp_type(plan, parameter.type)}>",
                    f"                (contract_http::decode_body(body{', true' if optional else ''}));",
                ]
            else:
                ref = plan.resolve(parameter.type)
                low = ref.min_length or 0
                high = ref.max_length if ref.max_length is not None else "std::numeric_limits<std::size_t>::max()"
                lines.append(
                    f"            auto arg_{parameter.name} = daemon_json::check_text("
                    f"contract_http::decode_uri_component(match[{captures[parameter.name]}].str(), false), "
                    f"{json.dumps(parameter.name)}, {low}, {high});"
                )
        arguments = [f"context.{service}" for service in route.runtime.services]
        arguments += [f"std::move(arg_{name})" for name in route.runtime.arguments]
        lines += [
            f"            const auto result = {route.runtime.handler}({', '.join(arguments)});",
            f"            return response{{{route.http.status}, daemon_json::encode(result)}};",
        ]
        for exception, status in route.runtime.exceptions.items():
            lines += [
                f"        }} catch (const {exception}& error) {{",
                f"            return failure({status}, error);",
            ]
        lines += [
            "        } catch (const std::invalid_argument& error) {",
            "            return failure(400, error);",
            "        } catch (const std::out_of_range& error) {",
            "            return failure(400, error);",
            "        } catch (const std::exception& error) {",
            "            return failure(500, error);",
            "        }",
            "    }",
        ]
    lines += ["    return std::nullopt;", "}", "} // namespace daemon_http", ""]
    return "\n".join(lines)
