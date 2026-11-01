// Generated from contract/interface/ai/models.py, contract/interface/ai/operations.py, contract/interface/cards/errors.py, contract/interface/cards/models.py, contract/interface/cards/operations.py, contract/interface/cards/references.py, contract/interface/daemon/models.py, contract/interface/daemon/operations.py, contract/interface/documents/operations.py, contract/interface/errors.py, contract/interface/events/models.py, contract/interface/events/operations.py, contract/interface/git/models.py, contract/interface/git/operations.py, contract/interface/history/errors.py, contract/interface/history/models.py, contract/interface/history/operations.py, contract/interface/links/models.py, contract/interface/links/operations.py, contract/interface/locations/models.py, contract/interface/locations/operations.py, contract/interface/maintenance/errors.py, contract/interface/maintenance/models.py, contract/interface/maintenance/operations.py, contract/interface/maintenance/rebuild.py, contract/interface/milestones/models.py, contract/interface/milestones/operations.py, contract/interface/pagination/models.py, contract/interface/projects/ids.py, contract/interface/projects/models.py, contract/interface/projects/operations.py, contract/interface/recovery/errors.py, contract/interface/recovery/models.py, contract/interface/recovery/operations.py, contract/interface/recovery/snapshot.py, contract/interface/resources/models.py, contract/interface/resources/operations.py, contract/interface/search/models.py, contract/interface/search/operations.py, contract/interface/trash/errors.py, contract/interface/trash/models.py, contract/interface/trash/operations.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// The daemon AI JSON codec, generated from contract/interface/ai/models.py.
#pragma once
#include "api/AiRuntime.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ai_json {
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
template <> struct codec<json> {
    static json encode(const json& value) { return value; }
    static json decode(const json& value) { return value; }
};
template <typename T> struct codec<std::optional<T>> {
    static json encode(const std::optional<T>& value) {
        return value ? codec<T>::encode(*value) : json(nullptr);
    }
    static std::optional<T> decode(const json& value) {
        return value.is_null() ? std::nullopt : std::optional<T>(codec<T>::decode(value));
    }
};
template <typename T> struct is_optional : std::false_type {};
template <typename T> struct is_optional<std::optional<T>> : std::true_type {};
template <typename T> T decode_optional_nullable(const json& value) {
    if constexpr (is_optional<typename T::value_type>::value) {
        if (value.is_null()) return T{std::in_place, typename T::value_type{}};
    }
    return codec<T>::decode(value);
}

template <typename... Ts> struct codec<std::variant<Ts...>> {
    static json encode(const std::variant<Ts...>& value) {
        return std::visit([](const auto& item) { return codec<std::decay_t<decltype(item)>>::encode(item); }, value);
    }
};

template <> struct codec<holder::api::Error> {
    static json encode(const holder::api::Error& value) {
        return {{"code", value.code}, {"message", value.message}};
    }
};
template <> struct codec<holder::api::ErrorResponse> {
    static json encode(const holder::api::ErrorResponse& value) {
        return {{"ok", value.ok}, {"error", codec<holder::api::Error>::encode(value.error)}};
    }
};

template <> struct codec<holder::api::RunnerPullProgress> {
    static json encode(const holder::api::RunnerPullProgress& value);
    static holder::api::RunnerPullProgress decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullStartResponseData> {
    static json encode(const holder::api::RunnerPullStartResponseData& value);
    static holder::api::RunnerPullStartResponseData decode(const json& value);
};
template <> struct codec<holder::api::PostAiRunnerPullRequest> {
    static json encode(const holder::api::PostAiRunnerPullRequest& value);
    static holder::api::PostAiRunnerPullRequest decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredential> {
    static json encode(const holder::api::AiProviderCredential& value);
    static holder::api::AiProviderCredential decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialUpsertRequest> {
    static json encode(const holder::api::AiProviderCredentialUpsertRequest& value);
    static holder::api::AiProviderCredentialUpsertRequest decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialDeleteResponseData> {
    static json encode(const holder::api::AiProviderCredentialDeleteResponseData& value);
    static holder::api::AiProviderCredentialDeleteResponseData decode(const json& value);
};
template <> struct codec<holder::api::CardLinkDeleteRequest> {
    static json encode(const holder::api::CardLinkDeleteRequest& value);
    static holder::api::CardLinkDeleteRequest decode(const json& value);
};
template <> struct codec<holder::api::AiLocalModelConfigData> {
    static json encode(const holder::api::AiLocalModelConfigData& value);
    static holder::api::AiLocalModelConfigData decode(const json& value);
};
template <> struct codec<holder::api::LocalModel> {
    static json encode(const holder::api::LocalModel& value);
    static holder::api::LocalModel decode(const json& value);
};
template <> struct codec<holder::api::AiRunEventData> {
    static json encode(const holder::api::AiRunEventData& value);
    static holder::api::AiRunEventData decode(const json& value);
};
template <> struct codec<holder::api::AiRunCreateRequest> {
    static json encode(const holder::api::AiRunCreateRequest& value);
    static holder::api::AiRunCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiThread> {
    static json encode(const holder::api::AiThread& value);
    static holder::api::AiThread decode(const json& value);
};
template <> struct codec<holder::api::AiLocalModelConfigUpdateRequest> {
    static json encode(const holder::api::AiLocalModelConfigUpdateRequest& value);
    static holder::api::AiLocalModelConfigUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiMessage> {
    static json encode(const holder::api::AiMessage& value);
    static holder::api::AiMessage decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCreateResponseData> {
    static json encode(const holder::api::AiMessageCreateResponseData& value);
    static holder::api::AiMessageCreateResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCreateRequest> {
    static json encode(const holder::api::AiMessageCreateRequest& value);
    static holder::api::AiMessageCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiMessageUpdateResponseData> {
    static json encode(const holder::api::AiMessageUpdateResponseData& value);
    static holder::api::AiMessageUpdateResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiMessageUpdateRequest> {
    static json encode(const holder::api::AiMessageUpdateRequest& value);
    static holder::api::AiMessageUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCaptureResponseData> {
    static json encode(const holder::api::AiMessageCaptureResponseData& value);
    static holder::api::AiMessageCaptureResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCaptureRequest> {
    static json encode(const holder::api::AiMessageCaptureRequest& value);
    static holder::api::AiMessageCaptureRequest decode(const json& value);
};
template <> struct codec<holder::api::AiNudge> {
    static json encode(const holder::api::AiNudge& value);
    static holder::api::AiNudge decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeDismissData> {
    static json encode(const holder::api::AiNudgeDismissData& value);
    static holder::api::AiNudgeDismissData decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeCandidate> {
    static json encode(const holder::api::AiNudgeCandidate& value);
    static holder::api::AiNudgeCandidate decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogModelLimits> {
    static json encode(const holder::api::AiProviderCatalogModelLimits& value);
    static holder::api::AiProviderCatalogModelLimits decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogApi> {
    static json encode(const holder::api::AiProviderCatalogApi& value);
    static holder::api::AiProviderCatalogApi decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogAuth> {
    static json encode(const holder::api::AiProviderCatalogAuth& value);
    static holder::api::AiProviderCatalogAuth decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerCreateRequest> {
    static json encode(const holder::api::AiRunnerCreateRequest& value);
    static holder::api::AiRunnerCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerUpdateRequest> {
    static json encode(const holder::api::AiRunnerUpdateRequest& value);
    static holder::api::AiRunnerUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerDeleteResponseData> {
    static json encode(const holder::api::AiRunnerDeleteResponseData& value);
    static holder::api::AiRunnerDeleteResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiRunItem> {
    static json encode(const holder::api::AiRunItem& value);
    static holder::api::AiRunItem decode(const json& value);
};
template <> struct codec<holder::api::CasteInfo> {
    static json encode(const holder::api::CasteInfo& value);
    static holder::api::CasteInfo decode(const json& value);
};
template <> struct codec<holder::api::CasteRecommendedModel> {
    static json encode(const holder::api::CasteRecommendedModel& value);
    static holder::api::CasteRecommendedModel decode(const json& value);
};
template <> struct codec<holder::api::AiThreadCreateResponseData> {
    static json encode(const holder::api::AiThreadCreateResponseData& value);
    static holder::api::AiThreadCreateResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiThreadCreateRequest> {
    static json encode(const holder::api::AiThreadCreateRequest& value);
    static holder::api::AiThreadCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiThreadPatchResponseData> {
    static json encode(const holder::api::AiThreadPatchResponseData& value);
    static holder::api::AiThreadPatchResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiThreadUpdateRequest> {
    static json encode(const holder::api::AiThreadUpdateRequest& value);
    static holder::api::AiThreadUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSetting> {
    static json encode(const holder::api::AiProviderSetting& value);
    static holder::api::AiProviderSetting decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingUpdateRequest> {
    static json encode(const holder::api::AiProviderSettingUpdateRequest& value);
    static holder::api::AiProviderSettingUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingDeleteData> {
    static json encode(const holder::api::AiProviderSettingDeleteData& value);
    static holder::api::AiProviderSettingDeleteData decode(const json& value);
};
template <> struct codec<holder::api::AiStreamResyncData> {
    static json encode(const holder::api::AiStreamResyncData& value);
    static holder::api::AiStreamResyncData decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLink> {
    static json encode(const holder::api::routes::CardLink& value);
    static holder::api::routes::CardLink decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLinkCreateRequest> {
    static json encode(const holder::api::routes::CardLinkCreateRequest& value);
    static holder::api::routes::CardLinkCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullJob> {
    static json encode(const holder::api::RunnerPullJob& value);
    static holder::api::RunnerPullJob decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullStartResponse> {
    static json encode(const holder::api::RunnerPullStartResponse& value);
    static holder::api::RunnerPullStartResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialsListData> {
    static json encode(const holder::api::AiProviderCredentialsListData& value);
    static holder::api::AiProviderCredentialsListData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialUpsertResponse> {
    static json encode(const holder::api::AiProviderCredentialUpsertResponse& value);
    static holder::api::AiProviderCredentialUpsertResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialDeleteResponse> {
    static json encode(const holder::api::AiProviderCredentialDeleteResponse& value);
    static holder::api::AiProviderCredentialDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::AiLocalModelConfigResponse> {
    static json encode(const holder::api::AiLocalModelConfigResponse& value);
    static holder::api::AiLocalModelConfigResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageListResponse> {
    static json encode(const holder::api::AiMessageListResponse& value);
    static holder::api::AiMessageListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageGetResponse> {
    static json encode(const holder::api::AiMessageGetResponse& value);
    static holder::api::AiMessageGetResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCreateResponse> {
    static json encode(const holder::api::AiMessageCreateResponse& value);
    static holder::api::AiMessageCreateResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageUpdateResponse> {
    static json encode(const holder::api::AiMessageUpdateResponse& value);
    static holder::api::AiMessageUpdateResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCaptureResponse> {
    static json encode(const holder::api::AiMessageCaptureResponse& value);
    static holder::api::AiMessageCaptureResponse decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeListData> {
    static json encode(const holder::api::AiNudgeListData& value);
    static holder::api::AiNudgeListData decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeDismissResponse> {
    static json encode(const holder::api::AiNudgeDismissResponse& value);
    static holder::api::AiNudgeDismissResponse decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeEvaluateData> {
    static json encode(const holder::api::AiNudgeEvaluateData& value);
    static holder::api::AiNudgeEvaluateData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogModel> {
    static json encode(const holder::api::AiProviderCatalogModel& value);
    static holder::api::AiProviderCatalogModel decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerDeleteResponse> {
    static json encode(const holder::api::AiRunnerDeleteResponse& value);
    static holder::api::AiRunnerDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::AiRunListResponse> {
    static json encode(const holder::api::AiRunListResponse& value);
    static holder::api::AiRunListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiRunGetResponse> {
    static json encode(const holder::api::AiRunGetResponse& value);
    static holder::api::AiRunGetResponse decode(const json& value);
};
template <> struct codec<holder::api::AiThreadListResponse> {
    static json encode(const holder::api::AiThreadListResponse& value);
    static holder::api::AiThreadListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiThreadGetResponse> {
    static json encode(const holder::api::AiThreadGetResponse& value);
    static holder::api::AiThreadGetResponse decode(const json& value);
};
template <> struct codec<holder::api::AiThreadCreateResponse> {
    static json encode(const holder::api::AiThreadCreateResponse& value);
    static holder::api::AiThreadCreateResponse decode(const json& value);
};
template <> struct codec<holder::api::AiThreadPatchResponse> {
    static json encode(const holder::api::AiThreadPatchResponse& value);
    static holder::api::AiThreadPatchResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingResponse> {
    static json encode(const holder::api::AiProviderSettingResponse& value);
    static holder::api::AiProviderSettingResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingsData> {
    static json encode(const holder::api::AiProviderSettingsData& value);
    static holder::api::AiProviderSettingsData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingDeleteResponse> {
    static json encode(const holder::api::AiProviderSettingDeleteResponse& value);
    static holder::api::AiProviderSettingDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLinkListResponse> {
    static json encode(const holder::api::routes::CardLinkListResponse& value);
    static holder::api::routes::CardLinkListResponse decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLinkCreateResponse> {
    static json encode(const holder::api::routes::CardLinkCreateResponse& value);
    static holder::api::routes::CardLinkCreateResponse decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullStatusResponse> {
    static json encode(const holder::api::RunnerPullStatusResponse& value);
    static holder::api::RunnerPullStatusResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialsListResponse> {
    static json encode(const holder::api::AiProviderCredentialsListResponse& value);
    static holder::api::AiProviderCredentialsListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerRuntime> {
    static json encode(const holder::api::AiRunnerRuntime& value);
    static holder::api::AiRunnerRuntime decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeListResponse> {
    static json encode(const holder::api::AiNudgeListResponse& value);
    static holder::api::AiNudgeListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeEvaluateResponse> {
    static json encode(const holder::api::AiNudgeEvaluateResponse& value);
    static holder::api::AiNudgeEvaluateResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogProvider> {
    static json encode(const holder::api::AiProviderCatalogProvider& value);
    static holder::api::AiProviderCatalogProvider decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingsResponse> {
    static json encode(const holder::api::AiProviderSettingsResponse& value);
    static holder::api::AiProviderSettingsResponse decode(const json& value);
};
template <> struct codec<holder::api::AiRunner> {
    static json encode(const holder::api::AiRunner& value);
    static holder::api::AiRunner decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogResponseData> {
    static json encode(const holder::api::AiProviderCatalogResponseData& value);
    static holder::api::AiProviderCatalogResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerListResponseData> {
    static json encode(const holder::api::AiRunnerListResponseData& value);
    static holder::api::AiRunnerListResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerResponse> {
    static json encode(const holder::api::AiRunnerResponse& value);
    static holder::api::AiRunnerResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogResponse> {
    static json encode(const holder::api::AiProviderCatalogResponse& value);
    static holder::api::AiProviderCatalogResponse decode(const json& value);
};
template <> struct codec<holder::api::RunnerCapabilitiesData> {
    static json encode(const holder::api::RunnerCapabilitiesData& value);
    static holder::api::RunnerCapabilitiesData decode(const json& value);
};
template <> struct codec<holder::api::AiStatusData> {
    static json encode(const holder::api::AiStatusData& value);
    static holder::api::AiStatusData decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerListResponse> {
    static json encode(const holder::api::AiRunnerListResponse& value);
    static holder::api::AiRunnerListResponse decode(const json& value);
};
template <> struct codec<holder::api::RunnerCapabilitiesResponse> {
    static json encode(const holder::api::RunnerCapabilitiesResponse& value);
    static holder::api::RunnerCapabilitiesResponse decode(const json& value);
};
template <> struct codec<holder::api::AiStatusResponse> {
    static json encode(const holder::api::AiStatusResponse& value);
    static holder::api::AiStatusResponse decode(const json& value);
};

inline json codec<holder::api::RunnerPullProgress>::encode(const holder::api::RunnerPullProgress& value) {
    json result = json::object();
    result["completed"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullProgress>().completed)>>::encode(value.completed);
    result["total"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullProgress>().total)>>::encode(value.total);
    result["percent"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullProgress>().percent)>>::encode(value.percent);
    result["stage"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullProgress>().stage)>>::encode(value.stage);
    return result;
}
inline holder::api::RunnerPullProgress codec<holder::api::RunnerPullProgress>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullProgress");
    holder::api::RunnerPullProgress out{};
    if (!value.contains("completed")) throw std::invalid_argument("missing completed");
    out.completed = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullProgress>().completed)>>::decode(value.at("completed"));
    if (value.at("completed").is_null()) throw std::invalid_argument("completed must not be null");
    if (!value.contains("total")) throw std::invalid_argument("missing total");
    out.total = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullProgress>().total)>>::decode(value.at("total"));
    if (value.at("total").is_null()) throw std::invalid_argument("total must not be null");
    if (!value.contains("percent")) throw std::invalid_argument("missing percent");
    out.percent = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullProgress>().percent)>>::decode(value.at("percent"));
    if (value.at("percent").is_null()) throw std::invalid_argument("percent must not be null");
    if (!value.contains("stage")) throw std::invalid_argument("missing stage");
    out.stage = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullProgress>().stage)>>::decode(value.at("stage")), "stage", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("stage").is_null()) throw std::invalid_argument("stage must not be null");
    return out;
}

inline json codec<holder::api::RunnerPullStartResponseData>::encode(const holder::api::RunnerPullStartResponseData& value) {
    json result = json::object();
    result["job_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().job_id)>>::encode(value.job_id);
    if (value.runner_id) result["runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().runner_id)>>::encode(value.runner_id);
    if (value.model_ref) result["model_ref"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().model_ref)>>::encode(value.model_ref);
    result["model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().model)>>::encode(value.model);
    result["status"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().status)>>::encode(value.status);
    return result;
}
inline holder::api::RunnerPullStartResponseData codec<holder::api::RunnerPullStartResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullStartResponseData");
    holder::api::RunnerPullStartResponseData out{};
    if (!value.contains("job_id")) throw std::invalid_argument("missing job_id");
    out.job_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().job_id)>>::decode(value.at("job_id")), "job_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("job_id").is_null()) throw std::invalid_argument("job_id must not be null");
    if (value.contains("runner_id")) out.runner_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().runner_id)>>(value.at("runner_id"));
    if (value.contains("model_ref")) out.model_ref = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().model_ref)>>(value.at("model_ref"));
    if (!value.contains("model")) throw std::invalid_argument("missing model");
    out.model = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().model)>>::decode(value.at("model")), "model", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("model").is_null()) throw std::invalid_argument("model must not be null");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    out.status = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponseData>().status)>>::decode(value.at("status")), "status", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("status").is_null()) throw std::invalid_argument("status must not be null");
    return out;
}

inline json codec<holder::api::PostAiRunnerPullRequest>::encode(const holder::api::PostAiRunnerPullRequest& value) {
    json result = json::object();
    result["model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::PostAiRunnerPullRequest>().model)>>::encode(value.model);
    if (value.runner_id) result["runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::PostAiRunnerPullRequest>().runner_id)>>::encode(value.runner_id);
    return result;
}
inline holder::api::PostAiRunnerPullRequest codec<holder::api::PostAiRunnerPullRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected PostAiRunnerPullRequest");
    holder::api::PostAiRunnerPullRequest out{};
    if (!value.contains("model")) throw std::invalid_argument("missing model");
    out.model = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::PostAiRunnerPullRequest>().model)>>::decode(value.at("model")), "model", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("model").is_null()) throw std::invalid_argument("model must not be null");
    if (value.contains("runner_id")) out.runner_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::PostAiRunnerPullRequest>().runner_id)>>(value.at("runner_id"));
    return out;
}

inline json codec<holder::api::AiProviderCredential>::encode(const holder::api::AiProviderCredential& value) {
    json result = json::object();
    result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().provider)>>::encode(value.provider);
    result["configured"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().configured)>>::encode(value.configured);
    result["api_key_preview"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().api_key_preview)>>::encode(value.api_key_preview);
    result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().created_at)>>::encode(value.created_at);
    result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiProviderCredential codec<holder::api::AiProviderCredential>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredential");
    holder::api::AiProviderCredential out{};
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    out.provider = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().provider)>>::decode(value.at("provider")), "provider", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("provider").is_null()) throw std::invalid_argument("provider must not be null");
    if (!value.contains("configured")) throw std::invalid_argument("missing configured");
    out.configured = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().configured)>>::decode(value.at("configured"));
    if (value.at("configured").is_null()) throw std::invalid_argument("configured must not be null");
    if (!value.contains("api_key_preview")) throw std::invalid_argument("missing api_key_preview");
    out.api_key_preview = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().api_key_preview)>>::decode(value.at("api_key_preview")), "api_key_preview", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("api_key_preview").is_null()) throw std::invalid_argument("api_key_preview must not be null");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    out.created_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().created_at)>>::decode(value.at("created_at"));
    if (value.at("created_at").is_null()) throw std::invalid_argument("created_at must not be null");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    out.updated_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredential>().updated_at)>>::decode(value.at("updated_at"));
    if (value.at("updated_at").is_null()) throw std::invalid_argument("updated_at must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCredentialUpsertRequest>::encode(const holder::api::AiProviderCredentialUpsertRequest& value) {
    json result = json::object();
    result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertRequest>().provider)>>::encode(value.provider);
    result["api_key"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertRequest>().api_key)>>::encode(value.api_key);
    if (value.updated_at) result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertRequest>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiProviderCredentialUpsertRequest codec<holder::api::AiProviderCredentialUpsertRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialUpsertRequest");
    holder::api::AiProviderCredentialUpsertRequest out{};
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    out.provider = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertRequest>().provider)>>::decode(value.at("provider")), "provider", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("provider").is_null()) throw std::invalid_argument("provider must not be null");
    if (!value.contains("api_key")) throw std::invalid_argument("missing api_key");
    out.api_key = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertRequest>().api_key)>>::decode(value.at("api_key")), "api_key", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("api_key").is_null()) throw std::invalid_argument("api_key must not be null");
    if (value.contains("updated_at")) out.updated_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertRequest>().updated_at)>>(value.at("updated_at"));
    return out;
}

inline json codec<holder::api::AiProviderCredentialDeleteResponseData>::encode(const holder::api::AiProviderCredentialDeleteResponseData& value) {
    json result = json::object();
    result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialDeleteResponseData>().provider)>>::encode(value.provider);
    return result;
}
inline holder::api::AiProviderCredentialDeleteResponseData codec<holder::api::AiProviderCredentialDeleteResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialDeleteResponseData");
    holder::api::AiProviderCredentialDeleteResponseData out{};
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    out.provider = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialDeleteResponseData>().provider)>>::decode(value.at("provider")), "provider", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("provider").is_null()) throw std::invalid_argument("provider must not be null");
    return out;
}

inline json codec<holder::api::CardLinkDeleteRequest>::encode(const holder::api::CardLinkDeleteRequest& value) {
    json result = json::object();
    if (value.to_card_id) result["to_card_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CardLinkDeleteRequest>().to_card_id)>>::encode(value.to_card_id);
    if (value.target_project_id) result["target_project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CardLinkDeleteRequest>().target_project_id)>>::encode(value.target_project_id);
    if (value.to_type) result["to_type"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CardLinkDeleteRequest>().to_type)>>::encode(value.to_type);
    if (value.kind) result["kind"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CardLinkDeleteRequest>().kind)>>::encode(value.kind);
    return result;
}
inline holder::api::CardLinkDeleteRequest codec<holder::api::CardLinkDeleteRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLinkDeleteRequest");
    holder::api::CardLinkDeleteRequest out{};
    if (value.contains("to_card_id")) out.to_card_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::CardLinkDeleteRequest>().to_card_id)>>(value.at("to_card_id"));
    if (value.contains("target_project_id")) out.target_project_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::CardLinkDeleteRequest>().target_project_id)>>(value.at("target_project_id"));
    if (value.contains("to_type")) out.to_type = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::CardLinkDeleteRequest>().to_type)>>(value.at("to_type"));
    if (value.contains("kind")) out.kind = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::CardLinkDeleteRequest>().kind)>>(value.at("kind"));
    return out;
}

inline json codec<holder::api::AiLocalModelConfigData>::encode(const holder::api::AiLocalModelConfigData& value) {
    json result = json::object();
    result["fast_model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigData>().fast_model)>>::encode(value.fast_model);
    result["strong_model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigData>().strong_model)>>::encode(value.strong_model);
    result["deep_model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigData>().deep_model)>>::encode(value.deep_model);
    result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigData>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiLocalModelConfigData codec<holder::api::AiLocalModelConfigData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiLocalModelConfigData");
    holder::api::AiLocalModelConfigData out{};
    if (!value.contains("fast_model")) throw std::invalid_argument("missing fast_model");
    out.fast_model = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigData>().fast_model)>>::decode(value.at("fast_model"));
    if (!value.contains("strong_model")) throw std::invalid_argument("missing strong_model");
    out.strong_model = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigData>().strong_model)>>::decode(value.at("strong_model"));
    if (!value.contains("deep_model")) throw std::invalid_argument("missing deep_model");
    out.deep_model = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigData>().deep_model)>>::decode(value.at("deep_model"));
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    out.updated_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigData>().updated_at)>>::decode(value.at("updated_at"));
    return out;
}

inline json codec<holder::api::LocalModel>::encode(const holder::api::LocalModel& value) {
    json result = json::object();
    result["name"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().name)>>::encode(value.name);
    if (value.runner_id) result["runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().runner_id)>>::encode(value.runner_id);
    if (value.model_ref) result["model_ref"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().model_ref)>>::encode(value.model_ref);
    if (value.digest) result["digest"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().digest)>>::encode(value.digest);
    if (value.size) result["size"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().size)>>::encode(value.size);
    if (value.modified_at) result["modified_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().modified_at)>>::encode(value.modified_at);
    return result;
}
inline holder::api::LocalModel codec<holder::api::LocalModel>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected LocalModel");
    holder::api::LocalModel out{};
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    out.name = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().name)>>::decode(value.at("name")), "name", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("name").is_null()) throw std::invalid_argument("name must not be null");
    if (value.contains("runner_id")) out.runner_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().runner_id)>>(value.at("runner_id"));
    if (value.contains("model_ref")) out.model_ref = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().model_ref)>>(value.at("model_ref"));
    if (value.contains("digest")) out.digest = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().digest)>>(value.at("digest"));
    if (value.contains("size")) out.size = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().size)>>(value.at("size"));
    if (value.contains("modified_at")) out.modified_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::LocalModel>().modified_at)>>(value.at("modified_at"));
    return out;
}

inline json codec<holder::api::AiRunEventData>::encode(const holder::api::AiRunEventData& value) {
    json result = json::object();
    result["run_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().run_id)>>::encode(value.run_id);
    if (value.status) result["status"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().status)>>::encode(value.status);
    if (value.created_at) result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().created_at)>>::encode(value.created_at);
    if (value.delta) result["delta"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().delta)>>::encode(value.delta);
    if (value.message) result["message"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().message)>>::encode(value.message);
    if (value.error) result["error"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().error)>>::encode(value.error);
    if (value.runner_id) result["runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().runner_id)>>::encode(value.runner_id);
    if (value.model_ref) result["model_ref"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().model_ref)>>::encode(value.model_ref);
    if (value.model) result["model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().model)>>::encode(value.model);
    if (value.provider) result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().provider)>>::encode(value.provider);
    if (value.context_compacted) result["context_compacted"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().context_compacted)>>::encode(value.context_compacted);
    if (value.cooldown_until) result["cooldown_until"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().cooldown_until)>>::encode(value.cooldown_until);
    if (value.router_model) result["router_model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().router_model)>>::encode(value.router_model);
    if (value.ranked) result["ranked"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().ranked)>>::encode(value.ranked);
    if (value.partial) result["partial"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().partial)>>::encode(value.partial);
    return result;
}
inline holder::api::AiRunEventData codec<holder::api::AiRunEventData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunEventData");
    holder::api::AiRunEventData out{};
    if (!value.contains("run_id")) throw std::invalid_argument("missing run_id");
    out.run_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().run_id)>>::decode(value.at("run_id")), "run_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("run_id").is_null()) throw std::invalid_argument("run_id must not be null");
    if (value.contains("status")) out.status = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().status)>>(value.at("status"));
    if (value.contains("created_at")) out.created_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().created_at)>>(value.at("created_at"));
    if (value.contains("delta")) out.delta = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().delta)>>(value.at("delta"));
    if (value.contains("message")) out.message = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().message)>>(value.at("message"));
    if (value.contains("error")) out.error = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().error)>>(value.at("error"));
    if (value.contains("runner_id")) out.runner_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().runner_id)>>(value.at("runner_id"));
    if (value.contains("model_ref")) out.model_ref = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().model_ref)>>(value.at("model_ref"));
    if (value.contains("model")) out.model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().model)>>(value.at("model"));
    if (value.contains("provider")) out.provider = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().provider)>>(value.at("provider"));
    if (value.contains("context_compacted")) out.context_compacted = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().context_compacted)>>(value.at("context_compacted"));
    if (value.contains("cooldown_until")) out.cooldown_until = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().cooldown_until)>>(value.at("cooldown_until"));
    if (value.contains("router_model")) out.router_model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().router_model)>>(value.at("router_model"));
    if (value.contains("ranked")) out.ranked = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().ranked)>>(value.at("ranked"));
    if (value.contains("partial")) out.partial = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunEventData>().partial)>>(value.at("partial"));
    return out;
}

inline json codec<holder::api::AiRunCreateRequest>::encode(const holder::api::AiRunCreateRequest& value) {
    json result = json::object();
    result["prompt"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().prompt)>>::encode(value.prompt);
    if (value.mode) result["mode"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().mode)>>::encode(value.mode);
    if (value.model) result["model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().model)>>::encode(value.model);
    if (value.provider) result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().provider)>>::encode(value.provider);
    if (value.project_id) result["project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().project_id)>>::encode(value.project_id);
    if (value.thread_id) result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().thread_id)>>::encode(value.thread_id);
    if (value.context) result["context"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().context)>>::encode(value.context);
    if (value.runner_id) result["runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().runner_id)>>::encode(value.runner_id);
    return result;
}
inline holder::api::AiRunCreateRequest codec<holder::api::AiRunCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunCreateRequest");
    holder::api::AiRunCreateRequest out{};
    if (!value.contains("prompt")) throw std::invalid_argument("missing prompt");
    out.prompt = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().prompt)>>::decode(value.at("prompt")), "prompt", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("prompt").is_null()) throw std::invalid_argument("prompt must not be null");
    if (value.contains("mode")) out.mode = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().mode)>>(value.at("mode"));
    if (value.contains("model")) out.model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().model)>>(value.at("model"));
    if (value.contains("provider")) out.provider = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().provider)>>(value.at("provider"));
    if (value.contains("project_id")) out.project_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().project_id)>>(value.at("project_id"));
    if (value.contains("thread_id")) out.thread_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().thread_id)>>(value.at("thread_id"));
    if (value.contains("context") && !value.at("context").is_null() && !value.at("context").is_object()) throw std::invalid_argument("context must be an object");
    if (value.contains("context")) out.context = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().context)>>(value.at("context"));
    if (value.contains("runner_id")) out.runner_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunCreateRequest>().runner_id)>>(value.at("runner_id"));
    return out;
}

inline json codec<holder::api::AiThread>::encode(const holder::api::AiThread& value) {
    json result = json::object();
    result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().thread_id)>>::encode(value.thread_id);
    result["project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().project_id)>>::encode(value.project_id);
    if (value.card_id) result["card_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().card_id)>>::encode(value.card_id);
    result["title"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().title)>>::encode(value.title);
    result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().created_at)>>::encode(value.created_at);
    result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiThread codec<holder::api::AiThread>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThread");
    holder::api::AiThread out{};
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    out.thread_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().thread_id)>>::decode(value.at("thread_id")), "thread_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("thread_id").is_null()) throw std::invalid_argument("thread_id must not be null");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    out.project_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().project_id)>>::decode(value.at("project_id")), "project_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("project_id").is_null()) throw std::invalid_argument("project_id must not be null");
    if (value.contains("card_id")) out.card_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().card_id)>>(value.at("card_id"));
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    out.title = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().title)>>::decode(value.at("title")), "title", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("title").is_null()) throw std::invalid_argument("title must not be null");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    out.created_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().created_at)>>::decode(value.at("created_at"));
    if (value.at("created_at").is_null()) throw std::invalid_argument("created_at must not be null");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    out.updated_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThread>().updated_at)>>::decode(value.at("updated_at"));
    if (value.at("updated_at").is_null()) throw std::invalid_argument("updated_at must not be null");
    return out;
}

inline json codec<holder::api::AiLocalModelConfigUpdateRequest>::encode(const holder::api::AiLocalModelConfigUpdateRequest& value) {
    json result = json::object();
    if (value.fast_model) result["fast_model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigUpdateRequest>().fast_model)>>::encode(value.fast_model);
    if (value.strong_model) result["strong_model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigUpdateRequest>().strong_model)>>::encode(value.strong_model);
    if (value.deep_model) result["deep_model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigUpdateRequest>().deep_model)>>::encode(value.deep_model);
    if (value.updated_at) result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigUpdateRequest>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiLocalModelConfigUpdateRequest codec<holder::api::AiLocalModelConfigUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiLocalModelConfigUpdateRequest");
    holder::api::AiLocalModelConfigUpdateRequest out{};
    if (value.contains("fast_model")) out.fast_model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigUpdateRequest>().fast_model)>>(value.at("fast_model"));
    if (value.contains("strong_model")) out.strong_model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigUpdateRequest>().strong_model)>>(value.at("strong_model"));
    if (value.contains("deep_model")) out.deep_model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigUpdateRequest>().deep_model)>>(value.at("deep_model"));
    if (value.contains("updated_at")) out.updated_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigUpdateRequest>().updated_at)>>(value.at("updated_at"));
    return out;
}

inline json codec<holder::api::AiMessage>::encode(const holder::api::AiMessage& value) {
    json result = json::object();
    result["message_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().message_id)>>::encode(value.message_id);
    result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().thread_id)>>::encode(value.thread_id);
    result["role"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().role)>>::encode(value.role);
    result["source"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().source)>>::encode(value.source);
    result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().provider)>>::encode(value.provider);
    result["model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().model)>>::encode(value.model);
    result["content"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().content)>>::encode(value.content);
    result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().created_at)>>::encode(value.created_at);
    result["deleted_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().deleted_at)>>::encode(value.deleted_at);
    result["prompt_hash"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().prompt_hash)>>::encode(value.prompt_hash);
    result["meta_json"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().meta_json)>>::encode(value.meta_json);
    return result;
}
inline holder::api::AiMessage codec<holder::api::AiMessage>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessage");
    holder::api::AiMessage out{};
    if (!value.contains("message_id")) throw std::invalid_argument("missing message_id");
    out.message_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().message_id)>>::decode(value.at("message_id")), "message_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("message_id").is_null()) throw std::invalid_argument("message_id must not be null");
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    out.thread_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().thread_id)>>::decode(value.at("thread_id")), "thread_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("thread_id").is_null()) throw std::invalid_argument("thread_id must not be null");
    if (!value.contains("role")) throw std::invalid_argument("missing role");
    out.role = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().role)>>::decode(value.at("role")), "role", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("role").is_null()) throw std::invalid_argument("role must not be null");
    if (!value.contains("source")) throw std::invalid_argument("missing source");
    out.source = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().source)>>::decode(value.at("source")), "source", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("source").is_null()) throw std::invalid_argument("source must not be null");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    out.provider = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().provider)>>::decode(value.at("provider"));
    if (!value.contains("model")) throw std::invalid_argument("missing model");
    out.model = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().model)>>::decode(value.at("model"));
    if (!value.contains("content")) throw std::invalid_argument("missing content");
    out.content = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().content)>>::decode(value.at("content")), "content", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("content").is_null()) throw std::invalid_argument("content must not be null");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    out.created_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().created_at)>>::decode(value.at("created_at"));
    if (value.at("created_at").is_null()) throw std::invalid_argument("created_at must not be null");
    if (!value.contains("deleted_at")) throw std::invalid_argument("missing deleted_at");
    out.deleted_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().deleted_at)>>::decode(value.at("deleted_at"));
    if (!value.contains("prompt_hash")) throw std::invalid_argument("missing prompt_hash");
    out.prompt_hash = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().prompt_hash)>>::decode(value.at("prompt_hash"));
    if (!value.contains("meta_json")) throw std::invalid_argument("missing meta_json");
    out.meta_json = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessage>().meta_json)>>::decode(value.at("meta_json"));
    return out;
}

inline json codec<holder::api::AiMessageCreateResponseData>::encode(const holder::api::AiMessageCreateResponseData& value) {
    json result = json::object();
    result["message_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateResponseData>().message_id)>>::encode(value.message_id);
    return result;
}
inline holder::api::AiMessageCreateResponseData codec<holder::api::AiMessageCreateResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCreateResponseData");
    holder::api::AiMessageCreateResponseData out{};
    if (!value.contains("message_id")) throw std::invalid_argument("missing message_id");
    out.message_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateResponseData>().message_id)>>::decode(value.at("message_id")), "message_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("message_id").is_null()) throw std::invalid_argument("message_id must not be null");
    return out;
}

inline json codec<holder::api::AiMessageCreateRequest>::encode(const holder::api::AiMessageCreateRequest& value) {
    json result = json::object();
    if (value.message_id) result["message_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().message_id)>>::encode(value.message_id);
    result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().thread_id)>>::encode(value.thread_id);
    result["role"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().role)>>::encode(value.role);
    result["source"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().source)>>::encode(value.source);
    if (value.provider) result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().provider)>>::encode(value.provider);
    if (value.model) result["model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().model)>>::encode(value.model);
    result["content"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().content)>>::encode(value.content);
    if (value.created_at) result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().created_at)>>::encode(value.created_at);
    if (value.prompt_hash) result["prompt_hash"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().prompt_hash)>>::encode(value.prompt_hash);
    if (value.meta_json) result["meta_json"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().meta_json)>>::encode(value.meta_json);
    return result;
}
inline holder::api::AiMessageCreateRequest codec<holder::api::AiMessageCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCreateRequest");
    holder::api::AiMessageCreateRequest out{};
    if (value.contains("message_id")) out.message_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().message_id)>>(value.at("message_id"));
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    out.thread_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().thread_id)>>::decode(value.at("thread_id")), "thread_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("thread_id").is_null()) throw std::invalid_argument("thread_id must not be null");
    if (!value.contains("role")) throw std::invalid_argument("missing role");
    out.role = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().role)>>::decode(value.at("role")), "role", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("role").is_null()) throw std::invalid_argument("role must not be null");
    if (!value.contains("source")) throw std::invalid_argument("missing source");
    out.source = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().source)>>::decode(value.at("source")), "source", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("source").is_null()) throw std::invalid_argument("source must not be null");
    if (value.contains("provider")) out.provider = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().provider)>>(value.at("provider"));
    if (value.contains("model")) out.model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().model)>>(value.at("model"));
    if (!value.contains("content")) throw std::invalid_argument("missing content");
    out.content = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().content)>>::decode(value.at("content")), "content", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("content").is_null()) throw std::invalid_argument("content must not be null");
    if (value.contains("created_at")) out.created_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().created_at)>>(value.at("created_at"));
    if (value.contains("prompt_hash")) out.prompt_hash = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().prompt_hash)>>(value.at("prompt_hash"));
    if (value.contains("meta_json")) out.meta_json = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateRequest>().meta_json)>>(value.at("meta_json"));
    return out;
}

inline json codec<holder::api::AiMessageUpdateResponseData>::encode(const holder::api::AiMessageUpdateResponseData& value) {
    json result = json::object();
    result["message_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateResponseData>().message_id)>>::encode(value.message_id);
    return result;
}
inline holder::api::AiMessageUpdateResponseData codec<holder::api::AiMessageUpdateResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageUpdateResponseData");
    holder::api::AiMessageUpdateResponseData out{};
    if (!value.contains("message_id")) throw std::invalid_argument("missing message_id");
    out.message_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateResponseData>().message_id)>>::decode(value.at("message_id")), "message_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("message_id").is_null()) throw std::invalid_argument("message_id must not be null");
    return out;
}

inline json codec<holder::api::AiMessageUpdateRequest>::encode(const holder::api::AiMessageUpdateRequest& value) {
    json result = json::object();
    if (value.role) result["role"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().role)>>::encode(value.role);
    if (value.source) result["source"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().source)>>::encode(value.source);
    if (value.provider) result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().provider)>>::encode(value.provider);
    if (value.model) result["model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().model)>>::encode(value.model);
    if (value.content) result["content"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().content)>>::encode(value.content);
    if (value.created_at) result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().created_at)>>::encode(value.created_at);
    if (value.prompt_hash) result["prompt_hash"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().prompt_hash)>>::encode(value.prompt_hash);
    if (value.meta_json) result["meta_json"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().meta_json)>>::encode(value.meta_json);
    return result;
}
inline holder::api::AiMessageUpdateRequest codec<holder::api::AiMessageUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageUpdateRequest");
    holder::api::AiMessageUpdateRequest out{};
    if (value.contains("role")) out.role = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().role)>>(value.at("role"));
    if (value.contains("source")) out.source = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().source)>>(value.at("source"));
    if (value.contains("provider")) out.provider = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().provider)>>(value.at("provider"));
    if (value.contains("model")) out.model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().model)>>(value.at("model"));
    if (value.contains("content")) out.content = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().content)>>(value.at("content"));
    if (value.contains("created_at")) out.created_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().created_at)>>(value.at("created_at"));
    if (value.contains("prompt_hash")) out.prompt_hash = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().prompt_hash)>>(value.at("prompt_hash"));
    if (value.contains("meta_json")) out.meta_json = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateRequest>().meta_json)>>(value.at("meta_json"));
    return out;
}

inline json codec<holder::api::AiMessageCaptureResponseData>::encode(const holder::api::AiMessageCaptureResponseData& value) {
    json result = json::object();
    result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponseData>().thread_id)>>::encode(value.thread_id);
    result["user_message_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponseData>().user_message_id)>>::encode(value.user_message_id);
    result["assistant_message_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponseData>().assistant_message_id)>>::encode(value.assistant_message_id);
    return result;
}
inline holder::api::AiMessageCaptureResponseData codec<holder::api::AiMessageCaptureResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCaptureResponseData");
    holder::api::AiMessageCaptureResponseData out{};
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    out.thread_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponseData>().thread_id)>>::decode(value.at("thread_id")), "thread_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("thread_id").is_null()) throw std::invalid_argument("thread_id must not be null");
    if (!value.contains("user_message_id")) throw std::invalid_argument("missing user_message_id");
    out.user_message_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponseData>().user_message_id)>>::decode(value.at("user_message_id")), "user_message_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("user_message_id").is_null()) throw std::invalid_argument("user_message_id must not be null");
    if (!value.contains("assistant_message_id")) throw std::invalid_argument("missing assistant_message_id");
    out.assistant_message_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponseData>().assistant_message_id)>>::decode(value.at("assistant_message_id")), "assistant_message_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("assistant_message_id").is_null()) throw std::invalid_argument("assistant_message_id must not be null");
    return out;
}

inline json codec<holder::api::AiMessageCaptureRequest>::encode(const holder::api::AiMessageCaptureRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().project_id)>>::encode(value.project_id);
    if (value.thread_id) result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().thread_id)>>::encode(value.thread_id);
    result["prompt"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().prompt)>>::encode(value.prompt);
    result["response"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().response)>>::encode(value.response);
    if (value.source) result["source"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().source)>>::encode(value.source);
    if (value.provider) result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().provider)>>::encode(value.provider);
    if (value.model) result["model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().model)>>::encode(value.model);
    if (value.url) result["url"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().url)>>::encode(value.url);
    if (value.context) result["context"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().context)>>::encode(value.context);
    if (value.created_at) result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().created_at)>>::encode(value.created_at);
    return result;
}
inline holder::api::AiMessageCaptureRequest codec<holder::api::AiMessageCaptureRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCaptureRequest");
    holder::api::AiMessageCaptureRequest out{};
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    out.project_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().project_id)>>::decode(value.at("project_id")), "project_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("project_id").is_null()) throw std::invalid_argument("project_id must not be null");
    if (value.contains("thread_id")) out.thread_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().thread_id)>>(value.at("thread_id"));
    if (!value.contains("prompt")) throw std::invalid_argument("missing prompt");
    out.prompt = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().prompt)>>::decode(value.at("prompt")), "prompt", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("prompt").is_null()) throw std::invalid_argument("prompt must not be null");
    if (!value.contains("response")) throw std::invalid_argument("missing response");
    out.response = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().response)>>::decode(value.at("response")), "response", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("response").is_null()) throw std::invalid_argument("response must not be null");
    if (value.contains("source")) out.source = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().source)>>(value.at("source"));
    if (value.contains("provider")) out.provider = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().provider)>>(value.at("provider"));
    if (value.contains("model")) out.model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().model)>>(value.at("model"));
    if (value.contains("url")) out.url = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().url)>>(value.at("url"));
    if (value.contains("context") && !value.at("context").is_null() && !value.at("context").is_object()) throw std::invalid_argument("context must be an object");
    if (value.contains("context")) out.context = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().context)>>(value.at("context"));
    if (value.contains("created_at")) out.created_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureRequest>().created_at)>>(value.at("created_at"));
    return out;
}

inline json codec<holder::api::AiNudge>::encode(const holder::api::AiNudge& value) {
    json result = json::object();
    result["nudge_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().nudge_id)>>::encode(value.nudge_id);
    result["kind"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().kind)>>::encode(value.kind);
    result["project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().project_id)>>::encode(value.project_id);
    result["card_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().card_id)>>::encode(value.card_id);
    result["title"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().title)>>::encode(value.title);
    result["body"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().body)>>::encode(value.body);
    result["meta_json"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().meta_json)>>::encode(value.meta_json);
    result["basis_fingerprint"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().basis_fingerprint)>>::encode(value.basis_fingerprint);
    result["basis_commit"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().basis_commit)>>::encode(value.basis_commit);
    result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().created_at)>>::encode(value.created_at);
    if (value.suggestions) result["suggestions"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().suggestions)>>::encode(value.suggestions);
    return result;
}
inline holder::api::AiNudge codec<holder::api::AiNudge>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudge");
    holder::api::AiNudge out{};
    if (!value.contains("nudge_id")) throw std::invalid_argument("missing nudge_id");
    out.nudge_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().nudge_id)>>::decode(value.at("nudge_id")), "nudge_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("nudge_id").is_null()) throw std::invalid_argument("nudge_id must not be null");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    out.kind = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().kind)>>::decode(value.at("kind")), "kind", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("kind").is_null()) throw std::invalid_argument("kind must not be null");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    out.project_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().project_id)>>::decode(value.at("project_id")), "project_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("project_id").is_null()) throw std::invalid_argument("project_id must not be null");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    out.card_id = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().card_id)>>::decode(value.at("card_id"));
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    out.title = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().title)>>::decode(value.at("title")), "title", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("title").is_null()) throw std::invalid_argument("title must not be null");
    if (!value.contains("body")) throw std::invalid_argument("missing body");
    out.body = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().body)>>::decode(value.at("body")), "body", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("body").is_null()) throw std::invalid_argument("body must not be null");
    if (value.contains("meta_json") && !value.at("meta_json").is_null() && !value.at("meta_json").is_object()) throw std::invalid_argument("meta_json must be an object");
    if (!value.contains("meta_json")) throw std::invalid_argument("missing meta_json");
    out.meta_json = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().meta_json)>>::decode(value.at("meta_json"));
    if (value.at("meta_json").is_null()) throw std::invalid_argument("meta_json must not be null");
    if (!value.contains("basis_fingerprint")) throw std::invalid_argument("missing basis_fingerprint");
    out.basis_fingerprint = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().basis_fingerprint)>>::decode(value.at("basis_fingerprint"));
    if (!value.contains("basis_commit")) throw std::invalid_argument("missing basis_commit");
    out.basis_commit = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().basis_commit)>>::decode(value.at("basis_commit"));
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    out.created_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().created_at)>>::decode(value.at("created_at"));
    if (value.at("created_at").is_null()) throw std::invalid_argument("created_at must not be null");
    if (value.contains("suggestions")) out.suggestions = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudge>().suggestions)>>(value.at("suggestions"));
    return out;
}

inline json codec<holder::api::AiNudgeDismissData>::encode(const holder::api::AiNudgeDismissData& value) {
    json result = json::object();
    result["nudge_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeDismissData>().nudge_id)>>::encode(value.nudge_id);
    result["dismissed"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeDismissData>().dismissed)>>::encode(value.dismissed);
    return result;
}
inline holder::api::AiNudgeDismissData codec<holder::api::AiNudgeDismissData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeDismissData");
    holder::api::AiNudgeDismissData out{};
    if (!value.contains("nudge_id")) throw std::invalid_argument("missing nudge_id");
    out.nudge_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeDismissData>().nudge_id)>>::decode(value.at("nudge_id")), "nudge_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("nudge_id").is_null()) throw std::invalid_argument("nudge_id must not be null");
    if (!value.contains("dismissed")) throw std::invalid_argument("missing dismissed");
    out.dismissed = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeDismissData>().dismissed)>>::decode(value.at("dismissed"));
    if (value.at("dismissed").is_null()) throw std::invalid_argument("dismissed must not be null");
    return out;
}

inline json codec<holder::api::AiNudgeCandidate>::encode(const holder::api::AiNudgeCandidate& value) {
    json result = json::object();
    result["kind"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().kind)>>::encode(value.kind);
    result["project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().project_id)>>::encode(value.project_id);
    if (value.card_id) result["card_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().card_id)>>::encode(value.card_id);
    result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().created_at)>>::encode(value.created_at);
    if (value.basis_fingerprint) result["basis_fingerprint"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().basis_fingerprint)>>::encode(value.basis_fingerprint);
    if (value.basis_commit) result["basis_commit"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().basis_commit)>>::encode(value.basis_commit);
    result["facts"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().facts)>>::encode(value.facts);
    return result;
}
inline holder::api::AiNudgeCandidate codec<holder::api::AiNudgeCandidate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeCandidate");
    holder::api::AiNudgeCandidate out{};
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    out.kind = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().kind)>>::decode(value.at("kind")), "kind", 1, std::numeric_limits<std::size_t>::max());
    if (value.at("kind").is_null()) throw std::invalid_argument("kind must not be null");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    out.project_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().project_id)>>::decode(value.at("project_id")), "project_id", 1, std::numeric_limits<std::size_t>::max());
    if (value.at("project_id").is_null()) throw std::invalid_argument("project_id must not be null");
    if (value.contains("card_id")) out.card_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().card_id)>>(value.at("card_id"));
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    out.created_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().created_at)>>::decode(value.at("created_at"));
    if (value.at("created_at").is_null()) throw std::invalid_argument("created_at must not be null");
    if (value.contains("basis_fingerprint")) out.basis_fingerprint = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().basis_fingerprint)>>(value.at("basis_fingerprint"));
    if (value.contains("basis_commit")) out.basis_commit = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().basis_commit)>>(value.at("basis_commit"));
    if (value.contains("facts") && !value.at("facts").is_null() && !value.at("facts").is_object()) throw std::invalid_argument("facts must be an object");
    if (!value.contains("facts")) throw std::invalid_argument("missing facts");
    out.facts = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeCandidate>().facts)>>::decode(value.at("facts"));
    if (value.at("facts").is_null()) throw std::invalid_argument("facts must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCatalogModelLimits>::encode(const holder::api::AiProviderCatalogModelLimits& value) {
    json result = json::object();
    result["rpm"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModelLimits>().rpm)>>::encode(value.rpm);
    result["tpm"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModelLimits>().tpm)>>::encode(value.tpm);
    result["rpd"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModelLimits>().rpd)>>::encode(value.rpd);
    return result;
}
inline holder::api::AiProviderCatalogModelLimits codec<holder::api::AiProviderCatalogModelLimits>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogModelLimits");
    holder::api::AiProviderCatalogModelLimits out{};
    if (!value.contains("rpm")) throw std::invalid_argument("missing rpm");
    out.rpm = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModelLimits>().rpm)>>::decode(value.at("rpm"));
    if (value.at("rpm").is_null()) throw std::invalid_argument("rpm must not be null");
    if (!value.contains("tpm")) throw std::invalid_argument("missing tpm");
    out.tpm = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModelLimits>().tpm)>>::decode(value.at("tpm"));
    if (value.at("tpm").is_null()) throw std::invalid_argument("tpm must not be null");
    if (!value.contains("rpd")) throw std::invalid_argument("missing rpd");
    out.rpd = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModelLimits>().rpd)>>::decode(value.at("rpd"));
    if (value.at("rpd").is_null()) throw std::invalid_argument("rpd must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCatalogApi>::encode(const holder::api::AiProviderCatalogApi& value) {
    json result = json::object();
    if (value.base_url) result["base_url"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogApi>().base_url)>>::encode(value.base_url);
    if (value.kind) result["kind"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogApi>().kind)>>::encode(value.kind);
    return result;
}
inline holder::api::AiProviderCatalogApi codec<holder::api::AiProviderCatalogApi>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogApi");
    holder::api::AiProviderCatalogApi out{};
    if (value.contains("base_url")) out.base_url = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogApi>().base_url)>>(value.at("base_url"));
    if (value.contains("kind")) out.kind = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogApi>().kind)>>(value.at("kind"));
    return out;
}

inline json codec<holder::api::AiProviderCatalogAuth>::encode(const holder::api::AiProviderCatalogAuth& value) {
    json result = json::object();
    if (value.type) result["type"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogAuth>().type)>>::encode(value.type);
    if (value.key_param) result["key_param"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogAuth>().key_param)>>::encode(value.key_param);
    if (value.header_name) result["header_name"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogAuth>().header_name)>>::encode(value.header_name);
    if (value.bearer_prefix) result["bearer_prefix"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogAuth>().bearer_prefix)>>::encode(value.bearer_prefix);
    return result;
}
inline holder::api::AiProviderCatalogAuth codec<holder::api::AiProviderCatalogAuth>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogAuth");
    holder::api::AiProviderCatalogAuth out{};
    if (value.contains("type")) out.type = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogAuth>().type)>>(value.at("type"));
    if (value.contains("key_param")) out.key_param = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogAuth>().key_param)>>(value.at("key_param"));
    if (value.contains("header_name")) out.header_name = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogAuth>().header_name)>>(value.at("header_name"));
    if (value.contains("bearer_prefix")) out.bearer_prefix = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogAuth>().bearer_prefix)>>(value.at("bearer_prefix"));
    return out;
}

inline json codec<holder::api::AiRunnerCreateRequest>::encode(const holder::api::AiRunnerCreateRequest& value) {
    json result = json::object();
    result["name"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerCreateRequest>().name)>>::encode(value.name);
    result["kind"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerCreateRequest>().kind)>>::encode(value.kind);
    result["base_url"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerCreateRequest>().base_url)>>::encode(value.base_url);
    if (value.enabled) result["enabled"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerCreateRequest>().enabled)>>::encode(value.enabled);
    return result;
}
inline holder::api::AiRunnerCreateRequest codec<holder::api::AiRunnerCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerCreateRequest");
    holder::api::AiRunnerCreateRequest out{};
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    out.name = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerCreateRequest>().name)>>::decode(value.at("name")), "name", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("name").is_null()) throw std::invalid_argument("name must not be null");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    out.kind = [&] { auto result = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerCreateRequest>().kind)>>::decode(value.at("kind")); if (result != "ollama") throw std::invalid_argument("invalid kind"); return result; }();
    if (value.at("kind").is_null()) throw std::invalid_argument("kind must not be null");
    if (!value.contains("base_url")) throw std::invalid_argument("missing base_url");
    out.base_url = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerCreateRequest>().base_url)>>::decode(value.at("base_url")), "base_url", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("base_url").is_null()) throw std::invalid_argument("base_url must not be null");
    if (value.contains("enabled")) out.enabled = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerCreateRequest>().enabled)>>(value.at("enabled"));
    return out;
}

inline json codec<holder::api::AiRunnerUpdateRequest>::encode(const holder::api::AiRunnerUpdateRequest& value) {
    json result = json::object();
    if (value.name) result["name"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerUpdateRequest>().name)>>::encode(value.name);
    if (value.base_url) result["base_url"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerUpdateRequest>().base_url)>>::encode(value.base_url);
    if (value.enabled) result["enabled"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerUpdateRequest>().enabled)>>::encode(value.enabled);
    return result;
}
inline holder::api::AiRunnerUpdateRequest codec<holder::api::AiRunnerUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerUpdateRequest");
    holder::api::AiRunnerUpdateRequest out{};
    if (value.contains("name")) out.name = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerUpdateRequest>().name)>>(value.at("name"));
    if (value.contains("base_url")) out.base_url = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerUpdateRequest>().base_url)>>(value.at("base_url"));
    if (value.contains("enabled")) out.enabled = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerUpdateRequest>().enabled)>>(value.at("enabled"));
    return out;
}

inline json codec<holder::api::AiRunnerDeleteResponseData>::encode(const holder::api::AiRunnerDeleteResponseData& value) {
    json result = json::object();
    result["runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerDeleteResponseData>().runner_id)>>::encode(value.runner_id);
    return result;
}
inline holder::api::AiRunnerDeleteResponseData codec<holder::api::AiRunnerDeleteResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerDeleteResponseData");
    holder::api::AiRunnerDeleteResponseData out{};
    if (!value.contains("runner_id")) throw std::invalid_argument("missing runner_id");
    out.runner_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerDeleteResponseData>().runner_id)>>::decode(value.at("runner_id")), "runner_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("runner_id").is_null()) throw std::invalid_argument("runner_id must not be null");
    return out;
}

inline json codec<holder::api::AiRunItem>::encode(const holder::api::AiRunItem& value) {
    json result = json::object();
    result["run_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().run_id)>>::encode(value.run_id);
    if (value.project_id) result["project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().project_id)>>::encode(value.project_id);
    if (value.thread_id) result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().thread_id)>>::encode(value.thread_id);
    if (value.message_id) result["message_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().message_id)>>::encode(value.message_id);
    result["mode"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().mode)>>::encode(value.mode);
    result["prompt"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().prompt)>>::encode(value.prompt);
    if (value.context_json) result["context_json"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().context_json)>>::encode(value.context_json);
    if (value.router_model) result["router_model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().router_model)>>::encode(value.router_model);
    if (value.router_runner_id) result["router_runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().router_runner_id)>>::encode(value.router_runner_id);
    if (value.ranked_json) result["ranked_json"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().ranked_json)>>::encode(value.ranked_json);
    if (value.policy_trace_json) result["policy_trace_json"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().policy_trace_json)>>::encode(value.policy_trace_json);
    if (value.policy_trace) result["policy_trace"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().policy_trace)>>::encode(value.policy_trace);
    if (value.chosen_model) result["chosen_model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().chosen_model)>>::encode(value.chosen_model);
    if (value.chosen_runner_id) result["chosen_runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().chosen_runner_id)>>::encode(value.chosen_runner_id);
    result["status"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().status)>>::encode(value.status);
    if (value.error) result["error"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().error)>>::encode(value.error);
    result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().created_at)>>::encode(value.created_at);
    result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiRunItem codec<holder::api::AiRunItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunItem");
    holder::api::AiRunItem out{};
    if (!value.contains("run_id")) throw std::invalid_argument("missing run_id");
    out.run_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().run_id)>>::decode(value.at("run_id")), "run_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("run_id").is_null()) throw std::invalid_argument("run_id must not be null");
    if (value.contains("project_id")) out.project_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().project_id)>>(value.at("project_id"));
    if (value.contains("thread_id")) out.thread_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().thread_id)>>(value.at("thread_id"));
    if (value.contains("message_id")) out.message_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().message_id)>>(value.at("message_id"));
    if (!value.contains("mode")) throw std::invalid_argument("missing mode");
    out.mode = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().mode)>>::decode(value.at("mode")), "mode", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("mode").is_null()) throw std::invalid_argument("mode must not be null");
    if (!value.contains("prompt")) throw std::invalid_argument("missing prompt");
    out.prompt = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().prompt)>>::decode(value.at("prompt")), "prompt", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("prompt").is_null()) throw std::invalid_argument("prompt must not be null");
    if (value.contains("context_json")) out.context_json = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().context_json)>>(value.at("context_json"));
    if (value.contains("router_model")) out.router_model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().router_model)>>(value.at("router_model"));
    if (value.contains("router_runner_id")) out.router_runner_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().router_runner_id)>>(value.at("router_runner_id"));
    if (value.contains("ranked_json")) out.ranked_json = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().ranked_json)>>(value.at("ranked_json"));
    if (value.contains("policy_trace_json")) out.policy_trace_json = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().policy_trace_json)>>(value.at("policy_trace_json"));
    if (value.contains("policy_trace") && !value.at("policy_trace").is_null() && !value.at("policy_trace").is_object()) throw std::invalid_argument("policy_trace must be an object");
    if (value.contains("policy_trace")) out.policy_trace = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().policy_trace)>>(value.at("policy_trace"));
    if (value.contains("chosen_model")) out.chosen_model = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().chosen_model)>>(value.at("chosen_model"));
    if (value.contains("chosen_runner_id")) out.chosen_runner_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().chosen_runner_id)>>(value.at("chosen_runner_id"));
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    out.status = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().status)>>::decode(value.at("status")), "status", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("status").is_null()) throw std::invalid_argument("status must not be null");
    if (value.contains("error")) out.error = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().error)>>(value.at("error"));
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    out.created_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().created_at)>>::decode(value.at("created_at"));
    if (value.at("created_at").is_null()) throw std::invalid_argument("created_at must not be null");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    out.updated_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunItem>().updated_at)>>::decode(value.at("updated_at"));
    if (value.at("updated_at").is_null()) throw std::invalid_argument("updated_at must not be null");
    return out;
}

inline json codec<holder::api::CasteInfo>::encode(const holder::api::CasteInfo& value) {
    json result = json::object();
    result["name"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteInfo>().name)>>::encode(value.name);
    result["reason"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteInfo>().reason)>>::encode(value.reason);
    return result;
}
inline holder::api::CasteInfo codec<holder::api::CasteInfo>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CasteInfo");
    holder::api::CasteInfo out{};
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    out.name = [&] { auto result = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteInfo>().name)>>::decode(value.at("name")); if (result != "mini" && result != "user" && result != "developer" && result != "workstation" && result != "rig") throw std::invalid_argument("invalid name"); return result; }();
    if (value.at("name").is_null()) throw std::invalid_argument("name must not be null");
    if (!value.contains("reason")) throw std::invalid_argument("missing reason");
    out.reason = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteInfo>().reason)>>::decode(value.at("reason"));
    return out;
}

inline json codec<holder::api::CasteRecommendedModel>::encode(const holder::api::CasteRecommendedModel& value) {
    json result = json::object();
    result["tag"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().tag)>>::encode(value.tag);
    result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().provider)>>::encode(value.provider);
    result["engine"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().engine)>>::encode(value.engine);
    result["category"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().category)>>::encode(value.category);
    result["required_caste"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().required_caste)>>::encode(value.required_caste);
    result["installed"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().installed)>>::encode(value.installed);
    return result;
}
inline holder::api::CasteRecommendedModel codec<holder::api::CasteRecommendedModel>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CasteRecommendedModel");
    holder::api::CasteRecommendedModel out{};
    if (!value.contains("tag")) throw std::invalid_argument("missing tag");
    out.tag = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().tag)>>::decode(value.at("tag")), "tag", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("tag").is_null()) throw std::invalid_argument("tag must not be null");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    out.provider = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().provider)>>::decode(value.at("provider"));
    if (!value.contains("engine")) throw std::invalid_argument("missing engine");
    out.engine = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().engine)>>::decode(value.at("engine"));
    if (!value.contains("category")) throw std::invalid_argument("missing category");
    out.category = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().category)>>::decode(value.at("category"));
    if (!value.contains("required_caste")) throw std::invalid_argument("missing required_caste");
    out.required_caste = [&] { auto result = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().required_caste)>>::decode(value.at("required_caste")); if (result != "mini" && result != "user" && result != "developer" && result != "workstation" && result != "rig") throw std::invalid_argument("invalid required_caste"); return result; }();
    if (value.at("required_caste").is_null()) throw std::invalid_argument("required_caste must not be null");
    if (!value.contains("installed")) throw std::invalid_argument("missing installed");
    out.installed = codec<std::remove_cvref_t<decltype(std::declval<holder::api::CasteRecommendedModel>().installed)>>::decode(value.at("installed"));
    if (value.at("installed").is_null()) throw std::invalid_argument("installed must not be null");
    return out;
}

inline json codec<holder::api::AiThreadCreateResponseData>::encode(const holder::api::AiThreadCreateResponseData& value) {
    json result = json::object();
    result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateResponseData>().thread_id)>>::encode(value.thread_id);
    return result;
}
inline holder::api::AiThreadCreateResponseData codec<holder::api::AiThreadCreateResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadCreateResponseData");
    holder::api::AiThreadCreateResponseData out{};
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    out.thread_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateResponseData>().thread_id)>>::decode(value.at("thread_id")), "thread_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("thread_id").is_null()) throw std::invalid_argument("thread_id must not be null");
    return out;
}

inline json codec<holder::api::AiThreadCreateRequest>::encode(const holder::api::AiThreadCreateRequest& value) {
    json result = json::object();
    if (value.thread_id) result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().thread_id)>>::encode(value.thread_id);
    result["project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().project_id)>>::encode(value.project_id);
    result["title"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().title)>>::encode(value.title);
    if (value.card_id) result["card_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().card_id)>>::encode(value.card_id);
    if (value.created_at) result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().created_at)>>::encode(value.created_at);
    if (value.updated_at) result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiThreadCreateRequest codec<holder::api::AiThreadCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadCreateRequest");
    holder::api::AiThreadCreateRequest out{};
    if (value.contains("thread_id")) out.thread_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().thread_id)>>(value.at("thread_id"));
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    out.project_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().project_id)>>::decode(value.at("project_id")), "project_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("project_id").is_null()) throw std::invalid_argument("project_id must not be null");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    out.title = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().title)>>::decode(value.at("title")), "title", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("title").is_null()) throw std::invalid_argument("title must not be null");
    if (value.contains("card_id")) out.card_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().card_id)>>(value.at("card_id"));
    if (value.contains("created_at")) out.created_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().created_at)>>(value.at("created_at"));
    if (value.contains("updated_at")) out.updated_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateRequest>().updated_at)>>(value.at("updated_at"));
    return out;
}

inline json codec<holder::api::AiThreadPatchResponseData>::encode(const holder::api::AiThreadPatchResponseData& value) {
    json result = json::object();
    result["thread_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadPatchResponseData>().thread_id)>>::encode(value.thread_id);
    return result;
}
inline holder::api::AiThreadPatchResponseData codec<holder::api::AiThreadPatchResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadPatchResponseData");
    holder::api::AiThreadPatchResponseData out{};
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    out.thread_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadPatchResponseData>().thread_id)>>::decode(value.at("thread_id")), "thread_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("thread_id").is_null()) throw std::invalid_argument("thread_id must not be null");
    return out;
}

inline json codec<holder::api::AiThreadUpdateRequest>::encode(const holder::api::AiThreadUpdateRequest& value) {
    json result = json::object();
    if (value.title) result["title"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadUpdateRequest>().title)>>::encode(value.title);
    if (value.card_id) result["card_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadUpdateRequest>().card_id)>>::encode(value.card_id);
    result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadUpdateRequest>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiThreadUpdateRequest codec<holder::api::AiThreadUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadUpdateRequest");
    holder::api::AiThreadUpdateRequest out{};
    if (value.contains("title")) out.title = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadUpdateRequest>().title)>>(value.at("title"));
    if (value.contains("card_id")) out.card_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadUpdateRequest>().card_id)>>(value.at("card_id"));
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    out.updated_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadUpdateRequest>().updated_at)>>::decode(value.at("updated_at"));
    if (value.at("updated_at").is_null()) throw std::invalid_argument("updated_at must not be null");
    return out;
}

inline json codec<holder::api::AiProviderSetting>::encode(const holder::api::AiProviderSetting& value) {
    json result = json::object();
    result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSetting>().provider)>>::encode(value.provider);
    result["enabled"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSetting>().enabled)>>::encode(value.enabled);
    result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSetting>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiProviderSetting codec<holder::api::AiProviderSetting>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSetting");
    holder::api::AiProviderSetting out{};
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    out.provider = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSetting>().provider)>>::decode(value.at("provider")), "provider", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("provider").is_null()) throw std::invalid_argument("provider must not be null");
    if (!value.contains("enabled")) throw std::invalid_argument("missing enabled");
    out.enabled = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSetting>().enabled)>>::decode(value.at("enabled"));
    if (value.at("enabled").is_null()) throw std::invalid_argument("enabled must not be null");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    out.updated_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSetting>().updated_at)>>::decode(value.at("updated_at"));
    if (value.at("updated_at").is_null()) throw std::invalid_argument("updated_at must not be null");
    return out;
}

inline json codec<holder::api::AiProviderSettingUpdateRequest>::encode(const holder::api::AiProviderSettingUpdateRequest& value) {
    json result = json::object();
    result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingUpdateRequest>().provider)>>::encode(value.provider);
    result["enabled"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingUpdateRequest>().enabled)>>::encode(value.enabled);
    if (value.updated_at) result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingUpdateRequest>().updated_at)>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiProviderSettingUpdateRequest codec<holder::api::AiProviderSettingUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingUpdateRequest");
    holder::api::AiProviderSettingUpdateRequest out{};
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    out.provider = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingUpdateRequest>().provider)>>::decode(value.at("provider")), "provider", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("provider").is_null()) throw std::invalid_argument("provider must not be null");
    if (!value.contains("enabled")) throw std::invalid_argument("missing enabled");
    out.enabled = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingUpdateRequest>().enabled)>>::decode(value.at("enabled"));
    if (value.at("enabled").is_null()) throw std::invalid_argument("enabled must not be null");
    if (value.contains("updated_at")) out.updated_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingUpdateRequest>().updated_at)>>(value.at("updated_at"));
    return out;
}

inline json codec<holder::api::AiProviderSettingDeleteData>::encode(const holder::api::AiProviderSettingDeleteData& value) {
    json result = json::object();
    result["provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingDeleteData>().provider)>>::encode(value.provider);
    return result;
}
inline holder::api::AiProviderSettingDeleteData codec<holder::api::AiProviderSettingDeleteData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingDeleteData");
    holder::api::AiProviderSettingDeleteData out{};
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    out.provider = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingDeleteData>().provider)>>::decode(value.at("provider")), "provider", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("provider").is_null()) throw std::invalid_argument("provider must not be null");
    return out;
}

inline json codec<holder::api::AiStreamResyncData>::encode(const holder::api::AiStreamResyncData& value) {
    json result = json::object();
    result["reason"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStreamResyncData>().reason)>>::encode(value.reason);
    return result;
}
inline holder::api::AiStreamResyncData codec<holder::api::AiStreamResyncData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiStreamResyncData");
    holder::api::AiStreamResyncData out{};
    if (!value.contains("reason")) throw std::invalid_argument("missing reason");
    out.reason = [&] { auto result = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStreamResyncData>().reason)>>::decode(value.at("reason")); if (result != "history_unavailable") throw std::invalid_argument("invalid reason"); return result; }();
    if (value.at("reason").is_null()) throw std::invalid_argument("reason must not be null");
    return out;
}

inline json codec<holder::api::routes::CardLink>::encode(const holder::api::routes::CardLink& value) {
    json result = json::object();
    result["from_card_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().from_card_id)>>::encode(value.from_card_id);
    result["to_card_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().to_card_id)>>::encode(value.to_card_id);
    if (value.target_project_id) result["target_project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().target_project_id)>>::encode(value.target_project_id);
    if (value.source_project_id) result["source_project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().source_project_id)>>::encode(value.source_project_id);
    result["to_type"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().to_type)>>::encode(value.to_type);
    result["kind"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().kind)>>::encode(value.kind);
    if (value.label) result["label"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().label)>>::encode(value.label);
    result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().created_at)>>::encode(value.created_at);
    return result;
}
inline holder::api::routes::CardLink codec<holder::api::routes::CardLink>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLink");
    holder::api::routes::CardLink out{};
    if (!value.contains("from_card_id")) throw std::invalid_argument("missing from_card_id");
    out.from_card_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().from_card_id)>>::decode(value.at("from_card_id")), "from_card_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("from_card_id").is_null()) throw std::invalid_argument("from_card_id must not be null");
    if (!value.contains("to_card_id")) throw std::invalid_argument("missing to_card_id");
    out.to_card_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().to_card_id)>>::decode(value.at("to_card_id")), "to_card_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("to_card_id").is_null()) throw std::invalid_argument("to_card_id must not be null");
    if (value.contains("target_project_id")) out.target_project_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().target_project_id)>>(value.at("target_project_id"));
    if (value.contains("source_project_id")) out.source_project_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().source_project_id)>>(value.at("source_project_id"));
    if (!value.contains("to_type")) throw std::invalid_argument("missing to_type");
    out.to_type = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().to_type)>>::decode(value.at("to_type")), "to_type", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("to_type").is_null()) throw std::invalid_argument("to_type must not be null");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    out.kind = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().kind)>>::decode(value.at("kind")), "kind", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("kind").is_null()) throw std::invalid_argument("kind must not be null");
    if (value.contains("label")) out.label = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().label)>>(value.at("label"));
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    out.created_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLink>().created_at)>>::decode(value.at("created_at"));
    if (value.at("created_at").is_null()) throw std::invalid_argument("created_at must not be null");
    return out;
}

inline json codec<holder::api::routes::CardLinkCreateRequest>::encode(const holder::api::routes::CardLinkCreateRequest& value) {
    json result = json::object();
    result["to_card_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().to_card_id)>>::encode(value.to_card_id);
    if (value.target_project_id) result["target_project_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().target_project_id)>>::encode(value.target_project_id);
    if (value.to_type) result["to_type"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().to_type)>>::encode(value.to_type);
    if (value.kind) result["kind"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().kind)>>::encode(value.kind);
    if (value.label) result["label"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().label)>>::encode(value.label);
    if (value.created_at) result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().created_at)>>::encode(value.created_at);
    return result;
}
inline holder::api::routes::CardLinkCreateRequest codec<holder::api::routes::CardLinkCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLinkCreateRequest");
    holder::api::routes::CardLinkCreateRequest out{};
    if (!value.contains("to_card_id")) throw std::invalid_argument("missing to_card_id");
    out.to_card_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().to_card_id)>>::decode(value.at("to_card_id")), "to_card_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("to_card_id").is_null()) throw std::invalid_argument("to_card_id must not be null");
    if (value.contains("target_project_id")) out.target_project_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().target_project_id)>>(value.at("target_project_id"));
    if (value.contains("to_type")) out.to_type = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().to_type)>>(value.at("to_type"));
    if (value.contains("kind")) out.kind = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().kind)>>(value.at("kind"));
    if (value.contains("label")) out.label = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().label)>>(value.at("label"));
    if (value.contains("created_at")) out.created_at = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateRequest>().created_at)>>(value.at("created_at"));
    return out;
}

inline json codec<holder::api::RunnerPullJob>::encode(const holder::api::RunnerPullJob& value) {
    json result = json::object();
    result["job_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().job_id)>>::encode(value.job_id);
    if (value.runner_id) result["runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().runner_id)>>::encode(value.runner_id);
    if (value.model_ref) result["model_ref"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().model_ref)>>::encode(value.model_ref);
    result["model"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().model)>>::encode(value.model);
    result["status"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().status)>>::encode(value.status);
    result["active"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().active)>>::encode(value.active);
    result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().updated_at)>>::encode(value.updated_at);
    if (value.error) result["error"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().error)>>::encode(value.error);
    result["progress"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().progress)>>::encode(value.progress);
    return result;
}
inline holder::api::RunnerPullJob codec<holder::api::RunnerPullJob>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullJob");
    holder::api::RunnerPullJob out{};
    if (!value.contains("job_id")) throw std::invalid_argument("missing job_id");
    out.job_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().job_id)>>::decode(value.at("job_id")), "job_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("job_id").is_null()) throw std::invalid_argument("job_id must not be null");
    if (value.contains("runner_id")) out.runner_id = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().runner_id)>>(value.at("runner_id"));
    if (value.contains("model_ref")) out.model_ref = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().model_ref)>>(value.at("model_ref"));
    if (!value.contains("model")) throw std::invalid_argument("missing model");
    out.model = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().model)>>::decode(value.at("model")), "model", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("model").is_null()) throw std::invalid_argument("model must not be null");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    out.status = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().status)>>::decode(value.at("status")), "status", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("status").is_null()) throw std::invalid_argument("status must not be null");
    if (!value.contains("active")) throw std::invalid_argument("missing active");
    out.active = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().active)>>::decode(value.at("active"));
    if (value.at("active").is_null()) throw std::invalid_argument("active must not be null");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    out.updated_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().updated_at)>>::decode(value.at("updated_at"));
    if (value.at("updated_at").is_null()) throw std::invalid_argument("updated_at must not be null");
    if (value.contains("error")) out.error = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().error)>>(value.at("error"));
    if (!value.contains("progress")) throw std::invalid_argument("missing progress");
    out.progress = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullJob>().progress)>>::decode(value.at("progress"));
    if (value.at("progress").is_null()) throw std::invalid_argument("progress must not be null");
    return out;
}

inline json codec<holder::api::RunnerPullStartResponse>::encode(const holder::api::RunnerPullStartResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::RunnerPullStartResponse codec<holder::api::RunnerPullStartResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullStartResponse");
    holder::api::RunnerPullStartResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStartResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCredentialsListData>::encode(const holder::api::AiProviderCredentialsListData& value) {
    json result = json::object();
    result["providers"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialsListData>().providers)>>::encode(value.providers);
    return result;
}
inline holder::api::AiProviderCredentialsListData codec<holder::api::AiProviderCredentialsListData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialsListData");
    holder::api::AiProviderCredentialsListData out{};
    if (!value.contains("providers")) throw std::invalid_argument("missing providers");
    out.providers = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialsListData>().providers)>>::decode(value.at("providers"));
    if (value.at("providers").is_null()) throw std::invalid_argument("providers must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCredentialUpsertResponse>::encode(const holder::api::AiProviderCredentialUpsertResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiProviderCredentialUpsertResponse codec<holder::api::AiProviderCredentialUpsertResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialUpsertResponse");
    holder::api::AiProviderCredentialUpsertResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialUpsertResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCredentialDeleteResponse>::encode(const holder::api::AiProviderCredentialDeleteResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialDeleteResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialDeleteResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiProviderCredentialDeleteResponse codec<holder::api::AiProviderCredentialDeleteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialDeleteResponse");
    holder::api::AiProviderCredentialDeleteResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialDeleteResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialDeleteResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiLocalModelConfigResponse>::encode(const holder::api::AiLocalModelConfigResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiLocalModelConfigResponse codec<holder::api::AiLocalModelConfigResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiLocalModelConfigResponse");
    holder::api::AiLocalModelConfigResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiLocalModelConfigResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiMessageListResponse>::encode(const holder::api::AiMessageListResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageListResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageListResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiMessageListResponse codec<holder::api::AiMessageListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageListResponse");
    holder::api::AiMessageListResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageListResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageListResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiMessageGetResponse>::encode(const holder::api::AiMessageGetResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageGetResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageGetResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiMessageGetResponse codec<holder::api::AiMessageGetResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageGetResponse");
    holder::api::AiMessageGetResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageGetResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageGetResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiMessageCreateResponse>::encode(const holder::api::AiMessageCreateResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiMessageCreateResponse codec<holder::api::AiMessageCreateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCreateResponse");
    holder::api::AiMessageCreateResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCreateResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiMessageUpdateResponse>::encode(const holder::api::AiMessageUpdateResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiMessageUpdateResponse codec<holder::api::AiMessageUpdateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageUpdateResponse");
    holder::api::AiMessageUpdateResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageUpdateResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiMessageCaptureResponse>::encode(const holder::api::AiMessageCaptureResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiMessageCaptureResponse codec<holder::api::AiMessageCaptureResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCaptureResponse");
    holder::api::AiMessageCaptureResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiMessageCaptureResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiNudgeListData>::encode(const holder::api::AiNudgeListData& value) {
    json result = json::object();
    result["nudges"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeListData>().nudges)>>::encode(value.nudges);
    return result;
}
inline holder::api::AiNudgeListData codec<holder::api::AiNudgeListData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeListData");
    holder::api::AiNudgeListData out{};
    if (!value.contains("nudges")) throw std::invalid_argument("missing nudges");
    out.nudges = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeListData>().nudges)>>::decode(value.at("nudges"));
    if (value.at("nudges").is_null()) throw std::invalid_argument("nudges must not be null");
    return out;
}

inline json codec<holder::api::AiNudgeDismissResponse>::encode(const holder::api::AiNudgeDismissResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeDismissResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeDismissResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiNudgeDismissResponse codec<holder::api::AiNudgeDismissResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeDismissResponse");
    holder::api::AiNudgeDismissResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeDismissResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeDismissResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiNudgeEvaluateData>::encode(const holder::api::AiNudgeEvaluateData& value) {
    json result = json::object();
    result["kind"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().kind)>>::encode(value.kind);
    result["accepted"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().accepted)>>::encode(value.accepted);
    result["should_nudge"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().should_nudge)>>::encode(value.should_nudge);
    result["reason"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().reason)>>::encode(value.reason);
    result["nudge"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().nudge)>>::encode(value.nudge);
    return result;
}
inline holder::api::AiNudgeEvaluateData codec<holder::api::AiNudgeEvaluateData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeEvaluateData");
    holder::api::AiNudgeEvaluateData out{};
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    out.kind = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().kind)>>::decode(value.at("kind")), "kind", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("kind").is_null()) throw std::invalid_argument("kind must not be null");
    if (!value.contains("accepted")) throw std::invalid_argument("missing accepted");
    out.accepted = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().accepted)>>::decode(value.at("accepted"));
    if (value.at("accepted").is_null()) throw std::invalid_argument("accepted must not be null");
    if (!value.contains("should_nudge")) throw std::invalid_argument("missing should_nudge");
    out.should_nudge = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().should_nudge)>>::decode(value.at("should_nudge"));
    if (value.at("should_nudge").is_null()) throw std::invalid_argument("should_nudge must not be null");
    if (!value.contains("reason")) throw std::invalid_argument("missing reason");
    out.reason = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().reason)>>::decode(value.at("reason")), "reason", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("reason").is_null()) throw std::invalid_argument("reason must not be null");
    if (!value.contains("nudge")) throw std::invalid_argument("missing nudge");
    out.nudge = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateData>().nudge)>>::decode(value.at("nudge"));
    return out;
}

inline json codec<holder::api::AiProviderCatalogModel>::encode(const holder::api::AiProviderCatalogModel& value) {
    json result = json::object();
    result["id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().id)>>::encode(value.id);
    result["endpoint"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().endpoint)>>::encode(value.endpoint);
    result["role"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().role)>>::encode(value.role);
    result["cost_tier"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().cost_tier)>>::encode(value.cost_tier);
    result["default_for_low_budget"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().default_for_low_budget)>>::encode(value.default_for_low_budget);
    result["limits"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().limits)>>::encode(value.limits);
    return result;
}
inline holder::api::AiProviderCatalogModel codec<holder::api::AiProviderCatalogModel>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogModel");
    holder::api::AiProviderCatalogModel out{};
    if (!value.contains("id")) throw std::invalid_argument("missing id");
    out.id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().id)>>::decode(value.at("id")), "id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("id").is_null()) throw std::invalid_argument("id must not be null");
    if (!value.contains("endpoint")) throw std::invalid_argument("missing endpoint");
    out.endpoint = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().endpoint)>>::decode(value.at("endpoint")), "endpoint", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("endpoint").is_null()) throw std::invalid_argument("endpoint must not be null");
    if (!value.contains("role")) throw std::invalid_argument("missing role");
    out.role = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().role)>>::decode(value.at("role"));
    if (!value.contains("cost_tier")) throw std::invalid_argument("missing cost_tier");
    out.cost_tier = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().cost_tier)>>::decode(value.at("cost_tier"));
    if (!value.contains("default_for_low_budget")) throw std::invalid_argument("missing default_for_low_budget");
    out.default_for_low_budget = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().default_for_low_budget)>>::decode(value.at("default_for_low_budget"));
    if (value.at("default_for_low_budget").is_null()) throw std::invalid_argument("default_for_low_budget must not be null");
    if (!value.contains("limits")) throw std::invalid_argument("missing limits");
    out.limits = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogModel>().limits)>>::decode(value.at("limits"));
    if (value.at("limits").is_null()) throw std::invalid_argument("limits must not be null");
    return out;
}

inline json codec<holder::api::AiRunnerDeleteResponse>::encode(const holder::api::AiRunnerDeleteResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerDeleteResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerDeleteResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiRunnerDeleteResponse codec<holder::api::AiRunnerDeleteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerDeleteResponse");
    holder::api::AiRunnerDeleteResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerDeleteResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerDeleteResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiRunListResponse>::encode(const holder::api::AiRunListResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunListResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunListResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiRunListResponse codec<holder::api::AiRunListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunListResponse");
    holder::api::AiRunListResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunListResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunListResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiRunGetResponse>::encode(const holder::api::AiRunGetResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunGetResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunGetResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiRunGetResponse codec<holder::api::AiRunGetResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunGetResponse");
    holder::api::AiRunGetResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunGetResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunGetResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiThreadListResponse>::encode(const holder::api::AiThreadListResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadListResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadListResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiThreadListResponse codec<holder::api::AiThreadListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadListResponse");
    holder::api::AiThreadListResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadListResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadListResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiThreadGetResponse>::encode(const holder::api::AiThreadGetResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadGetResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadGetResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiThreadGetResponse codec<holder::api::AiThreadGetResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadGetResponse");
    holder::api::AiThreadGetResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadGetResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadGetResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiThreadCreateResponse>::encode(const holder::api::AiThreadCreateResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiThreadCreateResponse codec<holder::api::AiThreadCreateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadCreateResponse");
    holder::api::AiThreadCreateResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadCreateResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiThreadPatchResponse>::encode(const holder::api::AiThreadPatchResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadPatchResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadPatchResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiThreadPatchResponse codec<holder::api::AiThreadPatchResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadPatchResponse");
    holder::api::AiThreadPatchResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadPatchResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiThreadPatchResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiProviderSettingResponse>::encode(const holder::api::AiProviderSettingResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiProviderSettingResponse codec<holder::api::AiProviderSettingResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingResponse");
    holder::api::AiProviderSettingResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiProviderSettingsData>::encode(const holder::api::AiProviderSettingsData& value) {
    json result = json::object();
    result["providers"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingsData>().providers)>>::encode(value.providers);
    return result;
}
inline holder::api::AiProviderSettingsData codec<holder::api::AiProviderSettingsData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingsData");
    holder::api::AiProviderSettingsData out{};
    if (!value.contains("providers")) throw std::invalid_argument("missing providers");
    out.providers = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingsData>().providers)>>::decode(value.at("providers"));
    if (value.at("providers").is_null()) throw std::invalid_argument("providers must not be null");
    return out;
}

inline json codec<holder::api::AiProviderSettingDeleteResponse>::encode(const holder::api::AiProviderSettingDeleteResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingDeleteResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingDeleteResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiProviderSettingDeleteResponse codec<holder::api::AiProviderSettingDeleteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingDeleteResponse");
    holder::api::AiProviderSettingDeleteResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingDeleteResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingDeleteResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::routes::CardLinkListResponse>::encode(const holder::api::routes::CardLinkListResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkListResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkListResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::routes::CardLinkListResponse codec<holder::api::routes::CardLinkListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLinkListResponse");
    holder::api::routes::CardLinkListResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkListResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkListResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::routes::CardLinkCreateResponse>::encode(const holder::api::routes::CardLinkCreateResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::routes::CardLinkCreateResponse codec<holder::api::routes::CardLinkCreateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLinkCreateResponse");
    holder::api::routes::CardLinkCreateResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::routes::CardLinkCreateResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::RunnerPullStatusResponse>::encode(const holder::api::RunnerPullStatusResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStatusResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStatusResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::RunnerPullStatusResponse codec<holder::api::RunnerPullStatusResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullStatusResponse");
    holder::api::RunnerPullStatusResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStatusResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerPullStatusResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCredentialsListResponse>::encode(const holder::api::AiProviderCredentialsListResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialsListResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialsListResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiProviderCredentialsListResponse codec<holder::api::AiProviderCredentialsListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialsListResponse");
    holder::api::AiProviderCredentialsListResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialsListResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCredentialsListResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiRunnerRuntime>::encode(const holder::api::AiRunnerRuntime& value) {
    json result = json::object();
    result["configured"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().configured)>>::encode(value.configured);
    result["available"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().available)>>::encode(value.available);
    result["spawn_attempted"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().spawn_attempted)>>::encode(value.spawn_attempted);
    result["last_checked"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().last_checked)>>::encode(value.last_checked);
    result["version"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().version)>>::encode(value.version);
    result["error"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().error)>>::encode(value.error);
    result["models"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().models)>>::encode(value.models);
    result["pulls"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().pulls)>>::encode(value.pulls);
    return result;
}
inline holder::api::AiRunnerRuntime codec<holder::api::AiRunnerRuntime>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerRuntime");
    holder::api::AiRunnerRuntime out{};
    if (!value.contains("configured")) throw std::invalid_argument("missing configured");
    out.configured = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().configured)>>::decode(value.at("configured"));
    if (value.at("configured").is_null()) throw std::invalid_argument("configured must not be null");
    if (!value.contains("available")) throw std::invalid_argument("missing available");
    out.available = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().available)>>::decode(value.at("available"));
    if (value.at("available").is_null()) throw std::invalid_argument("available must not be null");
    if (!value.contains("spawn_attempted")) throw std::invalid_argument("missing spawn_attempted");
    out.spawn_attempted = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().spawn_attempted)>>::decode(value.at("spawn_attempted"));
    if (value.at("spawn_attempted").is_null()) throw std::invalid_argument("spawn_attempted must not be null");
    if (!value.contains("last_checked")) throw std::invalid_argument("missing last_checked");
    out.last_checked = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().last_checked)>>::decode(value.at("last_checked"));
    if (value.at("last_checked").is_null()) throw std::invalid_argument("last_checked must not be null");
    if (!value.contains("version")) throw std::invalid_argument("missing version");
    out.version = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().version)>>::decode(value.at("version"));
    if (!value.contains("error")) throw std::invalid_argument("missing error");
    out.error = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().error)>>::decode(value.at("error"));
    if (!value.contains("models")) throw std::invalid_argument("missing models");
    out.models = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().models)>>::decode(value.at("models"));
    if (value.at("models").is_null()) throw std::invalid_argument("models must not be null");
    if (!value.contains("pulls")) throw std::invalid_argument("missing pulls");
    out.pulls = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerRuntime>().pulls)>>::decode(value.at("pulls"));
    if (value.at("pulls").is_null()) throw std::invalid_argument("pulls must not be null");
    return out;
}

inline json codec<holder::api::AiNudgeListResponse>::encode(const holder::api::AiNudgeListResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeListResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeListResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiNudgeListResponse codec<holder::api::AiNudgeListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeListResponse");
    holder::api::AiNudgeListResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeListResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeListResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiNudgeEvaluateResponse>::encode(const holder::api::AiNudgeEvaluateResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiNudgeEvaluateResponse codec<holder::api::AiNudgeEvaluateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeEvaluateResponse");
    holder::api::AiNudgeEvaluateResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiNudgeEvaluateResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCatalogProvider>::encode(const holder::api::AiProviderCatalogProvider& value) {
    json result = json::object();
    result["id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().id)>>::encode(value.id);
    result["display_name"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().display_name)>>::encode(value.display_name);
    result["enabled"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().enabled)>>::encode(value.enabled);
    result["configured"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().configured)>>::encode(value.configured);
    result["cost_tier"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().cost_tier)>>::encode(value.cost_tier);
    result["setup_url"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().setup_url)>>::encode(value.setup_url);
    result["docs_url"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().docs_url)>>::encode(value.docs_url);
    result["api_key_label"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().api_key_label)>>::encode(value.api_key_label);
    result["api_key_hint"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().api_key_hint)>>::encode(value.api_key_hint);
    result["api"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().api)>>::encode(value.api);
    result["auth"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().auth)>>::encode(value.auth);
    result["models"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().models)>>::encode(value.models);
    return result;
}
inline holder::api::AiProviderCatalogProvider codec<holder::api::AiProviderCatalogProvider>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogProvider");
    holder::api::AiProviderCatalogProvider out{};
    if (!value.contains("id")) throw std::invalid_argument("missing id");
    out.id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().id)>>::decode(value.at("id")), "id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("id").is_null()) throw std::invalid_argument("id must not be null");
    if (!value.contains("display_name")) throw std::invalid_argument("missing display_name");
    out.display_name = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().display_name)>>::decode(value.at("display_name")), "display_name", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("display_name").is_null()) throw std::invalid_argument("display_name must not be null");
    if (!value.contains("enabled")) throw std::invalid_argument("missing enabled");
    out.enabled = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().enabled)>>::decode(value.at("enabled"));
    if (value.at("enabled").is_null()) throw std::invalid_argument("enabled must not be null");
    if (!value.contains("configured")) throw std::invalid_argument("missing configured");
    out.configured = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().configured)>>::decode(value.at("configured"));
    if (value.at("configured").is_null()) throw std::invalid_argument("configured must not be null");
    if (!value.contains("cost_tier")) throw std::invalid_argument("missing cost_tier");
    out.cost_tier = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().cost_tier)>>::decode(value.at("cost_tier"));
    if (!value.contains("setup_url")) throw std::invalid_argument("missing setup_url");
    out.setup_url = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().setup_url)>>::decode(value.at("setup_url"));
    if (!value.contains("docs_url")) throw std::invalid_argument("missing docs_url");
    out.docs_url = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().docs_url)>>::decode(value.at("docs_url"));
    if (!value.contains("api_key_label")) throw std::invalid_argument("missing api_key_label");
    out.api_key_label = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().api_key_label)>>::decode(value.at("api_key_label"));
    if (!value.contains("api_key_hint")) throw std::invalid_argument("missing api_key_hint");
    out.api_key_hint = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().api_key_hint)>>::decode(value.at("api_key_hint"));
    if (!value.contains("api")) throw std::invalid_argument("missing api");
    out.api = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().api)>>::decode(value.at("api"));
    if (value.at("api").is_null()) throw std::invalid_argument("api must not be null");
    if (!value.contains("auth")) throw std::invalid_argument("missing auth");
    out.auth = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().auth)>>::decode(value.at("auth"));
    if (value.at("auth").is_null()) throw std::invalid_argument("auth must not be null");
    if (!value.contains("models")) throw std::invalid_argument("missing models");
    out.models = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogProvider>().models)>>::decode(value.at("models"));
    if (value.at("models").is_null()) throw std::invalid_argument("models must not be null");
    return out;
}

inline json codec<holder::api::AiProviderSettingsResponse>::encode(const holder::api::AiProviderSettingsResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingsResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingsResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiProviderSettingsResponse codec<holder::api::AiProviderSettingsResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingsResponse");
    holder::api::AiProviderSettingsResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingsResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderSettingsResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiRunner>::encode(const holder::api::AiRunner& value) {
    json result = json::object();
    result["runner_id"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().runner_id)>>::encode(value.runner_id);
    result["name"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().name)>>::encode(value.name);
    result["kind"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().kind)>>::encode(value.kind);
    result["base_url"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().base_url)>>::encode(value.base_url);
    result["source"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().source)>>::encode(value.source);
    result["enabled"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().enabled)>>::encode(value.enabled);
    result["created_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().created_at)>>::encode(value.created_at);
    result["updated_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().updated_at)>>::encode(value.updated_at);
    result["runtime"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().runtime)>>::encode(value.runtime);
    return result;
}
inline holder::api::AiRunner codec<holder::api::AiRunner>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunner");
    holder::api::AiRunner out{};
    if (!value.contains("runner_id")) throw std::invalid_argument("missing runner_id");
    out.runner_id = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().runner_id)>>::decode(value.at("runner_id")), "runner_id", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("runner_id").is_null()) throw std::invalid_argument("runner_id must not be null");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    out.name = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().name)>>::decode(value.at("name")), "name", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("name").is_null()) throw std::invalid_argument("name must not be null");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    out.kind = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().kind)>>::decode(value.at("kind")), "kind", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("kind").is_null()) throw std::invalid_argument("kind must not be null");
    if (!value.contains("base_url")) throw std::invalid_argument("missing base_url");
    out.base_url = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().base_url)>>::decode(value.at("base_url"));
    if (!value.contains("source")) throw std::invalid_argument("missing source");
    out.source = ai_json::check_text(codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().source)>>::decode(value.at("source")), "source", 0, std::numeric_limits<std::size_t>::max());
    if (value.at("source").is_null()) throw std::invalid_argument("source must not be null");
    if (!value.contains("enabled")) throw std::invalid_argument("missing enabled");
    out.enabled = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().enabled)>>::decode(value.at("enabled"));
    if (value.at("enabled").is_null()) throw std::invalid_argument("enabled must not be null");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    out.created_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().created_at)>>::decode(value.at("created_at"));
    if (value.at("created_at").is_null()) throw std::invalid_argument("created_at must not be null");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    out.updated_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().updated_at)>>::decode(value.at("updated_at"));
    if (value.at("updated_at").is_null()) throw std::invalid_argument("updated_at must not be null");
    if (!value.contains("runtime")) throw std::invalid_argument("missing runtime");
    out.runtime = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunner>().runtime)>>::decode(value.at("runtime"));
    if (value.at("runtime").is_null()) throw std::invalid_argument("runtime must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCatalogResponseData>::encode(const holder::api::AiProviderCatalogResponseData& value) {
    json result = json::object();
    result["default_provider"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogResponseData>().default_provider)>>::encode(value.default_provider);
    result["providers"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogResponseData>().providers)>>::encode(value.providers);
    return result;
}
inline holder::api::AiProviderCatalogResponseData codec<holder::api::AiProviderCatalogResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogResponseData");
    holder::api::AiProviderCatalogResponseData out{};
    if (!value.contains("default_provider")) throw std::invalid_argument("missing default_provider");
    out.default_provider = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogResponseData>().default_provider)>>::decode(value.at("default_provider"));
    if (!value.contains("providers")) throw std::invalid_argument("missing providers");
    out.providers = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogResponseData>().providers)>>::decode(value.at("providers"));
    if (value.at("providers").is_null()) throw std::invalid_argument("providers must not be null");
    return out;
}

inline json codec<holder::api::AiRunnerListResponseData>::encode(const holder::api::AiRunnerListResponseData& value) {
    json result = json::object();
    result["runners"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerListResponseData>().runners)>>::encode(value.runners);
    return result;
}
inline holder::api::AiRunnerListResponseData codec<holder::api::AiRunnerListResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerListResponseData");
    holder::api::AiRunnerListResponseData out{};
    if (!value.contains("runners")) throw std::invalid_argument("missing runners");
    out.runners = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerListResponseData>().runners)>>::decode(value.at("runners"));
    if (value.at("runners").is_null()) throw std::invalid_argument("runners must not be null");
    return out;
}

inline json codec<holder::api::AiRunnerResponse>::encode(const holder::api::AiRunnerResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiRunnerResponse codec<holder::api::AiRunnerResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerResponse");
    holder::api::AiRunnerResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiProviderCatalogResponse>::encode(const holder::api::AiProviderCatalogResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiProviderCatalogResponse codec<holder::api::AiProviderCatalogResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogResponse");
    holder::api::AiProviderCatalogResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiProviderCatalogResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::RunnerCapabilitiesData>::encode(const holder::api::RunnerCapabilitiesData& value) {
    json result = json::object();
    result["local_model_config"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().local_model_config)>>::encode(value.local_model_config);
    if (value.caste) result["caste"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().caste)>>::encode(value.caste);
    result["runners"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().runners)>>::encode(value.runners);
    result["recommended_models"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().recommended_models)>>::encode(value.recommended_models);
    result["recommended_install"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().recommended_install)>>::encode(value.recommended_install);
    return result;
}
inline holder::api::RunnerCapabilitiesData codec<holder::api::RunnerCapabilitiesData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerCapabilitiesData");
    holder::api::RunnerCapabilitiesData out{};
    if (!value.contains("local_model_config")) throw std::invalid_argument("missing local_model_config");
    out.local_model_config = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().local_model_config)>>::decode(value.at("local_model_config"));
    if (value.at("local_model_config").is_null()) throw std::invalid_argument("local_model_config must not be null");
    if (value.contains("caste")) out.caste = decode_optional_nullable<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().caste)>>(value.at("caste"));
    if (!value.contains("runners")) throw std::invalid_argument("missing runners");
    out.runners = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().runners)>>::decode(value.at("runners"));
    if (value.at("runners").is_null()) throw std::invalid_argument("runners must not be null");
    if (!value.contains("recommended_models")) throw std::invalid_argument("missing recommended_models");
    out.recommended_models = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().recommended_models)>>::decode(value.at("recommended_models"));
    if (value.at("recommended_models").is_null()) throw std::invalid_argument("recommended_models must not be null");
    if (!value.contains("recommended_install")) throw std::invalid_argument("missing recommended_install");
    out.recommended_install = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesData>().recommended_install)>>::decode(value.at("recommended_install"));
    if (value.at("recommended_install").is_null()) throw std::invalid_argument("recommended_install must not be null");
    return out;
}

inline json codec<holder::api::AiStatusData>::encode(const holder::api::AiStatusData& value) {
    json result = json::object();
    result["checked_at"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().checked_at)>>::encode(value.checked_at);
    result["active_runs"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().active_runs)>>::encode(value.active_runs);
    result["active_pull_jobs"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().active_pull_jobs)>>::encode(value.active_pull_jobs);
    result["runners"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().runners)>>::encode(value.runners);
    result["cloud"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().cloud)>>::encode(value.cloud);
    result["cloud_configured_providers"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().cloud_configured_providers)>>::encode(value.cloud_configured_providers);
    return result;
}
inline holder::api::AiStatusData codec<holder::api::AiStatusData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiStatusData");
    holder::api::AiStatusData out{};
    if (!value.contains("checked_at")) throw std::invalid_argument("missing checked_at");
    out.checked_at = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().checked_at)>>::decode(value.at("checked_at"));
    if (value.at("checked_at").is_null()) throw std::invalid_argument("checked_at must not be null");
    if (!value.contains("active_runs")) throw std::invalid_argument("missing active_runs");
    out.active_runs = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().active_runs)>>::decode(value.at("active_runs"));
    if (value.at("active_runs").is_null()) throw std::invalid_argument("active_runs must not be null");
    if (!value.contains("active_pull_jobs")) throw std::invalid_argument("missing active_pull_jobs");
    out.active_pull_jobs = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().active_pull_jobs)>>::decode(value.at("active_pull_jobs"));
    if (value.at("active_pull_jobs").is_null()) throw std::invalid_argument("active_pull_jobs must not be null");
    if (!value.contains("runners")) throw std::invalid_argument("missing runners");
    out.runners = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().runners)>>::decode(value.at("runners"));
    if (value.at("runners").is_null()) throw std::invalid_argument("runners must not be null");
    if (!value.contains("cloud")) throw std::invalid_argument("missing cloud");
    out.cloud = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().cloud)>>::decode(value.at("cloud"));
    if (value.at("cloud").is_null()) throw std::invalid_argument("cloud must not be null");
    if (!value.contains("cloud_configured_providers")) throw std::invalid_argument("missing cloud_configured_providers");
    out.cloud_configured_providers = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusData>().cloud_configured_providers)>>::decode(value.at("cloud_configured_providers"));
    if (value.at("cloud_configured_providers").is_null()) throw std::invalid_argument("cloud_configured_providers must not be null");
    return out;
}

inline json codec<holder::api::AiRunnerListResponse>::encode(const holder::api::AiRunnerListResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerListResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerListResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiRunnerListResponse codec<holder::api::AiRunnerListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerListResponse");
    holder::api::AiRunnerListResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerListResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiRunnerListResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::RunnerCapabilitiesResponse>::encode(const holder::api::RunnerCapabilitiesResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::RunnerCapabilitiesResponse codec<holder::api::RunnerCapabilitiesResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerCapabilitiesResponse");
    holder::api::RunnerCapabilitiesResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::RunnerCapabilitiesResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

inline json codec<holder::api::AiStatusResponse>::encode(const holder::api::AiStatusResponse& value) {
    json result = json::object();
    result["ok"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusResponse>().ok)>>::encode(value.ok);
    result["data"] = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusResponse>().data)>>::encode(value.data);
    return result;
}
inline holder::api::AiStatusResponse codec<holder::api::AiStatusResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiStatusResponse");
    holder::api::AiStatusResponse out{};
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    out.ok = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusResponse>().ok)>>::decode(value.at("ok"));
    if (value.at("ok").is_null()) throw std::invalid_argument("ok must not be null");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    out.data = codec<std::remove_cvref_t<decltype(std::declval<holder::api::AiStatusResponse>().data)>>::decode(value.at("data"));
    if (value.at("data").is_null()) throw std::invalid_argument("data must not be null");
    return out;
}

template <typename T> json encode(const T& value) { return codec<T>::encode(value); }
template <typename T> T decode(const json& value) { return codec<T>::decode(value); }
} // namespace ai_json

namespace ai_events {
inline api_runtime::event<nlohmann::json> chunk(const holder::api::AiRunEventData& data) {
    return {"chunk", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> completed(const holder::api::RunnerPullJob& data) {
    return {"completed", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> done(const holder::api::AiRunEventData& data) {
    return {"done", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> failed(const holder::api::AiRunEventData& data) {
    return {"failed", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> failed(const holder::api::RunnerPullJob& data) {
    return {"failed", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> fallback(const holder::api::AiRunEventData& data) {
    return {"fallback", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> progress(const holder::api::AiRunEventData& data) {
    return {"progress", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> progress(const holder::api::RunnerPullJob& data) {
    return {"progress", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> resync_required(const holder::api::AiStreamResyncData& data) {
    return {"resync_required", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> router(const holder::api::AiRunEventData& data) {
    return {"router", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> router_result(const holder::api::AiRunEventData& data) {
    return {"router_result", ai_json::encode(data)};
}
inline api_runtime::event<nlohmann::json> run_started(const holder::api::AiRunEventData& data) {
    return {"run_started", ai_json::encode(data)};
}
} // namespace ai_events
