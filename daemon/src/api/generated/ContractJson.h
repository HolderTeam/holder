// Generated from contract/interface/app.py, contract/interface/cards.py, contract/interface/errors.py, contract/interface/links.py, contract/interface/milestones.py, contract/interface/projects.py, contract/interface/resources.py by holder_contract.
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
#include <map>
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
// A map is a JSON object; JSON names its keys, so each is text and appears once.
template <typename T> struct codec<std::map<std::string, T>> {
    static json encode(const std::map<std::string, T>& value) {
        json result = json::object();
        for (const auto& [key, item] : value) result[key] = codec<T>::encode(item);
        return result;
    }
    static std::map<std::string, T> decode(const json& value) {
        if (!value.is_object()) throw std::invalid_argument("must be an object");
        std::map<std::string, T> result;
        for (auto it = value.begin(); it != value.end(); ++it) {
            try {
                result.emplace(it.key(), codec<T>::decode(it.value()));
            } catch (const std::invalid_argument& error) {
                throw std::invalid_argument("has " + it.key() + ", which " + error.what());
            }
        }
        return result;
    }
};

template <> struct codec<holder::contract::LinkSourceType> {
    static json encode(holder::contract::LinkSourceType value) {
        switch (value) {
        case holder::contract::LinkSourceType::Card: return "card";
        case holder::contract::LinkSourceType::AiMessage: return "ai_message";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::LinkSourceType decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "card") return holder::contract::LinkSourceType::Card;
        if (text == "ai_message") return holder::contract::LinkSourceType::AiMessage;
        throw std::invalid_argument("must be one of card, ai_message");
    }
};

template <> struct codec<holder::contract::LinkTargetType> {
    static json encode(holder::contract::LinkTargetType value) {
        switch (value) {
        case holder::contract::LinkTargetType::Card: return "card";
        case holder::contract::LinkTargetType::AiMessage: return "ai_message";
        case holder::contract::LinkTargetType::AiThread: return "ai_thread";
        case holder::contract::LinkTargetType::Resource: return "resource";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::LinkTargetType decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "card") return holder::contract::LinkTargetType::Card;
        if (text == "ai_message") return holder::contract::LinkTargetType::AiMessage;
        if (text == "ai_thread") return holder::contract::LinkTargetType::AiThread;
        if (text == "resource") return holder::contract::LinkTargetType::Resource;
        throw std::invalid_argument("must be one of card, ai_message, ai_thread, resource");
    }
};

template <> struct codec<holder::contract::AssetPlacementEncoding> {
    static json encode(holder::contract::AssetPlacementEncoding value) {
        switch (value) {
        case holder::contract::AssetPlacementEncoding::Plain: return "plain";
        case holder::contract::AssetPlacementEncoding::HolderAssetV1: return "holder_asset_v1";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::AssetPlacementEncoding decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "plain") return holder::contract::AssetPlacementEncoding::Plain;
        if (text == "holder_asset_v1") return holder::contract::AssetPlacementEncoding::HolderAssetV1;
        throw std::invalid_argument("must be one of plain, holder_asset_v1");
    }
};

template <> struct codec<holder::contract::ProjectPrivacyMode> {
    static json encode(holder::contract::ProjectPrivacyMode value) {
        switch (value) {
        case holder::contract::ProjectPrivacyMode::EncryptedGit: return "encrypted_git";
        case holder::contract::ProjectPrivacyMode::Plain: return "plain";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::ProjectPrivacyMode decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "encrypted_git") return holder::contract::ProjectPrivacyMode::EncryptedGit;
        if (text == "plain") return holder::contract::ProjectPrivacyMode::Plain;
        throw std::invalid_argument("must be one of encrypted_git, plain");
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

template <> struct codec<holder::contract::NewLinkTargetType> {
    static json encode(holder::contract::NewLinkTargetType value) {
        switch (value) {
        case holder::contract::NewLinkTargetType::Card: return "card";
        case holder::contract::NewLinkTargetType::AiMessage: return "ai_message";
        case holder::contract::NewLinkTargetType::AiThread: return "ai_thread";
        case holder::contract::NewLinkTargetType::Resource: return "resource";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::NewLinkTargetType decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "card") return holder::contract::NewLinkTargetType::Card;
        if (text == "ai_message") return holder::contract::NewLinkTargetType::AiMessage;
        if (text == "ai_thread") return holder::contract::NewLinkTargetType::AiThread;
        if (text == "resource") return holder::contract::NewLinkTargetType::Resource;
        throw std::invalid_argument("must be one of card, ai_message, ai_thread, resource");
    }
};

template <> struct codec<holder::contract::NewProjectPrivacyMode> {
    static json encode(holder::contract::NewProjectPrivacyMode value) {
        switch (value) {
        case holder::contract::NewProjectPrivacyMode::EncryptedGit: return "encrypted_git";
        case holder::contract::NewProjectPrivacyMode::Plain: return "plain";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::NewProjectPrivacyMode decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "encrypted_git") return holder::contract::NewProjectPrivacyMode::EncryptedGit;
        if (text == "plain") return holder::contract::NewProjectPrivacyMode::Plain;
        throw std::invalid_argument("must be one of encrypted_git, plain");
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
template <> struct codec<holder::contract::Link> {
    static json encode(const holder::contract::Link& value);
    static holder::contract::Link decode(const json& value);
};
template <> struct codec<holder::contract::MilestoneDate> {
    static json encode(const holder::contract::MilestoneDate& value);
    static holder::contract::MilestoneDate decode(const json& value);
};
template <> struct codec<holder::contract::Milestone> {
    static json encode(const holder::contract::Milestone& value);
    static holder::contract::Milestone decode(const json& value);
};
template <> struct codec<holder::contract::CalendarMilestone> {
    static json encode(const holder::contract::CalendarMilestone& value);
    static holder::contract::CalendarMilestone decode(const json& value);
};
template <> struct codec<holder::contract::CardMetadata> {
    static json encode(const holder::contract::CardMetadata& value);
    static holder::contract::CardMetadata decode(const json& value);
};
template <> struct codec<holder::contract::Calendar> {
    static json encode(const holder::contract::Calendar& value);
    static holder::contract::Calendar decode(const json& value);
};
template <> struct codec<holder::contract::AssetPlacement> {
    static json encode(const holder::contract::AssetPlacement& value);
    static holder::contract::AssetPlacement decode(const json& value);
};
template <> struct codec<holder::contract::ResourceAsset> {
    static json encode(const holder::contract::ResourceAsset& value);
    static holder::contract::ResourceAsset decode(const json& value);
};
template <> struct codec<holder::contract::Resource> {
    static json encode(const holder::contract::Resource& value);
    static holder::contract::Resource decode(const json& value);
};
template <> struct codec<holder::contract::ProjectListItem> {
    static json encode(const holder::contract::ProjectListItem& value);
    static holder::contract::ProjectListItem decode(const json& value);
};
template <> struct codec<holder::contract::Project> {
    static json encode(const holder::contract::Project& value);
    static holder::contract::Project decode(const json& value);
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
template <> struct codec<holder::contract::NewLink> {
    static json encode(const holder::contract::NewLink& value);
    static holder::contract::NewLink decode(const json& value);
};
template <> struct codec<holder::contract::NewMilestone> {
    static json encode(const holder::contract::NewMilestone& value);
    static holder::contract::NewMilestone decode(const json& value);
};
template <> struct codec<holder::contract::MilestoneUpdate> {
    static json encode(const holder::contract::MilestoneUpdate& value);
    static holder::contract::MilestoneUpdate decode(const json& value);
};
template <> struct codec<holder::contract::NewResource> {
    static json encode(const holder::contract::NewResource& value);
    static holder::contract::NewResource decode(const json& value);
};
template <> struct codec<holder::contract::ResourceUpdate> {
    static json encode(const holder::contract::ResourceUpdate& value);
    static holder::contract::ResourceUpdate decode(const json& value);
};
template <> struct codec<holder::contract::NewProject> {
    static json encode(const holder::contract::NewProject& value);
    static holder::contract::NewProject decode(const json& value);
};
template <> struct codec<holder::contract::ProjectUpdate> {
    static json encode(const holder::contract::ProjectUpdate& value);
    static holder::contract::ProjectUpdate decode(const json& value);
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

inline json codec<holder::contract::Link>::encode(const holder::contract::Link& value) {
    json result = json::object();
    result["source_project_id"] = codec<std::string>::encode(value.source_project_id);
    result["source_type"] = codec<holder::contract::LinkSourceType>::encode(value.source_type);
    result["source_id"] = codec<std::string>::encode(value.source_id);
    result["target_project_id"] = codec<std::string>::encode(value.target_project_id);
    result["target_type"] = codec<holder::contract::LinkTargetType>::encode(value.target_type);
    result["target_id"] = codec<std::string>::encode(value.target_id);
    result["kind"] = codec<std::string>::encode(value.kind);
    result["label"] = value.label ? codec<std::string>::encode(*value.label) : json(nullptr);
    result["created_at"] = codec<long long>::encode(value.created_at);
    return result;
}
inline holder::contract::Link codec<holder::contract::Link>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a Link object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "source_project_id" && it.key() != "source_type" && it.key() != "source_id" && it.key() != "target_project_id" && it.key() != "target_type" && it.key() != "target_id" && it.key() != "kind" && it.key() != "label" && it.key() != "created_at") throw std::invalid_argument("Link has no field " + it.key());
    holder::contract::Link out;
    if (value.contains("source_project_id") && !value.at("source_project_id").is_null()) {
        const auto& item = value.at("source_project_id");
        out.source_project_id = check_text(decode_field<std::string>(item, "source_project_id"), "source_project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("source_project_id is required and must not be null");
    }
    if (value.contains("source_type") && !value.at("source_type").is_null()) {
        const auto& item = value.at("source_type");
        out.source_type = decode_field<holder::contract::LinkSourceType>(item, "source_type");
    } else {
        throw std::invalid_argument("source_type is required and must not be null");
    }
    if (value.contains("source_id") && !value.at("source_id").is_null()) {
        const auto& item = value.at("source_id");
        out.source_id = check_text(decode_field<std::string>(item, "source_id"), "source_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("source_id is required and must not be null");
    }
    if (value.contains("target_project_id") && !value.at("target_project_id").is_null()) {
        const auto& item = value.at("target_project_id");
        out.target_project_id = check_text(decode_field<std::string>(item, "target_project_id"), "target_project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("target_project_id is required and must not be null");
    }
    if (value.contains("target_type") && !value.at("target_type").is_null()) {
        const auto& item = value.at("target_type");
        out.target_type = decode_field<holder::contract::LinkTargetType>(item, "target_type");
    } else {
        throw std::invalid_argument("target_type is required and must not be null");
    }
    if (value.contains("target_id") && !value.at("target_id").is_null()) {
        const auto& item = value.at("target_id");
        out.target_id = check_text(decode_field<std::string>(item, "target_id"), "target_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("target_id is required and must not be null");
    }
    if (value.contains("kind") && !value.at("kind").is_null()) {
        const auto& item = value.at("kind");
        out.kind = check_text(decode_field<std::string>(item, "kind"), "kind", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("kind is required and must not be null");
    }
    if (value.contains("label") && !value.at("label").is_null()) {
        const auto& item = value.at("label");
        out.label = check_text(decode_field<std::string>(item, "label"), "label", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = decode_field<long long>(item, "created_at");
    } else {
        throw std::invalid_argument("created_at is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::MilestoneDate>::encode(const holder::contract::MilestoneDate& value) {
    json result = json::object();
    result["role"] = codec<std::string>::encode(value.role);
    if (value.date) result["date"] = codec<std::string>::encode(*value.date);
    if (value.datetime) result["datetime"] = codec<std::string>::encode(*value.datetime);
    return result;
}
inline holder::contract::MilestoneDate codec<holder::contract::MilestoneDate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a MilestoneDate object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "role" && it.key() != "date" && it.key() != "datetime") throw std::invalid_argument("MilestoneDate has no field " + it.key());
    holder::contract::MilestoneDate out;
    if (value.contains("role") && !value.at("role").is_null()) {
        const auto& item = value.at("role");
        out.role = check_text(decode_field<std::string>(item, "role"), "role", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("role is required and must not be null");
    }
    if (value.contains("date") && !value.at("date").is_null()) {
        const auto& item = value.at("date");
        out.date = check_text(decode_field<std::string>(item, "date"), "date", 10, 10);
    } else if (value.contains("date")) {
        throw std::invalid_argument("date must not be null");
    }
    if (value.contains("datetime") && !value.at("datetime").is_null()) {
        const auto& item = value.at("datetime");
        out.datetime = check_text(decode_field<std::string>(item, "datetime"), "datetime", 20, 25);
    } else if (value.contains("datetime")) {
        throw std::invalid_argument("datetime must not be null");
    }
    return out;
}

inline json codec<holder::contract::Milestone>::encode(const holder::contract::Milestone& value) {
    json result = json::object();
    result["milestone_id"] = codec<std::string>::encode(value.milestone_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["dates"] = codec<std::vector<holder::contract::MilestoneDate>>::encode(value.dates);
    result["kind"] = value.kind ? codec<std::string>::encode(*value.kind) : json(nullptr);
    result["description"] = value.description ? codec<std::string>::encode(*value.description) : json(nullptr);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::contract::Milestone codec<holder::contract::Milestone>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a Milestone object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "milestone_id" && it.key() != "project_id" && it.key() != "card_id" && it.key() != "dates" && it.key() != "kind" && it.key() != "description" && it.key() != "created_at" && it.key() != "updated_at") throw std::invalid_argument("Milestone has no field " + it.key());
    holder::contract::Milestone out;
    if (value.contains("milestone_id") && !value.at("milestone_id").is_null()) {
        const auto& item = value.at("milestone_id");
        out.milestone_id = check_text(decode_field<std::string>(item, "milestone_id"), "milestone_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("milestone_id is required and must not be null");
    }
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_id is required and must not be null");
    }
    if (value.contains("dates") && !value.at("dates").is_null()) {
        const auto& item = value.at("dates");
        out.dates = decode_field<std::vector<holder::contract::MilestoneDate>>(item, "dates");
    } else {
        throw std::invalid_argument("dates is required and must not be null");
    }
    if (value.contains("kind") && !value.at("kind").is_null()) {
        const auto& item = value.at("kind");
        out.kind = check_text(decode_field<std::string>(item, "kind"), "kind", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("description") && !value.at("description").is_null()) {
        const auto& item = value.at("description");
        out.description = check_text(decode_field<std::string>(item, "description"), "description", 1, std::numeric_limits<std::size_t>::max());
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
    return out;
}

inline json codec<holder::contract::CalendarMilestone>::encode(const holder::contract::CalendarMilestone& value) {
    json result = json::object();
    result["milestone_id"] = codec<std::string>::encode(value.milestone_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["dates"] = codec<std::vector<holder::contract::MilestoneDate>>::encode(value.dates);
    result["kind"] = value.kind ? codec<std::string>::encode(*value.kind) : json(nullptr);
    result["description"] = value.description ? codec<std::string>::encode(*value.description) : json(nullptr);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    result["card_title"] = codec<std::string>::encode(value.card_title);
    return result;
}
inline holder::contract::CalendarMilestone codec<holder::contract::CalendarMilestone>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CalendarMilestone object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "milestone_id" && it.key() != "project_id" && it.key() != "card_id" && it.key() != "dates" && it.key() != "kind" && it.key() != "description" && it.key() != "created_at" && it.key() != "updated_at" && it.key() != "card_title") throw std::invalid_argument("CalendarMilestone has no field " + it.key());
    holder::contract::CalendarMilestone out;
    if (value.contains("milestone_id") && !value.at("milestone_id").is_null()) {
        const auto& item = value.at("milestone_id");
        out.milestone_id = check_text(decode_field<std::string>(item, "milestone_id"), "milestone_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("milestone_id is required and must not be null");
    }
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_id is required and must not be null");
    }
    if (value.contains("dates") && !value.at("dates").is_null()) {
        const auto& item = value.at("dates");
        out.dates = decode_field<std::vector<holder::contract::MilestoneDate>>(item, "dates");
    } else {
        throw std::invalid_argument("dates is required and must not be null");
    }
    if (value.contains("kind") && !value.at("kind").is_null()) {
        const auto& item = value.at("kind");
        out.kind = check_text(decode_field<std::string>(item, "kind"), "kind", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("description") && !value.at("description").is_null()) {
        const auto& item = value.at("description");
        out.description = check_text(decode_field<std::string>(item, "description"), "description", 1, std::numeric_limits<std::size_t>::max());
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
    if (value.contains("card_title") && !value.at("card_title").is_null()) {
        const auto& item = value.at("card_title");
        out.card_title = check_text(decode_field<std::string>(item, "card_title"), "card_title", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_title is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardMetadata>::encode(const holder::contract::CardMetadata& value) {
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
    return result;
}
inline holder::contract::CardMetadata codec<holder::contract::CardMetadata>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardMetadata object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "card_id" && it.key() != "project_id" && it.key() != "title" && it.key() != "rel_path" && it.key() != "parent_card_id" && it.key() != "sort_key" && it.key() != "created_at" && it.key() != "updated_at" && it.key() != "deleted_at") throw std::invalid_argument("CardMetadata has no field " + it.key());
    holder::contract::CardMetadata out;
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
    return out;
}

inline json codec<holder::contract::Calendar>::encode(const holder::contract::Calendar& value) {
    json result = json::object();
    result["milestones"] = codec<std::vector<holder::contract::CalendarMilestone>>::encode(value.milestones);
    result["created_cards"] = codec<std::vector<holder::contract::CardMetadata>>::encode(value.created_cards);
    result["updated_cards"] = codec<std::vector<holder::contract::CardMetadata>>::encode(value.updated_cards);
    return result;
}
inline holder::contract::Calendar codec<holder::contract::Calendar>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a Calendar object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "milestones" && it.key() != "created_cards" && it.key() != "updated_cards") throw std::invalid_argument("Calendar has no field " + it.key());
    holder::contract::Calendar out;
    if (value.contains("milestones") && !value.at("milestones").is_null()) {
        const auto& item = value.at("milestones");
        out.milestones = decode_field<std::vector<holder::contract::CalendarMilestone>>(item, "milestones");
    } else {
        throw std::invalid_argument("milestones is required and must not be null");
    }
    if (value.contains("created_cards") && !value.at("created_cards").is_null()) {
        const auto& item = value.at("created_cards");
        out.created_cards = decode_field<std::vector<holder::contract::CardMetadata>>(item, "created_cards");
    } else {
        throw std::invalid_argument("created_cards is required and must not be null");
    }
    if (value.contains("updated_cards") && !value.at("updated_cards").is_null()) {
        const auto& item = value.at("updated_cards");
        out.updated_cards = decode_field<std::vector<holder::contract::CardMetadata>>(item, "updated_cards");
    } else {
        throw std::invalid_argument("updated_cards is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::AssetPlacement>::encode(const holder::contract::AssetPlacement& value) {
    json result = json::object();
    result["placement_id"] = codec<std::string>::encode(value.placement_id);
    result["location_id"] = codec<std::string>::encode(value.location_id);
    result["encoding"] = codec<holder::contract::AssetPlacementEncoding>::encode(value.encoding);
    result["stored_byte_size"] = codec<long long>::encode(value.stored_byte_size);
    result["created_at"] = codec<long long>::encode(value.created_at);
    return result;
}
inline holder::contract::AssetPlacement codec<holder::contract::AssetPlacement>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a AssetPlacement object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "placement_id" && it.key() != "location_id" && it.key() != "encoding" && it.key() != "stored_byte_size" && it.key() != "created_at") throw std::invalid_argument("AssetPlacement has no field " + it.key());
    holder::contract::AssetPlacement out;
    if (value.contains("placement_id") && !value.at("placement_id").is_null()) {
        const auto& item = value.at("placement_id");
        out.placement_id = check_text(decode_field<std::string>(item, "placement_id"), "placement_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("placement_id is required and must not be null");
    }
    if (value.contains("location_id") && !value.at("location_id").is_null()) {
        const auto& item = value.at("location_id");
        out.location_id = check_text(decode_field<std::string>(item, "location_id"), "location_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("location_id is required and must not be null");
    }
    if (value.contains("encoding") && !value.at("encoding").is_null()) {
        const auto& item = value.at("encoding");
        out.encoding = decode_field<holder::contract::AssetPlacementEncoding>(item, "encoding");
    } else {
        throw std::invalid_argument("encoding is required and must not be null");
    }
    if (value.contains("stored_byte_size") && !value.at("stored_byte_size").is_null()) {
        const auto& item = value.at("stored_byte_size");
        out.stored_byte_size = decode_field<long long>(item, "stored_byte_size");
    } else {
        throw std::invalid_argument("stored_byte_size is required and must not be null");
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = decode_field<long long>(item, "created_at");
    } else {
        throw std::invalid_argument("created_at is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::ResourceAsset>::encode(const holder::contract::ResourceAsset& value) {
    json result = json::object();
    result["asset_id"] = codec<std::string>::encode(value.asset_id);
    result["original_filename"] = codec<std::string>::encode(value.original_filename);
    result["media_type"] = codec<std::string>::encode(value.media_type);
    result["byte_size"] = codec<long long>::encode(value.byte_size);
    result["plaintext_sha256"] = codec<std::string>::encode(value.plaintext_sha256);
    result["placements"] = codec<std::vector<holder::contract::AssetPlacement>>::encode(value.placements);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::contract::ResourceAsset codec<holder::contract::ResourceAsset>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ResourceAsset object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "asset_id" && it.key() != "original_filename" && it.key() != "media_type" && it.key() != "byte_size" && it.key() != "plaintext_sha256" && it.key() != "placements" && it.key() != "created_at" && it.key() != "updated_at") throw std::invalid_argument("ResourceAsset has no field " + it.key());
    holder::contract::ResourceAsset out;
    if (value.contains("asset_id") && !value.at("asset_id").is_null()) {
        const auto& item = value.at("asset_id");
        out.asset_id = check_text(decode_field<std::string>(item, "asset_id"), "asset_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("asset_id is required and must not be null");
    }
    if (value.contains("original_filename") && !value.at("original_filename").is_null()) {
        const auto& item = value.at("original_filename");
        out.original_filename = check_text(decode_field<std::string>(item, "original_filename"), "original_filename", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("original_filename is required and must not be null");
    }
    if (value.contains("media_type") && !value.at("media_type").is_null()) {
        const auto& item = value.at("media_type");
        out.media_type = check_text(decode_field<std::string>(item, "media_type"), "media_type", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("media_type is required and must not be null");
    }
    if (value.contains("byte_size") && !value.at("byte_size").is_null()) {
        const auto& item = value.at("byte_size");
        out.byte_size = decode_field<long long>(item, "byte_size");
    } else {
        throw std::invalid_argument("byte_size is required and must not be null");
    }
    if (value.contains("plaintext_sha256") && !value.at("plaintext_sha256").is_null()) {
        const auto& item = value.at("plaintext_sha256");
        out.plaintext_sha256 = check_text(decode_field<std::string>(item, "plaintext_sha256"), "plaintext_sha256", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("plaintext_sha256 is required and must not be null");
    }
    if (value.contains("placements") && !value.at("placements").is_null()) {
        const auto& item = value.at("placements");
        out.placements = decode_field<std::vector<holder::contract::AssetPlacement>>(item, "placements");
    } else {
        throw std::invalid_argument("placements is required and must not be null");
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
    return out;
}

inline json codec<holder::contract::Resource>::encode(const holder::contract::Resource& value) {
    json result = json::object();
    result["resource_id"] = codec<std::string>::encode(value.resource_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["type"] = codec<std::string>::encode(value.type);
    result["label"] = codec<std::string>::encode(value.label);
    result["metadata"] = codec<std::map<std::string, std::vector<std::string>>>::encode(value.metadata);
    result["assets"] = codec<std::vector<holder::contract::ResourceAsset>>::encode(value.assets);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::contract::Resource codec<holder::contract::Resource>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a Resource object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "resource_id" && it.key() != "project_id" && it.key() != "type" && it.key() != "label" && it.key() != "metadata" && it.key() != "assets" && it.key() != "created_at" && it.key() != "updated_at") throw std::invalid_argument("Resource has no field " + it.key());
    holder::contract::Resource out;
    if (value.contains("resource_id") && !value.at("resource_id").is_null()) {
        const auto& item = value.at("resource_id");
        out.resource_id = check_text(decode_field<std::string>(item, "resource_id"), "resource_id", 4, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("resource_id is required and must not be null");
    }
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("type") && !value.at("type").is_null()) {
        const auto& item = value.at("type");
        out.type = check_text(decode_field<std::string>(item, "type"), "type", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("type is required and must not be null");
    }
    if (value.contains("label") && !value.at("label").is_null()) {
        const auto& item = value.at("label");
        out.label = check_text(decode_field<std::string>(item, "label"), "label", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("label is required and must not be null");
    }
    if (value.contains("metadata") && !value.at("metadata").is_null()) {
        const auto& item = value.at("metadata");
        out.metadata = [&] { auto entries = decode_field<std::map<std::string, std::vector<std::string>>>(item, "metadata"); for (const auto& [key, values] : entries) { check_text(key, "metadata key", 0, std::numeric_limits<std::size_t>::max()); for (const auto& text : values) check_text(text, "metadata", 0, std::numeric_limits<std::size_t>::max()); } return entries; }();
    } else {
        throw std::invalid_argument("metadata is required and must not be null");
    }
    if (value.contains("assets") && !value.at("assets").is_null()) {
        const auto& item = value.at("assets");
        out.assets = decode_field<std::vector<holder::contract::ResourceAsset>>(item, "assets");
    } else {
        throw std::invalid_argument("assets is required and must not be null");
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
    return out;
}

inline json codec<holder::contract::ProjectListItem>::encode(const holder::contract::ProjectListItem& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["name"] = codec<std::string>::encode(value.name);
    result["root_path"] = codec<std::string>::encode(value.root_path);
    result["git_remote_url"] = value.git_remote_url ? codec<std::string>::encode(*value.git_remote_url) : json(nullptr);
    result["git_provider"] = value.git_provider ? codec<std::string>::encode(*value.git_provider) : json(nullptr);
    result["privacy_mode"] = codec<holder::contract::ProjectPrivacyMode>::encode(value.privacy_mode);
    result["project_key_id"] = value.project_key_id ? codec<std::string>::encode(*value.project_key_id) : json(nullptr);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    if (value.card_count) result["card_count"] = codec<std::int32_t>::encode(*value.card_count);
    if (value.root_card_count) result["root_card_count"] = codec<std::int32_t>::encode(*value.root_card_count);
    return result;
}
inline holder::contract::ProjectListItem codec<holder::contract::ProjectListItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ProjectListItem object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "name" && it.key() != "root_path" && it.key() != "git_remote_url" && it.key() != "git_provider" && it.key() != "privacy_mode" && it.key() != "project_key_id" && it.key() != "created_at" && it.key() != "updated_at" && it.key() != "card_count" && it.key() != "root_card_count") throw std::invalid_argument("ProjectListItem has no field " + it.key());
    holder::contract::ProjectListItem out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("name") && !value.at("name").is_null()) {
        const auto& item = value.at("name");
        out.name = check_text(decode_field<std::string>(item, "name"), "name", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("name is required and must not be null");
    }
    if (value.contains("root_path") && !value.at("root_path").is_null()) {
        const auto& item = value.at("root_path");
        out.root_path = check_text(decode_field<std::string>(item, "root_path"), "root_path", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("root_path is required and must not be null");
    }
    if (value.contains("git_remote_url") && !value.at("git_remote_url").is_null()) {
        const auto& item = value.at("git_remote_url");
        out.git_remote_url = check_text(decode_field<std::string>(item, "git_remote_url"), "git_remote_url", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("git_provider") && !value.at("git_provider").is_null()) {
        const auto& item = value.at("git_provider");
        out.git_provider = check_text(decode_field<std::string>(item, "git_provider"), "git_provider", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("privacy_mode") && !value.at("privacy_mode").is_null()) {
        const auto& item = value.at("privacy_mode");
        out.privacy_mode = decode_field<holder::contract::ProjectPrivacyMode>(item, "privacy_mode");
    } else {
        throw std::invalid_argument("privacy_mode is required and must not be null");
    }
    if (value.contains("project_key_id") && !value.at("project_key_id").is_null()) {
        const auto& item = value.at("project_key_id");
        out.project_key_id = check_text(decode_field<std::string>(item, "project_key_id"), "project_key_id", 0, std::numeric_limits<std::size_t>::max());
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
    if (value.contains("card_count") && !value.at("card_count").is_null()) {
        const auto& item = value.at("card_count");
        out.card_count = decode_field<std::int32_t>(item, "card_count");
    } else if (value.contains("card_count")) {
        throw std::invalid_argument("card_count must not be null");
    }
    if (value.contains("root_card_count") && !value.at("root_card_count").is_null()) {
        const auto& item = value.at("root_card_count");
        out.root_card_count = decode_field<std::int32_t>(item, "root_card_count");
    } else if (value.contains("root_card_count")) {
        throw std::invalid_argument("root_card_count must not be null");
    }
    return out;
}

inline json codec<holder::contract::Project>::encode(const holder::contract::Project& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["name"] = codec<std::string>::encode(value.name);
    result["root_path"] = codec<std::string>::encode(value.root_path);
    result["git_remote_url"] = value.git_remote_url ? codec<std::string>::encode(*value.git_remote_url) : json(nullptr);
    result["git_provider"] = value.git_provider ? codec<std::string>::encode(*value.git_provider) : json(nullptr);
    result["privacy_mode"] = codec<holder::contract::ProjectPrivacyMode>::encode(value.privacy_mode);
    result["project_key_id"] = value.project_key_id ? codec<std::string>::encode(*value.project_key_id) : json(nullptr);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::contract::Project codec<holder::contract::Project>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a Project object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "name" && it.key() != "root_path" && it.key() != "git_remote_url" && it.key() != "git_provider" && it.key() != "privacy_mode" && it.key() != "project_key_id" && it.key() != "created_at" && it.key() != "updated_at") throw std::invalid_argument("Project has no field " + it.key());
    holder::contract::Project out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("name") && !value.at("name").is_null()) {
        const auto& item = value.at("name");
        out.name = check_text(decode_field<std::string>(item, "name"), "name", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("name is required and must not be null");
    }
    if (value.contains("root_path") && !value.at("root_path").is_null()) {
        const auto& item = value.at("root_path");
        out.root_path = check_text(decode_field<std::string>(item, "root_path"), "root_path", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("root_path is required and must not be null");
    }
    if (value.contains("git_remote_url") && !value.at("git_remote_url").is_null()) {
        const auto& item = value.at("git_remote_url");
        out.git_remote_url = check_text(decode_field<std::string>(item, "git_remote_url"), "git_remote_url", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("git_provider") && !value.at("git_provider").is_null()) {
        const auto& item = value.at("git_provider");
        out.git_provider = check_text(decode_field<std::string>(item, "git_provider"), "git_provider", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("privacy_mode") && !value.at("privacy_mode").is_null()) {
        const auto& item = value.at("privacy_mode");
        out.privacy_mode = decode_field<holder::contract::ProjectPrivacyMode>(item, "privacy_mode");
    } else {
        throw std::invalid_argument("privacy_mode is required and must not be null");
    }
    if (value.contains("project_key_id") && !value.at("project_key_id").is_null()) {
        const auto& item = value.at("project_key_id");
        out.project_key_id = check_text(decode_field<std::string>(item, "project_key_id"), "project_key_id", 0, std::numeric_limits<std::size_t>::max());
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

inline json codec<holder::contract::NewLink>::encode(const holder::contract::NewLink& value) {
    json result = json::object();
    result["target_type"] = codec<holder::contract::NewLinkTargetType>::encode(value.target_type);
    result["target_id"] = codec<std::string>::encode(value.target_id);
    if (value.target_project_id) result["target_project_id"] = codec<std::string>::encode(*value.target_project_id);
    result["kind"] = codec<std::string>::encode(value.kind);
    result["label"] = value.label ? codec<std::string>::encode(*value.label) : json(nullptr);
    if (value.created_at) result["created_at"] = codec<long long>::encode(*value.created_at);
    return result;
}
inline holder::contract::NewLink codec<holder::contract::NewLink>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a NewLink object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "target_type" && it.key() != "target_id" && it.key() != "target_project_id" && it.key() != "kind" && it.key() != "label" && it.key() != "created_at") throw std::invalid_argument("NewLink has no field " + it.key());
    holder::contract::NewLink out;
    if (value.contains("target_type") && !value.at("target_type").is_null()) {
        const auto& item = value.at("target_type");
        out.target_type = decode_field<holder::contract::NewLinkTargetType>(item, "target_type");
    } else {
        throw std::invalid_argument("target_type is required and must not be null");
    }
    if (value.contains("target_id") && !value.at("target_id").is_null()) {
        const auto& item = value.at("target_id");
        out.target_id = check_text(decode_field<std::string>(item, "target_id"), "target_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("target_id is required and must not be null");
    }
    if (value.contains("target_project_id") && !value.at("target_project_id").is_null()) {
        const auto& item = value.at("target_project_id");
        out.target_project_id = check_text(decode_field<std::string>(item, "target_project_id"), "target_project_id", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("target_project_id")) {
        throw std::invalid_argument("target_project_id must not be null");
    }
    if (value.contains("kind") && !value.at("kind").is_null()) {
        const auto& item = value.at("kind");
        out.kind = check_text(decode_field<std::string>(item, "kind"), "kind", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("kind")) {
        throw std::invalid_argument("kind must not be null");
    }
    if (value.contains("label") && !value.at("label").is_null()) {
        const auto& item = value.at("label");
        out.label = check_text(decode_field<std::string>(item, "label"), "label", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = decode_field<long long>(item, "created_at");
    } else if (value.contains("created_at")) {
        throw std::invalid_argument("created_at must not be null");
    }
    return out;
}

inline json codec<holder::contract::NewMilestone>::encode(const holder::contract::NewMilestone& value) {
    json result = json::object();
    result["dates"] = codec<std::vector<holder::contract::MilestoneDate>>::encode(value.dates);
    result["kind"] = value.kind ? codec<std::string>::encode(*value.kind) : json(nullptr);
    result["description"] = value.description ? codec<std::string>::encode(*value.description) : json(nullptr);
    return result;
}
inline holder::contract::NewMilestone codec<holder::contract::NewMilestone>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a NewMilestone object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "dates" && it.key() != "kind" && it.key() != "description") throw std::invalid_argument("NewMilestone has no field " + it.key());
    holder::contract::NewMilestone out;
    if (value.contains("dates") && !value.at("dates").is_null()) {
        const auto& item = value.at("dates");
        out.dates = decode_field<std::vector<holder::contract::MilestoneDate>>(item, "dates");
    } else {
        throw std::invalid_argument("dates is required and must not be null");
    }
    if (value.contains("kind") && !value.at("kind").is_null()) {
        const auto& item = value.at("kind");
        out.kind = check_text(decode_field<std::string>(item, "kind"), "kind", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("description") && !value.at("description").is_null()) {
        const auto& item = value.at("description");
        out.description = check_text(decode_field<std::string>(item, "description"), "description", 1, std::numeric_limits<std::size_t>::max());
    }
    return out;
}

inline json codec<holder::contract::MilestoneUpdate>::encode(const holder::contract::MilestoneUpdate& value) {
    json result = json::object();
    if (value.dates) result["dates"] = codec<std::vector<holder::contract::MilestoneDate>>::encode(*value.dates);
    if (value.kind) result["kind"] = *value.kind ? codec<std::string>::encode(**value.kind) : json(nullptr);
    if (value.description) result["description"] = *value.description ? codec<std::string>::encode(**value.description) : json(nullptr);
    return result;
}
inline holder::contract::MilestoneUpdate codec<holder::contract::MilestoneUpdate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a MilestoneUpdate object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "dates" && it.key() != "kind" && it.key() != "description") throw std::invalid_argument("MilestoneUpdate has no field " + it.key());
    holder::contract::MilestoneUpdate out;
    if (value.contains("dates") && !value.at("dates").is_null()) {
        const auto& item = value.at("dates");
        out.dates = decode_field<std::vector<holder::contract::MilestoneDate>>(item, "dates");
    } else if (value.contains("dates")) {
        throw std::invalid_argument("dates must not be null");
    }
    if (value.contains("kind") && !value.at("kind").is_null()) {
        const auto& item = value.at("kind");
        out.kind = check_text(decode_field<std::string>(item, "kind"), "kind", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("kind")) {
        out.kind.emplace();
    }
    if (value.contains("description") && !value.at("description").is_null()) {
        const auto& item = value.at("description");
        out.description = check_text(decode_field<std::string>(item, "description"), "description", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("description")) {
        out.description.emplace();
    }
    return out;
}

inline json codec<holder::contract::NewResource>::encode(const holder::contract::NewResource& value) {
    json result = json::object();
    if (value.resource_id) result["resource_id"] = codec<std::string>::encode(*value.resource_id);
    result["type"] = codec<std::string>::encode(value.type);
    result["label"] = codec<std::string>::encode(value.label);
    if (value.metadata) result["metadata"] = codec<std::map<std::string, std::vector<std::string>>>::encode(*value.metadata);
    if (value.created_at) result["created_at"] = codec<long long>::encode(*value.created_at);
    if (value.updated_at) result["updated_at"] = codec<long long>::encode(*value.updated_at);
    return result;
}
inline holder::contract::NewResource codec<holder::contract::NewResource>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a NewResource object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "resource_id" && it.key() != "type" && it.key() != "label" && it.key() != "metadata" && it.key() != "created_at" && it.key() != "updated_at") throw std::invalid_argument("NewResource has no field " + it.key());
    holder::contract::NewResource out;
    if (value.contains("resource_id") && !value.at("resource_id").is_null()) {
        const auto& item = value.at("resource_id");
        out.resource_id = check_text(decode_field<std::string>(item, "resource_id"), "resource_id", 4, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("resource_id")) {
        throw std::invalid_argument("resource_id must not be null");
    }
    if (value.contains("type") && !value.at("type").is_null()) {
        const auto& item = value.at("type");
        out.type = check_text(decode_field<std::string>(item, "type"), "type", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("type is required and must not be null");
    }
    if (value.contains("label") && !value.at("label").is_null()) {
        const auto& item = value.at("label");
        out.label = check_text(decode_field<std::string>(item, "label"), "label", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("label is required and must not be null");
    }
    if (value.contains("metadata") && !value.at("metadata").is_null()) {
        const auto& item = value.at("metadata");
        out.metadata = [&] { auto entries = decode_field<std::map<std::string, std::vector<std::string>>>(item, "metadata"); for (const auto& [key, values] : entries) { check_text(key, "metadata key", 0, std::numeric_limits<std::size_t>::max()); for (const auto& text : values) check_text(text, "metadata", 0, std::numeric_limits<std::size_t>::max()); } return entries; }();
    } else if (value.contains("metadata")) {
        throw std::invalid_argument("metadata must not be null");
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

inline json codec<holder::contract::ResourceUpdate>::encode(const holder::contract::ResourceUpdate& value) {
    json result = json::object();
    if (value.type) result["type"] = codec<std::string>::encode(*value.type);
    if (value.label) result["label"] = codec<std::string>::encode(*value.label);
    if (value.metadata) result["metadata"] = codec<std::map<std::string, std::vector<std::string>>>::encode(*value.metadata);
    if (value.updated_at) result["updated_at"] = codec<long long>::encode(*value.updated_at);
    return result;
}
inline holder::contract::ResourceUpdate codec<holder::contract::ResourceUpdate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ResourceUpdate object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "type" && it.key() != "label" && it.key() != "metadata" && it.key() != "updated_at") throw std::invalid_argument("ResourceUpdate has no field " + it.key());
    holder::contract::ResourceUpdate out;
    if (value.contains("type") && !value.at("type").is_null()) {
        const auto& item = value.at("type");
        out.type = check_text(decode_field<std::string>(item, "type"), "type", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("type")) {
        throw std::invalid_argument("type must not be null");
    }
    if (value.contains("label") && !value.at("label").is_null()) {
        const auto& item = value.at("label");
        out.label = check_text(decode_field<std::string>(item, "label"), "label", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("label")) {
        throw std::invalid_argument("label must not be null");
    }
    if (value.contains("metadata") && !value.at("metadata").is_null()) {
        const auto& item = value.at("metadata");
        out.metadata = [&] { auto entries = decode_field<std::map<std::string, std::vector<std::string>>>(item, "metadata"); for (const auto& [key, values] : entries) { check_text(key, "metadata key", 0, std::numeric_limits<std::size_t>::max()); for (const auto& text : values) check_text(text, "metadata", 0, std::numeric_limits<std::size_t>::max()); } return entries; }();
    } else if (value.contains("metadata")) {
        throw std::invalid_argument("metadata must not be null");
    }
    if (value.contains("updated_at") && !value.at("updated_at").is_null()) {
        const auto& item = value.at("updated_at");
        out.updated_at = decode_field<long long>(item, "updated_at");
    } else if (value.contains("updated_at")) {
        throw std::invalid_argument("updated_at must not be null");
    }
    return out;
}

inline json codec<holder::contract::NewProject>::encode(const holder::contract::NewProject& value) {
    json result = json::object();
    if (value.project_id) result["project_id"] = codec<std::string>::encode(*value.project_id);
    result["name"] = codec<std::string>::encode(value.name);
    if (value.root_path) result["root_path"] = codec<std::string>::encode(*value.root_path);
    result["git_remote_url"] = value.git_remote_url ? codec<std::string>::encode(*value.git_remote_url) : json(nullptr);
    result["git_provider"] = value.git_provider ? codec<std::string>::encode(*value.git_provider) : json(nullptr);
    result["privacy_mode"] = codec<holder::contract::NewProjectPrivacyMode>::encode(value.privacy_mode);
    if (value.created_at) result["created_at"] = codec<long long>::encode(*value.created_at);
    if (value.updated_at) result["updated_at"] = codec<long long>::encode(*value.updated_at);
    return result;
}
inline holder::contract::NewProject codec<holder::contract::NewProject>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a NewProject object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "name" && it.key() != "root_path" && it.key() != "git_remote_url" && it.key() != "git_provider" && it.key() != "privacy_mode" && it.key() != "created_at" && it.key() != "updated_at") throw std::invalid_argument("NewProject has no field " + it.key());
    holder::contract::NewProject out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("project_id")) {
        throw std::invalid_argument("project_id must not be null");
    }
    if (value.contains("name") && !value.at("name").is_null()) {
        const auto& item = value.at("name");
        out.name = check_text(decode_field<std::string>(item, "name"), "name", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("name is required and must not be null");
    }
    if (value.contains("root_path") && !value.at("root_path").is_null()) {
        const auto& item = value.at("root_path");
        out.root_path = check_text(decode_field<std::string>(item, "root_path"), "root_path", 0, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("root_path")) {
        throw std::invalid_argument("root_path must not be null");
    }
    if (value.contains("git_remote_url") && !value.at("git_remote_url").is_null()) {
        const auto& item = value.at("git_remote_url");
        out.git_remote_url = check_text(decode_field<std::string>(item, "git_remote_url"), "git_remote_url", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("git_provider") && !value.at("git_provider").is_null()) {
        const auto& item = value.at("git_provider");
        out.git_provider = check_text(decode_field<std::string>(item, "git_provider"), "git_provider", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("privacy_mode") && !value.at("privacy_mode").is_null()) {
        const auto& item = value.at("privacy_mode");
        out.privacy_mode = decode_field<holder::contract::NewProjectPrivacyMode>(item, "privacy_mode");
    } else if (value.contains("privacy_mode")) {
        throw std::invalid_argument("privacy_mode must not be null");
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = check_range<long long>(decode_field<long long>(item, "created_at"), "created_at", 1, std::numeric_limits<long long>::max());
    } else if (value.contains("created_at")) {
        throw std::invalid_argument("created_at must not be null");
    }
    if (value.contains("updated_at") && !value.at("updated_at").is_null()) {
        const auto& item = value.at("updated_at");
        out.updated_at = check_range<long long>(decode_field<long long>(item, "updated_at"), "updated_at", 1, std::numeric_limits<long long>::max());
    } else if (value.contains("updated_at")) {
        throw std::invalid_argument("updated_at must not be null");
    }
    return out;
}

inline json codec<holder::contract::ProjectUpdate>::encode(const holder::contract::ProjectUpdate& value) {
    json result = json::object();
    if (value.name) result["name"] = codec<std::string>::encode(*value.name);
    if (value.git_remote_url) result["git_remote_url"] = *value.git_remote_url ? codec<std::string>::encode(**value.git_remote_url) : json(nullptr);
    if (value.git_provider) result["git_provider"] = *value.git_provider ? codec<std::string>::encode(**value.git_provider) : json(nullptr);
    return result;
}
inline holder::contract::ProjectUpdate codec<holder::contract::ProjectUpdate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ProjectUpdate object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "name" && it.key() != "git_remote_url" && it.key() != "git_provider") throw std::invalid_argument("ProjectUpdate has no field " + it.key());
    holder::contract::ProjectUpdate out;
    if (value.contains("name") && !value.at("name").is_null()) {
        const auto& item = value.at("name");
        out.name = check_text(decode_field<std::string>(item, "name"), "name", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("name")) {
        throw std::invalid_argument("name must not be null");
    }
    if (value.contains("git_remote_url") && !value.at("git_remote_url").is_null()) {
        const auto& item = value.at("git_remote_url");
        out.git_remote_url = check_text(decode_field<std::string>(item, "git_remote_url"), "git_remote_url", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("git_remote_url")) {
        out.git_remote_url.emplace();
    }
    if (value.contains("git_provider") && !value.at("git_provider").is_null()) {
        const auto& item = value.at("git_provider");
        out.git_provider = check_text(decode_field<std::string>(item, "git_provider"), "git_provider", 0, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("git_provider")) {
        out.git_provider.emplace();
    }
    return out;
}

template <typename T> json encode(const T& value) { return codec<T>::encode(value); }
template <typename T> T decode(const json& value) { return codec<T>::decode(value); }
}  // namespace contract_json
