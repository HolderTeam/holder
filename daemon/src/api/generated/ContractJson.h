// Generated from contract/interface/ai.py, contract/interface/card_references.py, contract/interface/cards/models.py, contract/interface/cards/operations.py, contract/interface/daemon_routes.py, contract/interface/errors.py, contract/interface/git.py, contract/interface/history.py, contract/interface/links.py, contract/interface/locations.py, contract/interface/maintenance.py, contract/interface/milestones.py, contract/interface/operations/ai.py, contract/interface/operations/git.py, contract/interface/operations/history.py, contract/interface/operations/links.py, contract/interface/operations/locations.py, contract/interface/operations/maintenance.py, contract/interface/operations/milestones.py, contract/interface/operations/projects.py, contract/interface/operations/resources.py, contract/interface/operations/search.py, contract/interface/operations/system.py, contract/interface/operations/trash.py, contract/interface/projects.py, contract/interface/rebuild.py, contract/interface/recovery.py, contract/interface/resources.py, contract/interface/search.py, contract/interface/trash.py by holder_contract.
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

template <> struct codec<holder::contract::ProjectStorageLocationProvider> {
    static json encode(holder::contract::ProjectStorageLocationProvider value) {
        switch (value) {
        case holder::contract::ProjectStorageLocationProvider::LocalDirectory: return "local_directory";
        case holder::contract::ProjectStorageLocationProvider::S3Compatible: return "s3_compatible";
        case holder::contract::ProjectStorageLocationProvider::GoogleDrive: return "google-drive";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::ProjectStorageLocationProvider decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "local_directory") return holder::contract::ProjectStorageLocationProvider::LocalDirectory;
        if (text == "s3_compatible") return holder::contract::ProjectStorageLocationProvider::S3Compatible;
        if (text == "google-drive") return holder::contract::ProjectStorageLocationProvider::GoogleDrive;
        throw std::invalid_argument("must be one of local_directory, s3_compatible, google-drive");
    }
};

template <> struct codec<holder::contract::CardBreadcrumbKind> {
    static json encode(holder::contract::CardBreadcrumbKind value) {
        switch (value) {
        case holder::contract::CardBreadcrumbKind::Project: return "project";
        case holder::contract::CardBreadcrumbKind::Card: return "card";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::CardBreadcrumbKind decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "project") return holder::contract::CardBreadcrumbKind::Project;
        if (text == "card") return holder::contract::CardBreadcrumbKind::Card;
        throw std::invalid_argument("must be one of project, card");
    }
};

template <> struct codec<holder::contract::CardReferenceResolveDataStatus> {
    static json encode(holder::contract::CardReferenceResolveDataStatus value) {
        switch (value) {
        case holder::contract::CardReferenceResolveDataStatus::Resolved: return "resolved";
        case holder::contract::CardReferenceResolveDataStatus::Ambiguous: return "ambiguous";
        case holder::contract::CardReferenceResolveDataStatus::NotFound: return "not_found";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::CardReferenceResolveDataStatus decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "resolved") return holder::contract::CardReferenceResolveDataStatus::Resolved;
        if (text == "ambiguous") return holder::contract::CardReferenceResolveDataStatus::Ambiguous;
        if (text == "not_found") return holder::contract::CardReferenceResolveDataStatus::NotFound;
        throw std::invalid_argument("must be one of resolved, ambiguous, not_found");
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

template <> struct codec<holder::contract::ProjectEncryptionCheckPrivacyMode> {
    static json encode(holder::contract::ProjectEncryptionCheckPrivacyMode value) {
        switch (value) {
        case holder::contract::ProjectEncryptionCheckPrivacyMode::EncryptedGit: return "encrypted_git";
        case holder::contract::ProjectEncryptionCheckPrivacyMode::Plain: return "plain";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::ProjectEncryptionCheckPrivacyMode decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "encrypted_git") return holder::contract::ProjectEncryptionCheckPrivacyMode::EncryptedGit;
        if (text == "plain") return holder::contract::ProjectEncryptionCheckPrivacyMode::Plain;
        throw std::invalid_argument("must be one of encrypted_git, plain");
    }
};

template <> struct codec<holder::contract::RecoveryTokenGlobalImportResultPullStatus> {
    static json encode(holder::contract::RecoveryTokenGlobalImportResultPullStatus value) {
        switch (value) {
        case holder::contract::RecoveryTokenGlobalImportResultPullStatus::NotAttempted: return "not_attempted";
        case holder::contract::RecoveryTokenGlobalImportResultPullStatus::Succeeded: return "succeeded";
        case holder::contract::RecoveryTokenGlobalImportResultPullStatus::Failed: return "failed";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::RecoveryTokenGlobalImportResultPullStatus decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "not_attempted") return holder::contract::RecoveryTokenGlobalImportResultPullStatus::NotAttempted;
        if (text == "succeeded") return holder::contract::RecoveryTokenGlobalImportResultPullStatus::Succeeded;
        if (text == "failed") return holder::contract::RecoveryTokenGlobalImportResultPullStatus::Failed;
        throw std::invalid_argument("must be one of not_attempted, succeeded, failed");
    }
};

template <> struct codec<holder::contract::PullPhaseStatus> {
    static json encode(holder::contract::PullPhaseStatus value) {
        switch (value) {
        case holder::contract::PullPhaseStatus::NotAttempted: return "not_attempted";
        case holder::contract::PullPhaseStatus::Succeeded: return "succeeded";
        case holder::contract::PullPhaseStatus::RemoteUnset: return "remote_unset";
        case holder::contract::PullPhaseStatus::Failed: return "failed";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::PullPhaseStatus decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "not_attempted") return holder::contract::PullPhaseStatus::NotAttempted;
        if (text == "succeeded") return holder::contract::PullPhaseStatus::Succeeded;
        if (text == "remote_unset") return holder::contract::PullPhaseStatus::RemoteUnset;
        if (text == "failed") return holder::contract::PullPhaseStatus::Failed;
        throw std::invalid_argument("must be one of not_attempted, succeeded, remote_unset, failed");
    }
};

template <> struct codec<holder::contract::PushPhaseStatus> {
    static json encode(holder::contract::PushPhaseStatus value) {
        switch (value) {
        case holder::contract::PushPhaseStatus::NotAttempted: return "not_attempted";
        case holder::contract::PushPhaseStatus::Pushed: return "pushed";
        case holder::contract::PushPhaseStatus::UpToDate: return "up_to_date";
        case holder::contract::PushPhaseStatus::AuthFailed: return "auth_failed";
        case holder::contract::PushPhaseStatus::NotFound: return "not_found";
        case holder::contract::PushPhaseStatus::NetworkError: return "network_error";
        case holder::contract::PushPhaseStatus::NonFastForward: return "non_fast_forward";
        case holder::contract::PushPhaseStatus::RemoteUnset: return "remote_unset";
        case holder::contract::PushPhaseStatus::UnknownError: return "unknown_error";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::PushPhaseStatus decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "not_attempted") return holder::contract::PushPhaseStatus::NotAttempted;
        if (text == "pushed") return holder::contract::PushPhaseStatus::Pushed;
        if (text == "up_to_date") return holder::contract::PushPhaseStatus::UpToDate;
        if (text == "auth_failed") return holder::contract::PushPhaseStatus::AuthFailed;
        if (text == "not_found") return holder::contract::PushPhaseStatus::NotFound;
        if (text == "network_error") return holder::contract::PushPhaseStatus::NetworkError;
        if (text == "non_fast_forward") return holder::contract::PushPhaseStatus::NonFastForward;
        if (text == "remote_unset") return holder::contract::PushPhaseStatus::RemoteUnset;
        if (text == "unknown_error") return holder::contract::PushPhaseStatus::UnknownError;
        throw std::invalid_argument("must be one of not_attempted, pushed, up_to_date, auth_failed, not_found, network_error, non_fast_forward, remote_unset, unknown_error");
    }
};

template <> struct codec<holder::contract::SyncResultStatus> {
    static json encode(holder::contract::SyncResultStatus value) {
        switch (value) {
        case holder::contract::SyncResultStatus::Succeeded: return "succeeded";
        case holder::contract::SyncResultStatus::Failed: return "failed";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::SyncResultStatus decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "succeeded") return holder::contract::SyncResultStatus::Succeeded;
        if (text == "failed") return holder::contract::SyncResultStatus::Failed;
        throw std::invalid_argument("must be one of succeeded, failed");
    }
};

template <> struct codec<holder::contract::RemoteTestResultStatus> {
    static json encode(holder::contract::RemoteTestResultStatus value) {
        switch (value) {
        case holder::contract::RemoteTestResultStatus::Reachable: return "reachable";
        case holder::contract::RemoteTestResultStatus::AuthFailed: return "auth_failed";
        case holder::contract::RemoteTestResultStatus::NotFound: return "not_found";
        case holder::contract::RemoteTestResultStatus::NetworkError: return "network_error";
        case holder::contract::RemoteTestResultStatus::InvalidRemoteUrl: return "invalid_remote_url";
        case holder::contract::RemoteTestResultStatus::RemoteUnset: return "remote_unset";
        case holder::contract::RemoteTestResultStatus::UnknownError: return "unknown_error";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::RemoteTestResultStatus decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "reachable") return holder::contract::RemoteTestResultStatus::Reachable;
        if (text == "auth_failed") return holder::contract::RemoteTestResultStatus::AuthFailed;
        if (text == "not_found") return holder::contract::RemoteTestResultStatus::NotFound;
        if (text == "network_error") return holder::contract::RemoteTestResultStatus::NetworkError;
        if (text == "invalid_remote_url") return holder::contract::RemoteTestResultStatus::InvalidRemoteUrl;
        if (text == "remote_unset") return holder::contract::RemoteTestResultStatus::RemoteUnset;
        if (text == "unknown_error") return holder::contract::RemoteTestResultStatus::UnknownError;
        throw std::invalid_argument("must be one of reachable, auth_failed, not_found, network_error, invalid_remote_url, remote_unset, unknown_error");
    }
};

template <> struct codec<holder::contract::HistoryAffectedObjectKind> {
    static json encode(holder::contract::HistoryAffectedObjectKind value) {
        switch (value) {
        case holder::contract::HistoryAffectedObjectKind::Card: return "card";
        case holder::contract::HistoryAffectedObjectKind::Resource: return "resource";
        case holder::contract::HistoryAffectedObjectKind::Location: return "location";
        case holder::contract::HistoryAffectedObjectKind::AiData: return "ai_data";
        case holder::contract::HistoryAffectedObjectKind::ProjectSettings: return "project_settings";
        case holder::contract::HistoryAffectedObjectKind::Unknown: return "unknown";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::HistoryAffectedObjectKind decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "card") return holder::contract::HistoryAffectedObjectKind::Card;
        if (text == "resource") return holder::contract::HistoryAffectedObjectKind::Resource;
        if (text == "location") return holder::contract::HistoryAffectedObjectKind::Location;
        if (text == "ai_data") return holder::contract::HistoryAffectedObjectKind::AiData;
        if (text == "project_settings") return holder::contract::HistoryAffectedObjectKind::ProjectSettings;
        if (text == "unknown") return holder::contract::HistoryAffectedObjectKind::Unknown;
        throw std::invalid_argument("must be one of card, resource, location, ai_data, project_settings, unknown");
    }
};

template <> struct codec<holder::contract::TrashItemType> {
    static json encode(holder::contract::TrashItemType value) {
        switch (value) {
        case holder::contract::TrashItemType::Card: return "card";
        case holder::contract::TrashItemType::AiMessage: return "ai_message";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::TrashItemType decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "card") return holder::contract::TrashItemType::Card;
        if (text == "ai_message") return holder::contract::TrashItemType::AiMessage;
        throw std::invalid_argument("must be one of card, ai_message");
    }
};

template <> struct codec<holder::contract::NewStorageLocationProvider> {
    static json encode(holder::contract::NewStorageLocationProvider value) {
        switch (value) {
        case holder::contract::NewStorageLocationProvider::LocalDirectory: return "local_directory";
        case holder::contract::NewStorageLocationProvider::S3Compatible: return "s3_compatible";
        case holder::contract::NewStorageLocationProvider::GoogleDrive: return "google-drive";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::NewStorageLocationProvider decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "local_directory") return holder::contract::NewStorageLocationProvider::LocalDirectory;
        if (text == "s3_compatible") return holder::contract::NewStorageLocationProvider::S3Compatible;
        if (text == "google-drive") return holder::contract::NewStorageLocationProvider::GoogleDrive;
        throw std::invalid_argument("must be one of local_directory, s3_compatible, google-drive");
    }
};

template <> struct codec<holder::contract::CardReferenceResolveRequestScope> {
    static json encode(holder::contract::CardReferenceResolveRequestScope value) {
        switch (value) {
        case holder::contract::CardReferenceResolveRequestScope::Live: return "live";
        case holder::contract::CardReferenceResolveRequestScope::Trashed: return "trashed";
        case holder::contract::CardReferenceResolveRequestScope::Either: return "either";
        }
        throw std::invalid_argument("is not a known value");
    }
    static holder::contract::CardReferenceResolveRequestScope decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        const auto& text = value.get_ref<const std::string&>();
        if (text == "live") return holder::contract::CardReferenceResolveRequestScope::Live;
        if (text == "trashed") return holder::contract::CardReferenceResolveRequestScope::Trashed;
        if (text == "either") return holder::contract::CardReferenceResolveRequestScope::Either;
        throw std::invalid_argument("must be one of live, trashed, either");
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

template <> struct codec<holder::contract::ProjectStorageLocation> {
    static json encode(const holder::contract::ProjectStorageLocation& value);
    static holder::contract::ProjectStorageLocation decode(const json& value);
};
template <> struct codec<holder::contract::LocationDeleteResult> {
    static json encode(const holder::contract::LocationDeleteResult& value);
    static holder::contract::LocationDeleteResult decode(const json& value);
};
template <> struct codec<holder::contract::TagOccurrence> {
    static json encode(const holder::contract::TagOccurrence& value);
    static holder::contract::TagOccurrence decode(const json& value);
};
template <> struct codec<holder::contract::Card> {
    static json encode(const holder::contract::Card& value);
    static holder::contract::Card decode(const json& value);
};
template <> struct codec<holder::contract::CardMetadata> {
    static json encode(const holder::contract::CardMetadata& value);
    static holder::contract::CardMetadata decode(const json& value);
};
template <> struct codec<holder::contract::CardBreadcrumb> {
    static json encode(const holder::contract::CardBreadcrumb& value);
    static holder::contract::CardBreadcrumb decode(const json& value);
};
template <> struct codec<holder::contract::CardReferenceCandidate> {
    static json encode(const holder::contract::CardReferenceCandidate& value);
    static holder::contract::CardReferenceCandidate decode(const json& value);
};
template <> struct codec<holder::contract::CardReferenceResolveData> {
    static json encode(const holder::contract::CardReferenceResolveData& value);
    static holder::contract::CardReferenceResolveData decode(const json& value);
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
template <> struct codec<holder::contract::EncryptionSafetyCheck> {
    static json encode(const holder::contract::EncryptionSafetyCheck& value);
    static holder::contract::EncryptionSafetyCheck decode(const json& value);
};
template <> struct codec<holder::contract::ProjectEncryptionCheck> {
    static json encode(const holder::contract::ProjectEncryptionCheck& value);
    static holder::contract::ProjectEncryptionCheck decode(const json& value);
};
template <> struct codec<holder::contract::HierarchyRepairMove> {
    static json encode(const holder::contract::HierarchyRepairMove& value);
    static holder::contract::HierarchyRepairMove decode(const json& value);
};
template <> struct codec<holder::contract::HierarchyRepair> {
    static json encode(const holder::contract::HierarchyRepair& value);
    static holder::contract::HierarchyRepair decode(const json& value);
};
template <> struct codec<holder::contract::ProjectRecoveryTokenExportResult> {
    static json encode(const holder::contract::ProjectRecoveryTokenExportResult& value);
    static holder::contract::ProjectRecoveryTokenExportResult decode(const json& value);
};
template <> struct codec<holder::contract::ProjectRecoveryTokenImportResult> {
    static json encode(const holder::contract::ProjectRecoveryTokenImportResult& value);
    static holder::contract::ProjectRecoveryTokenImportResult decode(const json& value);
};
template <> struct codec<holder::contract::RecoveryTokenGlobalImportResult> {
    static json encode(const holder::contract::RecoveryTokenGlobalImportResult& value);
    static holder::contract::RecoveryTokenGlobalImportResult decode(const json& value);
};
template <> struct codec<holder::contract::RebuildResult> {
    static json encode(const holder::contract::RebuildResult& value);
    static holder::contract::RebuildResult decode(const json& value);
};
template <> struct codec<holder::contract::ReindexResult> {
    static json encode(const holder::contract::ReindexResult& value);
    static holder::contract::ReindexResult decode(const json& value);
};
template <> struct codec<holder::contract::SyncState> {
    static json encode(const holder::contract::SyncState& value);
    static holder::contract::SyncState decode(const json& value);
};
template <> struct codec<holder::contract::PullPhase> {
    static json encode(const holder::contract::PullPhase& value);
    static holder::contract::PullPhase decode(const json& value);
};
template <> struct codec<holder::contract::PushPhase> {
    static json encode(const holder::contract::PushPhase& value);
    static holder::contract::PushPhase decode(const json& value);
};
template <> struct codec<holder::contract::SyncResult> {
    static json encode(const holder::contract::SyncResult& value);
    static holder::contract::SyncResult decode(const json& value);
};
template <> struct codec<holder::contract::DueSyncResult> {
    static json encode(const holder::contract::DueSyncResult& value);
    static holder::contract::DueSyncResult decode(const json& value);
};
template <> struct codec<holder::contract::RemoteTestResult> {
    static json encode(const holder::contract::RemoteTestResult& value);
    static holder::contract::RemoteTestResult decode(const json& value);
};
template <> struct codec<holder::contract::HistoryAuthor> {
    static json encode(const holder::contract::HistoryAuthor& value);
    static holder::contract::HistoryAuthor decode(const json& value);
};
template <> struct codec<holder::contract::HistoryAffectedPath> {
    static json encode(const holder::contract::HistoryAffectedPath& value);
    static holder::contract::HistoryAffectedPath decode(const json& value);
};
template <> struct codec<holder::contract::HistoryAffectedObject> {
    static json encode(const holder::contract::HistoryAffectedObject& value);
    static holder::contract::HistoryAffectedObject decode(const json& value);
};
template <> struct codec<holder::contract::ProjectActivity> {
    static json encode(const holder::contract::ProjectActivity& value);
    static holder::contract::ProjectActivity decode(const json& value);
};
template <> struct codec<holder::contract::ProjectHistoryPage> {
    static json encode(const holder::contract::ProjectHistoryPage& value);
    static holder::contract::ProjectHistoryPage decode(const json& value);
};
template <> struct codec<holder::contract::CardHistorySave> {
    static json encode(const holder::contract::CardHistorySave& value);
    static holder::contract::CardHistorySave decode(const json& value);
};
template <> struct codec<holder::contract::CardHistoryEntry> {
    static json encode(const holder::contract::CardHistoryEntry& value);
    static holder::contract::CardHistoryEntry decode(const json& value);
};
template <> struct codec<holder::contract::CardHistoryPage> {
    static json encode(const holder::contract::CardHistoryPage& value);
    static holder::contract::CardHistoryPage decode(const json& value);
};
template <> struct codec<holder::contract::CardHistoryVersion> {
    static json encode(const holder::contract::CardHistoryVersion& value);
    static holder::contract::CardHistoryVersion decode(const json& value);
};
template <> struct codec<holder::contract::CardHistoryDiffLine> {
    static json encode(const holder::contract::CardHistoryDiffLine& value);
    static holder::contract::CardHistoryDiffLine decode(const json& value);
};
template <> struct codec<holder::contract::CardHistoryComparison> {
    static json encode(const holder::contract::CardHistoryComparison& value);
    static holder::contract::CardHistoryComparison decode(const json& value);
};
template <> struct codec<holder::contract::CardHistorySnapshot> {
    static json encode(const holder::contract::CardHistorySnapshot& value);
    static holder::contract::CardHistorySnapshot decode(const json& value);
};
template <> struct codec<holder::contract::CardHistoryRestoreResult> {
    static json encode(const holder::contract::CardHistoryRestoreResult& value);
    static holder::contract::CardHistoryRestoreResult decode(const json& value);
};
template <> struct codec<holder::contract::TrashItem> {
    static json encode(const holder::contract::TrashItem& value);
    static holder::contract::TrashItem decode(const json& value);
};
template <> struct codec<holder::contract::TrashEmptyResult> {
    static json encode(const holder::contract::TrashEmptyResult& value);
    static holder::contract::TrashEmptyResult decode(const json& value);
};
template <> struct codec<holder::contract::TrashDeleteResult> {
    static json encode(const holder::contract::TrashDeleteResult& value);
    static holder::contract::TrashDeleteResult decode(const json& value);
};
template <> struct codec<holder::contract::SearchCard> {
    static json encode(const holder::contract::SearchCard& value);
    static holder::contract::SearchCard decode(const json& value);
};
template <> struct codec<holder::contract::SearchMessage> {
    static json encode(const holder::contract::SearchMessage& value);
    static holder::contract::SearchMessage decode(const json& value);
};
template <> struct codec<holder::contract::NewStorageLocation> {
    static json encode(const holder::contract::NewStorageLocation& value);
    static holder::contract::NewStorageLocation decode(const json& value);
};
template <> struct codec<holder::contract::StorageLocationUpdate> {
    static json encode(const holder::contract::StorageLocationUpdate& value);
    static holder::contract::StorageLocationUpdate decode(const json& value);
};
template <> struct codec<holder::contract::CardReferenceResolveRequest> {
    static json encode(const holder::contract::CardReferenceResolveRequest& value);
    static holder::contract::CardReferenceResolveRequest decode(const json& value);
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
template <> struct codec<holder::contract::HierarchyRepairRequest> {
    static json encode(const holder::contract::HierarchyRepairRequest& value);
    static holder::contract::HierarchyRepairRequest decode(const json& value);
};
template <> struct codec<holder::contract::RecoveryTokenExportRequest> {
    static json encode(const holder::contract::RecoveryTokenExportRequest& value);
    static holder::contract::RecoveryTokenExportRequest decode(const json& value);
};
template <> struct codec<holder::contract::RecoveryTokenImportRequest> {
    static json encode(const holder::contract::RecoveryTokenImportRequest& value);
    static holder::contract::RecoveryTokenImportRequest decode(const json& value);
};
template <> struct codec<holder::contract::RebuildRequest> {
    static json encode(const holder::contract::RebuildRequest& value);
    static holder::contract::RebuildRequest decode(const json& value);
};
template <> struct codec<holder::contract::PushOptions> {
    static json encode(const holder::contract::PushOptions& value);
    static holder::contract::PushOptions decode(const json& value);
};
template <> struct codec<holder::contract::RemoteTest> {
    static json encode(const holder::contract::RemoteTest& value);
    static holder::contract::RemoteTest decode(const json& value);
};

inline json codec<holder::contract::ProjectStorageLocation>::encode(const holder::contract::ProjectStorageLocation& value) {
    json result = json::object();
    result["location_id"] = codec<std::string>::encode(value.location_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["name"] = codec<std::string>::encode(value.name);
    result["provider"] = codec<holder::contract::ProjectStorageLocationProvider>::encode(value.provider);
    result["configuration"] = codec<std::map<std::string, std::string>>::encode(value.configuration);
    result["bound"] = codec<bool>::encode(value.bound);
    result["binding_preview"] = value.binding_preview ? codec<std::string>::encode(*value.binding_preview) : json(nullptr);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::contract::ProjectStorageLocation codec<holder::contract::ProjectStorageLocation>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ProjectStorageLocation object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "location_id" && it.key() != "project_id" && it.key() != "name" && it.key() != "provider" && it.key() != "configuration" && it.key() != "bound" && it.key() != "binding_preview" && it.key() != "created_at" && it.key() != "updated_at") throw std::invalid_argument("ProjectStorageLocation has no field " + it.key());
    holder::contract::ProjectStorageLocation out;
    if (value.contains("location_id") && !value.at("location_id").is_null()) {
        const auto& item = value.at("location_id");
        out.location_id = check_text(decode_field<std::string>(item, "location_id"), "location_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("location_id is required and must not be null");
    }
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
    if (value.contains("provider") && !value.at("provider").is_null()) {
        const auto& item = value.at("provider");
        out.provider = decode_field<holder::contract::ProjectStorageLocationProvider>(item, "provider");
    } else {
        throw std::invalid_argument("provider is required and must not be null");
    }
    if (value.contains("configuration") && !value.at("configuration").is_null()) {
        const auto& item = value.at("configuration");
        out.configuration = [&] { auto entries = decode_field<std::map<std::string, std::string>>(item, "configuration"); for (const auto& [key, text] : entries) { check_text(key, "configuration key", 0, std::numeric_limits<std::size_t>::max()); check_text(text, "configuration", 0, std::numeric_limits<std::size_t>::max()); } return entries; }();
    } else {
        throw std::invalid_argument("configuration is required and must not be null");
    }
    if (value.contains("bound") && !value.at("bound").is_null()) {
        const auto& item = value.at("bound");
        out.bound = decode_field<bool>(item, "bound");
    } else {
        throw std::invalid_argument("bound is required and must not be null");
    }
    if (value.contains("binding_preview") && !value.at("binding_preview").is_null()) {
        const auto& item = value.at("binding_preview");
        out.binding_preview = check_text(decode_field<std::string>(item, "binding_preview"), "binding_preview", 0, std::numeric_limits<std::size_t>::max());
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

inline json codec<holder::contract::LocationDeleteResult>::encode(const holder::contract::LocationDeleteResult& value) {
    json result = json::object();
    result["location_id"] = codec<std::string>::encode(value.location_id);
    return result;
}
inline holder::contract::LocationDeleteResult codec<holder::contract::LocationDeleteResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a LocationDeleteResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "location_id") throw std::invalid_argument("LocationDeleteResult has no field " + it.key());
    holder::contract::LocationDeleteResult out;
    if (value.contains("location_id") && !value.at("location_id").is_null()) {
        const auto& item = value.at("location_id");
        out.location_id = check_text(decode_field<std::string>(item, "location_id"), "location_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("location_id is required and must not be null");
    }
    return out;
}

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

inline json codec<holder::contract::CardBreadcrumb>::encode(const holder::contract::CardBreadcrumb& value) {
    json result = json::object();
    result["kind"] = codec<holder::contract::CardBreadcrumbKind>::encode(value.kind);
    result["id"] = codec<std::string>::encode(value.id);
    result["title"] = codec<std::string>::encode(value.title);
    return result;
}
inline holder::contract::CardBreadcrumb codec<holder::contract::CardBreadcrumb>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardBreadcrumb object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "kind" && it.key() != "id" && it.key() != "title") throw std::invalid_argument("CardBreadcrumb has no field " + it.key());
    holder::contract::CardBreadcrumb out;
    if (value.contains("kind") && !value.at("kind").is_null()) {
        const auto& item = value.at("kind");
        out.kind = decode_field<holder::contract::CardBreadcrumbKind>(item, "kind");
    } else {
        throw std::invalid_argument("kind is required and must not be null");
    }
    if (value.contains("id") && !value.at("id").is_null()) {
        const auto& item = value.at("id");
        out.id = check_text(decode_field<std::string>(item, "id"), "id", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("id is required and must not be null");
    }
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("title is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardReferenceCandidate>::encode(const holder::contract::CardReferenceCandidate& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["deleted_at"] = value.deleted_at ? codec<long long>::encode(*value.deleted_at) : json(nullptr);
    return result;
}
inline holder::contract::CardReferenceCandidate codec<holder::contract::CardReferenceCandidate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardReferenceCandidate object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "card_id" && it.key() != "title" && it.key() != "deleted_at") throw std::invalid_argument("CardReferenceCandidate has no field " + it.key());
    holder::contract::CardReferenceCandidate out;
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_id is required and must not be null");
    }
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("title is required and must not be null");
    }
    if (value.contains("deleted_at") && !value.at("deleted_at").is_null()) {
        const auto& item = value.at("deleted_at");
        out.deleted_at = decode_field<long long>(item, "deleted_at");
    }
    return out;
}

inline json codec<holder::contract::CardReferenceResolveData>::encode(const holder::contract::CardReferenceResolveData& value) {
    json result = json::object();
    result["status"] = codec<holder::contract::CardReferenceResolveDataStatus>::encode(value.status);
    if (value.match_kind) result["match_kind"] = codec<std::string>::encode(*value.match_kind);
    if (value.card) result["card"] = codec<holder::contract::CardReferenceCandidate>::encode(*value.card);
    if (value.candidates) result["candidates"] = codec<std::vector<holder::contract::CardReferenceCandidate>>::encode(*value.candidates);
    return result;
}
inline holder::contract::CardReferenceResolveData codec<holder::contract::CardReferenceResolveData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardReferenceResolveData object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "status" && it.key() != "match_kind" && it.key() != "card" && it.key() != "candidates") throw std::invalid_argument("CardReferenceResolveData has no field " + it.key());
    holder::contract::CardReferenceResolveData out;
    if (value.contains("status") && !value.at("status").is_null()) {
        const auto& item = value.at("status");
        out.status = decode_field<holder::contract::CardReferenceResolveDataStatus>(item, "status");
    } else {
        throw std::invalid_argument("status is required and must not be null");
    }
    if (value.contains("match_kind") && !value.at("match_kind").is_null()) {
        const auto& item = value.at("match_kind");
        out.match_kind = check_text(decode_field<std::string>(item, "match_kind"), "match_kind", 0, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("match_kind")) {
        throw std::invalid_argument("match_kind must not be null");
    }
    if (value.contains("card") && !value.at("card").is_null()) {
        const auto& item = value.at("card");
        out.card = codec<holder::contract::CardReferenceCandidate>::decode(item);
    } else if (value.contains("card")) {
        throw std::invalid_argument("card must not be null");
    }
    if (value.contains("candidates") && !value.at("candidates").is_null()) {
        const auto& item = value.at("candidates");
        out.candidates = decode_field<std::vector<holder::contract::CardReferenceCandidate>>(item, "candidates");
    } else if (value.contains("candidates")) {
        throw std::invalid_argument("candidates must not be null");
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

inline json codec<holder::contract::EncryptionSafetyCheck>::encode(const holder::contract::EncryptionSafetyCheck& value) {
    json result = json::object();
    result["ok"] = codec<bool>::encode(value.ok);
    result["checked_files"] = codec<std::int32_t>::encode(value.checked_files);
    result["unsafe_files"] = codec<std::int32_t>::encode(value.unsafe_files);
    result["unsafe_paths"] = codec<std::vector<std::string>>::encode(value.unsafe_paths);
    result["message"] = codec<std::string>::encode(value.message);
    return result;
}
inline holder::contract::EncryptionSafetyCheck codec<holder::contract::EncryptionSafetyCheck>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a EncryptionSafetyCheck object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "ok" && it.key() != "checked_files" && it.key() != "unsafe_files" && it.key() != "unsafe_paths" && it.key() != "message") throw std::invalid_argument("EncryptionSafetyCheck has no field " + it.key());
    holder::contract::EncryptionSafetyCheck out;
    if (value.contains("ok") && !value.at("ok").is_null()) {
        const auto& item = value.at("ok");
        out.ok = decode_field<bool>(item, "ok");
    } else {
        throw std::invalid_argument("ok is required and must not be null");
    }
    if (value.contains("checked_files") && !value.at("checked_files").is_null()) {
        const auto& item = value.at("checked_files");
        out.checked_files = check_range<std::int32_t>(decode_field<std::int32_t>(item, "checked_files"), "checked_files", 0, std::numeric_limits<std::int32_t>::max());
    } else {
        throw std::invalid_argument("checked_files is required and must not be null");
    }
    if (value.contains("unsafe_files") && !value.at("unsafe_files").is_null()) {
        const auto& item = value.at("unsafe_files");
        out.unsafe_files = check_range<std::int32_t>(decode_field<std::int32_t>(item, "unsafe_files"), "unsafe_files", 0, std::numeric_limits<std::int32_t>::max());
    } else {
        throw std::invalid_argument("unsafe_files is required and must not be null");
    }
    if (value.contains("unsafe_paths") && !value.at("unsafe_paths").is_null()) {
        const auto& item = value.at("unsafe_paths");
        out.unsafe_paths = [&] { auto items = decode_field<std::vector<std::string>>(item, "unsafe_paths"); for (const auto& text : items) check_text(text, "unsafe_paths", 0, std::numeric_limits<std::size_t>::max()); return items; }();
    } else {
        throw std::invalid_argument("unsafe_paths is required and must not be null");
    }
    if (value.contains("message") && !value.at("message").is_null()) {
        const auto& item = value.at("message");
        out.message = check_text(decode_field<std::string>(item, "message"), "message", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("message is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::ProjectEncryptionCheck>::encode(const holder::contract::ProjectEncryptionCheck& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["privacy_mode"] = codec<holder::contract::ProjectEncryptionCheckPrivacyMode>::encode(value.privacy_mode);
    result["check"] = codec<holder::contract::EncryptionSafetyCheck>::encode(value.check);
    return result;
}
inline holder::contract::ProjectEncryptionCheck codec<holder::contract::ProjectEncryptionCheck>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ProjectEncryptionCheck object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "privacy_mode" && it.key() != "check") throw std::invalid_argument("ProjectEncryptionCheck has no field " + it.key());
    holder::contract::ProjectEncryptionCheck out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("privacy_mode") && !value.at("privacy_mode").is_null()) {
        const auto& item = value.at("privacy_mode");
        out.privacy_mode = decode_field<holder::contract::ProjectEncryptionCheckPrivacyMode>(item, "privacy_mode");
    } else {
        throw std::invalid_argument("privacy_mode is required and must not be null");
    }
    if (value.contains("check") && !value.at("check").is_null()) {
        const auto& item = value.at("check");
        out.check = codec<holder::contract::EncryptionSafetyCheck>::decode(item);
    } else {
        throw std::invalid_argument("check is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::HierarchyRepairMove>::encode(const holder::contract::HierarchyRepairMove& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["parent_card_id"] = value.parent_card_id ? codec<std::string>::encode(*value.parent_card_id) : json(nullptr);
    return result;
}
inline holder::contract::HierarchyRepairMove codec<holder::contract::HierarchyRepairMove>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a HierarchyRepairMove object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "card_id" && it.key() != "parent_card_id") throw std::invalid_argument("HierarchyRepairMove has no field " + it.key());
    holder::contract::HierarchyRepairMove out;
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_id is required and must not be null");
    }
    if (value.contains("parent_card_id") && !value.at("parent_card_id").is_null()) {
        const auto& item = value.at("parent_card_id");
        out.parent_card_id = check_text(decode_field<std::string>(item, "parent_card_id"), "parent_card_id", 1, std::numeric_limits<std::size_t>::max());
    }
    return out;
}

inline json codec<holder::contract::HierarchyRepair>::encode(const holder::contract::HierarchyRepair& value) {
    json result = json::object();
    result["applied"] = codec<bool>::encode(value.applied);
    result["moved"] = codec<std::vector<holder::contract::HierarchyRepairMove>>::encode(value.moved);
    result["changed_card_count"] = codec<std::int32_t>::encode(value.changed_card_count);
    result["cycle_card_ids"] = codec<std::vector<std::string>>::encode(value.cycle_card_ids);
    return result;
}
inline holder::contract::HierarchyRepair codec<holder::contract::HierarchyRepair>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a HierarchyRepair object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "applied" && it.key() != "moved" && it.key() != "changed_card_count" && it.key() != "cycle_card_ids") throw std::invalid_argument("HierarchyRepair has no field " + it.key());
    holder::contract::HierarchyRepair out;
    if (value.contains("applied") && !value.at("applied").is_null()) {
        const auto& item = value.at("applied");
        out.applied = decode_field<bool>(item, "applied");
    } else {
        throw std::invalid_argument("applied is required and must not be null");
    }
    if (value.contains("moved") && !value.at("moved").is_null()) {
        const auto& item = value.at("moved");
        out.moved = decode_field<std::vector<holder::contract::HierarchyRepairMove>>(item, "moved");
    } else {
        throw std::invalid_argument("moved is required and must not be null");
    }
    if (value.contains("changed_card_count") && !value.at("changed_card_count").is_null()) {
        const auto& item = value.at("changed_card_count");
        out.changed_card_count = check_range<std::int32_t>(decode_field<std::int32_t>(item, "changed_card_count"), "changed_card_count", 0, std::numeric_limits<std::int32_t>::max());
    } else {
        throw std::invalid_argument("changed_card_count is required and must not be null");
    }
    if (value.contains("cycle_card_ids") && !value.at("cycle_card_ids").is_null()) {
        const auto& item = value.at("cycle_card_ids");
        out.cycle_card_ids = [&] { auto items = decode_field<std::vector<std::string>>(item, "cycle_card_ids"); for (const auto& text : items) check_text(text, "cycle_card_ids", 0, std::numeric_limits<std::size_t>::max()); return items; }();
    } else {
        throw std::invalid_argument("cycle_card_ids is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::ProjectRecoveryTokenExportResult>::encode(const holder::contract::ProjectRecoveryTokenExportResult& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["key_id"] = codec<std::string>::encode(value.key_id);
    result["recovery_token"] = codec<std::string>::encode(value.recovery_token);
    return result;
}
inline holder::contract::ProjectRecoveryTokenExportResult codec<holder::contract::ProjectRecoveryTokenExportResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ProjectRecoveryTokenExportResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "key_id" && it.key() != "recovery_token") throw std::invalid_argument("ProjectRecoveryTokenExportResult has no field " + it.key());
    holder::contract::ProjectRecoveryTokenExportResult out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("key_id") && !value.at("key_id").is_null()) {
        const auto& item = value.at("key_id");
        out.key_id = check_text(decode_field<std::string>(item, "key_id"), "key_id", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("key_id is required and must not be null");
    }
    if (value.contains("recovery_token") && !value.at("recovery_token").is_null()) {
        const auto& item = value.at("recovery_token");
        out.recovery_token = check_text(decode_field<std::string>(item, "recovery_token"), "recovery_token", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("recovery_token is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::ProjectRecoveryTokenImportResult>::encode(const holder::contract::ProjectRecoveryTokenImportResult& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    return result;
}
inline holder::contract::ProjectRecoveryTokenImportResult codec<holder::contract::ProjectRecoveryTokenImportResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ProjectRecoveryTokenImportResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id") throw std::invalid_argument("ProjectRecoveryTokenImportResult has no field " + it.key());
    holder::contract::ProjectRecoveryTokenImportResult out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::RecoveryTokenGlobalImportResult>::encode(const holder::contract::RecoveryTokenGlobalImportResult& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["project_created"] = codec<bool>::encode(value.project_created);
    result["remote_hint_present"] = codec<bool>::encode(value.remote_hint_present);
    result["remote_configured"] = codec<bool>::encode(value.remote_configured);
    result["remote_error"] = value.remote_error ? codec<std::string>::encode(*value.remote_error) : json(nullptr);
    result["pull_status"] = codec<holder::contract::RecoveryTokenGlobalImportResultPullStatus>::encode(value.pull_status);
    result["pull_error"] = value.pull_error ? codec<std::string>::encode(*value.pull_error) : json(nullptr);
    return result;
}
inline holder::contract::RecoveryTokenGlobalImportResult codec<holder::contract::RecoveryTokenGlobalImportResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a RecoveryTokenGlobalImportResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "project_created" && it.key() != "remote_hint_present" && it.key() != "remote_configured" && it.key() != "remote_error" && it.key() != "pull_status" && it.key() != "pull_error") throw std::invalid_argument("RecoveryTokenGlobalImportResult has no field " + it.key());
    holder::contract::RecoveryTokenGlobalImportResult out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("project_created") && !value.at("project_created").is_null()) {
        const auto& item = value.at("project_created");
        out.project_created = decode_field<bool>(item, "project_created");
    } else {
        throw std::invalid_argument("project_created is required and must not be null");
    }
    if (value.contains("remote_hint_present") && !value.at("remote_hint_present").is_null()) {
        const auto& item = value.at("remote_hint_present");
        out.remote_hint_present = decode_field<bool>(item, "remote_hint_present");
    } else {
        throw std::invalid_argument("remote_hint_present is required and must not be null");
    }
    if (value.contains("remote_configured") && !value.at("remote_configured").is_null()) {
        const auto& item = value.at("remote_configured");
        out.remote_configured = decode_field<bool>(item, "remote_configured");
    } else {
        throw std::invalid_argument("remote_configured is required and must not be null");
    }
    if (value.contains("remote_error") && !value.at("remote_error").is_null()) {
        const auto& item = value.at("remote_error");
        out.remote_error = check_text(decode_field<std::string>(item, "remote_error"), "remote_error", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("pull_status") && !value.at("pull_status").is_null()) {
        const auto& item = value.at("pull_status");
        out.pull_status = decode_field<holder::contract::RecoveryTokenGlobalImportResultPullStatus>(item, "pull_status");
    } else {
        throw std::invalid_argument("pull_status is required and must not be null");
    }
    if (value.contains("pull_error") && !value.at("pull_error").is_null()) {
        const auto& item = value.at("pull_error");
        out.pull_error = check_text(decode_field<std::string>(item, "pull_error"), "pull_error", 0, std::numeric_limits<std::size_t>::max());
    }
    return out;
}

inline json codec<holder::contract::RebuildResult>::encode(const holder::contract::RebuildResult& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["cards"] = codec<std::int32_t>::encode(value.cards);
    result["ai_messages"] = codec<std::int32_t>::encode(value.ai_messages);
    result["ai_threads"] = codec<std::int32_t>::encode(value.ai_threads);
    result["links"] = codec<std::int32_t>::encode(value.links);
    return result;
}
inline holder::contract::RebuildResult codec<holder::contract::RebuildResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a RebuildResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "cards" && it.key() != "ai_messages" && it.key() != "ai_threads" && it.key() != "links") throw std::invalid_argument("RebuildResult has no field " + it.key());
    holder::contract::RebuildResult out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("cards") && !value.at("cards").is_null()) {
        const auto& item = value.at("cards");
        out.cards = check_range<std::int32_t>(decode_field<std::int32_t>(item, "cards"), "cards", 0, std::numeric_limits<std::int32_t>::max());
    } else {
        throw std::invalid_argument("cards is required and must not be null");
    }
    if (value.contains("ai_messages") && !value.at("ai_messages").is_null()) {
        const auto& item = value.at("ai_messages");
        out.ai_messages = check_range<std::int32_t>(decode_field<std::int32_t>(item, "ai_messages"), "ai_messages", 0, std::numeric_limits<std::int32_t>::max());
    } else {
        throw std::invalid_argument("ai_messages is required and must not be null");
    }
    if (value.contains("ai_threads") && !value.at("ai_threads").is_null()) {
        const auto& item = value.at("ai_threads");
        out.ai_threads = check_range<std::int32_t>(decode_field<std::int32_t>(item, "ai_threads"), "ai_threads", 0, std::numeric_limits<std::int32_t>::max());
    } else {
        throw std::invalid_argument("ai_threads is required and must not be null");
    }
    if (value.contains("links") && !value.at("links").is_null()) {
        const auto& item = value.at("links");
        out.links = check_range<std::int32_t>(decode_field<std::int32_t>(item, "links"), "links", 0, std::numeric_limits<std::int32_t>::max());
    } else {
        throw std::invalid_argument("links is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::ReindexResult>::encode(const holder::contract::ReindexResult& value) {
    json result = json::object();
    result["message"] = codec<std::string>::encode(value.message);
    return result;
}
inline holder::contract::ReindexResult codec<holder::contract::ReindexResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ReindexResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "message") throw std::invalid_argument("ReindexResult has no field " + it.key());
    holder::contract::ReindexResult out;
    if (value.contains("message") && !value.at("message").is_null()) {
        const auto& item = value.at("message");
        out.message = check_text(decode_field<std::string>(item, "message"), "message", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("message is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::SyncState>::encode(const holder::contract::SyncState& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["last_commit_at"] = value.last_commit_at ? codec<long long>::encode(*value.last_commit_at) : json(nullptr);
    result["last_push_at"] = value.last_push_at ? codec<long long>::encode(*value.last_push_at) : json(nullptr);
    result["last_pull_at"] = value.last_pull_at ? codec<long long>::encode(*value.last_pull_at) : json(nullptr);
    result["uncommitted_changes_count"] = codec<std::int32_t>::encode(value.uncommitted_changes_count);
    result["unpushed_commits_count"] = codec<std::int32_t>::encode(value.unpushed_commits_count);
    result["last_push_status"] = value.last_push_status ? codec<std::string>::encode(*value.last_push_status) : json(nullptr);
    result["last_pull_status"] = value.last_pull_status ? codec<std::string>::encode(*value.last_pull_status) : json(nullptr);
    result["last_sync_error"] = value.last_sync_error ? codec<std::string>::encode(*value.last_sync_error) : json(nullptr);
    result["last_sync_error_at"] = value.last_sync_error_at ? codec<long long>::encode(*value.last_sync_error_at) : json(nullptr);
    result["retry_count"] = codec<std::int32_t>::encode(value.retry_count);
    result["next_retry_at"] = value.next_retry_at ? codec<long long>::encode(*value.next_retry_at) : json(nullptr);
    result["pull_retry_count"] = codec<std::int32_t>::encode(value.pull_retry_count);
    result["next_pull_retry_at"] = value.next_pull_retry_at ? codec<long long>::encode(*value.next_pull_retry_at) : json(nullptr);
    result["updated_at"] = value.updated_at ? codec<long long>::encode(*value.updated_at) : json(nullptr);
    return result;
}
inline holder::contract::SyncState codec<holder::contract::SyncState>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a SyncState object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "last_commit_at" && it.key() != "last_push_at" && it.key() != "last_pull_at" && it.key() != "uncommitted_changes_count" && it.key() != "unpushed_commits_count" && it.key() != "last_push_status" && it.key() != "last_pull_status" && it.key() != "last_sync_error" && it.key() != "last_sync_error_at" && it.key() != "retry_count" && it.key() != "next_retry_at" && it.key() != "pull_retry_count" && it.key() != "next_pull_retry_at" && it.key() != "updated_at") throw std::invalid_argument("SyncState has no field " + it.key());
    holder::contract::SyncState out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("last_commit_at") && !value.at("last_commit_at").is_null()) {
        const auto& item = value.at("last_commit_at");
        out.last_commit_at = decode_field<long long>(item, "last_commit_at");
    }
    if (value.contains("last_push_at") && !value.at("last_push_at").is_null()) {
        const auto& item = value.at("last_push_at");
        out.last_push_at = decode_field<long long>(item, "last_push_at");
    }
    if (value.contains("last_pull_at") && !value.at("last_pull_at").is_null()) {
        const auto& item = value.at("last_pull_at");
        out.last_pull_at = decode_field<long long>(item, "last_pull_at");
    }
    if (value.contains("uncommitted_changes_count") && !value.at("uncommitted_changes_count").is_null()) {
        const auto& item = value.at("uncommitted_changes_count");
        out.uncommitted_changes_count = decode_field<std::int32_t>(item, "uncommitted_changes_count");
    } else {
        throw std::invalid_argument("uncommitted_changes_count is required and must not be null");
    }
    if (value.contains("unpushed_commits_count") && !value.at("unpushed_commits_count").is_null()) {
        const auto& item = value.at("unpushed_commits_count");
        out.unpushed_commits_count = decode_field<std::int32_t>(item, "unpushed_commits_count");
    } else {
        throw std::invalid_argument("unpushed_commits_count is required and must not be null");
    }
    if (value.contains("last_push_status") && !value.at("last_push_status").is_null()) {
        const auto& item = value.at("last_push_status");
        out.last_push_status = check_text(decode_field<std::string>(item, "last_push_status"), "last_push_status", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("last_pull_status") && !value.at("last_pull_status").is_null()) {
        const auto& item = value.at("last_pull_status");
        out.last_pull_status = check_text(decode_field<std::string>(item, "last_pull_status"), "last_pull_status", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("last_sync_error") && !value.at("last_sync_error").is_null()) {
        const auto& item = value.at("last_sync_error");
        out.last_sync_error = check_text(decode_field<std::string>(item, "last_sync_error"), "last_sync_error", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("last_sync_error_at") && !value.at("last_sync_error_at").is_null()) {
        const auto& item = value.at("last_sync_error_at");
        out.last_sync_error_at = decode_field<long long>(item, "last_sync_error_at");
    }
    if (value.contains("retry_count") && !value.at("retry_count").is_null()) {
        const auto& item = value.at("retry_count");
        out.retry_count = decode_field<std::int32_t>(item, "retry_count");
    } else {
        throw std::invalid_argument("retry_count is required and must not be null");
    }
    if (value.contains("next_retry_at") && !value.at("next_retry_at").is_null()) {
        const auto& item = value.at("next_retry_at");
        out.next_retry_at = decode_field<long long>(item, "next_retry_at");
    }
    if (value.contains("pull_retry_count") && !value.at("pull_retry_count").is_null()) {
        const auto& item = value.at("pull_retry_count");
        out.pull_retry_count = decode_field<std::int32_t>(item, "pull_retry_count");
    } else {
        throw std::invalid_argument("pull_retry_count is required and must not be null");
    }
    if (value.contains("next_pull_retry_at") && !value.at("next_pull_retry_at").is_null()) {
        const auto& item = value.at("next_pull_retry_at");
        out.next_pull_retry_at = decode_field<long long>(item, "next_pull_retry_at");
    }
    if (value.contains("updated_at") && !value.at("updated_at").is_null()) {
        const auto& item = value.at("updated_at");
        out.updated_at = decode_field<long long>(item, "updated_at");
    }
    return out;
}

inline json codec<holder::contract::PullPhase>::encode(const holder::contract::PullPhase& value) {
    json result = json::object();
    result["attempted"] = codec<bool>::encode(value.attempted);
    result["status"] = codec<holder::contract::PullPhaseStatus>::encode(value.status);
    result["conflicts_resolved"] = codec<std::int32_t>::encode(value.conflicts_resolved);
    result["hidden_cards_repaired"] = codec<std::int32_t>::encode(value.hidden_cards_repaired);
    result["error_message"] = value.error_message ? codec<std::string>::encode(*value.error_message) : json(nullptr);
    return result;
}
inline holder::contract::PullPhase codec<holder::contract::PullPhase>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a PullPhase object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "attempted" && it.key() != "status" && it.key() != "conflicts_resolved" && it.key() != "hidden_cards_repaired" && it.key() != "error_message") throw std::invalid_argument("PullPhase has no field " + it.key());
    holder::contract::PullPhase out;
    if (value.contains("attempted") && !value.at("attempted").is_null()) {
        const auto& item = value.at("attempted");
        out.attempted = decode_field<bool>(item, "attempted");
    } else {
        throw std::invalid_argument("attempted is required and must not be null");
    }
    if (value.contains("status") && !value.at("status").is_null()) {
        const auto& item = value.at("status");
        out.status = decode_field<holder::contract::PullPhaseStatus>(item, "status");
    } else {
        throw std::invalid_argument("status is required and must not be null");
    }
    if (value.contains("conflicts_resolved") && !value.at("conflicts_resolved").is_null()) {
        const auto& item = value.at("conflicts_resolved");
        out.conflicts_resolved = decode_field<std::int32_t>(item, "conflicts_resolved");
    } else {
        throw std::invalid_argument("conflicts_resolved is required and must not be null");
    }
    if (value.contains("hidden_cards_repaired") && !value.at("hidden_cards_repaired").is_null()) {
        const auto& item = value.at("hidden_cards_repaired");
        out.hidden_cards_repaired = decode_field<std::int32_t>(item, "hidden_cards_repaired");
    } else {
        throw std::invalid_argument("hidden_cards_repaired is required and must not be null");
    }
    if (value.contains("error_message") && !value.at("error_message").is_null()) {
        const auto& item = value.at("error_message");
        out.error_message = check_text(decode_field<std::string>(item, "error_message"), "error_message", 0, std::numeric_limits<std::size_t>::max());
    }
    return out;
}

inline json codec<holder::contract::PushPhase>::encode(const holder::contract::PushPhase& value) {
    json result = json::object();
    result["attempted"] = codec<bool>::encode(value.attempted);
    result["status"] = codec<holder::contract::PushPhaseStatus>::encode(value.status);
    result["ahead_count"] = codec<std::int32_t>::encode(value.ahead_count);
    result["behind_count"] = codec<std::int32_t>::encode(value.behind_count);
    result["local_head_commit"] = value.local_head_commit ? codec<std::string>::encode(*value.local_head_commit) : json(nullptr);
    result["error_message"] = value.error_message ? codec<std::string>::encode(*value.error_message) : json(nullptr);
    return result;
}
inline holder::contract::PushPhase codec<holder::contract::PushPhase>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a PushPhase object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "attempted" && it.key() != "status" && it.key() != "ahead_count" && it.key() != "behind_count" && it.key() != "local_head_commit" && it.key() != "error_message") throw std::invalid_argument("PushPhase has no field " + it.key());
    holder::contract::PushPhase out;
    if (value.contains("attempted") && !value.at("attempted").is_null()) {
        const auto& item = value.at("attempted");
        out.attempted = decode_field<bool>(item, "attempted");
    } else {
        throw std::invalid_argument("attempted is required and must not be null");
    }
    if (value.contains("status") && !value.at("status").is_null()) {
        const auto& item = value.at("status");
        out.status = decode_field<holder::contract::PushPhaseStatus>(item, "status");
    } else {
        throw std::invalid_argument("status is required and must not be null");
    }
    if (value.contains("ahead_count") && !value.at("ahead_count").is_null()) {
        const auto& item = value.at("ahead_count");
        out.ahead_count = decode_field<std::int32_t>(item, "ahead_count");
    } else {
        throw std::invalid_argument("ahead_count is required and must not be null");
    }
    if (value.contains("behind_count") && !value.at("behind_count").is_null()) {
        const auto& item = value.at("behind_count");
        out.behind_count = decode_field<std::int32_t>(item, "behind_count");
    } else {
        throw std::invalid_argument("behind_count is required and must not be null");
    }
    if (value.contains("local_head_commit") && !value.at("local_head_commit").is_null()) {
        const auto& item = value.at("local_head_commit");
        out.local_head_commit = check_text(decode_field<std::string>(item, "local_head_commit"), "local_head_commit", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("error_message") && !value.at("error_message").is_null()) {
        const auto& item = value.at("error_message");
        out.error_message = check_text(decode_field<std::string>(item, "error_message"), "error_message", 0, std::numeric_limits<std::size_t>::max());
    }
    return out;
}

inline json codec<holder::contract::SyncResult>::encode(const holder::contract::SyncResult& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["status"] = codec<holder::contract::SyncResultStatus>::encode(value.status);
    result["remote_url"] = value.remote_url ? codec<std::string>::encode(*value.remote_url) : json(nullptr);
    result["pull"] = codec<holder::contract::PullPhase>::encode(value.pull);
    result["push"] = codec<holder::contract::PushPhase>::encode(value.push);
    return result;
}
inline holder::contract::SyncResult codec<holder::contract::SyncResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a SyncResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "status" && it.key() != "remote_url" && it.key() != "pull" && it.key() != "push") throw std::invalid_argument("SyncResult has no field " + it.key());
    holder::contract::SyncResult out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("status") && !value.at("status").is_null()) {
        const auto& item = value.at("status");
        out.status = decode_field<holder::contract::SyncResultStatus>(item, "status");
    } else {
        throw std::invalid_argument("status is required and must not be null");
    }
    if (value.contains("remote_url") && !value.at("remote_url").is_null()) {
        const auto& item = value.at("remote_url");
        out.remote_url = check_text(decode_field<std::string>(item, "remote_url"), "remote_url", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("pull") && !value.at("pull").is_null()) {
        const auto& item = value.at("pull");
        out.pull = codec<holder::contract::PullPhase>::decode(item);
    } else {
        throw std::invalid_argument("pull is required and must not be null");
    }
    if (value.contains("push") && !value.at("push").is_null()) {
        const auto& item = value.at("push");
        out.push = codec<holder::contract::PushPhase>::decode(item);
    } else {
        throw std::invalid_argument("push is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::DueSyncResult>::encode(const holder::contract::DueSyncResult& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["remote_url"] = value.remote_url ? codec<std::string>::encode(*value.remote_url) : json(nullptr);
    result["pull"] = codec<holder::contract::PullPhase>::encode(value.pull);
    result["push"] = codec<holder::contract::PushPhase>::encode(value.push);
    return result;
}
inline holder::contract::DueSyncResult codec<holder::contract::DueSyncResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a DueSyncResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "remote_url" && it.key() != "pull" && it.key() != "push") throw std::invalid_argument("DueSyncResult has no field " + it.key());
    holder::contract::DueSyncResult out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("remote_url") && !value.at("remote_url").is_null()) {
        const auto& item = value.at("remote_url");
        out.remote_url = check_text(decode_field<std::string>(item, "remote_url"), "remote_url", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("pull") && !value.at("pull").is_null()) {
        const auto& item = value.at("pull");
        out.pull = codec<holder::contract::PullPhase>::decode(item);
    } else {
        throw std::invalid_argument("pull is required and must not be null");
    }
    if (value.contains("push") && !value.at("push").is_null()) {
        const auto& item = value.at("push");
        out.push = codec<holder::contract::PushPhase>::decode(item);
    } else {
        throw std::invalid_argument("push is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::RemoteTestResult>::encode(const holder::contract::RemoteTestResult& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["status"] = codec<holder::contract::RemoteTestResultStatus>::encode(value.status);
    result["remote_url"] = value.remote_url ? codec<std::string>::encode(*value.remote_url) : json(nullptr);
    result["remote_has_head"] = codec<bool>::encode(value.remote_has_head);
    result["error_message"] = value.error_message ? codec<std::string>::encode(*value.error_message) : json(nullptr);
    return result;
}
inline holder::contract::RemoteTestResult codec<holder::contract::RemoteTestResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a RemoteTestResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "status" && it.key() != "remote_url" && it.key() != "remote_has_head" && it.key() != "error_message") throw std::invalid_argument("RemoteTestResult has no field " + it.key());
    holder::contract::RemoteTestResult out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("status") && !value.at("status").is_null()) {
        const auto& item = value.at("status");
        out.status = decode_field<holder::contract::RemoteTestResultStatus>(item, "status");
    } else {
        throw std::invalid_argument("status is required and must not be null");
    }
    if (value.contains("remote_url") && !value.at("remote_url").is_null()) {
        const auto& item = value.at("remote_url");
        out.remote_url = check_text(decode_field<std::string>(item, "remote_url"), "remote_url", 1, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("remote_has_head") && !value.at("remote_has_head").is_null()) {
        const auto& item = value.at("remote_has_head");
        out.remote_has_head = decode_field<bool>(item, "remote_has_head");
    } else {
        throw std::invalid_argument("remote_has_head is required and must not be null");
    }
    if (value.contains("error_message") && !value.at("error_message").is_null()) {
        const auto& item = value.at("error_message");
        out.error_message = check_text(decode_field<std::string>(item, "error_message"), "error_message", 0, std::numeric_limits<std::size_t>::max());
    }
    return out;
}

inline json codec<holder::contract::HistoryAuthor>::encode(const holder::contract::HistoryAuthor& value) {
    json result = json::object();
    result["name"] = codec<std::string>::encode(value.name);
    result["email"] = codec<std::string>::encode(value.email);
    return result;
}
inline holder::contract::HistoryAuthor codec<holder::contract::HistoryAuthor>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a HistoryAuthor object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "name" && it.key() != "email") throw std::invalid_argument("HistoryAuthor has no field " + it.key());
    holder::contract::HistoryAuthor out;
    if (value.contains("name") && !value.at("name").is_null()) {
        const auto& item = value.at("name");
        out.name = check_text(decode_field<std::string>(item, "name"), "name", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("name is required and must not be null");
    }
    if (value.contains("email") && !value.at("email").is_null()) {
        const auto& item = value.at("email");
        out.email = check_text(decode_field<std::string>(item, "email"), "email", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("email is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::HistoryAffectedPath>::encode(const holder::contract::HistoryAffectedPath& value) {
    json result = json::object();
    result["path"] = codec<std::string>::encode(value.path);
    if (value.title) result["title"] = codec<std::string>::encode(*value.title);
    if (value.detail) result["detail"] = codec<std::string>::encode(*value.detail);
    return result;
}
inline holder::contract::HistoryAffectedPath codec<holder::contract::HistoryAffectedPath>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a HistoryAffectedPath object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "path" && it.key() != "title" && it.key() != "detail") throw std::invalid_argument("HistoryAffectedPath has no field " + it.key());
    holder::contract::HistoryAffectedPath out;
    if (value.contains("path") && !value.at("path").is_null()) {
        const auto& item = value.at("path");
        out.path = check_text(decode_field<std::string>(item, "path"), "path", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("path is required and must not be null");
    }
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 0, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("title")) {
        throw std::invalid_argument("title must not be null");
    }
    if (value.contains("detail") && !value.at("detail").is_null()) {
        const auto& item = value.at("detail");
        out.detail = check_text(decode_field<std::string>(item, "detail"), "detail", 0, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("detail")) {
        throw std::invalid_argument("detail must not be null");
    }
    return out;
}

inline json codec<holder::contract::HistoryAffectedObject>::encode(const holder::contract::HistoryAffectedObject& value) {
    json result = json::object();
    result["kind"] = codec<holder::contract::HistoryAffectedObjectKind>::encode(value.kind);
    result["paths"] = codec<std::vector<std::string>>::encode(value.paths);
    result["items"] = codec<std::vector<holder::contract::HistoryAffectedPath>>::encode(value.items);
    return result;
}
inline holder::contract::HistoryAffectedObject codec<holder::contract::HistoryAffectedObject>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a HistoryAffectedObject object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "kind" && it.key() != "paths" && it.key() != "items") throw std::invalid_argument("HistoryAffectedObject has no field " + it.key());
    holder::contract::HistoryAffectedObject out;
    if (value.contains("kind") && !value.at("kind").is_null()) {
        const auto& item = value.at("kind");
        out.kind = decode_field<holder::contract::HistoryAffectedObjectKind>(item, "kind");
    } else {
        throw std::invalid_argument("kind is required and must not be null");
    }
    if (value.contains("paths") && !value.at("paths").is_null()) {
        const auto& item = value.at("paths");
        out.paths = [&] { auto items = decode_field<std::vector<std::string>>(item, "paths"); for (const auto& text : items) check_text(text, "paths", 0, std::numeric_limits<std::size_t>::max()); return items; }();
    } else {
        throw std::invalid_argument("paths is required and must not be null");
    }
    if (value.contains("items") && !value.at("items").is_null()) {
        const auto& item = value.at("items");
        out.items = decode_field<std::vector<holder::contract::HistoryAffectedPath>>(item, "items");
    } else {
        throw std::invalid_argument("items is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::ProjectActivity>::encode(const holder::contract::ProjectActivity& value) {
    json result = json::object();
    result["oid"] = codec<std::string>::encode(value.oid);
    result["parent_oids"] = codec<std::vector<std::string>>::encode(value.parent_oids);
    result["author"] = codec<holder::contract::HistoryAuthor>::encode(value.author);
    result["authored_at"] = codec<long long>::encode(value.authored_at);
    result["committed_at"] = codec<long long>::encode(value.committed_at);
    result["message"] = codec<std::string>::encode(value.message);
    result["affected_objects"] = codec<std::vector<holder::contract::HistoryAffectedObject>>::encode(value.affected_objects);
    result["is_merge"] = codec<bool>::encode(value.is_merge);
    return result;
}
inline holder::contract::ProjectActivity codec<holder::contract::ProjectActivity>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ProjectActivity object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "oid" && it.key() != "parent_oids" && it.key() != "author" && it.key() != "authored_at" && it.key() != "committed_at" && it.key() != "message" && it.key() != "affected_objects" && it.key() != "is_merge") throw std::invalid_argument("ProjectActivity has no field " + it.key());
    holder::contract::ProjectActivity out;
    if (value.contains("oid") && !value.at("oid").is_null()) {
        const auto& item = value.at("oid");
        out.oid = check_text(decode_field<std::string>(item, "oid"), "oid", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("oid is required and must not be null");
    }
    if (value.contains("parent_oids") && !value.at("parent_oids").is_null()) {
        const auto& item = value.at("parent_oids");
        out.parent_oids = [&] { auto items = decode_field<std::vector<std::string>>(item, "parent_oids"); for (const auto& text : items) check_text(text, "parent_oids", 0, std::numeric_limits<std::size_t>::max()); return items; }();
    } else {
        throw std::invalid_argument("parent_oids is required and must not be null");
    }
    if (value.contains("author") && !value.at("author").is_null()) {
        const auto& item = value.at("author");
        out.author = codec<holder::contract::HistoryAuthor>::decode(item);
    } else {
        throw std::invalid_argument("author is required and must not be null");
    }
    if (value.contains("authored_at") && !value.at("authored_at").is_null()) {
        const auto& item = value.at("authored_at");
        out.authored_at = decode_field<long long>(item, "authored_at");
    } else {
        throw std::invalid_argument("authored_at is required and must not be null");
    }
    if (value.contains("committed_at") && !value.at("committed_at").is_null()) {
        const auto& item = value.at("committed_at");
        out.committed_at = decode_field<long long>(item, "committed_at");
    } else {
        throw std::invalid_argument("committed_at is required and must not be null");
    }
    if (value.contains("message") && !value.at("message").is_null()) {
        const auto& item = value.at("message");
        out.message = check_text(decode_field<std::string>(item, "message"), "message", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("message is required and must not be null");
    }
    if (value.contains("affected_objects") && !value.at("affected_objects").is_null()) {
        const auto& item = value.at("affected_objects");
        out.affected_objects = decode_field<std::vector<holder::contract::HistoryAffectedObject>>(item, "affected_objects");
    } else {
        throw std::invalid_argument("affected_objects is required and must not be null");
    }
    if (value.contains("is_merge") && !value.at("is_merge").is_null()) {
        const auto& item = value.at("is_merge");
        out.is_merge = decode_field<bool>(item, "is_merge");
    } else {
        throw std::invalid_argument("is_merge is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::ProjectHistoryPage>::encode(const holder::contract::ProjectHistoryPage& value) {
    json result = json::object();
    result["head_oid"] = value.head_oid ? codec<std::string>::encode(*value.head_oid) : json(nullptr);
    result["activities"] = codec<std::vector<holder::contract::ProjectActivity>>::encode(value.activities);
    result["next_cursor"] = value.next_cursor ? codec<std::string>::encode(*value.next_cursor) : json(nullptr);
    result["scan_limited"] = codec<bool>::encode(value.scan_limited);
    return result;
}
inline holder::contract::ProjectHistoryPage codec<holder::contract::ProjectHistoryPage>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a ProjectHistoryPage object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "head_oid" && it.key() != "activities" && it.key() != "next_cursor" && it.key() != "scan_limited") throw std::invalid_argument("ProjectHistoryPage has no field " + it.key());
    holder::contract::ProjectHistoryPage out;
    if (value.contains("head_oid") && !value.at("head_oid").is_null()) {
        const auto& item = value.at("head_oid");
        out.head_oid = check_text(decode_field<std::string>(item, "head_oid"), "head_oid", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("activities") && !value.at("activities").is_null()) {
        const auto& item = value.at("activities");
        out.activities = decode_field<std::vector<holder::contract::ProjectActivity>>(item, "activities");
    } else {
        throw std::invalid_argument("activities is required and must not be null");
    }
    if (value.contains("next_cursor") && !value.at("next_cursor").is_null()) {
        const auto& item = value.at("next_cursor");
        out.next_cursor = check_text(decode_field<std::string>(item, "next_cursor"), "next_cursor", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("scan_limited") && !value.at("scan_limited").is_null()) {
        const auto& item = value.at("scan_limited");
        out.scan_limited = decode_field<bool>(item, "scan_limited");
    } else {
        throw std::invalid_argument("scan_limited is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardHistorySave>::encode(const holder::contract::CardHistorySave& value) {
    json result = json::object();
    result["oid"] = codec<std::string>::encode(value.oid);
    result["parent_oids"] = codec<std::vector<std::string>>::encode(value.parent_oids);
    result["authored_at"] = codec<long long>::encode(value.authored_at);
    result["committed_at"] = codec<long long>::encode(value.committed_at);
    result["message"] = codec<std::string>::encode(value.message);
    return result;
}
inline holder::contract::CardHistorySave codec<holder::contract::CardHistorySave>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardHistorySave object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "oid" && it.key() != "parent_oids" && it.key() != "authored_at" && it.key() != "committed_at" && it.key() != "message") throw std::invalid_argument("CardHistorySave has no field " + it.key());
    holder::contract::CardHistorySave out;
    if (value.contains("oid") && !value.at("oid").is_null()) {
        const auto& item = value.at("oid");
        out.oid = check_text(decode_field<std::string>(item, "oid"), "oid", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("oid is required and must not be null");
    }
    if (value.contains("parent_oids") && !value.at("parent_oids").is_null()) {
        const auto& item = value.at("parent_oids");
        out.parent_oids = [&] { auto items = decode_field<std::vector<std::string>>(item, "parent_oids"); for (const auto& text : items) check_text(text, "parent_oids", 0, std::numeric_limits<std::size_t>::max()); return items; }();
    } else {
        throw std::invalid_argument("parent_oids is required and must not be null");
    }
    if (value.contains("authored_at") && !value.at("authored_at").is_null()) {
        const auto& item = value.at("authored_at");
        out.authored_at = decode_field<long long>(item, "authored_at");
    } else {
        throw std::invalid_argument("authored_at is required and must not be null");
    }
    if (value.contains("committed_at") && !value.at("committed_at").is_null()) {
        const auto& item = value.at("committed_at");
        out.committed_at = decode_field<long long>(item, "committed_at");
    } else {
        throw std::invalid_argument("committed_at is required and must not be null");
    }
    if (value.contains("message") && !value.at("message").is_null()) {
        const auto& item = value.at("message");
        out.message = check_text(decode_field<std::string>(item, "message"), "message", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("message is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardHistoryEntry>::encode(const holder::contract::CardHistoryEntry& value) {
    json result = json::object();
    result["first_oid"] = codec<std::string>::encode(value.first_oid);
    result["last_oid"] = codec<std::string>::encode(value.last_oid);
    result["parent_oids"] = codec<std::vector<std::string>>::encode(value.parent_oids);
    result["visible_parent_oids"] = codec<std::vector<std::string>>::encode(value.visible_parent_oids);
    result["author"] = codec<holder::contract::HistoryAuthor>::encode(value.author);
    result["started_at"] = codec<long long>::encode(value.started_at);
    result["ended_at"] = codec<long long>::encode(value.ended_at);
    result["kind"] = codec<std::string>::encode(value.kind);
    result["summary"] = codec<std::string>::encode(value.summary);
    result["commit_count"] = codec<std::int32_t>::encode(value.commit_count);
    result["is_merge"] = codec<bool>::encode(value.is_merge);
    result["saves"] = codec<std::vector<holder::contract::CardHistorySave>>::encode(value.saves);
    return result;
}
inline holder::contract::CardHistoryEntry codec<holder::contract::CardHistoryEntry>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardHistoryEntry object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "first_oid" && it.key() != "last_oid" && it.key() != "parent_oids" && it.key() != "visible_parent_oids" && it.key() != "author" && it.key() != "started_at" && it.key() != "ended_at" && it.key() != "kind" && it.key() != "summary" && it.key() != "commit_count" && it.key() != "is_merge" && it.key() != "saves") throw std::invalid_argument("CardHistoryEntry has no field " + it.key());
    holder::contract::CardHistoryEntry out;
    if (value.contains("first_oid") && !value.at("first_oid").is_null()) {
        const auto& item = value.at("first_oid");
        out.first_oid = check_text(decode_field<std::string>(item, "first_oid"), "first_oid", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("first_oid is required and must not be null");
    }
    if (value.contains("last_oid") && !value.at("last_oid").is_null()) {
        const auto& item = value.at("last_oid");
        out.last_oid = check_text(decode_field<std::string>(item, "last_oid"), "last_oid", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("last_oid is required and must not be null");
    }
    if (value.contains("parent_oids") && !value.at("parent_oids").is_null()) {
        const auto& item = value.at("parent_oids");
        out.parent_oids = [&] { auto items = decode_field<std::vector<std::string>>(item, "parent_oids"); for (const auto& text : items) check_text(text, "parent_oids", 0, std::numeric_limits<std::size_t>::max()); return items; }();
    } else {
        throw std::invalid_argument("parent_oids is required and must not be null");
    }
    if (value.contains("visible_parent_oids") && !value.at("visible_parent_oids").is_null()) {
        const auto& item = value.at("visible_parent_oids");
        out.visible_parent_oids = [&] { auto items = decode_field<std::vector<std::string>>(item, "visible_parent_oids"); for (const auto& text : items) check_text(text, "visible_parent_oids", 0, std::numeric_limits<std::size_t>::max()); return items; }();
    } else {
        throw std::invalid_argument("visible_parent_oids is required and must not be null");
    }
    if (value.contains("author") && !value.at("author").is_null()) {
        const auto& item = value.at("author");
        out.author = codec<holder::contract::HistoryAuthor>::decode(item);
    } else {
        throw std::invalid_argument("author is required and must not be null");
    }
    if (value.contains("started_at") && !value.at("started_at").is_null()) {
        const auto& item = value.at("started_at");
        out.started_at = decode_field<long long>(item, "started_at");
    } else {
        throw std::invalid_argument("started_at is required and must not be null");
    }
    if (value.contains("ended_at") && !value.at("ended_at").is_null()) {
        const auto& item = value.at("ended_at");
        out.ended_at = decode_field<long long>(item, "ended_at");
    } else {
        throw std::invalid_argument("ended_at is required and must not be null");
    }
    if (value.contains("kind") && !value.at("kind").is_null()) {
        const auto& item = value.at("kind");
        out.kind = check_text(decode_field<std::string>(item, "kind"), "kind", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("kind is required and must not be null");
    }
    if (value.contains("summary") && !value.at("summary").is_null()) {
        const auto& item = value.at("summary");
        out.summary = check_text(decode_field<std::string>(item, "summary"), "summary", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("summary is required and must not be null");
    }
    if (value.contains("commit_count") && !value.at("commit_count").is_null()) {
        const auto& item = value.at("commit_count");
        out.commit_count = check_range<std::int32_t>(decode_field<std::int32_t>(item, "commit_count"), "commit_count", 1, std::numeric_limits<std::int32_t>::max());
    } else {
        throw std::invalid_argument("commit_count is required and must not be null");
    }
    if (value.contains("is_merge") && !value.at("is_merge").is_null()) {
        const auto& item = value.at("is_merge");
        out.is_merge = decode_field<bool>(item, "is_merge");
    } else {
        throw std::invalid_argument("is_merge is required and must not be null");
    }
    if (value.contains("saves") && !value.at("saves").is_null()) {
        const auto& item = value.at("saves");
        out.saves = decode_field<std::vector<holder::contract::CardHistorySave>>(item, "saves");
    } else {
        throw std::invalid_argument("saves is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardHistoryPage>::encode(const holder::contract::CardHistoryPage& value) {
    json result = json::object();
    result["head_oid"] = value.head_oid ? codec<std::string>::encode(*value.head_oid) : json(nullptr);
    result["entries"] = codec<std::vector<holder::contract::CardHistoryEntry>>::encode(value.entries);
    result["next_cursor"] = value.next_cursor ? codec<std::string>::encode(*value.next_cursor) : json(nullptr);
    result["scan_limited"] = codec<bool>::encode(value.scan_limited);
    return result;
}
inline holder::contract::CardHistoryPage codec<holder::contract::CardHistoryPage>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardHistoryPage object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "head_oid" && it.key() != "entries" && it.key() != "next_cursor" && it.key() != "scan_limited") throw std::invalid_argument("CardHistoryPage has no field " + it.key());
    holder::contract::CardHistoryPage out;
    if (value.contains("head_oid") && !value.at("head_oid").is_null()) {
        const auto& item = value.at("head_oid");
        out.head_oid = check_text(decode_field<std::string>(item, "head_oid"), "head_oid", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("entries") && !value.at("entries").is_null()) {
        const auto& item = value.at("entries");
        out.entries = decode_field<std::vector<holder::contract::CardHistoryEntry>>(item, "entries");
    } else {
        throw std::invalid_argument("entries is required and must not be null");
    }
    if (value.contains("next_cursor") && !value.at("next_cursor").is_null()) {
        const auto& item = value.at("next_cursor");
        out.next_cursor = check_text(decode_field<std::string>(item, "next_cursor"), "next_cursor", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("scan_limited") && !value.at("scan_limited").is_null()) {
        const auto& item = value.at("scan_limited");
        out.scan_limited = decode_field<bool>(item, "scan_limited");
    } else {
        throw std::invalid_argument("scan_limited is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardHistoryVersion>::encode(const holder::contract::CardHistoryVersion& value) {
    json result = json::object();
    result["exists"] = codec<bool>::encode(value.exists);
    result["oid"] = codec<std::string>::encode(value.oid);
    result["title"] = codec<std::string>::encode(value.title);
    result["body"] = codec<std::string>::encode(value.body);
    return result;
}
inline holder::contract::CardHistoryVersion codec<holder::contract::CardHistoryVersion>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardHistoryVersion object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "exists" && it.key() != "oid" && it.key() != "title" && it.key() != "body") throw std::invalid_argument("CardHistoryVersion has no field " + it.key());
    holder::contract::CardHistoryVersion out;
    if (value.contains("exists") && !value.at("exists").is_null()) {
        const auto& item = value.at("exists");
        out.exists = decode_field<bool>(item, "exists");
    } else {
        throw std::invalid_argument("exists is required and must not be null");
    }
    if (value.contains("oid") && !value.at("oid").is_null()) {
        const auto& item = value.at("oid");
        out.oid = check_text(decode_field<std::string>(item, "oid"), "oid", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("oid is required and must not be null");
    }
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("title is required and must not be null");
    }
    if (value.contains("body") && !value.at("body").is_null()) {
        const auto& item = value.at("body");
        out.body = check_text(decode_field<std::string>(item, "body"), "body", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("body is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardHistoryDiffLine>::encode(const holder::contract::CardHistoryDiffLine& value) {
    json result = json::object();
    result["origin"] = codec<std::string>::encode(value.origin);
    result["text"] = codec<std::string>::encode(value.text);
    result["old_line"] = value.old_line ? codec<long long>::encode(*value.old_line) : json(nullptr);
    result["new_line"] = value.new_line ? codec<long long>::encode(*value.new_line) : json(nullptr);
    return result;
}
inline holder::contract::CardHistoryDiffLine codec<holder::contract::CardHistoryDiffLine>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardHistoryDiffLine object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "origin" && it.key() != "text" && it.key() != "old_line" && it.key() != "new_line") throw std::invalid_argument("CardHistoryDiffLine has no field " + it.key());
    holder::contract::CardHistoryDiffLine out;
    if (value.contains("origin") && !value.at("origin").is_null()) {
        const auto& item = value.at("origin");
        out.origin = check_text(decode_field<std::string>(item, "origin"), "origin", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("origin is required and must not be null");
    }
    if (value.contains("text") && !value.at("text").is_null()) {
        const auto& item = value.at("text");
        out.text = check_text(decode_field<std::string>(item, "text"), "text", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("text is required and must not be null");
    }
    if (value.contains("old_line") && !value.at("old_line").is_null()) {
        const auto& item = value.at("old_line");
        out.old_line = decode_field<long long>(item, "old_line");
    }
    if (value.contains("new_line") && !value.at("new_line").is_null()) {
        const auto& item = value.at("new_line");
        out.new_line = decode_field<long long>(item, "new_line");
    }
    return out;
}

inline json codec<holder::contract::CardHistoryComparison>::encode(const holder::contract::CardHistoryComparison& value) {
    json result = json::object();
    result["before"] = codec<holder::contract::CardHistoryVersion>::encode(value.before);
    result["after"] = codec<holder::contract::CardHistoryVersion>::encode(value.after);
    result["summary"] = codec<std::string>::encode(value.summary);
    result["lines"] = codec<std::vector<holder::contract::CardHistoryDiffLine>>::encode(value.lines);
    result["truncated"] = codec<bool>::encode(value.truncated);
    return result;
}
inline holder::contract::CardHistoryComparison codec<holder::contract::CardHistoryComparison>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardHistoryComparison object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "before" && it.key() != "after" && it.key() != "summary" && it.key() != "lines" && it.key() != "truncated") throw std::invalid_argument("CardHistoryComparison has no field " + it.key());
    holder::contract::CardHistoryComparison out;
    if (value.contains("before") && !value.at("before").is_null()) {
        const auto& item = value.at("before");
        out.before = codec<holder::contract::CardHistoryVersion>::decode(item);
    } else {
        throw std::invalid_argument("before is required and must not be null");
    }
    if (value.contains("after") && !value.at("after").is_null()) {
        const auto& item = value.at("after");
        out.after = codec<holder::contract::CardHistoryVersion>::decode(item);
    } else {
        throw std::invalid_argument("after is required and must not be null");
    }
    if (value.contains("summary") && !value.at("summary").is_null()) {
        const auto& item = value.at("summary");
        out.summary = check_text(decode_field<std::string>(item, "summary"), "summary", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("summary is required and must not be null");
    }
    if (value.contains("lines") && !value.at("lines").is_null()) {
        const auto& item = value.at("lines");
        out.lines = decode_field<std::vector<holder::contract::CardHistoryDiffLine>>(item, "lines");
    } else {
        throw std::invalid_argument("lines is required and must not be null");
    }
    if (value.contains("truncated") && !value.at("truncated").is_null()) {
        const auto& item = value.at("truncated");
        out.truncated = decode_field<bool>(item, "truncated");
    } else {
        throw std::invalid_argument("truncated is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardHistorySnapshot>::encode(const holder::contract::CardHistorySnapshot& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["snapshot"] = codec<holder::contract::CardHistoryVersion>::encode(value.snapshot);
    return result;
}
inline holder::contract::CardHistorySnapshot codec<holder::contract::CardHistorySnapshot>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardHistorySnapshot object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "card_id" && it.key() != "snapshot") throw std::invalid_argument("CardHistorySnapshot has no field " + it.key());
    holder::contract::CardHistorySnapshot out;
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_id is required and must not be null");
    }
    if (value.contains("snapshot") && !value.at("snapshot").is_null()) {
        const auto& item = value.at("snapshot");
        out.snapshot = codec<holder::contract::CardHistoryVersion>::decode(item);
    } else {
        throw std::invalid_argument("snapshot is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardHistoryRestoreResult>::encode(const holder::contract::CardHistoryRestoreResult& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["restored_from_oid"] = codec<std::string>::encode(value.restored_from_oid);
    result["result_oid"] = codec<std::string>::encode(value.result_oid);
    result["title"] = codec<std::string>::encode(value.title);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    result["deleted_at"] = value.deleted_at ? codec<long long>::encode(*value.deleted_at) : json(nullptr);
    return result;
}
inline holder::contract::CardHistoryRestoreResult codec<holder::contract::CardHistoryRestoreResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardHistoryRestoreResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "card_id" && it.key() != "restored_from_oid" && it.key() != "result_oid" && it.key() != "title" && it.key() != "updated_at" && it.key() != "deleted_at") throw std::invalid_argument("CardHistoryRestoreResult has no field " + it.key());
    holder::contract::CardHistoryRestoreResult out;
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_id is required and must not be null");
    }
    if (value.contains("restored_from_oid") && !value.at("restored_from_oid").is_null()) {
        const auto& item = value.at("restored_from_oid");
        out.restored_from_oid = check_text(decode_field<std::string>(item, "restored_from_oid"), "restored_from_oid", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("restored_from_oid is required and must not be null");
    }
    if (value.contains("result_oid") && !value.at("result_oid").is_null()) {
        const auto& item = value.at("result_oid");
        out.result_oid = check_text(decode_field<std::string>(item, "result_oid"), "result_oid", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("result_oid is required and must not be null");
    }
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("title is required and must not be null");
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

inline json codec<holder::contract::TrashItem>::encode(const holder::contract::TrashItem& value) {
    json result = json::object();
    result["type"] = codec<holder::contract::TrashItemType>::encode(value.type);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["card_id"] = value.card_id ? codec<std::string>::encode(*value.card_id) : json(nullptr);
    result["message_id"] = value.message_id ? codec<std::string>::encode(*value.message_id) : json(nullptr);
    result["thread_id"] = value.thread_id ? codec<std::string>::encode(*value.thread_id) : json(nullptr);
    result["title"] = value.title ? codec<std::string>::encode(*value.title) : json(nullptr);
    result["role"] = value.role ? codec<std::string>::encode(*value.role) : json(nullptr);
    result["deleted_at"] = codec<long long>::encode(value.deleted_at);
    result["created_at"] = value.created_at ? codec<long long>::encode(*value.created_at) : json(nullptr);
    result["rel_path"] = value.rel_path ? codec<std::string>::encode(*value.rel_path) : json(nullptr);
    return result;
}
inline holder::contract::TrashItem codec<holder::contract::TrashItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a TrashItem object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "type" && it.key() != "project_id" && it.key() != "card_id" && it.key() != "message_id" && it.key() != "thread_id" && it.key() != "title" && it.key() != "role" && it.key() != "deleted_at" && it.key() != "created_at" && it.key() != "rel_path") throw std::invalid_argument("TrashItem has no field " + it.key());
    holder::contract::TrashItem out;
    if (value.contains("type") && !value.at("type").is_null()) {
        const auto& item = value.at("type");
        out.type = decode_field<holder::contract::TrashItemType>(item, "type");
    } else {
        throw std::invalid_argument("type is required and must not be null");
    }
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("message_id") && !value.at("message_id").is_null()) {
        const auto& item = value.at("message_id");
        out.message_id = check_text(decode_field<std::string>(item, "message_id"), "message_id", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("thread_id") && !value.at("thread_id").is_null()) {
        const auto& item = value.at("thread_id");
        out.thread_id = check_text(decode_field<std::string>(item, "thread_id"), "thread_id", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("role") && !value.at("role").is_null()) {
        const auto& item = value.at("role");
        out.role = check_text(decode_field<std::string>(item, "role"), "role", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("deleted_at") && !value.at("deleted_at").is_null()) {
        const auto& item = value.at("deleted_at");
        out.deleted_at = decode_field<long long>(item, "deleted_at");
    } else {
        throw std::invalid_argument("deleted_at is required and must not be null");
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = decode_field<long long>(item, "created_at");
    }
    if (value.contains("rel_path") && !value.at("rel_path").is_null()) {
        const auto& item = value.at("rel_path");
        out.rel_path = check_text(decode_field<std::string>(item, "rel_path"), "rel_path", 0, std::numeric_limits<std::size_t>::max());
    }
    return out;
}

inline json codec<holder::contract::TrashEmptyResult>::encode(const holder::contract::TrashEmptyResult& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    return result;
}
inline holder::contract::TrashEmptyResult codec<holder::contract::TrashEmptyResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a TrashEmptyResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id") throw std::invalid_argument("TrashEmptyResult has no field " + it.key());
    holder::contract::TrashEmptyResult out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::TrashDeleteResult>::encode(const holder::contract::TrashDeleteResult& value) {
    json result = json::object();
    result["card_id"] = value.card_id ? codec<std::string>::encode(*value.card_id) : json(nullptr);
    result["message_id"] = value.message_id ? codec<std::string>::encode(*value.message_id) : json(nullptr);
    return result;
}
inline holder::contract::TrashDeleteResult codec<holder::contract::TrashDeleteResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a TrashDeleteResult object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "card_id" && it.key() != "message_id") throw std::invalid_argument("TrashDeleteResult has no field " + it.key());
    holder::contract::TrashDeleteResult out;
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 0, std::numeric_limits<std::size_t>::max());
    }
    if (value.contains("message_id") && !value.at("message_id").is_null()) {
        const auto& item = value.at("message_id");
        out.message_id = check_text(decode_field<std::string>(item, "message_id"), "message_id", 0, std::numeric_limits<std::size_t>::max());
    }
    return out;
}

inline json codec<holder::contract::SearchCard>::encode(const holder::contract::SearchCard& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["snippet"] = codec<std::string>::encode(value.snippet);
    result["rank"] = codec<double>::encode(value.rank);
    return result;
}
inline holder::contract::SearchCard codec<holder::contract::SearchCard>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a SearchCard object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "card_id" && it.key() != "title" && it.key() != "updated_at" && it.key() != "created_at" && it.key() != "snippet" && it.key() != "rank") throw std::invalid_argument("SearchCard has no field " + it.key());
    holder::contract::SearchCard out;
    if (value.contains("card_id") && !value.at("card_id").is_null()) {
        const auto& item = value.at("card_id");
        out.card_id = check_text(decode_field<std::string>(item, "card_id"), "card_id", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("card_id is required and must not be null");
    }
    if (value.contains("title") && !value.at("title").is_null()) {
        const auto& item = value.at("title");
        out.title = check_text(decode_field<std::string>(item, "title"), "title", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("title is required and must not be null");
    }
    if (value.contains("updated_at") && !value.at("updated_at").is_null()) {
        const auto& item = value.at("updated_at");
        out.updated_at = decode_field<long long>(item, "updated_at");
    } else {
        throw std::invalid_argument("updated_at is required and must not be null");
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = decode_field<long long>(item, "created_at");
    } else {
        throw std::invalid_argument("created_at is required and must not be null");
    }
    if (value.contains("snippet") && !value.at("snippet").is_null()) {
        const auto& item = value.at("snippet");
        out.snippet = check_text(decode_field<std::string>(item, "snippet"), "snippet", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("snippet is required and must not be null");
    }
    if (value.contains("rank") && !value.at("rank").is_null()) {
        const auto& item = value.at("rank");
        out.rank = decode_field<double>(item, "rank");
    } else {
        throw std::invalid_argument("rank is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::SearchMessage>::encode(const holder::contract::SearchMessage& value) {
    json result = json::object();
    result["message_id"] = codec<std::string>::encode(value.message_id);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["snippet"] = codec<std::string>::encode(value.snippet);
    result["rank"] = codec<double>::encode(value.rank);
    return result;
}
inline holder::contract::SearchMessage codec<holder::contract::SearchMessage>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a SearchMessage object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "message_id" && it.key() != "created_at" && it.key() != "snippet" && it.key() != "rank") throw std::invalid_argument("SearchMessage has no field " + it.key());
    holder::contract::SearchMessage out;
    if (value.contains("message_id") && !value.at("message_id").is_null()) {
        const auto& item = value.at("message_id");
        out.message_id = check_text(decode_field<std::string>(item, "message_id"), "message_id", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("message_id is required and must not be null");
    }
    if (value.contains("created_at") && !value.at("created_at").is_null()) {
        const auto& item = value.at("created_at");
        out.created_at = decode_field<long long>(item, "created_at");
    } else {
        throw std::invalid_argument("created_at is required and must not be null");
    }
    if (value.contains("snippet") && !value.at("snippet").is_null()) {
        const auto& item = value.at("snippet");
        out.snippet = check_text(decode_field<std::string>(item, "snippet"), "snippet", 0, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("snippet is required and must not be null");
    }
    if (value.contains("rank") && !value.at("rank").is_null()) {
        const auto& item = value.at("rank");
        out.rank = decode_field<double>(item, "rank");
    } else {
        throw std::invalid_argument("rank is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::NewStorageLocation>::encode(const holder::contract::NewStorageLocation& value) {
    json result = json::object();
    if (value.location_id) result["location_id"] = codec<std::string>::encode(*value.location_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["name"] = codec<std::string>::encode(value.name);
    result["provider"] = codec<holder::contract::NewStorageLocationProvider>::encode(value.provider);
    if (value.configuration) result["configuration"] = codec<std::map<std::string, std::string>>::encode(*value.configuration);
    if (value.created_at) result["created_at"] = codec<long long>::encode(*value.created_at);
    if (value.updated_at) result["updated_at"] = codec<long long>::encode(*value.updated_at);
    return result;
}
inline holder::contract::NewStorageLocation codec<holder::contract::NewStorageLocation>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a NewStorageLocation object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "location_id" && it.key() != "project_id" && it.key() != "name" && it.key() != "provider" && it.key() != "configuration" && it.key() != "created_at" && it.key() != "updated_at") throw std::invalid_argument("NewStorageLocation has no field " + it.key());
    holder::contract::NewStorageLocation out;
    if (value.contains("location_id") && !value.at("location_id").is_null()) {
        const auto& item = value.at("location_id");
        out.location_id = check_text(decode_field<std::string>(item, "location_id"), "location_id", 0, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("location_id")) {
        throw std::invalid_argument("location_id must not be null");
    }
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
    if (value.contains("provider") && !value.at("provider").is_null()) {
        const auto& item = value.at("provider");
        out.provider = decode_field<holder::contract::NewStorageLocationProvider>(item, "provider");
    } else {
        throw std::invalid_argument("provider is required and must not be null");
    }
    if (value.contains("configuration") && !value.at("configuration").is_null()) {
        const auto& item = value.at("configuration");
        out.configuration = [&] { auto entries = decode_field<std::map<std::string, std::string>>(item, "configuration"); for (const auto& [key, text] : entries) { check_text(key, "configuration key", 0, std::numeric_limits<std::size_t>::max()); check_text(text, "configuration", 0, std::numeric_limits<std::size_t>::max()); } return entries; }();
    } else if (value.contains("configuration")) {
        throw std::invalid_argument("configuration must not be null");
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

inline json codec<holder::contract::StorageLocationUpdate>::encode(const holder::contract::StorageLocationUpdate& value) {
    json result = json::object();
    if (value.name) result["name"] = codec<std::string>::encode(*value.name);
    if (value.configuration) result["configuration"] = codec<std::map<std::string, std::string>>::encode(*value.configuration);
    if (value.updated_at) result["updated_at"] = codec<long long>::encode(*value.updated_at);
    return result;
}
inline holder::contract::StorageLocationUpdate codec<holder::contract::StorageLocationUpdate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a StorageLocationUpdate object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "name" && it.key() != "configuration" && it.key() != "updated_at") throw std::invalid_argument("StorageLocationUpdate has no field " + it.key());
    holder::contract::StorageLocationUpdate out;
    if (value.contains("name") && !value.at("name").is_null()) {
        const auto& item = value.at("name");
        out.name = check_text(decode_field<std::string>(item, "name"), "name", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("name")) {
        throw std::invalid_argument("name must not be null");
    }
    if (value.contains("configuration") && !value.at("configuration").is_null()) {
        const auto& item = value.at("configuration");
        out.configuration = [&] { auto entries = decode_field<std::map<std::string, std::string>>(item, "configuration"); for (const auto& [key, text] : entries) { check_text(key, "configuration key", 0, std::numeric_limits<std::size_t>::max()); check_text(text, "configuration", 0, std::numeric_limits<std::size_t>::max()); } return entries; }();
    } else if (value.contains("configuration")) {
        throw std::invalid_argument("configuration must not be null");
    }
    if (value.contains("updated_at") && !value.at("updated_at").is_null()) {
        const auto& item = value.at("updated_at");
        out.updated_at = decode_field<long long>(item, "updated_at");
    } else if (value.contains("updated_at")) {
        throw std::invalid_argument("updated_at must not be null");
    }
    return out;
}

inline json codec<holder::contract::CardReferenceResolveRequest>::encode(const holder::contract::CardReferenceResolveRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["reference"] = codec<std::string>::encode(value.reference);
    result["scope"] = codec<holder::contract::CardReferenceResolveRequestScope>::encode(value.scope);
    return result;
}
inline holder::contract::CardReferenceResolveRequest codec<holder::contract::CardReferenceResolveRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a CardReferenceResolveRequest object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id" && it.key() != "reference" && it.key() != "scope") throw std::invalid_argument("CardReferenceResolveRequest has no field " + it.key());
    holder::contract::CardReferenceResolveRequest out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    if (value.contains("reference") && !value.at("reference").is_null()) {
        const auto& item = value.at("reference");
        out.reference = check_text(decode_field<std::string>(item, "reference"), "reference", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("reference is required and must not be null");
    }
    if (value.contains("scope") && !value.at("scope").is_null()) {
        const auto& item = value.at("scope");
        out.scope = decode_field<holder::contract::CardReferenceResolveRequestScope>(item, "scope");
    } else {
        throw std::invalid_argument("scope is required and must not be null");
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

inline json codec<holder::contract::HierarchyRepairRequest>::encode(const holder::contract::HierarchyRepairRequest& value) {
    json result = json::object();
    result["apply"] = codec<bool>::encode(value.apply);
    return result;
}
inline holder::contract::HierarchyRepairRequest codec<holder::contract::HierarchyRepairRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a HierarchyRepairRequest object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "apply") throw std::invalid_argument("HierarchyRepairRequest has no field " + it.key());
    holder::contract::HierarchyRepairRequest out;
    if (value.contains("apply") && !value.at("apply").is_null()) {
        const auto& item = value.at("apply");
        out.apply = decode_field<bool>(item, "apply");
    } else if (value.contains("apply")) {
        throw std::invalid_argument("apply must not be null");
    }
    return out;
}

inline json codec<holder::contract::RecoveryTokenExportRequest>::encode(const holder::contract::RecoveryTokenExportRequest& value) {
    json result = json::object();
    result["pin"] = codec<std::string>::encode(value.pin);
    return result;
}
inline holder::contract::RecoveryTokenExportRequest codec<holder::contract::RecoveryTokenExportRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a RecoveryTokenExportRequest object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "pin") throw std::invalid_argument("RecoveryTokenExportRequest has no field " + it.key());
    holder::contract::RecoveryTokenExportRequest out;
    if (value.contains("pin") && !value.at("pin").is_null()) {
        const auto& item = value.at("pin");
        out.pin = check_text(decode_field<std::string>(item, "pin"), "pin", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("pin is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::RecoveryTokenImportRequest>::encode(const holder::contract::RecoveryTokenImportRequest& value) {
    json result = json::object();
    result["pin"] = codec<std::string>::encode(value.pin);
    result["recovery_token"] = codec<std::string>::encode(value.recovery_token);
    return result;
}
inline holder::contract::RecoveryTokenImportRequest codec<holder::contract::RecoveryTokenImportRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a RecoveryTokenImportRequest object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "pin" && it.key() != "recovery_token") throw std::invalid_argument("RecoveryTokenImportRequest has no field " + it.key());
    holder::contract::RecoveryTokenImportRequest out;
    if (value.contains("pin") && !value.at("pin").is_null()) {
        const auto& item = value.at("pin");
        out.pin = check_text(decode_field<std::string>(item, "pin"), "pin", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("pin is required and must not be null");
    }
    if (value.contains("recovery_token") && !value.at("recovery_token").is_null()) {
        const auto& item = value.at("recovery_token");
        out.recovery_token = check_text(decode_field<std::string>(item, "recovery_token"), "recovery_token", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("recovery_token is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::RebuildRequest>::encode(const holder::contract::RebuildRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    return result;
}
inline holder::contract::RebuildRequest codec<holder::contract::RebuildRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a RebuildRequest object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "project_id") throw std::invalid_argument("RebuildRequest has no field " + it.key());
    holder::contract::RebuildRequest out;
    if (value.contains("project_id") && !value.at("project_id").is_null()) {
        const auto& item = value.at("project_id");
        out.project_id = check_text(decode_field<std::string>(item, "project_id"), "project_id", 1, std::numeric_limits<std::size_t>::max());
    } else {
        throw std::invalid_argument("project_id is required and must not be null");
    }
    return out;
}

inline json codec<holder::contract::PushOptions>::encode(const holder::contract::PushOptions& value) {
    json result = json::object();
    if (value.branch) result["branch"] = codec<std::string>::encode(*value.branch);
    result["set_upstream"] = codec<bool>::encode(value.set_upstream);
    return result;
}
inline holder::contract::PushOptions codec<holder::contract::PushOptions>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a PushOptions object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "branch" && it.key() != "set_upstream") throw std::invalid_argument("PushOptions has no field " + it.key());
    holder::contract::PushOptions out;
    if (value.contains("branch") && !value.at("branch").is_null()) {
        const auto& item = value.at("branch");
        out.branch = check_text(decode_field<std::string>(item, "branch"), "branch", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("branch")) {
        throw std::invalid_argument("branch must not be null");
    }
    if (value.contains("set_upstream") && !value.at("set_upstream").is_null()) {
        const auto& item = value.at("set_upstream");
        out.set_upstream = decode_field<bool>(item, "set_upstream");
    } else if (value.contains("set_upstream")) {
        throw std::invalid_argument("set_upstream must not be null");
    }
    return out;
}

inline json codec<holder::contract::RemoteTest>::encode(const holder::contract::RemoteTest& value) {
    json result = json::object();
    if (value.remote_url) result["remote_url"] = codec<std::string>::encode(*value.remote_url);
    return result;
}
inline holder::contract::RemoteTest codec<holder::contract::RemoteTest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected a RemoteTest object");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "remote_url") throw std::invalid_argument("RemoteTest has no field " + it.key());
    holder::contract::RemoteTest out;
    if (value.contains("remote_url") && !value.at("remote_url").is_null()) {
        const auto& item = value.at("remote_url");
        out.remote_url = check_text(decode_field<std::string>(item, "remote_url"), "remote_url", 1, std::numeric_limits<std::size_t>::max());
    } else if (value.contains("remote_url")) {
        throw std::invalid_argument("remote_url must not be null");
    }
    return out;
}

template <typename T> json encode(const T& value) { return codec<T>::encode(value); }
template <typename T> T decode(const json& value) { return codec<T>::decode(value); }
}  // namespace contract_json
