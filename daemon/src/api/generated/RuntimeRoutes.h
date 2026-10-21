// Generated from RIG IR; do not edit.
#pragma once
#include "api/generated/RuntimeJson.h"
#include <charconv>
#include <exception>
#include <initializer_list>
#include <limits>
#include <map>
#include <optional>
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
struct response { int status; json body; std::string_view operation; };
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
template <typename Context>
inline std::optional<response> dispatch(const std::string& method,
                                    const std::string& path,
                                    const std::string& body, Context& context) {
    const auto query_start = path.find('?');
    const auto route_path = path.substr(0, query_start);
    const auto query_string = query_start == std::string::npos ? std::string_view{} :
        std::string_view(path).substr(query_start + 1);
    std::smatch match;
    (void)body;
    static const std::regex route_6("^/alive$");
    if (method == "GET" && std::regex_match(route_path, match, route_6)) {
        try {
            return response{200, rig_json::encode(holder::api::check_alive()), "check_alive"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "check_alive"};
        }
    }
    static const std::regex route_7("^/bye$");
    if (method == "POST" && std::regex_match(route_path, match, route_7)) {
        try {
            return response{200, rig_json::encode(holder::api::say_goodbye()), "say_goodbye"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "say_goodbye"};
        }
    }
    static const std::regex route_9("^/health$");
    if (method == "GET" && std::regex_match(route_path, match, route_9)) {
        try {
            return response{200, rig_json::encode(holder::api::get_health(context.db, context.started_at)), "get_health"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_health"};
        }
    }
    static const std::regex route_14("^/rebuild$");
    if (method == "POST" && std::regex_match(route_path, match, route_14)) {
        std::optional<holder::api::RebuildRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::RebuildRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "rebuild_project_from_files");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "rebuild_project_from_files");
        }
        try {
            return response{200, rig_json::encode(holder::api::rebuild_project_from_files(context.db, context.fts, std::move(*argument_0))), "rebuild_project_from_files"};
        } catch (const holder::api::RebuildFailed& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "rebuild_project_from_files"};
        } catch (const holder::api::RebuildProjectNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "rebuild_project_from_files"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "rebuild_project_from_files"};
        }
    }
    static const std::regex route_15("^/projects$");
    if (method == "GET" && std::regex_match(route_path, match, route_15)) {
        std::optional<std::optional<std::string>> argument_0;
        std::optional<std::optional<long long>> argument_1;
        std::optional<std::optional<long long>> argument_2;
        std::optional<std::optional<std::string>> argument_3;
        std::optional<std::optional<int>> argument_4;
        std::optional<std::optional<int>> argument_5;
        std::optional<bool> argument_6;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("name");
            if (query_0 == query_values.end())
                argument_0.emplace(std::nullopt);
            else
                argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("updated_after");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(decode_query_int64(query_1->second));
            const auto query_2 = query_values.find("updated_before");
            if (query_2 == query_values.end())
                argument_2.emplace(std::nullopt);
            else
                argument_2.emplace(decode_query_int64(query_2->second));
            const auto query_3 = query_values.find("order");
            if (query_3 == query_values.end())
                argument_3.emplace(std::nullopt);
            else
                argument_3.emplace(std::string(query_3->second));
            const auto query_4 = query_values.find("limit");
            if (query_4 == query_values.end())
                argument_4.emplace(std::nullopt);
            else
                argument_4.emplace(decode_query_int(query_4->second));
            const auto query_5 = query_values.find("offset");
            if (query_5 == query_values.end())
                argument_5.emplace(std::nullopt);
            else
                argument_5.emplace(decode_query_int(query_5->second));
            const auto query_6 = query_values.find("count");
            if (query_6 == query_values.end())
                argument_6.emplace(false);
            else
                argument_6.emplace(decode_query_bool(query_6->second));
            enforce_string_values(*argument_3, {"updated_at_desc", "updated_at_asc", "created_at_desc", "created_at_asc", "name_desc", "name_asc"}, "order");
            enforce_range(*argument_4, 0, std::numeric_limits<int>::max(), "limit");
            enforce_range(*argument_5, 0, std::numeric_limits<int>::max(), "offset");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_projects");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_projects");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_projects(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3), std::move(*argument_4), std::move(*argument_5), std::move(*argument_6))), "get_projects"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_projects"};
        }
    }
    static const std::regex route_19("^/projects$");
    if (method == "POST" && std::regex_match(route_path, match, route_19)) {
        std::optional<holder::api::ProjectCreateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::ProjectCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "create_project");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "create_project");
        }
        try {
            return response{201, rig_json::encode(holder::api::create_project(context.db, context.git_ops, context.uuid_v4, std::move(*argument_0))), "create_project"};
        } catch (const holder::api::ProjectCreateUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "create_project"};
        } catch (const holder::api::ProjectCreateInternal& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "create_project"};
        } catch (const holder::api::ProjectCreateBadRequest& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "create_project"};
        } catch (const std::exception& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "create_project"};
        }
    }
    static const std::regex route_22("^/calendar$");
    if (method == "GET" && std::regex_match(route_path, match, route_22)) {
        std::optional<std::string> argument_0;
        std::optional<long long> argument_1;
        std::optional<long long> argument_2;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("from");
            if (query_1 == query_values.end())
                throw std::invalid_argument("missing query parameter: from");
            argument_1.emplace(decode_query_int64(query_1->second));
            const auto query_2 = query_values.find("to");
            if (query_2 == query_values.end())
                throw std::invalid_argument("missing query parameter: to");
            argument_2.emplace(decode_query_int64(query_2->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_calendar");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_calendar");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_calendar(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2))), "get_calendar"};
        } catch (const holder::api::CalendarInvalidRange& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_calendar"};
        } catch (const holder::api::CalendarProjectNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_calendar"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_calendar"};
        }
    }
    static const std::regex route_25("^/openapi\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_25))
        return std::nullopt;  // download_openapi_yaml is served by the host
    static const std::regex route_26("^/openapi\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_26))
        return std::nullopt;  // download_openapi_json is served by the host
    static const std::regex route_27("^/ai_catalog\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_27))
        return std::nullopt;  // download_ai_catalog_yaml is served by the host
    static const std::regex route_28("^/git_providers\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_28))
        return std::nullopt;  // download_git_providers_yaml is served by the host
    static const std::regex route_29("^/ai_catalog\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_29))
        return std::nullopt;  // download_ai_catalog_json is served by the host
    static const std::regex route_30("^/git_providers\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_30))
        return std::nullopt;  // download_git_providers_json is served by the host
    static const std::regex route_31("^/trash$");
    if (method == "GET" && std::regex_match(route_path, match, route_31)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("type");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(std::string(query_1->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_trash");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_trash");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_trash(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "get_trash"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_trash"};
        }
    }
    static const std::regex route_71("^/cards$");
    if (method == "GET" && std::regex_match(route_path, route_71))
        return std::nullopt;  // get_cards is served by the host
    static const std::regex route_72("^/cards$");
    if (method == "POST" && std::regex_match(route_path, route_72))
        return std::nullopt;  // post_cards is served by the host
    static const std::regex route_85("^/events$");
    if (method == "GET" && std::regex_match(route_path, route_85))
        return std::nullopt;  // get_events is served by the host
    static const std::regex route_86("^/imports$");
    if (method == "POST" && std::regex_match(route_path, route_86))
        return std::nullopt;  // post_imports is served by the host
    static const std::regex route_88("^/locations$");
    if (method == "GET" && std::regex_match(route_path, route_88))
        return std::nullopt;  // get_locations is served by the host
    static const std::regex route_89("^/locations$");
    if (method == "POST" && std::regex_match(route_path, route_89))
        return std::nullopt;  // post_locations is served by the host
    static const std::regex route_97("^/ping$");
    if (method == "GET" && std::regex_match(route_path, route_97))
        return std::nullopt;  // get_ping is served by the host
    static const std::regex route_114("^/reindex$");
    if (method == "POST" && std::regex_match(route_path, route_114))
        return std::nullopt;  // post_reindex is served by the host
    static const std::regex route_115("^/resources$");
    if (method == "GET" && std::regex_match(route_path, route_115))
        return std::nullopt;  // get_resources is served by the host
    static const std::regex route_116("^/resources$");
    if (method == "POST" && std::regex_match(route_path, route_116))
        return std::nullopt;  // post_resources is served by the host
    static const std::regex route_123("^/trash$");
    if (method == "DELETE" && std::regex_match(route_path, route_123))
        return std::nullopt;  // delete_trash is served by the host
    static const std::regex route_4("^/ai/runners$");
    if (method == "GET" && std::regex_match(route_path, match, route_4)) {
        try {
            return response{200, rig_json::encode(holder::api::get_ai_runners(context.registry)), "get_ai_runners"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_runners"};
        }
    }
    static const std::regex route_5("^/ai/threads$");
    if (method == "GET" && std::regex_match(route_path, match, route_5)) {
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
    static const std::regex route_8("^/card\\-references/resolve$");
    if (method == "POST" && std::regex_match(route_path, match, route_8)) {
        std::optional<holder::api::CardReferenceResolveRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::CardReferenceResolveRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "resolve_card_reference");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "resolve_card_reference");
        }
        try {
            return response{200, rig_json::encode(holder::api::resolve_card_reference(context.db, std::move(*argument_0))), "resolve_card_reference"};
        } catch (const holder::api::CardReferenceInvalidRequest& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "resolve_card_reference"};
        } catch (const holder::api::CardReferenceProjectNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "resolve_card_reference"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "resolve_card_reference"};
        }
    }
    static const std::regex route_32("^/ai/capabilities$");
    if (method == "GET" && std::regex_match(route_path, route_32))
        return std::nullopt;  // get_ai_capabilities is served by the host
    static const std::regex route_33("^/ai/status$");
    if (method == "GET" && std::regex_match(route_path, route_33))
        return std::nullopt;  // get_ai_status is served by the host
    static const std::regex route_36("^/ai/runners$");
    if (method == "POST" && std::regex_match(route_path, route_36))
        return std::nullopt;  // post_ai_runners is served by the host
    static const std::regex route_43("^/ai/runs$");
    if (method == "POST" && std::regex_match(route_path, route_43))
        return std::nullopt;  // post_ai_runs is served by the host
    static const std::regex route_44("^/ai/runs$");
    if (method == "GET" && std::regex_match(route_path, route_44))
        return std::nullopt;  // get_ai_runs is served by the host
    static const std::regex route_49("^/ai/messages$");
    if (method == "GET" && std::regex_match(route_path, route_49))
        return std::nullopt;  // get_ai_messages is served by the host
    static const std::regex route_50("^/ai/messages$");
    if (method == "POST" && std::regex_match(route_path, route_50))
        return std::nullopt;  // post_ai_messages is served by the host
    static const std::regex route_52("^/ai/threads$");
    if (method == "POST" && std::regex_match(route_path, route_52))
        return std::nullopt;  // post_ai_threads is served by the host
    static const std::regex route_73("^/cards/context$");
    if (method == "GET" && std::regex_match(route_path, route_73))
        return std::nullopt;  // get_cards_context is served by the host
    static const std::regex route_84("^/events/cursor$");
    if (method == "GET" && std::regex_match(route_path, route_84))
        return std::nullopt;  // get_events_cursor is served by the host
    static const std::regex route_96("^/locations/preferred$");
    if (method == "PUT" && std::regex_match(route_path, route_96))
        return std::nullopt;  // put_locations_preferred is served by the host
    static const std::regex route_113("^/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, route_113))
        return std::nullopt;  // post_recovery_token_import is served by the host
    static const std::regex route_121("^/search/cards$");
    if (method == "GET" && std::regex_match(route_path, route_121))
        return std::nullopt;  // get_search_cards is served by the host
    static const std::regex route_122("^/search/ai$");
    if (method == "GET" && std::regex_match(route_path, route_122))
        return std::nullopt;  // get_search_ai is served by the host
    static const std::regex route_1("^/ai/providers/credentials$");
    if (method == "GET" && std::regex_match(route_path, match, route_1)) {
        try {
            return response{200, rig_json::encode(holder::api::list_provider_credentials(context.db)), "list_provider_credentials"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_provider_credentials"};
        }
    }
    static const std::regex route_2("^/ai/providers/credentials$");
    if (method == "PUT" && std::regex_match(route_path, match, route_2)) {
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
        } catch (const holder::api::ProviderCredentialError& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "upsert_provider_credential"};
        } catch (const holder::privacy::PrivacyError& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "upsert_provider_credential"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "upsert_provider_credential"};
        }
    }
    static const std::regex route_3("^/ai/local\\-models/config$");
    if (method == "GET" && std::regex_match(route_path, match, route_3)) {
        try {
            return response{200, rig_json::encode(holder::api::get_local_model_config(context.db)), "get_local_model_config"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_local_model_config"};
        }
    }
    static const std::regex route_11("^/ai/providers/settings$");
    if (method == "GET" && std::regex_match(route_path, match, route_11)) {
        try {
            return response{200, rig_json::encode(holder::api::list_provider_settings(context.db)), "list_provider_settings"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_provider_settings"};
        }
    }
    static const std::regex route_12("^/ai/providers/settings$");
    if (method == "PUT" && std::regex_match(route_path, match, route_12)) {
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
        } catch (const holder::api::ProviderSettingError& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "set_provider_setting"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "set_provider_setting"};
        }
    }
    static const std::regex route_34("^/ai/providers/catalog$");
    if (method == "GET" && std::regex_match(route_path, route_34))
        return std::nullopt;  // get_ai_providers_catalog is served by the host
    static const std::regex route_41("^/ai/runner/retry$");
    if (method == "POST" && std::regex_match(route_path, route_41))
        return std::nullopt;  // post_ai_runner_retry is served by the host
    static const std::regex route_42("^/ai/local\\-models/config$");
    if (method == "PUT" && std::regex_match(route_path, route_42))
        return std::nullopt;  // put_ai_local_models_config is served by the host
    static const std::regex route_47("^/ai/runner/pull$");
    if (method == "POST" && std::regex_match(route_path, route_47))
        return std::nullopt;  // post_ai_runner_pull is served by the host
    static const std::regex route_51("^/ai/messages/capture$");
    if (method == "POST" && std::regex_match(route_path, route_51))
        return std::nullopt;  // post_ai_messages_capture is served by the host
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
    static const std::regex route_13("^/ai/providers/settings/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_13)) {
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
        } catch (const holder::api::ProviderSettingError& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_provider_setting"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_provider_setting"};
        }
    }
    static const std::regex route_35("^/ai/providers/credentials/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_35))
        return std::nullopt;  // delete_ai_providers_credentials_provider is served by the host
    static const std::regex route_48("^/ai/runner/pull/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, route_48))
        return std::nullopt;  // get_ai_runner_pull_job_id_events is served by the host
    static const std::regex route_37("^/ai/runners/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_37))
        return std::nullopt;  // get_ai_runners_runner_id is served by the host
    static const std::regex route_38("^/ai/runners/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_38))
        return std::nullopt;  // patch_ai_runners_runner_id is served by the host
    static const std::regex route_39("^/ai/runners/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_39))
        return std::nullopt;  // delete_ai_runners_runner_id is served by the host
    static const std::regex route_45("^/ai/runs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_45))
        return std::nullopt;  // get_ai_runs_run_id is served by the host
    static const std::regex route_53("^/ai/threads/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_53))
        return std::nullopt;  // get_ai_threads_thread_id is served by the host
    static const std::regex route_54("^/ai/threads/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_54))
        return std::nullopt;  // patch_ai_threads_thread_id is served by the host
    static const std::regex route_55("^/ai/threads/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_55))
        return std::nullopt;  // delete_ai_threads_thread_id is served by the host
    static const std::regex route_56("^/ai/messages/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_56))
        return std::nullopt;  // get_ai_messages_message_id is served by the host
    static const std::regex route_57("^/ai/messages/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_57))
        return std::nullopt;  // patch_ai_messages_message_id is served by the host
    static const std::regex route_58("^/ai/messages/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_58))
        return std::nullopt;  // delete_ai_messages_message_id is served by the host
    static const std::regex route_40("^/ai/runners/([^/]+)/retry$");
    if (method == "POST" && std::regex_match(route_path, route_40))
        return std::nullopt;  // post_ai_runners_runner_id_retry is served by the host
    static const std::regex route_46("^/ai/runs/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, route_46))
        return std::nullopt;  // get_ai_runs_run_id_events is served by the host
    static const std::regex route_59("^/ai/messages/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, route_59))
        return std::nullopt;  // post_ai_messages_message_id_restore is served by the host
    static const std::regex route_60("^/ai/messages/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, route_60))
        return std::nullopt;  // get_ai_messages_message_id_links is served by the host
    static const std::regex route_61("^/ai/messages/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, route_61))
        return std::nullopt;  // post_ai_messages_message_id_links is served by the host
    static const std::regex route_62("^/ai/messages/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, route_62))
        return std::nullopt;  // delete_ai_messages_message_id_links is served by the host
    static const std::regex route_63("^/ai/messages/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, route_63))
        return std::nullopt;  // get_ai_messages_message_id_backlinks is served by the host
    static const std::regex route_16("^/projects/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_16)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_project");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_project");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_project(context.db, std::move(*argument_0))), "get_project"};
        } catch (const holder::api::ProjectReadNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_project"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_project"};
        }
    }
    static const std::regex route_20("^/projects/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_20)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::ProjectUpdateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::ProjectUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "update_project");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "update_project");
        }
        try {
            return response{200, rig_json::encode(holder::api::update_project(context.db, context.git_ops, context.uuid_v4, std::move(*argument_0), std::move(*argument_1))), "update_project"};
        } catch (const holder::api::ProjectUpdateNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "update_project"};
        } catch (const holder::api::ProjectUpdateUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "update_project"};
        } catch (const holder::api::ProjectUpdateInternal& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "update_project"};
        } catch (const holder::api::ProjectUpdateBadRequest& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "update_project"};
        } catch (const std::exception& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "update_project"};
        }
    }
    static const std::regex route_21("^/projects/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_21)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_project");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_project");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_project(context.db, std::move(*argument_0))), "delete_project"};
        } catch (const holder::api::ProjectDeleteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_project"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_project"};
        }
    }
    static const std::regex route_74("^/cards/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_74))
        return std::nullopt;  // get_cards_card_id is served by the host
    static const std::regex route_75("^/cards/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_75))
        return std::nullopt;  // patch_cards_card_id is served by the host
    static const std::regex route_76("^/cards/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_76))
        return std::nullopt;  // delete_cards_card_id is served by the host
    static const std::regex route_87("^/imports/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_87))
        return std::nullopt;  // get_imports_job_id is served by the host
    static const std::regex route_90("^/locations/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_90))
        return std::nullopt;  // get_locations_location_id is served by the host
    static const std::regex route_91("^/locations/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_91))
        return std::nullopt;  // patch_locations_location_id is served by the host
    static const std::regex route_92("^/locations/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_92))
        return std::nullopt;  // delete_locations_location_id is served by the host
    static const std::regex route_117("^/resources/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_117))
        return std::nullopt;  // get_resources_resource_id is served by the host
    static const std::regex route_118("^/resources/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_118))
        return std::nullopt;  // patch_resources_resource_id is served by the host
    static const std::regex route_119("^/resources/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_119))
        return std::nullopt;  // delete_resources_resource_id is served by the host
    static const std::regex route_10("^/projects/([^/]+)/tags$");
    if (method == "GET" && std::regex_match(route_path, match, route_10)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "list_project_tags");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "list_project_tags");
        }
        try {
            return response{200, rig_json::encode(holder::api::list_project_tags(context.db, std::move(*argument_0))), "list_project_tags"};
        } catch (const holder::api::ProjectNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "list_project_tags"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_project_tags"};
        }
    }
    static const std::regex route_18("^/projects/([^/]+)/encryption\\-check$");
    if (method == "GET" && std::regex_match(route_path, match, route_18)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "check_project_encryption");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "check_project_encryption");
        }
        try {
            return response{200, rig_json::encode(holder::api::check_project_encryption(context.db, std::move(*argument_0))), "check_project_encryption"};
        } catch (const holder::api::ProjectReadNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "check_project_encryption"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "check_project_encryption"};
        }
    }
    static const std::regex route_23("^/cards/([^/]+)/milestones\\-with\\-dates$");
    if (method == "GET" && std::regex_match(route_path, match, route_23)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_card_milestones_with_dates");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_card_milestones_with_dates");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_card_milestones_with_dates(context.db, std::move(*argument_0))), "get_card_milestones_with_dates"};
        } catch (const holder::api::CardMilestonesNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_card_milestones_with_dates"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_card_milestones_with_dates"};
        }
    }
    static const std::regex route_24("^/projects/([^/]+)/milestones\\-with\\-dates$");
    if (method == "GET" && std::regex_match(route_path, match, route_24)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<std::string> argument_2;
        std::optional<std::string> argument_3;
        std::optional<std::string> argument_4;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            const auto query_1 = query_values.find("role");
            if (query_1 == query_values.end())
                throw std::invalid_argument("missing query parameter: role");
            argument_1.emplace(std::string(query_1->second));
            const auto query_2 = query_values.find("value_type");
            if (query_2 == query_values.end())
                throw std::invalid_argument("missing query parameter: value_type");
            argument_2.emplace(std::string(query_2->second));
            const auto query_3 = query_values.find("from");
            if (query_3 == query_values.end())
                throw std::invalid_argument("missing query parameter: from");
            argument_3.emplace(std::string(query_3->second));
            const auto query_4 = query_values.find("to");
            if (query_4 == query_values.end())
                throw std::invalid_argument("missing query parameter: to");
            argument_4.emplace(std::string(query_4->second));
            enforce_length(*argument_1, 1, std::numeric_limits<std::size_t>::max(), "role");
            enforce_string_values(*argument_2, {"date", "datetime"}, "value_type");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_project_milestones_with_dates");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_project_milestones_with_dates");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_project_milestones_with_dates(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3), std::move(*argument_4))), "get_project_milestones_with_dates"};
        } catch (const holder::api::ProjectMilestonesInvalidQuery& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_project_milestones_with_dates"};
        } catch (const holder::api::ProjectMilestonesNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_project_milestones_with_dates"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_project_milestones_with_dates"};
        }
    }
    static const std::regex route_64("^/cards/([^/]+)/milestones\\-with\\-dates$");
    if (method == "POST" && std::regex_match(route_path, route_64))
        return std::nullopt;  // post_cards_card_id_milestones_with_dates is served by the host
    static const std::regex route_67("^/cards/([^/]+)/milestones$");
    if (method == "GET" && std::regex_match(route_path, route_67))
        return std::nullopt;  // get_cards_card_id_milestones is served by the host
    static const std::regex route_68("^/cards/([^/]+)/milestones$");
    if (method == "POST" && std::regex_match(route_path, route_68))
        return std::nullopt;  // post_cards_card_id_milestones is served by the host
    static const std::regex route_77("^/cards/([^/]+)/move$");
    if (method == "POST" && std::regex_match(route_path, route_77))
        return std::nullopt;  // post_cards_card_id_move is served by the host
    static const std::regex route_78("^/cards/([^/]+)/tags$");
    if (method == "POST" && std::regex_match(route_path, route_78))
        return std::nullopt;  // post_cards_card_id_tags is served by the host
    static const std::regex route_79("^/cards/([^/]+)/tags$");
    if (method == "DELETE" && std::regex_match(route_path, route_79))
        return std::nullopt;  // delete_cards_card_id_tags is served by the host
    static const std::regex route_80("^/cards/([^/]+)/resources$");
    if (method == "POST" && std::regex_match(route_path, route_80))
        return std::nullopt;  // post_cards_card_id_resources is served by the host
    static const std::regex route_81("^/cards/([^/]+)/resources$");
    if (method == "DELETE" && std::regex_match(route_path, route_81))
        return std::nullopt;  // delete_cards_card_id_resources is served by the host
    static const std::regex route_82("^/cards/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, route_82))
        return std::nullopt;  // delete_cards_card_id_links is served by the host
    static const std::regex route_83("^/cards/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, route_83))
        return std::nullopt;  // post_cards_card_id_restore is served by the host
    static const std::regex route_93("^/locations/([^/]+)/binding$");
    if (method == "PUT" && std::regex_match(route_path, route_93))
        return std::nullopt;  // put_locations_location_id_binding is served by the host
    static const std::regex route_94("^/locations/([^/]+)/binding$");
    if (method == "DELETE" && std::regex_match(route_path, route_94))
        return std::nullopt;  // delete_locations_location_id_binding is served by the host
    static const std::regex route_95("^/locations/([^/]+)/test$");
    if (method == "POST" && std::regex_match(route_path, route_95))
        return std::nullopt;  // post_locations_location_id_test is served by the host
    static const std::regex route_98("^/projects/([^/]+)/repair\\-hierarchy$");
    if (method == "POST" && std::regex_match(route_path, route_98))
        return std::nullopt;  // post_projects_project_id_repair_hierarchy is served by the host
    static const std::regex route_99("^/projects/([^/]+)/cards$");
    if (method == "GET" && std::regex_match(route_path, route_99))
        return std::nullopt;  // get_projects_project_id_cards is served by the host
    static const std::regex route_101("^/projects/([^/]+)/history$");
    if (method == "GET" && std::regex_match(route_path, route_101))
        return std::nullopt;  // get_projects_project_id_history is served by the host
    static const std::regex route_125("^/cards/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, match, route_125)) {
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
            return invalid_request(error, "list_card_links");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "list_card_links");
        }
        try {
            return response{200, rig_json::encode(holder::api::routes::list_card_links(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "list_card_links"};
        } catch (const holder::api::routes::CardLinkSourceNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "list_card_links"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_card_links"};
        }
    }
    static const std::regex route_126("^/cards/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, match, route_126)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<int>> argument_1;
        std::optional<std::string> argument_2;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            const auto query_1 = query_values.find("include_deleted");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(decode_query_int(query_1->second));
            const auto query_2 = query_values.find("scope");
            if (query_2 == query_values.end())
                argument_2.emplace("local");
            else
                argument_2.emplace(std::string(query_2->second));
            enforce_int_values(*argument_1, {0, 1}, "include_deleted");
            enforce_string_values(*argument_2, {"local", "external", "all_available"}, "scope");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "list_card_backlinks");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "list_card_backlinks");
        }
        try {
            return response{200, rig_json::encode(holder::api::routes::list_card_backlinks(context.db, context.fts, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2))), "list_card_backlinks"};
        } catch (const holder::api::routes::CardLinkSourceNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "list_card_backlinks"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_card_backlinks"};
        }
    }
    static const std::regex route_127("^/cards/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, route_127))
        return std::nullopt;  // create_card_link is served by the host
    static const std::regex route_17("^/projects/([^/]+)/git/sync\\-status$");
    if (method == "GET" && std::regex_match(route_path, match, route_17)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_project_git_sync_status");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_project_git_sync_status");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_project_git_sync_status(context.db, std::move(*argument_0))), "get_project_git_sync_status"};
        } catch (const holder::api::ProjectReadNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_project_git_sync_status"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_project_git_sync_status"};
        }
    }
    static const std::regex route_106("^/projects/([^/]+)/git/test\\-remote$");
    if (method == "POST" && std::regex_match(route_path, route_106))
        return std::nullopt;  // post_projects_project_id_git_test_remote is served by the host
    static const std::regex route_107("^/projects/([^/]+)/git/push$");
    if (method == "POST" && std::regex_match(route_path, route_107))
        return std::nullopt;  // post_projects_project_id_git_push is served by the host
    static const std::regex route_108("^/projects/([^/]+)/git/pull$");
    if (method == "POST" && std::regex_match(route_path, route_108))
        return std::nullopt;  // post_projects_project_id_git_pull is served by the host
    static const std::regex route_109("^/projects/([^/]+)/git/sync$");
    if (method == "POST" && std::regex_match(route_path, route_109))
        return std::nullopt;  // post_projects_project_id_git_sync is served by the host
    static const std::regex route_110("^/projects/([^/]+)/recovery\\-token/export$");
    if (method == "POST" && std::regex_match(route_path, route_110))
        return std::nullopt;  // post_projects_project_id_recovery_token_export is served by the host
    static const std::regex route_111("^/projects/([^/]+)/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, route_111))
        return std::nullopt;  // post_projects_project_id_recovery_token_import is served by the host
    static const std::regex route_102("^/projects/([^/]+)/history/cards/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_102))
        return std::nullopt;  // get_projects_project_id_history_cards_card_id is served by the host
    static const std::regex route_103("^/projects/([^/]+)/history/cards/([^/]+)/compare$");
    if (method == "GET" && std::regex_match(route_path, route_103))
        return std::nullopt;  // get_projects_project_id_history_cards_card_id_compare is served by the host
    static const std::regex route_104("^/projects/([^/]+)/history/cards/([^/]+)/snapshot$");
    if (method == "GET" && std::regex_match(route_path, route_104))
        return std::nullopt;  // get_projects_project_id_history_cards_card_id_snapshot is served by the host
    static const std::regex route_105("^/projects/([^/]+)/history/cards/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, route_105))
        return std::nullopt;  // post_projects_project_id_history_cards_card_id_restore is served by the host
    static const std::regex route_65("^/cards/([^/]+)/milestones\\-with\\-dates/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_65))
        return std::nullopt;  // patch_cards_card_id_milestones_with_dates_milestone_id is served by the host
    static const std::regex route_66("^/cards/([^/]+)/milestones\\-with\\-dates/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_66))
        return std::nullopt;  // delete_cards_card_id_milestones_with_dates_milestone_id is served by the host
    static const std::regex route_69("^/cards/([^/]+)/milestones/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_69))
        return std::nullopt;  // patch_cards_card_id_milestones_milestone_id is served by the host
    static const std::regex route_70("^/cards/([^/]+)/milestones/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_70))
        return std::nullopt;  // delete_cards_card_id_milestones_milestone_id is served by the host
    static const std::regex route_100("^/projects/([^/]+)/cards/([^/]+)/breadcrumbs$");
    if (method == "GET" && std::regex_match(route_path, route_100))
        return std::nullopt;  // get_projects_project_id_cards_card_id_breadcrumbs is served by the host
    static const std::regex route_120("^/resources/([^/]+)/assets/([^/]+)/content$");
    if (method == "GET" && std::regex_match(route_path, route_120))
        return std::nullopt;  // get_resources_resource_id_assets_asset_id_content is served by the host
    static const std::regex route_112("^/projects/([^/]+)/trash/([^/]+)/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_112))
        return std::nullopt;  // delete_projects_project_id_trash_type_id is served by the host
    static const std::regex route_124("^/trash/([^/]+)/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_124))
        return std::nullopt;  // delete_trash_type_id is served by the host
    return std::nullopt;
}
// Whether the operation dispatch would select for this request is RIG_PUBLIC,
// so the hosting server can skip authentication for it.
inline bool is_public(const std::string& method, const std::string& path) {
    const auto route_path = path.substr(0, path.find('?'));
    static const std::regex route_6("^/alive$");
    if (method == "GET" && std::regex_match(route_path, route_6)) return true;
    static const std::regex route_7("^/bye$");
    if (method == "POST" && std::regex_match(route_path, route_7)) return false;
    static const std::regex route_9("^/health$");
    if (method == "GET" && std::regex_match(route_path, route_9)) return false;
    static const std::regex route_14("^/rebuild$");
    if (method == "POST" && std::regex_match(route_path, route_14)) return false;
    static const std::regex route_15("^/projects$");
    if (method == "GET" && std::regex_match(route_path, route_15)) return false;
    static const std::regex route_19("^/projects$");
    if (method == "POST" && std::regex_match(route_path, route_19)) return false;
    static const std::regex route_22("^/calendar$");
    if (method == "GET" && std::regex_match(route_path, route_22)) return false;
    static const std::regex route_25("^/openapi\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_25)) return true;
    static const std::regex route_26("^/openapi\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_26)) return true;
    static const std::regex route_27("^/ai_catalog\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_27)) return true;
    static const std::regex route_28("^/git_providers\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_28)) return true;
    static const std::regex route_29("^/ai_catalog\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_29)) return true;
    static const std::regex route_30("^/git_providers\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_30)) return true;
    static const std::regex route_31("^/trash$");
    if (method == "GET" && std::regex_match(route_path, route_31)) return false;
    static const std::regex route_71("^/cards$");
    if (method == "GET" && std::regex_match(route_path, route_71)) return false;
    static const std::regex route_72("^/cards$");
    if (method == "POST" && std::regex_match(route_path, route_72)) return false;
    static const std::regex route_85("^/events$");
    if (method == "GET" && std::regex_match(route_path, route_85)) return false;
    static const std::regex route_86("^/imports$");
    if (method == "POST" && std::regex_match(route_path, route_86)) return false;
    static const std::regex route_88("^/locations$");
    if (method == "GET" && std::regex_match(route_path, route_88)) return false;
    static const std::regex route_89("^/locations$");
    if (method == "POST" && std::regex_match(route_path, route_89)) return false;
    static const std::regex route_97("^/ping$");
    if (method == "GET" && std::regex_match(route_path, route_97)) return true;
    return false;
}
} // namespace rig_http
