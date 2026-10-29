// Generated from RIG IR; do not edit.
#pragma once
#include "api/generated/RuntimeJson.h"
#include <any>
#include <charconv>
#include <exception>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

namespace rig_http {
using json = nlohmann::json;
struct response {
    int status;
    json body;
    std::string_view operation;
    // The methods the path accepts, for a 405's Allow header; empty otherwise.
    std::string allow = {};
    // A host-value operation's value, for the host to write the body with; empty otherwise.
    std::any host = {};
    // A text operation's media type, whose body is a JSON string the host writes as text.
    std::string_view media_type = "application/json";
};
// The error passed to rig_error_body for a 405.
struct method_not_allowed : std::runtime_error {
    using std::runtime_error::runtime_error;
};
// The application supplies rig_error_body, found by argument-dependent
// lookup, to build each declared error body from an exception.
template <typename Body, typename Error>
json encode_error(int status, const Error& error) {
    const Body body = rig_error_body(std::type_identity<Body>{}, status, error);
    return rig_json::encode(body);
}
inline response invalid_request(const std::exception& error, std::string_view operation) {
    return {400, encode_error<holder::api::ErrorResponse>(400, error), operation};
}
inline void enforce_range(int value, int minimum, int maximum, const char* name) {
    if (value < minimum || value > maximum)
        throw std::invalid_argument(std::string("out of range: ") + name);
}
inline void enforce_range(const std::optional<int>& value,
                          int minimum, int maximum, const char* name) {
    if (value) enforce_range(*value, minimum, maximum, name);
}
inline void enforce_int_values(int value, std::initializer_list<int> choices, const char* name) {
    for (int choice : choices) if (value == choice) return;
    throw std::invalid_argument(std::string("invalid integer value: ") + name);
}
inline void enforce_int_values(const std::optional<int>& value,
                               std::initializer_list<int> choices, const char* name) {
    if (value) enforce_int_values(*value, choices, name);
}
inline void enforce_string_values(const std::string& value,
                                  std::initializer_list<std::string_view> choices, const char* name) {
    for (auto choice : choices) if (value == choice) return;
    throw std::invalid_argument(std::string("invalid value: ") + name);
}
inline void enforce_string_values(const std::optional<std::string>& value,
                                  std::initializer_list<std::string_view> choices, const char* name) {
    if (value) enforce_string_values(*value, choices, name);
}
inline void enforce_length(const std::string& value, std::size_t minimum, std::size_t maximum,
                           const char* name) {
    if (value.size() < minimum || value.size() > maximum)
        throw std::invalid_argument(std::string("invalid length: ") + name);
}
inline void enforce_length(const std::optional<std::string>& value, std::size_t minimum,
                           std::size_t maximum, const char* name) {
    if (value) enforce_length(*value, minimum, maximum, name);
}
inline void enforce_pattern(const std::string& value, const std::regex& pattern, const char* name) {
    if (!std::regex_search(value, pattern))
        throw std::invalid_argument(std::string("invalid format: ") + name);
}
inline void enforce_pattern(const std::optional<std::string>& value, const std::regex& pattern,
                            const char* name) {
    if (value) enforce_pattern(*value, pattern, name);
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
inline std::string decode_path_string(std::string_view input) {
    return decode_uri_component(input, false);
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
inline int decode_int(std::string_view input, const char* message) {
    int value;
    const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), value);
    if (error != std::errc{} || end != input.data() + input.size())
        throw std::invalid_argument(message);
    return value;
}
inline int decode_path_int(std::string_view input) {
    return decode_int(input, "invalid integer path parameter");
}
inline int decode_query_int(std::string_view input) {
    return decode_int(input, "invalid integer query parameter");
}
inline long long decode_query_int64(std::string_view input) {
    long long value;
    const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), value);
    if (error != std::errc{} || end != input.data() + input.size())
        throw std::invalid_argument("invalid integer query parameter");
    return value;
}
inline bool decode_query_bool(std::string_view input) {
    if (input == "true" || input == "1") return true;
    if (input == "false" || input == "0") return false;
    throw std::invalid_argument("invalid boolean query parameter");
}
template <typename T> T decode_body(const std::string& body) {
    try {
        return rig_json::decode<T>(json::parse(body));
    } catch (const json::exception& error) {
        throw std::invalid_argument(error.what());
    }
}
// Looks up a request header by name, ignoring case; nothing when it is absent.
using header_lookup = std::function<std::optional<std::string>(std::string_view)>;
template <typename Context>
inline std::optional<response> dispatch(const std::string& method,
                                    const std::string& path,
                                    const std::string& body, Context& context,
                                    const header_lookup& header = {}) {
    const auto query_start = path.find('?');
    const auto route_path = path.substr(0, query_start);
    const auto query_string = query_start == std::string::npos ? std::string_view{} :
        std::string_view(path).substr(query_start + 1);
    std::smatch match;
    (void)body;
    (void)header;
    static const std::regex route_37("^/alive$");
    if (method == "GET" && std::regex_match(route_path, match, route_37)) {
        try {
            return response{200, rig_json::encode(holder::api::check_alive()), "check_alive"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "check_alive"};
        }
    }
    static const std::regex route_38("^/bye$");
    if (method == "POST" && std::regex_match(route_path, match, route_38)) {
        try {
            return response{200, rig_json::encode(holder::api::say_goodbye()), "say_goodbye"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "say_goodbye"};
        }
    }
    static const std::regex route_42("^/health$");
    if (method == "GET" && std::regex_match(route_path, match, route_42)) {
        try {
            return response{200, rig_json::encode(holder::api::get_health(context.db, context.started_at)), "get_health"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_health"};
        }
    }
    static const std::regex route_43("^/locations$");
    if (method == "GET" && std::regex_match(route_path, match, route_43)) {
        std::optional<std::string> argument_0;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_locations");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_locations");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_locations(context.db, context.secret_store, std::move(*argument_0))), "get_locations"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_locations"};
        }
    }
    static const std::regex route_47("^/ping$");
    if (method == "GET" && std::regex_match(route_path, match, route_47)) {
        try {
            return response{200, json(holder::api::get_ping()), "get_ping", {}, {}, "text/plain"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ping"};
        }
    }
    static const std::regex route_48("^/openapi\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, match, route_48)) {
        try {
            return response{200, json(holder::api::download_openapi_yaml()), "download_openapi_yaml", {}, {}, "application/yaml"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "download_openapi_yaml"};
        }
    }
    static const std::regex route_49("^/openapi\\.json$");
    if (method == "GET" && std::regex_match(route_path, match, route_49)) {
        try {
            return response{200, rig_json::encode(holder::api::download_openapi_json()), "download_openapi_json"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "download_openapi_json"};
        }
    }
    static const std::regex route_50("^/ai_catalog\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, match, route_50)) {
        try {
            return response{200, json(holder::api::download_ai_catalog_yaml()), "download_ai_catalog_yaml", {}, {}, "application/yaml"};
        } catch (const holder::api::StaticFileNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "download_ai_catalog_yaml"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "download_ai_catalog_yaml"};
        }
    }
    static const std::regex route_51("^/git_providers\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, match, route_51)) {
        try {
            return response{200, json(holder::api::download_git_providers_yaml()), "download_git_providers_yaml", {}, {}, "application/yaml"};
        } catch (const holder::api::StaticFileNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "download_git_providers_yaml"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "download_git_providers_yaml"};
        }
    }
    static const std::regex route_52("^/ai_catalog\\.json$");
    if (method == "GET" && std::regex_match(route_path, match, route_52)) {
        try {
            return response{200, rig_json::encode(holder::api::download_ai_catalog_json()), "download_ai_catalog_json"};
        } catch (const holder::api::StaticFileNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "download_ai_catalog_json"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "download_ai_catalog_json"};
        }
    }
    static const std::regex route_53("^/git_providers\\.json$");
    if (method == "GET" && std::regex_match(route_path, match, route_53)) {
        try {
            return response{200, rig_json::encode(holder::api::download_git_providers_json()), "download_git_providers_json"};
        } catch (const holder::api::StaticFileNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "download_git_providers_json"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "download_git_providers_json"};
        }
    }
    static const std::regex route_54("^/docs$");
    if (method == "GET" && std::regex_match(route_path, match, route_54)) {
        try {
            auto outcome = holder::api::get_docs();
            if (outcome.status != 200)
                throw std::logic_error("get_docs chose an undeclared success status");
            return response{outcome.status, json(), "get_docs", {}, std::move(outcome.value)};
        } catch (const holder::api::StaticFileNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_docs"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_docs"};
        }
    }
    static const std::regex route_63("^/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_63)) {
        std::optional<std::optional<std::string>> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        std::optional<std::optional<std::string>> argument_2;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                argument_0.emplace(std::nullopt);
            else
                argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("last_revision");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(std::string(query_1->second));
            auto header_2 = header ? header("Last-Event-ID") : std::nullopt;
            argument_2.emplace(std::move(header_2));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
            static const std::regex pattern_1("^[0-9a-f]{40}$");
            enforce_pattern(*argument_1, pattern_1, "last_revision");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_events");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_events");
        }
        try {
            auto outcome = holder::api::get_events(context.db, context.events, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2));
            if (outcome.status != 200)
                throw std::logic_error("get_events chose an undeclared success status");
            return response{outcome.status, json(), "get_events", {}, std::move(outcome.value)};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_events"};
        } catch (const holder::api::EventCursorProjectNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_events"};
        } catch (const holder::api::ChangeFeedUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "get_events"};
        } catch (const holder::api::ProjectUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "get_events"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_events"};
        }
    }
    static const std::regex route_7("^/ai/messages$");
    if (method == "GET" && std::regex_match(route_path, match, route_7)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<int>> argument_1;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("thread_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: thread_id");
            argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("include_deleted");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(decode_query_int(query_1->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "thread_id");
            enforce_int_values(*argument_1, {0, 1}, "include_deleted");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_messages");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_messages");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_messages(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "get_ai_messages"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_messages"};
        }
    }
    static const std::regex route_11("^/ai/messages$");
    if (method == "POST" && std::regex_match(route_path, match, route_11)) {
        std::optional<holder::api::AiMessageCreateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiMessageCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_messages");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_messages");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_ai_messages(context.db, context.fts, context.uuid_v4, std::move(*argument_0))), "post_ai_messages"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_messages"};
        } catch (const holder::model::ConflictError& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_ai_messages"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_messages"};
        }
    }
    static const std::regex route_18("^/ai/nudges$");
    if (method == "GET" && std::regex_match(route_path, match, route_18)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("card_id");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(std::string(query_1->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_nudges");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_nudges");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_nudges(context.db, context.registry, std::move(*argument_0), std::move(*argument_1))), "get_ai_nudges"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_nudges"};
        }
    }
    static const std::regex route_22("^/ai/runners$");
    if (method == "GET" && std::regex_match(route_path, match, route_22)) {
        try {
            return response{200, rig_json::encode(holder::api::get_ai_runners(context.registry)), "get_ai_runners"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_runners"};
        }
    }
    static const std::regex route_24("^/ai/runners$");
    if (method == "POST" && std::regex_match(route_path, match, route_24)) {
        std::optional<holder::api::AiRunnerCreateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiRunnerCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_runners");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_runners");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_ai_runners(context.db, context.registry, context.uuid_v4, std::move(*argument_0))), "post_ai_runners"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_runners"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_runners"};
        }
    }
    static const std::regex route_28("^/ai/runs$");
    if (method == "GET" && std::regex_match(route_path, match, route_28)) {
        std::optional<std::optional<std::string>> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                argument_0.emplace(std::nullopt);
            else
                argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("thread_id");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(std::string(query_1->second));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_runs");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_runs");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_runs(context.db, std::move(*argument_0), std::move(*argument_1))), "get_ai_runs"};
        } catch (const holder::api::AiRunsMissingScope& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_ai_runs"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_runs"};
        }
    }
    static const std::regex route_30("^/ai/capabilities$");
    if (method == "GET" && std::regex_match(route_path, match, route_30)) {
        std::optional<std::optional<std::string>> argument_0;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("runner_id");
            if (query_0 == query_values.end())
                argument_0.emplace(std::nullopt);
            else
                argument_0.emplace(std::string(query_0->second));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_capabilities");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_capabilities");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_capabilities(context.db, context.registry, std::move(*argument_0))), "get_ai_capabilities"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_capabilities"};
        }
    }
    static const std::regex route_31("^/ai/status$");
    if (method == "GET" && std::regex_match(route_path, match, route_31)) {
        try {
            return response{200, rig_json::encode(holder::api::get_ai_status(context.db, context.registry)), "get_ai_status"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_status"};
        }
    }
    static const std::regex route_32("^/ai/threads$");
    if (method == "GET" && std::regex_match(route_path, match, route_32)) {
        std::optional<std::string> argument_0;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_threads");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_threads");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_threads(context.db, std::move(*argument_0))), "get_ai_threads"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_threads"};
        }
    }
    static const std::regex route_34("^/ai/threads$");
    if (method == "POST" && std::regex_match(route_path, match, route_34)) {
        std::optional<holder::api::AiThreadCreateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiThreadCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_threads");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_threads");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_ai_threads(context.db, context.uuid_v4, std::move(*argument_0))), "post_ai_threads"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_threads"};
        } catch (const holder::model::ConflictError& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_ai_threads"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_threads"};
        }
    }
    static const std::regex route_39("^/events/cursor$");
    if (method == "GET" && std::regex_match(route_path, match, route_39)) {
        std::optional<std::optional<std::string>> argument_0;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                argument_0.emplace(std::nullopt);
            else
                argument_0.emplace(std::string(query_0->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_events_cursor");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_events_cursor");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_events_cursor(context.db, context.events, std::move(*argument_0))), "get_events_cursor"};
        } catch (const holder::api::EventCursorProjectNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_events_cursor"};
        } catch (const holder::api::ChangeFeedUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "get_events_cursor"};
        } catch (const holder::api::ProjectUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "get_events_cursor"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_events_cursor"};
        }
    }
    static const std::regex route_59("^/locations/preferred$");
    if (method == "PUT" && std::regex_match(route_path, match, route_59)) {
        std::optional<holder::api::PutLocationsPreferredRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::PutLocationsPreferredRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "put_locations_preferred");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "put_locations_preferred");
        }
        try {
            return response{200, rig_json::encode(holder::api::put_locations_preferred(context.db, context.secret_store, std::move(*argument_0))), "put_locations_preferred"};
        } catch (const holder::api::LocationNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "put_locations_preferred"};
        } catch (const holder::api::SecretStoreUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "put_locations_preferred"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "put_locations_preferred"};
        }
    }
    static const std::regex route_60("^/ai/runs$");
    if (method == "POST" && std::regex_match(route_path, match, route_60)) {
        std::optional<holder::api::AiRunCreateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiRunCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_runs");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_runs");
        }
        try {
            auto outcome = holder::api::post_ai_runs(context.socket, context.db, context.fts, context.secret_store, context.registry, context.uuid_v4, context.events, std::move(*argument_0));
            if (outcome.status != 200)
                throw std::logic_error("post_ai_runs chose an undeclared success status");
            return response{outcome.status, json(), "post_ai_runs", {}, std::move(outcome.value)};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_runs"};
        } catch (const holder::api::AiRunUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_ai_runs"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_runs"};
        }
    }
    static const std::regex route_1("^/ai/runner/pull$");
    if (method == "POST" && std::regex_match(route_path, match, route_1)) {
        std::optional<holder::api::PostAiRunnerPullRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::PostAiRunnerPullRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_runner_pull");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_runner_pull");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_ai_runner_pull(context.registry, std::move(*argument_0))), "post_ai_runner_pull"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_runner_pull"};
        } catch (const holder::api::RunnerNotConfigured& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_ai_runner_pull"};
        } catch (const holder::api::RunnerUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_ai_runner_pull"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_runner_pull"};
        }
    }
    static const std::regex route_2("^/ai/providers/credentials$");
    if (method == "GET" && std::regex_match(route_path, match, route_2)) {
        try {
            return response{200, rig_json::encode(holder::api::list_provider_credentials(context.db)), "list_provider_credentials"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_provider_credentials"};
        }
    }
    static const std::regex route_3("^/ai/providers/credentials$");
    if (method == "PUT" && std::regex_match(route_path, match, route_3)) {
        std::optional<holder::api::AiProviderCredentialUpsertRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiProviderCredentialUpsertRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "upsert_provider_credential");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "upsert_provider_credential");
        }
        try {
            return response{200, rig_json::encode(holder::api::upsert_provider_credential(context.db, context.secret_store, std::move(*argument_0))), "upsert_provider_credential"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "upsert_provider_credential"};
        } catch (const holder::api::SecretStoreUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "upsert_provider_credential"};
        } catch (const holder::api::ProviderCredentialSecretFailure& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "upsert_provider_credential"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "upsert_provider_credential"};
        }
    }
    static const std::regex route_5("^/ai/local\\-models/config$");
    if (method == "GET" && std::regex_match(route_path, match, route_5)) {
        try {
            return response{200, rig_json::encode(holder::api::get_local_model_config(context.db)), "get_local_model_config"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_local_model_config"};
        }
    }
    static const std::regex route_6("^/ai/local\\-models/config$");
    if (method == "PUT" && std::regex_match(route_path, match, route_6)) {
        std::optional<holder::api::AiLocalModelConfigUpdateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiLocalModelConfigUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "put_local_model_config");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "put_local_model_config");
        }
        try {
            return response{200, rig_json::encode(holder::api::put_local_model_config(context.db, std::move(*argument_0))), "put_local_model_config"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "put_local_model_config"};
        }
    }
    static const std::regex route_17("^/ai/messages/capture$");
    if (method == "POST" && std::regex_match(route_path, match, route_17)) {
        std::optional<holder::api::AiMessageCaptureRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiMessageCaptureRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_messages_capture");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_messages_capture");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_ai_messages_capture(context.db, context.fts, context.uuid_v4, std::move(*argument_0))), "post_ai_messages_capture"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_messages_capture"};
        } catch (const holder::api::AiCaptureNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_ai_messages_capture"};
        } catch (const holder::model::ConflictError& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_ai_messages_capture"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_messages_capture"};
        }
    }
    static const std::regex route_20("^/ai/nudges/evaluate$");
    if (method == "POST" && std::regex_match(route_path, match, route_20)) {
        std::optional<holder::api::AiNudgeCandidate> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiNudgeCandidate>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_nudges_evaluate");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_nudges_evaluate");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_ai_nudges_evaluate(context.db, context.registry, std::move(*argument_0))), "post_ai_nudges_evaluate"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_nudges_evaluate"};
        }
    }
    static const std::regex route_21("^/ai/providers/catalog$");
    if (method == "GET" && std::regex_match(route_path, match, route_21)) {
        try {
            return response{200, rig_json::encode(holder::api::get_ai_provider_catalog(context.db)), "get_ai_provider_catalog"};
        } catch (const holder::api::AiProviderCatalogMissing& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_ai_provider_catalog"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_provider_catalog"};
        }
    }
    static const std::regex route_44("^/ai/providers/settings$");
    if (method == "GET" && std::regex_match(route_path, match, route_44)) {
        try {
            return response{200, rig_json::encode(holder::api::list_provider_settings(context.db)), "list_provider_settings"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_provider_settings"};
        }
    }
    static const std::regex route_45("^/ai/providers/settings$");
    if (method == "PUT" && std::regex_match(route_path, match, route_45)) {
        std::optional<holder::api::AiProviderSettingUpdateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiProviderSettingUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "set_provider_setting");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "set_provider_setting");
        }
        try {
            return response{200, rig_json::encode(holder::api::set_provider_setting(context.db, std::move(*argument_0))), "set_provider_setting"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "set_provider_setting"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "set_provider_setting"};
        }
    }
    static const std::regex route_0("^/ai/runner/pull/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_0)) {
        std::optional<std::optional<std::string>> argument_0;
        std::optional<std::string> argument_1;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("runner_id");
            if (query_0 == query_values.end())
                argument_0.emplace(std::nullopt);
            else
                argument_0.emplace(std::string(query_0->second));
            argument_1.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_pull_status");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_pull_status");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_pull_status(context.registry, std::move(*argument_0), std::move(*argument_1))), "get_pull_status"};
        } catch (const holder::api::RunnerNotConfigured& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_pull_status"};
        } catch (const holder::api::PullJobNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_pull_status"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_pull_status"};
        }
    }
    static const std::regex route_4("^/ai/providers/credentials/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_4)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_provider_credential");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_provider_credential");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_provider_credential(context.db, context.secret_store, std::move(*argument_0))), "delete_provider_credential"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_provider_credential"};
        } catch (const holder::api::SecretStoreUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "delete_provider_credential"};
        } catch (const holder::api::ProviderCredentialSecretFailure& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "delete_provider_credential"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_provider_credential"};
        }
    }
    static const std::regex route_46("^/ai/providers/settings/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_46)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_provider_setting");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_provider_setting");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_provider_setting(context.db, std::move(*argument_0))), "delete_provider_setting"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_provider_setting"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_provider_setting"};
        }
    }
    static const std::regex route_62("^/ai/runner/pull/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_62)) {
        std::optional<std::optional<std::string>> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        std::optional<std::string> argument_2;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("runner_id");
            if (query_0 == query_values.end())
                argument_0.emplace(std::nullopt);
            else
                argument_0.emplace(std::string(query_0->second));
            auto header_1 = header ? header("Last-Event-ID") : std::nullopt;
            argument_1.emplace(std::move(header_1));
            argument_2.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_runner_pull_job_id_events");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_runner_pull_job_id_events");
        }
        try {
            auto outcome = holder::api::get_ai_runner_pull_job_id_events(context.registry, context.events, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2));
            if (outcome.status != 200 && outcome.status != 204)
                throw std::logic_error("get_ai_runner_pull_job_id_events chose an undeclared success status");
            return response{outcome.status, json(), "get_ai_runner_pull_job_id_events", {}, std::move(outcome.value)};
        } catch (const holder::api::RunnerNotConfigured& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_ai_runner_pull_job_id_events"};
        } catch (const holder::api::PullJobNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_ai_runner_pull_job_id_events"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_ai_runner_pull_job_id_events"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_runner_pull_job_id_events"};
        }
    }
    static const std::regex route_8("^/ai/messages/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_8)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_message");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_message");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_message(context.db, context.fts, std::move(*argument_0))), "get_ai_message"};
        } catch (const holder::api::AiMessageNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_ai_message"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_message"};
        }
    }
    static const std::regex route_12("^/ai/messages/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_12)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::AiMessageUpdateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::AiMessageUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "patch_ai_messages_message_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "patch_ai_messages_message_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::patch_ai_messages_message_id(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "patch_ai_messages_message_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_ai_messages_message_id"};
        } catch (const holder::api::AiMessageNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "patch_ai_messages_message_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "patch_ai_messages_message_id"};
        }
    }
    static const std::regex route_13("^/ai/messages/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_13)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_ai_messages_message_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_ai_messages_message_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_ai_messages_message_id(context.db, context.fts, std::move(*argument_0))), "delete_ai_messages_message_id"};
        } catch (const holder::api::AiMessageWrongState& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_ai_messages_message_id"};
        } catch (const holder::api::AiMessageNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_ai_messages_message_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_ai_messages_message_id"};
        }
    }
    static const std::regex route_23("^/ai/runners/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_23)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_runner");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_runner");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_runner(context.registry, std::move(*argument_0))), "get_ai_runner"};
        } catch (const holder::api::AiRunnerNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_ai_runner"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_runner"};
        }
    }
    static const std::regex route_25("^/ai/runners/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_25)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::AiRunnerUpdateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::AiRunnerUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "patch_ai_runners_runner_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "patch_ai_runners_runner_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::patch_ai_runners_runner_id(context.db, context.registry, std::move(*argument_0), std::move(*argument_1))), "patch_ai_runners_runner_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_ai_runners_runner_id"};
        } catch (const holder::api::AiRunnerNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "patch_ai_runners_runner_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "patch_ai_runners_runner_id"};
        }
    }
    static const std::regex route_26("^/ai/runners/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_26)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_ai_runners_runner_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_ai_runners_runner_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_ai_runners_runner_id(context.db, context.registry, std::move(*argument_0))), "delete_ai_runners_runner_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_ai_runners_runner_id"};
        } catch (const holder::api::AiRunnerNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_ai_runners_runner_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_ai_runners_runner_id"};
        }
    }
    static const std::regex route_29("^/ai/runs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_29)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_run");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_run");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_run(context.db, std::move(*argument_0))), "get_ai_run"};
        } catch (const holder::api::AiRunNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_ai_run"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_run"};
        }
    }
    static const std::regex route_33("^/ai/threads/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_33)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_thread");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_thread");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_thread(context.db, std::move(*argument_0))), "get_ai_thread"};
        } catch (const holder::api::AiThreadNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_ai_thread"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_thread"};
        }
    }
    static const std::regex route_35("^/ai/threads/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_35)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::AiThreadUpdateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::AiThreadUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "patch_ai_threads_thread_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "patch_ai_threads_thread_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::patch_ai_threads_thread_id(context.db, std::move(*argument_0), std::move(*argument_1))), "patch_ai_threads_thread_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_ai_threads_thread_id"};
        } catch (const holder::api::AiThreadNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "patch_ai_threads_thread_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "patch_ai_threads_thread_id"};
        }
    }
    static const std::regex route_36("^/ai/threads/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_36)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_ai_threads_thread_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_ai_threads_thread_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_ai_threads_thread_id(context.db, std::move(*argument_0))), "delete_ai_threads_thread_id"};
        } catch (const holder::api::AiThreadNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_ai_threads_thread_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_ai_threads_thread_id"};
        }
    }
    static const std::regex route_9("^/ai/messages/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, match, route_9)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<int>> argument_1;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            const auto query_1 = query_values.find("include_deleted");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(decode_query_int(query_1->second));
            enforce_int_values(*argument_1, {0, 1}, "include_deleted");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_message_links");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_message_links");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_message_links(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "get_ai_message_links"};
        } catch (const holder::api::AiMessageNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_ai_message_links"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_message_links"};
        }
    }
    static const std::regex route_10("^/ai/messages/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, match, route_10)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<int>> argument_1;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            const auto query_1 = query_values.find("include_deleted");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(decode_query_int(query_1->second));
            enforce_int_values(*argument_1, {0, 1}, "include_deleted");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_message_backlinks");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_message_backlinks");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_ai_message_backlinks(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "get_ai_message_backlinks"};
        } catch (const holder::api::AiMessageNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_ai_message_backlinks"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_message_backlinks"};
        }
    }
    static const std::regex route_14("^/ai/messages/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, match, route_14)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_messages_message_id_restore");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_messages_message_id_restore");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_ai_messages_message_id_restore(context.db, context.fts, std::move(*argument_0))), "post_ai_messages_message_id_restore"};
        } catch (const holder::api::AiMessageWrongState& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_messages_message_id_restore"};
        } catch (const holder::api::AiMessageNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_ai_messages_message_id_restore"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_messages_message_id_restore"};
        }
    }
    static const std::regex route_15("^/ai/messages/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, match, route_15)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::routes::CardLinkCreateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::routes::CardLinkCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_messages_message_id_links");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_messages_message_id_links");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_ai_messages_message_id_links(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "post_ai_messages_message_id_links"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_messages_message_id_links"};
        } catch (const holder::api::AiMessageNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_ai_messages_message_id_links"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_messages_message_id_links"};
        }
    }
    static const std::regex route_16("^/ai/messages/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_16)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<holder::api::CardLinkDeleteRequest>> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(body.empty() ? std::optional<holder::api::CardLinkDeleteRequest>{} : std::optional<holder::api::CardLinkDeleteRequest>(decode_body<holder::api::CardLinkDeleteRequest>(body)));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_ai_messages_message_id_links");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_ai_messages_message_id_links");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_ai_messages_message_id_links(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "delete_ai_messages_message_id_links"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_ai_messages_message_id_links"};
        } catch (const holder::api::AiMessageNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_ai_messages_message_id_links"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_ai_messages_message_id_links"};
        }
    }
    static const std::regex route_19("^/ai/nudges/([^/]+)/dismiss$");
    if (method == "POST" && std::regex_match(route_path, match, route_19)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_nudges_nudge_id_dismiss");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_nudges_nudge_id_dismiss");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_ai_nudges_nudge_id_dismiss(context.db, context.registry, std::move(*argument_0))), "post_ai_nudges_nudge_id_dismiss"};
        } catch (const holder::api::AiNudgeNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_ai_nudges_nudge_id_dismiss"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_nudges_nudge_id_dismiss"};
        }
    }
    static const std::regex route_27("^/ai/runners/([^/]+)/retry$");
    if (method == "POST" && std::regex_match(route_path, match, route_27)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_runners_runner_id_retry");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_runners_runner_id_retry");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_ai_runners_runner_id_retry(context.registry, std::move(*argument_0))), "post_ai_runners_runner_id_retry"};
        } catch (const holder::api::AiRunnerNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_ai_runners_runner_id_retry"};
        } catch (const holder::api::AiRunnerNotConfigured& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_ai_runners_runner_id_retry"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_runners_runner_id_retry"};
        }
    }
    static const std::regex route_61("^/ai/runs/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_61)) {
        std::optional<std::optional<std::string>> argument_0;
        std::optional<std::string> argument_1;
        try {
            auto header_0 = header ? header("Last-Event-ID") : std::nullopt;
            argument_0.emplace(std::move(header_0));
            argument_1.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_ai_runs_run_id_events");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_ai_runs_run_id_events");
        }
        try {
            auto outcome = holder::api::get_ai_runs_run_id_events(context.db, context.events, std::move(*argument_0), std::move(*argument_1));
            if (outcome.status != 200 && outcome.status != 204)
                throw std::logic_error("get_ai_runs_run_id_events chose an undeclared success status");
            return response{outcome.status, json(), "get_ai_runs_run_id_events", {}, std::move(outcome.value)};
        } catch (const holder::api::AiRunNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_ai_runs_run_id_events"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_ai_runs_run_id_events"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_runs_run_id_events"};
        }
    }
    static const std::regex route_55("^/docs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_55)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_docs_file");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_docs_file");
        }
        try {
            auto outcome = holder::api::get_docs_file(std::move(*argument_0));
            if (outcome.status != 200)
                throw std::logic_error("get_docs_file chose an undeclared success status");
            return response{outcome.status, json(), "get_docs_file", {}, std::move(outcome.value)};
        } catch (const holder::api::StaticFileNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_docs_file"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_docs_file"};
        }
    }
    static const std::regex route_56("^/locations/([^/]+)/binding$");
    if (method == "PUT" && std::regex_match(route_path, match, route_56)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::LocationBindingRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::LocationBindingRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "put_locations_location_id_binding");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "put_locations_location_id_binding");
        }
        try {
            return response{200, rig_json::encode(holder::api::put_locations_location_id_binding(context.db, context.secret_store, std::move(*argument_0), std::move(*argument_1))), "put_locations_location_id_binding"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "put_locations_location_id_binding"};
        } catch (const holder::api::LocationNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "put_locations_location_id_binding"};
        } catch (const holder::api::SecretStoreUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "put_locations_location_id_binding"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "put_locations_location_id_binding"};
        }
    }
    static const std::regex route_57("^/locations/([^/]+)/binding$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_57)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_locations_location_id_binding");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_locations_location_id_binding");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_locations_location_id_binding(context.db, context.secret_store, std::move(*argument_0))), "delete_locations_location_id_binding"};
        } catch (const holder::api::LocationNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_locations_location_id_binding"};
        } catch (const holder::api::SecretStoreUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "delete_locations_location_id_binding"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_locations_location_id_binding"};
        }
    }
    static const std::regex route_58("^/locations/([^/]+)/test$");
    if (method == "POST" && std::regex_match(route_path, match, route_58)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_locations_location_id_test");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_locations_location_id_test");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_locations_location_id_test(context.db, context.secret_store, context.uuid_v4, std::move(*argument_0))), "post_locations_location_id_test"};
        } catch (const holder::api::support::StorageBadRequest& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_locations_location_id_test"};
        } catch (const holder::api::LocationNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_locations_location_id_test"};
        } catch (const holder::api::support::StorageBindingRequired& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_locations_location_id_test"};
        } catch (const holder::api::support::StorageConflict& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_locations_location_id_test"};
        } catch (const holder::api::support::StorageIntegrityFailed& error) {
            return response{422, encode_error<holder::api::ErrorResponse>(422, error), "post_locations_location_id_test"};
        } catch (const holder::api::support::StorageGatewayFailure& error) {
            return response{502, encode_error<holder::api::ErrorResponse>(502, error), "post_locations_location_id_test"};
        } catch (const holder::api::support::StorageUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_locations_location_id_test"};
        } catch (const holder::api::SecretStoreUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_locations_location_id_test"};
        } catch (const holder::api::support::StorageCapacityExceeded& error) {
            return response{507, encode_error<holder::api::ErrorResponse>(507, error), "post_locations_location_id_test"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_locations_location_id_test"};
        }
    }
    static const std::regex route_40("^/locations/([^/]+)/oauth/google\\-drive/authorize$");
    if (method == "POST" && std::regex_match(route_path, match, route_40)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_locations_location_id_oauth_google_drive_authorize");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_locations_location_id_oauth_google_drive_authorize");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_locations_location_id_oauth_google_drive_authorize(context.db, context.host, std::move(*argument_0))), "post_locations_location_id_oauth_google_drive_authorize"};
        } catch (const std::exception& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_locations_location_id_oauth_google_drive_authorize"};
        }
    }
    static const std::regex route_41("^/locations/([^/]+)/oauth/google\\-drive/callback$");
    if (method == "GET" && std::regex_match(route_path, match, route_41)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        std::optional<std::optional<std::string>> argument_2;
        std::optional<std::optional<std::string>> argument_3;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            const auto query_1 = query_values.find("code");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(std::string(query_1->second));
            const auto query_2 = query_values.find("state");
            if (query_2 == query_values.end())
                argument_2.emplace(std::nullopt);
            else
                argument_2.emplace(std::string(query_2->second));
            const auto query_3 = query_values.find("error");
            if (query_3 == query_values.end())
                argument_3.emplace(std::nullopt);
            else
                argument_3.emplace(std::string(query_3->second));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_locations_location_id_oauth_google_drive_callback");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_locations_location_id_oauth_google_drive_callback");
        }
        try {
            auto outcome = holder::api::get_locations_location_id_oauth_google_drive_callback(context.db, context.secret_store, context.git_ops, context.host, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3));
            if (outcome.status != 200)
                throw std::logic_error("get_locations_location_id_oauth_google_drive_callback chose an undeclared success status");
            return response{outcome.status, json(), "get_locations_location_id_oauth_google_drive_callback", {}, std::move(outcome.value)};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_locations_location_id_oauth_google_drive_callback"};
        }
    }
    std::set<std::string_view> allowed;
    static const std::regex path_0("^/alive$");
    if (std::regex_match(route_path, path_0)) allowed.insert({"GET"});
    static const std::regex path_1("^/bye$");
    if (std::regex_match(route_path, path_1)) allowed.insert({"POST"});
    static const std::regex path_2("^/health$");
    if (std::regex_match(route_path, path_2)) allowed.insert({"GET"});
    static const std::regex path_3("^/locations$");
    if (std::regex_match(route_path, path_3)) allowed.insert({"GET"});
    static const std::regex path_4("^/ping$");
    if (std::regex_match(route_path, path_4)) allowed.insert({"GET"});
    static const std::regex path_5("^/openapi\\.yaml$");
    if (std::regex_match(route_path, path_5)) allowed.insert({"GET"});
    static const std::regex path_6("^/openapi\\.json$");
    if (std::regex_match(route_path, path_6)) allowed.insert({"GET"});
    static const std::regex path_7("^/ai_catalog\\.yaml$");
    if (std::regex_match(route_path, path_7)) allowed.insert({"GET"});
    static const std::regex path_8("^/git_providers\\.yaml$");
    if (std::regex_match(route_path, path_8)) allowed.insert({"GET"});
    static const std::regex path_9("^/ai_catalog\\.json$");
    if (std::regex_match(route_path, path_9)) allowed.insert({"GET"});
    static const std::regex path_10("^/git_providers\\.json$");
    if (std::regex_match(route_path, path_10)) allowed.insert({"GET"});
    static const std::regex path_11("^/docs$");
    if (std::regex_match(route_path, path_11)) allowed.insert({"GET"});
    static const std::regex path_12("^/events$");
    if (std::regex_match(route_path, path_12)) allowed.insert({"GET"});
    static const std::regex path_13("^/ai/messages$");
    if (std::regex_match(route_path, path_13)) allowed.insert({"GET", "POST"});
    static const std::regex path_14("^/ai/nudges$");
    if (std::regex_match(route_path, path_14)) allowed.insert({"GET"});
    static const std::regex path_15("^/ai/runners$");
    if (std::regex_match(route_path, path_15)) allowed.insert({"GET", "POST"});
    static const std::regex path_16("^/ai/runs$");
    if (std::regex_match(route_path, path_16)) allowed.insert({"GET", "POST"});
    static const std::regex path_17("^/ai/capabilities$");
    if (std::regex_match(route_path, path_17)) allowed.insert({"GET"});
    static const std::regex path_18("^/ai/status$");
    if (std::regex_match(route_path, path_18)) allowed.insert({"GET"});
    static const std::regex path_19("^/ai/threads$");
    if (std::regex_match(route_path, path_19)) allowed.insert({"GET", "POST"});
    static const std::regex path_20("^/events/cursor$");
    if (std::regex_match(route_path, path_20)) allowed.insert({"GET"});
    static const std::regex path_21("^/locations/preferred$");
    if (std::regex_match(route_path, path_21)) allowed.insert({"PUT"});
    static const std::regex path_22("^/ai/runner/pull$");
    if (std::regex_match(route_path, path_22)) allowed.insert({"POST"});
    static const std::regex path_23("^/ai/providers/credentials$");
    if (std::regex_match(route_path, path_23)) allowed.insert({"GET", "PUT"});
    static const std::regex path_24("^/ai/local\\-models/config$");
    if (std::regex_match(route_path, path_24)) allowed.insert({"GET", "PUT"});
    static const std::regex path_25("^/ai/messages/capture$");
    if (std::regex_match(route_path, path_25)) allowed.insert({"POST"});
    static const std::regex path_26("^/ai/nudges/evaluate$");
    if (std::regex_match(route_path, path_26)) allowed.insert({"POST"});
    static const std::regex path_27("^/ai/providers/catalog$");
    if (std::regex_match(route_path, path_27)) allowed.insert({"GET"});
    static const std::regex path_28("^/ai/providers/settings$");
    if (std::regex_match(route_path, path_28)) allowed.insert({"GET", "PUT"});
    static const std::regex path_29("^/ai/runner/pull/([^/]+)$");
    if (std::regex_match(route_path, path_29)) allowed.insert({"GET"});
    static const std::regex path_30("^/ai/providers/credentials/([^/]+)$");
    if (std::regex_match(route_path, path_30)) allowed.insert({"DELETE"});
    static const std::regex path_31("^/ai/providers/settings/([^/]+)$");
    if (std::regex_match(route_path, path_31)) allowed.insert({"DELETE"});
    static const std::regex path_32("^/ai/runner/pull/([^/]+)/events$");
    if (std::regex_match(route_path, path_32)) allowed.insert({"GET"});
    static const std::regex path_33("^/ai/messages/([^/]+)$");
    if (std::regex_match(route_path, path_33)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_34("^/ai/runners/([^/]+)$");
    if (std::regex_match(route_path, path_34)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_35("^/ai/runs/([^/]+)$");
    if (std::regex_match(route_path, path_35)) allowed.insert({"GET"});
    static const std::regex path_36("^/ai/threads/([^/]+)$");
    if (std::regex_match(route_path, path_36)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_37("^/ai/messages/([^/]+)/links$");
    if (std::regex_match(route_path, path_37)) allowed.insert({"DELETE", "GET", "POST"});
    static const std::regex path_38("^/ai/messages/([^/]+)/backlinks$");
    if (std::regex_match(route_path, path_38)) allowed.insert({"GET"});
    static const std::regex path_39("^/ai/messages/([^/]+)/restore$");
    if (std::regex_match(route_path, path_39)) allowed.insert({"POST"});
    static const std::regex path_40("^/ai/nudges/([^/]+)/dismiss$");
    if (std::regex_match(route_path, path_40)) allowed.insert({"POST"});
    static const std::regex path_41("^/ai/runners/([^/]+)/retry$");
    if (std::regex_match(route_path, path_41)) allowed.insert({"POST"});
    static const std::regex path_42("^/ai/runs/([^/]+)/events$");
    if (std::regex_match(route_path, path_42)) allowed.insert({"GET"});
    static const std::regex path_43("^/docs/([^/]+)$");
    if (std::regex_match(route_path, path_43)) allowed.insert({"GET"});
    static const std::regex path_44("^/locations/([^/]+)/binding$");
    if (std::regex_match(route_path, path_44)) allowed.insert({"DELETE", "PUT"});
    static const std::regex path_45("^/locations/([^/]+)/test$");
    if (std::regex_match(route_path, path_45)) allowed.insert({"POST"});
    static const std::regex path_46("^/locations/([^/]+)/oauth/google\\-drive/authorize$");
    if (std::regex_match(route_path, path_46)) allowed.insert({"POST"});
    static const std::regex path_47("^/locations/([^/]+)/oauth/google\\-drive/callback$");
    if (std::regex_match(route_path, path_47)) allowed.insert({"GET"});
    if (!allowed.empty()) {
        std::string allow;
        for (const auto allowed_method : allowed) {
            if (!allow.empty()) allow += ", ";
            allow += allowed_method;
        }
        const method_not_allowed error("method not allowed");
        return response{405, encode_error<holder::api::ErrorResponse>(405, error), "", std::move(allow)};
    }
    return std::nullopt;
}
// Whether the operation dispatch would select for this request is RIG_PUBLIC,
// so the hosting server can skip authentication for it.
inline bool is_public(const std::string& method, const std::string& path) {
    const auto route_path = path.substr(0, path.find('?'));
    static const std::regex route_37("^/alive$");
    if (method == "GET" && std::regex_match(route_path, route_37)) return true;
    static const std::regex route_38("^/bye$");
    if (method == "POST" && std::regex_match(route_path, route_38)) return false;
    static const std::regex route_42("^/health$");
    if (method == "GET" && std::regex_match(route_path, route_42)) return false;
    static const std::regex route_43("^/locations$");
    if (method == "GET" && std::regex_match(route_path, route_43)) return false;
    static const std::regex route_47("^/ping$");
    if (method == "GET" && std::regex_match(route_path, route_47)) return true;
    static const std::regex route_48("^/openapi\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_48)) return true;
    static const std::regex route_49("^/openapi\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_49)) return true;
    static const std::regex route_50("^/ai_catalog\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_50)) return true;
    static const std::regex route_51("^/git_providers\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_51)) return true;
    static const std::regex route_52("^/ai_catalog\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_52)) return true;
    static const std::regex route_53("^/git_providers\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_53)) return true;
    static const std::regex route_54("^/docs$");
    if (method == "GET" && std::regex_match(route_path, route_54)) return true;
    static const std::regex route_63("^/events$");
    if (method == "GET" && std::regex_match(route_path, route_63)) return false;
    static const std::regex route_7("^/ai/messages$");
    if (method == "GET" && std::regex_match(route_path, route_7)) return false;
    static const std::regex route_11("^/ai/messages$");
    if (method == "POST" && std::regex_match(route_path, route_11)) return false;
    static const std::regex route_18("^/ai/nudges$");
    if (method == "GET" && std::regex_match(route_path, route_18)) return false;
    static const std::regex route_22("^/ai/runners$");
    if (method == "GET" && std::regex_match(route_path, route_22)) return false;
    static const std::regex route_24("^/ai/runners$");
    if (method == "POST" && std::regex_match(route_path, route_24)) return false;
    static const std::regex route_28("^/ai/runs$");
    if (method == "GET" && std::regex_match(route_path, route_28)) return false;
    static const std::regex route_30("^/ai/capabilities$");
    if (method == "GET" && std::regex_match(route_path, route_30)) return false;
    static const std::regex route_31("^/ai/status$");
    if (method == "GET" && std::regex_match(route_path, route_31)) return false;
    static const std::regex route_32("^/ai/threads$");
    if (method == "GET" && std::regex_match(route_path, route_32)) return false;
    static const std::regex route_34("^/ai/threads$");
    if (method == "POST" && std::regex_match(route_path, route_34)) return false;
    static const std::regex route_39("^/events/cursor$");
    if (method == "GET" && std::regex_match(route_path, route_39)) return false;
    static const std::regex route_59("^/locations/preferred$");
    if (method == "PUT" && std::regex_match(route_path, route_59)) return false;
    static const std::regex route_60("^/ai/runs$");
    if (method == "POST" && std::regex_match(route_path, route_60)) return false;
    static const std::regex route_1("^/ai/runner/pull$");
    if (method == "POST" && std::regex_match(route_path, route_1)) return false;
    static const std::regex route_2("^/ai/providers/credentials$");
    if (method == "GET" && std::regex_match(route_path, route_2)) return false;
    static const std::regex route_3("^/ai/providers/credentials$");
    if (method == "PUT" && std::regex_match(route_path, route_3)) return false;
    static const std::regex route_5("^/ai/local\\-models/config$");
    if (method == "GET" && std::regex_match(route_path, route_5)) return false;
    static const std::regex route_6("^/ai/local\\-models/config$");
    if (method == "PUT" && std::regex_match(route_path, route_6)) return false;
    static const std::regex route_17("^/ai/messages/capture$");
    if (method == "POST" && std::regex_match(route_path, route_17)) return false;
    static const std::regex route_20("^/ai/nudges/evaluate$");
    if (method == "POST" && std::regex_match(route_path, route_20)) return false;
    static const std::regex route_21("^/ai/providers/catalog$");
    if (method == "GET" && std::regex_match(route_path, route_21)) return false;
    static const std::regex route_44("^/ai/providers/settings$");
    if (method == "GET" && std::regex_match(route_path, route_44)) return false;
    static const std::regex route_45("^/ai/providers/settings$");
    if (method == "PUT" && std::regex_match(route_path, route_45)) return false;
    static const std::regex route_0("^/ai/runner/pull/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_0)) return false;
    static const std::regex route_4("^/ai/providers/credentials/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_4)) return false;
    static const std::regex route_46("^/ai/providers/settings/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_46)) return false;
    static const std::regex route_62("^/ai/runner/pull/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, route_62)) return false;
    static const std::regex route_8("^/ai/messages/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_8)) return false;
    static const std::regex route_12("^/ai/messages/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_12)) return false;
    static const std::regex route_13("^/ai/messages/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_13)) return false;
    static const std::regex route_23("^/ai/runners/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_23)) return false;
    static const std::regex route_25("^/ai/runners/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_25)) return false;
    static const std::regex route_26("^/ai/runners/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_26)) return false;
    static const std::regex route_29("^/ai/runs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_29)) return false;
    static const std::regex route_33("^/ai/threads/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_33)) return false;
    static const std::regex route_35("^/ai/threads/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_35)) return false;
    static const std::regex route_36("^/ai/threads/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_36)) return false;
    static const std::regex route_9("^/ai/messages/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, route_9)) return false;
    static const std::regex route_10("^/ai/messages/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, route_10)) return false;
    static const std::regex route_14("^/ai/messages/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, route_14)) return false;
    static const std::regex route_15("^/ai/messages/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, route_15)) return false;
    static const std::regex route_16("^/ai/messages/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, route_16)) return false;
    static const std::regex route_19("^/ai/nudges/([^/]+)/dismiss$");
    if (method == "POST" && std::regex_match(route_path, route_19)) return false;
    static const std::regex route_27("^/ai/runners/([^/]+)/retry$");
    if (method == "POST" && std::regex_match(route_path, route_27)) return false;
    static const std::regex route_61("^/ai/runs/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, route_61)) return false;
    static const std::regex route_55("^/docs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_55)) return true;
    static const std::regex route_56("^/locations/([^/]+)/binding$");
    if (method == "PUT" && std::regex_match(route_path, route_56)) return false;
    static const std::regex route_57("^/locations/([^/]+)/binding$");
    if (method == "DELETE" && std::regex_match(route_path, route_57)) return false;
    static const std::regex route_58("^/locations/([^/]+)/test$");
    if (method == "POST" && std::regex_match(route_path, route_58)) return false;
    static const std::regex route_40("^/locations/([^/]+)/oauth/google\\-drive/authorize$");
    if (method == "POST" && std::regex_match(route_path, route_40)) return false;
    static const std::regex route_41("^/locations/([^/]+)/oauth/google\\-drive/callback$");
    if (method == "GET" && std::regex_match(route_path, route_41)) return true;
    return false;
}
// Whether the operation dispatch would select for this request is a RIG_HOST_RESPONSE
// operation, so the hosting server serves only the routes the contract declares.
inline bool is_host_served(const std::string& method, const std::string& path) {
    (void)method;
    (void)path;
    return false;
}
} // namespace rig_http
