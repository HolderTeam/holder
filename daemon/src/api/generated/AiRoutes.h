// Generated from contract/interface/ai.py, contract/interface/card_references.py, contract/interface/cards/models.py, contract/interface/cards/operations.py, contract/interface/daemon_routes.py, contract/interface/errors.py, contract/interface/git.py, contract/interface/history.py, contract/interface/links.py, contract/interface/locations.py, contract/interface/maintenance.py, contract/interface/milestones.py, contract/interface/operations/ai.py, contract/interface/operations/git.py, contract/interface/operations/history.py, contract/interface/operations/links.py, contract/interface/operations/locations.py, contract/interface/operations/maintenance.py, contract/interface/operations/milestones.py, contract/interface/operations/resources.py, contract/interface/operations/search.py, contract/interface/operations/system.py, contract/interface/operations/trash.py, contract/interface/pagination/models.py, contract/interface/projects/ids.py, contract/interface/projects/models.py, contract/interface/projects/operations.py, contract/interface/rebuild.py, contract/interface/recovery.py, contract/interface/resources.py, contract/interface/search.py, contract/interface/trash.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// AI HTTP routes generated from contract/interface/operations/ai.py.
#pragma once
#include "api/generated/AiJson.h"
#include "api/generated/ContractRoutes.h"
#include <any>
#include <charconv>
#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
namespace ai_http {
using json = nlohmann::json;
struct response { int status; json body; std::string_view operation;
    std::any host = {};
    std::string_view media_type = "application/json"; };
using header_lookup = std::function<std::optional<std::string>(std::string_view)>;

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
inline int error_status(const std::exception& error) {
    if (dynamic_cast<const std::invalid_argument*>(&error)) return 400;
    if (dynamic_cast<const holder::model::ConflictError*>(&error)) return 409;
    if (dynamic_cast<const holder::api::AiProviderCatalogMissing*>(&error)) return 400;
    if (dynamic_cast<const holder::api::AiRunsMissingScope*>(&error)) return 400;
    if (dynamic_cast<const holder::api::AiMessageWrongState*>(&error)) return 400;
    if (dynamic_cast<const holder::api::AiCaptureNotFound*>(&error)) return 404;
    if (dynamic_cast<const holder::api::AiMessageNotFound*>(&error)) return 404;
    if (dynamic_cast<const holder::api::AiNudgeNotFound*>(&error)) return 404;
    if (dynamic_cast<const holder::api::AiRunNotFound*>(&error)) return 404;
    if (dynamic_cast<const holder::api::AiRunnerNotConfigured*>(&error)) return 404;
    if (dynamic_cast<const holder::api::AiRunnerNotFound*>(&error)) return 404;
    if (dynamic_cast<const holder::api::AiThreadNotFound*>(&error)) return 404;
    if (dynamic_cast<const holder::api::PullJobNotFound*>(&error)) return 404;
    if (dynamic_cast<const holder::api::RunnerNotConfigured*>(&error)) return 404;
    if (dynamic_cast<const holder::api::AiRunUnavailable*>(&error)) return 503;
    if (dynamic_cast<const holder::api::ProviderCredentialSecretFailure*>(&error)) return 503;
    if (dynamic_cast<const holder::api::RunnerUnavailable*>(&error)) return 503;
    if (dynamic_cast<const holder::api::SecretStoreUnavailable*>(&error)) return 503;
    return 500;
}
inline response failure(int status, const std::exception& error, std::string_view operation) {
    const auto type = std::type_identity<holder::api::ErrorResponse>{};
    if (const auto* privacy = dynamic_cast<const holder::privacy::PrivacyError*>(&error))
        return {status, ai_json::encode(holder::api::api_error_body(type, status, *privacy)), operation};
    if (const auto* coded = dynamic_cast<const holder::api::CodedError*>(&error))
        return {status, ai_json::encode(holder::api::api_error_body(type, status, *coded)), operation};
    return {status, ai_json::encode(holder::api::api_error_body(type, status, error)), operation};
}
template <typename Context>
inline std::optional<response> dispatch(const std::string& method, const std::string& path,
    const std::string& body, Context& context, const header_lookup& header = {}) {
    const auto separator = path.find('?');
    const auto route_path = path.substr(0, separator);
    const auto query_string = separator == std::string::npos ? std::string_view{} :
        std::string_view(path).substr(separator + 1);
    std::smatch match;
    (void)body; (void)context; (void)header;
    static const std::regex route_7("^/ai/messages$");
    if (method == "GET" && std::regex_match(route_path, match, route_7)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {"thread_id", "include_deleted"});
            const auto query_thread_id = query_values.find("thread_id");
            if (query_thread_id == query_values.end()) throw std::invalid_argument("missing thread_id");
            auto arg_thread_id = ai_json::check_text(query_thread_id->second, "thread_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_include_deleted = query_values.find("include_deleted");
            std::optional<int> arg_include_deleted;
            if (query_include_deleted != query_values.end()) {
                auto decoded_include_deleted = contract_http::decode_integer<int>(query_include_deleted->second, "include_deleted");
                if (decoded_include_deleted != 0 && decoded_include_deleted != 1) throw std::invalid_argument("invalid include_deleted");
                arg_include_deleted = std::move(decoded_include_deleted);
            }
            return response{200, ai_json::encode(holder::api::get_ai_messages(context.db, context.fts, std::move(arg_thread_id), std::move(arg_include_deleted))), "get_ai_messages"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_messages");
        }
    }
    static const std::regex route_11("^/ai/messages$");
    if (method == "POST" && std::regex_match(route_path, match, route_11)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_request = decode_body<holder::api::AiMessageCreateRequest>(body);
            return response{201, ai_json::encode(holder::api::post_ai_messages(context.db, context.fts, context.uuid_v4, std::move(arg_request))), "post_ai_messages"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_messages");
        }
    }
    static const std::regex route_18("^/ai/nudges$");
    if (method == "GET" && std::regex_match(route_path, match, route_18)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {"project_id", "card_id"});
            const auto query_project_id = query_values.find("project_id");
            if (query_project_id == query_values.end()) throw std::invalid_argument("missing project_id");
            auto arg_project_id = ai_json::check_text(query_project_id->second, "project_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_card_id = query_values.find("card_id");
            std::optional<std::string> arg_card_id;
            if (query_card_id != query_values.end()) {
                auto decoded_card_id = ai_json::check_text(query_card_id->second, "card_id", 0, std::numeric_limits<std::size_t>::max());
                arg_card_id = std::move(decoded_card_id);
            }
            return response{200, ai_json::encode(holder::api::get_ai_nudges(context.db, context.registry, std::move(arg_project_id), std::move(arg_card_id))), "get_ai_nudges"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_nudges");
        }
    }
    static const std::regex route_22("^/ai/runners$");
    if (method == "GET" && std::regex_match(route_path, match, route_22)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            return response{200, ai_json::encode(holder::api::get_ai_runners(context.registry)), "get_ai_runners"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_runners");
        }
    }
    static const std::regex route_24("^/ai/runners$");
    if (method == "POST" && std::regex_match(route_path, match, route_24)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_request = decode_body<holder::api::AiRunnerCreateRequest>(body);
            return response{201, ai_json::encode(holder::api::post_ai_runners(context.db, context.registry, context.uuid_v4, std::move(arg_request))), "post_ai_runners"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_runners");
        }
    }
    static const std::regex route_28("^/ai/runs$");
    if (method == "GET" && std::regex_match(route_path, match, route_28)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {"project_id", "thread_id"});
            const auto query_project_id = query_values.find("project_id");
            std::optional<std::string> arg_project_id;
            if (query_project_id != query_values.end()) {
                auto decoded_project_id = ai_json::check_text(query_project_id->second, "project_id", 0, std::numeric_limits<std::size_t>::max());
                arg_project_id = std::move(decoded_project_id);
            }
            const auto query_thread_id = query_values.find("thread_id");
            std::optional<std::string> arg_thread_id;
            if (query_thread_id != query_values.end()) {
                auto decoded_thread_id = ai_json::check_text(query_thread_id->second, "thread_id", 0, std::numeric_limits<std::size_t>::max());
                arg_thread_id = std::move(decoded_thread_id);
            }
            return response{200, ai_json::encode(holder::api::get_ai_runs(context.db, std::move(arg_project_id), std::move(arg_thread_id))), "get_ai_runs"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_runs");
        }
    }
    static const std::regex route_30("^/ai/capabilities$");
    if (method == "GET" && std::regex_match(route_path, match, route_30)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {"runner_id"});
            const auto query_runner_id = query_values.find("runner_id");
            std::optional<std::string> arg_runner_id;
            if (query_runner_id != query_values.end()) {
                auto decoded_runner_id = ai_json::check_text(query_runner_id->second, "runner_id", 0, std::numeric_limits<std::size_t>::max());
                arg_runner_id = std::move(decoded_runner_id);
            }
            return response{200, ai_json::encode(holder::api::get_ai_capabilities(context.db, context.registry, std::move(arg_runner_id))), "get_ai_capabilities"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_capabilities");
        }
    }
    static const std::regex route_31("^/ai/status$");
    if (method == "GET" && std::regex_match(route_path, match, route_31)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            return response{200, ai_json::encode(holder::api::get_ai_status(context.db, context.registry)), "get_ai_status"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_status");
        }
    }
    static const std::regex route_32("^/ai/threads$");
    if (method == "GET" && std::regex_match(route_path, match, route_32)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {"project_id"});
            const auto query_project_id = query_values.find("project_id");
            if (query_project_id == query_values.end()) throw std::invalid_argument("missing project_id");
            auto arg_project_id = ai_json::check_text(query_project_id->second, "project_id", 1, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::get_ai_threads(context.db, std::move(arg_project_id))), "get_ai_threads"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_threads");
        }
    }
    static const std::regex route_34("^/ai/threads$");
    if (method == "POST" && std::regex_match(route_path, match, route_34)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_request = decode_body<holder::api::AiThreadCreateRequest>(body);
            return response{201, ai_json::encode(holder::api::post_ai_threads(context.db, context.uuid_v4, std::move(arg_request))), "post_ai_threads"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_threads");
        }
    }
    static const std::regex route_40("^/ai/runs$");
    if (method == "POST" && std::regex_match(route_path, match, route_40)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_request = decode_body<holder::api::AiRunCreateRequest>(body);
            auto outcome = holder::api::post_ai_runs(context.socket, context.db, context.fts, context.secret_store, context.registry, context.uuid_v4, context.events, std::move(arg_request));
            if (outcome.status != 200 && outcome.status != 204)
                throw std::logic_error("post_ai_runs chose an undeclared success status");
            return response{outcome.status, json(), "post_ai_runs", std::move(outcome.value)};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_runs");
        }
    }
    static const std::regex route_1("^/ai/runner/pull$");
    if (method == "POST" && std::regex_match(route_path, match, route_1)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_request = decode_body<holder::api::PostAiRunnerPullRequest>(body);
            return response{200, ai_json::encode(holder::api::post_ai_runner_pull(context.registry, std::move(arg_request))), "post_ai_runner_pull"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_runner_pull");
        }
    }
    static const std::regex route_2("^/ai/providers/credentials$");
    if (method == "GET" && std::regex_match(route_path, match, route_2)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            return response{200, ai_json::encode(holder::api::list_provider_credentials(context.db)), "list_provider_credentials"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "list_provider_credentials");
        }
    }
    static const std::regex route_3("^/ai/providers/credentials$");
    if (method == "PUT" && std::regex_match(route_path, match, route_3)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_request = decode_body<holder::api::AiProviderCredentialUpsertRequest>(body);
            return response{200, ai_json::encode(holder::api::upsert_provider_credential(context.db, context.secret_store, std::move(arg_request))), "upsert_provider_credential"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "upsert_provider_credential");
        }
    }
    static const std::regex route_5("^/ai/local\\-models/config$");
    if (method == "GET" && std::regex_match(route_path, match, route_5)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            return response{200, ai_json::encode(holder::api::get_local_model_config(context.db)), "get_local_model_config"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_local_model_config");
        }
    }
    static const std::regex route_6("^/ai/local\\-models/config$");
    if (method == "PUT" && std::regex_match(route_path, match, route_6)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_request = decode_body<holder::api::AiLocalModelConfigUpdateRequest>(body);
            return response{200, ai_json::encode(holder::api::put_local_model_config(context.db, std::move(arg_request))), "put_local_model_config"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "put_local_model_config");
        }
    }
    static const std::regex route_17("^/ai/messages/capture$");
    if (method == "POST" && std::regex_match(route_path, match, route_17)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_request = decode_body<holder::api::AiMessageCaptureRequest>(body);
            return response{201, ai_json::encode(holder::api::post_ai_messages_capture(context.db, context.fts, context.uuid_v4, std::move(arg_request))), "post_ai_messages_capture"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_messages_capture");
        }
    }
    static const std::regex route_20("^/ai/nudges/evaluate$");
    if (method == "POST" && std::regex_match(route_path, match, route_20)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_candidate = decode_body<holder::api::AiNudgeCandidate>(body);
            return response{200, ai_json::encode(holder::api::post_ai_nudges_evaluate(context.db, context.registry, std::move(arg_candidate))), "post_ai_nudges_evaluate"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_nudges_evaluate");
        }
    }
    static const std::regex route_21("^/ai/providers/catalog$");
    if (method == "GET" && std::regex_match(route_path, match, route_21)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            return response{200, ai_json::encode(holder::api::get_ai_provider_catalog(context.db)), "get_ai_provider_catalog"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_provider_catalog");
        }
    }
    static const std::regex route_37("^/ai/providers/settings$");
    if (method == "GET" && std::regex_match(route_path, match, route_37)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            return response{200, ai_json::encode(holder::api::list_provider_settings(context.db)), "list_provider_settings"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "list_provider_settings");
        }
    }
    static const std::regex route_38("^/ai/providers/settings$");
    if (method == "PUT" && std::regex_match(route_path, match, route_38)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_request = decode_body<holder::api::AiProviderSettingUpdateRequest>(body);
            return response{200, ai_json::encode(holder::api::set_provider_setting(context.db, std::move(arg_request))), "set_provider_setting"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "set_provider_setting");
        }
    }
    static const std::regex route_0("^/ai/runner/pull/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_0)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {"runner_id"});
            const auto query_runner_id = query_values.find("runner_id");
            std::optional<std::string> arg_runner_id;
            if (query_runner_id != query_values.end()) {
                auto decoded_runner_id = ai_json::check_text(query_runner_id->second, "runner_id", 0, std::numeric_limits<std::size_t>::max());
                arg_runner_id = std::move(decoded_runner_id);
            }
            auto arg_job_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "job_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::get_pull_status(context.registry, std::move(arg_runner_id), std::move(arg_job_id))), "get_pull_status"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_pull_status");
        }
    }
    static const std::regex route_4("^/ai/providers/credentials/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_4)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_provider = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "provider", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::delete_provider_credential(context.db, context.secret_store, std::move(arg_provider))), "delete_provider_credential"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "delete_provider_credential");
        }
    }
    static const std::regex route_39("^/ai/providers/settings/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_39)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_provider = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "provider", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::delete_provider_setting(context.db, std::move(arg_provider))), "delete_provider_setting"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "delete_provider_setting");
        }
    }
    static const std::regex route_42("^/ai/runner/pull/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_42)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {"runner_id"});
            const auto query_runner_id = query_values.find("runner_id");
            std::optional<std::string> arg_runner_id;
            if (query_runner_id != query_values.end()) {
                auto decoded_runner_id = ai_json::check_text(query_runner_id->second, "runner_id", 0, std::numeric_limits<std::size_t>::max());
                arg_runner_id = std::move(decoded_runner_id);
            }
            auto arg_last_event_id = header ? header("Last-Event-ID") : std::nullopt;
            auto arg_job_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "job_id", 0, std::numeric_limits<std::size_t>::max());
            auto outcome = holder::api::get_ai_runner_pull_job_id_events(context.registry, context.events, std::move(arg_runner_id), std::move(arg_last_event_id), std::move(arg_job_id));
            if (outcome.status != 200 && outcome.status != 204)
                throw std::logic_error("get_ai_runner_pull_job_id_events chose an undeclared success status");
            return response{outcome.status, json(), "get_ai_runner_pull_job_id_events", std::move(outcome.value)};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_runner_pull_job_id_events");
        }
    }
    static const std::regex route_8("^/ai/messages/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_8)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_message_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "message_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::get_ai_message(context.db, context.fts, std::move(arg_message_id))), "get_ai_message"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_message");
        }
    }
    static const std::regex route_12("^/ai/messages/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_12)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_message_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "message_id", 0, std::numeric_limits<std::size_t>::max());
            auto arg_request = decode_body<holder::api::AiMessageUpdateRequest>(body);
            return response{200, ai_json::encode(holder::api::patch_ai_messages_message_id(context.db, context.fts, std::move(arg_message_id), std::move(arg_request))), "patch_ai_messages_message_id"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "patch_ai_messages_message_id");
        }
    }
    static const std::regex route_13("^/ai/messages/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_13)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_message_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "message_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::delete_ai_messages_message_id(context.db, context.fts, std::move(arg_message_id))), "delete_ai_messages_message_id"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "delete_ai_messages_message_id");
        }
    }
    static const std::regex route_23("^/ai/runners/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_23)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_runner_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "runner_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::get_ai_runner(context.registry, std::move(arg_runner_id))), "get_ai_runner"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_runner");
        }
    }
    static const std::regex route_25("^/ai/runners/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_25)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_runner_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "runner_id", 0, std::numeric_limits<std::size_t>::max());
            auto arg_request = decode_body<holder::api::AiRunnerUpdateRequest>(body);
            return response{200, ai_json::encode(holder::api::patch_ai_runners_runner_id(context.db, context.registry, std::move(arg_runner_id), std::move(arg_request))), "patch_ai_runners_runner_id"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "patch_ai_runners_runner_id");
        }
    }
    static const std::regex route_26("^/ai/runners/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_26)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_runner_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "runner_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::delete_ai_runners_runner_id(context.db, context.registry, std::move(arg_runner_id))), "delete_ai_runners_runner_id"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "delete_ai_runners_runner_id");
        }
    }
    static const std::regex route_29("^/ai/runs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_29)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_run_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "run_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::get_ai_run(context.db, std::move(arg_run_id))), "get_ai_run"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_run");
        }
    }
    static const std::regex route_33("^/ai/threads/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_33)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_thread_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "thread_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::get_ai_thread(context.db, std::move(arg_thread_id))), "get_ai_thread"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_thread");
        }
    }
    static const std::regex route_35("^/ai/threads/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_35)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_thread_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "thread_id", 0, std::numeric_limits<std::size_t>::max());
            auto arg_request = decode_body<holder::api::AiThreadUpdateRequest>(body);
            return response{200, ai_json::encode(holder::api::patch_ai_threads_thread_id(context.db, std::move(arg_thread_id), std::move(arg_request))), "patch_ai_threads_thread_id"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "patch_ai_threads_thread_id");
        }
    }
    static const std::regex route_36("^/ai/threads/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_36)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_thread_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "thread_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::delete_ai_threads_thread_id(context.db, std::move(arg_thread_id))), "delete_ai_threads_thread_id"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "delete_ai_threads_thread_id");
        }
    }
    static const std::regex route_9("^/ai/messages/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, match, route_9)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {"include_deleted"});
            const auto query_include_deleted = query_values.find("include_deleted");
            std::optional<int> arg_include_deleted;
            if (query_include_deleted != query_values.end()) {
                auto decoded_include_deleted = contract_http::decode_integer<int>(query_include_deleted->second, "include_deleted");
                if (decoded_include_deleted != 0 && decoded_include_deleted != 1) throw std::invalid_argument("invalid include_deleted");
                arg_include_deleted = std::move(decoded_include_deleted);
            }
            auto arg_message_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "message_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::get_ai_message_links(context.db, context.fts, std::move(arg_message_id), std::move(arg_include_deleted))), "get_ai_message_links"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_message_links");
        }
    }
    static const std::regex route_10("^/ai/messages/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, match, route_10)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {"include_deleted"});
            const auto query_include_deleted = query_values.find("include_deleted");
            std::optional<int> arg_include_deleted;
            if (query_include_deleted != query_values.end()) {
                auto decoded_include_deleted = contract_http::decode_integer<int>(query_include_deleted->second, "include_deleted");
                if (decoded_include_deleted != 0 && decoded_include_deleted != 1) throw std::invalid_argument("invalid include_deleted");
                arg_include_deleted = std::move(decoded_include_deleted);
            }
            auto arg_message_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "message_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::get_ai_message_backlinks(context.db, context.fts, std::move(arg_message_id), std::move(arg_include_deleted))), "get_ai_message_backlinks"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_message_backlinks");
        }
    }
    static const std::regex route_14("^/ai/messages/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, match, route_14)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_message_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "message_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::post_ai_messages_message_id_restore(context.db, context.fts, std::move(arg_message_id))), "post_ai_messages_message_id_restore"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_messages_message_id_restore");
        }
    }
    static const std::regex route_15("^/ai/messages/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, match, route_15)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_message_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "message_id", 0, std::numeric_limits<std::size_t>::max());
            auto arg_request = decode_body<holder::api::routes::CardLinkCreateRequest>(body);
            return response{201, ai_json::encode(holder::api::post_ai_messages_message_id_links(context.db, context.fts, std::move(arg_message_id), std::move(arg_request))), "post_ai_messages_message_id_links"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_messages_message_id_links");
        }
    }
    static const std::regex route_16("^/ai/messages/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_16)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_message_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "message_id", 0, std::numeric_limits<std::size_t>::max());
            std::optional<holder::api::CardLinkDeleteRequest> arg_request;
            if (!body.empty()) arg_request = decode_body<holder::api::CardLinkDeleteRequest>(body);
            return response{200, ai_json::encode(holder::api::delete_ai_messages_message_id_links(context.db, context.fts, std::move(arg_message_id), std::move(arg_request))), "delete_ai_messages_message_id_links"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "delete_ai_messages_message_id_links");
        }
    }
    static const std::regex route_19("^/ai/nudges/([^/]+)/dismiss$");
    if (method == "POST" && std::regex_match(route_path, match, route_19)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_nudge_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "nudge_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::post_ai_nudges_nudge_id_dismiss(context.db, context.registry, std::move(arg_nudge_id))), "post_ai_nudges_nudge_id_dismiss"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_nudges_nudge_id_dismiss");
        }
    }
    static const std::regex route_27("^/ai/runners/([^/]+)/retry$");
    if (method == "POST" && std::regex_match(route_path, match, route_27)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_runner_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "runner_id", 0, std::numeric_limits<std::size_t>::max());
            return response{200, ai_json::encode(holder::api::post_ai_runners_runner_id_retry(context.registry, std::move(arg_runner_id))), "post_ai_runners_runner_id_retry"};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "post_ai_runners_runner_id_retry");
        }
    }
    static const std::regex route_41("^/ai/runs/([^/]+)/events$");
    if (method == "GET" && std::regex_match(route_path, match, route_41)) {
        try {
            const auto query_values = contract_http::decode_query(query_string);
            contract_http::check_query_names(query_values, {});
            auto arg_last_event_id = header ? header("Last-Event-ID") : std::nullopt;
            auto arg_run_id = ai_json::check_text(contract_http::decode_uri_component(match[1].str(), false), "run_id", 0, std::numeric_limits<std::size_t>::max());
            auto outcome = holder::api::get_ai_runs_run_id_events(context.db, context.events, std::move(arg_last_event_id), std::move(arg_run_id));
            if (outcome.status != 200 && outcome.status != 204)
                throw std::logic_error("get_ai_runs_run_id_events chose an undeclared success status");
            return response{outcome.status, json(), "get_ai_runs_run_id_events", std::move(outcome.value)};
        } catch (const std::exception& error) {
            return failure(error_status(error), error, "get_ai_runs_run_id_events");
        }
    }
    return std::nullopt;
}
} // namespace ai_http
