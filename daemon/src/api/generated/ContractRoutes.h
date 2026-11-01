// Generated from contract/interface/ai/models.py, contract/interface/ai/operations.py, contract/interface/cards/models.py, contract/interface/cards/operations.py, contract/interface/cards/references.py, contract/interface/daemon/models.py, contract/interface/daemon/operations.py, contract/interface/documents/operations.py, contract/interface/errors.py, contract/interface/events/models.py, contract/interface/events/operations.py, contract/interface/git/models.py, contract/interface/git/operations.py, contract/interface/history/models.py, contract/interface/history/operations.py, contract/interface/links/models.py, contract/interface/links/operations.py, contract/interface/locations/models.py, contract/interface/locations/operations.py, contract/interface/maintenance/models.py, contract/interface/maintenance/operations.py, contract/interface/maintenance/rebuild.py, contract/interface/milestones/models.py, contract/interface/milestones/operations.py, contract/interface/pagination/models.py, contract/interface/projects/ids.py, contract/interface/projects/models.py, contract/interface/projects/operations.py, contract/interface/recovery/models.py, contract/interface/recovery/operations.py, contract/interface/resources/models.py, contract/interface/resources/operations.py, contract/interface/search/models.py, contract/interface/search/operations.py, contract/interface/trash/models.py, contract/interface/trash/operations.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// The daemon's HTTP routes for the contract's operations; contract/holder_contract/http_routes.py says how they behave.
#pragma once
#include "api/generated/ContractJson.h"
#include <charconv>
#include <algorithm>
#include <initializer_list>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

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
// The response for a request the contract has an operation for; nothing otherwise.
inline std::optional<response> dispatch(const std::string& method, const std::string& path,
                                        const std::string& body, holder_context& context) {
    const auto query_start = path.find('?');
    const auto route_path = path.substr(0, query_start);
    const auto query_string = query_start == std::string::npos ? std::string_view{} :
        std::string_view(path).substr(query_start + 1);
    std::smatch match;
    (void)body;
    (void)context;
    static const std::regex route_1("^/locations$");
    if (method == "POST" && std::regex_match(route_path, match, route_1)) {
        holder::contract::NewStorageLocation request{};
        try {
            check_query_names(decode_query(query_string), {});
            request = contract_json::decode<holder::contract::NewStorageLocation>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "create_storage_location");
        }
        try {
            return success(201, contract_json::encode(holder::contract::create_storage_location(context, std::move(request))), "create_storage_location");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "create_storage_location");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "create_storage_location");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "create_storage_location");
        }
    }
    static const std::regex route_37("^/projects$");
    if (method == "GET" && std::regex_match(route_path, match, route_37)) {
        std::optional<std::string> name{};
        std::optional<long long> updated_after{};
        std::optional<long long> updated_before{};
        std::optional<holder::contract::ListProjectsOrder> order{};
        std::int32_t limit{};
        std::int32_t offset{};
        bool include_card_counts{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"name", "updated_after", "updated_before", "order", "limit", "offset", "include_card_counts"});
            const auto query_name = query_values.find("name");
            if (query_name != query_values.end()) {
                const auto& text = query_name->second;
                name = contract_json::check_text(text, "name", 1, std::numeric_limits<std::size_t>::max());
            }
            const auto query_updated_after = query_values.find("updated_after");
            if (query_updated_after != query_values.end()) {
                const auto& text = query_updated_after->second;
                updated_after = decode_integer<long long>(text, "updated_after");
            }
            const auto query_updated_before = query_values.find("updated_before");
            if (query_updated_before != query_values.end()) {
                const auto& text = query_updated_before->second;
                updated_before = decode_integer<long long>(text, "updated_before");
            }
            const auto query_order = query_values.find("order");
            if (query_order != query_values.end()) {
                const auto& text = query_order->second;
                if (text == "updated_at_desc") order = holder::contract::ListProjectsOrder::UpdatedAtDesc;
                else if (text == "updated_at_asc") order = holder::contract::ListProjectsOrder::UpdatedAtAsc;
                else if (text == "created_at_desc") order = holder::contract::ListProjectsOrder::CreatedAtDesc;
                else if (text == "created_at_asc") order = holder::contract::ListProjectsOrder::CreatedAtAsc;
                else if (text == "name_desc") order = holder::contract::ListProjectsOrder::NameDesc;
                else if (text == "name_asc") order = holder::contract::ListProjectsOrder::NameAsc;
                else throw std::invalid_argument("order must be one of updated_at_desc, updated_at_asc, created_at_desc, created_at_asc, name_desc, name_asc");
            }
            const auto query_limit = query_values.find("limit");
            if (query_limit != query_values.end()) {
                const auto& text = query_limit->second;
                limit = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "limit"), "limit", 1, 1000);
            } else {
                limit = 100;
            }
            const auto query_offset = query_values.find("offset");
            if (query_offset != query_values.end()) {
                const auto& text = query_offset->second;
                offset = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "offset"), "offset", 0, std::numeric_limits<std::int32_t>::max());
            } else {
                offset = 0;
            }
            const auto query_include_card_counts = query_values.find("include_card_counts");
            if (query_include_card_counts != query_values.end()) {
                const auto& text = query_include_card_counts->second;
                include_card_counts = decode_bool(text, "include_card_counts");
            } else {
                include_card_counts = false;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_projects");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_projects(context, std::move(name), std::move(updated_after), std::move(updated_before), std::move(order), std::move(limit), std::move(offset), std::move(include_card_counts))), "list_projects");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_projects");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_projects");
        }
    }
    static const std::regex route_39("^/projects$");
    if (method == "POST" && std::regex_match(route_path, match, route_39)) {
        holder::contract::NewProject project{};
        try {
            check_query_names(decode_query(query_string), {});
            project = contract_json::decode<holder::contract::NewProject>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "create_project");
        }
        try {
            return success(201, contract_json::encode(holder::contract::create_project(context, std::move(project))), "create_project");
        } catch (const holder::contract::Conflict& error) {
            return failure(409, "conflict", error.what(), "create_project");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "create_project");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "create_project");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "create_project");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "create_project");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "create_project");
        }
    }
    static const std::regex route_44("^/rebuild$");
    if (method == "POST" && std::regex_match(route_path, match, route_44)) {
        holder::contract::RebuildRequest request{};
        try {
            check_query_names(decode_query(query_string), {});
            request = contract_json::decode<holder::contract::RebuildRequest>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "rebuild_project");
        }
        try {
            return success(200, contract_json::encode(holder::contract::rebuild_project(context, std::move(request))), "rebuild_project");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "rebuild_project");
        } catch (const holder::contract::RebuildFailed& error) {
            return failure(400, "rebuild_failed", error.what(), "rebuild_project");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "rebuild_project");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "rebuild_project");
        }
    }
    static const std::regex route_45("^/reindex$");
    if (method == "POST" && std::regex_match(route_path, match, route_45)) {
        try {
            check_query_names(decode_query(query_string), {});
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "reindex_search");
        }
        try {
            return success(200, contract_json::encode(holder::contract::reindex_search(context)), "reindex_search");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "reindex_search");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "reindex_search");
        }
    }
    static const std::regex route_60("^/trash$");
    if (method == "GET" && std::regex_match(route_path, match, route_60)) {
        std::string project_id{};
        std::optional<std::string> type{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"project_id", "type"});
            const auto query_project_id = query_values.find("project_id");
            if (query_project_id != query_values.end()) {
                const auto& text = query_project_id->second;
                project_id = contract_json::check_text(text, "project_id", 1, std::numeric_limits<std::size_t>::max());
            } else {
                throw std::invalid_argument("the query parameter project_id is required");
            }
            const auto query_type = query_values.find("type");
            if (query_type != query_values.end()) {
                const auto& text = query_type->second;
                type = contract_json::check_text(text, "type", 0, std::numeric_limits<std::size_t>::max());
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_trash");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_trash(context, std::move(project_id), std::move(type))), "list_trash");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_trash");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_trash");
        }
    }
    static const std::regex route_61("^/trash$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_61)) {
        std::string project_id{};
        holder::contract::EmptyTrashType type{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"project_id", "type"});
            const auto query_project_id = query_values.find("project_id");
            if (query_project_id != query_values.end()) {
                const auto& text = query_project_id->second;
                project_id = contract_json::check_text(text, "project_id", 1, std::numeric_limits<std::size_t>::max());
            } else {
                throw std::invalid_argument("the query parameter project_id is required");
            }
            const auto query_type = query_values.find("type");
            if (query_type != query_values.end()) {
                const auto& text = query_type->second;
                if (text == "card") type = holder::contract::EmptyTrashType::Card;
                else if (text == "ai_message") type = holder::contract::EmptyTrashType::AiMessage;
                else if (text == "all") type = holder::contract::EmptyTrashType::All;
                else throw std::invalid_argument("type must be one of card, ai_message, all");
            } else {
                type = holder::contract::EmptyTrashType::All;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "empty_trash");
        }
        try {
            return success(200, contract_json::encode(holder::contract::empty_trash(context, std::move(project_id), std::move(type))), "empty_trash");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "empty_trash");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "empty_trash");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "empty_trash");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "empty_trash");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "empty_trash");
        }
    }
    static const std::regex route_7("^/card\\-references/resolve$");
    if (method == "POST" && std::regex_match(route_path, match, route_7)) {
        holder::contract::CardReferenceResolveRequest request{};
        try {
            check_query_names(decode_query(query_string), {});
            request = contract_json::decode<holder::contract::CardReferenceResolveRequest>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "resolve_card_reference");
        }
        try {
            return success(200, contract_json::encode(holder::contract::resolve_card_reference(context, std::move(request))), "resolve_card_reference");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "resolve_card_reference");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "resolve_card_reference");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "resolve_card_reference");
        }
    }
    static const std::regex route_48("^/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, match, route_48)) {
        holder::contract::RecoveryTokenImportRequest request{};
        try {
            check_query_names(decode_query(query_string), {});
            request = contract_json::decode<holder::contract::RecoveryTokenImportRequest>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "import_recovery_token_global");
        }
        try {
            auto result = holder::contract::import_recovery_token_global(context, std::move(request));
            return success(result.project_created ? 201 : 200, contract_json::encode(result), "import_recovery_token_global");
        } catch (const holder::contract::RecoveryTokenInvalid& error) {
            return failure(400, "recovery_token_invalid", error.what(), "import_recovery_token_global");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "import_recovery_token_global");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "import_recovery_token_global");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "import_recovery_token_global");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "import_recovery_token_global");
        }
    }
    static const std::regex route_64("^/search/cards$");
    if (method == "GET" && std::regex_match(route_path, match, route_64)) {
        std::string project_id{};
        std::string q{};
        std::int32_t limit{};
        std::int32_t offset{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"project_id", "q", "limit", "offset"});
            const auto query_project_id = query_values.find("project_id");
            if (query_project_id != query_values.end()) {
                const auto& text = query_project_id->second;
                project_id = contract_json::check_text(text, "project_id", 1, std::numeric_limits<std::size_t>::max());
            } else {
                throw std::invalid_argument("the query parameter project_id is required");
            }
            const auto query_q = query_values.find("q");
            if (query_q != query_values.end()) {
                const auto& text = query_q->second;
                q = contract_json::check_text(text, "q", 1, std::numeric_limits<std::size_t>::max());
            } else {
                throw std::invalid_argument("the query parameter q is required");
            }
            const auto query_limit = query_values.find("limit");
            if (query_limit != query_values.end()) {
                const auto& text = query_limit->second;
                limit = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "limit"), "limit", 1, std::numeric_limits<std::int32_t>::max());
            } else {
                limit = 20;
            }
            const auto query_offset = query_values.find("offset");
            if (query_offset != query_values.end()) {
                const auto& text = query_offset->second;
                offset = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "offset"), "offset", 0, std::numeric_limits<std::int32_t>::max());
            } else {
                offset = 0;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "search_cards");
        }
        try {
            return success(200, contract_json::encode(holder::contract::search_cards(context, std::move(project_id), std::move(q), std::move(limit), std::move(offset))), "search_cards");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "search_cards");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "search_cards");
        }
    }
    static const std::regex route_65("^/search/ai$");
    if (method == "GET" && std::regex_match(route_path, match, route_65)) {
        std::string project_id{};
        std::string q{};
        std::int32_t limit{};
        std::int32_t offset{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"project_id", "q", "limit", "offset"});
            const auto query_project_id = query_values.find("project_id");
            if (query_project_id != query_values.end()) {
                const auto& text = query_project_id->second;
                project_id = contract_json::check_text(text, "project_id", 1, std::numeric_limits<std::size_t>::max());
            } else {
                throw std::invalid_argument("the query parameter project_id is required");
            }
            const auto query_q = query_values.find("q");
            if (query_q != query_values.end()) {
                const auto& text = query_q->second;
                q = contract_json::check_text(text, "q", 1, std::numeric_limits<std::size_t>::max());
            } else {
                throw std::invalid_argument("the query parameter q is required");
            }
            const auto query_limit = query_values.find("limit");
            if (query_limit != query_values.end()) {
                const auto& text = query_limit->second;
                limit = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "limit"), "limit", 1, std::numeric_limits<std::int32_t>::max());
            } else {
                limit = 20;
            }
            const auto query_offset = query_values.find("offset");
            if (query_offset != query_values.end()) {
                const auto& text = query_offset->second;
                offset = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "offset"), "offset", 0, std::numeric_limits<std::int32_t>::max());
            } else {
                offset = 0;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "search_ai_messages");
        }
        try {
            return success(200, contract_json::encode(holder::contract::search_ai_messages(context, std::move(project_id), std::move(q), std::move(limit), std::move(offset))), "search_ai_messages");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "search_ai_messages");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "search_ai_messages");
        }
    }
    static const std::regex route_0("^/locations/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_0)) {
        std::string location_id{};
        try {
            check_query_names(decode_query(query_string), {});
            location_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "location_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "get_storage_location");
        }
        try {
            return success(200, contract_json::encode(holder::contract::get_storage_location(context, std::move(location_id))), "get_storage_location");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "get_storage_location");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "get_storage_location");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "get_storage_location");
        }
    }
    static const std::regex route_2("^/locations/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_2)) {
        std::string location_id{};
        holder::contract::StorageLocationUpdate request{};
        try {
            check_query_names(decode_query(query_string), {});
            location_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "location_id", 1, std::numeric_limits<std::size_t>::max());
            request = contract_json::decode<holder::contract::StorageLocationUpdate>(decode_body(body, true));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "update_storage_location");
        }
        try {
            return success(200, contract_json::encode(holder::contract::update_storage_location(context, std::move(location_id), std::move(request))), "update_storage_location");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "update_storage_location");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "update_storage_location");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "update_storage_location");
        }
    }
    static const std::regex route_3("^/locations/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_3)) {
        std::string location_id{};
        try {
            check_query_names(decode_query(query_string), {});
            location_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "location_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_storage_location");
        }
        try {
            return success(200, contract_json::encode(holder::contract::delete_storage_location(context, std::move(location_id))), "delete_storage_location");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "delete_storage_location");
        } catch (const holder::contract::Conflict& error) {
            return failure(409, "conflict", error.what(), "delete_storage_location");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_storage_location");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "delete_storage_location");
        }
    }
    static const std::regex route_38("^/projects/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_38)) {
        std::string project_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "get_project");
        }
        try {
            return success(200, contract_json::encode(holder::contract::get_project(context, std::move(project_id))), "get_project");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "get_project");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "get_project");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "get_project");
        }
    }
    static const std::regex route_40("^/projects/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_40)) {
        std::string project_id{};
        holder::contract::ProjectUpdate update{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            update = contract_json::decode<holder::contract::ProjectUpdate>(decode_body(body, true));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "update_project");
        }
        try {
            return success(200, contract_json::encode(holder::contract::update_project(context, std::move(project_id), std::move(update))), "update_project");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "update_project");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "update_project");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "update_project");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "update_project");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "update_project");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "update_project");
        }
    }
    static const std::regex route_41("^/projects/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_41)) {
        std::string project_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_project");
        }
        try {
            return success(200, contract_json::encode(holder::contract::delete_project(context, std::move(project_id))), "delete_project");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "delete_project");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_project");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "delete_project");
        }
    }
    static const std::regex route_8("^/projects/([^/]+)/cards$");
    if (method == "GET" && std::regex_match(route_path, match, route_8)) {
        std::string project_id{};
        std::optional<holder::contract::ListCardsParent> parent{};
        std::optional<std::string> tag{};
        holder::contract::ListCardsOrder order{};
        std::int32_t limit{};
        std::optional<std::string> cursor{};
        bool include_content{};
        bool include_child_count{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"parent", "tag", "order", "limit", "cursor", "include_content", "include_child_count"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_parent = query_values.find("parent");
            if (query_parent != query_values.end()) {
                const auto& text = query_parent->second;
                if (text == "roots") parent = holder::contract::ListCardsParent{holder::contract::ListCardsParentRoots{}};
                else parent = holder::contract::ListCardsParent{contract_json::check_text(text, "parent", 1, std::numeric_limits<std::size_t>::max())};
            }
            const auto query_tag = query_values.find("tag");
            if (query_tag != query_values.end()) {
                const auto& text = query_tag->second;
                tag = contract_json::check_text(text, "tag", 1, std::numeric_limits<std::size_t>::max());
            }
            const auto query_order = query_values.find("order");
            if (query_order != query_values.end()) {
                const auto& text = query_order->second;
                if (text == "card_id_asc") order = holder::contract::ListCardsOrder::CardIdAsc;
                else if (text == "updated_desc") order = holder::contract::ListCardsOrder::UpdatedDesc;
                else if (text == "sort_key_asc") order = holder::contract::ListCardsOrder::SortKeyAsc;
                else throw std::invalid_argument("order must be one of card_id_asc, updated_desc, sort_key_asc");
            } else {
                order = holder::contract::ListCardsOrder::CardIdAsc;
            }
            const auto query_limit = query_values.find("limit");
            if (query_limit != query_values.end()) {
                const auto& text = query_limit->second;
                limit = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "limit"), "limit", 1, 5000);
            } else {
                limit = 200;
            }
            const auto query_cursor = query_values.find("cursor");
            if (query_cursor != query_values.end()) {
                const auto& text = query_cursor->second;
                cursor = contract_json::check_text(text, "cursor", 1, 8192);
            }
            const auto query_include_content = query_values.find("include_content");
            if (query_include_content != query_values.end()) {
                const auto& text = query_include_content->second;
                include_content = decode_bool(text, "include_content");
            } else {
                include_content = false;
            }
            const auto query_include_child_count = query_values.find("include_child_count");
            if (query_include_child_count != query_values.end()) {
                const auto& text = query_include_child_count->second;
                include_child_count = decode_bool(text, "include_child_count");
            } else {
                include_child_count = false;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_cards");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_cards(context, std::move(project_id), std::move(parent), std::move(tag), std::move(order), std::move(limit), std::move(cursor), std::move(include_content), std::move(include_child_count))), "list_cards");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_cards");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "list_cards");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "list_cards");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "list_cards");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_cards");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_cards");
        }
    }
    static const std::regex route_9("^/projects/([^/]+)/cards$");
    if (method == "POST" && std::regex_match(route_path, match, route_9)) {
        std::string project_id{};
        holder::contract::NewCard card{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card = contract_json::decode<holder::contract::NewCard>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "create_card");
        }
        try {
            return success(201, contract_json::encode(holder::contract::create_card(context, std::move(project_id), std::move(card))), "create_card");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "create_card");
        } catch (const holder::contract::Conflict& error) {
            return failure(409, "conflict", error.what(), "create_card");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "create_card");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "create_card");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "create_card");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "create_card");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "create_card");
        }
    }
    static const std::regex route_16("^/projects/([^/]+)/tags$");
    if (method == "GET" && std::regex_match(route_path, match, route_16)) {
        std::string project_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_project_tags");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_project_tags(context, std::move(project_id))), "list_project_tags");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_project_tags");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_project_tags");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_project_tags");
        }
    }
    static const std::regex route_27("^/projects/([^/]+)/calendar$");
    if (method == "GET" && std::regex_match(route_path, match, route_27)) {
        std::string project_id{};
        long long first_instant{};
        long long last_instant{};
        std::string first_day{};
        std::string last_day{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"first_instant", "last_instant", "first_day", "last_day"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_first_instant = query_values.find("first_instant");
            if (query_first_instant != query_values.end()) {
                const auto& text = query_first_instant->second;
                first_instant = decode_integer<long long>(text, "first_instant");
            } else {
                throw std::invalid_argument("the query parameter first_instant is required");
            }
            const auto query_last_instant = query_values.find("last_instant");
            if (query_last_instant != query_values.end()) {
                const auto& text = query_last_instant->second;
                last_instant = decode_integer<long long>(text, "last_instant");
            } else {
                throw std::invalid_argument("the query parameter last_instant is required");
            }
            const auto query_first_day = query_values.find("first_day");
            if (query_first_day != query_values.end()) {
                const auto& text = query_first_day->second;
                first_day = contract_json::check_text(text, "first_day", 10, 10);
            } else {
                throw std::invalid_argument("the query parameter first_day is required");
            }
            const auto query_last_day = query_values.find("last_day");
            if (query_last_day != query_values.end()) {
                const auto& text = query_last_day->second;
                last_day = contract_json::check_text(text, "last_day", 10, 10);
            } else {
                throw std::invalid_argument("the query parameter last_day is required");
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "get_calendar");
        }
        try {
            return success(200, contract_json::encode(holder::contract::get_calendar(context, std::move(project_id), std::move(first_instant), std::move(last_instant), std::move(first_day), std::move(last_day))), "get_calendar");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "get_calendar");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "get_calendar");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "get_calendar");
        }
    }
    static const std::regex route_28("^/projects/([^/]+)/resources$");
    if (method == "GET" && std::regex_match(route_path, match, route_28)) {
        std::string project_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_resources");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_resources(context, std::move(project_id))), "list_resources");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_resources");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_resources");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_resources");
        }
    }
    static const std::regex route_30("^/projects/([^/]+)/resources$");
    if (method == "POST" && std::regex_match(route_path, match, route_30)) {
        std::string project_id{};
        holder::contract::NewResource resource{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            resource = contract_json::decode<holder::contract::NewResource>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "create_resource");
        }
        try {
            return success(201, contract_json::encode(holder::contract::create_resource(context, std::move(project_id), std::move(resource))), "create_resource");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "create_resource");
        } catch (const holder::contract::Conflict& error) {
            return failure(409, "conflict", error.what(), "create_resource");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "create_resource");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "create_resource");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "create_resource");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "create_resource");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "create_resource");
        }
    }
    static const std::regex route_42("^/projects/([^/]+)/encryption\\-check$");
    if (method == "GET" && std::regex_match(route_path, match, route_42)) {
        std::string project_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "check_project_encryption");
        }
        try {
            return success(200, contract_json::encode(holder::contract::check_project_encryption(context, std::move(project_id))), "check_project_encryption");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "check_project_encryption");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "check_project_encryption");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "check_project_encryption");
        }
    }
    static const std::regex route_43("^/projects/([^/]+)/repair\\-hierarchy$");
    if (method == "POST" && std::regex_match(route_path, match, route_43)) {
        std::string project_id{};
        holder::contract::HierarchyRepairRequest request{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            request = contract_json::decode<holder::contract::HierarchyRepairRequest>(decode_body(body, true));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "repair_project_hierarchy");
        }
        try {
            return success(200, contract_json::encode(holder::contract::repair_project_hierarchy(context, std::move(project_id), std::move(request))), "repair_project_hierarchy");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "repair_project_hierarchy");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "repair_project_hierarchy");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "repair_project_hierarchy");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "repair_project_hierarchy");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "repair_project_hierarchy");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "repair_project_hierarchy");
        }
    }
    static const std::regex route_55("^/projects/([^/]+)/history$");
    if (method == "GET" && std::regex_match(route_path, match, route_55)) {
        std::string project_id{};
        std::int32_t limit{};
        std::optional<std::string> cursor{};
        std::optional<holder::contract::ListProjectHistoryKind> kind{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"limit", "cursor", "kind"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_limit = query_values.find("limit");
            if (query_limit != query_values.end()) {
                const auto& text = query_limit->second;
                limit = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "limit"), "limit", 1, 200);
            } else {
                limit = 50;
            }
            const auto query_cursor = query_values.find("cursor");
            if (query_cursor != query_values.end()) {
                const auto& text = query_cursor->second;
                cursor = contract_json::check_text(text, "cursor", 0, std::numeric_limits<std::size_t>::max());
            }
            const auto query_kind = query_values.find("kind");
            if (query_kind != query_values.end()) {
                const auto& text = query_kind->second;
                if (text == "card") kind = holder::contract::ListProjectHistoryKind::Card;
                else if (text == "resource") kind = holder::contract::ListProjectHistoryKind::Resource;
                else if (text == "location") kind = holder::contract::ListProjectHistoryKind::Location;
                else if (text == "ai_data") kind = holder::contract::ListProjectHistoryKind::AiData;
                else if (text == "project_settings") kind = holder::contract::ListProjectHistoryKind::ProjectSettings;
                else if (text == "unknown") kind = holder::contract::ListProjectHistoryKind::Unknown;
                else throw std::invalid_argument("kind must be one of card, resource, location, ai_data, project_settings, unknown");
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_project_history");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_project_history(context, std::move(project_id), std::move(limit), std::move(cursor), std::move(kind))), "list_project_history");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_project_history");
        } catch (const holder::contract::HistoryKeyUnavailable& error) {
            return failure(409, "history_key_unavailable", error.what(), "list_project_history");
        } catch (const holder::contract::HistoryResponseTooLarge& error) {
            return failure(413, "history_response_too_large", error.what(), "list_project_history");
        } catch (const holder::contract::HistoryUnavailable& error) {
            return failure(503, "history_unavailable", error.what(), "list_project_history");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_project_history");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_project_history");
        }
    }
    static const std::regex route_25("^/projects/([^/]+)/milestones/days$");
    if (method == "GET" && std::regex_match(route_path, match, route_25)) {
        std::string project_id{};
        std::string first_day{};
        std::string last_day{};
        std::optional<std::string> role{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"first_day", "last_day", "role"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_first_day = query_values.find("first_day");
            if (query_first_day != query_values.end()) {
                const auto& text = query_first_day->second;
                first_day = contract_json::check_text(text, "first_day", 10, 10);
            } else {
                throw std::invalid_argument("the query parameter first_day is required");
            }
            const auto query_last_day = query_values.find("last_day");
            if (query_last_day != query_values.end()) {
                const auto& text = query_last_day->second;
                last_day = contract_json::check_text(text, "last_day", 10, 10);
            } else {
                throw std::invalid_argument("the query parameter last_day is required");
            }
            const auto query_role = query_values.find("role");
            if (query_role != query_values.end()) {
                const auto& text = query_role->second;
                role = contract_json::check_text(text, "role", 1, std::numeric_limits<std::size_t>::max());
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_milestones_by_day");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_milestones_by_day(context, std::move(project_id), std::move(first_day), std::move(last_day), std::move(role))), "list_milestones_by_day");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_milestones_by_day");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_milestones_by_day");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_milestones_by_day");
        }
    }
    static const std::regex route_26("^/projects/([^/]+)/milestones/instants$");
    if (method == "GET" && std::regex_match(route_path, match, route_26)) {
        std::string project_id{};
        long long first_instant{};
        long long last_instant{};
        std::optional<std::string> role{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"first_instant", "last_instant", "role"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_first_instant = query_values.find("first_instant");
            if (query_first_instant != query_values.end()) {
                const auto& text = query_first_instant->second;
                first_instant = decode_integer<long long>(text, "first_instant");
            } else {
                throw std::invalid_argument("the query parameter first_instant is required");
            }
            const auto query_last_instant = query_values.find("last_instant");
            if (query_last_instant != query_values.end()) {
                const auto& text = query_last_instant->second;
                last_instant = decode_integer<long long>(text, "last_instant");
            } else {
                throw std::invalid_argument("the query parameter last_instant is required");
            }
            const auto query_role = query_values.find("role");
            if (query_role != query_values.end()) {
                const auto& text = query_role->second;
                role = contract_json::check_text(text, "role", 1, std::numeric_limits<std::size_t>::max());
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_milestones_by_instant");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_milestones_by_instant(context, std::move(project_id), std::move(first_instant), std::move(last_instant), std::move(role))), "list_milestones_by_instant");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_milestones_by_instant");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_milestones_by_instant");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_milestones_by_instant");
        }
    }
    static const std::regex route_46("^/projects/([^/]+)/recovery\\-token/export$");
    if (method == "POST" && std::regex_match(route_path, match, route_46)) {
        std::string project_id{};
        holder::contract::RecoveryTokenExportRequest request{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            request = contract_json::decode<holder::contract::RecoveryTokenExportRequest>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "export_project_recovery_token");
        }
        try {
            return success(200, contract_json::encode(holder::contract::export_project_recovery_token(context, std::move(project_id), std::move(request))), "export_project_recovery_token");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "export_project_recovery_token");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "export_project_recovery_token");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "export_project_recovery_token");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "export_project_recovery_token");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "export_project_recovery_token");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "export_project_recovery_token");
        }
    }
    static const std::regex route_47("^/projects/([^/]+)/recovery\\-token/import$");
    if (method == "POST" && std::regex_match(route_path, match, route_47)) {
        std::string project_id{};
        holder::contract::RecoveryTokenImportRequest request{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            request = contract_json::decode<holder::contract::RecoveryTokenImportRequest>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "import_project_recovery_token");
        }
        try {
            return success(200, contract_json::encode(holder::contract::import_project_recovery_token(context, std::move(project_id), std::move(request))), "import_project_recovery_token");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "import_project_recovery_token");
        } catch (const holder::contract::RecoveryTokenInvalid& error) {
            return failure(400, "recovery_token_invalid", error.what(), "import_project_recovery_token");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "import_project_recovery_token");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "import_project_recovery_token");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "import_project_recovery_token");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "import_project_recovery_token");
        }
    }
    static const std::regex route_49("^/projects/([^/]+)/git/sync\\-status$");
    if (method == "GET" && std::regex_match(route_path, match, route_49)) {
        std::string project_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "get_project_sync_state");
        }
        try {
            return success(200, contract_json::encode(holder::contract::get_project_sync_state(context, std::move(project_id))), "get_project_sync_state");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "get_project_sync_state");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "get_project_sync_state");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "get_project_sync_state");
        }
    }
    static const std::regex route_50("^/projects/([^/]+)/git/pull$");
    if (method == "POST" && std::regex_match(route_path, match, route_50)) {
        std::string project_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "pull_project");
        }
        try {
            return success(200, contract_json::encode(holder::contract::pull_project(context, std::move(project_id))), "pull_project");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "pull_project");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "pull_project");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "pull_project");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "pull_project");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "pull_project");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "pull_project");
        }
    }
    static const std::regex route_51("^/projects/([^/]+)/git/push$");
    if (method == "POST" && std::regex_match(route_path, match, route_51)) {
        std::string project_id{};
        holder::contract::PushOptions options{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            options = contract_json::decode<holder::contract::PushOptions>(decode_body(body, true));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "push_project");
        }
        try {
            return success(200, contract_json::encode(holder::contract::push_project(context, std::move(project_id), std::move(options))), "push_project");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "push_project");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "push_project");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "push_project");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "push_project");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "push_project");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "push_project");
        }
    }
    static const std::regex route_52("^/projects/([^/]+)/git/sync$");
    if (method == "POST" && std::regex_match(route_path, match, route_52)) {
        std::string project_id{};
        holder::contract::PushOptions options{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            options = contract_json::decode<holder::contract::PushOptions>(decode_body(body, true));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "sync_project");
        }
        try {
            return success(200, contract_json::encode(holder::contract::sync_project(context, std::move(project_id), std::move(options))), "sync_project");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "sync_project");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "sync_project");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "sync_project");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "sync_project");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "sync_project");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "sync_project");
        }
    }
    static const std::regex route_53("^/projects/([^/]+)/git/sync\\-if\\-due$");
    if (method == "POST" && std::regex_match(route_path, match, route_53)) {
        std::string project_id{};
        std::int32_t push_interval_seconds{};
        std::int32_t pull_interval_seconds{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"push_interval_seconds", "pull_interval_seconds"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_push_interval_seconds = query_values.find("push_interval_seconds");
            if (query_push_interval_seconds != query_values.end()) {
                const auto& text = query_push_interval_seconds->second;
                push_interval_seconds = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "push_interval_seconds"), "push_interval_seconds", 0, std::numeric_limits<std::int32_t>::max());
            } else {
                push_interval_seconds = 0;
            }
            const auto query_pull_interval_seconds = query_values.find("pull_interval_seconds");
            if (query_pull_interval_seconds != query_values.end()) {
                const auto& text = query_pull_interval_seconds->second;
                pull_interval_seconds = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "pull_interval_seconds"), "pull_interval_seconds", 0, std::numeric_limits<std::int32_t>::max());
            } else {
                pull_interval_seconds = 0;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "sync_project_if_due");
        }
        try {
            return success(200, contract_json::encode(holder::contract::sync_project_if_due(context, std::move(project_id), std::move(push_interval_seconds), std::move(pull_interval_seconds))), "sync_project_if_due");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "sync_project_if_due");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "sync_project_if_due");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "sync_project_if_due");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "sync_project_if_due");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "sync_project_if_due");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "sync_project_if_due");
        }
    }
    static const std::regex route_54("^/projects/([^/]+)/git/test\\-remote$");
    if (method == "POST" && std::regex_match(route_path, match, route_54)) {
        std::string project_id{};
        holder::contract::RemoteTest remote{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            remote = contract_json::decode<holder::contract::RemoteTest>(decode_body(body, true));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "test_project_remote");
        }
        try {
            return success(200, contract_json::encode(holder::contract::test_project_remote(context, std::move(project_id), std::move(remote))), "test_project_remote");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "test_project_remote");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "test_project_remote");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "test_project_remote");
        }
    }
    static const std::regex route_56("^/projects/([^/]+)/history/cards/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_56)) {
        std::string project_id{};
        std::string card_id{};
        std::int32_t limit{};
        std::optional<std::string> cursor{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"limit", "cursor"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_limit = query_values.find("limit");
            if (query_limit != query_values.end()) {
                const auto& text = query_limit->second;
                limit = contract_json::check_range<std::int32_t>(decode_integer<std::int32_t>(text, "limit"), "limit", 1, 200);
            } else {
                limit = 50;
            }
            const auto query_cursor = query_values.find("cursor");
            if (query_cursor != query_values.end()) {
                const auto& text = query_cursor->second;
                cursor = contract_json::check_text(text, "cursor", 0, std::numeric_limits<std::size_t>::max());
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_history");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_card_history(context, std::move(project_id), std::move(card_id), std::move(limit), std::move(cursor))), "list_card_history");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_card_history");
        } catch (const holder::contract::HistoryKeyUnavailable& error) {
            return failure(409, "history_key_unavailable", error.what(), "list_card_history");
        } catch (const holder::contract::HistoryResponseTooLarge& error) {
            return failure(413, "history_response_too_large", error.what(), "list_card_history");
        } catch (const holder::contract::HistoryUnavailable& error) {
            return failure(503, "history_unavailable", error.what(), "list_card_history");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_history");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_card_history");
        }
    }
    static const std::regex route_57("^/projects/([^/]+)/history/cards/([^/]+)/compare$");
    if (method == "GET" && std::regex_match(route_path, match, route_57)) {
        std::string project_id{};
        std::string card_id{};
        std::string to_oid{};
        std::optional<std::string> from_oid{};
        holder::contract::CompareCardHistoryMode mode{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"to_oid", "from_oid", "mode"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_to_oid = query_values.find("to_oid");
            if (query_to_oid != query_values.end()) {
                const auto& text = query_to_oid->second;
                to_oid = contract_json::check_text(text, "to_oid", 8, 40);
            } else {
                throw std::invalid_argument("the query parameter to_oid is required");
            }
            const auto query_from_oid = query_values.find("from_oid");
            if (query_from_oid != query_values.end()) {
                const auto& text = query_from_oid->second;
                from_oid = contract_json::check_text(text, "from_oid", 8, 40);
            }
            const auto query_mode = query_values.find("mode");
            if (query_mode != query_values.end()) {
                const auto& text = query_mode->second;
                if (text == "since") mode = holder::contract::CompareCardHistoryMode::Since;
                else if (text == "change") mode = holder::contract::CompareCardHistoryMode::Change;
                else throw std::invalid_argument("mode must be one of since, change");
            } else {
                mode = holder::contract::CompareCardHistoryMode::Since;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "compare_card_history");
        }
        try {
            return success(200, contract_json::encode(holder::contract::compare_card_history(context, std::move(project_id), std::move(card_id), std::move(to_oid), std::move(from_oid), std::move(mode))), "compare_card_history");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "compare_card_history");
        } catch (const holder::contract::RevisionAmbiguous& error) {
            return failure(409, "revision_ambiguous", error.what(), "compare_card_history");
        } catch (const holder::contract::HistoryKeyUnavailable& error) {
            return failure(409, "history_key_unavailable", error.what(), "compare_card_history");
        } catch (const holder::contract::HistoryResponseTooLarge& error) {
            return failure(413, "history_response_too_large", error.what(), "compare_card_history");
        } catch (const holder::contract::HistoryUnavailable& error) {
            return failure(503, "history_unavailable", error.what(), "compare_card_history");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "compare_card_history");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "compare_card_history");
        }
    }
    static const std::regex route_58("^/projects/([^/]+)/history/cards/([^/]+)/snapshot$");
    if (method == "GET" && std::regex_match(route_path, match, route_58)) {
        std::string project_id{};
        std::string card_id{};
        std::string oid{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"oid"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_oid = query_values.find("oid");
            if (query_oid != query_values.end()) {
                const auto& text = query_oid->second;
                oid = contract_json::check_text(text, "oid", 8, 40);
            } else {
                throw std::invalid_argument("the query parameter oid is required");
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "get_card_history_snapshot");
        }
        try {
            return success(200, contract_json::encode(holder::contract::get_card_history_snapshot(context, std::move(project_id), std::move(card_id), std::move(oid))), "get_card_history_snapshot");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "get_card_history_snapshot");
        } catch (const holder::contract::RevisionAmbiguous& error) {
            return failure(409, "revision_ambiguous", error.what(), "get_card_history_snapshot");
        } catch (const holder::contract::HistoryKeyUnavailable& error) {
            return failure(409, "history_key_unavailable", error.what(), "get_card_history_snapshot");
        } catch (const holder::contract::HistoryResponseTooLarge& error) {
            return failure(413, "history_response_too_large", error.what(), "get_card_history_snapshot");
        } catch (const holder::contract::HistoryUnavailable& error) {
            return failure(503, "history_unavailable", error.what(), "get_card_history_snapshot");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "get_card_history_snapshot");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "get_card_history_snapshot");
        }
    }
    static const std::regex route_59("^/projects/([^/]+)/history/cards/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, match, route_59)) {
        std::string project_id{};
        std::string card_id{};
        std::string oid{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"oid"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_oid = query_values.find("oid");
            if (query_oid != query_values.end()) {
                const auto& text = query_oid->second;
                oid = contract_json::check_text(text, "oid", 8, 40);
            } else {
                throw std::invalid_argument("the query parameter oid is required");
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "restore_card_history_snapshot");
        }
        try {
            return success(200, contract_json::encode(holder::contract::restore_card_history_snapshot(context, std::move(project_id), std::move(card_id), std::move(oid))), "restore_card_history_snapshot");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "restore_card_history_snapshot");
        } catch (const holder::contract::RevisionAmbiguous& error) {
            return failure(409, "revision_ambiguous", error.what(), "restore_card_history_snapshot");
        } catch (const holder::contract::HistoryKeyUnavailable& error) {
            return failure(409, "history_key_unavailable", error.what(), "restore_card_history_snapshot");
        } catch (const holder::contract::HistoryUnavailable& error) {
            return failure(503, "history_unavailable", error.what(), "restore_card_history_snapshot");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "restore_card_history_snapshot");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "restore_card_history_snapshot");
        }
    }
    static const std::regex route_4("^/projects/([^/]+)/cards/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_4)) {
        std::string project_id{};
        std::string card_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "get_card");
        }
        try {
            return success(200, contract_json::encode(holder::contract::get_card(context, std::move(project_id), std::move(card_id))), "get_card");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "get_card");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "get_card");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "get_card");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "get_card");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "get_card");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "get_card");
        }
    }
    static const std::regex route_10("^/projects/([^/]+)/cards/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_10)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::CardUpdate update{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            update = contract_json::decode<holder::contract::CardUpdate>(decode_body(body, true));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "update_card");
        }
        try {
            return success(200, contract_json::encode(holder::contract::update_card(context, std::move(project_id), std::move(card_id), std::move(update))), "update_card");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "update_card");
        } catch (const holder::contract::InvalidMove& error) {
            return failure(422, "invalid_move", error.what(), "update_card");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "update_card");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "update_card");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "update_card");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "update_card");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "update_card");
        }
    }
    static const std::regex route_11("^/projects/([^/]+)/cards/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_11)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::TrashCardChildren children{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"children"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_children = query_values.find("children");
            if (query_children != query_values.end()) {
                const auto& text = query_children->second;
                if (text == "promote") children = holder::contract::TrashCardChildren::Promote;
                else if (text == "trash_subtree") children = holder::contract::TrashCardChildren::TrashSubtree;
                else throw std::invalid_argument("children must be one of promote, trash_subtree");
            } else {
                children = holder::contract::TrashCardChildren::Promote;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "trash_card");
        }
        try {
            return success(200, contract_json::encode(holder::contract::trash_card(context, std::move(project_id), std::move(card_id), std::move(children))), "trash_card");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "trash_card");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "trash_card");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "trash_card");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "trash_card");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "trash_card");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "trash_card");
        }
    }
    static const std::regex route_29("^/projects/([^/]+)/resources/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_29)) {
        std::string project_id{};
        std::string resource_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            resource_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "resource_id", 4, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "get_resource");
        }
        try {
            return success(200, contract_json::encode(holder::contract::get_resource(context, std::move(project_id), std::move(resource_id))), "get_resource");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "get_resource");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "get_resource");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "get_resource");
        }
    }
    static const std::regex route_31("^/projects/([^/]+)/resources/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_31)) {
        std::string project_id{};
        std::string resource_id{};
        holder::contract::ResourceUpdate update{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            resource_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "resource_id", 4, std::numeric_limits<std::size_t>::max());
            update = contract_json::decode<holder::contract::ResourceUpdate>(decode_body(body, true));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "update_resource");
        }
        try {
            return success(200, contract_json::encode(holder::contract::update_resource(context, std::move(project_id), std::move(resource_id), std::move(update))), "update_resource");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "update_resource");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "update_resource");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "update_resource");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "update_resource");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "update_resource");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "update_resource");
        }
    }
    static const std::regex route_32("^/projects/([^/]+)/resources/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_32)) {
        std::string project_id{};
        std::string resource_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            resource_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "resource_id", 4, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_resource");
        }
        try {
            return success(200, contract_json::encode(holder::contract::delete_resource(context, std::move(project_id), std::move(resource_id))), "delete_resource");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "delete_resource");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "delete_resource");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "delete_resource");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "delete_resource");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_resource");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "delete_resource");
        }
    }
    static const std::regex route_5("^/projects/([^/]+)/cards/([^/]+)/metadata$");
    if (method == "GET" && std::regex_match(route_path, match, route_5)) {
        std::string project_id{};
        std::string card_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "get_card_metadata");
        }
        try {
            return success(200, contract_json::encode(holder::contract::get_card_metadata(context, std::move(project_id), std::move(card_id))), "get_card_metadata");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "get_card_metadata");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "get_card_metadata");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "get_card_metadata");
        }
    }
    static const std::regex route_6("^/projects/([^/]+)/cards/([^/]+)/breadcrumbs$");
    if (method == "GET" && std::regex_match(route_path, match, route_6)) {
        std::string project_id{};
        std::string card_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "get_card_breadcrumbs");
        }
        try {
            return success(200, contract_json::encode(holder::contract::get_card_breadcrumbs(context, std::move(project_id), std::move(card_id))), "get_card_breadcrumbs");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "get_card_breadcrumbs");
        } catch (const holder::contract::HierarchyInvalid& error) {
            return failure(409, "hierarchy_invalid", error.what(), "get_card_breadcrumbs");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "get_card_breadcrumbs");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "get_card_breadcrumbs");
        }
    }
    static const std::regex route_12("^/projects/([^/]+)/cards/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, match, route_12)) {
        std::string project_id{};
        std::string card_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "restore_card");
        }
        try {
            return success(200, contract_json::encode(holder::contract::restore_card(context, std::move(project_id), std::move(card_id))), "restore_card");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "restore_card");
        } catch (const holder::contract::Conflict& error) {
            return failure(409, "conflict", error.what(), "restore_card");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "restore_card");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "restore_card");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "restore_card");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "restore_card");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "restore_card");
        }
    }
    static const std::regex route_13("^/projects/([^/]+)/cards/([^/]+)/move$");
    if (method == "POST" && std::regex_match(route_path, match, route_13)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::CardMove move{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            move = contract_json::decode<holder::contract::CardMove>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "move_card");
        }
        try {
            return success(200, contract_json::encode(holder::contract::move_card(context, std::move(project_id), std::move(card_id), std::move(move))), "move_card");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "move_card");
        } catch (const holder::contract::InvalidMove& error) {
            return failure(422, "invalid_move", error.what(), "move_card");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "move_card");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "move_card");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "move_card");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "move_card");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "move_card");
        }
    }
    static const std::regex route_17("^/projects/([^/]+)/cards/([^/]+)/links$");
    if (method == "GET" && std::regex_match(route_path, match, route_17)) {
        std::string project_id{};
        std::string card_id{};
        bool include_deleted{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"include_deleted"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_include_deleted = query_values.find("include_deleted");
            if (query_include_deleted != query_values.end()) {
                const auto& text = query_include_deleted->second;
                include_deleted = decode_bool(text, "include_deleted");
            } else {
                include_deleted = false;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_links");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_card_links(context, std::move(project_id), std::move(card_id), std::move(include_deleted))), "list_card_links");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_card_links");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_links");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_card_links");
        }
    }
    static const std::regex route_18("^/projects/([^/]+)/cards/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, match, route_18)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::ListCardBacklinksScope scope{};
        bool include_deleted{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"scope", "include_deleted"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_scope = query_values.find("scope");
            if (query_scope != query_values.end()) {
                const auto& text = query_scope->second;
                if (text == "local") scope = holder::contract::ListCardBacklinksScope::Local;
                else if (text == "external") scope = holder::contract::ListCardBacklinksScope::External;
                else if (text == "all_available") scope = holder::contract::ListCardBacklinksScope::AllAvailable;
                else throw std::invalid_argument("scope must be one of local, external, all_available");
            } else {
                scope = holder::contract::ListCardBacklinksScope::Local;
            }
            const auto query_include_deleted = query_values.find("include_deleted");
            if (query_include_deleted != query_values.end()) {
                const auto& text = query_include_deleted->second;
                include_deleted = decode_bool(text, "include_deleted");
            } else {
                include_deleted = false;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_backlinks");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_card_backlinks(context, std::move(project_id), std::move(card_id), std::move(scope), std::move(include_deleted))), "list_card_backlinks");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_card_backlinks");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_backlinks");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_card_backlinks");
        }
    }
    static const std::regex route_19("^/projects/([^/]+)/cards/([^/]+)/links$");
    if (method == "POST" && std::regex_match(route_path, match, route_19)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::NewLink link{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            link = contract_json::decode<holder::contract::NewLink>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "add_card_link");
        }
        try {
            return success(200, contract_json::encode(holder::contract::add_card_link(context, std::move(project_id), std::move(card_id), std::move(link))), "add_card_link");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "add_card_link");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "add_card_link");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "add_card_link");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "add_card_link");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "add_card_link");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "add_card_link");
        }
    }
    static const std::regex route_20("^/projects/([^/]+)/cards/([^/]+)/links$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_20)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::RemoveCardLinkTargetType target_type{};
        std::string target_id{};
        std::string kind{};
        std::optional<std::string> target_project_id{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"target_type", "target_id", "kind", "target_project_id"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            const auto query_target_type = query_values.find("target_type");
            if (query_target_type != query_values.end()) {
                const auto& text = query_target_type->second;
                if (text == "card") target_type = holder::contract::RemoveCardLinkTargetType::Card;
                else if (text == "ai_message") target_type = holder::contract::RemoveCardLinkTargetType::AiMessage;
                else if (text == "ai_thread") target_type = holder::contract::RemoveCardLinkTargetType::AiThread;
                else if (text == "resource") target_type = holder::contract::RemoveCardLinkTargetType::Resource;
                else throw std::invalid_argument("target_type must be one of card, ai_message, ai_thread, resource");
            } else {
                throw std::invalid_argument("the query parameter target_type is required");
            }
            const auto query_target_id = query_values.find("target_id");
            if (query_target_id != query_values.end()) {
                const auto& text = query_target_id->second;
                target_id = contract_json::check_text(text, "target_id", 1, std::numeric_limits<std::size_t>::max());
            } else {
                throw std::invalid_argument("the query parameter target_id is required");
            }
            const auto query_kind = query_values.find("kind");
            if (query_kind != query_values.end()) {
                const auto& text = query_kind->second;
                kind = contract_json::check_text(text, "kind", 1, std::numeric_limits<std::size_t>::max());
            } else {
                kind = "ref";
            }
            const auto query_target_project_id = query_values.find("target_project_id");
            if (query_target_project_id != query_values.end()) {
                const auto& text = query_target_project_id->second;
                target_project_id = contract_json::check_text(text, "target_project_id", 1, std::numeric_limits<std::size_t>::max());
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "remove_card_link");
        }
        try {
            return success(200, contract_json::encode(holder::contract::remove_card_link(context, std::move(project_id), std::move(card_id), std::move(target_type), std::move(target_id), std::move(kind), std::move(target_project_id))), "remove_card_link");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "remove_card_link");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "remove_card_link");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "remove_card_link");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "remove_card_link");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "remove_card_link");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "remove_card_link");
        }
    }
    static const std::regex route_21("^/projects/([^/]+)/cards/([^/]+)/milestones$");
    if (method == "GET" && std::regex_match(route_path, match, route_21)) {
        std::string project_id{};
        std::string card_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_milestones");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_card_milestones(context, std::move(project_id), std::move(card_id))), "list_card_milestones");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_card_milestones");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_milestones");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_card_milestones");
        }
    }
    static const std::regex route_22("^/projects/([^/]+)/cards/([^/]+)/milestones$");
    if (method == "POST" && std::regex_match(route_path, match, route_22)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::NewMilestone milestone{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            milestone = contract_json::decode<holder::contract::NewMilestone>(decode_body(body));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "add_card_milestone");
        }
        try {
            return success(201, contract_json::encode(holder::contract::add_card_milestone(context, std::move(project_id), std::move(card_id), std::move(milestone))), "add_card_milestone");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "add_card_milestone");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "add_card_milestone");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "add_card_milestone");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "add_card_milestone");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "add_card_milestone");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "add_card_milestone");
        }
    }
    static const std::regex route_33("^/projects/([^/]+)/resources/([^/]+)/backlinks$");
    if (method == "GET" && std::regex_match(route_path, match, route_33)) {
        std::string project_id{};
        std::string resource_id{};
        bool include_deleted{};
        try {
            const auto query_values = decode_query(query_string);
            check_query_names(query_values, {"include_deleted"});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            resource_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "resource_id", 4, std::numeric_limits<std::size_t>::max());
            const auto query_include_deleted = query_values.find("include_deleted");
            if (query_include_deleted != query_values.end()) {
                const auto& text = query_include_deleted->second;
                include_deleted = decode_bool(text, "include_deleted");
            } else {
                include_deleted = false;
            }
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_resource_backlinks");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_resource_backlinks(context, std::move(project_id), std::move(resource_id), std::move(include_deleted))), "list_resource_backlinks");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_resource_backlinks");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_resource_backlinks");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_resource_backlinks");
        }
    }
    static const std::regex route_34("^/projects/([^/]+)/cards/([^/]+)/resources$");
    if (method == "GET" && std::regex_match(route_path, match, route_34)) {
        std::string project_id{};
        std::string card_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_resources");
        }
        try {
            return success(200, contract_json::encode(holder::contract::list_card_resources(context, std::move(project_id), std::move(card_id))), "list_card_resources");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "list_card_resources");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "list_card_resources");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "list_card_resources");
        }
    }
    static const std::regex route_14("^/projects/([^/]+)/cards/([^/]+)/tags/([^/]+)$");
    if (method == "PUT" && std::regex_match(route_path, match, route_14)) {
        std::string project_id{};
        std::string card_id{};
        std::string tag{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            tag = contract_json::check_text(decode_uri_component(match[3].str(), false), "tag", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "add_card_tag");
        }
        try {
            return success(200, contract_json::encode(holder::contract::add_card_tag(context, std::move(project_id), std::move(card_id), std::move(tag))), "add_card_tag");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "add_card_tag");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "add_card_tag");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "add_card_tag");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "add_card_tag");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "add_card_tag");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "add_card_tag");
        }
    }
    static const std::regex route_15("^/projects/([^/]+)/cards/([^/]+)/tags/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_15)) {
        std::string project_id{};
        std::string card_id{};
        std::string tag{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            tag = contract_json::check_text(decode_uri_component(match[3].str(), false), "tag", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "remove_card_tag");
        }
        try {
            return success(200, contract_json::encode(holder::contract::remove_card_tag(context, std::move(project_id), std::move(card_id), std::move(tag))), "remove_card_tag");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "remove_card_tag");
        } catch (const holder::contract::Conflict& error) {
            return failure(409, "conflict", error.what(), "remove_card_tag");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "remove_card_tag");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "remove_card_tag");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "remove_card_tag");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "remove_card_tag");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "remove_card_tag");
        }
    }
    static const std::regex route_23("^/projects/([^/]+)/cards/([^/]+)/milestones/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_23)) {
        std::string project_id{};
        std::string card_id{};
        std::string milestone_id{};
        holder::contract::MilestoneUpdate update{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            milestone_id = contract_json::check_text(decode_uri_component(match[3].str(), false), "milestone_id", 1, std::numeric_limits<std::size_t>::max());
            update = contract_json::decode<holder::contract::MilestoneUpdate>(decode_body(body, true));
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "update_card_milestone");
        }
        try {
            return success(200, contract_json::encode(holder::contract::update_card_milestone(context, std::move(project_id), std::move(card_id), std::move(milestone_id), std::move(update))), "update_card_milestone");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "update_card_milestone");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "update_card_milestone");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "update_card_milestone");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "update_card_milestone");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "update_card_milestone");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "update_card_milestone");
        }
    }
    static const std::regex route_24("^/projects/([^/]+)/cards/([^/]+)/milestones/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_24)) {
        std::string project_id{};
        std::string card_id{};
        std::string milestone_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            milestone_id = contract_json::check_text(decode_uri_component(match[3].str(), false), "milestone_id", 1, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "remove_card_milestone");
        }
        try {
            return success(200, contract_json::encode(holder::contract::remove_card_milestone(context, std::move(project_id), std::move(card_id), std::move(milestone_id))), "remove_card_milestone");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "remove_card_milestone");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "remove_card_milestone");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "remove_card_milestone");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "remove_card_milestone");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "remove_card_milestone");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "remove_card_milestone");
        }
    }
    static const std::regex route_35("^/projects/([^/]+)/cards/([^/]+)/resources/([^/]+)$");
    if (method == "PUT" && std::regex_match(route_path, match, route_35)) {
        std::string project_id{};
        std::string card_id{};
        std::string resource_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            resource_id = contract_json::check_text(decode_uri_component(match[3].str(), false), "resource_id", 4, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "attach_card_resource");
        }
        try {
            return success(200, contract_json::encode(holder::contract::attach_card_resource(context, std::move(project_id), std::move(card_id), std::move(resource_id))), "attach_card_resource");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "attach_card_resource");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "attach_card_resource");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "attach_card_resource");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "attach_card_resource");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "attach_card_resource");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "attach_card_resource");
        }
    }
    static const std::regex route_36("^/projects/([^/]+)/cards/([^/]+)/resources/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_36)) {
        std::string project_id{};
        std::string card_id{};
        std::string resource_id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            resource_id = contract_json::check_text(decode_uri_component(match[3].str(), false), "resource_id", 4, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "detach_card_resource");
        }
        try {
            return success(200, contract_json::encode(holder::contract::detach_card_resource(context, std::move(project_id), std::move(card_id), std::move(resource_id))), "detach_card_resource");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "detach_card_resource");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "detach_card_resource");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "detach_card_resource");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "detach_card_resource");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "detach_card_resource");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "detach_card_resource");
        }
    }
    static const std::regex route_63("^/projects/([^/]+)/trash/([^/]+)/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_63)) {
        std::string project_id{};
        holder::contract::DeleteProjectTrashItemType type{};
        std::string id{};
        try {
            check_query_names(decode_query(query_string), {});
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            const auto type_text = decode_uri_component(match[2].str(), false);
            if (type_text == "card") type = holder::contract::DeleteProjectTrashItemType::Card;
            else if (type_text == "ai_message") type = holder::contract::DeleteProjectTrashItemType::AiMessage;
            else throw std::invalid_argument("type must be one of card, ai_message");
            id = contract_json::check_text(decode_uri_component(match[3].str(), false), "id", 0, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_project_trash_item");
        }
        try {
            return success(200, contract_json::encode(holder::contract::delete_project_trash_item(context, std::move(project_id), std::move(type), std::move(id))), "delete_project_trash_item");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "delete_project_trash_item");
        } catch (const holder::contract::NotTrashed& error) {
            return failure(409, "not_trashed", error.what(), "delete_project_trash_item");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "delete_project_trash_item");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "delete_project_trash_item");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "delete_project_trash_item");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_project_trash_item");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "delete_project_trash_item");
        }
    }
    static const std::regex route_62("^/trash/([^/]+)/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_62)) {
        holder::contract::DeleteTrashItemType type{};
        std::string id{};
        try {
            check_query_names(decode_query(query_string), {});
            const auto type_text = decode_uri_component(match[1].str(), false);
            if (type_text == "card") type = holder::contract::DeleteTrashItemType::Card;
            else if (type_text == "ai_message") type = holder::contract::DeleteTrashItemType::AiMessage;
            else throw std::invalid_argument("type must be one of card, ai_message");
            id = contract_json::check_text(decode_uri_component(match[2].str(), false), "id", 0, std::numeric_limits<std::size_t>::max());
        } catch (const std::invalid_argument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_trash_item");
        }
        try {
            return success(200, contract_json::encode(holder::contract::delete_trash_item(context, std::move(type), std::move(id))), "delete_trash_item");
        } catch (const holder::contract::NotFound& error) {
            return failure(404, "not_found", error.what(), "delete_trash_item");
        } catch (const holder::contract::NotTrashed& error) {
            return failure(409, "not_trashed", error.what(), "delete_trash_item");
        } catch (const holder::contract::KeyringUnavailable& error) {
            return failure(503, "keyring_unavailable", error.what(), "delete_trash_item");
        } catch (const holder::contract::KeyMissing& error) {
            return failure(409, "key_missing", error.what(), "delete_trash_item");
        } catch (const holder::contract::DataCorrupted& error) {
            return failure(500, "data_corrupted", error.what(), "delete_trash_item");
        } catch (const holder::contract::InvalidArgument& error) {
            return failure(400, "invalid_argument", error.what(), "delete_trash_item");
        } catch (const std::exception& error) {
            return failure(500, "internal_error", error.what(), "delete_trash_item");
        }
    }
    return std::nullopt;
}

// The methods dispatch answers on this path, for a 405's Allow header.
inline std::set<std::string_view> allowed_methods(const std::string& path) {
    const auto route_path = path.substr(0, path.find('?'));
    std::set<std::string_view> methods;
    static const std::regex path_0("^/locations$");
    if (std::regex_match(route_path, path_0)) return {"GET", "POST"};
    static const std::regex path_1("^/imports$");
    if (std::regex_match(route_path, path_1)) return {"POST"};
    static const std::regex path_2("^/projects$");
    if (std::regex_match(route_path, path_2)) return {"GET", "POST"};
    static const std::regex path_3("^/rebuild$");
    if (std::regex_match(route_path, path_3)) return {"POST"};
    static const std::regex path_4("^/reindex$");
    if (std::regex_match(route_path, path_4)) return {"POST"};
    static const std::regex path_5("^/trash$");
    if (std::regex_match(route_path, path_5)) return {"DELETE", "GET"};
    static const std::regex path_6("^/alive$");
    if (std::regex_match(route_path, path_6)) return {"GET"};
    static const std::regex path_7("^/ping$");
    if (std::regex_match(route_path, path_7)) return {"GET"};
    static const std::regex path_8("^/health$");
    if (std::regex_match(route_path, path_8)) return {"GET"};
    static const std::regex path_9("^/bye$");
    if (std::regex_match(route_path, path_9)) return {"POST"};
    static const std::regex path_10("^/events$");
    if (std::regex_match(route_path, path_10)) return {"GET"};
    static const std::regex path_11("^/openapi\\.yaml$");
    if (std::regex_match(route_path, path_11)) return {"GET"};
    static const std::regex path_12("^/openapi\\.json$");
    if (std::regex_match(route_path, path_12)) return {"GET"};
    static const std::regex path_13("^/ai_catalog\\.yaml$");
    if (std::regex_match(route_path, path_13)) return {"GET"};
    static const std::regex path_14("^/ai_catalog\\.json$");
    if (std::regex_match(route_path, path_14)) return {"GET"};
    static const std::regex path_15("^/git_providers\\.yaml$");
    if (std::regex_match(route_path, path_15)) return {"GET"};
    static const std::regex path_16("^/git_providers\\.json$");
    if (std::regex_match(route_path, path_16)) return {"GET"};
    static const std::regex path_17("^/docs$");
    if (std::regex_match(route_path, path_17)) return {"GET"};
    static const std::regex path_18("^/locations/preferred$");
    if (std::regex_match(route_path, path_18)) return {"PUT"};
    static const std::regex path_19("^/card\\-references/resolve$");
    if (std::regex_match(route_path, path_19)) return {"POST"};
    static const std::regex path_20("^/recovery\\-token/import$");
    if (std::regex_match(route_path, path_20)) return {"POST"};
    static const std::regex path_21("^/search/cards$");
    if (std::regex_match(route_path, path_21)) return {"GET"};
    static const std::regex path_22("^/search/ai$");
    if (std::regex_match(route_path, path_22)) return {"GET"};
    static const std::regex path_23("^/events/cursor$");
    if (std::regex_match(route_path, path_23)) return {"GET"};
    static const std::regex path_24("^/ai/messages$");
    if (std::regex_match(route_path, path_24)) return {"GET", "POST"};
    static const std::regex path_25("^/ai/nudges$");
    if (std::regex_match(route_path, path_25)) return {"GET"};
    static const std::regex path_26("^/ai/runners$");
    if (std::regex_match(route_path, path_26)) return {"GET", "POST"};
    static const std::regex path_27("^/ai/runs$");
    if (std::regex_match(route_path, path_27)) return {"GET", "POST"};
    static const std::regex path_28("^/ai/capabilities$");
    if (std::regex_match(route_path, path_28)) return {"GET"};
    static const std::regex path_29("^/ai/status$");
    if (std::regex_match(route_path, path_29)) return {"GET"};
    static const std::regex path_30("^/ai/threads$");
    if (std::regex_match(route_path, path_30)) return {"GET", "POST"};
    static const std::regex path_31("^/ai/runner/pull$");
    if (std::regex_match(route_path, path_31)) return {"POST"};
    static const std::regex path_32("^/ai/providers/credentials$");
    if (std::regex_match(route_path, path_32)) return {"GET", "PUT"};
    static const std::regex path_33("^/ai/local\\-models/config$");
    if (std::regex_match(route_path, path_33)) return {"GET", "PUT"};
    static const std::regex path_34("^/ai/messages/capture$");
    if (std::regex_match(route_path, path_34)) return {"POST"};
    static const std::regex path_35("^/ai/nudges/evaluate$");
    if (std::regex_match(route_path, path_35)) return {"POST"};
    static const std::regex path_36("^/ai/providers/catalog$");
    if (std::regex_match(route_path, path_36)) return {"GET"};
    static const std::regex path_37("^/ai/providers/settings$");
    if (std::regex_match(route_path, path_37)) return {"GET", "PUT"};
    static const std::regex path_38("^/ai/runner/pull/([^/]+)$");
    if (std::regex_match(route_path, path_38)) return {"GET"};
    static const std::regex path_39("^/ai/providers/credentials/([^/]+)$");
    if (std::regex_match(route_path, path_39)) return {"DELETE"};
    static const std::regex path_40("^/ai/providers/settings/([^/]+)$");
    if (std::regex_match(route_path, path_40)) return {"DELETE"};
    static const std::regex path_41("^/ai/runner/pull/([^/]+)/events$");
    if (std::regex_match(route_path, path_41)) return {"GET"};
    static const std::regex path_42("^/ai/messages/([^/]+)$");
    if (std::regex_match(route_path, path_42)) return {"DELETE", "GET", "PATCH"};
    static const std::regex path_43("^/ai/runners/([^/]+)$");
    if (std::regex_match(route_path, path_43)) return {"DELETE", "GET", "PATCH"};
    static const std::regex path_44("^/ai/runs/([^/]+)$");
    if (std::regex_match(route_path, path_44)) return {"GET"};
    static const std::regex path_45("^/ai/threads/([^/]+)$");
    if (std::regex_match(route_path, path_45)) return {"DELETE", "GET", "PATCH"};
    static const std::regex path_46("^/ai/messages/([^/]+)/links$");
    if (std::regex_match(route_path, path_46)) return {"DELETE", "GET", "POST"};
    static const std::regex path_47("^/ai/messages/([^/]+)/backlinks$");
    if (std::regex_match(route_path, path_47)) return {"GET"};
    static const std::regex path_48("^/ai/messages/([^/]+)/restore$");
    if (std::regex_match(route_path, path_48)) return {"POST"};
    static const std::regex path_49("^/ai/nudges/([^/]+)/dismiss$");
    if (std::regex_match(route_path, path_49)) return {"POST"};
    static const std::regex path_50("^/ai/runners/([^/]+)/retry$");
    if (std::regex_match(route_path, path_50)) return {"POST"};
    static const std::regex path_51("^/ai/runs/([^/]+)/events$");
    if (std::regex_match(route_path, path_51)) return {"GET"};
    static const std::regex path_52("^/locations/([^/]+)$");
    if (std::regex_match(route_path, path_52)) return {"DELETE", "GET", "PATCH"};
    static const std::regex path_53("^/imports/([^/]+)$");
    if (std::regex_match(route_path, path_53)) return {"GET"};
    static const std::regex path_54("^/projects/([^/]+)$");
    if (std::regex_match(route_path, path_54)) return {"DELETE", "GET", "PATCH"};
    static const std::regex path_55("^/docs/([^/]+)$");
    if (std::regex_match(route_path, path_55)) return {"GET"};
    static const std::regex path_56("^/locations/([^/]+)/binding$");
    if (std::regex_match(route_path, path_56)) return {"DELETE", "PUT"};
    static const std::regex path_57("^/locations/([^/]+)/test$");
    if (std::regex_match(route_path, path_57)) return {"POST"};
    static const std::regex path_58("^/projects/([^/]+)/cards$");
    if (std::regex_match(route_path, path_58)) return {"GET", "POST"};
    static const std::regex path_59("^/projects/([^/]+)/tags$");
    if (std::regex_match(route_path, path_59)) return {"GET"};
    static const std::regex path_60("^/projects/([^/]+)/calendar$");
    if (std::regex_match(route_path, path_60)) return {"GET"};
    static const std::regex path_61("^/projects/([^/]+)/resources$");
    if (std::regex_match(route_path, path_61)) return {"GET", "POST"};
    static const std::regex path_62("^/projects/([^/]+)/encryption\\-check$");
    if (std::regex_match(route_path, path_62)) return {"GET"};
    static const std::regex path_63("^/projects/([^/]+)/repair\\-hierarchy$");
    if (std::regex_match(route_path, path_63)) return {"POST"};
    static const std::regex path_64("^/projects/([^/]+)/history$");
    if (std::regex_match(route_path, path_64)) return {"GET"};
    static const std::regex path_65("^/projects/([^/]+)/milestones/days$");
    if (std::regex_match(route_path, path_65)) return {"GET"};
    static const std::regex path_66("^/projects/([^/]+)/milestones/instants$");
    if (std::regex_match(route_path, path_66)) return {"GET"};
    static const std::regex path_67("^/projects/([^/]+)/recovery\\-token/export$");
    if (std::regex_match(route_path, path_67)) return {"POST"};
    static const std::regex path_68("^/projects/([^/]+)/recovery\\-token/import$");
    if (std::regex_match(route_path, path_68)) return {"POST"};
    static const std::regex path_69("^/projects/([^/]+)/git/sync\\-status$");
    if (std::regex_match(route_path, path_69)) return {"GET"};
    static const std::regex path_70("^/projects/([^/]+)/git/pull$");
    if (std::regex_match(route_path, path_70)) return {"POST"};
    static const std::regex path_71("^/projects/([^/]+)/git/push$");
    if (std::regex_match(route_path, path_71)) return {"POST"};
    static const std::regex path_72("^/projects/([^/]+)/git/sync$");
    if (std::regex_match(route_path, path_72)) return {"POST"};
    static const std::regex path_73("^/projects/([^/]+)/git/sync\\-if\\-due$");
    if (std::regex_match(route_path, path_73)) return {"POST"};
    static const std::regex path_74("^/projects/([^/]+)/git/test\\-remote$");
    if (std::regex_match(route_path, path_74)) return {"POST"};
    static const std::regex path_75("^/locations/([^/]+)/oauth/google\\-drive/authorize$");
    if (std::regex_match(route_path, path_75)) return {"POST"};
    static const std::regex path_76("^/locations/([^/]+)/oauth/google\\-drive/callback$");
    if (std::regex_match(route_path, path_76)) return {"GET"};
    static const std::regex path_77("^/projects/([^/]+)/history/cards/([^/]+)$");
    if (std::regex_match(route_path, path_77)) return {"GET"};
    static const std::regex path_78("^/projects/([^/]+)/history/cards/([^/]+)/compare$");
    if (std::regex_match(route_path, path_78)) return {"GET"};
    static const std::regex path_79("^/projects/([^/]+)/history/cards/([^/]+)/snapshot$");
    if (std::regex_match(route_path, path_79)) return {"GET"};
    static const std::regex path_80("^/projects/([^/]+)/history/cards/([^/]+)/restore$");
    if (std::regex_match(route_path, path_80)) return {"POST"};
    static const std::regex path_81("^/projects/([^/]+)/cards/([^/]+)$");
    if (std::regex_match(route_path, path_81)) return {"DELETE", "GET", "PATCH"};
    static const std::regex path_82("^/projects/([^/]+)/resources/([^/]+)$");
    if (std::regex_match(route_path, path_82)) return {"DELETE", "GET", "PATCH"};
    static const std::regex path_83("^/projects/([^/]+)/cards/([^/]+)/metadata$");
    if (std::regex_match(route_path, path_83)) return {"GET"};
    static const std::regex path_84("^/projects/([^/]+)/cards/([^/]+)/breadcrumbs$");
    if (std::regex_match(route_path, path_84)) return {"GET"};
    static const std::regex path_85("^/projects/([^/]+)/cards/([^/]+)/restore$");
    if (std::regex_match(route_path, path_85)) return {"POST"};
    static const std::regex path_86("^/projects/([^/]+)/cards/([^/]+)/move$");
    if (std::regex_match(route_path, path_86)) return {"POST"};
    static const std::regex path_87("^/projects/([^/]+)/cards/([^/]+)/links$");
    if (std::regex_match(route_path, path_87)) return {"DELETE", "GET", "POST"};
    static const std::regex path_88("^/projects/([^/]+)/cards/([^/]+)/backlinks$");
    if (std::regex_match(route_path, path_88)) return {"GET"};
    static const std::regex path_89("^/projects/([^/]+)/cards/([^/]+)/milestones$");
    if (std::regex_match(route_path, path_89)) return {"GET", "POST"};
    static const std::regex path_90("^/projects/([^/]+)/resources/([^/]+)/backlinks$");
    if (std::regex_match(route_path, path_90)) return {"GET"};
    static const std::regex path_91("^/projects/([^/]+)/cards/([^/]+)/resources$");
    if (std::regex_match(route_path, path_91)) return {"GET"};
    static const std::regex path_92("^/resources/([^/]+)/assets/([^/]+)/content$");
    if (std::regex_match(route_path, path_92)) return {"GET"};
    static const std::regex path_93("^/projects/([^/]+)/cards/([^/]+)/tags/([^/]+)$");
    if (std::regex_match(route_path, path_93)) return {"DELETE", "PUT"};
    static const std::regex path_94("^/projects/([^/]+)/cards/([^/]+)/milestones/([^/]+)$");
    if (std::regex_match(route_path, path_94)) return {"DELETE", "PATCH"};
    static const std::regex path_95("^/projects/([^/]+)/cards/([^/]+)/resources/([^/]+)$");
    if (std::regex_match(route_path, path_95)) return {"DELETE", "PUT"};
    static const std::regex path_96("^/projects/([^/]+)/trash/([^/]+)/([^/]+)$");
    if (std::regex_match(route_path, path_96)) return {"DELETE"};
    static const std::regex path_97("^/trash/([^/]+)/([^/]+)$");
    if (std::regex_match(route_path, path_97)) return {"DELETE"};
    return methods;
}
// Public routes do not require the daemon's bearer token.
inline bool is_public(const std::string& method, const std::string& path) {
    const auto route_path = path.substr(0, path.find('?'));
    static const std::regex public_0("^/locations/([^/]+)/oauth/google\\-drive/callback$");
    if (method == "GET" && std::regex_match(route_path, public_0)) return true;
    static const std::regex public_1("^/alive$");
    if (method == "GET" && std::regex_match(route_path, public_1)) return true;
    static const std::regex public_2("^/ping$");
    if (method == "GET" && std::regex_match(route_path, public_2)) return true;
    static const std::regex public_3("^/openapi\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, public_3)) return true;
    static const std::regex public_4("^/openapi\\.json$");
    if (method == "GET" && std::regex_match(route_path, public_4)) return true;
    static const std::regex public_5("^/ai_catalog\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, public_5)) return true;
    static const std::regex public_6("^/ai_catalog\\.json$");
    if (method == "GET" && std::regex_match(route_path, public_6)) return true;
    static const std::regex public_7("^/git_providers\\.yaml$");
    if (method == "GET" && std::regex_match(route_path, public_7)) return true;
    static const std::regex public_8("^/git_providers\\.json$");
    if (method == "GET" && std::regex_match(route_path, public_8)) return true;
    static const std::regex public_9("^/docs$");
    if (method == "GET" && std::regex_match(route_path, public_9)) return true;
    static const std::regex public_10("^/docs/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, public_10)) return true;
    return false;
}
}  // namespace contract_http
