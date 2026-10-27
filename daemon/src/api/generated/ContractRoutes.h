// Generated from contract/interface/app.py, contract/interface/cards.py, contract/interface/errors.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// The daemon's HTTP routes for the contract's operations; contract/holder_contract/http_routes.py says how they behave.
#pragma once
#include "api/generated/ContractJson.h"
#include <charconv>
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
inline json decode_body(const std::string& body) {
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
    static const std::regex route_1("^/projects/([^/]+)/cards$");
    if (method == "GET" && std::regex_match(route_path, match, route_1)) {
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
    static const std::regex route_2("^/projects/([^/]+)/cards$");
    if (method == "POST" && std::regex_match(route_path, match, route_2)) {
        std::string project_id{};
        holder::contract::NewCard card{};
        try {
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
    static const std::regex route_9("^/projects/([^/]+)/tags$");
    if (method == "GET" && std::regex_match(route_path, match, route_9)) {
        std::string project_id{};
        try {
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
    static const std::regex route_0("^/projects/([^/]+)/cards/([^/]+)$");
    if (method == "GET" && std::regex_match(route_path, match, route_0)) {
        std::string project_id{};
        std::string card_id{};
        try {
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
    static const std::regex route_3("^/projects/([^/]+)/cards/([^/]+)$");
    if (method == "PATCH" && std::regex_match(route_path, match, route_3)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::CardUpdate update{};
        try {
            project_id = contract_json::check_text(decode_uri_component(match[1].str(), false), "project_id", 1, std::numeric_limits<std::size_t>::max());
            card_id = contract_json::check_text(decode_uri_component(match[2].str(), false), "card_id", 1, std::numeric_limits<std::size_t>::max());
            update = contract_json::decode<holder::contract::CardUpdate>(decode_body(body));
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
    static const std::regex route_4("^/projects/([^/]+)/cards/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_4)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::TrashCardChildren children{};
        try {
            const auto query_values = decode_query(query_string);
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
    static const std::regex route_5("^/projects/([^/]+)/cards/([^/]+)/restore$");
    if (method == "POST" && std::regex_match(route_path, match, route_5)) {
        std::string project_id{};
        std::string card_id{};
        try {
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
    static const std::regex route_6("^/projects/([^/]+)/cards/([^/]+)/move$");
    if (method == "POST" && std::regex_match(route_path, match, route_6)) {
        std::string project_id{};
        std::string card_id{};
        holder::contract::CardMove move{};
        try {
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
    static const std::regex route_7("^/projects/([^/]+)/cards/([^/]+)/tags/([^/]+)$");
    if (method == "PUT" && std::regex_match(route_path, match, route_7)) {
        std::string project_id{};
        std::string card_id{};
        std::string tag{};
        try {
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
    static const std::regex route_8("^/projects/([^/]+)/cards/([^/]+)/tags/([^/]+)$");
    if (method == "DELETE" && std::regex_match(route_path, match, route_8)) {
        std::string project_id{};
        std::string card_id{};
        std::string tag{};
        try {
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
    return std::nullopt;
}

// The methods dispatch answers on this path, for a 405's Allow header.
inline std::set<std::string_view> allowed_methods(const std::string& path) {
    const auto route_path = path.substr(0, path.find('?'));
    std::set<std::string_view> methods;
    static const std::regex path_0("^/projects/([^/]+)/cards$");
    if (std::regex_match(route_path, path_0)) methods.insert({"GET", "POST"});
    static const std::regex path_1("^/projects/([^/]+)/tags$");
    if (std::regex_match(route_path, path_1)) methods.insert({"GET"});
    static const std::regex path_2("^/projects/([^/]+)/cards/([^/]+)$");
    if (std::regex_match(route_path, path_2)) methods.insert({"DELETE", "GET", "PATCH"});
    static const std::regex path_3("^/projects/([^/]+)/cards/([^/]+)/restore$");
    if (std::regex_match(route_path, path_3)) methods.insert({"POST"});
    static const std::regex path_4("^/projects/([^/]+)/cards/([^/]+)/move$");
    if (std::regex_match(route_path, path_4)) methods.insert({"POST"});
    static const std::regex path_5("^/projects/([^/]+)/cards/([^/]+)/tags/([^/]+)$");
    if (std::regex_match(route_path, path_5)) methods.insert({"DELETE", "PUT"});
    return methods;
}
}  // namespace contract_http
