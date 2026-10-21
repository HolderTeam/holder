// Generated from RIG IR; do not edit.
#pragma once
#include "api/generated/RuntimeJson.h"
#include <charconv>
#include <exception>
#include <initializer_list>
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
inline bool decode_query_bool(std::string_view input) {
    if (input == "true") return true;
    if (input == "false") return false;
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
    static const std::regex route_0("^/alive$");
    if (method == "GET" && std::regex_match(route_path, match, route_0)) {
        try {
            return response{200, rig_json::encode(holder::api::check_alive()), "check_alive"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "check_alive"};
        }
    }
    static const std::regex route_1("^/bye$");
    if (method == "POST" && std::regex_match(route_path, match, route_1)) {
        try {
            return response{200, rig_json::encode(holder::api::say_goodbye()), "say_goodbye"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "say_goodbye"};
        }
    }
    static const std::regex route_9("^/rebuild$");
    if (method == "POST" && std::regex_match(route_path, match, route_9)) {
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
    static const std::regex route_2("^/card\\-references/resolve$");
    if (method == "POST" && std::regex_match(route_path, match, route_2)) {
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
    static const std::regex route_4("^/ai/providers/credentials$");
    if (method == "GET" && std::regex_match(route_path, match, route_4)) {
        try {
            return response{200, rig_json::encode(holder::api::list_provider_credentials(context.db)), "list_provider_credentials"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_provider_credentials"};
        }
    }
    static const std::regex route_5("^/ai/providers/credentials$");
    if (method == "PUT" && std::regex_match(route_path, match, route_5)) {
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
    static const std::regex route_6("^/ai/providers/settings$");
    if (method == "GET" && std::regex_match(route_path, match, route_6)) {
        try {
            return response{200, rig_json::encode(holder::api::list_provider_settings(context.db)), "list_provider_settings"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_provider_settings"};
        }
    }
    static const std::regex route_7("^/ai/providers/settings$");
    if (method == "PUT" && std::regex_match(route_path, match, route_7)) {
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
    static const std::regex route_8("^/ai/providers/settings/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_8)) {
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
    static const std::regex route_10("^/ai/runner/pull/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_10)) {
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
    static const std::regex route_3("^/projects/([^/]+)/tags$");
    if (method == "GET" && std::regex_match(route_path, match, route_3)) {
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
    static const std::regex route_11("^/cards/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, match, route_11)) {
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
    return std::nullopt;
}
// Whether the operation dispatch would select for this request is RIG_PUBLIC,
// so the hosting server can skip authentication for it.
inline bool is_public(const std::string& method, const std::string& path) {
    const auto route_path = path.substr(0, path.find('?'));
    static const std::regex route_0("^/alive$");
    if (method == "GET" && std::regex_match(route_path, route_0)) return true;
    return false;
}
} // namespace rig_http
