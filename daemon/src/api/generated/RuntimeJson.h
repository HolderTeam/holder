// Generated from RIG IR; do not edit.
#pragma once
#include "api/RigRuntime.h"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace rig_json {
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
template <> struct codec<holder::api::AliveResponse> {
    static json encode(const holder::api::AliveResponse& value);
    static holder::api::AliveResponse decode(const json& value);
};
template <> struct codec<holder::api::ByeResponse> {
    static json encode(const holder::api::ByeResponse& value);
    static holder::api::ByeResponse decode(const json& value);
};
template <> struct codec<holder::api::CardReferenceResolveRequest> {
    static json encode(const holder::api::CardReferenceResolveRequest& value);
    static holder::api::CardReferenceResolveRequest decode(const json& value);
};
template <> struct codec<holder::api::CardReferenceCandidate> {
    static json encode(const holder::api::CardReferenceCandidate& value);
    static holder::api::CardReferenceCandidate decode(const json& value);
};
template <> struct codec<holder::api::CardReferenceResolveData> {
    static json encode(const holder::api::CardReferenceResolveData& value);
    static holder::api::CardReferenceResolveData decode(const json& value);
};
template <> struct codec<holder::api::CardReferenceResolveResponse> {
    static json encode(const holder::api::CardReferenceResolveResponse& value);
    static holder::api::CardReferenceResolveResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectTag> {
    static json encode(const holder::api::ProjectTag& value);
    static holder::api::ProjectTag decode(const json& value);
};
template <> struct codec<holder::api::ProjectTagListResponse> {
    static json encode(const holder::api::ProjectTagListResponse& value);
    static holder::api::ProjectTagListResponse decode(const json& value);
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
template <> struct codec<holder::api::RebuildRequest> {
    static json encode(const holder::api::RebuildRequest& value);
    static holder::api::RebuildRequest decode(const json& value);
};
template <> struct codec<holder::api::RebuildData> {
    static json encode(const holder::api::RebuildData& value);
    static holder::api::RebuildData decode(const json& value);
};
template <> struct codec<holder::api::RebuildResponse> {
    static json encode(const holder::api::RebuildResponse& value);
    static holder::api::RebuildResponse decode(const json& value);
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
template <> struct codec<holder::api::AiProviderCredentialDeleteResponseData> {
    static json encode(const holder::api::AiProviderCredentialDeleteResponseData& value);
    static holder::api::AiProviderCredentialDeleteResponseData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderCredentialDeleteResponse> {
    static json encode(const holder::api::AiProviderCredentialDeleteResponse& value);
    static holder::api::AiProviderCredentialDeleteResponse decode(const json& value);
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
template <> struct codec<holder::api::AiLocalModelConfigData> {
    static json encode(const holder::api::AiLocalModelConfigData& value);
    static holder::api::AiLocalModelConfigData decode(const json& value);
};
template <> struct codec<holder::api::AiLocalModelConfigResponse> {
    static json encode(const holder::api::AiLocalModelConfigResponse& value);
    static holder::api::AiLocalModelConfigResponse decode(const json& value);
};
template <> struct codec<holder::api::AiLocalModelConfigUpdateRequest> {
    static json encode(const holder::api::AiLocalModelConfigUpdateRequest& value);
    static holder::api::AiLocalModelConfigUpdateRequest decode(const json& value);
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
template <> struct codec<holder::api::AiMessage> {
    static json encode(const holder::api::AiMessage& value);
    static holder::api::AiMessage decode(const json& value);
};
template <> struct codec<holder::api::AiMessageListResponse> {
    static json encode(const holder::api::AiMessageListResponse& value);
    static holder::api::AiMessageListResponse decode(const json& value);
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
template <> struct codec<holder::api::AiThread> {
    static json encode(const holder::api::AiThread& value);
    static holder::api::AiThread decode(const json& value);
};
template <> struct codec<holder::api::AiThreadListResponse> {
    static json encode(const holder::api::AiThreadListResponse& value);
    static holder::api::AiThreadListResponse decode(const json& value);
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
template <> struct codec<holder::api::AiThreadGetResponse> {
    static json encode(const holder::api::AiThreadGetResponse& value);
    static holder::api::AiThreadGetResponse decode(const json& value);
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
template <> struct codec<holder::api::AiMessageGetResponse> {
    static json encode(const holder::api::AiMessageGetResponse& value);
    static holder::api::AiMessageGetResponse decode(const json& value);
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
template <> struct codec<holder::api::CardCreateResponseData> {
    static json encode(const holder::api::CardCreateResponseData& value);
    static holder::api::CardCreateResponseData decode(const json& value);
};
template <> struct codec<holder::api::CardCreateResponse> {
    static json encode(const holder::api::CardCreateResponse& value);
    static holder::api::CardCreateResponse decode(const json& value);
};
template <> struct codec<holder::api::CardCreateRequest> {
    static json encode(const holder::api::CardCreateRequest& value);
    static holder::api::CardCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::CardData> {
    static json encode(const holder::api::CardData& value);
    static holder::api::CardData decode(const json& value);
};
template <> struct codec<holder::api::CardGetResponse> {
    static json encode(const holder::api::CardGetResponse& value);
    static holder::api::CardGetResponse decode(const json& value);
};
template <> struct codec<holder::api::CardPatchResponseData> {
    static json encode(const holder::api::CardPatchResponseData& value);
    static holder::api::CardPatchResponseData decode(const json& value);
};
template <> struct codec<holder::api::CardPatchResponse> {
    static json encode(const holder::api::CardPatchResponse& value);
    static holder::api::CardPatchResponse decode(const json& value);
};
template <> struct codec<holder::api::CardUpdateRequest> {
    static json encode(const holder::api::CardUpdateRequest& value);
    static holder::api::CardUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::HealthDataCryptFilter> {
    static json encode(const holder::api::HealthDataCryptFilter& value);
    static holder::api::HealthDataCryptFilter decode(const json& value);
};
template <> struct codec<holder::api::HealthData> {
    static json encode(const holder::api::HealthData& value);
    static holder::api::HealthData decode(const json& value);
};
template <> struct codec<holder::api::HealthResponse> {
    static json encode(const holder::api::HealthResponse& value);
    static holder::api::HealthResponse decode(const json& value);
};
template <> struct codec<holder::api::AssetImportJob> {
    static json encode(const holder::api::AssetImportJob& value);
    static holder::api::AssetImportJob decode(const json& value);
};
template <> struct codec<holder::api::AssetImportJobResponse> {
    static json encode(const holder::api::AssetImportJobResponse& value);
    static holder::api::AssetImportJobResponse decode(const json& value);
};
template <> struct codec<holder::api::AssetImportRequest> {
    static json encode(const holder::api::AssetImportRequest& value);
    static holder::api::AssetImportRequest decode(const json& value);
};
template <> struct codec<holder::api::StorageLocation> {
    static json encode(const holder::api::StorageLocation& value);
    static holder::api::StorageLocation decode(const json& value);
};
template <> struct codec<holder::api::StorageLocationListResponse> {
    static json encode(const holder::api::StorageLocationListResponse& value);
    static holder::api::StorageLocationListResponse decode(const json& value);
};
template <> struct codec<holder::api::StorageLocationResponse> {
    static json encode(const holder::api::StorageLocationResponse& value);
    static holder::api::StorageLocationResponse decode(const json& value);
};
template <> struct codec<holder::api::StorageLocationRequest> {
    static json encode(const holder::api::StorageLocationRequest& value);
    static holder::api::StorageLocationRequest decode(const json& value);
};
template <> struct codec<holder::api::PatchLocationsLocationIdRequest> {
    static json encode(const holder::api::PatchLocationsLocationIdRequest& value);
    static holder::api::PatchLocationsLocationIdRequest decode(const json& value);
};
template <> struct codec<holder::api::LocationActionResponseData> {
    static json encode(const holder::api::LocationActionResponseData& value);
    static holder::api::LocationActionResponseData decode(const json& value);
};
template <> struct codec<holder::api::LocationActionResponse> {
    static json encode(const holder::api::LocationActionResponse& value);
    static holder::api::LocationActionResponse decode(const json& value);
};
template <> struct codec<holder::api::LocationBindingRequest> {
    static json encode(const holder::api::LocationBindingRequest& value);
    static holder::api::LocationBindingRequest decode(const json& value);
};
template <> struct codec<holder::api::LocationTestResponseData> {
    static json encode(const holder::api::LocationTestResponseData& value);
    static holder::api::LocationTestResponseData decode(const json& value);
};
template <> struct codec<holder::api::LocationTestResponse> {
    static json encode(const holder::api::LocationTestResponse& value);
    static holder::api::LocationTestResponse decode(const json& value);
};
template <> struct codec<holder::api::PutLocationsPreferredRequest> {
    static json encode(const holder::api::PutLocationsPreferredRequest& value);
    static holder::api::PutLocationsPreferredRequest decode(const json& value);
};
template <> struct codec<holder::api::ProjectCreateResponseData> {
    static json encode(const holder::api::ProjectCreateResponseData& value);
    static holder::api::ProjectCreateResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectCreateResponse> {
    static json encode(const holder::api::ProjectCreateResponse& value);
    static holder::api::ProjectCreateResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectCreateRequest> {
    static json encode(const holder::api::ProjectCreateRequest& value);
    static holder::api::ProjectCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::ProjectSync> {
    static json encode(const holder::api::ProjectSync& value);
    static holder::api::ProjectSync decode(const json& value);
};
template <> struct codec<holder::api::Project> {
    static json encode(const holder::api::Project& value);
    static holder::api::Project decode(const json& value);
};
template <> struct codec<holder::api::ProjectGetResponse> {
    static json encode(const holder::api::ProjectGetResponse& value);
    static holder::api::ProjectGetResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectDeleteResponseData> {
    static json encode(const holder::api::ProjectDeleteResponseData& value);
    static holder::api::ProjectDeleteResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectDeleteResponse> {
    static json encode(const holder::api::ProjectDeleteResponse& value);
    static holder::api::ProjectDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::CardBreadcrumb> {
    static json encode(const holder::api::CardBreadcrumb& value);
    static holder::api::CardBreadcrumb decode(const json& value);
};
template <> struct codec<holder::api::CardBreadcrumbsResponse> {
    static json encode(const holder::api::CardBreadcrumbsResponse& value);
    static holder::api::CardBreadcrumbsResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncStatusResponseData> {
    static json encode(const holder::api::ProjectGitSyncStatusResponseData& value);
    static holder::api::ProjectGitSyncStatusResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncStatusResponse> {
    static json encode(const holder::api::ProjectGitSyncStatusResponse& value);
    static holder::api::ProjectGitSyncStatusResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectRecoveryTokenExportResponseData> {
    static json encode(const holder::api::ProjectRecoveryTokenExportResponseData& value);
    static holder::api::ProjectRecoveryTokenExportResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectRecoveryTokenExportResponse> {
    static json encode(const holder::api::ProjectRecoveryTokenExportResponse& value);
    static holder::api::ProjectRecoveryTokenExportResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectRecoveryTokenExportRequest> {
    static json encode(const holder::api::ProjectRecoveryTokenExportRequest& value);
    static holder::api::ProjectRecoveryTokenExportRequest decode(const json& value);
};
template <> struct codec<holder::api::ProjectRecoveryTokenImportResponseData> {
    static json encode(const holder::api::ProjectRecoveryTokenImportResponseData& value);
    static holder::api::ProjectRecoveryTokenImportResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectRecoveryTokenImportResponse> {
    static json encode(const holder::api::ProjectRecoveryTokenImportResponse& value);
    static holder::api::ProjectRecoveryTokenImportResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectRecoveryTokenImportRequest> {
    static json encode(const holder::api::ProjectRecoveryTokenImportRequest& value);
    static holder::api::ProjectRecoveryTokenImportRequest decode(const json& value);
};
template <> struct codec<holder::api::ProjectEncryptionCheck> {
    static json encode(const holder::api::ProjectEncryptionCheck& value);
    static holder::api::ProjectEncryptionCheck decode(const json& value);
};
template <> struct codec<holder::api::ProjectEncryptionCheckResponseData> {
    static json encode(const holder::api::ProjectEncryptionCheckResponseData& value);
    static holder::api::ProjectEncryptionCheckResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectEncryptionCheckResponse> {
    static json encode(const holder::api::ProjectEncryptionCheckResponse& value);
    static holder::api::ProjectEncryptionCheckResponse decode(const json& value);
};
template <> struct codec<holder::api::ReindexData> {
    static json encode(const holder::api::ReindexData& value);
    static holder::api::ReindexData decode(const json& value);
};
template <> struct codec<holder::api::ReindexResponse> {
    static json encode(const holder::api::ReindexResponse& value);
    static holder::api::ReindexResponse decode(const json& value);
};
template <> struct codec<holder::api::ResourceDeleteResponseData> {
    static json encode(const holder::api::ResourceDeleteResponseData& value);
    static holder::api::ResourceDeleteResponseData decode(const json& value);
};
template <> struct codec<holder::api::ResourceDeleteResponse> {
    static json encode(const holder::api::ResourceDeleteResponse& value);
    static holder::api::ResourceDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::SearchCardItem> {
    static json encode(const holder::api::SearchCardItem& value);
    static holder::api::SearchCardItem decode(const json& value);
};
template <> struct codec<holder::api::SearchCardsResponse> {
    static json encode(const holder::api::SearchCardsResponse& value);
    static holder::api::SearchCardsResponse decode(const json& value);
};
template <> struct codec<holder::api::SearchMessageItem> {
    static json encode(const holder::api::SearchMessageItem& value);
    static holder::api::SearchMessageItem decode(const json& value);
};
template <> struct codec<holder::api::SearchMessagesResponse> {
    static json encode(const holder::api::SearchMessagesResponse& value);
    static holder::api::SearchMessagesResponse decode(const json& value);
};
template <> struct codec<holder::api::TrashItem> {
    static json encode(const holder::api::TrashItem& value);
    static holder::api::TrashItem decode(const json& value);
};
template <> struct codec<holder::api::TrashListResponse> {
    static json encode(const holder::api::TrashListResponse& value);
    static holder::api::TrashListResponse decode(const json& value);
};
template <> struct codec<holder::api::TrashDeleteResponse> {
    static json encode(const holder::api::TrashDeleteResponse& value);
    static holder::api::TrashDeleteResponse decode(const json& value);
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

inline json codec<holder::api::AliveResponse>::encode(const holder::api::AliveResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    return result;
}
inline holder::api::AliveResponse codec<holder::api::AliveResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AliveResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AliveResponse{
        codec<bool>::decode(value.at("ok")),
    };
}

inline json codec<holder::api::ByeResponse>::encode(const holder::api::ByeResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    return result;
}
inline holder::api::ByeResponse codec<holder::api::ByeResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ByeResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ByeResponse{
        codec<bool>::decode(value.at("ok")),
    };
}

inline json codec<holder::api::CardReferenceResolveRequest>::encode(const holder::api::CardReferenceResolveRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["reference"] = codec<std::string>::encode(value.reference);
    if (value.scope != "live" && value.scope != "trashed" && value.scope != "either") throw std::invalid_argument("invalid scope");
    result["scope"] = codec<std::string>::encode(value.scope);
    return result;
}
inline holder::api::CardReferenceResolveRequest codec<holder::api::CardReferenceResolveRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardReferenceResolveRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("reference")) throw std::invalid_argument("missing reference");
    if (!value.contains("scope")) throw std::invalid_argument("missing scope");
    if (value.at("scope") != "live" && value.at("scope") != "trashed" && value.at("scope") != "either") throw std::invalid_argument("invalid scope");
    return holder::api::CardReferenceResolveRequest{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("reference")),
        codec<std::string>::decode(value.at("scope")),
    };
}

inline json codec<holder::api::CardReferenceCandidate>::encode(const holder::api::CardReferenceCandidate& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["deleted_at"] = codec<std::optional<long long>>::encode(value.deleted_at);
    return result;
}
inline holder::api::CardReferenceCandidate codec<holder::api::CardReferenceCandidate>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardReferenceCandidate");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("deleted_at")) throw std::invalid_argument("missing deleted_at");
    return holder::api::CardReferenceCandidate{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("title")),
        codec<std::optional<long long>>::decode(value.at("deleted_at")),
    };
}

inline json codec<holder::api::CardReferenceResolveData>::encode(const holder::api::CardReferenceResolveData& value) {
    json result = json::object();
    if (value.status != "resolved" && value.status != "ambiguous" && value.status != "not_found") throw std::invalid_argument("invalid status");
    result["status"] = codec<std::string>::encode(value.status);
    if (value.match_kind && *value.match_kind != "full_id" && *value.match_kind != "id_prefix" && *value.match_kind != "exact_title") throw std::invalid_argument("invalid match_kind");
    if (value.match_kind) {
        result["match_kind"] = codec<std::optional<std::string>>::encode(value.match_kind);
    }
    if (value.card) {
        result["card"] = codec<std::optional<holder::api::CardReferenceCandidate>>::encode(value.card);
    }
    if (value.candidates) {
        result["candidates"] = codec<std::optional<std::vector<holder::api::CardReferenceCandidate>>>::encode(value.candidates);
    }
    return result;
}
inline holder::api::CardReferenceResolveData codec<holder::api::CardReferenceResolveData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardReferenceResolveData");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    if (value.at("status") != "resolved" && value.at("status") != "ambiguous" && value.at("status") != "not_found") throw std::invalid_argument("invalid status");
    if (value.contains("match_kind") && !value.at("match_kind").is_null() && value.at("match_kind") != "full_id" && value.at("match_kind") != "id_prefix" && value.at("match_kind") != "exact_title") throw std::invalid_argument("invalid match_kind");
    return holder::api::CardReferenceResolveData{
        codec<std::string>::decode(value.at("status")),
        value.contains("match_kind") ? codec<std::optional<std::string>>::decode(value.at("match_kind")) : std::nullopt,
        value.contains("card") ? codec<std::optional<holder::api::CardReferenceCandidate>>::decode(value.at("card")) : std::nullopt,
        value.contains("candidates") ? codec<std::optional<std::vector<holder::api::CardReferenceCandidate>>>::decode(value.at("candidates")) : std::nullopt,
    };
}

inline json codec<holder::api::CardReferenceResolveResponse>::encode(const holder::api::CardReferenceResolveResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardReferenceResolveData>::encode(value.data);
    return result;
}
inline holder::api::CardReferenceResolveResponse codec<holder::api::CardReferenceResolveResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardReferenceResolveResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardReferenceResolveResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardReferenceResolveData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectTag>::encode(const holder::api::ProjectTag& value) {
    json result = json::object();
    result["tag"] = codec<std::string>::encode(value.tag);
    result["card_count"] = codec<int>::encode(value.card_count);
    return result;
}
inline holder::api::ProjectTag codec<holder::api::ProjectTag>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectTag");
    if (!value.contains("tag")) throw std::invalid_argument("missing tag");
    if (!value.contains("card_count")) throw std::invalid_argument("missing card_count");
    return holder::api::ProjectTag{
        codec<std::string>::decode(value.at("tag")),
        codec<int>::decode(value.at("card_count")),
    };
}

inline json codec<holder::api::ProjectTagListResponse>::encode(const holder::api::ProjectTagListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::ProjectTag>>::encode(value.data);
    return result;
}
inline holder::api::ProjectTagListResponse codec<holder::api::ProjectTagListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectTagListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectTagListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::ProjectTag>>::decode(value.at("data")),
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

inline json codec<holder::api::RebuildRequest>::encode(const holder::api::RebuildRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    return result;
}
inline holder::api::RebuildRequest codec<holder::api::RebuildRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RebuildRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    return holder::api::RebuildRequest{
        codec<std::string>::decode(value.at("project_id")),
    };
}

inline json codec<holder::api::RebuildData>::encode(const holder::api::RebuildData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["cards"] = codec<int>::encode(value.cards);
    result["ai_messages"] = codec<int>::encode(value.ai_messages);
    result["ai_threads"] = codec<int>::encode(value.ai_threads);
    result["links"] = codec<int>::encode(value.links);
    return result;
}
inline holder::api::RebuildData codec<holder::api::RebuildData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RebuildData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("cards")) throw std::invalid_argument("missing cards");
    if (!value.contains("ai_messages")) throw std::invalid_argument("missing ai_messages");
    if (!value.contains("ai_threads")) throw std::invalid_argument("missing ai_threads");
    if (!value.contains("links")) throw std::invalid_argument("missing links");
    return holder::api::RebuildData{
        codec<std::string>::decode(value.at("project_id")),
        codec<int>::decode(value.at("cards")),
        codec<int>::decode(value.at("ai_messages")),
        codec<int>::decode(value.at("ai_threads")),
        codec<int>::decode(value.at("links")),
    };
}

inline json codec<holder::api::RebuildResponse>::encode(const holder::api::RebuildResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::RebuildData>::encode(value.data);
    return result;
}
inline holder::api::RebuildResponse codec<holder::api::RebuildResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RebuildResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::RebuildResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::RebuildData>::decode(value.at("data")),
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
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    if (!value.contains("progress")) throw std::invalid_argument("missing progress");
    return holder::api::RunnerPullJob{
        codec<std::string>::decode(value.at("job_id")),
        value.contains("runner_id") ? codec<std::optional<std::string>>::decode(value.at("runner_id")) : std::nullopt,
        value.contains("model_ref") ? codec<std::optional<std::string>>::decode(value.at("model_ref")) : std::nullopt,
        codec<std::string>::decode(value.at("model")),
        codec<std::string>::decode(value.at("status")),
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

inline json codec<holder::api::AiProviderCatalogProvider>::encode(const holder::api::AiProviderCatalogProvider& value) {
    json result = json::object();
    result["id"] = codec<std::string>::encode(value.id);
    result["display_name"] = codec<std::string>::encode(value.display_name);
    result["enabled"] = codec<bool>::encode(value.enabled);
    result["configured"] = codec<bool>::encode(value.configured);
    result["api"] = codec<std::optional<nlohmann::json>>::encode(value.api);
    result["auth"] = codec<std::optional<nlohmann::json>>::encode(value.auth);
    result["models"] = codec<std::vector<nlohmann::json>>::encode(value.models);
    if (value.setup_url) {
        result["setup_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.setup_url);
    }
    if (value.docs_url) {
        result["docs_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.docs_url);
    }
    if (value.api_key_label) {
        result["api_key_label"] = codec<std::optional<std::optional<std::string>>>::encode(value.api_key_label);
    }
    if (value.api_key_hint) {
        result["api_key_hint"] = codec<std::optional<std::optional<std::string>>>::encode(value.api_key_hint);
    }
    return result;
}
inline holder::api::AiProviderCatalogProvider codec<holder::api::AiProviderCatalogProvider>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogProvider");
    if (!value.contains("id")) throw std::invalid_argument("missing id");
    if (!value.contains("display_name")) throw std::invalid_argument("missing display_name");
    if (!value.contains("enabled")) throw std::invalid_argument("missing enabled");
    if (!value.contains("configured")) throw std::invalid_argument("missing configured");
    if (!value.contains("api")) throw std::invalid_argument("missing api");
    if (!value.contains("auth")) throw std::invalid_argument("missing auth");
    if (!value.contains("models")) throw std::invalid_argument("missing models");
    return holder::api::AiProviderCatalogProvider{
        codec<std::string>::decode(value.at("id")),
        codec<std::string>::decode(value.at("display_name")),
        codec<bool>::decode(value.at("enabled")),
        codec<bool>::decode(value.at("configured")),
        codec<std::optional<nlohmann::json>>::decode(value.at("api")),
        codec<std::optional<nlohmann::json>>::decode(value.at("auth")),
        codec<std::vector<nlohmann::json>>::decode(value.at("models")),
        value.contains("setup_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("setup_url"))} : std::nullopt,
        value.contains("docs_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("docs_url"))} : std::nullopt,
        value.contains("api_key_label") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("api_key_label"))} : std::nullopt,
        value.contains("api_key_hint") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("api_key_hint"))} : std::nullopt,
    };
}

inline json codec<holder::api::AiProviderCatalogResponseData>::encode(const holder::api::AiProviderCatalogResponseData& value) {
    json result = json::object();
    if (value.default_provider) {
        result["default_provider"] = codec<std::optional<std::optional<std::string>>>::encode(value.default_provider);
    }
    result["providers"] = codec<std::vector<holder::api::AiProviderCatalogProvider>>::encode(value.providers);
    return result;
}
inline holder::api::AiProviderCatalogResponseData codec<holder::api::AiProviderCatalogResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiProviderCatalogResponseData");
    if (!value.contains("providers")) throw std::invalid_argument("missing providers");
    return holder::api::AiProviderCatalogResponseData{
        value.contains("default_provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("default_provider"))} : std::nullopt,
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

inline json codec<holder::api::LocalModel>::encode(const holder::api::LocalModel& value) {
    json result = json::object();
    result["name"] = codec<std::string>::encode(value.name);
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
        value.contains("ranked_json") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("ranked_json"))} : std::nullopt,
        value.contains("policy_trace_json") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("policy_trace_json"))} : std::nullopt,
        value.contains("policy_trace") ? std::optional<std::optional<nlohmann::json>>{std::in_place, codec<std::optional<nlohmann::json>>::decode(value.at("policy_trace"))} : std::nullopt,
        value.contains("chosen_model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("chosen_model"))} : std::nullopt,
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

inline json codec<holder::api::AiMessage>::encode(const holder::api::AiMessage& value) {
    json result = json::object();
    result["message_id"] = codec<std::string>::encode(value.message_id);
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
    result["created_at"] = codec<long long>::encode(value.created_at);
    if (value.deleted_at) {
        result["deleted_at"] = codec<std::optional<std::optional<long long>>>::encode(value.deleted_at);
    }
    if (value.prompt_hash) {
        result["prompt_hash"] = codec<std::optional<std::optional<std::string>>>::encode(value.prompt_hash);
    }
    if (value.meta_json) {
        result["meta_json"] = codec<std::optional<std::optional<std::string>>>::encode(value.meta_json);
    }
    return result;
}
inline holder::api::AiMessage codec<holder::api::AiMessage>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiMessage");
    if (!value.contains("message_id")) throw std::invalid_argument("missing message_id");
    if (!value.contains("thread_id")) throw std::invalid_argument("missing thread_id");
    if (!value.contains("role")) throw std::invalid_argument("missing role");
    if (!value.contains("source")) throw std::invalid_argument("missing source");
    if (!value.contains("content")) throw std::invalid_argument("missing content");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    return holder::api::AiMessage{
        codec<std::string>::decode(value.at("message_id")),
        codec<std::string>::decode(value.at("thread_id")),
        codec<std::string>::decode(value.at("role")),
        codec<std::string>::decode(value.at("source")),
        value.contains("provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("provider"))} : std::nullopt,
        value.contains("model") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("model"))} : std::nullopt,
        codec<std::string>::decode(value.at("content")),
        codec<long long>::decode(value.at("created_at")),
        value.contains("deleted_at") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("deleted_at"))} : std::nullopt,
        value.contains("prompt_hash") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("prompt_hash"))} : std::nullopt,
        value.contains("meta_json") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("meta_json"))} : std::nullopt,
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

inline json codec<holder::api::CardCreateResponseData>::encode(const holder::api::CardCreateResponseData& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["rel_path"] = codec<std::string>::encode(value.rel_path);
    return result;
}
inline holder::api::CardCreateResponseData codec<holder::api::CardCreateResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardCreateResponseData");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("rel_path")) throw std::invalid_argument("missing rel_path");
    return holder::api::CardCreateResponseData{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("rel_path")),
    };
}

inline json codec<holder::api::CardCreateResponse>::encode(const holder::api::CardCreateResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardCreateResponseData>::encode(value.data);
    return result;
}
inline holder::api::CardCreateResponse codec<holder::api::CardCreateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardCreateResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardCreateResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardCreateResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardCreateRequest>::encode(const holder::api::CardCreateRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["content"] = codec<std::string>::encode(value.content);
    if (value.created_at) {
        result["created_at"] = codec<std::optional<long long>>::encode(value.created_at);
    }
    if (value.updated_at) {
        result["updated_at"] = codec<std::optional<long long>>::encode(value.updated_at);
    }
    if (value.parent_card_id) {
        result["parent_card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.parent_card_id);
    }
    if (value.sort_key) {
        result["sort_key"] = codec<std::optional<double>>::encode(value.sort_key);
    }
    if (value.rel_path) {
        result["rel_path"] = codec<std::optional<std::optional<std::string>>>::encode(value.rel_path);
    }
    return result;
}
inline holder::api::CardCreateRequest codec<holder::api::CardCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardCreateRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("content")) throw std::invalid_argument("missing content");
    return holder::api::CardCreateRequest{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("title")),
        codec<std::string>::decode(value.at("content")),
        value.contains("created_at") ? codec<std::optional<long long>>::decode(value.at("created_at")) : std::nullopt,
        value.contains("updated_at") ? codec<std::optional<long long>>::decode(value.at("updated_at")) : std::nullopt,
        value.contains("parent_card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("parent_card_id"))} : std::nullopt,
        value.contains("sort_key") ? codec<std::optional<double>>::decode(value.at("sort_key")) : std::nullopt,
        value.contains("rel_path") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("rel_path"))} : std::nullopt,
    };
}

inline json codec<holder::api::CardData>::encode(const holder::api::CardData& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["rel_path"] = codec<std::string>::encode(value.rel_path);
    result["sort_key"] = codec<double>::encode(value.sort_key);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    if (value.parent_card_id) {
        result["parent_card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.parent_card_id);
    }
    if (value.deleted_at) {
        result["deleted_at"] = codec<std::optional<std::optional<long long>>>::encode(value.deleted_at);
    }
    result["content"] = codec<std::string>::encode(value.content);
    return result;
}
inline holder::api::CardData codec<holder::api::CardData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardData");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("rel_path")) throw std::invalid_argument("missing rel_path");
    if (!value.contains("sort_key")) throw std::invalid_argument("missing sort_key");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    if (!value.contains("content")) throw std::invalid_argument("missing content");
    return holder::api::CardData{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("title")),
        codec<std::string>::decode(value.at("rel_path")),
        codec<double>::decode(value.at("sort_key")),
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
        value.contains("parent_card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("parent_card_id"))} : std::nullopt,
        value.contains("deleted_at") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("deleted_at"))} : std::nullopt,
        codec<std::string>::decode(value.at("content")),
    };
}

inline json codec<holder::api::CardGetResponse>::encode(const holder::api::CardGetResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardData>::encode(value.data);
    return result;
}
inline holder::api::CardGetResponse codec<holder::api::CardGetResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardGetResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardGetResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardPatchResponseData>::encode(const holder::api::CardPatchResponseData& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    return result;
}
inline holder::api::CardPatchResponseData codec<holder::api::CardPatchResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardPatchResponseData");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    return holder::api::CardPatchResponseData{
        codec<std::string>::decode(value.at("card_id")),
    };
}

inline json codec<holder::api::CardPatchResponse>::encode(const holder::api::CardPatchResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardPatchResponseData>::encode(value.data);
    return result;
}
inline holder::api::CardPatchResponse codec<holder::api::CardPatchResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardPatchResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardPatchResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardPatchResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardUpdateRequest>::encode(const holder::api::CardUpdateRequest& value) {
    json result = json::object();
    if (value.content) {
        result["content"] = codec<std::optional<std::string>>::encode(value.content);
    }
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    if (value.title) {
        result["title"] = codec<std::optional<std::optional<std::string>>>::encode(value.title);
    }
    if (value.parent_card_id) {
        result["parent_card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.parent_card_id);
    }
    if (value.sort_key) {
        result["sort_key"] = codec<std::optional<double>>::encode(value.sort_key);
    }
    return result;
}
inline holder::api::CardUpdateRequest codec<holder::api::CardUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardUpdateRequest");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::CardUpdateRequest{
        value.contains("content") ? codec<std::optional<std::string>>::decode(value.at("content")) : std::nullopt,
        codec<long long>::decode(value.at("updated_at")),
        value.contains("title") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("title"))} : std::nullopt,
        value.contains("parent_card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("parent_card_id"))} : std::nullopt,
        value.contains("sort_key") ? codec<std::optional<double>>::decode(value.at("sort_key")) : std::nullopt,
    };
}

inline json codec<holder::api::HealthDataCryptFilter>::encode(const holder::api::HealthDataCryptFilter& value) {
    json result = json::object();
    result["path"] = codec<std::string>::encode(value.path);
    result["exists"] = codec<bool>::encode(value.exists);
    result["executable"] = codec<bool>::encode(value.executable);
    return result;
}
inline holder::api::HealthDataCryptFilter codec<holder::api::HealthDataCryptFilter>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected HealthDataCryptFilter");
    if (!value.contains("path")) throw std::invalid_argument("missing path");
    if (!value.contains("exists")) throw std::invalid_argument("missing exists");
    if (!value.contains("executable")) throw std::invalid_argument("missing executable");
    return holder::api::HealthDataCryptFilter{
        codec<std::string>::decode(value.at("path")),
        codec<bool>::decode(value.at("exists")),
        codec<bool>::decode(value.at("executable")),
    };
}

inline json codec<holder::api::HealthData>::encode(const holder::api::HealthData& value) {
    json result = json::object();
    result["db_ok"] = codec<bool>::encode(value.db_ok);
    result["uptime_ms"] = codec<long long>::encode(value.uptime_ms);
    result["api_version"] = codec<std::string>::encode(value.api_version);
    result["server_version"] = codec<std::string>::encode(value.server_version);
    result["pid"] = codec<int>::encode(value.pid);
    result["crypt_filter"] = codec<holder::api::HealthDataCryptFilter>::encode(value.crypt_filter);
    return result;
}
inline holder::api::HealthData codec<holder::api::HealthData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected HealthData");
    if (!value.contains("db_ok")) throw std::invalid_argument("missing db_ok");
    if (!value.contains("uptime_ms")) throw std::invalid_argument("missing uptime_ms");
    if (!value.contains("api_version")) throw std::invalid_argument("missing api_version");
    if (!value.contains("server_version")) throw std::invalid_argument("missing server_version");
    if (!value.contains("pid")) throw std::invalid_argument("missing pid");
    if (!value.contains("crypt_filter")) throw std::invalid_argument("missing crypt_filter");
    return holder::api::HealthData{
        codec<bool>::decode(value.at("db_ok")),
        codec<long long>::decode(value.at("uptime_ms")),
        codec<std::string>::decode(value.at("api_version")),
        codec<std::string>::decode(value.at("server_version")),
        codec<int>::decode(value.at("pid")),
        codec<holder::api::HealthDataCryptFilter>::decode(value.at("crypt_filter")),
    };
}

inline json codec<holder::api::HealthResponse>::encode(const holder::api::HealthResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::HealthData>::encode(value.data);
    return result;
}
inline holder::api::HealthResponse codec<holder::api::HealthResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected HealthResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::HealthResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::HealthData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AssetImportJob>::encode(const holder::api::AssetImportJob& value) {
    json result = json::object();
    result["job_id"] = codec<std::string>::encode(value.job_id);
    if (value.status != "queued" && value.status != "staging" && value.status != "storing" && value.status != "committing" && value.status != "completed" && value.status != "failed") throw std::invalid_argument("invalid status");
    result["status"] = codec<std::string>::encode(value.status);
    if (value.resource_id) {
        result["resource_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.resource_id);
    }
    if (value.asset_id) {
        result["asset_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.asset_id);
    }
    result["duplicate_reused"] = codec<bool>::encode(value.duplicate_reused);
    if (value.link_created) {
        result["link_created"] = codec<std::optional<bool>>::encode(value.link_created);
    }
    if (value.error) {
        result["error"] = codec<std::optional<std::optional<std::string>>>::encode(value.error);
    }
    return result;
}
inline holder::api::AssetImportJob codec<holder::api::AssetImportJob>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AssetImportJob");
    if (!value.contains("job_id")) throw std::invalid_argument("missing job_id");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    if (!value.contains("duplicate_reused")) throw std::invalid_argument("missing duplicate_reused");
    if (value.at("status") != "queued" && value.at("status") != "staging" && value.at("status") != "storing" && value.at("status") != "committing" && value.at("status") != "completed" && value.at("status") != "failed") throw std::invalid_argument("invalid status");
    return holder::api::AssetImportJob{
        codec<std::string>::decode(value.at("job_id")),
        codec<std::string>::decode(value.at("status")),
        value.contains("resource_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("resource_id"))} : std::nullopt,
        value.contains("asset_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("asset_id"))} : std::nullopt,
        codec<bool>::decode(value.at("duplicate_reused")),
        value.contains("link_created") ? codec<std::optional<bool>>::decode(value.at("link_created")) : std::nullopt,
        value.contains("error") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("error"))} : std::nullopt,
    };
}

inline json codec<holder::api::AssetImportJobResponse>::encode(const holder::api::AssetImportJobResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AssetImportJob>::encode(value.data);
    return result;
}
inline holder::api::AssetImportJobResponse codec<holder::api::AssetImportJobResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AssetImportJobResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AssetImportJobResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AssetImportJob>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AssetImportRequest>::encode(const holder::api::AssetImportRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["location_id"] = codec<std::string>::encode(value.location_id);
    result["source_path"] = codec<std::string>::encode(value.source_path);
    return result;
}
inline holder::api::AssetImportRequest codec<holder::api::AssetImportRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AssetImportRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("location_id")) throw std::invalid_argument("missing location_id");
    if (!value.contains("source_path")) throw std::invalid_argument("missing source_path");
    return holder::api::AssetImportRequest{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("location_id")),
        codec<std::string>::decode(value.at("source_path")),
    };
}

inline json codec<holder::api::StorageLocation>::encode(const holder::api::StorageLocation& value) {
    json result = json::object();
    result["location_id"] = codec<std::string>::encode(value.location_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["name"] = codec<std::string>::encode(value.name);
    if (value.provider != "local_directory" && value.provider != "s3_compatible") throw std::invalid_argument("invalid provider");
    result["provider"] = codec<std::string>::encode(value.provider);
    result["configuration"] = codec<std::map<std::string, std::string>>::encode(value.configuration);
    result["bound"] = codec<bool>::encode(value.bound);
    if (value.binding_preview) {
        result["binding_preview"] = codec<std::optional<std::optional<std::string>>>::encode(value.binding_preview);
    }
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::StorageLocation codec<holder::api::StorageLocation>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected StorageLocation");
    if (!value.contains("location_id")) throw std::invalid_argument("missing location_id");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    if (!value.contains("configuration")) throw std::invalid_argument("missing configuration");
    if (!value.contains("bound")) throw std::invalid_argument("missing bound");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    if (value.at("provider") != "local_directory" && value.at("provider") != "s3_compatible") throw std::invalid_argument("invalid provider");
    return holder::api::StorageLocation{
        codec<std::string>::decode(value.at("location_id")),
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("name")),
        codec<std::string>::decode(value.at("provider")),
        codec<std::map<std::string, std::string>>::decode(value.at("configuration")),
        codec<bool>::decode(value.at("bound")),
        value.contains("binding_preview") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("binding_preview"))} : std::nullopt,
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::StorageLocationListResponse>::encode(const holder::api::StorageLocationListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::StorageLocation>>::encode(value.data);
    result["preferred_location_id"] = codec<std::optional<std::string>>::encode(value.preferred_location_id);
    return result;
}
inline holder::api::StorageLocationListResponse codec<holder::api::StorageLocationListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected StorageLocationListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (!value.contains("preferred_location_id")) throw std::invalid_argument("missing preferred_location_id");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::StorageLocationListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::StorageLocation>>::decode(value.at("data")),
        codec<std::optional<std::string>>::decode(value.at("preferred_location_id")),
    };
}

inline json codec<holder::api::StorageLocationResponse>::encode(const holder::api::StorageLocationResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::StorageLocation>::encode(value.data);
    return result;
}
inline holder::api::StorageLocationResponse codec<holder::api::StorageLocationResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected StorageLocationResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::StorageLocationResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::StorageLocation>::decode(value.at("data")),
    };
}

inline json codec<holder::api::StorageLocationRequest>::encode(const holder::api::StorageLocationRequest& value) {
    json result = json::object();
    if (value.location_id) {
        result["location_id"] = codec<std::optional<std::string>>::encode(value.location_id);
    }
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["name"] = codec<std::string>::encode(value.name);
    if (value.provider != "local_directory" && value.provider != "s3_compatible") throw std::invalid_argument("invalid provider");
    result["provider"] = codec<std::string>::encode(value.provider);
    if (value.configuration) {
        result["configuration"] = codec<std::optional<std::map<std::string, std::string>>>::encode(value.configuration);
    }
    return result;
}
inline holder::api::StorageLocationRequest codec<holder::api::StorageLocationRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected StorageLocationRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    if (!value.contains("provider")) throw std::invalid_argument("missing provider");
    if (value.at("provider") != "local_directory" && value.at("provider") != "s3_compatible") throw std::invalid_argument("invalid provider");
    return holder::api::StorageLocationRequest{
        value.contains("location_id") ? codec<std::optional<std::string>>::decode(value.at("location_id")) : std::nullopt,
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("name")),
        codec<std::string>::decode(value.at("provider")),
        value.contains("configuration") ? codec<std::optional<std::map<std::string, std::string>>>::decode(value.at("configuration")) : std::nullopt,
    };
}

inline json codec<holder::api::PatchLocationsLocationIdRequest>::encode(const holder::api::PatchLocationsLocationIdRequest& value) {
    json result = json::object();
    if (value.name) {
        result["name"] = codec<std::optional<std::string>>::encode(value.name);
    }
    if (value.configuration) {
        result["configuration"] = codec<std::optional<std::map<std::string, std::string>>>::encode(value.configuration);
    }
    return result;
}
inline holder::api::PatchLocationsLocationIdRequest codec<holder::api::PatchLocationsLocationIdRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected PatchLocationsLocationIdRequest");
    return holder::api::PatchLocationsLocationIdRequest{
        value.contains("name") ? codec<std::optional<std::string>>::decode(value.at("name")) : std::nullopt,
        value.contains("configuration") ? codec<std::optional<std::map<std::string, std::string>>>::decode(value.at("configuration")) : std::nullopt,
    };
}

inline json codec<holder::api::LocationActionResponseData>::encode(const holder::api::LocationActionResponseData& value) {
    json result = json::object();
    result["location_id"] = codec<std::string>::encode(value.location_id);
    if (value.bound) {
        result["bound"] = codec<std::optional<bool>>::encode(value.bound);
    }
    if (value.binding_preview) {
        result["binding_preview"] = codec<std::optional<std::optional<std::string>>>::encode(value.binding_preview);
    }
    return result;
}
inline holder::api::LocationActionResponseData codec<holder::api::LocationActionResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected LocationActionResponseData");
    if (!value.contains("location_id")) throw std::invalid_argument("missing location_id");
    return holder::api::LocationActionResponseData{
        codec<std::string>::decode(value.at("location_id")),
        value.contains("bound") ? codec<std::optional<bool>>::decode(value.at("bound")) : std::nullopt,
        value.contains("binding_preview") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("binding_preview"))} : std::nullopt,
    };
}

inline json codec<holder::api::LocationActionResponse>::encode(const holder::api::LocationActionResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::LocationActionResponseData>::encode(value.data);
    return result;
}
inline holder::api::LocationActionResponse codec<holder::api::LocationActionResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected LocationActionResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::LocationActionResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::LocationActionResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::LocationBindingRequest>::encode(const holder::api::LocationBindingRequest& value) {
    json result = json::object();
    result["values"] = codec<std::map<std::string, std::string>>::encode(value.values);
    if (value.preview) {
        result["preview"] = codec<std::optional<std::string>>::encode(value.preview);
    }
    return result;
}
inline holder::api::LocationBindingRequest codec<holder::api::LocationBindingRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected LocationBindingRequest");
    if (!value.contains("values")) throw std::invalid_argument("missing values");
    return holder::api::LocationBindingRequest{
        codec<std::map<std::string, std::string>>::decode(value.at("values")),
        value.contains("preview") ? codec<std::optional<std::string>>::decode(value.at("preview")) : std::nullopt,
    };
}

inline json codec<holder::api::LocationTestResponseData>::encode(const holder::api::LocationTestResponseData& value) {
    json result = json::object();
    result["available"] = codec<bool>::encode(value.available);
    return result;
}
inline holder::api::LocationTestResponseData codec<holder::api::LocationTestResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected LocationTestResponseData");
    if (!value.contains("available")) throw std::invalid_argument("missing available");
    return holder::api::LocationTestResponseData{
        codec<bool>::decode(value.at("available")),
    };
}

inline json codec<holder::api::LocationTestResponse>::encode(const holder::api::LocationTestResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::LocationTestResponseData>::encode(value.data);
    return result;
}
inline holder::api::LocationTestResponse codec<holder::api::LocationTestResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected LocationTestResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::LocationTestResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::LocationTestResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::PutLocationsPreferredRequest>::encode(const holder::api::PutLocationsPreferredRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["location_id"] = codec<std::string>::encode(value.location_id);
    return result;
}
inline holder::api::PutLocationsPreferredRequest codec<holder::api::PutLocationsPreferredRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected PutLocationsPreferredRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("location_id")) throw std::invalid_argument("missing location_id");
    return holder::api::PutLocationsPreferredRequest{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("location_id")),
    };
}

inline json codec<holder::api::ProjectCreateResponseData>::encode(const holder::api::ProjectCreateResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    return result;
}
inline holder::api::ProjectCreateResponseData codec<holder::api::ProjectCreateResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectCreateResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    return holder::api::ProjectCreateResponseData{
        codec<std::string>::decode(value.at("project_id")),
    };
}

inline json codec<holder::api::ProjectCreateResponse>::encode(const holder::api::ProjectCreateResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectCreateResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectCreateResponse codec<holder::api::ProjectCreateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectCreateResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectCreateResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectCreateResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectCreateRequest>::encode(const holder::api::ProjectCreateRequest& value) {
    json result = json::object();
    if (value.project_id) {
        result["project_id"] = codec<std::optional<std::string>>::encode(value.project_id);
    }
    result["name"] = codec<std::string>::encode(value.name);
    if (value.root_path) {
        result["root_path"] = codec<std::optional<std::string>>::encode(value.root_path);
    }
    if (value.git_remote_url) {
        result["git_remote_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.git_remote_url);
    }
    if (value.git_provider) {
        result["git_provider"] = codec<std::optional<std::optional<std::string>>>::encode(value.git_provider);
    }
    if (value.privacy_mode && *value.privacy_mode != "encrypted_git" && *value.privacy_mode != "plain") throw std::invalid_argument("invalid privacy_mode");
    if (value.privacy_mode) {
        result["privacy_mode"] = codec<std::optional<std::string>>::encode(value.privacy_mode);
    }
    if (value.project_key_id) {
        result["project_key_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.project_key_id);
    }
    if (value.created_at) {
        result["created_at"] = codec<std::optional<long long>>::encode(value.created_at);
    }
    if (value.updated_at) {
        result["updated_at"] = codec<std::optional<long long>>::encode(value.updated_at);
    }
    return result;
}
inline holder::api::ProjectCreateRequest codec<holder::api::ProjectCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectCreateRequest");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    if (value.contains("privacy_mode") && !value.at("privacy_mode").is_null() && value.at("privacy_mode") != "encrypted_git" && value.at("privacy_mode") != "plain") throw std::invalid_argument("invalid privacy_mode");
    return holder::api::ProjectCreateRequest{
        value.contains("project_id") ? codec<std::optional<std::string>>::decode(value.at("project_id")) : std::nullopt,
        codec<std::string>::decode(value.at("name")),
        value.contains("root_path") ? codec<std::optional<std::string>>::decode(value.at("root_path")) : std::nullopt,
        value.contains("git_remote_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("git_remote_url"))} : std::nullopt,
        value.contains("git_provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("git_provider"))} : std::nullopt,
        value.contains("privacy_mode") ? codec<std::optional<std::string>>::decode(value.at("privacy_mode")) : std::nullopt,
        value.contains("project_key_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("project_key_id"))} : std::nullopt,
        value.contains("created_at") ? codec<std::optional<long long>>::decode(value.at("created_at")) : std::nullopt,
        value.contains("updated_at") ? codec<std::optional<long long>>::decode(value.at("updated_at")) : std::nullopt,
    };
}

inline json codec<holder::api::ProjectSync>::encode(const holder::api::ProjectSync& value) {
    json result = json::object();
    result["last_commit_at"] = codec<std::optional<long long>>::encode(value.last_commit_at);
    result["last_push_at"] = codec<std::optional<long long>>::encode(value.last_push_at);
    result["last_pull_at"] = codec<std::optional<long long>>::encode(value.last_pull_at);
    result["uncommitted_changes_count"] = codec<int>::encode(value.uncommitted_changes_count);
    result["unpushed_commits_count"] = codec<int>::encode(value.unpushed_commits_count);
    result["last_push_status"] = codec<std::optional<std::string>>::encode(value.last_push_status);
    result["last_pull_status"] = codec<std::optional<std::string>>::encode(value.last_pull_status);
    result["last_sync_error"] = codec<std::optional<std::string>>::encode(value.last_sync_error);
    result["last_sync_error_at"] = codec<std::optional<long long>>::encode(value.last_sync_error_at);
    result["retry_count"] = codec<int>::encode(value.retry_count);
    result["next_retry_at"] = codec<std::optional<long long>>::encode(value.next_retry_at);
    result["pull_retry_count"] = codec<int>::encode(value.pull_retry_count);
    result["next_pull_retry_at"] = codec<std::optional<long long>>::encode(value.next_pull_retry_at);
    result["updated_at"] = codec<std::optional<long long>>::encode(value.updated_at);
    return result;
}
inline holder::api::ProjectSync codec<holder::api::ProjectSync>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectSync");
    if (!value.contains("last_commit_at")) throw std::invalid_argument("missing last_commit_at");
    if (!value.contains("last_push_at")) throw std::invalid_argument("missing last_push_at");
    if (!value.contains("last_pull_at")) throw std::invalid_argument("missing last_pull_at");
    if (!value.contains("uncommitted_changes_count")) throw std::invalid_argument("missing uncommitted_changes_count");
    if (!value.contains("unpushed_commits_count")) throw std::invalid_argument("missing unpushed_commits_count");
    if (!value.contains("last_push_status")) throw std::invalid_argument("missing last_push_status");
    if (!value.contains("last_pull_status")) throw std::invalid_argument("missing last_pull_status");
    if (!value.contains("last_sync_error")) throw std::invalid_argument("missing last_sync_error");
    if (!value.contains("last_sync_error_at")) throw std::invalid_argument("missing last_sync_error_at");
    if (!value.contains("retry_count")) throw std::invalid_argument("missing retry_count");
    if (!value.contains("next_retry_at")) throw std::invalid_argument("missing next_retry_at");
    if (!value.contains("pull_retry_count")) throw std::invalid_argument("missing pull_retry_count");
    if (!value.contains("next_pull_retry_at")) throw std::invalid_argument("missing next_pull_retry_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::ProjectSync{
        codec<std::optional<long long>>::decode(value.at("last_commit_at")),
        codec<std::optional<long long>>::decode(value.at("last_push_at")),
        codec<std::optional<long long>>::decode(value.at("last_pull_at")),
        codec<int>::decode(value.at("uncommitted_changes_count")),
        codec<int>::decode(value.at("unpushed_commits_count")),
        codec<std::optional<std::string>>::decode(value.at("last_push_status")),
        codec<std::optional<std::string>>::decode(value.at("last_pull_status")),
        codec<std::optional<std::string>>::decode(value.at("last_sync_error")),
        codec<std::optional<long long>>::decode(value.at("last_sync_error_at")),
        codec<int>::decode(value.at("retry_count")),
        codec<std::optional<long long>>::decode(value.at("next_retry_at")),
        codec<int>::decode(value.at("pull_retry_count")),
        codec<std::optional<long long>>::decode(value.at("next_pull_retry_at")),
        codec<std::optional<long long>>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::Project>::encode(const holder::api::Project& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["name"] = codec<std::string>::encode(value.name);
    result["root_path"] = codec<std::string>::encode(value.root_path);
    if (value.git_remote_url) {
        result["git_remote_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.git_remote_url);
    }
    if (value.git_provider) {
        result["git_provider"] = codec<std::optional<std::optional<std::string>>>::encode(value.git_provider);
    }
    if (value.privacy_mode != "encrypted_git" && value.privacy_mode != "plain") throw std::invalid_argument("invalid privacy_mode");
    result["privacy_mode"] = codec<std::string>::encode(value.privacy_mode);
    if (value.project_key_id) {
        result["project_key_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.project_key_id);
    }
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    if (value.card_count) {
        result["card_count"] = codec<std::optional<int>>::encode(value.card_count);
    }
    if (value.root_card_count) {
        result["root_card_count"] = codec<std::optional<int>>::encode(value.root_card_count);
    }
    result["sync"] = codec<holder::api::ProjectSync>::encode(value.sync);
    return result;
}
inline holder::api::Project codec<holder::api::Project>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected Project");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    if (!value.contains("root_path")) throw std::invalid_argument("missing root_path");
    if (!value.contains("privacy_mode")) throw std::invalid_argument("missing privacy_mode");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    if (!value.contains("sync")) throw std::invalid_argument("missing sync");
    if (value.at("privacy_mode") != "encrypted_git" && value.at("privacy_mode") != "plain") throw std::invalid_argument("invalid privacy_mode");
    return holder::api::Project{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("name")),
        codec<std::string>::decode(value.at("root_path")),
        value.contains("git_remote_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("git_remote_url"))} : std::nullopt,
        value.contains("git_provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("git_provider"))} : std::nullopt,
        codec<std::string>::decode(value.at("privacy_mode")),
        value.contains("project_key_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("project_key_id"))} : std::nullopt,
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
        value.contains("card_count") ? codec<std::optional<int>>::decode(value.at("card_count")) : std::nullopt,
        value.contains("root_card_count") ? codec<std::optional<int>>::decode(value.at("root_card_count")) : std::nullopt,
        codec<holder::api::ProjectSync>::decode(value.at("sync")),
    };
}

inline json codec<holder::api::ProjectGetResponse>::encode(const holder::api::ProjectGetResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::Project>::encode(value.data);
    return result;
}
inline holder::api::ProjectGetResponse codec<holder::api::ProjectGetResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGetResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectGetResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::Project>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectDeleteResponseData>::encode(const holder::api::ProjectDeleteResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    return result;
}
inline holder::api::ProjectDeleteResponseData codec<holder::api::ProjectDeleteResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectDeleteResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    return holder::api::ProjectDeleteResponseData{
        codec<std::string>::decode(value.at("project_id")),
    };
}

inline json codec<holder::api::ProjectDeleteResponse>::encode(const holder::api::ProjectDeleteResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectDeleteResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectDeleteResponse codec<holder::api::ProjectDeleteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectDeleteResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectDeleteResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectDeleteResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardBreadcrumb>::encode(const holder::api::CardBreadcrumb& value) {
    json result = json::object();
    if (value.kind != "project" && value.kind != "card") throw std::invalid_argument("invalid kind");
    result["kind"] = codec<std::string>::encode(value.kind);
    result["id"] = codec<std::string>::encode(value.id);
    result["title"] = codec<std::string>::encode(value.title);
    return result;
}
inline holder::api::CardBreadcrumb codec<holder::api::CardBreadcrumb>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardBreadcrumb");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("id")) throw std::invalid_argument("missing id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (value.at("kind") != "project" && value.at("kind") != "card") throw std::invalid_argument("invalid kind");
    return holder::api::CardBreadcrumb{
        codec<std::string>::decode(value.at("kind")),
        codec<std::string>::decode(value.at("id")),
        codec<std::string>::decode(value.at("title")),
    };
}

inline json codec<holder::api::CardBreadcrumbsResponse>::encode(const holder::api::CardBreadcrumbsResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::CardBreadcrumb>>::encode(value.data);
    return result;
}
inline holder::api::CardBreadcrumbsResponse codec<holder::api::CardBreadcrumbsResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardBreadcrumbsResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardBreadcrumbsResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::CardBreadcrumb>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectGitSyncStatusResponseData>::encode(const holder::api::ProjectGitSyncStatusResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["sync"] = codec<holder::api::ProjectSync>::encode(value.sync);
    return result;
}
inline holder::api::ProjectGitSyncStatusResponseData codec<holder::api::ProjectGitSyncStatusResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitSyncStatusResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("sync")) throw std::invalid_argument("missing sync");
    return holder::api::ProjectGitSyncStatusResponseData{
        codec<std::string>::decode(value.at("project_id")),
        codec<holder::api::ProjectSync>::decode(value.at("sync")),
    };
}

inline json codec<holder::api::ProjectGitSyncStatusResponse>::encode(const holder::api::ProjectGitSyncStatusResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectGitSyncStatusResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectGitSyncStatusResponse codec<holder::api::ProjectGitSyncStatusResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitSyncStatusResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectGitSyncStatusResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectGitSyncStatusResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectRecoveryTokenExportResponseData>::encode(const holder::api::ProjectRecoveryTokenExportResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["key_id"] = codec<std::string>::encode(value.key_id);
    result["recovery_token"] = codec<std::string>::encode(value.recovery_token);
    return result;
}
inline holder::api::ProjectRecoveryTokenExportResponseData codec<holder::api::ProjectRecoveryTokenExportResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectRecoveryTokenExportResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("key_id")) throw std::invalid_argument("missing key_id");
    if (!value.contains("recovery_token")) throw std::invalid_argument("missing recovery_token");
    return holder::api::ProjectRecoveryTokenExportResponseData{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("key_id")),
        codec<std::string>::decode(value.at("recovery_token")),
    };
}

inline json codec<holder::api::ProjectRecoveryTokenExportResponse>::encode(const holder::api::ProjectRecoveryTokenExportResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectRecoveryTokenExportResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectRecoveryTokenExportResponse codec<holder::api::ProjectRecoveryTokenExportResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectRecoveryTokenExportResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectRecoveryTokenExportResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectRecoveryTokenExportResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectRecoveryTokenExportRequest>::encode(const holder::api::ProjectRecoveryTokenExportRequest& value) {
    json result = json::object();
    result["pin"] = codec<std::string>::encode(value.pin);
    return result;
}
inline holder::api::ProjectRecoveryTokenExportRequest codec<holder::api::ProjectRecoveryTokenExportRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectRecoveryTokenExportRequest");
    if (!value.contains("pin")) throw std::invalid_argument("missing pin");
    return holder::api::ProjectRecoveryTokenExportRequest{
        codec<std::string>::decode(value.at("pin")),
    };
}

inline json codec<holder::api::ProjectRecoveryTokenImportResponseData>::encode(const holder::api::ProjectRecoveryTokenImportResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    return result;
}
inline holder::api::ProjectRecoveryTokenImportResponseData codec<holder::api::ProjectRecoveryTokenImportResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectRecoveryTokenImportResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    return holder::api::ProjectRecoveryTokenImportResponseData{
        codec<std::string>::decode(value.at("project_id")),
    };
}

inline json codec<holder::api::ProjectRecoveryTokenImportResponse>::encode(const holder::api::ProjectRecoveryTokenImportResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectRecoveryTokenImportResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectRecoveryTokenImportResponse codec<holder::api::ProjectRecoveryTokenImportResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectRecoveryTokenImportResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectRecoveryTokenImportResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectRecoveryTokenImportResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectRecoveryTokenImportRequest>::encode(const holder::api::ProjectRecoveryTokenImportRequest& value) {
    json result = json::object();
    result["pin"] = codec<std::string>::encode(value.pin);
    result["recovery_token"] = codec<std::string>::encode(value.recovery_token);
    return result;
}
inline holder::api::ProjectRecoveryTokenImportRequest codec<holder::api::ProjectRecoveryTokenImportRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectRecoveryTokenImportRequest");
    if (!value.contains("pin")) throw std::invalid_argument("missing pin");
    if (!value.contains("recovery_token")) throw std::invalid_argument("missing recovery_token");
    return holder::api::ProjectRecoveryTokenImportRequest{
        codec<std::string>::decode(value.at("pin")),
        codec<std::string>::decode(value.at("recovery_token")),
    };
}

inline json codec<holder::api::ProjectEncryptionCheck>::encode(const holder::api::ProjectEncryptionCheck& value) {
    json result = json::object();
    result["ok"] = codec<bool>::encode(value.ok);
    result["checked_files"] = codec<int>::encode(value.checked_files);
    result["unsafe_files"] = codec<int>::encode(value.unsafe_files);
    result["unsafe_paths"] = codec<std::vector<std::string>>::encode(value.unsafe_paths);
    result["message"] = codec<std::string>::encode(value.message);
    return result;
}
inline holder::api::ProjectEncryptionCheck codec<holder::api::ProjectEncryptionCheck>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectEncryptionCheck");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("checked_files")) throw std::invalid_argument("missing checked_files");
    if (!value.contains("unsafe_files")) throw std::invalid_argument("missing unsafe_files");
    if (!value.contains("unsafe_paths")) throw std::invalid_argument("missing unsafe_paths");
    if (!value.contains("message")) throw std::invalid_argument("missing message");
    return holder::api::ProjectEncryptionCheck{
        codec<bool>::decode(value.at("ok")),
        codec<int>::decode(value.at("checked_files")),
        codec<int>::decode(value.at("unsafe_files")),
        codec<std::vector<std::string>>::decode(value.at("unsafe_paths")),
        codec<std::string>::decode(value.at("message")),
    };
}

inline json codec<holder::api::ProjectEncryptionCheckResponseData>::encode(const holder::api::ProjectEncryptionCheckResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    if (value.privacy_mode != "encrypted_git" && value.privacy_mode != "plain") throw std::invalid_argument("invalid privacy_mode");
    result["privacy_mode"] = codec<std::string>::encode(value.privacy_mode);
    result["check"] = codec<holder::api::ProjectEncryptionCheck>::encode(value.check);
    return result;
}
inline holder::api::ProjectEncryptionCheckResponseData codec<holder::api::ProjectEncryptionCheckResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectEncryptionCheckResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("privacy_mode")) throw std::invalid_argument("missing privacy_mode");
    if (!value.contains("check")) throw std::invalid_argument("missing check");
    if (value.at("privacy_mode") != "encrypted_git" && value.at("privacy_mode") != "plain") throw std::invalid_argument("invalid privacy_mode");
    return holder::api::ProjectEncryptionCheckResponseData{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("privacy_mode")),
        codec<holder::api::ProjectEncryptionCheck>::decode(value.at("check")),
    };
}

inline json codec<holder::api::ProjectEncryptionCheckResponse>::encode(const holder::api::ProjectEncryptionCheckResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectEncryptionCheckResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectEncryptionCheckResponse codec<holder::api::ProjectEncryptionCheckResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectEncryptionCheckResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectEncryptionCheckResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectEncryptionCheckResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ReindexData>::encode(const holder::api::ReindexData& value) {
    json result = json::object();
    result["message"] = codec<std::string>::encode(value.message);
    return result;
}
inline holder::api::ReindexData codec<holder::api::ReindexData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ReindexData");
    if (!value.contains("message")) throw std::invalid_argument("missing message");
    return holder::api::ReindexData{
        codec<std::string>::decode(value.at("message")),
    };
}

inline json codec<holder::api::ReindexResponse>::encode(const holder::api::ReindexResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ReindexData>::encode(value.data);
    return result;
}
inline holder::api::ReindexResponse codec<holder::api::ReindexResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ReindexResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ReindexResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ReindexData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ResourceDeleteResponseData>::encode(const holder::api::ResourceDeleteResponseData& value) {
    json result = json::object();
    result["resource_id"] = codec<std::string>::encode(value.resource_id);
    return result;
}
inline holder::api::ResourceDeleteResponseData codec<holder::api::ResourceDeleteResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceDeleteResponseData");
    if (!value.contains("resource_id")) throw std::invalid_argument("missing resource_id");
    return holder::api::ResourceDeleteResponseData{
        codec<std::string>::decode(value.at("resource_id")),
    };
}

inline json codec<holder::api::ResourceDeleteResponse>::encode(const holder::api::ResourceDeleteResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ResourceDeleteResponseData>::encode(value.data);
    return result;
}
inline holder::api::ResourceDeleteResponse codec<holder::api::ResourceDeleteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceDeleteResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ResourceDeleteResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ResourceDeleteResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::SearchCardItem>::encode(const holder::api::SearchCardItem& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["snippet"] = codec<std::string>::encode(value.snippet);
    result["rank"] = codec<double>::encode(value.rank);
    return result;
}
inline holder::api::SearchCardItem codec<holder::api::SearchCardItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected SearchCardItem");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("snippet")) throw std::invalid_argument("missing snippet");
    if (!value.contains("rank")) throw std::invalid_argument("missing rank");
    return holder::api::SearchCardItem{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("title")),
        codec<long long>::decode(value.at("updated_at")),
        codec<long long>::decode(value.at("created_at")),
        codec<std::string>::decode(value.at("snippet")),
        codec<double>::decode(value.at("rank")),
    };
}

inline json codec<holder::api::SearchCardsResponse>::encode(const holder::api::SearchCardsResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::SearchCardItem>>::encode(value.data);
    return result;
}
inline holder::api::SearchCardsResponse codec<holder::api::SearchCardsResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected SearchCardsResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::SearchCardsResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::SearchCardItem>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::SearchMessageItem>::encode(const holder::api::SearchMessageItem& value) {
    json result = json::object();
    result["message_id"] = codec<std::string>::encode(value.message_id);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["snippet"] = codec<std::string>::encode(value.snippet);
    result["rank"] = codec<double>::encode(value.rank);
    return result;
}
inline holder::api::SearchMessageItem codec<holder::api::SearchMessageItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected SearchMessageItem");
    if (!value.contains("message_id")) throw std::invalid_argument("missing message_id");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("snippet")) throw std::invalid_argument("missing snippet");
    if (!value.contains("rank")) throw std::invalid_argument("missing rank");
    return holder::api::SearchMessageItem{
        codec<std::string>::decode(value.at("message_id")),
        codec<long long>::decode(value.at("created_at")),
        codec<std::string>::decode(value.at("snippet")),
        codec<double>::decode(value.at("rank")),
    };
}

inline json codec<holder::api::SearchMessagesResponse>::encode(const holder::api::SearchMessagesResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::SearchMessageItem>>::encode(value.data);
    return result;
}
inline holder::api::SearchMessagesResponse codec<holder::api::SearchMessagesResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected SearchMessagesResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::SearchMessagesResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::SearchMessageItem>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::TrashItem>::encode(const holder::api::TrashItem& value) {
    json result = json::object();
    result["type"] = codec<std::string>::encode(value.type);
    if (value.card_id) {
        result["card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.card_id);
    }
    if (value.message_id) {
        result["message_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.message_id);
    }
    if (value.thread_id) {
        result["thread_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.thread_id);
    }
    if (value.project_id) {
        result["project_id"] = codec<std::optional<std::string>>::encode(value.project_id);
    }
    if (value.title) {
        result["title"] = codec<std::optional<std::optional<std::string>>>::encode(value.title);
    }
    if (value.role) {
        result["role"] = codec<std::optional<std::optional<std::string>>>::encode(value.role);
    }
    if (value.deleted_at) {
        result["deleted_at"] = codec<std::optional<long long>>::encode(value.deleted_at);
    }
    if (value.created_at) {
        result["created_at"] = codec<std::optional<long long>>::encode(value.created_at);
    }
    if (value.rel_path) {
        result["rel_path"] = codec<std::optional<std::optional<std::string>>>::encode(value.rel_path);
    }
    return result;
}
inline holder::api::TrashItem codec<holder::api::TrashItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected TrashItem");
    if (!value.contains("type")) throw std::invalid_argument("missing type");
    return holder::api::TrashItem{
        codec<std::string>::decode(value.at("type")),
        value.contains("card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("card_id"))} : std::nullopt,
        value.contains("message_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("message_id"))} : std::nullopt,
        value.contains("thread_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("thread_id"))} : std::nullopt,
        value.contains("project_id") ? codec<std::optional<std::string>>::decode(value.at("project_id")) : std::nullopt,
        value.contains("title") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("title"))} : std::nullopt,
        value.contains("role") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("role"))} : std::nullopt,
        value.contains("deleted_at") ? codec<std::optional<long long>>::decode(value.at("deleted_at")) : std::nullopt,
        value.contains("created_at") ? codec<std::optional<long long>>::decode(value.at("created_at")) : std::nullopt,
        value.contains("rel_path") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("rel_path"))} : std::nullopt,
    };
}

inline json codec<holder::api::TrashListResponse>::encode(const holder::api::TrashListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::TrashItem>>::encode(value.data);
    return result;
}
inline holder::api::TrashListResponse codec<holder::api::TrashListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected TrashListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::TrashListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::TrashItem>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::TrashDeleteResponse>::encode(const holder::api::TrashDeleteResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<nlohmann::json>::encode(value.data);
    return result;
}
inline holder::api::TrashDeleteResponse codec<holder::api::TrashDeleteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected TrashDeleteResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::TrashDeleteResponse{
        codec<bool>::decode(value.at("ok")),
        codec<nlohmann::json>::decode(value.at("data")),
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
} // namespace rig_json
