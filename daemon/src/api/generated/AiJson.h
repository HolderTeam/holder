// Retained AI JSON codec. AI route schemas are declared in contract/interface/.
#pragma once
#include "api/AiRuntime.h"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <limits>
#include <map>
#include <regex>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace ai_json {
using json = nlohmann::json;
template <typename T> struct codec;

template <> struct codec<int> {
    static json encode(int value) { return value; }
    static int decode(const json& value) {
        if (!value.is_number_integer()) throw std::invalid_argument("expected int");
        if (value.is_number_unsigned()) {
            if (value.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
                throw std::invalid_argument("int out of range");
        } else {
            const auto number = value.get<std::int64_t>();
            if (number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max())
                throw std::invalid_argument("int out of range");
        }
        return value.get<int>();
    }
};
template <> struct codec<bool> {
    static json encode(bool value) { return value; }
    static bool decode(const json& value) {
        if (!value.is_boolean()) throw std::invalid_argument("expected bool");
        return value.get<bool>();
    }
};
template <> struct codec<long long> {
    static json encode(long long value) { return value; }
    static long long decode(const json& value) {
        if (!value.is_number_integer()) throw std::invalid_argument("expected int64");
        if (value.is_number_unsigned() &&
            value.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<long long>::max()))
            throw std::invalid_argument("int64 out of range");
        return value.get<long long>();
    }
};
template <> struct codec<double> {
    static json encode(double value) { return value; }
    static double decode(const json& value) {
        if (!value.is_number()) throw std::invalid_argument("expected double");
        return value.get<double>();
    }
};
template <> struct codec<std::string> {
    static json encode(const std::string& value) { return value; }
    static std::string decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("expected string");
        return value.get<std::string>();
    }
};
// A free-form JSON object: any members, but an object.
template <> struct codec<json> {
    static json encode(const json& value) {
        if (!value.is_object()) throw std::invalid_argument("expected object");
        return value;
    }
    static json decode(const json& value) { return encode(value); }
};
template <typename T> struct codec<std::map<std::string, T>> {
    static json encode(const std::map<std::string, T>& value) {
        json result = json::object();
        for (const auto& [key, item] : value) result[key] = codec<T>::encode(item);
        return result;
    }
    static std::map<std::string, T> decode(const json& value) {
        if (!value.is_object()) throw std::invalid_argument("expected object");
        std::map<std::string, T> result;
        for (auto it = value.begin(); it != value.end(); ++it)
            result.emplace(it.key(), codec<T>::decode(it.value()));
        return result;
    }
};
// OpenAPI oneOf: exactly one alternative must accept the value.
template <typename... Ts> struct codec<std::variant<Ts...>> {
    static json encode(const std::variant<Ts...>& value) {
        return std::visit([](const auto& item) {
            return codec<std::decay_t<decltype(item)>>::encode(item);
        }, value);
    }
    static std::variant<Ts...> decode(const json& value) {
        std::optional<std::variant<Ts...>> result;
        int matches = 0;
        (try_alternative<Ts>(value, result, matches), ...);
        if (matches == 0) throw std::invalid_argument("no alternative matches");
        if (matches > 1) throw std::invalid_argument("several alternatives match");
        return std::move(*result);
    }
    template <typename T>
    static void try_alternative(const json& value, std::optional<std::variant<Ts...>>& result,
                                int& matches) {
        try {
            T decoded = codec<T>::decode(value);
            if (!result) result.emplace(std::in_place_type<T>, std::move(decoded));
            ++matches;
        } catch (const std::invalid_argument&) {
        } catch (const json::exception&) {
        }
    }
};
template <typename T> struct codec<std::vector<T>> {
    static json encode(const std::vector<T>& value) {
        json result = json::array();
        for (const auto& item : value) result.push_back(codec<T>::encode(item));
        return result;
    }
    static std::vector<T> decode(const json& value) {
        if (!value.is_array()) throw std::invalid_argument("expected array");
        std::vector<T> result;
        for (const auto& item : value) result.push_back(codec<T>::decode(item));
        return result;
    }
};
template <typename T> struct codec<std::optional<T>> {
    static json encode(const std::optional<T>& value) {
        return value ? codec<T>::encode(*value) : json(nullptr);
    }
    static std::optional<T> decode(const json& value) {
        return value.is_null() ? std::nullopt : std::optional<T>(codec<T>::decode(value));
    }
};

template <> struct codec<holder::api::Error> {
    static json encode(const holder::api::Error& value);
    static holder::api::Error decode(const json& value);
};
template <> struct codec<holder::api::ErrorResponse> {
    static json encode(const holder::api::ErrorResponse& value);
    static holder::api::ErrorResponse decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullProgress> {
    static json encode(const holder::api::RunnerPullProgress& value);
    static holder::api::RunnerPullProgress decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullJob> {
    static json encode(const holder::api::RunnerPullJob& value);
    static holder::api::RunnerPullJob decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullStatusResponse> {
    static json encode(const holder::api::RunnerPullStatusResponse& value);
    static holder::api::RunnerPullStatusResponse decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullStartResponseData> {
    static json encode(const holder::api::RunnerPullStartResponseData& value);
    static holder::api::RunnerPullStartResponseData decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullStartResponse> {
    static json encode(const holder::api::RunnerPullStartResponse& value);
    static holder::api::RunnerPullStartResponse decode(const json& value);
};
template <> struct codec<holder::api::PostAiRunnerPullRequest> {
    static json encode(const holder::api::PostAiRunnerPullRequest& value);
    static holder::api::PostAiRunnerPullRequest decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredential> {
    static json encode(const holder::api::AiProviderCredential& value);
    static holder::api::AiProviderCredential decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialsListData> {
    static json encode(const holder::api::AiProviderCredentialsListData& value);
    static holder::api::AiProviderCredentialsListData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialsListResponse> {
    static json encode(const holder::api::AiProviderCredentialsListResponse& value);
    static holder::api::AiProviderCredentialsListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialUpsertRequest> {
    static json encode(const holder::api::AiProviderCredentialUpsertRequest& value);
    static holder::api::AiProviderCredentialUpsertRequest decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialUpsertResponse> {
    static json encode(const holder::api::AiProviderCredentialUpsertResponse& value);
    static holder::api::AiProviderCredentialUpsertResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialDeleteResponseData> {
    static json encode(const holder::api::AiProviderCredentialDeleteResponseData& value);
    static holder::api::AiProviderCredentialDeleteResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialDeleteResponse> {
    static json encode(const holder::api::AiProviderCredentialDeleteResponse& value);
    static holder::api::AiProviderCredentialDeleteResponse decode(const json& value);
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
template <> struct codec<holder::api::AiRunnerRuntime> {
    static json encode(const holder::api::AiRunnerRuntime& value);
    static holder::api::AiRunnerRuntime decode(const json& value);
};
template <> struct codec<holder::api::AiRunner> {
    static json encode(const holder::api::AiRunner& value);
    static holder::api::AiRunner decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerListResponseData> {
    static json encode(const holder::api::AiRunnerListResponseData& value);
    static holder::api::AiRunnerListResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerListResponse> {
    static json encode(const holder::api::AiRunnerListResponse& value);
    static holder::api::AiRunnerListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiRunnerResponse> {
    static json encode(const holder::api::AiRunnerResponse& value);
    static holder::api::AiRunnerResponse decode(const json& value);
};
template <> struct codec<holder::api::AiLocalModelConfigResponse> {
    static json encode(const holder::api::AiLocalModelConfigResponse& value);
    static holder::api::AiLocalModelConfigResponse decode(const json& value);
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
template <> struct codec<holder::api::AiMessageListResponse> {
    static json encode(const holder::api::AiMessageListResponse& value);
    static holder::api::AiMessageListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageGetResponse> {
    static json encode(const holder::api::AiMessageGetResponse& value);
    static holder::api::AiMessageGetResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCreateResponseData> {
    static json encode(const holder::api::AiMessageCreateResponseData& value);
    static holder::api::AiMessageCreateResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCreateResponse> {
    static json encode(const holder::api::AiMessageCreateResponse& value);
    static holder::api::AiMessageCreateResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCreateRequest> {
    static json encode(const holder::api::AiMessageCreateRequest& value);
    static holder::api::AiMessageCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiMessageUpdateResponseData> {
    static json encode(const holder::api::AiMessageUpdateResponseData& value);
    static holder::api::AiMessageUpdateResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiMessageUpdateResponse> {
    static json encode(const holder::api::AiMessageUpdateResponse& value);
    static holder::api::AiMessageUpdateResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageUpdateRequest> {
    static json encode(const holder::api::AiMessageUpdateRequest& value);
    static holder::api::AiMessageUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCaptureResponseData> {
    static json encode(const holder::api::AiMessageCaptureResponseData& value);
    static holder::api::AiMessageCaptureResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCaptureResponse> {
    static json encode(const holder::api::AiMessageCaptureResponse& value);
    static holder::api::AiMessageCaptureResponse decode(const json& value);
};
template <> struct codec<holder::api::AiMessageCaptureRequest> {
    static json encode(const holder::api::AiMessageCaptureRequest& value);
    static holder::api::AiMessageCaptureRequest decode(const json& value);
};
template <> struct codec<holder::api::AiNudge> {
    static json encode(const holder::api::AiNudge& value);
    static holder::api::AiNudge decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeListData> {
    static json encode(const holder::api::AiNudgeListData& value);
    static holder::api::AiNudgeListData decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeListResponse> {
    static json encode(const holder::api::AiNudgeListResponse& value);
    static holder::api::AiNudgeListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeDismissData> {
    static json encode(const holder::api::AiNudgeDismissData& value);
    static holder::api::AiNudgeDismissData decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeDismissResponse> {
    static json encode(const holder::api::AiNudgeDismissResponse& value);
    static holder::api::AiNudgeDismissResponse decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeCandidate> {
    static json encode(const holder::api::AiNudgeCandidate& value);
    static holder::api::AiNudgeCandidate decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeEvaluateData> {
    static json encode(const holder::api::AiNudgeEvaluateData& value);
    static holder::api::AiNudgeEvaluateData decode(const json& value);
};
template <> struct codec<holder::api::AiNudgeEvaluateResponse> {
    static json encode(const holder::api::AiNudgeEvaluateResponse& value);
    static holder::api::AiNudgeEvaluateResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogModelLimits> {
    static json encode(const holder::api::AiProviderCatalogModelLimits& value);
    static holder::api::AiProviderCatalogModelLimits decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogModel> {
    static json encode(const holder::api::AiProviderCatalogModel& value);
    static holder::api::AiProviderCatalogModel decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogApi> {
    static json encode(const holder::api::AiProviderCatalogApi& value);
    static holder::api::AiProviderCatalogApi decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogAuth> {
    static json encode(const holder::api::AiProviderCatalogAuth& value);
    static holder::api::AiProviderCatalogAuth decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogProvider> {
    static json encode(const holder::api::AiProviderCatalogProvider& value);
    static holder::api::AiProviderCatalogProvider decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogResponseData> {
    static json encode(const holder::api::AiProviderCatalogResponseData& value);
    static holder::api::AiProviderCatalogResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCatalogResponse> {
    static json encode(const holder::api::AiProviderCatalogResponse& value);
    static holder::api::AiProviderCatalogResponse decode(const json& value);
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
template <> struct codec<holder::api::AiRunnerDeleteResponse> {
    static json encode(const holder::api::AiRunnerDeleteResponse& value);
    static holder::api::AiRunnerDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::AiRunItem> {
    static json encode(const holder::api::AiRunItem& value);
    static holder::api::AiRunItem decode(const json& value);
};
template <> struct codec<holder::api::AiRunListResponse> {
    static json encode(const holder::api::AiRunListResponse& value);
    static holder::api::AiRunListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiRunGetResponse> {
    static json encode(const holder::api::AiRunGetResponse& value);
    static holder::api::AiRunGetResponse decode(const json& value);
};
template <> struct codec<holder::api::CasteInfo> {
    static json encode(const holder::api::CasteInfo& value);
    static holder::api::CasteInfo decode(const json& value);
};
template <> struct codec<holder::api::CasteRecommendedModel> {
    static json encode(const holder::api::CasteRecommendedModel& value);
    static holder::api::CasteRecommendedModel decode(const json& value);
};
template <> struct codec<holder::api::RunnerCapabilitiesData> {
    static json encode(const holder::api::RunnerCapabilitiesData& value);
    static holder::api::RunnerCapabilitiesData decode(const json& value);
};
template <> struct codec<holder::api::RunnerCapabilitiesResponse> {
    static json encode(const holder::api::RunnerCapabilitiesResponse& value);
    static holder::api::RunnerCapabilitiesResponse decode(const json& value);
};
template <> struct codec<holder::api::AiStatusData> {
    static json encode(const holder::api::AiStatusData& value);
    static holder::api::AiStatusData decode(const json& value);
};
template <> struct codec<holder::api::AiStatusResponse> {
    static json encode(const holder::api::AiStatusResponse& value);
    static holder::api::AiStatusResponse decode(const json& value);
};
template <> struct codec<holder::api::AiThreadListResponse> {
    static json encode(const holder::api::AiThreadListResponse& value);
    static holder::api::AiThreadListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiThreadGetResponse> {
    static json encode(const holder::api::AiThreadGetResponse& value);
    static holder::api::AiThreadGetResponse decode(const json& value);
};
template <> struct codec<holder::api::AiThreadCreateResponseData> {
    static json encode(const holder::api::AiThreadCreateResponseData& value);
    static holder::api::AiThreadCreateResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiThreadCreateResponse> {
    static json encode(const holder::api::AiThreadCreateResponse& value);
    static holder::api::AiThreadCreateResponse decode(const json& value);
};
template <> struct codec<holder::api::AiThreadCreateRequest> {
    static json encode(const holder::api::AiThreadCreateRequest& value);
    static holder::api::AiThreadCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiThreadPatchResponseData> {
    static json encode(const holder::api::AiThreadPatchResponseData& value);
    static holder::api::AiThreadPatchResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiThreadPatchResponse> {
    static json encode(const holder::api::AiThreadPatchResponse& value);
    static holder::api::AiThreadPatchResponse decode(const json& value);
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
template <> struct codec<holder::api::AiProviderSettingResponse> {
    static json encode(const holder::api::AiProviderSettingResponse& value);
    static holder::api::AiProviderSettingResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingsData> {
    static json encode(const holder::api::AiProviderSettingsData& value);
    static holder::api::AiProviderSettingsData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingsResponse> {
    static json encode(const holder::api::AiProviderSettingsResponse& value);
    static holder::api::AiProviderSettingsResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingDeleteData> {
    static json encode(const holder::api::AiProviderSettingDeleteData& value);
    static holder::api::AiProviderSettingDeleteData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingDeleteResponse> {
    static json encode(const holder::api::AiProviderSettingDeleteResponse& value);
    static holder::api::AiProviderSettingDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::AiStreamResyncData> {
    static json encode(const holder::api::AiStreamResyncData& value);
    static holder::api::AiStreamResyncData decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLinkListResponse> {
    static json encode(const holder::api::routes::CardLinkListResponse& value);
    static holder::api::routes::CardLinkListResponse decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLink> {
    static json encode(const holder::api::routes::CardLink& value);
    static holder::api::routes::CardLink decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLinkCreateResponse> {
    static json encode(const holder::api::routes::CardLinkCreateResponse& value);
    static holder::api::routes::CardLinkCreateResponse decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLinkCreateRequest> {
    static json encode(const holder::api::routes::CardLinkCreateRequest& value);
    static holder::api::routes::CardLinkCreateRequest decode(const json& value);
};

inline json codec<holder::api::Error>::encode(const holder::api::Error& value) {
    json result = json::object();
    result["code"] = codec<std::string>::encode(value.code);
    result["message"] = codec<std::string>::encode(value.message);
    return result;
}
inline holder::api::Error codec<holder::api::Error>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected Error");
    if (!value.contains("code")) throw std::invalid_argument("missing code");
    if (!value.contains("message")) throw std::invalid_argument("missing message");
    return holder::api::Error{
        codec<std::string>::decode(value.at("code")),
        codec<std::string>::decode(value.at("message")),
    };
}

inline json codec<holder::api::ErrorResponse>::encode(const holder::api::ErrorResponse& value) {
    json result = json::object();
    if (value.ok != false) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["error"] = codec<holder::api::Error>::encode(value.error);
    return result;
}
inline holder::api::ErrorResponse codec<holder::api::ErrorResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ErrorResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("error")) throw std::invalid_argument("missing error");
    if (value.at("ok") != false) throw std::invalid_argument("invalid ok");
    return holder::api::ErrorResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::Error>::decode(value.at("error")),
    };
}

inline json codec<holder::api::RunnerPullProgress>::encode(const holder::api::RunnerPullProgress& value) {
    json result = json::object();
    result["completed"] = codec<long long>::encode(value.completed);
    result["total"] = codec<long long>::encode(value.total);
    result["percent"] = codec<double>::encode(value.percent);
    result["stage"] = codec<std::string>::encode(value.stage);
    return result;
}
inline holder::api::RunnerPullProgress codec<holder::api::RunnerPullProgress>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullProgress");
    if (!value.contains("completed")) throw std::invalid_argument("missing completed");
    if (!value.contains("total")) throw std::invalid_argument("missing total");
    if (!value.contains("percent")) throw std::invalid_argument("missing percent");
    if (!value.contains("stage")) throw std::invalid_argument("missing stage");
    return holder::api::RunnerPullProgress{
        codec<long long>::decode(value.at("completed")),
        codec<long long>::decode(value.at("total")),
        codec<double>::decode(value.at("percent")),
        codec<std::string>::decode(value.at("stage")),
    };
}

inline json codec<holder::api::RunnerPullJob>::encode(const holder::api::RunnerPullJob& value) {
    json result = json::object();
    result["job_id"] = codec<std::string>::encode(value.job_id);
    if (value.runner_id) {
        result["runner_id"] = codec<std::optional<std::string>>::encode(value.runner_id);
    }
    if (value.model_ref) {
        result["model_ref"] = codec<std::optional<std::string>>::encode(value.model_ref);
    }
    result["model"] = codec<std::string>::encode(value.model);
    result["status"] = codec<std::string>::encode(value.status);
    result["active"] = codec<bool>::encode(value.active);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    if (value.error) {
        result["error"] = codec<std::optional<std::optional<std::string>>>::encode(value.error);
    }
    result["progress"] = codec<holder::api::RunnerPullProgress>::encode(value.progress);
    return result;
}
inline holder::api::RunnerPullJob codec<holder::api::RunnerPullJob>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullJob");
    if (!value.contains("job_id")) throw std::invalid_argument("missing job_id");
    if (!value.contains("model")) throw std::invalid_argument("missing model");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    if (!value.contains("active")) throw std::invalid_argument("missing active");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    if (!value.contains("progress")) throw std::invalid_argument("missing progress");
    return holder::api::RunnerPullJob{
        codec<std::string>::decode(value.at("job_id")),
        value.contains("runner_id") ? codec<std::optional<std::string>>::decode(value.at("runner_id")) : std::nullopt,
        value.contains("model_ref") ? codec<std::optional<std::string>>::decode(value.at("model_ref")) : std::nullopt,
        codec<std::string>::decode(value.at("model")),
        codec<std::string>::decode(value.at("status")),
        codec<bool>::decode(value.at("active")),
        codec<long long>::decode(value.at("updated_at")),
        value.contains("error") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("error"))} : std::nullopt,
        codec<holder::api::RunnerPullProgress>::decode(value.at("progress")),
    };
}

inline json codec<holder::api::RunnerPullStatusResponse>::encode(const holder::api::RunnerPullStatusResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::RunnerPullJob>::encode(value.data);
    return result;
}
inline holder::api::RunnerPullStatusResponse codec<holder::api::RunnerPullStatusResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullStatusResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::RunnerPullStatusResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::RunnerPullJob>::decode(value.at("data")),
    };
}

inline json codec<holder::api::RunnerPullStartResponseData>::encode(const holder::api::RunnerPullStartResponseData& value) {
    json result = json::object();
    result["job_id"] = codec<std::string>::encode(value.job_id);
    if (value.runner_id) {
        result["runner_id"] = codec<std::optional<std::string>>::encode(value.runner_id);
    }
    if (value.model_ref) {
        result["model_ref"] = codec<std::optional<std::string>>::encode(value.model_ref);
    }
    result["model"] = codec<std::string>::encode(value.model);
    result["status"] = codec<std::string>::encode(value.status);
    return result;
}
inline holder::api::RunnerPullStartResponseData codec<holder::api::RunnerPullStartResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullStartResponseData");
    if (!value.contains("job_id")) throw std::invalid_argument("missing job_id");
    if (!value.contains("model")) throw std::invalid_argument("missing model");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    return holder::api::RunnerPullStartResponseData{
        codec<std::string>::decode(value.at("job_id")),
        value.contains("runner_id") ? codec<std::optional<std::string>>::decode(value.at("runner_id")) : std::nullopt,
        value.contains("model_ref") ? codec<std::optional<std::string>>::decode(value.at("model_ref")) : std::nullopt,
        codec<std::string>::decode(value.at("model")),
        codec<std::string>::decode(value.at("status")),
    };
}

inline json codec<holder::api::RunnerPullStartResponse>::encode(const holder::api::RunnerPullStartResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::RunnerPullStartResponseData>::encode(value.data);
    return result;
}
inline holder::api::RunnerPullStartResponse codec<holder::api::RunnerPullStartResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerPullStartResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::RunnerPullStartResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::RunnerPullStartResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::PostAiRunnerPullRequest>::encode(const holder::api::PostAiRunnerPullRequest& value) {
    json result = json::object();
    result["model"] = codec<std::string>::encode(value.model);
    if (value.runner_id) {
        result["runner_id"] = codec<std::optional<std::string>>::encode(value.runner_id);
    }
    return result;
}
inline holder::api::PostAiRunnerPullRequest codec<holder::api::PostAiRunnerPullRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected PostAiRunnerPullRequest");
    if (!value.contains("model")) throw std::invalid_argument("missing model");
    return holder::api::PostAiRunnerPullRequest{
        codec<std::string>::decode(value.at("model")),
        value.contains("runner_id") ? codec<std::optional<std::string>>::decode(value.at("runner_id")) : std::nullopt,
    };
}

inline json codec<holder::api::AiProviderCredential>::encode(const holder::api::AiProviderCredential& value) {
    json result = json::object();
    result["provider"] = codec<std::string>::encode(value.provider);
    result["configured"] = codec<bool>::encode(value.configured);
    result["api_key_preview"] = codec<std::string>::encode(value.api_key_preview);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::AiProviderCredential codec<holder::api::AiProviderCredential>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredential");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    if (!value.contains("configured")) throw std::invalid_argument("missing configured");
    if (!value.contains("api_key_preview")) throw std::invalid_argument("missing api_key_preview");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::AiProviderCredential{
        codec<std::string>::decode(value.at("provider")),
        codec<bool>::decode(value.at("configured")),
        codec<std::string>::decode(value.at("api_key_preview")),
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::AiProviderCredentialsListData>::encode(const holder::api::AiProviderCredentialsListData& value) {
    json result = json::object();
    result["providers"] = codec<std::vector<holder::api::AiProviderCredential>>::encode(value.providers);
    return result;
}
inline holder::api::AiProviderCredentialsListData codec<holder::api::AiProviderCredentialsListData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialsListData");
    if (!value.contains("providers")) throw std::invalid_argument("missing providers");
    return holder::api::AiProviderCredentialsListData{
        codec<std::vector<holder::api::AiProviderCredential>>::decode(value.at("providers")),
    };
}

inline json codec<holder::api::AiProviderCredentialsListResponse>::encode(const holder::api::AiProviderCredentialsListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiProviderCredentialsListData>::encode(value.data);
    return result;
}
inline holder::api::AiProviderCredentialsListResponse codec<holder::api::AiProviderCredentialsListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialsListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiProviderCredentialsListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiProviderCredentialsListData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiProviderCredentialUpsertRequest>::encode(const holder::api::AiProviderCredentialUpsertRequest& value) {
    json result = json::object();
    result["provider"] = codec<std::string>::encode(value.provider);
    result["api_key"] = codec<std::string>::encode(value.api_key);
    if (value.updated_at) {
        result["updated_at"] = codec<std::optional<std::optional<long long>>>::encode(value.updated_at);
    }
    return result;
}
inline holder::api::AiProviderCredentialUpsertRequest codec<holder::api::AiProviderCredentialUpsertRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialUpsertRequest");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    if (!value.contains("api_key")) throw std::invalid_argument("missing api_key");
    return holder::api::AiProviderCredentialUpsertRequest{
        codec<std::string>::decode(value.at("provider")),
        codec<std::string>::decode(value.at("api_key")),
        value.contains("updated_at") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("updated_at"))} : std::nullopt,
    };
}

inline json codec<holder::api::AiProviderCredentialUpsertResponse>::encode(const holder::api::AiProviderCredentialUpsertResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiProviderCredential>::encode(value.data);
    return result;
}
inline holder::api::AiProviderCredentialUpsertResponse codec<holder::api::AiProviderCredentialUpsertResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialUpsertResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiProviderCredentialUpsertResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiProviderCredential>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiProviderCredentialDeleteResponseData>::encode(const holder::api::AiProviderCredentialDeleteResponseData& value) {
    json result = json::object();
    result["provider"] = codec<std::string>::encode(value.provider);
    return result;
}
inline holder::api::AiProviderCredentialDeleteResponseData codec<holder::api::AiProviderCredentialDeleteResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialDeleteResponseData");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    return holder::api::AiProviderCredentialDeleteResponseData{
        codec<std::string>::decode(value.at("provider")),
    };
}

inline json codec<holder::api::AiProviderCredentialDeleteResponse>::encode(const holder::api::AiProviderCredentialDeleteResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiProviderCredentialDeleteResponseData>::encode(value.data);
    return result;
}
inline holder::api::AiProviderCredentialDeleteResponse codec<holder::api::AiProviderCredentialDeleteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCredentialDeleteResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiProviderCredentialDeleteResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiProviderCredentialDeleteResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardLinkDeleteRequest>::encode(const holder::api::CardLinkDeleteRequest& value) {
    json result = json::object();
    if (value.to_card_id) {
        result["to_card_id"] = codec<std::optional<std::string>>::encode(value.to_card_id);
    }
    if (value.target_project_id) {
        result["target_project_id"] = codec<std::optional<std::string>>::encode(value.target_project_id);
    }
    if (value.to_type) {
        result["to_type"] = codec<std::optional<std::string>>::encode(value.to_type);
    }
    if (value.kind) {
        result["kind"] = codec<std::optional<std::string>>::encode(value.kind);
    }
    return result;
}
inline holder::api::CardLinkDeleteRequest codec<holder::api::CardLinkDeleteRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLinkDeleteRequest");
    return holder::api::CardLinkDeleteRequest{
        value.contains("to_card_id") ? codec<std::optional<std::string>>::decode(value.at("to_card_id")) : std::nullopt,
        value.contains("target_project_id") ? codec<std::optional<std::string>>::decode(value.at("target_project_id")) : std::nullopt,
        value.contains("to_type") ? codec<std::optional<std::string>>::decode(value.at("to_type")) : std::nullopt,
        value.contains("kind") ? codec<std::optional<std::string>>::decode(value.at("kind")) : std::nullopt,
    };
}

inline json codec<holder::api::AiLocalModelConfigData>::encode(const holder::api::AiLocalModelConfigData& value) {
    json result = json::object();
    result["fast_model"] = codec<std::optional<std::string>>::encode(value.fast_model);
    result["strong_model"] = codec<std::optional<std::string>>::encode(value.strong_model);
    result["deep_model"] = codec<std::optional<std::string>>::encode(value.deep_model);
    result["updated_at"] = codec<std::optional<long long>>::encode(value.updated_at);
    return result;
}
inline holder::api::AiLocalModelConfigData codec<holder::api::AiLocalModelConfigData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiLocalModelConfigData");
    if (!value.contains("fast_model")) throw std::invalid_argument("missing fast_model");
    if (!value.contains("strong_model")) throw std::invalid_argument("missing strong_model");
    if (!value.contains("deep_model")) throw std::invalid_argument("missing deep_model");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::AiLocalModelConfigData{
        codec<std::optional<std::string>>::decode(value.at("fast_model")),
        codec<std::optional<std::string>>::decode(value.at("strong_model")),
        codec<std::optional<std::string>>::decode(value.at("deep_model")),
        codec<std::optional<long long>>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::LocalModel>::encode(const holder::api::LocalModel& value) {
    json result = json::object();
    result["name"] = codec<std::string>::encode(value.name);
    if (value.runner_id) {
        result["runner_id"] = codec<std::optional<std::string>>::encode(value.runner_id);
    }
    if (value.model_ref) {
        result["model_ref"] = codec<std::optional<std::string>>::encode(value.model_ref);
    }
    if (value.digest) {
        result["digest"] = codec<std::optional<std::string>>::encode(value.digest);
    }
    if (value.size) {
        result["size"] = codec<std::optional<long long>>::encode(value.size);
    }
    if (value.modified_at) {
        result["modified_at"] = codec<std::optional<std::string>>::encode(value.modified_at);
    }
    return result;
}
inline holder::api::LocalModel codec<holder::api::LocalModel>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected LocalModel");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    return holder::api::LocalModel{
        codec<std::string>::decode(value.at("name")),
        value.contains("runner_id") ? codec<std::optional<std::string>>::decode(value.at("runner_id")) : std::nullopt,
        value.contains("model_ref") ? codec<std::optional<std::string>>::decode(value.at("model_ref")) : std::nullopt,
        value.contains("digest") ? codec<std::optional<std::string>>::decode(value.at("digest")) : std::nullopt,
        value.contains("size") ? codec<std::optional<long long>>::decode(value.at("size")) : std::nullopt,
        value.contains("modified_at") ? codec<std::optional<std::string>>::decode(value.at("modified_at")) : std::nullopt,
    };
}

inline json codec<holder::api::AiRunnerRuntime>::encode(const holder::api::AiRunnerRuntime& value) {
    json result = json::object();
    result["configured"] = codec<bool>::encode(value.configured);
    result["available"] = codec<bool>::encode(value.available);
    result["spawn_attempted"] = codec<bool>::encode(value.spawn_attempted);
    result["last_checked"] = codec<long long>::encode(value.last_checked);
    result["version"] = codec<std::optional<std::string>>::encode(value.version);
    result["error"] = codec<std::optional<std::string>>::encode(value.error);
    result["models"] = codec<std::vector<holder::api::LocalModel>>::encode(value.models);
    result["pulls"] = codec<std::vector<holder::api::RunnerPullJob>>::encode(value.pulls);
    return result;
}
inline holder::api::AiRunnerRuntime codec<holder::api::AiRunnerRuntime>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerRuntime");
    if (!value.contains("configured")) throw std::invalid_argument("missing configured");
    if (!value.contains("available")) throw std::invalid_argument("missing available");
    if (!value.contains("spawn_attempted")) throw std::invalid_argument("missing spawn_attempted");
    if (!value.contains("last_checked")) throw std::invalid_argument("missing last_checked");
    if (!value.contains("version")) throw std::invalid_argument("missing version");
    if (!value.contains("error")) throw std::invalid_argument("missing error");
    if (!value.contains("models")) throw std::invalid_argument("missing models");
    if (!value.contains("pulls")) throw std::invalid_argument("missing pulls");
    return holder::api::AiRunnerRuntime{
        codec<bool>::decode(value.at("configured")),
        codec<bool>::decode(value.at("available")),
        codec<bool>::decode(value.at("spawn_attempted")),
        codec<long long>::decode(value.at("last_checked")),
        codec<std::optional<std::string>>::decode(value.at("version")),
        codec<std::optional<std::string>>::decode(value.at("error")),
        codec<std::vector<holder::api::LocalModel>>::decode(value.at("models")),
        codec<std::vector<holder::api::RunnerPullJob>>::decode(value.at("pulls")),
    };
}

inline json codec<holder::api::AiRunner>::encode(const holder::api::AiRunner& value) {
    json result = json::object();
    result["runner_id"] = codec<std::string>::encode(value.runner_id);
    result["name"] = codec<std::string>::encode(value.name);
    result["kind"] = codec<std::string>::encode(value.kind);
    result["base_url"] = codec<std::optional<std::string>>::encode(value.base_url);
    result["source"] = codec<std::string>::encode(value.source);
    result["enabled"] = codec<bool>::encode(value.enabled);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    result["runtime"] = codec<holder::api::AiRunnerRuntime>::encode(value.runtime);
    return result;
}
inline holder::api::AiRunner codec<holder::api::AiRunner>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunner");
    if (!value.contains("runner_id")) throw std::invalid_argument("missing runner_id");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("base_url")) throw std::invalid_argument("missing base_url");
    if (!value.contains("source")) throw std::invalid_argument("missing source");
    if (!value.contains("enabled")) throw std::invalid_argument("missing enabled");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    if (!value.contains("runtime")) throw std::invalid_argument("missing runtime");
    return holder::api::AiRunner{
        codec<std::string>::decode(value.at("runner_id")),
        codec<std::string>::decode(value.at("name")),
        codec<std::string>::decode(value.at("kind")),
        codec<std::optional<std::string>>::decode(value.at("base_url")),
        codec<std::string>::decode(value.at("source")),
        codec<bool>::decode(value.at("enabled")),
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
        codec<holder::api::AiRunnerRuntime>::decode(value.at("runtime")),
    };
}

inline json codec<holder::api::AiRunnerListResponseData>::encode(const holder::api::AiRunnerListResponseData& value) {
    json result = json::object();
    result["runners"] = codec<std::vector<holder::api::AiRunner>>::encode(value.runners);
    return result;
}
inline holder::api::AiRunnerListResponseData codec<holder::api::AiRunnerListResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerListResponseData");
    if (!value.contains("runners")) throw std::invalid_argument("missing runners");
    return holder::api::AiRunnerListResponseData{
        codec<std::vector<holder::api::AiRunner>>::decode(value.at("runners")),
    };
}

inline json codec<holder::api::AiRunnerListResponse>::encode(const holder::api::AiRunnerListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiRunnerListResponseData>::encode(value.data);
    return result;
}
inline holder::api::AiRunnerListResponse codec<holder::api::AiRunnerListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiRunnerListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiRunnerListResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiRunnerResponse>::encode(const holder::api::AiRunnerResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiRunner>::encode(value.data);
    return result;
}
inline holder::api::AiRunnerResponse codec<holder::api::AiRunnerResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiRunnerResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiRunner>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiLocalModelConfigResponse>::encode(const holder::api::AiLocalModelConfigResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiLocalModelConfigData>::encode(value.data);
    return result;
}
inline holder::api::AiLocalModelConfigResponse codec<holder::api::AiLocalModelConfigResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiLocalModelConfigResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiLocalModelConfigResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiLocalModelConfigData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiRunEventData>::encode(const holder::api::AiRunEventData& value) {
    json result = json::object();
    result["run_id"] = codec<std::string>::encode(value.run_id);
    if (value.status) {
        result["status"] = codec<std::optional<std::string>>::encode(value.status);
    }
    if (value.created_at) {
        result["created_at"] = codec<std::optional<long long>>::encode(value.created_at);
    }
    if (value.delta) {
        result["delta"] = codec<std::optional<std::string>>::encode(value.delta);
    }
    if (value.message) {
        result["message"] = codec<std::optional<std::string>>::encode(value.message);
    }
    if (value.error) {
        result["error"] = codec<std::optional<std::string>>::encode(value.error);
    }
    if (value.runner_id) {
        result["runner_id"] = codec<std::optional<std::string>>::encode(value.runner_id);
    }
    if (value.model_ref) {
        result["model_ref"] = codec<std::optional<std::string>>::encode(value.model_ref);
    }
    if (value.model) {
        result["model"] = codec<std::optional<std::string>>::encode(value.model);
    }
    if (value.provider) {
        result["provider"] = codec<std::optional<std::string>>::encode(value.provider);
    }
    if (value.context_compacted) {
        result["context_compacted"] = codec<std::optional<bool>>::encode(value.context_compacted);
    }
    if (value.cooldown_until) {
        result["cooldown_until"] = codec<std::optional<long long>>::encode(value.cooldown_until);
    }
    if (value.router_model) {
        result["router_model"] = codec<std::optional<std::string>>::encode(value.router_model);
    }
    if (value.ranked) {
        result["ranked"] = codec<std::optional<std::vector<std::string>>>::encode(value.ranked);
    }
    if (value.partial) {
        result["partial"] = codec<std::optional<bool>>::encode(value.partial);
    }
    return result;
}
inline holder::api::AiRunEventData codec<holder::api::AiRunEventData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunEventData");
    if (!value.contains("run_id")) throw std::invalid_argument("missing run_id");
    return holder::api::AiRunEventData{
        codec<std::string>::decode(value.at("run_id")),
        value.contains("status") ? codec<std::optional<std::string>>::decode(value.at("status")) : std::nullopt,
        value.contains("created_at") ? codec<std::optional<long long>>::decode(value.at("created_at")) : std::nullopt,
        value.contains("delta") ? codec<std::optional<std::string>>::decode(value.at("delta")) : std::nullopt,
        value.contains("message") ? codec<std::optional<std::string>>::decode(value.at("message")) : std::nullopt,
        value.contains("error") ? codec<std::optional<std::string>>::decode(value.at("error")) : std::nullopt,
        value.contains("runner_id") ? codec<std::optional<std::string>>::decode(value.at("runner_id")) : std::nullopt,
        value.contains("model_ref") ? codec<std::optional<std::string>>::decode(value.at("model_ref")) : std::nullopt,
        value.contains("model") ? codec<std::optional<std::string>>::decode(value.at("model")) : std::nullopt,
        value.contains("provider") ? codec<std::optional<std::string>>::decode(value.at("provider")) : std::nullopt,
        value.contains("context_compacted") ? codec<std::optional<bool>>::decode(value.at("context_compacted")) : std::nullopt,
        value.contains("cooldown_until") ? codec<std::optional<long long>>::decode(value.at("cooldown_until")) : std::nullopt,
        value.contains("router_model") ? codec<std::optional<std::string>>::decode(value.at("router_model")) : std::nullopt,
        value.contains("ranked") ? codec<std::optional<std::vector<std::string>>>::decode(value.at("ranked")) : std::nullopt,
        value.contains("partial") ? codec<std::optional<bool>>::decode(value.at("partial")) : std::nullopt,
    };
}

inline json codec<holder::api::AiRunCreateRequest>::encode(const holder::api::AiRunCreateRequest& value) {
    json result = json::object();
    result["prompt"] = codec<std::string>::encode(value.prompt);
    if (value.mode) {
        result["mode"] = codec<std::optional<std::string>>::encode(value.mode);
    }
    if (value.model) {
        result["model"] = codec<std::optional<std::optional<std::string>>>::encode(value.model);
    }
    if (value.provider) {
        result["provider"] = codec<std::optional<std::optional<std::string>>>::encode(value.provider);
    }
    if (value.project_id) {
        result["project_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.project_id);
    }
    if (value.thread_id) {
        result["thread_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.thread_id);
    }
    if (value.context) {
        result["context"] = codec<std::optional<std::optional<nlohmann::json>>>::encode(value.context);
    }
    if (value.runner_id) {
        result["runner_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.runner_id);
    }
    return result;
}
inline holder::api::AiRunCreateRequest codec<holder::api::AiRunCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunCreateRequest");
    if (!value.contains("prompt")) throw std::invalid_argument("missing prompt");
    return holder::api::AiRunCreateRequest{
        codec<std::string>::decode(value.at("prompt")),
        value.contains("mode") ? codec<std::optional<std::string>>::decode(value.at("mode")) : std::nullopt,
        value.contains("model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("model"))} : std::nullopt,
        value.contains("provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("provider"))} : std::nullopt,
        value.contains("project_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("project_id"))} : std::nullopt,
        value.contains("thread_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("thread_id"))} : std::nullopt,
        value.contains("context") ? std::optional<std::optional<nlohmann::json>>{std::in_place, codec<std::optional<nlohmann::json>>::decode(value.at("context"))} : std::nullopt,
        value.contains("runner_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("runner_id"))} : std::nullopt,
    };
}

inline json codec<holder::api::AiThread>::encode(const holder::api::AiThread& value) {
    json result = json::object();
    result["thread_id"] = codec<std::string>::encode(value.thread_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    if (value.card_id) {
        result["card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.card_id);
    }
    result["title"] = codec<std::string>::encode(value.title);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::AiThread codec<holder::api::AiThread>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThread");
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::AiThread{
        codec<std::string>::decode(value.at("thread_id")),
        codec<std::string>::decode(value.at("project_id")),
        value.contains("card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("card_id"))} : std::nullopt,
        codec<std::string>::decode(value.at("title")),
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::AiLocalModelConfigUpdateRequest>::encode(const holder::api::AiLocalModelConfigUpdateRequest& value) {
    json result = json::object();
    if (value.fast_model) {
        result["fast_model"] = codec<std::optional<std::optional<std::string>>>::encode(value.fast_model);
    }
    if (value.strong_model) {
        result["strong_model"] = codec<std::optional<std::optional<std::string>>>::encode(value.strong_model);
    }
    if (value.deep_model) {
        result["deep_model"] = codec<std::optional<std::optional<std::string>>>::encode(value.deep_model);
    }
    if (value.updated_at) {
        result["updated_at"] = codec<std::optional<std::optional<long long>>>::encode(value.updated_at);
    }
    return result;
}
inline holder::api::AiLocalModelConfigUpdateRequest codec<holder::api::AiLocalModelConfigUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiLocalModelConfigUpdateRequest");
    return holder::api::AiLocalModelConfigUpdateRequest{
        value.contains("fast_model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("fast_model"))} : std::nullopt,
        value.contains("strong_model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("strong_model"))} : std::nullopt,
        value.contains("deep_model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("deep_model"))} : std::nullopt,
        value.contains("updated_at") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("updated_at"))} : std::nullopt,
    };
}

inline json codec<holder::api::AiMessage>::encode(const holder::api::AiMessage& value) {
    json result = json::object();
    result["message_id"] = codec<std::string>::encode(value.message_id);
    result["thread_id"] = codec<std::string>::encode(value.thread_id);
    result["role"] = codec<std::string>::encode(value.role);
    result["source"] = codec<std::string>::encode(value.source);
    result["provider"] = codec<std::optional<std::string>>::encode(value.provider);
    result["model"] = codec<std::optional<std::string>>::encode(value.model);
    result["content"] = codec<std::string>::encode(value.content);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["deleted_at"] = codec<std::optional<long long>>::encode(value.deleted_at);
    result["prompt_hash"] = codec<std::optional<std::string>>::encode(value.prompt_hash);
    result["meta_json"] = codec<std::optional<std::string>>::encode(value.meta_json);
    return result;
}
inline holder::api::AiMessage codec<holder::api::AiMessage>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessage");
    if (!value.contains("message_id")) throw std::invalid_argument("missing message_id");
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    if (!value.contains("role")) throw std::invalid_argument("missing role");
    if (!value.contains("source")) throw std::invalid_argument("missing source");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    if (!value.contains("model")) throw std::invalid_argument("missing model");
    if (!value.contains("content")) throw std::invalid_argument("missing content");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("deleted_at")) throw std::invalid_argument("missing deleted_at");
    if (!value.contains("prompt_hash")) throw std::invalid_argument("missing prompt_hash");
    if (!value.contains("meta_json")) throw std::invalid_argument("missing meta_json");
    return holder::api::AiMessage{
        codec<std::string>::decode(value.at("message_id")),
        codec<std::string>::decode(value.at("thread_id")),
        codec<std::string>::decode(value.at("role")),
        codec<std::string>::decode(value.at("source")),
        codec<std::optional<std::string>>::decode(value.at("provider")),
        codec<std::optional<std::string>>::decode(value.at("model")),
        codec<std::string>::decode(value.at("content")),
        codec<long long>::decode(value.at("created_at")),
        codec<std::optional<long long>>::decode(value.at("deleted_at")),
        codec<std::optional<std::string>>::decode(value.at("prompt_hash")),
        codec<std::optional<std::string>>::decode(value.at("meta_json")),
    };
}

inline json codec<holder::api::AiMessageListResponse>::encode(const holder::api::AiMessageListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::AiMessage>>::encode(value.data);
    return result;
}
inline holder::api::AiMessageListResponse codec<holder::api::AiMessageListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiMessageListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::AiMessage>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiMessageGetResponse>::encode(const holder::api::AiMessageGetResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiMessage>::encode(value.data);
    return result;
}
inline holder::api::AiMessageGetResponse codec<holder::api::AiMessageGetResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageGetResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiMessageGetResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiMessage>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiMessageCreateResponseData>::encode(const holder::api::AiMessageCreateResponseData& value) {
    json result = json::object();
    result["message_id"] = codec<std::string>::encode(value.message_id);
    return result;
}
inline holder::api::AiMessageCreateResponseData codec<holder::api::AiMessageCreateResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCreateResponseData");
    if (!value.contains("message_id")) throw std::invalid_argument("missing message_id");
    return holder::api::AiMessageCreateResponseData{
        codec<std::string>::decode(value.at("message_id")),
    };
}

inline json codec<holder::api::AiMessageCreateResponse>::encode(const holder::api::AiMessageCreateResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiMessageCreateResponseData>::encode(value.data);
    return result;
}
inline holder::api::AiMessageCreateResponse codec<holder::api::AiMessageCreateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCreateResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiMessageCreateResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiMessageCreateResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiMessageCreateRequest>::encode(const holder::api::AiMessageCreateRequest& value) {
    json result = json::object();
    if (value.message_id) {
        result["message_id"] = codec<std::optional<std::string>>::encode(value.message_id);
    }
    result["thread_id"] = codec<std::string>::encode(value.thread_id);
    result["role"] = codec<std::string>::encode(value.role);
    result["source"] = codec<std::string>::encode(value.source);
    if (value.provider) {
        result["provider"] = codec<std::optional<std::optional<std::string>>>::encode(value.provider);
    }
    if (value.model) {
        result["model"] = codec<std::optional<std::optional<std::string>>>::encode(value.model);
    }
    result["content"] = codec<std::string>::encode(value.content);
    if (value.created_at) {
        result["created_at"] = codec<std::optional<long long>>::encode(value.created_at);
    }
    if (value.prompt_hash) {
        result["prompt_hash"] = codec<std::optional<std::optional<std::string>>>::encode(value.prompt_hash);
    }
    if (value.meta_json) {
        result["meta_json"] = codec<std::optional<std::optional<std::string>>>::encode(value.meta_json);
    }
    return result;
}
inline holder::api::AiMessageCreateRequest codec<holder::api::AiMessageCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCreateRequest");
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    if (!value.contains("role")) throw std::invalid_argument("missing role");
    if (!value.contains("source")) throw std::invalid_argument("missing source");
    if (!value.contains("content")) throw std::invalid_argument("missing content");
    return holder::api::AiMessageCreateRequest{
        value.contains("message_id") ? codec<std::optional<std::string>>::decode(value.at("message_id")) : std::nullopt,
        codec<std::string>::decode(value.at("thread_id")),
        codec<std::string>::decode(value.at("role")),
        codec<std::string>::decode(value.at("source")),
        value.contains("provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("provider"))} : std::nullopt,
        value.contains("model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("model"))} : std::nullopt,
        codec<std::string>::decode(value.at("content")),
        value.contains("created_at") ? codec<std::optional<long long>>::decode(value.at("created_at")) : std::nullopt,
        value.contains("prompt_hash") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("prompt_hash"))} : std::nullopt,
        value.contains("meta_json") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("meta_json"))} : std::nullopt,
    };
}

inline json codec<holder::api::AiMessageUpdateResponseData>::encode(const holder::api::AiMessageUpdateResponseData& value) {
    json result = json::object();
    result["message_id"] = codec<std::string>::encode(value.message_id);
    return result;
}
inline holder::api::AiMessageUpdateResponseData codec<holder::api::AiMessageUpdateResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageUpdateResponseData");
    if (!value.contains("message_id")) throw std::invalid_argument("missing message_id");
    return holder::api::AiMessageUpdateResponseData{
        codec<std::string>::decode(value.at("message_id")),
    };
}

inline json codec<holder::api::AiMessageUpdateResponse>::encode(const holder::api::AiMessageUpdateResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiMessageUpdateResponseData>::encode(value.data);
    return result;
}
inline holder::api::AiMessageUpdateResponse codec<holder::api::AiMessageUpdateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageUpdateResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiMessageUpdateResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiMessageUpdateResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiMessageUpdateRequest>::encode(const holder::api::AiMessageUpdateRequest& value) {
    json result = json::object();
    if (value.role) {
        result["role"] = codec<std::optional<std::optional<std::string>>>::encode(value.role);
    }
    if (value.source) {
        result["source"] = codec<std::optional<std::optional<std::string>>>::encode(value.source);
    }
    if (value.provider) {
        result["provider"] = codec<std::optional<std::optional<std::string>>>::encode(value.provider);
    }
    if (value.model) {
        result["model"] = codec<std::optional<std::optional<std::string>>>::encode(value.model);
    }
    if (value.content) {
        result["content"] = codec<std::optional<std::optional<std::string>>>::encode(value.content);
    }
    if (value.created_at) {
        result["created_at"] = codec<std::optional<long long>>::encode(value.created_at);
    }
    if (value.prompt_hash) {
        result["prompt_hash"] = codec<std::optional<std::optional<std::string>>>::encode(value.prompt_hash);
    }
    if (value.meta_json) {
        result["meta_json"] = codec<std::optional<std::optional<std::string>>>::encode(value.meta_json);
    }
    return result;
}
inline holder::api::AiMessageUpdateRequest codec<holder::api::AiMessageUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageUpdateRequest");
    return holder::api::AiMessageUpdateRequest{
        value.contains("role") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("role"))} : std::nullopt,
        value.contains("source") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("source"))} : std::nullopt,
        value.contains("provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("provider"))} : std::nullopt,
        value.contains("model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("model"))} : std::nullopt,
        value.contains("content") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("content"))} : std::nullopt,
        value.contains("created_at") ? codec<std::optional<long long>>::decode(value.at("created_at")) : std::nullopt,
        value.contains("prompt_hash") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("prompt_hash"))} : std::nullopt,
        value.contains("meta_json") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("meta_json"))} : std::nullopt,
    };
}

inline json codec<holder::api::AiMessageCaptureResponseData>::encode(const holder::api::AiMessageCaptureResponseData& value) {
    json result = json::object();
    result["thread_id"] = codec<std::string>::encode(value.thread_id);
    result["user_message_id"] = codec<std::string>::encode(value.user_message_id);
    result["assistant_message_id"] = codec<std::string>::encode(value.assistant_message_id);
    return result;
}
inline holder::api::AiMessageCaptureResponseData codec<holder::api::AiMessageCaptureResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCaptureResponseData");
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    if (!value.contains("user_message_id")) throw std::invalid_argument("missing user_message_id");
    if (!value.contains("assistant_message_id")) throw std::invalid_argument("missing assistant_message_id");
    return holder::api::AiMessageCaptureResponseData{
        codec<std::string>::decode(value.at("thread_id")),
        codec<std::string>::decode(value.at("user_message_id")),
        codec<std::string>::decode(value.at("assistant_message_id")),
    };
}

inline json codec<holder::api::AiMessageCaptureResponse>::encode(const holder::api::AiMessageCaptureResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiMessageCaptureResponseData>::encode(value.data);
    return result;
}
inline holder::api::AiMessageCaptureResponse codec<holder::api::AiMessageCaptureResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCaptureResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiMessageCaptureResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiMessageCaptureResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiMessageCaptureRequest>::encode(const holder::api::AiMessageCaptureRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    if (value.thread_id) {
        result["thread_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.thread_id);
    }
    result["prompt"] = codec<std::string>::encode(value.prompt);
    result["response"] = codec<std::string>::encode(value.response);
    if (value.source) {
        result["source"] = codec<std::optional<std::optional<std::string>>>::encode(value.source);
    }
    if (value.provider) {
        result["provider"] = codec<std::optional<std::optional<std::string>>>::encode(value.provider);
    }
    if (value.model) {
        result["model"] = codec<std::optional<std::optional<std::string>>>::encode(value.model);
    }
    if (value.url) {
        result["url"] = codec<std::optional<std::optional<std::string>>>::encode(value.url);
    }
    if (value.context) {
        result["context"] = codec<std::optional<std::optional<nlohmann::json>>>::encode(value.context);
    }
    if (value.created_at) {
        result["created_at"] = codec<std::optional<std::optional<long long>>>::encode(value.created_at);
    }
    return result;
}
inline holder::api::AiMessageCaptureRequest codec<holder::api::AiMessageCaptureRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessageCaptureRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("prompt")) throw std::invalid_argument("missing prompt");
    if (!value.contains("response")) throw std::invalid_argument("missing response");
    return holder::api::AiMessageCaptureRequest{
        codec<std::string>::decode(value.at("project_id")),
        value.contains("thread_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("thread_id"))} : std::nullopt,
        codec<std::string>::decode(value.at("prompt")),
        codec<std::string>::decode(value.at("response")),
        value.contains("source") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("source"))} : std::nullopt,
        value.contains("provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("provider"))} : std::nullopt,
        value.contains("model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("model"))} : std::nullopt,
        value.contains("url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("url"))} : std::nullopt,
        value.contains("context") ? std::optional<std::optional<nlohmann::json>>{std::in_place, codec<std::optional<nlohmann::json>>::decode(value.at("context"))} : std::nullopt,
        value.contains("created_at") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("created_at"))} : std::nullopt,
    };
}

inline json codec<holder::api::AiNudge>::encode(const holder::api::AiNudge& value) {
    json result = json::object();
    result["nudge_id"] = codec<std::string>::encode(value.nudge_id);
    result["kind"] = codec<std::string>::encode(value.kind);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["card_id"] = codec<std::optional<std::string>>::encode(value.card_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["body"] = codec<std::string>::encode(value.body);
    result["meta_json"] = codec<nlohmann::json>::encode(value.meta_json);
    result["basis_fingerprint"] = codec<std::optional<std::string>>::encode(value.basis_fingerprint);
    result["basis_commit"] = codec<std::optional<std::string>>::encode(value.basis_commit);
    result["created_at"] = codec<long long>::encode(value.created_at);
    if (value.suggestions) {
        result["suggestions"] = codec<std::optional<std::vector<std::string>>>::encode(value.suggestions);
    }
    return result;
}
inline holder::api::AiNudge codec<holder::api::AiNudge>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudge");
    if (!value.contains("nudge_id")) throw std::invalid_argument("missing nudge_id");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("body")) throw std::invalid_argument("missing body");
    if (!value.contains("meta_json")) throw std::invalid_argument("missing meta_json");
    if (!value.contains("basis_fingerprint")) throw std::invalid_argument("missing basis_fingerprint");
    if (!value.contains("basis_commit")) throw std::invalid_argument("missing basis_commit");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    return holder::api::AiNudge{
        codec<std::string>::decode(value.at("nudge_id")),
        codec<std::string>::decode(value.at("kind")),
        codec<std::string>::decode(value.at("project_id")),
        codec<std::optional<std::string>>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("title")),
        codec<std::string>::decode(value.at("body")),
        codec<nlohmann::json>::decode(value.at("meta_json")),
        codec<std::optional<std::string>>::decode(value.at("basis_fingerprint")),
        codec<std::optional<std::string>>::decode(value.at("basis_commit")),
        codec<long long>::decode(value.at("created_at")),
        value.contains("suggestions") ? codec<std::optional<std::vector<std::string>>>::decode(value.at("suggestions")) : std::nullopt,
    };
}

inline json codec<holder::api::AiNudgeListData>::encode(const holder::api::AiNudgeListData& value) {
    json result = json::object();
    result["nudges"] = codec<std::vector<holder::api::AiNudge>>::encode(value.nudges);
    return result;
}
inline holder::api::AiNudgeListData codec<holder::api::AiNudgeListData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeListData");
    if (!value.contains("nudges")) throw std::invalid_argument("missing nudges");
    return holder::api::AiNudgeListData{
        codec<std::vector<holder::api::AiNudge>>::decode(value.at("nudges")),
    };
}

inline json codec<holder::api::AiNudgeListResponse>::encode(const holder::api::AiNudgeListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiNudgeListData>::encode(value.data);
    return result;
}
inline holder::api::AiNudgeListResponse codec<holder::api::AiNudgeListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiNudgeListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiNudgeListData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiNudgeDismissData>::encode(const holder::api::AiNudgeDismissData& value) {
    json result = json::object();
    result["nudge_id"] = codec<std::string>::encode(value.nudge_id);
    if (value.dismissed != true) throw std::invalid_argument("invalid dismissed");
    result["dismissed"] = codec<bool>::encode(value.dismissed);
    return result;
}
inline holder::api::AiNudgeDismissData codec<holder::api::AiNudgeDismissData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeDismissData");
    if (!value.contains("nudge_id")) throw std::invalid_argument("missing nudge_id");
    if (!value.contains("dismissed")) throw std::invalid_argument("missing dismissed");
    if (value.at("dismissed") != true) throw std::invalid_argument("invalid dismissed");
    return holder::api::AiNudgeDismissData{
        codec<std::string>::decode(value.at("nudge_id")),
        codec<bool>::decode(value.at("dismissed")),
    };
}

inline json codec<holder::api::AiNudgeDismissResponse>::encode(const holder::api::AiNudgeDismissResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiNudgeDismissData>::encode(value.data);
    return result;
}
inline holder::api::AiNudgeDismissResponse codec<holder::api::AiNudgeDismissResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeDismissResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiNudgeDismissResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiNudgeDismissData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiNudgeCandidate>::encode(const holder::api::AiNudgeCandidate& value) {
    json result = json::object();
    result["kind"] = codec<std::string>::encode(value.kind);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    if (value.card_id) {
        result["card_id"] = codec<std::optional<std::string>>::encode(value.card_id);
    }
    result["created_at"] = codec<long long>::encode(value.created_at);
    if (value.basis_fingerprint) {
        result["basis_fingerprint"] = codec<std::optional<std::string>>::encode(value.basis_fingerprint);
    }
    if (value.basis_commit) {
        result["basis_commit"] = codec<std::optional<std::string>>::encode(value.basis_commit);
    }
    result["facts"] = codec<nlohmann::json>::encode(value.facts);
    return result;
}
inline holder::api::AiNudgeCandidate codec<holder::api::AiNudgeCandidate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeCandidate");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("facts")) throw std::invalid_argument("missing facts");
    if (value.contains("kind") && value.at("kind").is_string()) {
        const auto length = value.at("kind").get_ref<const std::string&>().size();
        if (length < 1 || length > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid length: kind");
    }
    if (value.contains("project_id") && value.at("project_id").is_string()) {
        const auto length = value.at("project_id").get_ref<const std::string&>().size();
        if (length < 1 || length > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid length: project_id");
    }
    return holder::api::AiNudgeCandidate{
        codec<std::string>::decode(value.at("kind")),
        codec<std::string>::decode(value.at("project_id")),
        value.contains("card_id") ? codec<std::optional<std::string>>::decode(value.at("card_id")) : std::nullopt,
        codec<long long>::decode(value.at("created_at")),
        value.contains("basis_fingerprint") ? codec<std::optional<std::string>>::decode(value.at("basis_fingerprint")) : std::nullopt,
        value.contains("basis_commit") ? codec<std::optional<std::string>>::decode(value.at("basis_commit")) : std::nullopt,
        codec<nlohmann::json>::decode(value.at("facts")),
    };
}

inline json codec<holder::api::AiNudgeEvaluateData>::encode(const holder::api::AiNudgeEvaluateData& value) {
    json result = json::object();
    result["kind"] = codec<std::string>::encode(value.kind);
    result["accepted"] = codec<bool>::encode(value.accepted);
    result["should_nudge"] = codec<bool>::encode(value.should_nudge);
    result["reason"] = codec<std::string>::encode(value.reason);
    result["nudge"] = codec<std::optional<holder::api::AiNudge>>::encode(value.nudge);
    return result;
}
inline holder::api::AiNudgeEvaluateData codec<holder::api::AiNudgeEvaluateData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeEvaluateData");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("accepted")) throw std::invalid_argument("missing accepted");
    if (!value.contains("should_nudge")) throw std::invalid_argument("missing should_nudge");
    if (!value.contains("reason")) throw std::invalid_argument("missing reason");
    if (!value.contains("nudge")) throw std::invalid_argument("missing nudge");
    return holder::api::AiNudgeEvaluateData{
        codec<std::string>::decode(value.at("kind")),
        codec<bool>::decode(value.at("accepted")),
        codec<bool>::decode(value.at("should_nudge")),
        codec<std::string>::decode(value.at("reason")),
        codec<std::optional<holder::api::AiNudge>>::decode(value.at("nudge")),
    };
}

inline json codec<holder::api::AiNudgeEvaluateResponse>::encode(const holder::api::AiNudgeEvaluateResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiNudgeEvaluateData>::encode(value.data);
    return result;
}
inline holder::api::AiNudgeEvaluateResponse codec<holder::api::AiNudgeEvaluateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiNudgeEvaluateResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiNudgeEvaluateResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiNudgeEvaluateData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiProviderCatalogModelLimits>::encode(const holder::api::AiProviderCatalogModelLimits& value) {
    json result = json::object();
    result["rpm"] = codec<long long>::encode(value.rpm);
    result["tpm"] = codec<long long>::encode(value.tpm);
    result["rpd"] = codec<long long>::encode(value.rpd);
    return result;
}
inline holder::api::AiProviderCatalogModelLimits codec<holder::api::AiProviderCatalogModelLimits>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogModelLimits");
    if (!value.contains("rpm")) throw std::invalid_argument("missing rpm");
    if (!value.contains("tpm")) throw std::invalid_argument("missing tpm");
    if (!value.contains("rpd")) throw std::invalid_argument("missing rpd");
    return holder::api::AiProviderCatalogModelLimits{
        codec<long long>::decode(value.at("rpm")),
        codec<long long>::decode(value.at("tpm")),
        codec<long long>::decode(value.at("rpd")),
    };
}

inline json codec<holder::api::AiProviderCatalogModel>::encode(const holder::api::AiProviderCatalogModel& value) {
    json result = json::object();
    result["id"] = codec<std::string>::encode(value.id);
    result["endpoint"] = codec<std::string>::encode(value.endpoint);
    result["role"] = codec<std::optional<std::string>>::encode(value.role);
    result["cost_tier"] = codec<std::optional<std::string>>::encode(value.cost_tier);
    result["default_for_low_budget"] = codec<bool>::encode(value.default_for_low_budget);
    result["limits"] = codec<holder::api::AiProviderCatalogModelLimits>::encode(value.limits);
    return result;
}
inline holder::api::AiProviderCatalogModel codec<holder::api::AiProviderCatalogModel>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogModel");
    if (!value.contains("id")) throw std::invalid_argument("missing id");
    if (!value.contains("endpoint")) throw std::invalid_argument("missing endpoint");
    if (!value.contains("role")) throw std::invalid_argument("missing role");
    if (!value.contains("cost_tier")) throw std::invalid_argument("missing cost_tier");
    if (!value.contains("default_for_low_budget")) throw std::invalid_argument("missing default_for_low_budget");
    if (!value.contains("limits")) throw std::invalid_argument("missing limits");
    return holder::api::AiProviderCatalogModel{
        codec<std::string>::decode(value.at("id")),
        codec<std::string>::decode(value.at("endpoint")),
        codec<std::optional<std::string>>::decode(value.at("role")),
        codec<std::optional<std::string>>::decode(value.at("cost_tier")),
        codec<bool>::decode(value.at("default_for_low_budget")),
        codec<holder::api::AiProviderCatalogModelLimits>::decode(value.at("limits")),
    };
}

inline json codec<holder::api::AiProviderCatalogApi>::encode(const holder::api::AiProviderCatalogApi& value) {
    json result = json::object();
    if (value.base_url) {
        result["base_url"] = codec<std::optional<std::string>>::encode(value.base_url);
    }
    if (value.kind) {
        result["kind"] = codec<std::optional<std::string>>::encode(value.kind);
    }
    return result;
}
inline holder::api::AiProviderCatalogApi codec<holder::api::AiProviderCatalogApi>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogApi");
    return holder::api::AiProviderCatalogApi{
        value.contains("base_url") ? codec<std::optional<std::string>>::decode(value.at("base_url")) : std::nullopt,
        value.contains("kind") ? codec<std::optional<std::string>>::decode(value.at("kind")) : std::nullopt,
    };
}

inline json codec<holder::api::AiProviderCatalogAuth>::encode(const holder::api::AiProviderCatalogAuth& value) {
    json result = json::object();
    if (value.type) {
        result["type"] = codec<std::optional<std::string>>::encode(value.type);
    }
    if (value.key_param) {
        result["key_param"] = codec<std::optional<std::string>>::encode(value.key_param);
    }
    if (value.header_name) {
        result["header_name"] = codec<std::optional<std::string>>::encode(value.header_name);
    }
    if (value.bearer_prefix) {
        result["bearer_prefix"] = codec<std::optional<std::string>>::encode(value.bearer_prefix);
    }
    return result;
}
inline holder::api::AiProviderCatalogAuth codec<holder::api::AiProviderCatalogAuth>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogAuth");
    return holder::api::AiProviderCatalogAuth{
        value.contains("type") ? codec<std::optional<std::string>>::decode(value.at("type")) : std::nullopt,
        value.contains("key_param") ? codec<std::optional<std::string>>::decode(value.at("key_param")) : std::nullopt,
        value.contains("header_name") ? codec<std::optional<std::string>>::decode(value.at("header_name")) : std::nullopt,
        value.contains("bearer_prefix") ? codec<std::optional<std::string>>::decode(value.at("bearer_prefix")) : std::nullopt,
    };
}

inline json codec<holder::api::AiProviderCatalogProvider>::encode(const holder::api::AiProviderCatalogProvider& value) {
    json result = json::object();
    result["id"] = codec<std::string>::encode(value.id);
    result["display_name"] = codec<std::string>::encode(value.display_name);
    result["enabled"] = codec<bool>::encode(value.enabled);
    result["configured"] = codec<bool>::encode(value.configured);
    result["cost_tier"] = codec<std::optional<std::string>>::encode(value.cost_tier);
    result["setup_url"] = codec<std::optional<std::string>>::encode(value.setup_url);
    result["docs_url"] = codec<std::optional<std::string>>::encode(value.docs_url);
    result["api_key_label"] = codec<std::optional<std::string>>::encode(value.api_key_label);
    result["api_key_hint"] = codec<std::optional<std::string>>::encode(value.api_key_hint);
    result["api"] = codec<holder::api::AiProviderCatalogApi>::encode(value.api);
    result["auth"] = codec<holder::api::AiProviderCatalogAuth>::encode(value.auth);
    result["models"] = codec<std::vector<holder::api::AiProviderCatalogModel>>::encode(value.models);
    return result;
}
inline holder::api::AiProviderCatalogProvider codec<holder::api::AiProviderCatalogProvider>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogProvider");
    if (!value.contains("id")) throw std::invalid_argument("missing id");
    if (!value.contains("display_name")) throw std::invalid_argument("missing display_name");
    if (!value.contains("enabled")) throw std::invalid_argument("missing enabled");
    if (!value.contains("configured")) throw std::invalid_argument("missing configured");
    if (!value.contains("cost_tier")) throw std::invalid_argument("missing cost_tier");
    if (!value.contains("setup_url")) throw std::invalid_argument("missing setup_url");
    if (!value.contains("docs_url")) throw std::invalid_argument("missing docs_url");
    if (!value.contains("api_key_label")) throw std::invalid_argument("missing api_key_label");
    if (!value.contains("api_key_hint")) throw std::invalid_argument("missing api_key_hint");
    if (!value.contains("api")) throw std::invalid_argument("missing api");
    if (!value.contains("auth")) throw std::invalid_argument("missing auth");
    if (!value.contains("models")) throw std::invalid_argument("missing models");
    return holder::api::AiProviderCatalogProvider{
        codec<std::string>::decode(value.at("id")),
        codec<std::string>::decode(value.at("display_name")),
        codec<bool>::decode(value.at("enabled")),
        codec<bool>::decode(value.at("configured")),
        codec<std::optional<std::string>>::decode(value.at("cost_tier")),
        codec<std::optional<std::string>>::decode(value.at("setup_url")),
        codec<std::optional<std::string>>::decode(value.at("docs_url")),
        codec<std::optional<std::string>>::decode(value.at("api_key_label")),
        codec<std::optional<std::string>>::decode(value.at("api_key_hint")),
        codec<holder::api::AiProviderCatalogApi>::decode(value.at("api")),
        codec<holder::api::AiProviderCatalogAuth>::decode(value.at("auth")),
        codec<std::vector<holder::api::AiProviderCatalogModel>>::decode(value.at("models")),
    };
}

inline json codec<holder::api::AiProviderCatalogResponseData>::encode(const holder::api::AiProviderCatalogResponseData& value) {
    json result = json::object();
    result["default_provider"] = codec<std::optional<std::string>>::encode(value.default_provider);
    result["providers"] = codec<std::vector<holder::api::AiProviderCatalogProvider>>::encode(value.providers);
    return result;
}
inline holder::api::AiProviderCatalogResponseData codec<holder::api::AiProviderCatalogResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogResponseData");
    if (!value.contains("default_provider")) throw std::invalid_argument("missing default_provider");
    if (!value.contains("providers")) throw std::invalid_argument("missing providers");
    return holder::api::AiProviderCatalogResponseData{
        codec<std::optional<std::string>>::decode(value.at("default_provider")),
        codec<std::vector<holder::api::AiProviderCatalogProvider>>::decode(value.at("providers")),
    };
}

inline json codec<holder::api::AiProviderCatalogResponse>::encode(const holder::api::AiProviderCatalogResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiProviderCatalogResponseData>::encode(value.data);
    return result;
}
inline holder::api::AiProviderCatalogResponse codec<holder::api::AiProviderCatalogResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiProviderCatalogResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiProviderCatalogResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiRunnerCreateRequest>::encode(const holder::api::AiRunnerCreateRequest& value) {
    json result = json::object();
    result["name"] = codec<std::string>::encode(value.name);
    if (value.kind != "ollama") throw std::invalid_argument("invalid kind");
    result["kind"] = codec<std::string>::encode(value.kind);
    result["base_url"] = codec<std::string>::encode(value.base_url);
    if (value.enabled) {
        result["enabled"] = codec<std::optional<bool>>::encode(value.enabled);
    }
    return result;
}
inline holder::api::AiRunnerCreateRequest codec<holder::api::AiRunnerCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerCreateRequest");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("base_url")) throw std::invalid_argument("missing base_url");
    if (value.at("kind") != "ollama") throw std::invalid_argument("invalid kind");
    return holder::api::AiRunnerCreateRequest{
        codec<std::string>::decode(value.at("name")),
        codec<std::string>::decode(value.at("kind")),
        codec<std::string>::decode(value.at("base_url")),
        value.contains("enabled") ? codec<std::optional<bool>>::decode(value.at("enabled")) : std::nullopt,
    };
}

inline json codec<holder::api::AiRunnerUpdateRequest>::encode(const holder::api::AiRunnerUpdateRequest& value) {
    json result = json::object();
    if (value.name) {
        result["name"] = codec<std::optional<std::string>>::encode(value.name);
    }
    if (value.base_url) {
        result["base_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.base_url);
    }
    if (value.enabled) {
        result["enabled"] = codec<std::optional<bool>>::encode(value.enabled);
    }
    return result;
}
inline holder::api::AiRunnerUpdateRequest codec<holder::api::AiRunnerUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerUpdateRequest");
    return holder::api::AiRunnerUpdateRequest{
        value.contains("name") ? codec<std::optional<std::string>>::decode(value.at("name")) : std::nullopt,
        value.contains("base_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("base_url"))} : std::nullopt,
        value.contains("enabled") ? codec<std::optional<bool>>::decode(value.at("enabled")) : std::nullopt,
    };
}

inline json codec<holder::api::AiRunnerDeleteResponseData>::encode(const holder::api::AiRunnerDeleteResponseData& value) {
    json result = json::object();
    result["runner_id"] = codec<std::string>::encode(value.runner_id);
    return result;
}
inline holder::api::AiRunnerDeleteResponseData codec<holder::api::AiRunnerDeleteResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerDeleteResponseData");
    if (!value.contains("runner_id")) throw std::invalid_argument("missing runner_id");
    return holder::api::AiRunnerDeleteResponseData{
        codec<std::string>::decode(value.at("runner_id")),
    };
}

inline json codec<holder::api::AiRunnerDeleteResponse>::encode(const holder::api::AiRunnerDeleteResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiRunnerDeleteResponseData>::encode(value.data);
    return result;
}
inline holder::api::AiRunnerDeleteResponse codec<holder::api::AiRunnerDeleteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunnerDeleteResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiRunnerDeleteResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiRunnerDeleteResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiRunItem>::encode(const holder::api::AiRunItem& value) {
    json result = json::object();
    result["run_id"] = codec<std::string>::encode(value.run_id);
    if (value.project_id) {
        result["project_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.project_id);
    }
    if (value.thread_id) {
        result["thread_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.thread_id);
    }
    if (value.message_id) {
        result["message_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.message_id);
    }
    result["mode"] = codec<std::string>::encode(value.mode);
    result["prompt"] = codec<std::string>::encode(value.prompt);
    if (value.context_json) {
        result["context_json"] = codec<std::optional<std::optional<std::string>>>::encode(value.context_json);
    }
    if (value.router_model) {
        result["router_model"] = codec<std::optional<std::optional<std::string>>>::encode(value.router_model);
    }
    if (value.router_runner_id) {
        result["router_runner_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.router_runner_id);
    }
    if (value.ranked_json) {
        result["ranked_json"] = codec<std::optional<std::optional<std::string>>>::encode(value.ranked_json);
    }
    if (value.policy_trace_json) {
        result["policy_trace_json"] = codec<std::optional<std::optional<std::string>>>::encode(value.policy_trace_json);
    }
    if (value.policy_trace) {
        result["policy_trace"] = codec<std::optional<std::optional<nlohmann::json>>>::encode(value.policy_trace);
    }
    if (value.chosen_model) {
        result["chosen_model"] = codec<std::optional<std::optional<std::string>>>::encode(value.chosen_model);
    }
    if (value.chosen_runner_id) {
        result["chosen_runner_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.chosen_runner_id);
    }
    result["status"] = codec<std::string>::encode(value.status);
    if (value.error) {
        result["error"] = codec<std::optional<std::optional<std::string>>>::encode(value.error);
    }
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::AiRunItem codec<holder::api::AiRunItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunItem");
    if (!value.contains("run_id")) throw std::invalid_argument("missing run_id");
    if (!value.contains("mode")) throw std::invalid_argument("missing mode");
    if (!value.contains("prompt")) throw std::invalid_argument("missing prompt");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::AiRunItem{
        codec<std::string>::decode(value.at("run_id")),
        value.contains("project_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("project_id"))} : std::nullopt,
        value.contains("thread_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("thread_id"))} : std::nullopt,
        value.contains("message_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("message_id"))} : std::nullopt,
        codec<std::string>::decode(value.at("mode")),
        codec<std::string>::decode(value.at("prompt")),
        value.contains("context_json") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("context_json"))} : std::nullopt,
        value.contains("router_model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("router_model"))} : std::nullopt,
        value.contains("router_runner_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("router_runner_id"))} : std::nullopt,
        value.contains("ranked_json") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("ranked_json"))} : std::nullopt,
        value.contains("policy_trace_json") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("policy_trace_json"))} : std::nullopt,
        value.contains("policy_trace") ? std::optional<std::optional<nlohmann::json>>{std::in_place, codec<std::optional<nlohmann::json>>::decode(value.at("policy_trace"))} : std::nullopt,
        value.contains("chosen_model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("chosen_model"))} : std::nullopt,
        value.contains("chosen_runner_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("chosen_runner_id"))} : std::nullopt,
        codec<std::string>::decode(value.at("status")),
        value.contains("error") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("error"))} : std::nullopt,
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::AiRunListResponse>::encode(const holder::api::AiRunListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::AiRunItem>>::encode(value.data);
    return result;
}
inline holder::api::AiRunListResponse codec<holder::api::AiRunListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiRunListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::AiRunItem>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiRunGetResponse>::encode(const holder::api::AiRunGetResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiRunItem>::encode(value.data);
    return result;
}
inline holder::api::AiRunGetResponse codec<holder::api::AiRunGetResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunGetResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiRunGetResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiRunItem>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CasteInfo>::encode(const holder::api::CasteInfo& value) {
    json result = json::object();
    if (value.name != "mini" && value.name != "user" && value.name != "developer" && value.name != "workstation" && value.name != "rig") throw std::invalid_argument("invalid name");
    result["name"] = codec<std::string>::encode(value.name);
    result["reason"] = codec<std::optional<std::string>>::encode(value.reason);
    return result;
}
inline holder::api::CasteInfo codec<holder::api::CasteInfo>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CasteInfo");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    if (!value.contains("reason")) throw std::invalid_argument("missing reason");
    if (value.at("name") != "mini" && value.at("name") != "user" && value.at("name") != "developer" && value.at("name") != "workstation" && value.at("name") != "rig") throw std::invalid_argument("invalid name");
    return holder::api::CasteInfo{
        codec<std::string>::decode(value.at("name")),
        codec<std::optional<std::string>>::decode(value.at("reason")),
    };
}

inline json codec<holder::api::CasteRecommendedModel>::encode(const holder::api::CasteRecommendedModel& value) {
    json result = json::object();
    result["tag"] = codec<std::string>::encode(value.tag);
    result["provider"] = codec<std::optional<std::string>>::encode(value.provider);
    result["engine"] = codec<std::optional<std::string>>::encode(value.engine);
    result["category"] = codec<std::optional<std::string>>::encode(value.category);
    if (value.required_caste != "mini" && value.required_caste != "user" && value.required_caste != "developer" && value.required_caste != "workstation" && value.required_caste != "rig") throw std::invalid_argument("invalid required_caste");
    result["required_caste"] = codec<std::string>::encode(value.required_caste);
    result["installed"] = codec<bool>::encode(value.installed);
    return result;
}
inline holder::api::CasteRecommendedModel codec<holder::api::CasteRecommendedModel>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CasteRecommendedModel");
    if (!value.contains("tag")) throw std::invalid_argument("missing tag");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    if (!value.contains("engine")) throw std::invalid_argument("missing engine");
    if (!value.contains("category")) throw std::invalid_argument("missing category");
    if (!value.contains("required_caste")) throw std::invalid_argument("missing required_caste");
    if (!value.contains("installed")) throw std::invalid_argument("missing installed");
    if (value.at("required_caste") != "mini" && value.at("required_caste") != "user" && value.at("required_caste") != "developer" && value.at("required_caste") != "workstation" && value.at("required_caste") != "rig") throw std::invalid_argument("invalid required_caste");
    return holder::api::CasteRecommendedModel{
        codec<std::string>::decode(value.at("tag")),
        codec<std::optional<std::string>>::decode(value.at("provider")),
        codec<std::optional<std::string>>::decode(value.at("engine")),
        codec<std::optional<std::string>>::decode(value.at("category")),
        codec<std::string>::decode(value.at("required_caste")),
        codec<bool>::decode(value.at("installed")),
    };
}

inline json codec<holder::api::RunnerCapabilitiesData>::encode(const holder::api::RunnerCapabilitiesData& value) {
    json result = json::object();
    result["local_model_config"] = codec<holder::api::AiLocalModelConfigData>::encode(value.local_model_config);
    if (value.caste) {
        result["caste"] = codec<std::optional<std::optional<holder::api::CasteInfo>>>::encode(value.caste);
    }
    result["runners"] = codec<std::vector<holder::api::AiRunner>>::encode(value.runners);
    result["recommended_models"] = codec<std::vector<holder::api::CasteRecommendedModel>>::encode(value.recommended_models);
    result["recommended_install"] = codec<std::vector<holder::api::CasteRecommendedModel>>::encode(value.recommended_install);
    return result;
}
inline holder::api::RunnerCapabilitiesData codec<holder::api::RunnerCapabilitiesData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerCapabilitiesData");
    if (!value.contains("local_model_config")) throw std::invalid_argument("missing local_model_config");
    if (!value.contains("runners")) throw std::invalid_argument("missing runners");
    if (!value.contains("recommended_models")) throw std::invalid_argument("missing recommended_models");
    if (!value.contains("recommended_install")) throw std::invalid_argument("missing recommended_install");
    return holder::api::RunnerCapabilitiesData{
        codec<holder::api::AiLocalModelConfigData>::decode(value.at("local_model_config")),
        value.contains("caste") ? std::optional<std::optional<holder::api::CasteInfo>>{std::in_place, codec<std::optional<holder::api::CasteInfo>>::decode(value.at("caste"))} : std::nullopt,
        codec<std::vector<holder::api::AiRunner>>::decode(value.at("runners")),
        codec<std::vector<holder::api::CasteRecommendedModel>>::decode(value.at("recommended_models")),
        codec<std::vector<holder::api::CasteRecommendedModel>>::decode(value.at("recommended_install")),
    };
}

inline json codec<holder::api::RunnerCapabilitiesResponse>::encode(const holder::api::RunnerCapabilitiesResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::RunnerCapabilitiesData>::encode(value.data);
    return result;
}
inline holder::api::RunnerCapabilitiesResponse codec<holder::api::RunnerCapabilitiesResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerCapabilitiesResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::RunnerCapabilitiesResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::RunnerCapabilitiesData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiStatusData>::encode(const holder::api::AiStatusData& value) {
    json result = json::object();
    result["checked_at"] = codec<long long>::encode(value.checked_at);
    result["active_runs"] = codec<long long>::encode(value.active_runs);
    result["active_pull_jobs"] = codec<long long>::encode(value.active_pull_jobs);
    result["runners"] = codec<std::vector<holder::api::AiRunner>>::encode(value.runners);
    result["cloud"] = codec<std::vector<holder::api::AiProviderCredential>>::encode(value.cloud);
    result["cloud_configured_providers"] = codec<long long>::encode(value.cloud_configured_providers);
    return result;
}
inline holder::api::AiStatusData codec<holder::api::AiStatusData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiStatusData");
    if (!value.contains("checked_at")) throw std::invalid_argument("missing checked_at");
    if (!value.contains("active_runs")) throw std::invalid_argument("missing active_runs");
    if (!value.contains("active_pull_jobs")) throw std::invalid_argument("missing active_pull_jobs");
    if (!value.contains("runners")) throw std::invalid_argument("missing runners");
    if (!value.contains("cloud")) throw std::invalid_argument("missing cloud");
    if (!value.contains("cloud_configured_providers")) throw std::invalid_argument("missing cloud_configured_providers");
    return holder::api::AiStatusData{
        codec<long long>::decode(value.at("checked_at")),
        codec<long long>::decode(value.at("active_runs")),
        codec<long long>::decode(value.at("active_pull_jobs")),
        codec<std::vector<holder::api::AiRunner>>::decode(value.at("runners")),
        codec<std::vector<holder::api::AiProviderCredential>>::decode(value.at("cloud")),
        codec<long long>::decode(value.at("cloud_configured_providers")),
    };
}

inline json codec<holder::api::AiStatusResponse>::encode(const holder::api::AiStatusResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiStatusData>::encode(value.data);
    return result;
}
inline holder::api::AiStatusResponse codec<holder::api::AiStatusResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiStatusResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiStatusResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiStatusData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiThreadListResponse>::encode(const holder::api::AiThreadListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::AiThread>>::encode(value.data);
    return result;
}
inline holder::api::AiThreadListResponse codec<holder::api::AiThreadListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiThreadListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::AiThread>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiThreadGetResponse>::encode(const holder::api::AiThreadGetResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiThread>::encode(value.data);
    return result;
}
inline holder::api::AiThreadGetResponse codec<holder::api::AiThreadGetResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadGetResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiThreadGetResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiThread>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiThreadCreateResponseData>::encode(const holder::api::AiThreadCreateResponseData& value) {
    json result = json::object();
    result["thread_id"] = codec<std::string>::encode(value.thread_id);
    return result;
}
inline holder::api::AiThreadCreateResponseData codec<holder::api::AiThreadCreateResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadCreateResponseData");
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    return holder::api::AiThreadCreateResponseData{
        codec<std::string>::decode(value.at("thread_id")),
    };
}

inline json codec<holder::api::AiThreadCreateResponse>::encode(const holder::api::AiThreadCreateResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiThreadCreateResponseData>::encode(value.data);
    return result;
}
inline holder::api::AiThreadCreateResponse codec<holder::api::AiThreadCreateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadCreateResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiThreadCreateResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiThreadCreateResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiThreadCreateRequest>::encode(const holder::api::AiThreadCreateRequest& value) {
    json result = json::object();
    if (value.thread_id) {
        result["thread_id"] = codec<std::optional<std::string>>::encode(value.thread_id);
    }
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["title"] = codec<std::string>::encode(value.title);
    if (value.card_id) {
        result["card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.card_id);
    }
    if (value.created_at) {
        result["created_at"] = codec<std::optional<long long>>::encode(value.created_at);
    }
    if (value.updated_at) {
        result["updated_at"] = codec<std::optional<long long>>::encode(value.updated_at);
    }
    return result;
}
inline holder::api::AiThreadCreateRequest codec<holder::api::AiThreadCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadCreateRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    return holder::api::AiThreadCreateRequest{
        value.contains("thread_id") ? codec<std::optional<std::string>>::decode(value.at("thread_id")) : std::nullopt,
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("title")),
        value.contains("card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("card_id"))} : std::nullopt,
        value.contains("created_at") ? codec<std::optional<long long>>::decode(value.at("created_at")) : std::nullopt,
        value.contains("updated_at") ? codec<std::optional<long long>>::decode(value.at("updated_at")) : std::nullopt,
    };
}

inline json codec<holder::api::AiThreadPatchResponseData>::encode(const holder::api::AiThreadPatchResponseData& value) {
    json result = json::object();
    result["thread_id"] = codec<std::string>::encode(value.thread_id);
    return result;
}
inline holder::api::AiThreadPatchResponseData codec<holder::api::AiThreadPatchResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadPatchResponseData");
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    return holder::api::AiThreadPatchResponseData{
        codec<std::string>::decode(value.at("thread_id")),
    };
}

inline json codec<holder::api::AiThreadPatchResponse>::encode(const holder::api::AiThreadPatchResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiThreadPatchResponseData>::encode(value.data);
    return result;
}
inline holder::api::AiThreadPatchResponse codec<holder::api::AiThreadPatchResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadPatchResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiThreadPatchResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiThreadPatchResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiThreadUpdateRequest>::encode(const holder::api::AiThreadUpdateRequest& value) {
    json result = json::object();
    if (value.title) {
        result["title"] = codec<std::optional<std::optional<std::string>>>::encode(value.title);
    }
    if (value.card_id) {
        result["card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.card_id);
    }
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::AiThreadUpdateRequest codec<holder::api::AiThreadUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiThreadUpdateRequest");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::AiThreadUpdateRequest{
        value.contains("title") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("title"))} : std::nullopt,
        value.contains("card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("card_id"))} : std::nullopt,
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::AiProviderSetting>::encode(const holder::api::AiProviderSetting& value) {
    json result = json::object();
    result["provider"] = codec<std::string>::encode(value.provider);
    result["enabled"] = codec<bool>::encode(value.enabled);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::AiProviderSetting codec<holder::api::AiProviderSetting>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSetting");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    if (!value.contains("enabled")) throw std::invalid_argument("missing enabled");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::AiProviderSetting{
        codec<std::string>::decode(value.at("provider")),
        codec<bool>::decode(value.at("enabled")),
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::AiProviderSettingUpdateRequest>::encode(const holder::api::AiProviderSettingUpdateRequest& value) {
    json result = json::object();
    result["provider"] = codec<std::string>::encode(value.provider);
    result["enabled"] = codec<bool>::encode(value.enabled);
    if (value.updated_at) {
        result["updated_at"] = codec<std::optional<long long>>::encode(value.updated_at);
    }
    return result;
}
inline holder::api::AiProviderSettingUpdateRequest codec<holder::api::AiProviderSettingUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingUpdateRequest");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    if (!value.contains("enabled")) throw std::invalid_argument("missing enabled");
    return holder::api::AiProviderSettingUpdateRequest{
        codec<std::string>::decode(value.at("provider")),
        codec<bool>::decode(value.at("enabled")),
        value.contains("updated_at") ? codec<std::optional<long long>>::decode(value.at("updated_at")) : std::nullopt,
    };
}

inline json codec<holder::api::AiProviderSettingResponse>::encode(const holder::api::AiProviderSettingResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiProviderSetting>::encode(value.data);
    return result;
}
inline holder::api::AiProviderSettingResponse codec<holder::api::AiProviderSettingResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiProviderSettingResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiProviderSetting>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiProviderSettingsData>::encode(const holder::api::AiProviderSettingsData& value) {
    json result = json::object();
    result["providers"] = codec<std::vector<holder::api::AiProviderSetting>>::encode(value.providers);
    return result;
}
inline holder::api::AiProviderSettingsData codec<holder::api::AiProviderSettingsData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingsData");
    if (!value.contains("providers")) throw std::invalid_argument("missing providers");
    return holder::api::AiProviderSettingsData{
        codec<std::vector<holder::api::AiProviderSetting>>::decode(value.at("providers")),
    };
}

inline json codec<holder::api::AiProviderSettingsResponse>::encode(const holder::api::AiProviderSettingsResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiProviderSettingsData>::encode(value.data);
    return result;
}
inline holder::api::AiProviderSettingsResponse codec<holder::api::AiProviderSettingsResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingsResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiProviderSettingsResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiProviderSettingsData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiProviderSettingDeleteData>::encode(const holder::api::AiProviderSettingDeleteData& value) {
    json result = json::object();
    result["provider"] = codec<std::string>::encode(value.provider);
    return result;
}
inline holder::api::AiProviderSettingDeleteData codec<holder::api::AiProviderSettingDeleteData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingDeleteData");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    return holder::api::AiProviderSettingDeleteData{
        codec<std::string>::decode(value.at("provider")),
    };
}

inline json codec<holder::api::AiProviderSettingDeleteResponse>::encode(const holder::api::AiProviderSettingDeleteResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiProviderSettingDeleteData>::encode(value.data);
    return result;
}
inline holder::api::AiProviderSettingDeleteResponse codec<holder::api::AiProviderSettingDeleteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderSettingDeleteResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiProviderSettingDeleteResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiProviderSettingDeleteData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiStreamResyncData>::encode(const holder::api::AiStreamResyncData& value) {
    json result = json::object();
    if (value.reason != "history_unavailable") throw std::invalid_argument("invalid reason");
    result["reason"] = codec<std::string>::encode(value.reason);
    return result;
}
inline holder::api::AiStreamResyncData codec<holder::api::AiStreamResyncData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiStreamResyncData");
    if (!value.contains("reason")) throw std::invalid_argument("missing reason");
    if (value.at("reason") != "history_unavailable") throw std::invalid_argument("invalid reason");
    return holder::api::AiStreamResyncData{
        codec<std::string>::decode(value.at("reason")),
    };
}

inline json codec<holder::api::routes::CardLinkListResponse>::encode(const holder::api::routes::CardLinkListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::routes::CardLink>>::encode(value.data);
    return result;
}
inline holder::api::routes::CardLinkListResponse codec<holder::api::routes::CardLinkListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLinkListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::routes::CardLinkListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::routes::CardLink>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::routes::CardLink>::encode(const holder::api::routes::CardLink& value) {
    json result = json::object();
    result["from_card_id"] = codec<std::string>::encode(value.from_card_id);
    result["to_card_id"] = codec<std::string>::encode(value.to_card_id);
    if (value.target_project_id) {
        result["target_project_id"] = codec<std::optional<std::string>>::encode(value.target_project_id);
    }
    if (value.source_project_id) {
        result["source_project_id"] = codec<std::optional<std::string>>::encode(value.source_project_id);
    }
    result["to_type"] = codec<std::string>::encode(value.to_type);
    result["kind"] = codec<std::string>::encode(value.kind);
    if (value.label) {
        result["label"] = codec<std::optional<std::optional<std::string>>>::encode(value.label);
    }
    result["created_at"] = codec<long long>::encode(value.created_at);
    return result;
}
inline holder::api::routes::CardLink codec<holder::api::routes::CardLink>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLink");
    if (!value.contains("from_card_id")) throw std::invalid_argument("missing from_card_id");
    if (!value.contains("to_card_id")) throw std::invalid_argument("missing to_card_id");
    if (!value.contains("to_type")) throw std::invalid_argument("missing to_type");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    return holder::api::routes::CardLink{
        codec<std::string>::decode(value.at("from_card_id")),
        codec<std::string>::decode(value.at("to_card_id")),
        value.contains("target_project_id") ? codec<std::optional<std::string>>::decode(value.at("target_project_id")) : std::nullopt,
        value.contains("source_project_id") ? codec<std::optional<std::string>>::decode(value.at("source_project_id")) : std::nullopt,
        codec<std::string>::decode(value.at("to_type")),
        codec<std::string>::decode(value.at("kind")),
        value.contains("label") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("label"))} : std::nullopt,
        codec<long long>::decode(value.at("created_at")),
    };
}

inline json codec<holder::api::routes::CardLinkCreateResponse>::encode(const holder::api::routes::CardLinkCreateResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::routes::CardLink>::encode(value.data);
    return result;
}
inline holder::api::routes::CardLinkCreateResponse codec<holder::api::routes::CardLinkCreateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLinkCreateResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::routes::CardLinkCreateResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::routes::CardLink>::decode(value.at("data")),
    };
}

inline json codec<holder::api::routes::CardLinkCreateRequest>::encode(const holder::api::routes::CardLinkCreateRequest& value) {
    json result = json::object();
    result["to_card_id"] = codec<std::string>::encode(value.to_card_id);
    if (value.target_project_id) {
        result["target_project_id"] = codec<std::optional<std::string>>::encode(value.target_project_id);
    }
    if (value.to_type) {
        result["to_type"] = codec<std::optional<std::string>>::encode(value.to_type);
    }
    if (value.kind) {
        result["kind"] = codec<std::optional<std::string>>::encode(value.kind);
    }
    if (value.label) {
        result["label"] = codec<std::optional<std::optional<std::string>>>::encode(value.label);
    }
    if (value.created_at) {
        result["created_at"] = codec<std::optional<long long>>::encode(value.created_at);
    }
    return result;
}
inline holder::api::routes::CardLinkCreateRequest codec<holder::api::routes::CardLinkCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardLinkCreateRequest");
    if (!value.contains("to_card_id")) throw std::invalid_argument("missing to_card_id");
    return holder::api::routes::CardLinkCreateRequest{
        codec<std::string>::decode(value.at("to_card_id")),
        value.contains("target_project_id") ? codec<std::optional<std::string>>::decode(value.at("target_project_id")) : std::nullopt,
        value.contains("to_type") ? codec<std::optional<std::string>>::decode(value.at("to_type")) : std::nullopt,
        value.contains("kind") ? codec<std::optional<std::string>>::decode(value.at("kind")) : std::nullopt,
        value.contains("label") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("label"))} : std::nullopt,
        value.contains("created_at") ? codec<std::optional<long long>>::decode(value.at("created_at")) : std::nullopt,
    };
}

template <typename T> json encode(const T& value) { return codec<T>::encode(value); }
template <typename T> T decode(const json& value) { return codec<T>::decode(value); }
} // namespace ai_json

// An event can only be built with the data type its operation declares for it.
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
