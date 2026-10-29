// Generated from RIG IR; do not edit.
#pragma once
#include "api/generated/AiJson.h"
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

namespace ai_http {
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
// The error passed to api_error_body for a 405.
struct method_not_allowed : std::runtime_error {
    using std::runtime_error::runtime_error;
};
// The application supplies api_error_body, found by argument-dependent
// lookup, to build each declared error body from an exception.
template <typename Body, typename Error>
json encode_error(int status, const Error& error) {
    const Body body = api_error_body(std::type_identity<Body>{}, status, error);
    return ai_json::encode(body);
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
        return ai_json::decode<T>(json::parse(body));
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
            return response{200, ai_json::encode(holder::api::get_ai_messages(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "get_ai_messages"};
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
            return response{201, ai_json::encode(holder::api::post_ai_messages(context.db, context.fts, context.uuid_v4, std::move(*argument_0))), "post_ai_messages"};
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
            return response{200, ai_json::encode(holder::api::get_ai_nudges(context.db, context.registry, std::move(*argument_0), std::move(*argument_1))), "get_ai_nudges"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_nudges"};
        }
    }
    static const std::regex route_22("^/ai/runners$");
    if (method == "GET" && std::regex_match(route_path, match, route_22)) {
        try {
            return response{200, ai_json::encode(holder::api::get_ai_runners(context.registry)), "get_ai_runners"};
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
            return response{201, ai_json::encode(holder::api::post_ai_runners(context.db, context.registry, context.uuid_v4, std::move(*argument_0))), "post_ai_runners"};
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
            return response{200, ai_json::encode(holder::api::get_ai_runs(context.db, std::move(*argument_0), std::move(*argument_1))), "get_ai_runs"};
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
            return response{200, ai_json::encode(holder::api::get_ai_capabilities(context.db, context.registry, std::move(*argument_0))), "get_ai_capabilities"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_capabilities"};
        }
    }
    static const std::regex route_31("^/ai/status$");
    if (method == "GET" && std::regex_match(route_path, match, route_31)) {
        try {
            return response{200, ai_json::encode(holder::api::get_ai_status(context.db, context.registry)), "get_ai_status"};
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
            return response{200, ai_json::encode(holder::api::get_ai_threads(context.db, std::move(*argument_0))), "get_ai_threads"};
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
            return response{201, ai_json::encode(holder::api::post_ai_threads(context.db, context.uuid_v4, std::move(*argument_0))), "post_ai_threads"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "post_ai_threads"};
        } catch (const holder::model::ConflictError& error) {
            return response{409, encode_error<holder::api::ErrorResponse>(409, error), "post_ai_threads"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_threads"};
        }
    }
    static const std::regex route_40("^/ai/runs$");
    if (method == "POST" && std::regex_match(route_path, match, route_40)) {
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
            return response{200, ai_json::encode(holder::api::post_ai_runner_pull(context.registry, std::move(*argument_0))), "post_ai_runner_pull"};
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
            return response{200, ai_json::encode(holder::api::list_provider_credentials(context.db)), "list_provider_credentials"};
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
            return response{200, ai_json::encode(holder::api::upsert_provider_credential(context.db, context.secret_store, std::move(*argument_0))), "upsert_provider_credential"};
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
            return response{200, ai_json::encode(holder::api::get_local_model_config(context.db)), "get_local_model_config"};
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
            return response{200, ai_json::encode(holder::api::put_local_model_config(context.db, std::move(*argument_0))), "put_local_model_config"};
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
            return response{201, ai_json::encode(holder::api::post_ai_messages_capture(context.db, context.fts, context.uuid_v4, std::move(*argument_0))), "post_ai_messages_capture"};
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
            return response{200, ai_json::encode(holder::api::post_ai_nudges_evaluate(context.db, context.registry, std::move(*argument_0))), "post_ai_nudges_evaluate"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_nudges_evaluate"};
        }
    }
    static const std::regex route_21("^/ai/providers/catalog$");
    if (method == "GET" && std::regex_match(route_path, match, route_21)) {
        try {
            return response{200, ai_json::encode(holder::api::get_ai_provider_catalog(context.db)), "get_ai_provider_catalog"};
        } catch (const holder::api::AiProviderCatalogMissing& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "get_ai_provider_catalog"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "get_ai_provider_catalog"};
        }
    }
    static const std::regex route_37("^/ai/providers/settings$");
    if (method == "GET" && std::regex_match(route_path, match, route_37)) {
        try {
            return response{200, ai_json::encode(holder::api::list_provider_settings(context.db)), "list_provider_settings"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "list_provider_settings"};
        }
    }
    static const std::regex route_38("^/ai/providers/settings$");
    if (method == "PUT" && std::regex_match(route_path, match, route_38)) {
        std::optional<holder::api::AiProviderSettingUpdateRequest> argument_0;
        try {
            argument_0.emplace(decode_body<holder::api::AiProviderSettingUpdateRequest>(body));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "set_provider_setting");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "set_provider_setting");
        }
        try {
            return response{200, ai_json::encode(holder::api::set_provider_setting(context.db, std::move(*argument_0))), "set_provider_setting"};
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
            return response{200, ai_json::encode(holder::api::get_pull_status(context.registry, std::move(*argument_0), std::move(*argument_1))), "get_pull_status"};
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
            return response{200, ai_json::encode(holder::api::delete_provider_credential(context.db, context.secret_store, std::move(*argument_0))), "delete_provider_credential"};
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
    static const std::regex route_39("^/ai/providers/settings/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_39)) {
        std::optional<std::string> argument_0;
        try {
            argument_0.emplace(decode_path_string(match[1].str()));
        } catch (const std::invalid_argument& error) {
            return invalid_request(error, "delete_provider_setting");
        } catch (const std::out_of_range& error) {
            return invalid_request(error, "delete_provider_setting");
        }
        try {
            return response{200, ai_json::encode(holder::api::delete_provider_setting(context.db, std::move(*argument_0))), "delete_provider_setting"};
        } catch (const std::invalid_argument& error) {
            return response{400, encode_error<holder::api::ErrorResponse>(400, error), "delete_provider_setting"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "delete_provider_setting"};
        }
    }
    static const std::regex route_42("^/ai/runner/pull/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_42)) {
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
            return response{200, ai_json::encode(holder::api::get_ai_message(context.db, context.fts, std::move(*argument_0))), "get_ai_message"};
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
            return response{200, ai_json::encode(holder::api::patch_ai_messages_message_id(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "patch_ai_messages_message_id"};
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
            return response{200, ai_json::encode(holder::api::delete_ai_messages_message_id(context.db, context.fts, std::move(*argument_0))), "delete_ai_messages_message_id"};
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
            return response{200, ai_json::encode(holder::api::get_ai_runner(context.registry, std::move(*argument_0))), "get_ai_runner"};
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
            return response{200, ai_json::encode(holder::api::patch_ai_runners_runner_id(context.db, context.registry, std::move(*argument_0), std::move(*argument_1))), "patch_ai_runners_runner_id"};
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
            return response{200, ai_json::encode(holder::api::delete_ai_runners_runner_id(context.db, context.registry, std::move(*argument_0))), "delete_ai_runners_runner_id"};
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
            return response{200, ai_json::encode(holder::api::get_ai_run(context.db, std::move(*argument_0))), "get_ai_run"};
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
            return response{200, ai_json::encode(holder::api::get_ai_thread(context.db, std::move(*argument_0))), "get_ai_thread"};
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
            return response{200, ai_json::encode(holder::api::patch_ai_threads_thread_id(context.db, std::move(*argument_0), std::move(*argument_1))), "patch_ai_threads_thread_id"};
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
            return response{200, ai_json::encode(holder::api::delete_ai_threads_thread_id(context.db, std::move(*argument_0))), "delete_ai_threads_thread_id"};
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
            return response{200, ai_json::encode(holder::api::get_ai_message_links(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "get_ai_message_links"};
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
            return response{200, ai_json::encode(holder::api::get_ai_message_backlinks(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "get_ai_message_backlinks"};
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
            return response{200, ai_json::encode(holder::api::post_ai_messages_message_id_restore(context.db, context.fts, std::move(*argument_0))), "post_ai_messages_message_id_restore"};
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
            return response{201, ai_json::encode(holder::api::post_ai_messages_message_id_links(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "post_ai_messages_message_id_links"};
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
            return response{200, ai_json::encode(holder::api::delete_ai_messages_message_id_links(context.db, context.fts, std::move(*argument_0), std::move(*argument_1))), "delete_ai_messages_message_id_links"};
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
            return response{200, ai_json::encode(holder::api::post_ai_nudges_nudge_id_dismiss(context.db, context.registry, std::move(*argument_0))), "post_ai_nudges_nudge_id_dismiss"};
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
            return response{200, ai_json::encode(holder::api::post_ai_runners_runner_id_retry(context.registry, std::move(*argument_0))), "post_ai_runners_runner_id_retry"};
        } catch (const holder::api::AiRunnerNotFound& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_ai_runners_runner_id_retry"};
        } catch (const holder::api::AiRunnerNotConfigured& error) {
            return response{404, encode_error<holder::api::ErrorResponse>(404, error), "post_ai_runners_runner_id_retry"};
        } catch (const std::exception& error) {
            return response{500, encode_error<holder::api::ErrorResponse>(500, error), "post_ai_runners_runner_id_retry"};
        }
    }
    static const std::regex route_41("^/ai/runs/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_41)) {
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
    std::set<std::string_view> allowed;
    static const std::regex path_0("^/ai/messages$");
    if (std::regex_match(route_path, path_0)) allowed.insert({"GET", "POST"});
    static const std::regex path_1("^/ai/nudges$");
    if (std::regex_match(route_path, path_1)) allowed.insert({"GET"});
    static const std::regex path_2("^/ai/runners$");
    if (std::regex_match(route_path, path_2)) allowed.insert({"GET", "POST"});
    static const std::regex path_3("^/ai/runs$");
    if (std::regex_match(route_path, path_3)) allowed.insert({"GET", "POST"});
    static const std::regex path_4("^/ai/capabilities$");
    if (std::regex_match(route_path, path_4)) allowed.insert({"GET"});
    static const std::regex path_5("^/ai/status$");
    if (std::regex_match(route_path, path_5)) allowed.insert({"GET"});
    static const std::regex path_6("^/ai/threads$");
    if (std::regex_match(route_path, path_6)) allowed.insert({"GET", "POST"});
    static const std::regex path_7("^/ai/runner/pull$");
    if (std::regex_match(route_path, path_7)) allowed.insert({"POST"});
    static const std::regex path_8("^/ai/providers/credentials$");
    if (std::regex_match(route_path, path_8)) allowed.insert({"GET", "PUT"});
    static const std::regex path_9("^/ai/local\\-models/config$");
    if (std::regex_match(route_path, path_9)) allowed.insert({"GET", "PUT"});
    static const std::regex path_10("^/ai/messages/capture$");
    if (std::regex_match(route_path, path_10)) allowed.insert({"POST"});
    static const std::regex path_11("^/ai/nudges/evaluate$");
    if (std::regex_match(route_path, path_11)) allowed.insert({"POST"});
    static const std::regex path_12("^/ai/providers/catalog$");
    if (std::regex_match(route_path, path_12)) allowed.insert({"GET"});
    static const std::regex path_13("^/ai/providers/settings$");
    if (std::regex_match(route_path, path_13)) allowed.insert({"GET", "PUT"});
    static const std::regex path_14("^/ai/runner/pull/([^/]+)$");
    if (std::regex_match(route_path, path_14)) allowed.insert({"GET"});
    static const std::regex path_15("^/ai/providers/credentials/([^/]+)$");
    if (std::regex_match(route_path, path_15)) allowed.insert({"DELETE"});
    static const std::regex path_16("^/ai/providers/settings/([^/]+)$");
    if (std::regex_match(route_path, path_16)) allowed.insert({"DELETE"});
    static const std::regex path_17("^/ai/runner/pull/([^/]+)/events$");
    if (std::regex_match(route_path, path_17)) allowed.insert({"GET"});
    static const std::regex path_18("^/ai/messages/([^/]+)$");
    if (std::regex_match(route_path, path_18)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_19("^/ai/runners/([^/]+)$");
    if (std::regex_match(route_path, path_19)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_20("^/ai/runs/([^/]+)$");
    if (std::regex_match(route_path, path_20)) allowed.insert({"GET"});
    static const std::regex path_21("^/ai/threads/([^/]+)$");
    if (std::regex_match(route_path, path_21)) allowed.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_22("^/ai/messages/([^/]+)/links$");
    if (std::regex_match(route_path, path_22)) allowed.insert({"DELETE", "GET", "POST"});
    static const std::regex path_23("^/ai/messages/([^/]+)/backlinks$");
    if (std::regex_match(route_path, path_23)) allowed.insert({"GET"});
    static const std::regex path_24("^/ai/messages/([^/]+)/restore$");
    if (std::regex_match(route_path, path_24)) allowed.insert({"POST"});
    static const std::regex path_25("^/ai/nudges/([^/]+)/dismiss$");
    if (std::regex_match(route_path, path_25)) allowed.insert({"POST"});
    static const std::regex path_26("^/ai/runners/([^/]+)/retry$");
    if (std::regex_match(route_path, path_26)) allowed.insert({"POST"});
    static const std::regex path_27("^/ai/runs/([^/]+)/events$");
    if (std::regex_match(route_path, path_27)) allowed.insert({"GET"});
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
    (void)method;
    (void)path;
    return false;
}
// Whether the operation dispatch would select for this request is a RIG_HOST_RESPONSE
// operation, so the hosting server serves only the routes the contract declares.
inline bool is_host_served(const std::string& method, const std::string& path) {
    (void)method;
    (void)path;
    return false;
}
} // namespace ai_http
