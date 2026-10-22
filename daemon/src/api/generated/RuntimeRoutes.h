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
    static const std::regex route_5("^/cards$");
    if (method == "POST" && std::regex_match(route_path, match, route_5)) {
        std::optional<holder::api::CardCreateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::CardCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_cards");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_cards");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_cards(context.db, context.card_store, std::move(*argument_0))), "post_cards"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards"};
        } catch (const holder::model::ConflictError& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_cards"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_cards"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_cards"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_cards"};
        }
    }
    static const std::regex route_46("^/alive$");
    if (method == "GET" && std::regex_match(route_path, match, route_46)) {
        try {
            return response{200, rig_json::encode(holder::api::check_alive()), "check_alive"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "check_alive"};
        }
    }
    static const std::regex route_47("^/bye$");
    if (method == "POST" && std::regex_match(route_path, match, route_47)) {
        try {
            return response{200, rig_json::encode(holder::api::say_goodbye()), "say_goodbye"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "say_goodbye"};
        }
    }
    static const std::regex route_51("^/health$");
    if (method == "GET" && std::regex_match(route_path, match, route_51)) {
        try {
            return response{200, rig_json::encode(holder::api::get_health(context.db, context.started_at)), "get_health"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_health"};
        }
    }
    static const std::regex route_57("^/locations$");
    if (method == "GET" && std::regex_match(route_path, match, route_57)) {
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
    static const std::regex route_63("^/rebuild$");
    if (method == "POST" && std::regex_match(route_path, match, route_63)) {
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
    static const std::regex route_64("^/resources$");
    if (method == "GET" && std::regex_match(route_path, match, route_64)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        std::optional<std::optional<int>> argument_2;
        std::optional<std::optional<int>> argument_3;
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
            const auto query_2 = query_values.find("limit");
            if (query_2 == query_values.end())
                argument_2.emplace(std::nullopt);
            else
                argument_2.emplace(decode_query_int(query_2->second));
            const auto query_3 = query_values.find("offset");
            if (query_3 == query_values.end())
                argument_3.emplace(std::nullopt);
            else
                argument_3.emplace(decode_query_int(query_3->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
            enforce_range(*argument_2, 1, 1000, "limit");
            enforce_range(*argument_3, 0, std::numeric_limits<int>::max(), "offset");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_resources");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_resources");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_resources(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3))), "get_resources"};
        } catch (const holder::api::ResourceListBadRequest& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_resources"};
        } catch (const holder::api::ResourceCardNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_resources"};
        } catch (const holder::api::ResourceCardInOtherProject& error) {
            return response{422, encode_error<holder::api::ErrorResponse>(422, error), "get_resources"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_resources"};
        }
    }
    static const std::regex route_66("^/resources$");
    if (method == "POST" && std::regex_match(route_path, match, route_66)) {
        std::optional<holder::api::ResourceCreateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::ResourceCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_resources");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_resources");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_resources(context.db, context.git_ops, context.uuid_v4, std::move(*argument_0))), "post_resources"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_resources"};
        } catch (const holder::model::ConflictError& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_resources"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_resources"};
        }
    }
    static const std::regex route_73("^/projects$");
    if (method == "GET" && std::regex_match(route_path, match, route_73)) {
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
    static const std::regex route_77("^/projects$");
    if (method == "POST" && std::regex_match(route_path, match, route_77)) {
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
    static const std::regex route_83("^/calendar$");
    if (method == "GET" && std::regex_match(route_path, match, route_83)) {
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
    static const std::regex route_85("^/cards$");
    if (method == "GET" && std::regex_match(route_path, match, route_85)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        std::optional<std::string> argument_2;
        std::optional<std::optional<std::string>> argument_3;
        std::optional<std::optional<std::string>> argument_4;
        std::optional<bool> argument_5;
        std::optional<int> argument_6;
        std::optional<std::optional<int>> argument_7;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("tag");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(std::string(query_1->second));
            const auto query_2 = query_values.find("view");
            if (query_2 == query_values.end())
                argument_2.emplace("tree");
            else
                argument_2.emplace(std::string(query_2->second));
            const auto query_3 = query_values.find("parent_card_id");
            if (query_3 == query_values.end())
                argument_3.emplace(std::nullopt);
            else
                argument_3.emplace(std::string(query_3->second));
            const auto query_4 = query_values.find("order");
            if (query_4 == query_values.end())
                argument_4.emplace(std::nullopt);
            else
                argument_4.emplace(std::string(query_4->second));
            const auto query_5 = query_values.find("count");
            if (query_5 == query_values.end())
                argument_5.emplace(false);
            else
                argument_5.emplace(decode_query_bool(query_5->second));
            const auto query_6 = query_values.find("limit");
            if (query_6 == query_values.end())
                argument_6.emplace(200);
            else
                argument_6.emplace(decode_query_int(query_6->second));
            const auto query_7 = query_values.find("include_deleted");
            if (query_7 == query_values.end())
                argument_7.emplace(std::nullopt);
            else
                argument_7.emplace(decode_query_int(query_7->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
            enforce_string_values(*argument_2, {"tree", "recent"}, "view");
            enforce_string_values(*argument_4, {"tree_default", "updated_desc", "title_asc"}, "order");
            enforce_range(*argument_6, 1, 5000, "limit");
            enforce_int_values(*argument_7, {0, 1}, "include_deleted");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_cards");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_cards");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_cards(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3), std::move(*argument_4), std::move(*argument_5), std::move(*argument_6), std::move(*argument_7))), "get_cards"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_cards"};
        }
    }
    static const std::regex route_92("^/ping$");
    if (method == "GET" && std::regex_match(route_path, match, route_92)) {
        try {
            return response{200, json(holder::api::get_ping()), "get_ping", {}, {}, "text/plain"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ping"};
        }
    }
    static const std::regex route_93("^/openapi\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_93))
        return std::nullopt;  // download_openapi_yaml is served by the host
    static const std::regex route_94("^/openapi\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_94))
        return std::nullopt;  // download_openapi_json is served by the host
    static const std::regex route_95("^/ai_catalog\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_95))
        return std::nullopt;  // download_ai_catalog_yaml is served by the host
    static const std::regex route_96("^/git_providers\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_96))
        return std::nullopt;  // download_git_providers_yaml is served by the host
    static const std::regex route_97("^/ai_catalog\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_97))
        return std::nullopt;  // download_ai_catalog_json is served by the host
    static const std::regex route_98("^/git_providers\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_98))
        return std::nullopt;  // download_git_providers_json is served by the host
    static const std::regex route_99("^/docs$");
    if (method == "GET" && std::regex_match(route_path, route_99))
        return std::nullopt;  // get_docs is served by the host
    static const std::regex route_101("^/trash$");
    if (method == "GET" && std::regex_match(route_path, match, route_101)) {
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
    static const std::regex route_109("^/imports$");
    if (method == "POST" && std::regex_match(route_path, match, route_109)) {
        std::optional<holder::api::AssetImportRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AssetImportRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_imports");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_imports");
        }
        try {
            return response{202, rig_json::encode(holder::api::post_imports(context.db, context.secret_store, context.git_ops, context.uuid_v4, std::move(*argument_0))), "post_imports"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_imports"};
        } catch (const holder::api::AssetImportLocationNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_imports"};
        } catch (const holder::api::support::StorageBindingRequired& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_imports"};
        } catch (const holder::api::AssetImportUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_imports"};
        } catch (const holder::api::SecretStoreUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_imports"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_imports"};
        }
    }
    static const std::regex route_110("^/locations$");
    if (method == "POST" && std::regex_match(route_path, match, route_110)) {
        std::optional<holder::api::StorageLocationRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::StorageLocationRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_locations");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_locations");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_locations(context.db, context.secret_store, context.git_ops, context.uuid_v4, std::move(*argument_0))), "post_locations"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_locations"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_locations"};
        }
    }
    static const std::regex route_120("^/reindex$");
    if (method == "POST" && std::regex_match(route_path, match, route_120)) {
        try {
            return response{200, rig_json::encode(holder::api::post_reindex(context.db)), "post_reindex"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_reindex"};
        }
    }
    static const std::regex route_126("^/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_126)) {
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
    static const std::regex route_127("^/trash$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_127)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("type");
            if (query_1 == query_values.end())
                argument_1.emplace("all");
            else
                argument_1.emplace(std::string(query_1->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
            enforce_string_values(*argument_1, {"card", "ai_message", "all"}, "type");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_trash");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_trash");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_trash(context.db, context.fts, context.card_store, std::move(*argument_0), std::move(*argument_1))), "delete_trash"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_trash"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "delete_trash"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_trash"};
        }
    }
    static const std::regex route_14("^/ai/messages$");
    if (method == "GET" && std::regex_match(route_path, match, route_14)) {
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
    static const std::regex route_18("^/ai/messages$");
    if (method == "POST" && std::regex_match(route_path, match, route_18)) {
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
    static const std::regex route_25("^/ai/nudges$");
    if (method == "GET" && std::regex_match(route_path, match, route_25)) {
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
    static const std::regex route_29("^/ai/runners$");
    if (method == "GET" && std::regex_match(route_path, match, route_29)) {
        try {
            return response{200, rig_json::encode(holder::api::get_ai_runners(context.registry)), "get_ai_runners"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_runners"};
        }
    }
    static const std::regex route_31("^/ai/runners$");
    if (method == "POST" && std::regex_match(route_path, match, route_31)) {
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
    static const std::regex route_36("^/ai/runs$");
    if (method == "GET" && std::regex_match(route_path, match, route_36)) {
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
    static const std::regex route_38("^/ai/capabilities$");
    if (method == "GET" && std::regex_match(route_path, match, route_38)) {
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
    static const std::regex route_39("^/ai/status$");
    if (method == "GET" && std::regex_match(route_path, match, route_39)) {
        try {
            return response{200, rig_json::encode(holder::api::get_ai_status(context.db, context.registry)), "get_ai_status"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_status"};
        }
    }
    static const std::regex route_40("^/ai/threads$");
    if (method == "GET" && std::regex_match(route_path, match, route_40)) {
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
    static const std::regex route_42("^/ai/threads$");
    if (method == "POST" && std::regex_match(route_path, match, route_42)) {
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
    static const std::regex route_48("^/card\\-references/resolve$");
    if (method == "POST" && std::regex_match(route_path, match, route_48)) {
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
    static const std::regex route_49("^/events/cursor$");
    if (method == "GET" && std::regex_match(route_path, match, route_49)) {
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
    static const std::regex route_86("^/cards/context$");
    if (method == "GET" && std::regex_match(route_path, match, route_86)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        std::optional<std::string> argument_2;
        std::optional<bool> argument_3;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("parent_card_id");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(std::string(query_1->second));
            const auto query_2 = query_values.find("order");
            if (query_2 == query_values.end())
                argument_2.emplace("tree_default");
            else
                argument_2.emplace(std::string(query_2->second));
            const auto query_3 = query_values.find("count");
            if (query_3 == query_values.end())
                argument_3.emplace(false);
            else
                argument_3.emplace(decode_query_bool(query_3->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
            enforce_string_values(*argument_2, {"tree_default", "updated_desc", "title_asc"}, "order");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_cards_context");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_cards_context");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_cards_context(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3))), "get_cards_context"};
        } catch (const holder::api::CardReadNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_cards_context"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_cards_context"};
        }
    }
    static const std::regex route_116("^/locations/preferred$");
    if (method == "PUT" && std::regex_match(route_path, match, route_116)) {
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
    static const std::regex route_117("^/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, match, route_117)) {
        std::optional<holder::api::ProjectRecoveryTokenImportRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::ProjectRecoveryTokenImportRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_recovery_token_import");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_recovery_token_import");
        }
        try {
            auto outcome = holder::api::post_recovery_token_import(context.db, context.git_ops, std::move(*argument_0));
            if (outcome.status != 200 && outcome.status != 201)
                throw std::logic_error("post_recovery_token_import chose an undeclared success status");
            return response{outcome.status, rig_json::encode(outcome.body), "post_recovery_token_import"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_recovery_token_import"};
        } catch (const holder::api::support::PrivacyBroken& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_recovery_token_import"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_recovery_token_import"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_recovery_token_import"};
        }
    }
    static const std::regex route_121("^/search/cards$");
    if (method == "GET" && std::regex_match(route_path, match, route_121)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<int> argument_2;
        std::optional<int> argument_3;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("q");
            if (query_1 == query_values.end())
                throw std::invalid_argument("missing query parameter: q");
            argument_1.emplace(std::string(query_1->second));
            const auto query_2 = query_values.find("limit");
            if (query_2 == query_values.end())
                argument_2.emplace(20);
            else
                argument_2.emplace(decode_query_int(query_2->second));
            const auto query_3 = query_values.find("offset");
            if (query_3 == query_values.end())
                argument_3.emplace(0);
            else
                argument_3.emplace(decode_query_int(query_3->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
            enforce_length(*argument_1, 1, std::numeric_limits<std::size_t>::max(), "q");
            enforce_range(*argument_2, 1, std::numeric_limits<int>::max(), "limit");
            enforce_range(*argument_3, 0, std::numeric_limits<int>::max(), "offset");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_search_cards");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_search_cards");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_search_cards(context.fts, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3))), "get_search_cards"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_search_cards"};
        } catch (const holder::api::SearchUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "get_search_cards"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_search_cards"};
        }
    }
    static const std::regex route_122("^/search/ai$");
    if (method == "GET" && std::regex_match(route_path, match, route_122)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<int> argument_2;
        std::optional<int> argument_3;
        try {
            const auto query_values = decode_query(query_string);
            const auto query_0 = query_values.find("project_id");
            if (query_0 == query_values.end())
                throw std::invalid_argument("missing query parameter: project_id");
            argument_0.emplace(std::string(query_0->second));
            const auto query_1 = query_values.find("q");
            if (query_1 == query_values.end())
                throw std::invalid_argument("missing query parameter: q");
            argument_1.emplace(std::string(query_1->second));
            const auto query_2 = query_values.find("limit");
            if (query_2 == query_values.end())
                argument_2.emplace(20);
            else
                argument_2.emplace(decode_query_int(query_2->second));
            const auto query_3 = query_values.find("offset");
            if (query_3 == query_values.end())
                argument_3.emplace(0);
            else
                argument_3.emplace(decode_query_int(query_3->second));
            enforce_length(*argument_0, 1, std::numeric_limits<std::size_t>::max(), "project_id");
            enforce_length(*argument_1, 1, std::numeric_limits<std::size_t>::max(), "q");
            enforce_range(*argument_2, 1, std::numeric_limits<int>::max(), "limit");
            enforce_range(*argument_3, 0, std::numeric_limits<int>::max(), "offset");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_search_ai");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_search_ai");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_search_ai(context.fts, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3))), "get_search_ai"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_search_ai"};
        } catch (const holder::api::SearchUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "get_search_ai"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_search_ai"};
        }
    }
    static const std::regex route_123("^/ai/runs$");
    if (method == "POST" && std::regex_match(route_path, match, route_123)) {
        std::optional<holder::api::AiRunCreateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiRunCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_ai_runs");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_ai_runs");
        }
        try {
            auto outcome = holder::api::post_ai_runs(context.db, context.fts, context.secret_store, context.registry, context.uuid_v4, context.events, std::move(*argument_0));
            if (outcome.status != 200)
                throw std::logic_error("post_ai_runs chose an undeclared success status");
            return response{outcome.status, json(), "post_ai_runs", {}, std::move(outcome.value)};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_runs"};
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
    static const std::regex route_12("^/ai/local\\-models/config$");
    if (method == "GET" && std::regex_match(route_path, match, route_12)) {
        try {
            return response{200, rig_json::encode(holder::api::get_local_model_config(context.db)), "get_local_model_config"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_local_model_config"};
        }
    }
    static const std::regex route_13("^/ai/local\\-models/config$");
    if (method == "PUT" && std::regex_match(route_path, match, route_13)) {
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
    static const std::regex route_24("^/ai/messages/capture$");
    if (method == "POST" && std::regex_match(route_path, match, route_24)) {
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
    static const std::regex route_27("^/ai/nudges/evaluate$");
    if (method == "POST" && std::regex_match(route_path, match, route_27)) {
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
    static const std::regex route_28("^/ai/providers/catalog$");
    if (method == "GET" && std::regex_match(route_path, match, route_28)) {
        try {
            return response{200, rig_json::encode(holder::api::get_ai_provider_catalog(context.db)), "get_ai_provider_catalog"};
        } catch (const holder::api::AiProviderCatalogMissing& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_ai_provider_catalog"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_provider_catalog"};
        }
    }
    static const std::regex route_60("^/ai/providers/settings$");
    if (method == "GET" && std::regex_match(route_path, match, route_60)) {
        try {
            return response{200, rig_json::encode(holder::api::list_provider_settings(context.db)), "list_provider_settings"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_provider_settings"};
        }
    }
    static const std::regex route_61("^/ai/providers/settings$");
    if (method == "PUT" && std::regex_match(route_path, match, route_61)) {
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
    static const std::regex route_62("^/ai/providers/settings/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_62)) {
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
    static const std::regex route_125("^/ai/runner/pull/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_125)) {
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
    static const std::regex route_15("^/ai/messages/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_15)) {
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
    static const std::regex route_19("^/ai/messages/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_19)) {
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
    static const std::regex route_20("^/ai/messages/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_20)) {
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
    static const std::regex route_30("^/ai/runners/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_30)) {
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
    static const std::regex route_32("^/ai/runners/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_32)) {
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
    static const std::regex route_33("^/ai/runners/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_33)) {
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
    static const std::regex route_37("^/ai/runs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_37)) {
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
    static const std::regex route_41("^/ai/threads/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_41)) {
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
    static const std::regex route_43("^/ai/threads/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_43)) {
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
    static const std::regex route_44("^/ai/threads/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_44)) {
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
    static const std::regex route_16("^/ai/messages/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, match, route_16)) {
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
    static const std::regex route_17("^/ai/messages/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, match, route_17)) {
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
    static const std::regex route_21("^/ai/messages/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, match, route_21)) {
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
    static const std::regex route_22("^/ai/messages/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, match, route_22)) {
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
    static const std::regex route_23("^/ai/messages/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_23)) {
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
    static const std::regex route_26("^/ai/nudges/([^/]+)/dismiss$");
    if (method == "POST" && std::regex_match(route_path, match, route_26)) {
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
    static const std::regex route_34("^/ai/runners/([^/]+)/retry$");
    if (method == "POST" && std::regex_match(route_path, match, route_34)) {
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
    static const std::regex route_124("^/ai/runs/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_124)) {
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
    static const std::regex route_6("^/cards/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_6)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::CardUpdateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::CardUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "patch_cards_card_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "patch_cards_card_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::patch_cards_card_id(context.card_store, std::move(*argument_0), std::move(*argument_1))), "patch_cards_card_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_cards_card_id"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_cards_card_id"};
        } catch (const holder::api::CardWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "patch_cards_card_id"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "patch_cards_card_id"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "patch_cards_card_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "patch_cards_card_id"};
        }
    }
    static const std::regex route_7("^/cards/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_7)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            const auto query_1 = query_values.find("children");
            if (query_1 == query_values.end())
                argument_1.emplace("promote");
            else
                argument_1.emplace(std::string(query_1->second));
            enforce_string_values(*argument_1, {"promote", "trash_subtree"}, "children");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_cards_card_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_cards_card_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_cards_card_id(context.card_store, std::move(*argument_0), std::move(*argument_1))), "delete_cards_card_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_cards_card_id"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_cards_card_id"};
        } catch (const holder::api::CardWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_cards_card_id"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "delete_cards_card_id"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "delete_cards_card_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_cards_card_id"};
        }
    }
    static const std::regex route_35("^/imports/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_35)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_import_job");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_import_job");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_import_job(std::move(*argument_0))), "get_import_job"};
        } catch (const holder::api::AssetImportJobNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_import_job"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_import_job"};
        }
    }
    static const std::regex route_58("^/locations/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_58)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_location");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_location");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_location(context.db, context.secret_store, std::move(*argument_0))), "get_location"};
        } catch (const holder::api::LocationNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_location"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_location"};
        }
    }
    static const std::regex route_65("^/resources/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_65)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_resource");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_resource");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_resource(context.db, std::move(*argument_0))), "get_resource"};
        } catch (const holder::api::ResourceNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_resource"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_resource"};
        }
    }
    static const std::regex route_67("^/resources/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_67)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::ResourceUpdateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::ResourceUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "patch_resources_resource_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "patch_resources_resource_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::patch_resources_resource_id(context.db, context.git_ops, std::move(*argument_0), std::move(*argument_1))), "patch_resources_resource_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_resources_resource_id"};
        } catch (const holder::api::ResourceNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "patch_resources_resource_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "patch_resources_resource_id"};
        }
    }
    static const std::regex route_68("^/resources/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_68)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_resources_resource_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_resources_resource_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_resources_resource_id(context.db, context.git_ops, std::move(*argument_0))), "delete_resources_resource_id"};
        } catch (const holder::api::ResourceNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_resources_resource_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_resources_resource_id"};
        }
    }
    static const std::regex route_74("^/projects/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_74)) {
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
    static const std::regex route_81("^/projects/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_81)) {
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
    static const std::regex route_82("^/projects/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_82)) {
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
    static const std::regex route_87("^/cards/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_87)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_card");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_card");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_card(context.db, context.card_store, std::move(*argument_0))), "get_card"};
        } catch (const holder::api::CardReadNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_card"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "get_card"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_card"};
        }
    }
    static const std::regex route_100("^/docs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_100))
        return std::nullopt;  // get_docs_file is served by the host
    static const std::regex route_111("^/locations/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_111)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::PatchLocationsLocationIdRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::PatchLocationsLocationIdRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "patch_locations_location_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "patch_locations_location_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::patch_locations_location_id(context.db, context.secret_store, context.git_ops, std::move(*argument_0), std::move(*argument_1))), "patch_locations_location_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_locations_location_id"};
        } catch (const holder::api::LocationNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "patch_locations_location_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "patch_locations_location_id"};
        }
    }
    static const std::regex route_112("^/locations/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_112)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_locations_location_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_locations_location_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_locations_location_id(context.db, context.secret_store, context.git_ops, std::move(*argument_0))), "delete_locations_location_id"};
        } catch (const holder::api::LocationNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_locations_location_id"};
        } catch (const holder::model::ConflictError& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "delete_locations_location_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_locations_location_id"};
        }
    }
    static const std::regex route_8("^/cards/([^/]+)/move$");
    if (method == "POST" && std::regex_match(route_path, match, route_8)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::CardMoveRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::CardMoveRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_cards_card_id_move");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_cards_card_id_move");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_cards_card_id_move(context.card_store, std::move(*argument_0), std::move(*argument_1))), "post_cards_card_id_move"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_move"};
        } catch (const holder::api::CardWriteInvalid& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_move"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_move"};
        } catch (const holder::api::CardWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_cards_card_id_move"};
        } catch (const holder::api::CardMoveTargetNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_cards_card_id_move"};
        } catch (const holder::api::CardWriteRejected& error) {
            return response{422, encode_error<holder::api::ErrorResponse>(422, error), "post_cards_card_id_move"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_cards_card_id_move"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_cards_card_id_move"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_cards_card_id_move"};
        }
    }
    static const std::regex route_9("^/cards/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, match, route_9)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_cards_card_id_restore");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_cards_card_id_restore");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_cards_card_id_restore(context.card_store, std::move(*argument_0))), "post_cards_card_id_restore"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_restore"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_restore"};
        } catch (const holder::api::CardWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_cards_card_id_restore"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_cards_card_id_restore"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_cards_card_id_restore"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_cards_card_id_restore"};
        }
    }
    static const std::regex route_10("^/cards/([^/]+)/tags$");
    if (method == "POST" && std::regex_match(route_path, match, route_10)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::CardTagMutationRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::CardTagMutationRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_cards_card_id_tags");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_cards_card_id_tags");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_cards_card_id_tags(context.card_store, std::move(*argument_0), std::move(*argument_1))), "post_cards_card_id_tags"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_tags"};
        } catch (const holder::api::CardWriteInvalid& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_tags"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_tags"};
        } catch (const holder::api::CardWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_cards_card_id_tags"};
        } catch (const holder::api::CardWriteRejected& error) {
            return response{422, encode_error<holder::api::ErrorResponse>(422, error), "post_cards_card_id_tags"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_cards_card_id_tags"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_cards_card_id_tags"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_cards_card_id_tags"};
        }
    }
    static const std::regex route_11("^/cards/([^/]+)/tags$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_11)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::CardTagMutationRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::CardTagMutationRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_cards_card_id_tags");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_cards_card_id_tags");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_cards_card_id_tags(context.card_store, std::move(*argument_0), std::move(*argument_1))), "delete_cards_card_id_tags"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_cards_card_id_tags"};
        } catch (const holder::api::CardWriteInvalid& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_cards_card_id_tags"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_cards_card_id_tags"};
        } catch (const holder::api::CardWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_cards_card_id_tags"};
        } catch (const holder::api::CardWriteRejected& error) {
            return response{422, encode_error<holder::api::ErrorResponse>(422, error), "delete_cards_card_id_tags"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "delete_cards_card_id_tags"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "delete_cards_card_id_tags"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_cards_card_id_tags"};
        }
    }
    static const std::regex route_52("^/projects/([^/]+)/history$");
    if (method == "GET" && std::regex_match(route_path, match, route_52)) {
        std::optional<std::string> argument_0;
        std::optional<int> argument_1;
        std::optional<std::optional<std::string>> argument_2;
        std::optional<std::optional<std::string>> argument_3;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            const auto query_1 = query_values.find("limit");
            if (query_1 == query_values.end())
                argument_1.emplace(50);
            else
                argument_1.emplace(decode_query_int(query_1->second));
            const auto query_2 = query_values.find("cursor");
            if (query_2 == query_values.end())
                argument_2.emplace(std::nullopt);
            else
                argument_2.emplace(std::string(query_2->second));
            const auto query_3 = query_values.find("kind");
            if (query_3 == query_values.end())
                argument_3.emplace(std::nullopt);
            else
                argument_3.emplace(std::string(query_3->second));
            enforce_range(*argument_1, 1, 200, "limit");
            enforce_string_values(*argument_3, {"card", "resource", "location", "ai_data", "project_settings", "unknown"}, "kind");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_project_history");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_project_history");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_project_history(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3))), "get_project_history"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_project_history"};
        } catch (const holder::api::HistoryNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_project_history"};
        } catch (const holder::api::HistoryConflict& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "get_project_history"};
        } catch (const holder::api::HistoryResponseTooLarge& error) {
            return response{413, encode_error<holder::api::ErrorResponse>(413, error), "get_project_history"};
        } catch (const holder::api::HistoryUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "get_project_history"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_project_history"};
        }
    }
    static const std::regex route_59("^/projects/([^/]+)/tags$");
    if (method == "GET" && std::regex_match(route_path, match, route_59)) {
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
    static const std::regex route_76("^/projects/([^/]+)/encryption\\-check$");
    if (method == "GET" && std::regex_match(route_path, match, route_76)) {
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
    static const std::regex route_78("^/projects/([^/]+)/cards$");
    if (method == "GET" && std::regex_match(route_path, match, route_78)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<std::string>> argument_1;
        std::optional<std::optional<std::variant<holder::api::ParentRoots, holder::api::ParentUuid>>> argument_2;
        std::optional<std::string> argument_3;
        std::optional<int> argument_4;
        std::optional<std::optional<std::string>> argument_5;
        std::optional<bool> argument_6;
        std::optional<bool> argument_7;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            const auto query_1 = query_values.find("tag");
            if (query_1 == query_values.end())
                argument_1.emplace(std::nullopt);
            else
                argument_1.emplace(std::string(query_1->second));
            const auto query_2 = query_values.find("parent");
            if (query_2 == query_values.end())
                argument_2.emplace(std::nullopt);
            else
                argument_2.emplace(rig_json::decode<std::variant<holder::api::ParentRoots, holder::api::ParentUuid>>(json(std::string(query_2->second))));
            const auto query_3 = query_values.find("order");
            if (query_3 == query_values.end())
                argument_3.emplace("card_id_asc");
            else
                argument_3.emplace(std::string(query_3->second));
            const auto query_4 = query_values.find("limit");
            if (query_4 == query_values.end())
                argument_4.emplace(200);
            else
                argument_4.emplace(decode_query_int(query_4->second));
            const auto query_5 = query_values.find("cursor");
            if (query_5 == query_values.end())
                argument_5.emplace(std::nullopt);
            else
                argument_5.emplace(std::string(query_5->second));
            const auto query_6 = query_values.find("include_content");
            if (query_6 == query_values.end())
                argument_6.emplace(false);
            else
                argument_6.emplace(decode_query_bool(query_6->second));
            const auto query_7 = query_values.find("include_child_count");
            if (query_7 == query_values.end())
                argument_7.emplace(false);
            else
                argument_7.emplace(decode_query_bool(query_7->second));
            enforce_length(*argument_1, 1, std::numeric_limits<std::size_t>::max(), "tag");
            enforce_string_values(*argument_3, {"card_id_asc", "updated_desc", "sort_key_asc"}, "order");
            enforce_range(*argument_4, 1, 5000, "limit");
            enforce_length(*argument_5, 1, 8192, "cursor");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_projects_project_id_cards");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_projects_project_id_cards");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_projects_project_id_cards(context.db, context.card_store, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3), std::move(*argument_4), std::move(*argument_5), std::move(*argument_6), std::move(*argument_7))), "get_projects_project_id_cards"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_projects_project_id_cards"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_projects_project_id_cards"};
        } catch (const holder::api::ProjectCardsNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_projects_project_id_cards"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "get_projects_project_id_cards"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "get_projects_project_id_cards"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_projects_project_id_cards"};
        }
    }
    static const std::regex route_80("^/projects/([^/]+)/repair\\-hierarchy$");
    if (method == "POST" && std::regex_match(route_path, match, route_80)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<holder::api::HierarchyRepairRequest>> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(body.empty() ? std::optional<holder::api::HierarchyRepairRequest>{} : std::optional<holder::api::HierarchyRepairRequest>(decode_body<holder::api::HierarchyRepairRequest>(body)));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_projects_project_id_repair_hierarchy");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_projects_project_id_repair_hierarchy");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_projects_project_id_repair_hierarchy(context.db, context.card_store, std::move(*argument_0), std::move(*argument_1))), "post_projects_project_id_repair_hierarchy"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_projects_project_id_repair_hierarchy"};
        } catch (const holder::api::ProjectCardsNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_projects_project_id_repair_hierarchy"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_projects_project_id_repair_hierarchy"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_projects_project_id_repair_hierarchy"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_repair_hierarchy"};
        }
    }
    static const std::regex route_84("^/cards/([^/]+)/milestones$");
    if (method == "GET" && std::regex_match(route_path, match, route_84)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "list_card_milestones");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "list_card_milestones");
        }
        try {
            return response{200, rig_json::encode(holder::api::list_card_milestones(context.db, std::move(*argument_0))), "list_card_milestones"};
        } catch (const holder::api::MilestoneCardNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "list_card_milestones"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_card_milestones"};
        }
    }
    static const std::regex route_88("^/cards/([^/]+)/resources$");
    if (method == "POST" && std::regex_match(route_path, match, route_88)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::PostCardsCardIdResourcesRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::PostCardsCardIdResourcesRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_cards_card_id_resources");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_cards_card_id_resources");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_cards_card_id_resources(context.db, context.fts, context.card_store, std::move(*argument_0), std::move(*argument_1))), "post_cards_card_id_resources"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_resources"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_resources"};
        } catch (const holder::api::CardResourceNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_cards_card_id_resources"};
        } catch (const holder::api::CardResourceProjectMismatch& error) {
            return response{422, encode_error<holder::api::ErrorResponse>(422, error), "post_cards_card_id_resources"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_cards_card_id_resources"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_cards_card_id_resources"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_cards_card_id_resources"};
        }
    }
    static const std::regex route_89("^/cards/([^/]+)/resources$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_89)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::DeleteCardsCardIdResourcesRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::DeleteCardsCardIdResourcesRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_cards_card_id_resources");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_cards_card_id_resources");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_cards_card_id_resources(context.db, context.fts, context.card_store, std::move(*argument_0), std::move(*argument_1))), "delete_cards_card_id_resources"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_cards_card_id_resources"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_cards_card_id_resources"};
        } catch (const holder::api::CardResourceNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_cards_card_id_resources"};
        } catch (const holder::api::CardResourceProjectMismatch& error) {
            return response{422, encode_error<holder::api::ErrorResponse>(422, error), "delete_cards_card_id_resources"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "delete_cards_card_id_resources"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "delete_cards_card_id_resources"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_cards_card_id_resources"};
        }
    }
    static const std::regex route_90("^/cards/([^/]+)/milestones\\-with\\-dates$");
    if (method == "GET" && std::regex_match(route_path, match, route_90)) {
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
    static const std::regex route_91("^/projects/([^/]+)/milestones\\-with\\-dates$");
    if (method == "GET" && std::regex_match(route_path, match, route_91)) {
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
    static const std::regex route_102("^/cards/([^/]+)/milestones\\-with\\-dates$");
    if (method == "POST" && std::regex_match(route_path, match, route_102)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::MilestoneWithDatesCreateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::MilestoneWithDatesCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_cards_card_id_milestones_with_dates");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_cards_card_id_milestones_with_dates");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_cards_card_id_milestones_with_dates(context.db, context.card_store, context.uuid_v4, std::move(*argument_0), std::move(*argument_1))), "post_cards_card_id_milestones_with_dates"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_milestones_with_dates"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_milestones_with_dates"};
        } catch (const holder::api::MilestoneWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_cards_card_id_milestones_with_dates"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_cards_card_id_milestones_with_dates"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_cards_card_id_milestones_with_dates"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_cards_card_id_milestones_with_dates"};
        }
    }
    static const std::regex route_105("^/cards/([^/]+)/milestones$");
    if (method == "POST" && std::regex_match(route_path, match, route_105)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::MilestoneCreateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::MilestoneCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_cards_card_id_milestones");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_cards_card_id_milestones");
        }
        try {
            return response{201, rig_json::encode(holder::api::post_cards_card_id_milestones(context.db, context.card_store, context.uuid_v4, std::move(*argument_0), std::move(*argument_1))), "post_cards_card_id_milestones"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_milestones"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_cards_card_id_milestones"};
        } catch (const holder::api::MilestoneWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_cards_card_id_milestones"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_cards_card_id_milestones"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_cards_card_id_milestones"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_cards_card_id_milestones"};
        }
    }
    static const std::regex route_113("^/locations/([^/]+)/binding$");
    if (method == "PUT" && std::regex_match(route_path, match, route_113)) {
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
    static const std::regex route_114("^/locations/([^/]+)/binding$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_114)) {
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
    static const std::regex route_115("^/locations/([^/]+)/test$");
    if (method == "POST" && std::regex_match(route_path, match, route_115)) {
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
    static const std::regex route_130("^/cards/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, match, route_130)) {
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
    static const std::regex route_131("^/cards/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, match, route_131)) {
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
    static const std::regex route_132("^/cards/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, match, route_132)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::routes::CardLinkCreateRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::routes::CardLinkCreateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "create_card_link");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "create_card_link");
        }
        try {
            return response{201, rig_json::encode(holder::api::routes::create_card_link(context.db, context.fts, context.card_store, std::move(*argument_0), std::move(*argument_1))), "create_card_link"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "create_card_link"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "create_card_link"};
        } catch (const holder::api::routes::CardLinkSourceNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "create_card_link"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "create_card_link"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "create_card_link"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "create_card_link"};
        }
    }
    static const std::regex route_133("^/cards/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_133)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<holder::api::CardLinkDeleteRequest>> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(body.empty() ? std::optional<holder::api::CardLinkDeleteRequest>{} : std::optional<holder::api::CardLinkDeleteRequest>(decode_body<holder::api::CardLinkDeleteRequest>(body)));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_card_links");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_card_links");
        }
        try {
            return response{200, rig_json::encode(holder::api::routes::delete_card_links(context.db, context.fts, context.card_store, std::move(*argument_0), std::move(*argument_1))), "delete_card_links"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_card_links"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_card_links"};
        } catch (const holder::api::routes::CardLinkSourceNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_card_links"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "delete_card_links"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "delete_card_links"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_card_links"};
        }
    }
    static const std::regex route_69("^/projects/([^/]+)/git/test\\-remote$");
    if (method == "POST" && std::regex_match(route_path, match, route_69)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<holder::api::ProjectGitTestRemoteRequest>> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(body.empty() ? std::optional<holder::api::ProjectGitTestRemoteRequest>{} : std::optional<holder::api::ProjectGitTestRemoteRequest>(decode_body<holder::api::ProjectGitTestRemoteRequest>(body)));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_projects_project_id_git_test_remote");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_projects_project_id_git_test_remote");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_projects_project_id_git_test_remote(context.db, context.git_ops, std::move(*argument_0), std::move(*argument_1))), "post_projects_project_id_git_test_remote"};
        } catch (const holder::api::ProjectGitNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_projects_project_id_git_test_remote"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_git_test_remote"};
        }
    }
    static const std::regex route_70("^/projects/([^/]+)/git/push$");
    if (method == "POST" && std::regex_match(route_path, match, route_70)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<holder::api::ProjectGitPushRequest>> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(body.empty() ? std::optional<holder::api::ProjectGitPushRequest>{} : std::optional<holder::api::ProjectGitPushRequest>(decode_body<holder::api::ProjectGitPushRequest>(body)));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_projects_project_id_git_push");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_projects_project_id_git_push");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_projects_project_id_git_push(context.db, context.git_ops, std::move(*argument_0), std::move(*argument_1))), "post_projects_project_id_git_push"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_projects_project_id_git_push"};
        } catch (const holder::api::ProjectGitNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_projects_project_id_git_push"};
        } catch (const holder::api::support::PrivacyBroken& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_git_push"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_projects_project_id_git_push"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_git_push"};
        }
    }
    static const std::regex route_71("^/projects/([^/]+)/git/pull$");
    if (method == "POST" && std::regex_match(route_path, match, route_71)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<holder::api::ProjectGitSyncRequest>> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(body.empty() ? std::optional<holder::api::ProjectGitSyncRequest>{} : std::optional<holder::api::ProjectGitSyncRequest>(decode_body<holder::api::ProjectGitSyncRequest>(body)));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_projects_project_id_git_pull");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_projects_project_id_git_pull");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_projects_project_id_git_pull(context.db, context.git_ops, std::move(*argument_0), std::move(*argument_1))), "post_projects_project_id_git_pull"};
        } catch (const holder::api::ProjectGitNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_projects_project_id_git_pull"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_git_pull"};
        }
    }
    static const std::regex route_72("^/projects/([^/]+)/git/sync$");
    if (method == "POST" && std::regex_match(route_path, match, route_72)) {
        std::optional<std::string> argument_0;
        std::optional<std::optional<holder::api::ProjectGitSyncRequest>> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(body.empty() ? std::optional<holder::api::ProjectGitSyncRequest>{} : std::optional<holder::api::ProjectGitSyncRequest>(decode_body<holder::api::ProjectGitSyncRequest>(body)));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_projects_project_id_git_sync");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_projects_project_id_git_sync");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_projects_project_id_git_sync(context.db, context.git_ops, std::move(*argument_0), std::move(*argument_1))), "post_projects_project_id_git_sync"};
        } catch (const holder::api::ProjectGitNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_projects_project_id_git_sync"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_git_sync"};
        }
    }
    static const std::regex route_75("^/projects/([^/]+)/git/sync\\-status$");
    if (method == "GET" && std::regex_match(route_path, match, route_75)) {
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
    static const std::regex route_118("^/projects/([^/]+)/recovery\\-token/export$");
    if (method == "POST" && std::regex_match(route_path, match, route_118)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::ProjectRecoveryTokenExportRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::ProjectRecoveryTokenExportRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_projects_project_id_recovery_token_export");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_projects_project_id_recovery_token_export");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_projects_project_id_recovery_token_export(context.db, std::move(*argument_0), std::move(*argument_1))), "post_projects_project_id_recovery_token_export"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_projects_project_id_recovery_token_export"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_projects_project_id_recovery_token_export"};
        } catch (const holder::api::RecoveryTokenProjectNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_projects_project_id_recovery_token_export"};
        } catch (const holder::api::support::PrivacyBroken& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_recovery_token_export"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_projects_project_id_recovery_token_export"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_recovery_token_export"};
        }
    }
    static const std::regex route_119("^/projects/([^/]+)/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, match, route_119)) {
        std::optional<std::string> argument_0;
        std::optional<holder::api::ProjectRecoveryTokenImportRequest> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_body<holder::api::ProjectRecoveryTokenImportRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_projects_project_id_recovery_token_import");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_projects_project_id_recovery_token_import");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_projects_project_id_recovery_token_import(context.db, std::move(*argument_0), std::move(*argument_1))), "post_projects_project_id_recovery_token_import"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_projects_project_id_recovery_token_import"};
        } catch (const holder::api::RecoveryTokenProjectNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_projects_project_id_recovery_token_import"};
        } catch (const holder::api::support::PrivacyBroken& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_recovery_token_import"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_projects_project_id_recovery_token_import"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_recovery_token_import"};
        }
    }
    static const std::regex route_50("^/locations/([^/]+)/oauth/google\\-drive/authorize$");
    if (method == "POST" && std::regex_match(route_path, match, route_50)) {
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
    static const std::regex route_108("^/locations/([^/]+)/oauth/google\\-drive/callback$");
    if (method == "GET" && std::regex_match(route_path, route_108))
        return std::nullopt;  // get_locations_location_id_oauth_google_drive_callback is served by the host
    static const std::regex route_53("^/projects/([^/]+)/history/cards/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_53)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<int> argument_2;
        std::optional<std::optional<std::string>> argument_3;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
            const auto query_2 = query_values.find("limit");
            if (query_2 == query_values.end())
                argument_2.emplace(50);
            else
                argument_2.emplace(decode_query_int(query_2->second));
            const auto query_3 = query_values.find("cursor");
            if (query_3 == query_values.end())
                argument_3.emplace(std::nullopt);
            else
                argument_3.emplace(std::string(query_3->second));
            enforce_range(*argument_2, 1, 200, "limit");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_card_history");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_card_history");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_card_history(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3))), "get_card_history"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_card_history"};
        } catch (const holder::api::HistoryNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_card_history"};
        } catch (const holder::api::HistoryConflict& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "get_card_history"};
        } catch (const holder::api::HistoryResponseTooLarge& error) {
            return response{413, encode_error<holder::api::ErrorResponse>(413, error), "get_card_history"};
        } catch (const holder::api::HistoryUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "get_card_history"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_card_history"};
        }
    }
    static const std::regex route_54("^/projects/([^/]+)/history/cards/([^/]+)/compare$");
    if (method == "GET" && std::regex_match(route_path, match, route_54)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<std::optional<holder::api::RevisionReference>> argument_2;
        std::optional<holder::api::RevisionReference> argument_3;
        std::optional<std::string> argument_4;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
            const auto query_2 = query_values.find("from");
            if (query_2 == query_values.end())
                argument_2.emplace(std::nullopt);
            else
                argument_2.emplace(rig_json::decode<holder::api::RevisionReference>(json(std::string(query_2->second))));
            const auto query_3 = query_values.find("to");
            if (query_3 == query_values.end())
                throw std::invalid_argument("missing query parameter: to");
            argument_3.emplace(rig_json::decode<holder::api::RevisionReference>(json(std::string(query_3->second))));
            const auto query_4 = query_values.find("mode");
            if (query_4 == query_values.end())
                argument_4.emplace("since");
            else
                argument_4.emplace(std::string(query_4->second));
            enforce_string_values(*argument_4, {"since", "change"}, "mode");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "compare_card_history");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "compare_card_history");
        }
        try {
            return response{200, rig_json::encode(holder::api::compare_card_history(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2), std::move(*argument_3), std::move(*argument_4))), "compare_card_history"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "compare_card_history"};
        } catch (const holder::api::HistoryNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "compare_card_history"};
        } catch (const holder::api::HistoryConflict& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "compare_card_history"};
        } catch (const holder::api::HistoryResponseTooLarge& error) {
            return response{413, encode_error<holder::api::ErrorResponse>(413, error), "compare_card_history"};
        } catch (const holder::api::HistoryUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "compare_card_history"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "compare_card_history"};
        }
    }
    static const std::regex route_55("^/projects/([^/]+)/history/cards/([^/]+)/snapshot$");
    if (method == "GET" && std::regex_match(route_path, match, route_55)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<holder::api::RevisionReference> argument_2;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
            const auto query_2 = query_values.find("oid");
            if (query_2 == query_values.end())
                throw std::invalid_argument("missing query parameter: oid");
            argument_2.emplace(rig_json::decode<holder::api::RevisionReference>(json(std::string(query_2->second))));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_card_history_snapshot");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_card_history_snapshot");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_card_history_snapshot(context.db, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2))), "get_card_history_snapshot"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_card_history_snapshot"};
        } catch (const holder::api::HistoryNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_card_history_snapshot"};
        } catch (const holder::api::HistoryConflict& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "get_card_history_snapshot"};
        } catch (const holder::api::HistoryResponseTooLarge& error) {
            return response{413, encode_error<holder::api::ErrorResponse>(413, error), "get_card_history_snapshot"};
        } catch (const holder::api::HistoryUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "get_card_history_snapshot"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_card_history_snapshot"};
        }
    }
    static const std::regex route_56("^/projects/([^/]+)/history/cards/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, match, route_56)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<holder::api::RevisionReference> argument_2;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
            const auto query_2 = query_values.find("oid");
            if (query_2 == query_values.end())
                throw std::invalid_argument("missing query parameter: oid");
            argument_2.emplace(rig_json::decode<holder::api::RevisionReference>(json(std::string(query_2->second))));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "post_projects_project_id_history_cards_card_id_restore");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "post_projects_project_id_history_cards_card_id_restore");
        }
        try {
            return response{200, rig_json::encode(holder::api::post_projects_project_id_history_cards_card_id_restore(context.db, context.card_store, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2))), "post_projects_project_id_history_cards_card_id_restore"};
        } catch (const holder::api::HistoryNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_projects_project_id_history_cards_card_id_restore"};
        } catch (const holder::api::HistoryConflict& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_projects_project_id_history_cards_card_id_restore"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "post_projects_project_id_history_cards_card_id_restore"};
        } catch (const holder::api::HistoryUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "post_projects_project_id_history_cards_card_id_restore"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_projects_project_id_history_cards_card_id_restore"};
        }
    }
    static const std::regex route_103("^/cards/([^/]+)/milestones\\-with\\-dates/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_103)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<holder::api::MilestoneWithDatesUpdateRequest> argument_2;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
            argument_2.emplace(decode_body<holder::api::MilestoneWithDatesUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "patch_cards_card_id_milestones_with_dates_milestone_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "patch_cards_card_id_milestones_with_dates_milestone_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::patch_cards_card_id_milestones_with_dates_milestone_id(context.db, context.card_store, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2))), "patch_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const holder::api::MilestoneWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "patch_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "patch_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "patch_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "patch_cards_card_id_milestones_with_dates_milestone_id"};
        }
    }
    static const std::regex route_104("^/cards/([^/]+)/milestones\\-with\\-dates/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_104)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_cards_card_id_milestones_with_dates_milestone_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_cards_card_id_milestones_with_dates_milestone_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_cards_card_id_milestones_with_dates_milestone_id(context.db, context.card_store, std::move(*argument_0), std::move(*argument_1))), "delete_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const holder::api::MilestoneWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "delete_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "delete_cards_card_id_milestones_with_dates_milestone_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_cards_card_id_milestones_with_dates_milestone_id"};
        }
    }
    static const std::regex route_106("^/cards/([^/]+)/milestones/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_106)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<holder::api::MilestoneUpdateRequest> argument_2;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
            argument_2.emplace(decode_body<holder::api::MilestoneUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "patch_cards_card_id_milestones_milestone_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "patch_cards_card_id_milestones_milestone_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::patch_cards_card_id_milestones_milestone_id(context.db, context.card_store, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2))), "patch_cards_card_id_milestones_milestone_id"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_cards_card_id_milestones_milestone_id"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "patch_cards_card_id_milestones_milestone_id"};
        } catch (const holder::api::MilestoneWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "patch_cards_card_id_milestones_milestone_id"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "patch_cards_card_id_milestones_milestone_id"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "patch_cards_card_id_milestones_milestone_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "patch_cards_card_id_milestones_milestone_id"};
        }
    }
    static const std::regex route_107("^/cards/([^/]+)/milestones/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_107)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_cards_card_id_milestones_milestone_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_cards_card_id_milestones_milestone_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_cards_card_id_milestones_milestone_id(context.db, context.card_store, std::move(*argument_0), std::move(*argument_1))), "delete_cards_card_id_milestones_milestone_id"};
        } catch (const holder::api::support::PrivacyRejected& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_cards_card_id_milestones_milestone_id"};
        } catch (const holder::api::MilestoneWriteNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_cards_card_id_milestones_milestone_id"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "delete_cards_card_id_milestones_milestone_id"};
        } catch (const holder::api::support::KeyringUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "delete_cards_card_id_milestones_milestone_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_cards_card_id_milestones_milestone_id"};
        }
    }
    static const std::regex route_45("^/resources/([^/]+)/assets/([^/]+)/content$");
    if (method == "GET" && std::regex_match(route_path, match, route_45)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<std::optional<std::string>> argument_2;
        try {
            const auto query_values = decode_query(query_string);
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
            const auto query_2 = query_values.find("placement_id");
            if (query_2 == query_values.end())
                argument_2.emplace(std::nullopt);
            else
                argument_2.emplace(std::string(query_2->second));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_resources_resource_id_assets_asset_id_content");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_resources_resource_id_assets_asset_id_content");
        }
        try {
            auto outcome = holder::api::get_resources_resource_id_assets_asset_id_content(context.db, context.secret_store, context.git_ops, context.uuid_v4, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2));
            if (outcome.status != 200)
                throw std::logic_error("get_resources_resource_id_assets_asset_id_content chose an undeclared success status");
            return response{outcome.status, json(), "get_resources_resource_id_assets_asset_id_content", {}, std::move(outcome.value)};
        } catch (const holder::api::support::StorageConflict& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "get_resources_resource_id_assets_asset_id_content"};
        } catch (const holder::api::support::StorageBindingRequired& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "get_resources_resource_id_assets_asset_id_content"};
        } catch (const holder::api::support::StorageIntegrityFailed& error) {
            return response{422, encode_error<holder::api::ErrorResponse>(422, error), "get_resources_resource_id_assets_asset_id_content"};
        } catch (const holder::api::support::StorageGatewayFailure& error) {
            return response{502, encode_error<holder::api::ErrorResponse>(502, error), "get_resources_resource_id_assets_asset_id_content"};
        } catch (const holder::api::support::StorageUnavailable& error) {
            return response{503, encode_error<holder::api::ErrorResponse>(503, error), "get_resources_resource_id_assets_asset_id_content"};
        } catch (const holder::api::support::StorageCapacityExceeded& error) {
            return response{507, encode_error<holder::api::ErrorResponse>(507, error), "get_resources_resource_id_assets_asset_id_content"};
        } catch (const holder::api::AssetContentNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_resources_resource_id_assets_asset_id_content"};
        } catch (const holder::api::support::StorageBadRequest& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_resources_resource_id_assets_asset_id_content"};
        } catch (const std::exception& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_resources_resource_id_assets_asset_id_content"};
        }
    }
    static const std::regex route_79("^/projects/([^/]+)/cards/([^/]+)/breadcrumbs$");
    if (method == "GET" && std::regex_match(route_path, match, route_79)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "get_projects_project_id_cards_card_id_breadcrumbs");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "get_projects_project_id_cards_card_id_breadcrumbs");
        }
        try {
            return response{200, rig_json::encode(holder::api::get_projects_project_id_cards_card_id_breadcrumbs(context.db, std::move(*argument_0), std::move(*argument_1))), "get_projects_project_id_cards_card_id_breadcrumbs"};
        } catch (const holder::api::ProjectCardsNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "get_projects_project_id_cards_card_id_breadcrumbs"};
        } catch (const holder::api::CardHierarchyInvalid& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "get_projects_project_id_cards_card_id_breadcrumbs"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_projects_project_id_cards_card_id_breadcrumbs"};
        }
    }
    static const std::regex route_129("^/projects/([^/]+)/trash/([^/]+)/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_129)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        std::optional<std::string> argument_2;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
            argument_2.emplace(decode_path_string(match[3].str()));
            enforce_string_values(*argument_1, {"card", "ai_message"}, "type");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_projects_project_id_trash_type_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_projects_project_id_trash_type_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_projects_project_id_trash_type_id(context.db, context.fts, context.card_store, std::move(*argument_0), std::move(*argument_1), std::move(*argument_2))), "delete_projects_project_id_trash_type_id"};
        } catch (const holder::api::TrashItemNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_projects_project_id_trash_type_id"};
        } catch (const holder::api::TrashItemNotTrashed& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "delete_projects_project_id_trash_type_id"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "delete_projects_project_id_trash_type_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_projects_project_id_trash_type_id"};
        }
    }
    static const std::regex route_128("^/trash/([^/]+)/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_128)) {
        std::optional<std::string> argument_0;
        std::optional<std::string> argument_1;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
            argument_1.emplace(decode_path_string(match[2].str()));
            enforce_string_values(*argument_0, {"card", "ai_message"}, "type");
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_trash_type_id");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_trash_type_id");
        }
        try {
            return response{200, rig_json::encode(holder::api::delete_trash_type_id(context.db, context.fts, context.card_store, std::move(*argument_0), std::move(*argument_1))), "delete_trash_type_id"};
        } catch (const holder::api::TrashItemNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "delete_trash_type_id"};
        } catch (const holder::api::TrashItemNotTrashed& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "delete_trash_type_id"};
        } catch (const holder::api::CardStoreUnavailable& error) {
            return response{501, encode_error<holder::api::ErrorResponse>(501, error), "delete_trash_type_id"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_trash_type_id"};
        }
    }
    std::set<std::string_view> allowed;
    static const std::regex path_0("^/cards$");
    if (std::regex_match(route_path, path_0)) allowed.insert({"GET", "POST"});
    static const std::regex path_1("^/alive$");
    if (std::regex_match(route_path, path_1)) allowed.insert({"GET"});
    static const std::regex path_2("^/bye$");
    if (std::regex_match(route_path, path_2)) allowed.insert({"POST"});
    static const std::regex path_3("^/health$");
    if (std::regex_match(route_path, path_3)) allowed.insert({"GET"});
    static const std::regex path_4("^/locations$");
    if (std::regex_match(route_path, path_4)) allowed.insert({"GET", "POST"});
    static const std::regex path_5("^/rebuild$");
    if (std::regex_match(route_path, path_5)) allowed.insert({"POST"});
    static const std::regex path_6("^/resources$");
    if (std::regex_match(route_path, path_6)) allowed.insert({"GET", "POST"});
    static const std::regex path_7("^/projects$");
    if (std::regex_match(route_path, path_7)) allowed.insert({"GET", "POST"});
    static const std::regex path_8("^/calendar$");
    if (std::regex_match(route_path, path_8)) allowed.insert({"GET"});
    static const std::regex path_9("^/ping$");
    if (std::regex_match(route_path, path_9)) allowed.insert({"GET"});
    static const std::regex path_10("^/openapi\\.yaml$");
    if (std::regex_match(route_path, path_10)) allowed.insert({"GET"});
    static const std::regex path_11("^/openapi\\.json$");
    if (std::regex_match(route_path, path_11)) allowed.insert({"GET"});
    static const std::regex path_12("^/ai_catalog\\.yaml$");
    if (std::regex_match(route_path, path_12)) allowed.insert({"GET"});
    static const std::regex path_13("^/git_providers\\.yaml$");
    if (std::regex_match(route_path, path_13)) allowed.insert({"GET"});
    static const std::regex path_14("^/ai_catalog\\.json$");
    if (std::regex_match(route_path, path_14)) allowed.insert({"GET"});
    static const std::regex path_15("^/git_providers\\.json$");
    if (std::regex_match(route_path, path_15)) allowed.insert({"GET"});
    static const std::regex path_16("^/docs$");
    if (std::regex_match(route_path, path_16)) allowed.insert({"GET"});
    static const std::regex path_17("^/trash$");
    if (std::regex_match(route_path, path_17)) allowed.insert({"DELETE", "GET"});
    static const std::regex path_18("^/imports$");
    if (std::regex_match(route_path, path_18)) allowed.insert({"POST"});
    static const std::regex path_19("^/reindex$");
    if (std::regex_match(route_path, path_19)) allowed.insert({"POST"});
    static const std::regex path_20("^/events$");
    if (std::regex_match(route_path, path_20)) allowed.insert({"GET"});
    static const std::regex path_21("^/ai/messages$");
    if (std::regex_match(route_path, path_21)) allowed.insert({"GET", "POST"});
    static const std::regex path_22("^/ai/nudges$");
    if (std::regex_match(route_path, path_22)) allowed.insert({"GET"});
    static const std::regex path_23("^/ai/runners$");
    if (std::regex_match(route_path, path_23)) allowed.insert({"GET", "POST"});
    static const std::regex path_24("^/ai/runs$");
    if (std::regex_match(route_path, path_24)) allowed.insert({"GET", "POST"});
    static const std::regex path_25("^/ai/capabilities$");
    if (std::regex_match(route_path, path_25)) allowed.insert({"GET"});
    static const std::regex path_26("^/ai/status$");
    if (std::regex_match(route_path, path_26)) allowed.insert({"GET"});
    static const std::regex path_27("^/ai/threads$");
    if (std::regex_match(route_path, path_27)) allowed.insert({"GET", "POST"});
    static const std::regex path_28("^/card\\-references/resolve$");
    if (std::regex_match(route_path, path_28)) allowed.insert({"POST"});
    static const std::regex path_29("^/events/cursor$");
    if (std::regex_match(route_path, path_29)) allowed.insert({"GET"});
    static const std::regex path_30("^/cards/context$");
    if (std::regex_match(route_path, path_30)) allowed.insert({"GET"});
    static const std::regex path_31("^/locations/preferred$");
    if (std::regex_match(route_path, path_31)) allowed.insert({"PUT"});
    static const std::regex path_32("^/recovery\\-token/import$");
    if (std::regex_match(route_path, path_32)) allowed.insert({"POST"});
    static const std::regex path_33("^/search/cards$");
    if (std::regex_match(route_path, path_33)) allowed.insert({"GET"});
    static const std::regex path_34("^/search/ai$");
    if (std::regex_match(route_path, path_34)) allowed.insert({"GET"});
    static const std::regex path_35("^/ai/runner/pull$");
    if (std::regex_match(route_path, path_35)) allowed.insert({"POST"});
    static const std::regex path_36("^/ai/providers/credentials$");
    if (std::regex_match(route_path, path_36)) allowed.insert({"GET", "PUT"});
    static const std::regex path_37("^/ai/local\\-models/config$");
    if (std::regex_match(route_path, path_37)) allowed.insert({"GET", "PUT"});
    static const std::regex path_38("^/ai/messages/capture$");
    if (std::regex_match(route_path, path_38)) allowed.insert({"POST"});
    static const std::regex path_39("^/ai/nudges/evaluate$");
    if (std::regex_match(route_path, path_39)) allowed.insert({"POST"});
    static const std::regex path_40("^/ai/providers/catalog$");
    if (std::regex_match(route_path, path_40)) allowed.insert({"GET"});
    static const std::regex path_41("^/ai/providers/settings$");
    if (std::regex_match(route_path, path_41)) allowed.insert({"GET", "PUT"});
    static const std::regex path_42("^/ai/runner/pull/([^/]+)$");
    if (std::regex_match(route_path, path_42)) allowed.insert({"GET"});
    static const std::regex path_43("^/ai/providers/credentials/([^/]+)$");
    if (std::regex_match(route_path, path_43)) allowed.insert({"DELETE"});
    static const std::regex path_44("^/ai/providers/settings/([^/]+)$");
    if (std::regex_match(route_path, path_44)) allowed.insert({"DELETE"});
    static const std::regex path_45("^/ai/runner/pull/([^/]+)/events$");
    if (std::regex_match(route_path, path_45)) allowed.insert({"GET"});
    static const std::regex path_46("^/ai/messages/([^/]+)$");
    if (std::regex_match(route_path, path_46)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_47("^/ai/runners/([^/]+)$");
    if (std::regex_match(route_path, path_47)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_48("^/ai/runs/([^/]+)$");
    if (std::regex_match(route_path, path_48)) allowed.insert({"GET"});
    static const std::regex path_49("^/ai/threads/([^/]+)$");
    if (std::regex_match(route_path, path_49)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_50("^/ai/messages/([^/]+)/links$");
    if (std::regex_match(route_path, path_50)) allowed.insert({"DELETE", "GET", "POST"});
    static const std::regex path_51("^/ai/messages/([^/]+)/backlinks$");
    if (std::regex_match(route_path, path_51)) allowed.insert({"GET"});
    static const std::regex path_52("^/ai/messages/([^/]+)/restore$");
    if (std::regex_match(route_path, path_52)) allowed.insert({"POST"});
    static const std::regex path_53("^/ai/nudges/([^/]+)/dismiss$");
    if (std::regex_match(route_path, path_53)) allowed.insert({"POST"});
    static const std::regex path_54("^/ai/runners/([^/]+)/retry$");
    if (std::regex_match(route_path, path_54)) allowed.insert({"POST"});
    static const std::regex path_55("^/ai/runs/([^/]+)/events$");
    if (std::regex_match(route_path, path_55)) allowed.insert({"GET"});
    static const std::regex path_56("^/cards/([^/]+)$");
    if (std::regex_match(route_path, path_56)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_57("^/imports/([^/]+)$");
    if (std::regex_match(route_path, path_57)) allowed.insert({"GET"});
    static const std::regex path_58("^/locations/([^/]+)$");
    if (std::regex_match(route_path, path_58)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_59("^/resources/([^/]+)$");
    if (std::regex_match(route_path, path_59)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_60("^/projects/([^/]+)$");
    if (std::regex_match(route_path, path_60)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_61("^/docs/([^/]+)$");
    if (std::regex_match(route_path, path_61)) allowed.insert({"GET"});
    static const std::regex path_62("^/cards/([^/]+)/move$");
    if (std::regex_match(route_path, path_62)) allowed.insert({"POST"});
    static const std::regex path_63("^/cards/([^/]+)/restore$");
    if (std::regex_match(route_path, path_63)) allowed.insert({"POST"});
    static const std::regex path_64("^/cards/([^/]+)/tags$");
    if (std::regex_match(route_path, path_64)) allowed.insert({"DELETE", "POST"});
    static const std::regex path_65("^/projects/([^/]+)/history$");
    if (std::regex_match(route_path, path_65)) allowed.insert({"GET"});
    static const std::regex path_66("^/projects/([^/]+)/tags$");
    if (std::regex_match(route_path, path_66)) allowed.insert({"GET"});
    static const std::regex path_67("^/projects/([^/]+)/encryption\\-check$");
    if (std::regex_match(route_path, path_67)) allowed.insert({"GET"});
    static const std::regex path_68("^/projects/([^/]+)/cards$");
    if (std::regex_match(route_path, path_68)) allowed.insert({"GET"});
    static const std::regex path_69("^/projects/([^/]+)/repair\\-hierarchy$");
    if (std::regex_match(route_path, path_69)) allowed.insert({"POST"});
    static const std::regex path_70("^/cards/([^/]+)/milestones$");
    if (std::regex_match(route_path, path_70)) allowed.insert({"GET", "POST"});
    static const std::regex path_71("^/cards/([^/]+)/resources$");
    if (std::regex_match(route_path, path_71)) allowed.insert({"DELETE", "POST"});
    static const std::regex path_72("^/cards/([^/]+)/milestones\\-with\\-dates$");
    if (std::regex_match(route_path, path_72)) allowed.insert({"GET", "POST"});
    static const std::regex path_73("^/projects/([^/]+)/milestones\\-with\\-dates$");
    if (std::regex_match(route_path, path_73)) allowed.insert({"GET"});
    static const std::regex path_74("^/locations/([^/]+)/binding$");
    if (std::regex_match(route_path, path_74)) allowed.insert({"DELETE", "PUT"});
    static const std::regex path_75("^/locations/([^/]+)/test$");
    if (std::regex_match(route_path, path_75)) allowed.insert({"POST"});
    static const std::regex path_76("^/cards/([^/]+)/links$");
    if (std::regex_match(route_path, path_76)) allowed.insert({"DELETE", "GET", "POST"});
    static const std::regex path_77("^/cards/([^/]+)/backlinks$");
    if (std::regex_match(route_path, path_77)) allowed.insert({"GET"});
    static const std::regex path_78("^/projects/([^/]+)/git/test\\-remote$");
    if (std::regex_match(route_path, path_78)) allowed.insert({"POST"});
    static const std::regex path_79("^/projects/([^/]+)/git/push$");
    if (std::regex_match(route_path, path_79)) allowed.insert({"POST"});
    static const std::regex path_80("^/projects/([^/]+)/git/pull$");
    if (std::regex_match(route_path, path_80)) allowed.insert({"POST"});
    static const std::regex path_81("^/projects/([^/]+)/git/sync$");
    if (std::regex_match(route_path, path_81)) allowed.insert({"POST"});
    static const std::regex path_82("^/projects/([^/]+)/git/sync\\-status$");
    if (std::regex_match(route_path, path_82)) allowed.insert({"GET"});
    static const std::regex path_83("^/projects/([^/]+)/recovery\\-token/export$");
    if (std::regex_match(route_path, path_83)) allowed.insert({"POST"});
    static const std::regex path_84("^/projects/([^/]+)/recovery\\-token/import$");
    if (std::regex_match(route_path, path_84)) allowed.insert({"POST"});
    static const std::regex path_85("^/locations/([^/]+)/oauth/google\\-drive/authorize$");
    if (std::regex_match(route_path, path_85)) allowed.insert({"POST"});
    static const std::regex path_86("^/locations/([^/]+)/oauth/google\\-drive/callback$");
    if (std::regex_match(route_path, path_86)) allowed.insert({"GET"});
    static const std::regex path_87("^/projects/([^/]+)/history/cards/([^/]+)$");
    if (std::regex_match(route_path, path_87)) allowed.insert({"GET"});
    static const std::regex path_88("^/projects/([^/]+)/history/cards/([^/]+)/compare$");
    if (std::regex_match(route_path, path_88)) allowed.insert({"GET"});
    static const std::regex path_89("^/projects/([^/]+)/history/cards/([^/]+)/snapshot$");
    if (std::regex_match(route_path, path_89)) allowed.insert({"GET"});
    static const std::regex path_90("^/projects/([^/]+)/history/cards/([^/]+)/restore$");
    if (std::regex_match(route_path, path_90)) allowed.insert({"POST"});
    static const std::regex path_91("^/cards/([^/]+)/milestones\\-with\\-dates/([^/]+)$");
    if (std::regex_match(route_path, path_91)) allowed.insert({"DELETE", "PATCH"});
    static const std::regex path_92("^/cards/([^/]+)/milestones/([^/]+)$");
    if (std::regex_match(route_path, path_92)) allowed.insert({"DELETE", "PATCH"});
    static const std::regex path_93("^/resources/([^/]+)/assets/([^/]+)/content$");
    if (std::regex_match(route_path, path_93)) allowed.insert({"GET"});
    static const std::regex path_94("^/projects/([^/]+)/cards/([^/]+)/breadcrumbs$");
    if (std::regex_match(route_path, path_94)) allowed.insert({"GET"});
    static const std::regex path_95("^/projects/([^/]+)/trash/([^/]+)/([^/]+)$");
    if (std::regex_match(route_path, path_95)) allowed.insert({"DELETE"});
    static const std::regex path_96("^/trash/([^/]+)/([^/]+)$");
    if (std::regex_match(route_path, path_96)) allowed.insert({"DELETE"});
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
    static const std::regex route_5("^/cards$");
    if (method == "POST" && std::regex_match(route_path, route_5)) return false;
    static const std::regex route_46("^/alive$");
    if (method == "GET" && std::regex_match(route_path, route_46)) return true;
    static const std::regex route_47("^/bye$");
    if (method == "POST" && std::regex_match(route_path, route_47)) return false;
    static const std::regex route_51("^/health$");
    if (method == "GET" && std::regex_match(route_path, route_51)) return false;
    static const std::regex route_57("^/locations$");
    if (method == "GET" && std::regex_match(route_path, route_57)) return false;
    static const std::regex route_63("^/rebuild$");
    if (method == "POST" && std::regex_match(route_path, route_63)) return false;
    static const std::regex route_64("^/resources$");
    if (method == "GET" && std::regex_match(route_path, route_64)) return false;
    static const std::regex route_66("^/resources$");
    if (method == "POST" && std::regex_match(route_path, route_66)) return false;
    static const std::regex route_73("^/projects$");
    if (method == "GET" && std::regex_match(route_path, route_73)) return false;
    static const std::regex route_77("^/projects$");
    if (method == "POST" && std::regex_match(route_path, route_77)) return false;
    static const std::regex route_83("^/calendar$");
    if (method == "GET" && std::regex_match(route_path, route_83)) return false;
    static const std::regex route_85("^/cards$");
    if (method == "GET" && std::regex_match(route_path, route_85)) return false;
    static const std::regex route_92("^/ping$");
    if (method == "GET" && std::regex_match(route_path, route_92)) return true;
    static const std::regex route_93("^/openapi\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_93)) return true;
    static const std::regex route_94("^/openapi\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_94)) return true;
    static const std::regex route_95("^/ai_catalog\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_95)) return true;
    static const std::regex route_96("^/git_providers\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_96)) return true;
    static const std::regex route_97("^/ai_catalog\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_97)) return true;
    static const std::regex route_98("^/git_providers\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_98)) return true;
    static const std::regex route_99("^/docs$");
    if (method == "GET" && std::regex_match(route_path, route_99)) return true;
    static const std::regex route_101("^/trash$");
    if (method == "GET" && std::regex_match(route_path, route_101)) return false;
    static const std::regex route_109("^/imports$");
    if (method == "POST" && std::regex_match(route_path, route_109)) return false;
    static const std::regex route_110("^/locations$");
    if (method == "POST" && std::regex_match(route_path, route_110)) return false;
    static const std::regex route_120("^/reindex$");
    if (method == "POST" && std::regex_match(route_path, route_120)) return false;
    static const std::regex route_126("^/events$");
    if (method == "GET" && std::regex_match(route_path, route_126)) return false;
    static const std::regex route_127("^/trash$");
    if (method == "DELETE" && std::regex_match(route_path, route_127)) return false;
    static const std::regex route_14("^/ai/messages$");
    if (method == "GET" && std::regex_match(route_path, route_14)) return false;
    static const std::regex route_18("^/ai/messages$");
    if (method == "POST" && std::regex_match(route_path, route_18)) return false;
    static const std::regex route_25("^/ai/nudges$");
    if (method == "GET" && std::regex_match(route_path, route_25)) return false;
    static const std::regex route_29("^/ai/runners$");
    if (method == "GET" && std::regex_match(route_path, route_29)) return false;
    static const std::regex route_31("^/ai/runners$");
    if (method == "POST" && std::regex_match(route_path, route_31)) return false;
    static const std::regex route_36("^/ai/runs$");
    if (method == "GET" && std::regex_match(route_path, route_36)) return false;
    static const std::regex route_38("^/ai/capabilities$");
    if (method == "GET" && std::regex_match(route_path, route_38)) return false;
    static const std::regex route_39("^/ai/status$");
    if (method == "GET" && std::regex_match(route_path, route_39)) return false;
    static const std::regex route_40("^/ai/threads$");
    if (method == "GET" && std::regex_match(route_path, route_40)) return false;
    static const std::regex route_42("^/ai/threads$");
    if (method == "POST" && std::regex_match(route_path, route_42)) return false;
    static const std::regex route_48("^/card\\-references/resolve$");
    if (method == "POST" && std::regex_match(route_path, route_48)) return false;
    static const std::regex route_49("^/events/cursor$");
    if (method == "GET" && std::regex_match(route_path, route_49)) return false;
    static const std::regex route_86("^/cards/context$");
    if (method == "GET" && std::regex_match(route_path, route_86)) return false;
    static const std::regex route_116("^/locations/preferred$");
    if (method == "PUT" && std::regex_match(route_path, route_116)) return false;
    static const std::regex route_117("^/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, route_117)) return false;
    static const std::regex route_121("^/search/cards$");
    if (method == "GET" && std::regex_match(route_path, route_121)) return false;
    static const std::regex route_122("^/search/ai$");
    if (method == "GET" && std::regex_match(route_path, route_122)) return false;
    static const std::regex route_123("^/ai/runs$");
    if (method == "POST" && std::regex_match(route_path, route_123)) return false;
    static const std::regex route_1("^/ai/runner/pull$");
    if (method == "POST" && std::regex_match(route_path, route_1)) return false;
    static const std::regex route_2("^/ai/providers/credentials$");
    if (method == "GET" && std::regex_match(route_path, route_2)) return false;
    static const std::regex route_3("^/ai/providers/credentials$");
    if (method == "PUT" && std::regex_match(route_path, route_3)) return false;
    static const std::regex route_12("^/ai/local\\-models/config$");
    if (method == "GET" && std::regex_match(route_path, route_12)) return false;
    static const std::regex route_13("^/ai/local\\-models/config$");
    if (method == "PUT" && std::regex_match(route_path, route_13)) return false;
    static const std::regex route_24("^/ai/messages/capture$");
    if (method == "POST" && std::regex_match(route_path, route_24)) return false;
    static const std::regex route_27("^/ai/nudges/evaluate$");
    if (method == "POST" && std::regex_match(route_path, route_27)) return false;
    static const std::regex route_28("^/ai/providers/catalog$");
    if (method == "GET" && std::regex_match(route_path, route_28)) return false;
    static const std::regex route_60("^/ai/providers/settings$");
    if (method == "GET" && std::regex_match(route_path, route_60)) return false;
    static const std::regex route_61("^/ai/providers/settings$");
    if (method == "PUT" && std::regex_match(route_path, route_61)) return false;
    static const std::regex route_0("^/ai/runner/pull/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_0)) return false;
    static const std::regex route_4("^/ai/providers/credentials/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_4)) return false;
    static const std::regex route_62("^/ai/providers/settings/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_62)) return false;
    static const std::regex route_125("^/ai/runner/pull/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, route_125)) return false;
    static const std::regex route_15("^/ai/messages/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_15)) return false;
    static const std::regex route_19("^/ai/messages/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_19)) return false;
    static const std::regex route_20("^/ai/messages/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_20)) return false;
    static const std::regex route_30("^/ai/runners/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_30)) return false;
    static const std::regex route_32("^/ai/runners/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_32)) return false;
    static const std::regex route_33("^/ai/runners/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_33)) return false;
    static const std::regex route_37("^/ai/runs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_37)) return false;
    static const std::regex route_41("^/ai/threads/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_41)) return false;
    static const std::regex route_43("^/ai/threads/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_43)) return false;
    static const std::regex route_44("^/ai/threads/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_44)) return false;
    static const std::regex route_16("^/ai/messages/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, route_16)) return false;
    static const std::regex route_17("^/ai/messages/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, route_17)) return false;
    static const std::regex route_21("^/ai/messages/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, route_21)) return false;
    static const std::regex route_22("^/ai/messages/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, route_22)) return false;
    static const std::regex route_23("^/ai/messages/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, route_23)) return false;
    static const std::regex route_26("^/ai/nudges/([^/]+)/dismiss$");
    if (method == "POST" && std::regex_match(route_path, route_26)) return false;
    static const std::regex route_34("^/ai/runners/([^/]+)/retry$");
    if (method == "POST" && std::regex_match(route_path, route_34)) return false;
    static const std::regex route_124("^/ai/runs/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, route_124)) return false;
    static const std::regex route_6("^/cards/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_6)) return false;
    static const std::regex route_7("^/cards/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_7)) return false;
    static const std::regex route_35("^/imports/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_35)) return false;
    static const std::regex route_58("^/locations/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_58)) return false;
    static const std::regex route_65("^/resources/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_65)) return false;
    static const std::regex route_67("^/resources/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_67)) return false;
    static const std::regex route_68("^/resources/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_68)) return false;
    static const std::regex route_74("^/projects/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_74)) return false;
    static const std::regex route_81("^/projects/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_81)) return false;
    static const std::regex route_82("^/projects/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_82)) return false;
    static const std::regex route_87("^/cards/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_87)) return false;
    static const std::regex route_100("^/docs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_100)) return true;
    static const std::regex route_111("^/locations/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_111)) return false;
    static const std::regex route_112("^/locations/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_112)) return false;
    static const std::regex route_8("^/cards/([^/]+)/move$");
    if (method == "POST" && std::regex_match(route_path, route_8)) return false;
    static const std::regex route_9("^/cards/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, route_9)) return false;
    static const std::regex route_10("^/cards/([^/]+)/tags$");
    if (method == "POST" && std::regex_match(route_path, route_10)) return false;
    static const std::regex route_11("^/cards/([^/]+)/tags$");
    if (method == "DELETE" && std::regex_match(route_path, route_11)) return false;
    static const std::regex route_52("^/projects/([^/]+)/history$");
    if (method == "GET" && std::regex_match(route_path, route_52)) return false;
    static const std::regex route_59("^/projects/([^/]+)/tags$");
    if (method == "GET" && std::regex_match(route_path, route_59)) return false;
    static const std::regex route_76("^/projects/([^/]+)/encryption\\-check$");
    if (method == "GET" && std::regex_match(route_path, route_76)) return false;
    static const std::regex route_78("^/projects/([^/]+)/cards$");
    if (method == "GET" && std::regex_match(route_path, route_78)) return false;
    static const std::regex route_80("^/projects/([^/]+)/repair\\-hierarchy$");
    if (method == "POST" && std::regex_match(route_path, route_80)) return false;
    static const std::regex route_84("^/cards/([^/]+)/milestones$");
    if (method == "GET" && std::regex_match(route_path, route_84)) return false;
    static const std::regex route_88("^/cards/([^/]+)/resources$");
    if (method == "POST" && std::regex_match(route_path, route_88)) return false;
    static const std::regex route_89("^/cards/([^/]+)/resources$");
    if (method == "DELETE" && std::regex_match(route_path, route_89)) return false;
    static const std::regex route_90("^/cards/([^/]+)/milestones\\-with\\-dates$");
    if (method == "GET" && std::regex_match(route_path, route_90)) return false;
    static const std::regex route_91("^/projects/([^/]+)/milestones\\-with\\-dates$");
    if (method == "GET" && std::regex_match(route_path, route_91)) return false;
    static const std::regex route_102("^/cards/([^/]+)/milestones\\-with\\-dates$");
    if (method == "POST" && std::regex_match(route_path, route_102)) return false;
    static const std::regex route_105("^/cards/([^/]+)/milestones$");
    if (method == "POST" && std::regex_match(route_path, route_105)) return false;
    static const std::regex route_113("^/locations/([^/]+)/binding$");
    if (method == "PUT" && std::regex_match(route_path, route_113)) return false;
    static const std::regex route_114("^/locations/([^/]+)/binding$");
    if (method == "DELETE" && std::regex_match(route_path, route_114)) return false;
    static const std::regex route_115("^/locations/([^/]+)/test$");
    if (method == "POST" && std::regex_match(route_path, route_115)) return false;
    static const std::regex route_130("^/cards/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, route_130)) return false;
    static const std::regex route_131("^/cards/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, route_131)) return false;
    static const std::regex route_132("^/cards/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, route_132)) return false;
    static const std::regex route_133("^/cards/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, route_133)) return false;
    static const std::regex route_69("^/projects/([^/]+)/git/test\\-remote$");
    if (method == "POST" && std::regex_match(route_path, route_69)) return false;
    static const std::regex route_70("^/projects/([^/]+)/git/push$");
    if (method == "POST" && std::regex_match(route_path, route_70)) return false;
    static const std::regex route_71("^/projects/([^/]+)/git/pull$");
    if (method == "POST" && std::regex_match(route_path, route_71)) return false;
    static const std::regex route_72("^/projects/([^/]+)/git/sync$");
    if (method == "POST" && std::regex_match(route_path, route_72)) return false;
    static const std::regex route_75("^/projects/([^/]+)/git/sync\\-status$");
    if (method == "GET" && std::regex_match(route_path, route_75)) return false;
    static const std::regex route_118("^/projects/([^/]+)/recovery\\-token/export$");
    if (method == "POST" && std::regex_match(route_path, route_118)) return false;
    static const std::regex route_119("^/projects/([^/]+)/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, route_119)) return false;
    static const std::regex route_50("^/locations/([^/]+)/oauth/google\\-drive/authorize$");
    if (method == "POST" && std::regex_match(route_path, route_50)) return false;
    static const std::regex route_108("^/locations/([^/]+)/oauth/google\\-drive/callback$");
    if (method == "GET" && std::regex_match(route_path, route_108)) return true;
    return false;
}
// Whether the operation dispatch would select for this request is a RIG_HOST_RESPONSE
// operation, so the hosting server serves only the routes the contract declares.
inline bool is_host_served(const std::string& method, const std::string& path) {
    const auto route_path = path.substr(0, path.find('?'));
    static const std::regex route_5("^/cards$");
    if (method == "POST" && std::regex_match(route_path, route_5)) return false;
    static const std::regex route_46("^/alive$");
    if (method == "GET" && std::regex_match(route_path, route_46)) return false;
    static const std::regex route_47("^/bye$");
    if (method == "POST" && std::regex_match(route_path, route_47)) return false;
    static const std::regex route_51("^/health$");
    if (method == "GET" && std::regex_match(route_path, route_51)) return false;
    static const std::regex route_57("^/locations$");
    if (method == "GET" && std::regex_match(route_path, route_57)) return false;
    static const std::regex route_63("^/rebuild$");
    if (method == "POST" && std::regex_match(route_path, route_63)) return false;
    static const std::regex route_64("^/resources$");
    if (method == "GET" && std::regex_match(route_path, route_64)) return false;
    static const std::regex route_66("^/resources$");
    if (method == "POST" && std::regex_match(route_path, route_66)) return false;
    static const std::regex route_73("^/projects$");
    if (method == "GET" && std::regex_match(route_path, route_73)) return false;
    static const std::regex route_77("^/projects$");
    if (method == "POST" && std::regex_match(route_path, route_77)) return false;
    static const std::regex route_83("^/calendar$");
    if (method == "GET" && std::regex_match(route_path, route_83)) return false;
    static const std::regex route_85("^/cards$");
    if (method == "GET" && std::regex_match(route_path, route_85)) return false;
    static const std::regex route_92("^/ping$");
    if (method == "GET" && std::regex_match(route_path, route_92)) return false;
    static const std::regex route_93("^/openapi\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_93)) return true;
    static const std::regex route_94("^/openapi\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_94)) return true;
    static const std::regex route_95("^/ai_catalog\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_95)) return true;
    static const std::regex route_96("^/git_providers\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, route_96)) return true;
    static const std::regex route_97("^/ai_catalog\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_97)) return true;
    static const std::regex route_98("^/git_providers\\.json$");
    if (method == "GET" && std::regex_match(route_path, route_98)) return true;
    static const std::regex route_99("^/docs$");
    if (method == "GET" && std::regex_match(route_path, route_99)) return true;
    static const std::regex route_101("^/trash$");
    if (method == "GET" && std::regex_match(route_path, route_101)) return false;
    static const std::regex route_109("^/imports$");
    if (method == "POST" && std::regex_match(route_path, route_109)) return false;
    static const std::regex route_110("^/locations$");
    if (method == "POST" && std::regex_match(route_path, route_110)) return false;
    static const std::regex route_120("^/reindex$");
    if (method == "POST" && std::regex_match(route_path, route_120)) return false;
    static const std::regex route_126("^/events$");
    if (method == "GET" && std::regex_match(route_path, route_126)) return false;
    static const std::regex route_127("^/trash$");
    if (method == "DELETE" && std::regex_match(route_path, route_127)) return false;
    static const std::regex route_14("^/ai/messages$");
    if (method == "GET" && std::regex_match(route_path, route_14)) return false;
    static const std::regex route_18("^/ai/messages$");
    if (method == "POST" && std::regex_match(route_path, route_18)) return false;
    static const std::regex route_25("^/ai/nudges$");
    if (method == "GET" && std::regex_match(route_path, route_25)) return false;
    static const std::regex route_29("^/ai/runners$");
    if (method == "GET" && std::regex_match(route_path, route_29)) return false;
    static const std::regex route_31("^/ai/runners$");
    if (method == "POST" && std::regex_match(route_path, route_31)) return false;
    static const std::regex route_36("^/ai/runs$");
    if (method == "GET" && std::regex_match(route_path, route_36)) return false;
    static const std::regex route_38("^/ai/capabilities$");
    if (method == "GET" && std::regex_match(route_path, route_38)) return false;
    static const std::regex route_39("^/ai/status$");
    if (method == "GET" && std::regex_match(route_path, route_39)) return false;
    static const std::regex route_40("^/ai/threads$");
    if (method == "GET" && std::regex_match(route_path, route_40)) return false;
    static const std::regex route_42("^/ai/threads$");
    if (method == "POST" && std::regex_match(route_path, route_42)) return false;
    static const std::regex route_48("^/card\\-references/resolve$");
    if (method == "POST" && std::regex_match(route_path, route_48)) return false;
    static const std::regex route_49("^/events/cursor$");
    if (method == "GET" && std::regex_match(route_path, route_49)) return false;
    static const std::regex route_86("^/cards/context$");
    if (method == "GET" && std::regex_match(route_path, route_86)) return false;
    static const std::regex route_116("^/locations/preferred$");
    if (method == "PUT" && std::regex_match(route_path, route_116)) return false;
    static const std::regex route_117("^/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, route_117)) return false;
    static const std::regex route_121("^/search/cards$");
    if (method == "GET" && std::regex_match(route_path, route_121)) return false;
    static const std::regex route_122("^/search/ai$");
    if (method == "GET" && std::regex_match(route_path, route_122)) return false;
    static const std::regex route_123("^/ai/runs$");
    if (method == "POST" && std::regex_match(route_path, route_123)) return false;
    static const std::regex route_1("^/ai/runner/pull$");
    if (method == "POST" && std::regex_match(route_path, route_1)) return false;
    static const std::regex route_2("^/ai/providers/credentials$");
    if (method == "GET" && std::regex_match(route_path, route_2)) return false;
    static const std::regex route_3("^/ai/providers/credentials$");
    if (method == "PUT" && std::regex_match(route_path, route_3)) return false;
    static const std::regex route_12("^/ai/local\\-models/config$");
    if (method == "GET" && std::regex_match(route_path, route_12)) return false;
    static const std::regex route_13("^/ai/local\\-models/config$");
    if (method == "PUT" && std::regex_match(route_path, route_13)) return false;
    static const std::regex route_24("^/ai/messages/capture$");
    if (method == "POST" && std::regex_match(route_path, route_24)) return false;
    static const std::regex route_27("^/ai/nudges/evaluate$");
    if (method == "POST" && std::regex_match(route_path, route_27)) return false;
    static const std::regex route_28("^/ai/providers/catalog$");
    if (method == "GET" && std::regex_match(route_path, route_28)) return false;
    static const std::regex route_60("^/ai/providers/settings$");
    if (method == "GET" && std::regex_match(route_path, route_60)) return false;
    static const std::regex route_61("^/ai/providers/settings$");
    if (method == "PUT" && std::regex_match(route_path, route_61)) return false;
    static const std::regex route_0("^/ai/runner/pull/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_0)) return false;
    static const std::regex route_4("^/ai/providers/credentials/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_4)) return false;
    static const std::regex route_62("^/ai/providers/settings/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_62)) return false;
    static const std::regex route_125("^/ai/runner/pull/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, route_125)) return false;
    static const std::regex route_15("^/ai/messages/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_15)) return false;
    static const std::regex route_19("^/ai/messages/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_19)) return false;
    static const std::regex route_20("^/ai/messages/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_20)) return false;
    static const std::regex route_30("^/ai/runners/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_30)) return false;
    static const std::regex route_32("^/ai/runners/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_32)) return false;
    static const std::regex route_33("^/ai/runners/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_33)) return false;
    static const std::regex route_37("^/ai/runs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_37)) return false;
    static const std::regex route_41("^/ai/threads/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_41)) return false;
    static const std::regex route_43("^/ai/threads/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_43)) return false;
    static const std::regex route_44("^/ai/threads/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_44)) return false;
    static const std::regex route_16("^/ai/messages/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, route_16)) return false;
    static const std::regex route_17("^/ai/messages/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, route_17)) return false;
    static const std::regex route_21("^/ai/messages/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, route_21)) return false;
    static const std::regex route_22("^/ai/messages/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, route_22)) return false;
    static const std::regex route_23("^/ai/messages/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, route_23)) return false;
    static const std::regex route_26("^/ai/nudges/([^/]+)/dismiss$");
    if (method == "POST" && std::regex_match(route_path, route_26)) return false;
    static const std::regex route_34("^/ai/runners/([^/]+)/retry$");
    if (method == "POST" && std::regex_match(route_path, route_34)) return false;
    static const std::regex route_124("^/ai/runs/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, route_124)) return false;
    static const std::regex route_6("^/cards/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_6)) return false;
    static const std::regex route_7("^/cards/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_7)) return false;
    static const std::regex route_35("^/imports/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_35)) return false;
    static const std::regex route_58("^/locations/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_58)) return false;
    static const std::regex route_65("^/resources/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_65)) return false;
    static const std::regex route_67("^/resources/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_67)) return false;
    static const std::regex route_68("^/resources/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_68)) return false;
    static const std::regex route_74("^/projects/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_74)) return false;
    static const std::regex route_81("^/projects/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_81)) return false;
    static const std::regex route_82("^/projects/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_82)) return false;
    static const std::regex route_87("^/cards/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_87)) return false;
    static const std::regex route_100("^/docs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, route_100)) return true;
    static const std::regex route_111("^/locations/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, route_111)) return false;
    static const std::regex route_112("^/locations/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, route_112)) return false;
    static const std::regex route_8("^/cards/([^/]+)/move$");
    if (method == "POST" && std::regex_match(route_path, route_8)) return false;
    static const std::regex route_9("^/cards/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, route_9)) return false;
    static const std::regex route_10("^/cards/([^/]+)/tags$");
    if (method == "POST" && std::regex_match(route_path, route_10)) return false;
    static const std::regex route_11("^/cards/([^/]+)/tags$");
    if (method == "DELETE" && std::regex_match(route_path, route_11)) return false;
    static const std::regex route_52("^/projects/([^/]+)/history$");
    if (method == "GET" && std::regex_match(route_path, route_52)) return false;
    static const std::regex route_59("^/projects/([^/]+)/tags$");
    if (method == "GET" && std::regex_match(route_path, route_59)) return false;
    static const std::regex route_76("^/projects/([^/]+)/encryption\\-check$");
    if (method == "GET" && std::regex_match(route_path, route_76)) return false;
    static const std::regex route_78("^/projects/([^/]+)/cards$");
    if (method == "GET" && std::regex_match(route_path, route_78)) return false;
    static const std::regex route_80("^/projects/([^/]+)/repair\\-hierarchy$");
    if (method == "POST" && std::regex_match(route_path, route_80)) return false;
    static const std::regex route_84("^/cards/([^/]+)/milestones$");
    if (method == "GET" && std::regex_match(route_path, route_84)) return false;
    static const std::regex route_88("^/cards/([^/]+)/resources$");
    if (method == "POST" && std::regex_match(route_path, route_88)) return false;
    static const std::regex route_89("^/cards/([^/]+)/resources$");
    if (method == "DELETE" && std::regex_match(route_path, route_89)) return false;
    static const std::regex route_90("^/cards/([^/]+)/milestones\\-with\\-dates$");
    if (method == "GET" && std::regex_match(route_path, route_90)) return false;
    static const std::regex route_91("^/projects/([^/]+)/milestones\\-with\\-dates$");
    if (method == "GET" && std::regex_match(route_path, route_91)) return false;
    static const std::regex route_102("^/cards/([^/]+)/milestones\\-with\\-dates$");
    if (method == "POST" && std::regex_match(route_path, route_102)) return false;
    static const std::regex route_105("^/cards/([^/]+)/milestones$");
    if (method == "POST" && std::regex_match(route_path, route_105)) return false;
    static const std::regex route_113("^/locations/([^/]+)/binding$");
    if (method == "PUT" && std::regex_match(route_path, route_113)) return false;
    static const std::regex route_114("^/locations/([^/]+)/binding$");
    if (method == "DELETE" && std::regex_match(route_path, route_114)) return false;
    static const std::regex route_115("^/locations/([^/]+)/test$");
    if (method == "POST" && std::regex_match(route_path, route_115)) return false;
    static const std::regex route_130("^/cards/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, route_130)) return false;
    static const std::regex route_131("^/cards/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, route_131)) return false;
    static const std::regex route_132("^/cards/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, route_132)) return false;
    static const std::regex route_133("^/cards/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, route_133)) return false;
    static const std::regex route_69("^/projects/([^/]+)/git/test\\-remote$");
    if (method == "POST" && std::regex_match(route_path, route_69)) return false;
    static const std::regex route_70("^/projects/([^/]+)/git/push$");
    if (method == "POST" && std::regex_match(route_path, route_70)) return false;
    static const std::regex route_71("^/projects/([^/]+)/git/pull$");
    if (method == "POST" && std::regex_match(route_path, route_71)) return false;
    static const std::regex route_72("^/projects/([^/]+)/git/sync$");
    if (method == "POST" && std::regex_match(route_path, route_72)) return false;
    static const std::regex route_75("^/projects/([^/]+)/git/sync\\-status$");
    if (method == "GET" && std::regex_match(route_path, route_75)) return false;
    static const std::regex route_118("^/projects/([^/]+)/recovery\\-token/export$");
    if (method == "POST" && std::regex_match(route_path, route_118)) return false;
    static const std::regex route_119("^/projects/([^/]+)/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, route_119)) return false;
    static const std::regex route_50("^/locations/([^/]+)/oauth/google\\-drive/authorize$");
    if (method == "POST" && std::regex_match(route_path, route_50)) return false;
    static const std::regex route_108("^/locations/([^/]+)/oauth/google\\-drive/callback$");
    if (method == "GET" && std::regex_match(route_path, route_108)) return true;
    return false;
}
} // namespace rig_http
