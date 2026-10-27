// Generated from contract/interface/app.py, contract/interface/cards.py, contract/interface/errors.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// The JSON codec for the contract's types, for the daemon's generated routes.
#pragma once
#include "api/Contract.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace contract_json {
using json = nlohmann::json;
template <typename T> struct codec;

// The number of characters in text, or nothing when it is not well-formed UTF-8.
inline std::optional<std::size_t> utf8_length(const std::string& text) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < text.size(); ++count) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t size = 1;
        char32_t point = lead;
        if (lead >= 0xC2 && lead <= 0xDF) {
            size = 2, point = lead & 0x1F;
        } else if (lead >= 0xE0 && lead <= 0xEF) {
            size = 3, point = lead & 0x0F;
        } else if (lead >= 0xF0 && lead <= 0xF4) {
            size = 4, point = lead & 0x07;
        } else if (lead >= 0x80) {
            return std::nullopt;
        }
        if (text.size() - i < size) return std::nullopt;
        for (std::size_t k = 1; k < size; ++k) {
            const auto next = static_cast<unsigned char>(text[i + k]);
            if ((next & 0xC0) != 0x80) return std::nullopt;
            point = (point << 6) | (next & 0x3F);
        }
        if ((size == 3 && (point < 0x800 || (point >= 0xD800 && point <= 0xDFFF))) ||
            (size == 4 && (point < 0x10000 || point > 0x10FFFF)))
            return std::nullopt;
        i += size;
    }
    return count;
}

// Text the contract accepts: UTF-8 without U+0000, its length in characters within bounds.
inline const std::string& check_text(const std::string& value, const char* name, std::size_t minimum,
                                     std::size_t maximum) {
    const auto length = utf8_length(value);
    if (!length) throw std::invalid_argument(std::string(name) + " must be UTF-8");
    if (value.find('\0') != std::string::npos)
        throw std::invalid_argument(std::string(name) + " must not contain U+0000");
    if (*length < minimum)
        throw std::invalid_argument(std::string(name) + (minimum == 1 ? " must not be empty" :
            " must be at least " + std::to_string(minimum) + " characters"));
    if (*length > maximum)
        throw std::invalid_argument(std::string(name) + " must be at most " + std::to_string(maximum) +
                                    " characters");
    return value;
}

template <typename T>
T check_range(T value, const char* name, T minimum, T maximum) {
    if (value < minimum || value > maximum)
        throw std::invalid_argument(std::string(name) + " must be from " + std::to_string(minimum) + " to " +
                                    std::to_string(maximum));
    return value;
}

inline double check_finite(double value, const char* name) {
    if (!std::isfinite(value)) throw std::invalid_argument(std::string(name) + " must be a finite number");
    return value;
}

// A field's value, with the field's name on any complaint about it.
template <typename T> T decode_field(const json& value, const char* name) {
    try {
        return codec<T>::decode(value);
    } catch (const std::invalid_argument& error) {
        throw std::invalid_argument(std::string(name) + " " + error.what());
    }
}

// An integer of type T. JSON has one kind of number, so an integer may be written with a fraction or
// an exponent, such as 691.0 or 1e3, as JSON Schema allows; it must be whole and in range.
template <typename T> T decode_integer(const json& value) {
    constexpr auto minimum = std::numeric_limits<T>::min();
    constexpr auto maximum = std::numeric_limits<T>::max();
    if (value.is_number_unsigned()) {
        if (value.get<std::uint64_t>() > static_cast<std::uint64_t>(maximum))
            throw std::invalid_argument("is out of range");
        return static_cast<T>(value.get<std::uint64_t>());
    }
    if (value.is_number_integer()) {
        const auto number = value.get<std::int64_t>();
        if (number < minimum || number > maximum) throw std::invalid_argument("is out of range");
        return static_cast<T>(number);
    }
    if (!value.is_number_float() || std::trunc(value.get<double>()) != value.get<double>())
        throw std::invalid_argument("must be an integer");
    // -minimum, a power of two, is the first double past maximum.
    const auto number = value.get<double>();
    if (number < static_cast<double>(minimum) || number >= -static_cast<double>(minimum))
        throw std::invalid_argument("is out of range");
    return static_cast<T>(number);
}

template <> struct codec<std::int32_t> {
    static json encode(std::int32_t value) { return value; }
    static std::int32_t decode(const json& value) { return decode_integer<std::int32_t>(value); }
};
template <> struct codec<bool> {
    static json encode(bool value) { return value; }
    static bool decode(const json& value) {
        if (!value.is_boolean()) throw std::invalid_argument("must be a boolean");
        return value.get<bool>();
    }
};
template <> struct codec<long long> {
    static json encode(long long value) { return value; }
    static long long decode(const json& value) { return decode_integer<long long>(value); }
};
template <> struct codec<double> {
    static json encode(double value) { return value; }
    static double decode(const json& value) {
        if (!value.is_number()) throw std::invalid_argument("must be a number");
        const double number = value.get<double>();
        if (!std::isfinite(number)) throw std::invalid_argument("must be a finite number");
        return number;
    }
};
template <> struct codec<std::string> {
    static json encode(const std::string& value) { return value; }
    static std::string decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        return value.get<std::string>();
    }
};
template <typename T> struct codec<std::vector<T>> {
    static json encode(const std::vector<T>& value) {
        json result = json::array();
        for (const auto& item : value) result.push_back(codec<T>::encode(item));
        return result;
    }
    static std::vector<T> decode(const json& value) {
        if (!value.is_array()) throw std::invalid_argument("must be an array");
        std::vector<T> result;
        for (const auto& item : value) result.push_back(codec<T>::decode(item));
        return result;
    }
};

template <> struct codec<holder::contract::CardMoveIntent> {
    static json encode(holder::contract::CardMoveIntent value) {
        switch (value) {
        case holder::contract::CardMoveIntent::Into: return "into";
        case holder::contract::CardMoveIntent::Before: return "before";
        case holder::contract::CardMoveIntent::After: return "after";
        case holder::contract::CardMoveIntent::Left: return "left";
        case holder::contract::CardMoveIntent::Right: return "right";
        case holder::contract::CardMoveIntent::ToStart: return "to_start";
        case holder::contract::CardMoveIntent::ToEnd: return "to_end";
        case holder::contract::CardMoveIntent::UpLevel: return "up_level";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::CardMoveIntent decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "into") return holder::contract::CardMoveIntent::Into;
        if (text == "before") return holder::contract::CardMoveIntent::Before;
        if (text == "after") return holder::contract::CardMoveIntent::After;
        if (text == "left") return holder::contract::CardMoveIntent::Left;
        if (text == "right") return holder::contract::CardMoveIntent::Right;
        if (text == "to_start") return holder::contract::CardMoveIntent::ToStart;
        if (text == "to_end") return holder::contract::CardMoveIntent::ToEnd;
        if (text == "up_level") return holder::contract::CardMoveIntent::UpLevel;
        throw std::invalid_argument("must be one of into, before, after, left, right, to_start, to_end, up_level");
    }
};

template <> struct codec<holder::contract::TagOccurrence> {
    static json encode(const holder::contract::TagOccurrence& value);
    static holder::contract::TagOccurrence decode(const json& value);
};
template <> struct codec<holder::contract::Card> {
    static json encode(const holder::contract::Card& value);
    static holder::contract::Card decode(const json& value);
};
template <> struct codec<holder::contract::CardListItem> {
    static json encode(const holder::contract::CardListItem& value);
    static holder::contract::CardListItem decode(const json& value);
};
template <> struct codec<holder::contract::Page<holder::contract::CardListItem>> {
    static json encode(const holder::contract::Page<holder::contract::CardListItem>& value);
    static holder::contract::Page<holder::contract::CardListItem> decode(const json& value);
};
template <> struct codec<holder::contract::TagCount> {
    static json encode(const holder::contract::TagCount& value);
    static holder::contract::TagCount decode(const json& value);
};
template <> struct codec<holder::contract::NewCard> {
    static json encode(const holder::contract::NewCard& value);
    static holder::contract::NewCard decode(const json& value);
};
template <> struct codec<holder::contract::CardUpdate> {
    static json encode(const holder::contract::CardUpdate& value);
    static holder::contract::CardUpdate decode(const json& value);
};
template <> struct codec<holder::contract::CardMove> {
    static json encode(const holder::contract::CardMove& value);
    static holder::contract::CardMove decode(const json& value);
};

inline json codec<holder::contract::TagOccurrence>::encode(const holder::contract::TagOccurrence& value) {
    json result = json::object();
    result["tag"] = codec<std::string>::encode(value.tag);
    result["byte_start"] = codec<long long>::encode(value.byte_start);
    result["byte_end"] = codec<long long>::encode(value.byte_end);
    return result;
}
inline holder::contract::TagOccurrence codec<holder::contract::TagOccurrence>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a TagOccurrence object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "tag" && it.key() != "byte_start" && it.key() != "byte_end") throw std::invalid_argument("TagOccurrence has no field " + it.key());
    holder::contract::TagOccurrence out;
    if (value.contains("tag") && !value.at("tag").is_null()) {
        const auto& item = value.at("tag");
        out.tag = check_text(decode_field<std::string>(item, "tag"), "tag", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("tag is required and must not be null");
    }
    if (value.contains("byte_start") && !value.at("byte_start").is_null()) {
        const auto& item = value.at("byte_start");
        out.byte_start = decode_field<long long>(item, "byte_start");
    } else {
        throw std::invalid_argument("byte_start is required and must not be null");
    }
    if (value.contains("byte_end") && !value.at("byte_end").is_null()) {
        const auto& item = value.at("byte_end");
        out.byte_end = decode_field<long long>(item, "byte_end");
    } else {
        throw std::invalid_argument("byte_end is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::Card>::encode(const holder::contract::Card& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["rel_path"] = codec<std::string>::encode(value.rel_path);
    result["parent_card_id"] = value.parent_card_id ? codec<std::string>::encode(*value.parent_card_id) : json(nullptr);
    result["sort_key"] = codec<double>::encode(value.sort_key);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    result["deleted_at"] = value.deleted_at ? codec<long long>::encode(*value.deleted_at) : json(nullptr);
    result["content"] = codec<std::string>::encode(value.content);
    result["tags"] = codec<std::vector<std::string>>::encode(value.tags);
    result["tag_occurrences"] = codec<std::vector<holder::contract::TagOccurrence>>::encode(value.tag_occurrences);
    return result;
}
inline holder::contract::Card codec<holder::contract::Card>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a Card object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "card_id" && it.key() != "project_id" && it.key() != "title" && it.key() != "rel_path" && it.key() != "parent_card_id" && it.key() != "sort_key" && it.key() != "created_at" && it.key() != "updated_at" && it.key() != "deleted_at" && it.key() != "content" && it.key() != "tags" && it.key() != "tag_occurrences") throw std::invalid_argument("Card has no field " + it.key());
    holder::contract::Card out;
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_id is required and must not be null");
    }
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("title is required and must not be null");
    }
    if (value.contains("rel_path") && !value.at("rel_path").is_null()) {
        const auto& item = value.at("rel_path");
        out.rel_path = check_text(decode_field<std::string>(item, "rel_path"), "rel_path", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("rel_path is required and must not be null");
    }
    if (value.contains("parent_card_id") && !value.at("parent_card_id").is_null()) {
        const auto& item = value.at("parent_card_id");
        out.parent_card_id = check_text(decode_field<std::string>(item, "parent_card_id"), "parent_card_id", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("sort_key") && !value.at("sort_key").is_null()) {
        const auto& item = value.at("sort_key");
        out.sort_key = decode_field<double>(item, "sort_key");
    } else {
        throw std::invalid_argument("sort_key is required and must not be null");
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = decode_field<long long>(item, "created_at");
    } else {
        throw std::invalid_argument("created_at is required and must not be null");
    }
    if (value.contains("updated_at") && !value.at("updated_at").is_null()) {
        const auto& item = value.at("updated_at");
        out.updated_at = decode_field<long long>(item, "updated_at");
    } else {
        throw std::invalid_argument("updated_at is required and must not be null");
    }
    if (value.contains("deleted_at") && !value.at("deleted_at").is_null()) {
        const auto& item = value.at("deleted_at");
        out.deleted_at = decode_field<long long>(item, "deleted_at");
    }
    if (value.contains("content") && !value.at("content").is_null()) {
        const auto& item = value.at("content");
        out.content = check_text(decode_field<std::string>(item, "content"), "content", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("content is required and must not be null");
    }
    if (value.contains("tags") && !value.at("tags").is_null()) {
        const auto& item = value.at("tags");
        out.tags = [&] { auto items = decode_field<std::vector<std::string>>(item, "tags"); for (const auto& text : items) check_text(text, "tags", 0, std::numeric_limits<std::size_t>::max()); return items; }();
    } else {
        throw std::invalid_argument("tags is required and must not be null");
    }
    if (value.contains("tag_occurrences") && !value.at("tag_occurrences").is_null()) {
        const auto& item = value.at("tag_occurrences");
        out.tag_occurrences = decode_field<std::vector<holder::contract::TagOccurrence>>(item, "tag_occurrences");
    } else {
        throw std::invalid_argument("tag_occurrences is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardListItem>::encode(const holder::contract::CardListItem& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["rel_path"] = codec<std::string>::encode(value.rel_path);
    result["parent_card_id"] = value.parent_card_id ? codec<std::string>::encode(*value.parent_card_id) : json(nullptr);
    result["sort_key"] = codec<double>::encode(value.sort_key);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    result["deleted_at"] = value.deleted_at ? codec<long long>::encode(*value.deleted_at) : json(nullptr);
    if (value.child_count) result["child_count"] = codec<std::int32_t>::encode(*value.child_count);
    if (value.content) result["content"] = codec<std::string>::encode(*value.content);
    return result;
}
inline holder::contract::CardListItem codec<holder::contract::CardListItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardListItem object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "card_id" && it.key() != "project_id" && it.key() != "title" && it.key() != "rel_path" && it.key() != "parent_card_id" && it.key() != "sort_key" && it.key() != "created_at" && it.key() != "updated_at" && it.key() != "deleted_at" && it.key() != "child_count" && it.key() != "content") throw std::invalid_argument("CardListItem has no field " + it.key());
    holder::contract::CardListItem out;
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_id is required and must not be null");
    }
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("title is required and must not be null");
    }
    if (value.contains("rel_path") && !value.at("rel_path").is_null()) {
        const auto& item = value.at("rel_path");
        out.rel_path = check_text(decode_field<std::string>(item, "rel_path"), "rel_path", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("rel_path is required and must not be null");
    }
    if (value.contains("parent_card_id") && !value.at("parent_card_id").is_null()) {
        const auto& item = value.at("parent_card_id");
        out.parent_card_id = check_text(decode_field<std::string>(item, "parent_card_id"), "parent_card_id", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("sort_key") && !value.at("sort_key").is_null()) {
        const auto& item = value.at("sort_key");
        out.sort_key = decode_field<double>(item, "sort_key");
    } else {
        throw std::invalid_argument("sort_key is required and must not be null");
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = decode_field<long long>(item, "created_at");
    } else {
        throw std::invalid_argument("created_at is required and must not be null");
    }
    if (value.contains("updated_at") && !value.at("updated_at").is_null()) {
        const auto& item = value.at("updated_at");
        out.updated_at = decode_field<long long>(item, "updated_at");
    } else {
        throw std::invalid_argument("updated_at is required and must not be null");
    }
    if (value.contains("deleted_at") && !value.at("deleted_at").is_null()) {
        const auto& item = value.at("deleted_at");
        out.deleted_at = decode_field<long long>(item, "deleted_at");
    }
    if (value.contains("child_count") && !value.at("child_count").is_null()) {
        const auto& item = value.at("child_count");
        out.child_count = decode_field<std::int32_t>(item, "child_count");
    } else if (value.contains("child_count")) {
        throw std::invalid_argument("child_count must not be null");
    }
    if (value.contains("content") && !value.at("content").is_null()) {
        const auto& item = value.at("content");
        out.content = check_text(decode_field<std::string>(item, "content"), "content", 0, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("content")) {
        throw std::invalid_argument("content must not be null");
    }
    return out;
}

inline json codec<holder::contract::Page<holder::contract::CardListItem>>::encode(const holder::contract::Page<holder::contract::CardListItem>& value) {
    json result = json::object();
    result["items"] = codec<std::vector<holder::contract::CardListItem>>::encode(value.items);
    result["next_cursor"] = value.next_cursor ? codec<std::string>::encode(*value.next_cursor) : json(nullptr);
    return result;
}
inline holder::contract::Page<holder::contract::CardListItem> codec<holder::contract::Page<holder::contract::CardListItem>>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a Page object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "items" && it.key() != "next_cursor") throw std::invalid_argument("Page has no field " + it.key());
    holder::contract::Page<holder::contract::CardListItem> out;
    if (value.contains("items") && !value.at("items").is_null()) {
        const auto& item = value.at("items");
        out.items = decode_field<std::vector<holder::contract::CardListItem>>(item, "items");
    } else {
        throw std::invalid_argument("items is required and must not be null");
    }
    if (value.contains("next_cursor") && !value.at("next_cursor").is_null()) {
        const auto& item = value.at("next_cursor");
        out.next_cursor = check_text(decode_field<std::string>(item, "next_cursor"), "next_cursor", 0, std::numeric_limits<std::size_t>::max());
    }
    return out;
}

inline json codec<holder::contract::TagCount>::encode(const holder::contract::TagCount& value) {
    json result = json::object();
    result["tag"] = codec<std::string>::encode(value.tag);
    result["card_count"] = codec<std::int32_t>::encode(value.card_count);
    return result;
}
inline holder::contract::TagCount codec<holder::contract::TagCount>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a TagCount object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "tag" && it.key() != "card_count") throw std::invalid_argument("TagCount has no field " + it.key());
    holder::contract::TagCount out;
    if (value.contains("tag") && !value.at("tag").is_null()) {
        const auto& item = value.at("tag");
        out.tag = check_text(decode_field<std::string>(item, "tag"), "tag", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("tag is required and must not be null");
    }
    if (value.contains("card_count") && !value.at("card_count").is_null()) {
        const auto& item = value.at("card_count");
        out.card_count = check_range<std::int32_t>(decode_field<std::int32_t>(item, "card_count"), "card_count", 1, std::numeric_limits<std::int32_t>::max());
    } else {
        throw std::invalid_argument("card_count is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::NewCard>::encode(const holder::contract::NewCard& value) {
    json result = json::object();
    result["title"] = codec<std::string>::encode(value.title);
    result["content"] = codec<std::string>::encode(value.content);
    result["parent_card_id"] = value.parent_card_id ? codec<std::string>::encode(*value.parent_card_id) : json(nullptr);
    if (value.sort_key) result["sort_key"] = codec<double>::encode(*value.sort_key);
    if (value.created_at) result["created_at"] = codec<long long>::encode(*value.created_at);
    if (value.updated_at) result["updated_at"] = codec<long long>::encode(*value.updated_at);
    return result;
}
inline holder::contract::NewCard codec<holder::contract::NewCard>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a NewCard object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "title" && it.key() != "content" && it.key() != "parent_card_id" && it.key() != "sort_key" && it.key() != "created_at" && it.key() != "updated_at") throw std::invalid_argument("NewCard has no field " + it.key());
    holder::contract::NewCard out;
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("title is required and must not be null");
    }
    if (value.contains("content") && !value.at("content").is_null()) {
        const auto& item = value.at("content");
        out.content = check_text(decode_field<std::string>(item, "content"), "content", 0, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("content")) {
        throw std::invalid_argument("content must not be null");
    }
    if (value.contains("parent_card_id") && !value.at("parent_card_id").is_null()) {
        const auto& item = value.at("parent_card_id");
        out.parent_card_id = check_text(decode_field<std::string>(item, "parent_card_id"), "parent_card_id", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("sort_key") && !value.at("sort_key").is_null()) {
        const auto& item = value.at("sort_key");
        out.sort_key = decode_field<double>(item, "sort_key");
    } else if (value.contains("sort_key")) {
        throw std::invalid_argument("sort_key must not be null");
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = decode_field<long long>(item, "created_at");
    } else if (value.contains("created_at")) {
        throw std::invalid_argument("created_at must not be null");
    }
    if (value.contains("updated_at") && !value.at("updated_at").is_null()) {
        const auto& item = value.at("updated_at");
        out.updated_at = decode_field<long long>(item, "updated_at");
    } else if (value.contains("updated_at")) {
        throw std::invalid_argument("updated_at must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardUpdate>::encode(const holder::contract::CardUpdate& value) {
    json result = json::object();
    if (value.title) result["title"] = codec<std::string>::encode(*value.title);
    if (value.content) result["content"] = codec<std::string>::encode(*value.content);
    if (value.parent_card_id) result["parent_card_id"] = *value.parent_card_id ? codec<std::string>::encode(**value.parent_card_id) : json(nullptr);
    if (value.sort_key) result["sort_key"] = codec<double>::encode(*value.sort_key);
    if (value.updated_at) result["updated_at"] = codec<long long>::encode(*value.updated_at);
    return result;
}
inline holder::contract::CardUpdate codec<holder::contract::CardUpdate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardUpdate object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "title" && it.key() != "content" && it.key() != "parent_card_id" && it.key() != "sort_key" && it.key() != "updated_at") throw std::invalid_argument("CardUpdate has no field " + it.key());
    holder::contract::CardUpdate out;
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("title")) {
        throw std::invalid_argument("title must not be null");
    }
    if (value.contains("content") && !value.at("content").is_null()) {
        const auto& item = value.at("content");
        out.content = check_text(decode_field<std::string>(item, "content"), "content", 0, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("content")) {
        throw std::invalid_argument("content must not be null");
    }
    if (value.contains("parent_card_id") && !value.at("parent_card_id").is_null()) {
        const auto& item = value.at("parent_card_id");
        out.parent_card_id = check_text(decode_field<std::string>(item, "parent_card_id"), "parent_card_id", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("parent_card_id")) {
        out.parent_card_id.emplace();
    }
    if (value.contains("sort_key") && !value.at("sort_key").is_null()) {
        const auto& item = value.at("sort_key");
        out.sort_key = decode_field<double>(item, "sort_key");
    } else if (value.contains("sort_key")) {
        throw std::invalid_argument("sort_key must not be null");
    }
    if (value.contains("updated_at") && !value.at("updated_at").is_null()) {
        const auto& item = value.at("updated_at");
        out.updated_at = decode_field<long long>(item, "updated_at");
    } else if (value.contains("updated_at")) {
        throw std::invalid_argument("updated_at must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardMove>::encode(const holder::contract::CardMove& value) {
    json result = json::object();
    result["intent"] = codec<holder::contract::CardMoveIntent>::encode(value.intent);
    if (value.target_card_id) result["target_card_id"] = codec<std::string>::encode(*value.target_card_id);
    if (value.parent_card_id) result["parent_card_id"] = *value.parent_card_id ? codec<std::string>::encode(**value.parent_card_id) : json(nullptr);
    return result;
}
inline holder::contract::CardMove codec<holder::contract::CardMove>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardMove object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "intent" && it.key() != "target_card_id" && it.key() != "parent_card_id") throw std::invalid_argument("CardMove has no field " + it.key());
    holder::contract::CardMove out;
    if (value.contains("intent") && !value.at("intent").is_null()) {
        const auto& item = value.at("intent");
        out.intent = decode_field<holder::contract::CardMoveIntent>(item, "intent");
    } else {
        throw std::invalid_argument("intent is required and must not be null");
    }
    if (value.contains("target_card_id") && !value.at("target_card_id").is_null()) {
        const auto& item = value.at("target_card_id");
        out.target_card_id = check_text(decode_field<std::string>(item, "target_card_id"), "target_card_id", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("target_card_id")) {
        throw std::invalid_argument("target_card_id must not be null");
    }
    if (value.contains("parent_card_id") && !value.at("parent_card_id").is_null()) {
        const auto& item = value.at("parent_card_id");
        out.parent_card_id = check_text(decode_field<std::string>(item, "parent_card_id"), "parent_card_id", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("parent_card_id")) {
        out.parent_card_id.emplace();
    }
    return out;
}

template <typename T> json encode(const T& value) { return codec<T>::encode(value); }
template <typename T> T decode(const json& value) { return codec<T>::decode(value); }
}  // namespace contract_json
