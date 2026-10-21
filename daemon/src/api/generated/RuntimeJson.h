// Generated from RIG IR; do not edit.
#pragma once
#include "api/RigRuntime.h"
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
template <> struct codec<holder::api::CasteInfo> {
    static json encode(const holder::api::CasteInfo& value);
    static holder::api::CasteInfo decode(const json& value);
};
template <> struct codec<holder::api::AiLocalModelConfigData> {
    static json encode(const holder::api::AiLocalModelConfigData& value);
    static holder::api::AiLocalModelConfigData decode(const json& value);
};
template <> struct codec<holder::api::LocalModel> {
    static json encode(const holder::api::LocalModel& value);
    static holder::api::LocalModel decode(const json& value);
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
template <> struct codec<holder::api::AiStatusPullJob> {
    static json encode(const holder::api::AiStatusPullJob& value);
    static holder::api::AiStatusPullJob decode(const json& value);
};
template <> struct codec<holder::api::AiStatusData> {
    static json encode(const holder::api::AiStatusData& value);
    static holder::api::AiStatusData decode(const json& value);
};
template <> struct codec<holder::api::AiStatusResponse> {
    static json encode(const holder::api::AiStatusResponse& value);
    static holder::api::AiStatusResponse decode(const json& value);
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
template <> struct codec<holder::api::AiLocalModelConfigResponse> {
    static json encode(const holder::api::AiLocalModelConfigResponse& value);
    static holder::api::AiLocalModelConfigResponse decode(const json& value);
};
template <> struct codec<holder::api::AiLocalModelConfigUpdateRequest> {
    static json encode(const holder::api::AiLocalModelConfigUpdateRequest& value);
    static holder::api::AiLocalModelConfigUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiRunEventData> {
    static json encode(const holder::api::AiRunEventData& value);
    static holder::api::AiRunEventData decode(const json& value);
};
template <> struct codec<holder::api::AiRunCreateRequest> {
    static json encode(const holder::api::AiRunCreateRequest& value);
    static holder::api::AiRunCreateRequest decode(const json& value);
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
template <> struct codec<holder::api::CardLinkDeleteRequest> {
    static json encode(const holder::api::CardLinkDeleteRequest& value);
    static holder::api::CardLinkDeleteRequest decode(const json& value);
};
template <> struct codec<holder::api::ProjectSync> {
    static json encode(const holder::api::ProjectSync& value);
    static holder::api::ProjectSync decode(const json& value);
};
template <> struct codec<holder::api::Project> {
    static json encode(const holder::api::Project& value);
    static holder::api::Project decode(const json& value);
};
template <> struct codec<holder::api::HierarchyRepairMove> {
    static json encode(const holder::api::HierarchyRepairMove& value);
    static holder::api::HierarchyRepairMove decode(const json& value);
};
template <> struct codec<holder::api::HierarchyRepair> {
    static json encode(const holder::api::HierarchyRepair& value);
    static holder::api::HierarchyRepair decode(const json& value);
};
template <> struct codec<holder::api::HierarchyRepairResponse> {
    static json encode(const holder::api::HierarchyRepairResponse& value);
    static holder::api::HierarchyRepairResponse decode(const json& value);
};
template <> struct codec<holder::api::HierarchyRepairRequest> {
    static json encode(const holder::api::HierarchyRepairRequest& value);
    static holder::api::HierarchyRepairRequest decode(const json& value);
};
template <> struct codec<holder::api::CardListItem> {
    static json encode(const holder::api::CardListItem& value);
    static holder::api::CardListItem decode(const json& value);
};
template <> struct codec<holder::api::CardPageItem> {
    static json encode(const holder::api::CardPageItem& value);
    static holder::api::CardPageItem decode(const json& value);
};
template <> struct codec<holder::api::CardPage> {
    static json encode(const holder::api::CardPage& value);
    static holder::api::CardPage decode(const json& value);
};
template <> struct codec<holder::api::CardPageResponse> {
    static json encode(const holder::api::CardPageResponse& value);
    static holder::api::CardPageResponse decode(const json& value);
};
template <> struct codec<holder::api::ParentRoots> {
    static json encode(const holder::api::ParentRoots& value);
    static holder::api::ParentRoots decode(const json& value);
};
template <> struct codec<holder::api::ParentUuid> {
    static json encode(const holder::api::ParentUuid& value);
    static holder::api::ParentUuid decode(const json& value);
};
template <> struct codec<holder::api::CardBreadcrumb> {
    static json encode(const holder::api::CardBreadcrumb& value);
    static holder::api::CardBreadcrumb decode(const json& value);
};
template <> struct codec<holder::api::CardBreadcrumbsResponse> {
    static json encode(const holder::api::CardBreadcrumbsResponse& value);
    static holder::api::CardBreadcrumbsResponse decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryAuthor> {
    static json encode(const holder::api::CardHistoryAuthor& value);
    static holder::api::CardHistoryAuthor decode(const json& value);
};
template <> struct codec<holder::api::ProjectHistoryAffectedObjectItemsItem> {
    static json encode(const holder::api::ProjectHistoryAffectedObjectItemsItem& value);
    static holder::api::ProjectHistoryAffectedObjectItemsItem decode(const json& value);
};
template <> struct codec<holder::api::ProjectHistoryAffectedObject> {
    static json encode(const holder::api::ProjectHistoryAffectedObject& value);
    static holder::api::ProjectHistoryAffectedObject decode(const json& value);
};
template <> struct codec<holder::api::ProjectHistoryActivity> {
    static json encode(const holder::api::ProjectHistoryActivity& value);
    static holder::api::ProjectHistoryActivity decode(const json& value);
};
template <> struct codec<holder::api::ProjectHistoryPageData> {
    static json encode(const holder::api::ProjectHistoryPageData& value);
    static holder::api::ProjectHistoryPageData decode(const json& value);
};
template <> struct codec<holder::api::ProjectHistoryPageResponse> {
    static json encode(const holder::api::ProjectHistoryPageResponse& value);
    static holder::api::ProjectHistoryPageResponse decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryEntrySavesItem> {
    static json encode(const holder::api::CardHistoryEntrySavesItem& value);
    static holder::api::CardHistoryEntrySavesItem decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryEntry> {
    static json encode(const holder::api::CardHistoryEntry& value);
    static holder::api::CardHistoryEntry decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryPageData> {
    static json encode(const holder::api::CardHistoryPageData& value);
    static holder::api::CardHistoryPageData decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryPageResponse> {
    static json encode(const holder::api::CardHistoryPageResponse& value);
    static holder::api::CardHistoryPageResponse decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryVersion> {
    static json encode(const holder::api::CardHistoryVersion& value);
    static holder::api::CardHistoryVersion decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryDiffLine> {
    static json encode(const holder::api::CardHistoryDiffLine& value);
    static holder::api::CardHistoryDiffLine decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryComparisonData> {
    static json encode(const holder::api::CardHistoryComparisonData& value);
    static holder::api::CardHistoryComparisonData decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryComparisonResponse> {
    static json encode(const holder::api::CardHistoryComparisonResponse& value);
    static holder::api::CardHistoryComparisonResponse decode(const json& value);
};
template <> struct codec<holder::api::RevisionReference> {
    static json encode(const holder::api::RevisionReference& value);
    static holder::api::RevisionReference decode(const json& value);
};
template <> struct codec<holder::api::CardHistorySnapshotResponseData> {
    static json encode(const holder::api::CardHistorySnapshotResponseData& value);
    static holder::api::CardHistorySnapshotResponseData decode(const json& value);
};
template <> struct codec<holder::api::CardHistorySnapshotResponse> {
    static json encode(const holder::api::CardHistorySnapshotResponse& value);
    static holder::api::CardHistorySnapshotResponse decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryRestoreResponseData> {
    static json encode(const holder::api::CardHistoryRestoreResponseData& value);
    static holder::api::CardHistoryRestoreResponseData decode(const json& value);
};
template <> struct codec<holder::api::CardHistoryRestoreResponse> {
    static json encode(const holder::api::CardHistoryRestoreResponse& value);
    static holder::api::CardHistoryRestoreResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitTestRemoteResponseData> {
    static json encode(const holder::api::ProjectGitTestRemoteResponseData& value);
    static holder::api::ProjectGitTestRemoteResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitTestRemoteResponse> {
    static json encode(const holder::api::ProjectGitTestRemoteResponse& value);
    static holder::api::ProjectGitTestRemoteResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitTestRemoteRequest> {
    static json encode(const holder::api::ProjectGitTestRemoteRequest& value);
    static holder::api::ProjectGitTestRemoteRequest decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitPushResponseData> {
    static json encode(const holder::api::ProjectGitPushResponseData& value);
    static holder::api::ProjectGitPushResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitPushResponse> {
    static json encode(const holder::api::ProjectGitPushResponse& value);
    static holder::api::ProjectGitPushResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitPushRequest> {
    static json encode(const holder::api::ProjectGitPushRequest& value);
    static holder::api::ProjectGitPushRequest decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncOperationResponseDataPull> {
    static json encode(const holder::api::ProjectGitSyncOperationResponseDataPull& value);
    static holder::api::ProjectGitSyncOperationResponseDataPull decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncOperationResponseDataPushStatus> {
    static json encode(const holder::api::ProjectGitSyncOperationResponseDataPushStatus& value);
    static holder::api::ProjectGitSyncOperationResponseDataPushStatus decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncOperationResponseDataPush> {
    static json encode(const holder::api::ProjectGitSyncOperationResponseDataPush& value);
    static holder::api::ProjectGitSyncOperationResponseDataPush decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncOperationResponseData> {
    static json encode(const holder::api::ProjectGitSyncOperationResponseData& value);
    static holder::api::ProjectGitSyncOperationResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncOperationResponse> {
    static json encode(const holder::api::ProjectGitSyncOperationResponse& value);
    static holder::api::ProjectGitSyncOperationResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncRequest> {
    static json encode(const holder::api::ProjectGitSyncRequest& value);
    static holder::api::ProjectGitSyncRequest decode(const json& value);
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
template <> struct codec<holder::api::MilestoneDateValue> {
    static json encode(const holder::api::MilestoneDateValue& value);
    static holder::api::MilestoneDateValue decode(const json& value);
};
template <> struct codec<holder::api::MilestoneWithDates> {
    static json encode(const holder::api::MilestoneWithDates& value);
    static holder::api::MilestoneWithDates decode(const json& value);
};
template <> struct codec<holder::api::MilestoneWithDatesListResponse> {
    static json encode(const holder::api::MilestoneWithDatesListResponse& value);
    static holder::api::MilestoneWithDatesListResponse decode(const json& value);
};
template <> struct codec<holder::api::TrashDeleteResponse> {
    static json encode(const holder::api::TrashDeleteResponse& value);
    static holder::api::TrashDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::MilestoneWithDatesResponse> {
    static json encode(const holder::api::MilestoneWithDatesResponse& value);
    static holder::api::MilestoneWithDatesResponse decode(const json& value);
};
template <> struct codec<holder::api::MilestoneWithDatesCreateRequest> {
    static json encode(const holder::api::MilestoneWithDatesCreateRequest& value);
    static holder::api::MilestoneWithDatesCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::MilestoneWithDatesUpdateRequest> {
    static json encode(const holder::api::MilestoneWithDatesUpdateRequest& value);
    static holder::api::MilestoneWithDatesUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::MilestoneRemoveResult> {
    static json encode(const holder::api::MilestoneRemoveResult& value);
    static holder::api::MilestoneRemoveResult decode(const json& value);
};
template <> struct codec<holder::api::MilestoneRemoveResponse> {
    static json encode(const holder::api::MilestoneRemoveResponse& value);
    static holder::api::MilestoneRemoveResponse decode(const json& value);
};
template <> struct codec<holder::api::Milestone> {
    static json encode(const holder::api::Milestone& value);
    static holder::api::Milestone decode(const json& value);
};
template <> struct codec<holder::api::MilestoneListResponse> {
    static json encode(const holder::api::MilestoneListResponse& value);
    static holder::api::MilestoneListResponse decode(const json& value);
};
template <> struct codec<holder::api::MilestoneResponse> {
    static json encode(const holder::api::MilestoneResponse& value);
    static holder::api::MilestoneResponse decode(const json& value);
};
template <> struct codec<holder::api::MilestoneCreateRequest> {
    static json encode(const holder::api::MilestoneCreateRequest& value);
    static holder::api::MilestoneCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::MilestoneUpdateRequest> {
    static json encode(const holder::api::MilestoneUpdateRequest& value);
    static holder::api::MilestoneUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::CardListResponse> {
    static json encode(const holder::api::CardListResponse& value);
    static holder::api::CardListResponse decode(const json& value);
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
template <> struct codec<holder::api::CardContextDataProject> {
    static json encode(const holder::api::CardContextDataProject& value);
    static holder::api::CardContextDataProject decode(const json& value);
};
template <> struct codec<holder::api::CardContextBreadcrumb> {
    static json encode(const holder::api::CardContextBreadcrumb& value);
    static holder::api::CardContextBreadcrumb decode(const json& value);
};
template <> struct codec<holder::api::CardContextCard> {
    static json encode(const holder::api::CardContextCard& value);
    static holder::api::CardContextCard decode(const json& value);
};
template <> struct codec<holder::api::CardContextData> {
    static json encode(const holder::api::CardContextData& value);
    static holder::api::CardContextData decode(const json& value);
};
template <> struct codec<holder::api::CardContextResponse> {
    static json encode(const holder::api::CardContextResponse& value);
    static holder::api::CardContextResponse decode(const json& value);
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
template <> struct codec<holder::api::CardMoveResult> {
    static json encode(const holder::api::CardMoveResult& value);
    static holder::api::CardMoveResult decode(const json& value);
};
template <> struct codec<holder::api::CardMoveResponse> {
    static json encode(const holder::api::CardMoveResponse& value);
    static holder::api::CardMoveResponse decode(const json& value);
};
template <> struct codec<holder::api::CardMoveIntent> {
    static json encode(const holder::api::CardMoveIntent& value);
    static holder::api::CardMoveIntent decode(const json& value);
};
template <> struct codec<holder::api::CardMoveRequest> {
    static json encode(const holder::api::CardMoveRequest& value);
    static holder::api::CardMoveRequest decode(const json& value);
};
template <> struct codec<holder::api::CardTagMutationResult> {
    static json encode(const holder::api::CardTagMutationResult& value);
    static holder::api::CardTagMutationResult decode(const json& value);
};
template <> struct codec<holder::api::CardTagMutationResponse> {
    static json encode(const holder::api::CardTagMutationResponse& value);
    static holder::api::CardTagMutationResponse decode(const json& value);
};
template <> struct codec<holder::api::CardTagMutationRequest> {
    static json encode(const holder::api::CardTagMutationRequest& value);
    static holder::api::CardTagMutationRequest decode(const json& value);
};
template <> struct codec<holder::api::ResourceAttachmentResponseData> {
    static json encode(const holder::api::ResourceAttachmentResponseData& value);
    static holder::api::ResourceAttachmentResponseData decode(const json& value);
};
template <> struct codec<holder::api::ResourceAttachmentResponse> {
    static json encode(const holder::api::ResourceAttachmentResponse& value);
    static holder::api::ResourceAttachmentResponse decode(const json& value);
};
template <> struct codec<holder::api::PostCardsCardIdResourcesRequest> {
    static json encode(const holder::api::PostCardsCardIdResourcesRequest& value);
    static holder::api::PostCardsCardIdResourcesRequest decode(const json& value);
};
template <> struct codec<holder::api::DeleteCardsCardIdResourcesRequest> {
    static json encode(const holder::api::DeleteCardsCardIdResourcesRequest& value);
    static holder::api::DeleteCardsCardIdResourcesRequest decode(const json& value);
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
template <> struct codec<holder::api::ResourceMetadata> {
    static json encode(const holder::api::ResourceMetadata& value);
    static holder::api::ResourceMetadata decode(const json& value);
};
template <> struct codec<holder::api::AssetPlacement> {
    static json encode(const holder::api::AssetPlacement& value);
    static holder::api::AssetPlacement decode(const json& value);
};
template <> struct codec<holder::api::ResourceAsset> {
    static json encode(const holder::api::ResourceAsset& value);
    static holder::api::ResourceAsset decode(const json& value);
};
template <> struct codec<holder::api::ResourceReferencedByCardsItem> {
    static json encode(const holder::api::ResourceReferencedByCardsItem& value);
    static holder::api::ResourceReferencedByCardsItem decode(const json& value);
};
template <> struct codec<holder::api::Resource> {
    static json encode(const holder::api::Resource& value);
    static holder::api::Resource decode(const json& value);
};
template <> struct codec<holder::api::ResourceListResponse> {
    static json encode(const holder::api::ResourceListResponse& value);
    static holder::api::ResourceListResponse decode(const json& value);
};
template <> struct codec<holder::api::ResourceResponse> {
    static json encode(const holder::api::ResourceResponse& value);
    static holder::api::ResourceResponse decode(const json& value);
};
template <> struct codec<holder::api::ResourceCreateRequest> {
    static json encode(const holder::api::ResourceCreateRequest& value);
    static holder::api::ResourceCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::ResourceUpdateRequest> {
    static json encode(const holder::api::ResourceUpdateRequest& value);
    static holder::api::ResourceUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::ResourceDeleteResponseData> {
    static json encode(const holder::api::ResourceDeleteResponseData& value);
    static holder::api::ResourceDeleteResponseData decode(const json& value);
};
template <> struct codec<holder::api::ResourceDeleteResponse> {
    static json encode(const holder::api::ResourceDeleteResponse& value);
    static holder::api::ResourceDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::EventCheckpoint> {
    static json encode(const holder::api::EventCheckpoint& value);
    static holder::api::EventCheckpoint decode(const json& value);
};
template <> struct codec<holder::api::EventCheckpointResponse> {
    static json encode(const holder::api::EventCheckpointResponse& value);
    static holder::api::EventCheckpointResponse decode(const json& value);
};
template <> struct codec<holder::api::ChangeEventData> {
    static json encode(const holder::api::ChangeEventData& value);
    static holder::api::ChangeEventData decode(const json& value);
};
template <> struct codec<holder::api::EventResyncData> {
    static json encode(const holder::api::EventResyncData& value);
    static holder::api::EventResyncData decode(const json& value);
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
template <> struct codec<holder::api::ReindexData> {
    static json encode(const holder::api::ReindexData& value);
    static holder::api::ReindexData decode(const json& value);
};
template <> struct codec<holder::api::ReindexResponse> {
    static json encode(const holder::api::ReindexResponse& value);
    static holder::api::ReindexResponse decode(const json& value);
};
template <> struct codec<holder::api::RecoveryTokenImportAutoResponseData> {
    static json encode(const holder::api::RecoveryTokenImportAutoResponseData& value);
    static holder::api::RecoveryTokenImportAutoResponseData decode(const json& value);
};
template <> struct codec<holder::api::RecoveryTokenImportAutoResponse> {
    static json encode(const holder::api::RecoveryTokenImportAutoResponse& value);
    static holder::api::RecoveryTokenImportAutoResponse decode(const json& value);
};
template <> struct codec<holder::api::AiThreadListResponse> {
    static json encode(const holder::api::AiThreadListResponse& value);
    static holder::api::AiThreadListResponse decode(const json& value);
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
template <> struct codec<holder::api::HealthDataPrivacy> {
    static json encode(const holder::api::HealthDataPrivacy& value);
    static holder::api::HealthDataPrivacy decode(const json& value);
};
template <> struct codec<holder::api::HealthData> {
    static json encode(const holder::api::HealthData& value);
    static holder::api::HealthData decode(const json& value);
};
template <> struct codec<holder::api::HealthResponse> {
    static json encode(const holder::api::HealthResponse& value);
    static holder::api::HealthResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectTag> {
    static json encode(const holder::api::ProjectTag& value);
    static holder::api::ProjectTag decode(const json& value);
};
template <> struct codec<holder::api::ProjectTagListResponse> {
    static json encode(const holder::api::ProjectTagListResponse& value);
    static holder::api::ProjectTagListResponse decode(const json& value);
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
template <> struct codec<holder::api::ProjectListResponse> {
    static json encode(const holder::api::ProjectListResponse& value);
    static holder::api::ProjectListResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectGetResponse> {
    static json encode(const holder::api::ProjectGetResponse& value);
    static holder::api::ProjectGetResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncStatusResponseData> {
    static json encode(const holder::api::ProjectGitSyncStatusResponseData& value);
    static holder::api::ProjectGitSyncStatusResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectGitSyncStatusResponse> {
    static json encode(const holder::api::ProjectGitSyncStatusResponse& value);
    static holder::api::ProjectGitSyncStatusResponse decode(const json& value);
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
template <> struct codec<holder::api::ProjectCreateRequest> {
    static json encode(const holder::api::ProjectCreateRequest& value);
    static holder::api::ProjectCreateRequest decode(const json& value);
};
template <> struct codec<holder::api::ProjectCreateResponse> {
    static json encode(const holder::api::ProjectCreateResponse& value);
    static holder::api::ProjectCreateResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectUpdateResponseData> {
    static json encode(const holder::api::ProjectUpdateResponseData& value);
    static holder::api::ProjectUpdateResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectUpdateResponse> {
    static json encode(const holder::api::ProjectUpdateResponse& value);
    static holder::api::ProjectUpdateResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectUpdateRequestPrivacyMode> {
    static json encode(const holder::api::ProjectUpdateRequestPrivacyMode& value);
    static holder::api::ProjectUpdateRequestPrivacyMode decode(const json& value);
};
template <> struct codec<holder::api::ProjectUpdateRequest> {
    static json encode(const holder::api::ProjectUpdateRequest& value);
    static holder::api::ProjectUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::ProjectDeleteResponseData> {
    static json encode(const holder::api::ProjectDeleteResponseData& value);
    static holder::api::ProjectDeleteResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectDeleteResponse> {
    static json encode(const holder::api::ProjectDeleteResponse& value);
    static holder::api::ProjectDeleteResponse decode(const json& value);
};
template <> struct codec<holder::api::CalendarCardActivity> {
    static json encode(const holder::api::CalendarCardActivity& value);
    static holder::api::CalendarCardActivity decode(const json& value);
};
template <> struct codec<holder::api::ProjectCalendarResponseData> {
    static json encode(const holder::api::ProjectCalendarResponseData& value);
    static holder::api::ProjectCalendarResponseData decode(const json& value);
};
template <> struct codec<holder::api::ProjectCalendarResponse> {
    static json encode(const holder::api::ProjectCalendarResponse& value);
    static holder::api::ProjectCalendarResponse decode(const json& value);
};
template <> struct codec<holder::api::TrashItem> {
    static json encode(const holder::api::TrashItem& value);
    static holder::api::TrashItem decode(const json& value);
};
template <> struct codec<holder::api::TrashListResponse> {
    static json encode(const holder::api::TrashListResponse& value);
    static holder::api::TrashListResponse decode(const json& value);
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
template <> struct codec<holder::api::routes::CardBacklinkListResponse> {
    static json encode(const holder::api::routes::CardBacklinkListResponse& value);
    static holder::api::routes::CardBacklinkListResponse decode(const json& value);
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

inline json codec<holder::api::CasteInfo>::encode(const holder::api::CasteInfo& value) {
    json result = json::object();
    if (value.name != "mini" && value.name != "user" && value.name != "developer" && value.name != "workstation" && value.name != "rig") throw std::invalid_argument("invalid name");
    result["name"] = codec<std::string>::encode(value.name);
    if (value.reason) {
        result["reason"] = codec<std::optional<std::optional<std::string>>>::encode(value.reason);
    }
    return result;
}
inline holder::api::CasteInfo codec<holder::api::CasteInfo>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CasteInfo");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    if (value.at("name") != "mini" && value.at("name") != "user" && value.at("name") != "developer" && value.at("name") != "workstation" && value.at("name") != "rig") throw std::invalid_argument("invalid name");
    return holder::api::CasteInfo{
        codec<std::string>::decode(value.at("name")),
        value.contains("reason") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("reason"))} : std::nullopt,
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

inline json codec<holder::api::CasteRecommendedModel>::encode(const holder::api::CasteRecommendedModel& value) {
    json result = json::object();
    result["tag"] = codec<std::string>::encode(value.tag);
    if (value.provider) {
        result["provider"] = codec<std::optional<std::optional<std::string>>>::encode(value.provider);
    }
    if (value.engine) {
        result["engine"] = codec<std::optional<std::optional<std::string>>>::encode(value.engine);
    }
    if (value.category) {
        result["category"] = codec<std::optional<std::optional<std::string>>>::encode(value.category);
    }
    if (value.required_caste != "mini" && value.required_caste != "user" && value.required_caste != "developer" && value.required_caste != "workstation" && value.required_caste != "rig") throw std::invalid_argument("invalid required_caste");
    result["required_caste"] = codec<std::string>::encode(value.required_caste);
    result["installed"] = codec<bool>::encode(value.installed);
    return result;
}
inline holder::api::CasteRecommendedModel codec<holder::api::CasteRecommendedModel>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CasteRecommendedModel");
    if (!value.contains("tag")) throw std::invalid_argument("missing tag");
    if (!value.contains("required_caste")) throw std::invalid_argument("missing required_caste");
    if (!value.contains("installed")) throw std::invalid_argument("missing installed");
    if (value.at("required_caste") != "mini" && value.at("required_caste") != "user" && value.at("required_caste") != "developer" && value.at("required_caste") != "workstation" && value.at("required_caste") != "rig") throw std::invalid_argument("invalid required_caste");
    return holder::api::CasteRecommendedModel{
        codec<std::string>::decode(value.at("tag")),
        value.contains("provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("provider"))} : std::nullopt,
        value.contains("engine") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("engine"))} : std::nullopt,
        value.contains("category") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("category"))} : std::nullopt,
        codec<std::string>::decode(value.at("required_caste")),
        codec<bool>::decode(value.at("installed")),
    };
}

inline json codec<holder::api::RunnerCapabilitiesData>::encode(const holder::api::RunnerCapabilitiesData& value) {
    json result = json::object();
    result["runner_available"] = codec<bool>::encode(value.runner_available);
    if (value.spawn_attempted) {
        result["spawn_attempted"] = codec<std::optional<bool>>::encode(value.spawn_attempted);
    }
    result["last_checked"] = codec<long long>::encode(value.last_checked);
    if (value.version) {
        result["version"] = codec<std::optional<std::string>>::encode(value.version);
    }
    if (value.error) {
        result["error"] = codec<std::optional<std::optional<std::string>>>::encode(value.error);
    }
    if (value.caste) {
        result["caste"] = codec<std::optional<std::optional<holder::api::CasteInfo>>>::encode(value.caste);
    }
    result["local_model_config"] = codec<holder::api::AiLocalModelConfigData>::encode(value.local_model_config);
    result["models"] = codec<std::vector<holder::api::LocalModel>>::encode(value.models);
    result["recommended_models"] = codec<std::vector<holder::api::CasteRecommendedModel>>::encode(value.recommended_models);
    result["recommended_install"] = codec<std::vector<holder::api::CasteRecommendedModel>>::encode(value.recommended_install);
    return result;
}
inline holder::api::RunnerCapabilitiesData codec<holder::api::RunnerCapabilitiesData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RunnerCapabilitiesData");
    if (!value.contains("runner_available")) throw std::invalid_argument("missing runner_available");
    if (!value.contains("last_checked")) throw std::invalid_argument("missing last_checked");
    if (!value.contains("local_model_config")) throw std::invalid_argument("missing local_model_config");
    if (!value.contains("models")) throw std::invalid_argument("missing models");
    if (!value.contains("recommended_models")) throw std::invalid_argument("missing recommended_models");
    if (!value.contains("recommended_install")) throw std::invalid_argument("missing recommended_install");
    return holder::api::RunnerCapabilitiesData{
        codec<bool>::decode(value.at("runner_available")),
        value.contains("spawn_attempted") ? codec<std::optional<bool>>::decode(value.at("spawn_attempted")) : std::nullopt,
        codec<long long>::decode(value.at("last_checked")),
        value.contains("version") ? codec<std::optional<std::string>>::decode(value.at("version")) : std::nullopt,
        value.contains("error") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("error"))} : std::nullopt,
        value.contains("caste") ? std::optional<std::optional<holder::api::CasteInfo>>{std::in_place, codec<std::optional<holder::api::CasteInfo>>::decode(value.at("caste"))} : std::nullopt,
        codec<holder::api::AiLocalModelConfigData>::decode(value.at("local_model_config")),
        codec<std::vector<holder::api::LocalModel>>::decode(value.at("models")),
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

inline json codec<holder::api::AiStatusPullJob>::encode(const holder::api::AiStatusPullJob& value) {
    json result = codec<holder::api::RunnerPullJob>::encode(value);
    result["active"] = codec<bool>::encode(value.active);
    return result;
}
inline holder::api::AiStatusPullJob codec<holder::api::AiStatusPullJob>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiStatusPullJob");
    if (!value.contains("active")) throw std::invalid_argument("missing active");
    json base = value;
    base.erase("active");
    return holder::api::AiStatusPullJob{
        codec<holder::api::RunnerPullJob>::decode(base),
        codec<bool>::decode(value.at("active")),
    };
}

inline json codec<holder::api::AiStatusData>::encode(const holder::api::AiStatusData& value) {
    json result = json::object();
    result["checked_at"] = codec<long long>::encode(value.checked_at);
    result["runner_available"] = codec<bool>::encode(value.runner_available);
    result["runner_last_checked"] = codec<long long>::encode(value.runner_last_checked);
    if (value.runner_version) {
        result["runner_version"] = codec<std::optional<std::optional<std::string>>>::encode(value.runner_version);
    }
    if (value.runner_error) {
        result["runner_error"] = codec<std::optional<std::optional<std::string>>>::encode(value.runner_error);
    }
    result["active_runs"] = codec<long long>::encode(value.active_runs);
    result["active_pull_jobs"] = codec<long long>::encode(value.active_pull_jobs);
    result["pulls"] = codec<std::vector<holder::api::AiStatusPullJob>>::encode(value.pulls);
    result["cloud"] = codec<std::vector<holder::api::AiProviderCredential>>::encode(value.cloud);
    result["cloud_configured_providers"] = codec<long long>::encode(value.cloud_configured_providers);
    return result;
}
inline holder::api::AiStatusData codec<holder::api::AiStatusData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiStatusData");
    if (!value.contains("checked_at")) throw std::invalid_argument("missing checked_at");
    if (!value.contains("runner_available")) throw std::invalid_argument("missing runner_available");
    if (!value.contains("runner_last_checked")) throw std::invalid_argument("missing runner_last_checked");
    if (!value.contains("active_runs")) throw std::invalid_argument("missing active_runs");
    if (!value.contains("active_pull_jobs")) throw std::invalid_argument("missing active_pull_jobs");
    if (!value.contains("pulls")) throw std::invalid_argument("missing pulls");
    if (!value.contains("cloud")) throw std::invalid_argument("missing cloud");
    if (!value.contains("cloud_configured_providers")) throw std::invalid_argument("missing cloud_configured_providers");
    return holder::api::AiStatusData{
        codec<long long>::decode(value.at("checked_at")),
        codec<bool>::decode(value.at("runner_available")),
        codec<long long>::decode(value.at("runner_last_checked")),
        value.contains("runner_version") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("runner_version"))} : std::nullopt,
        value.contains("runner_error") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("runner_error"))} : std::nullopt,
        codec<long long>::decode(value.at("active_runs")),
        codec<long long>::decode(value.at("active_pull_jobs")),
        codec<std::vector<holder::api::AiStatusPullJob>>::decode(value.at("pulls")),
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
    };
}

inline json codec<holder::api::AiRunCreateRequest>::encode(const holder::api::AiRunCreateRequest& value) {
    json result = json::object();
    result["prompt"] = codec<std::string>::encode(value.prompt);
    if (value.mode) {
        result["mode"] = codec<std::optional<std::string>>::encode(value.mode);
    }
    if (value.model) {
        result["model"] = codec<std::optional<std::string>>::encode(value.model);
    }
    if (value.provider) {
        result["provider"] = codec<std::optional<std::string>>::encode(value.provider);
    }
    if (value.project_id) {
        result["project_id"] = codec<std::optional<std::string>>::encode(value.project_id);
    }
    if (value.thread_id) {
        result["thread_id"] = codec<std::optional<std::string>>::encode(value.thread_id);
    }
    if (value.context) {
        result["context"] = codec<std::optional<nlohmann::json>>::encode(value.context);
    }
    return result;
}
inline holder::api::AiRunCreateRequest codec<holder::api::AiRunCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AiRunCreateRequest");
    if (!value.contains("prompt")) throw std::invalid_argument("missing prompt");
    return holder::api::AiRunCreateRequest{
        codec<std::string>::decode(value.at("prompt")),
        value.contains("mode") ? codec<std::optional<std::string>>::decode(value.at("mode")) : std::nullopt,
        value.contains("model") ? codec<std::optional<std::string>>::decode(value.at("model")) : std::nullopt,
        value.contains("provider") ? codec<std::optional<std::string>>::decode(value.at("provider")) : std::nullopt,
        value.contains("project_id") ? codec<std::optional<std::string>>::decode(value.at("project_id")) : std::nullopt,
        value.contains("thread_id") ? codec<std::optional<std::string>>::decode(value.at("thread_id")) : std::nullopt,
        value.contains("context") ? codec<std::optional<nlohmann::json>>::decode(value.at("context")) : std::nullopt,
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

inline json codec<holder::api::HierarchyRepairMove>::encode(const holder::api::HierarchyRepairMove& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["parent_card_id"] = codec<std::optional<std::string>>::encode(value.parent_card_id);
    return result;
}
inline holder::api::HierarchyRepairMove codec<holder::api::HierarchyRepairMove>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected HierarchyRepairMove");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("parent_card_id")) throw std::invalid_argument("missing parent_card_id");
    return holder::api::HierarchyRepairMove{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::optional<std::string>>::decode(value.at("parent_card_id")),
    };
}

inline json codec<holder::api::HierarchyRepair>::encode(const holder::api::HierarchyRepair& value) {
    json result = json::object();
    result["applied"] = codec<bool>::encode(value.applied);
    result["moved"] = codec<std::vector<holder::api::HierarchyRepairMove>>::encode(value.moved);
    result["changed_card_count"] = codec<int>::encode(value.changed_card_count);
    result["cycle_card_ids"] = codec<std::vector<std::string>>::encode(value.cycle_card_ids);
    return result;
}
inline holder::api::HierarchyRepair codec<holder::api::HierarchyRepair>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected HierarchyRepair");
    if (!value.contains("applied")) throw std::invalid_argument("missing applied");
    if (!value.contains("moved")) throw std::invalid_argument("missing moved");
    if (!value.contains("changed_card_count")) throw std::invalid_argument("missing changed_card_count");
    if (!value.contains("cycle_card_ids")) throw std::invalid_argument("missing cycle_card_ids");
    return holder::api::HierarchyRepair{
        codec<bool>::decode(value.at("applied")),
        codec<std::vector<holder::api::HierarchyRepairMove>>::decode(value.at("moved")),
        codec<int>::decode(value.at("changed_card_count")),
        codec<std::vector<std::string>>::decode(value.at("cycle_card_ids")),
    };
}

inline json codec<holder::api::HierarchyRepairResponse>::encode(const holder::api::HierarchyRepairResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::HierarchyRepair>::encode(value.data);
    return result;
}
inline holder::api::HierarchyRepairResponse codec<holder::api::HierarchyRepairResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected HierarchyRepairResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::HierarchyRepairResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::HierarchyRepair>::decode(value.at("data")),
    };
}

inline json codec<holder::api::HierarchyRepairRequest>::encode(const holder::api::HierarchyRepairRequest& value) {
    json result = json::object();
    result["apply"] = codec<bool>::encode(value.apply);
    return result;
}
inline holder::api::HierarchyRepairRequest codec<holder::api::HierarchyRepairRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected HierarchyRepairRequest");
    return holder::api::HierarchyRepairRequest{
        value.contains("apply") ? codec<bool>::decode(value.at("apply")) : bool(false),
    };
}

inline json codec<holder::api::CardListItem>::encode(const holder::api::CardListItem& value) {
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
    if (value.child_count) {
        result["child_count"] = codec<std::optional<int>>::encode(value.child_count);
    }
    return result;
}
inline holder::api::CardListItem codec<holder::api::CardListItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardListItem");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("rel_path")) throw std::invalid_argument("missing rel_path");
    if (!value.contains("sort_key")) throw std::invalid_argument("missing sort_key");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::CardListItem{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("title")),
        codec<std::string>::decode(value.at("rel_path")),
        codec<double>::decode(value.at("sort_key")),
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
        value.contains("parent_card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("parent_card_id"))} : std::nullopt,
        value.contains("deleted_at") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("deleted_at"))} : std::nullopt,
        value.contains("child_count") ? codec<std::optional<int>>::decode(value.at("child_count")) : std::nullopt,
    };
}

inline json codec<holder::api::CardPageItem>::encode(const holder::api::CardPageItem& value) {
    json result = codec<holder::api::CardListItem>::encode(value);
    if (value.content) {
        result["content"] = codec<std::optional<std::string>>::encode(value.content);
    }
    return result;
}
inline holder::api::CardPageItem codec<holder::api::CardPageItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardPageItem");
    json base = value;
    base.erase("content");
    return holder::api::CardPageItem{
        codec<holder::api::CardListItem>::decode(base),
        value.contains("content") ? codec<std::optional<std::string>>::decode(value.at("content")) : std::nullopt,
    };
}

inline json codec<holder::api::CardPage>::encode(const holder::api::CardPage& value) {
    json result = json::object();
    result["items"] = codec<std::vector<holder::api::CardPageItem>>::encode(value.items);
    result["next_cursor"] = codec<std::optional<std::string>>::encode(value.next_cursor);
    return result;
}
inline holder::api::CardPage codec<holder::api::CardPage>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardPage");
    if (!value.contains("items")) throw std::invalid_argument("missing items");
    if (!value.contains("next_cursor")) throw std::invalid_argument("missing next_cursor");
    return holder::api::CardPage{
        codec<std::vector<holder::api::CardPageItem>>::decode(value.at("items")),
        codec<std::optional<std::string>>::decode(value.at("next_cursor")),
    };
}

inline json codec<holder::api::CardPageResponse>::encode(const holder::api::CardPageResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardPage>::encode(value.data);
    return result;
}
inline holder::api::CardPageResponse codec<holder::api::CardPageResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardPageResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardPageResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardPage>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ParentRoots>::encode(const holder::api::ParentRoots& value) {
    return codec<std::string>::encode(value.value);
}
inline holder::api::ParentRoots codec<holder::api::ParentRoots>::decode(const json& value) {
    if (value.is_string()) {
        if (value != "roots") throw std::invalid_argument("invalid value");
    }
    return holder::api::ParentRoots{codec<std::string>::decode(value)};
}

inline json codec<holder::api::ParentUuid>::encode(const holder::api::ParentUuid& value) {
    return codec<std::string>::encode(value.value);
}
inline holder::api::ParentUuid codec<holder::api::ParentUuid>::decode(const json& value) {
    if (value.is_string()) {
        static const std::regex pattern("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[47][0-9a-fA-F]{3}-[89aAbB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}$");
        if (!std::regex_search(value.get_ref<const std::string&>(), pattern))
            throw std::invalid_argument("invalid format: value");
    }
    return holder::api::ParentUuid{codec<std::string>::decode(value)};
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

inline json codec<holder::api::CardHistoryAuthor>::encode(const holder::api::CardHistoryAuthor& value) {
    json result = json::object();
    result["name"] = codec<std::string>::encode(value.name);
    result["email"] = codec<std::string>::encode(value.email);
    return result;
}
inline holder::api::CardHistoryAuthor codec<holder::api::CardHistoryAuthor>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryAuthor");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    if (!value.contains("email")) throw std::invalid_argument("missing email");
    return holder::api::CardHistoryAuthor{
        codec<std::string>::decode(value.at("name")),
        codec<std::string>::decode(value.at("email")),
    };
}

inline json codec<holder::api::ProjectHistoryAffectedObjectItemsItem>::encode(const holder::api::ProjectHistoryAffectedObjectItemsItem& value) {
    json result = json::object();
    result["path"] = codec<std::string>::encode(value.path);
    if (value.title) {
        result["title"] = codec<std::optional<std::string>>::encode(value.title);
    }
    if (value.detail) {
        result["detail"] = codec<std::optional<std::string>>::encode(value.detail);
    }
    return result;
}
inline holder::api::ProjectHistoryAffectedObjectItemsItem codec<holder::api::ProjectHistoryAffectedObjectItemsItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectHistoryAffectedObjectItemsItem");
    if (!value.contains("path")) throw std::invalid_argument("missing path");
    return holder::api::ProjectHistoryAffectedObjectItemsItem{
        codec<std::string>::decode(value.at("path")),
        value.contains("title") ? codec<std::optional<std::string>>::decode(value.at("title")) : std::nullopt,
        value.contains("detail") ? codec<std::optional<std::string>>::decode(value.at("detail")) : std::nullopt,
    };
}

inline json codec<holder::api::ProjectHistoryAffectedObject>::encode(const holder::api::ProjectHistoryAffectedObject& value) {
    json result = json::object();
    if (value.kind != "card" && value.kind != "resource" && value.kind != "location" && value.kind != "ai_data" && value.kind != "project_settings" && value.kind != "unknown") throw std::invalid_argument("invalid kind");
    result["kind"] = codec<std::string>::encode(value.kind);
    result["paths"] = codec<std::vector<std::string>>::encode(value.paths);
    if (value.items) {
        result["items"] = codec<std::optional<std::vector<holder::api::ProjectHistoryAffectedObjectItemsItem>>>::encode(value.items);
    }
    return result;
}
inline holder::api::ProjectHistoryAffectedObject codec<holder::api::ProjectHistoryAffectedObject>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectHistoryAffectedObject");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("paths")) throw std::invalid_argument("missing paths");
    if (value.at("kind") != "card" && value.at("kind") != "resource" && value.at("kind") != "location" && value.at("kind") != "ai_data" && value.at("kind") != "project_settings" && value.at("kind") != "unknown") throw std::invalid_argument("invalid kind");
    return holder::api::ProjectHistoryAffectedObject{
        codec<std::string>::decode(value.at("kind")),
        codec<std::vector<std::string>>::decode(value.at("paths")),
        value.contains("items") ? codec<std::optional<std::vector<holder::api::ProjectHistoryAffectedObjectItemsItem>>>::decode(value.at("items")) : std::nullopt,
    };
}

inline json codec<holder::api::ProjectHistoryActivity>::encode(const holder::api::ProjectHistoryActivity& value) {
    json result = json::object();
    result["oid"] = codec<std::string>::encode(value.oid);
    result["parent_oids"] = codec<std::vector<std::string>>::encode(value.parent_oids);
    result["author"] = codec<holder::api::CardHistoryAuthor>::encode(value.author);
    result["authored_at"] = codec<long long>::encode(value.authored_at);
    result["committed_at"] = codec<long long>::encode(value.committed_at);
    result["message"] = codec<std::string>::encode(value.message);
    result["affected_objects"] = codec<std::vector<holder::api::ProjectHistoryAffectedObject>>::encode(value.affected_objects);
    result["is_merge"] = codec<bool>::encode(value.is_merge);
    return result;
}
inline holder::api::ProjectHistoryActivity codec<holder::api::ProjectHistoryActivity>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectHistoryActivity");
    if (!value.contains("oid")) throw std::invalid_argument("missing oid");
    if (!value.contains("parent_oids")) throw std::invalid_argument("missing parent_oids");
    if (!value.contains("author")) throw std::invalid_argument("missing author");
    if (!value.contains("authored_at")) throw std::invalid_argument("missing authored_at");
    if (!value.contains("committed_at")) throw std::invalid_argument("missing committed_at");
    if (!value.contains("message")) throw std::invalid_argument("missing message");
    if (!value.contains("affected_objects")) throw std::invalid_argument("missing affected_objects");
    if (!value.contains("is_merge")) throw std::invalid_argument("missing is_merge");
    return holder::api::ProjectHistoryActivity{
        codec<std::string>::decode(value.at("oid")),
        codec<std::vector<std::string>>::decode(value.at("parent_oids")),
        codec<holder::api::CardHistoryAuthor>::decode(value.at("author")),
        codec<long long>::decode(value.at("authored_at")),
        codec<long long>::decode(value.at("committed_at")),
        codec<std::string>::decode(value.at("message")),
        codec<std::vector<holder::api::ProjectHistoryAffectedObject>>::decode(value.at("affected_objects")),
        codec<bool>::decode(value.at("is_merge")),
    };
}

inline json codec<holder::api::ProjectHistoryPageData>::encode(const holder::api::ProjectHistoryPageData& value) {
    json result = json::object();
    result["head_oid"] = codec<std::optional<std::string>>::encode(value.head_oid);
    result["activities"] = codec<std::vector<holder::api::ProjectHistoryActivity>>::encode(value.activities);
    result["next_cursor"] = codec<std::optional<std::string>>::encode(value.next_cursor);
    result["scan_limited"] = codec<bool>::encode(value.scan_limited);
    return result;
}
inline holder::api::ProjectHistoryPageData codec<holder::api::ProjectHistoryPageData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectHistoryPageData");
    if (!value.contains("head_oid")) throw std::invalid_argument("missing head_oid");
    if (!value.contains("activities")) throw std::invalid_argument("missing activities");
    if (!value.contains("next_cursor")) throw std::invalid_argument("missing next_cursor");
    if (!value.contains("scan_limited")) throw std::invalid_argument("missing scan_limited");
    return holder::api::ProjectHistoryPageData{
        codec<std::optional<std::string>>::decode(value.at("head_oid")),
        codec<std::vector<holder::api::ProjectHistoryActivity>>::decode(value.at("activities")),
        codec<std::optional<std::string>>::decode(value.at("next_cursor")),
        codec<bool>::decode(value.at("scan_limited")),
    };
}

inline json codec<holder::api::ProjectHistoryPageResponse>::encode(const holder::api::ProjectHistoryPageResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectHistoryPageData>::encode(value.data);
    return result;
}
inline holder::api::ProjectHistoryPageResponse codec<holder::api::ProjectHistoryPageResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectHistoryPageResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectHistoryPageResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectHistoryPageData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardHistoryEntrySavesItem>::encode(const holder::api::CardHistoryEntrySavesItem& value) {
    json result = json::object();
    result["oid"] = codec<std::string>::encode(value.oid);
    result["parent_oids"] = codec<std::vector<std::string>>::encode(value.parent_oids);
    result["authored_at"] = codec<long long>::encode(value.authored_at);
    result["committed_at"] = codec<long long>::encode(value.committed_at);
    result["message"] = codec<std::string>::encode(value.message);
    return result;
}
inline holder::api::CardHistoryEntrySavesItem codec<holder::api::CardHistoryEntrySavesItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryEntrySavesItem");
    if (!value.contains("oid")) throw std::invalid_argument("missing oid");
    if (!value.contains("parent_oids")) throw std::invalid_argument("missing parent_oids");
    if (!value.contains("authored_at")) throw std::invalid_argument("missing authored_at");
    if (!value.contains("committed_at")) throw std::invalid_argument("missing committed_at");
    if (!value.contains("message")) throw std::invalid_argument("missing message");
    return holder::api::CardHistoryEntrySavesItem{
        codec<std::string>::decode(value.at("oid")),
        codec<std::vector<std::string>>::decode(value.at("parent_oids")),
        codec<long long>::decode(value.at("authored_at")),
        codec<long long>::decode(value.at("committed_at")),
        codec<std::string>::decode(value.at("message")),
    };
}

inline json codec<holder::api::CardHistoryEntry>::encode(const holder::api::CardHistoryEntry& value) {
    json result = json::object();
    result["first_oid"] = codec<std::string>::encode(value.first_oid);
    result["last_oid"] = codec<std::string>::encode(value.last_oid);
    result["parent_oids"] = codec<std::vector<std::string>>::encode(value.parent_oids);
    result["visible_parent_oids"] = codec<std::vector<std::string>>::encode(value.visible_parent_oids);
    result["author"] = codec<holder::api::CardHistoryAuthor>::encode(value.author);
    result["started_at"] = codec<long long>::encode(value.started_at);
    result["ended_at"] = codec<long long>::encode(value.ended_at);
    if (value.kind != "created" && value.kind != "updated" && value.kind != "moved" && value.kind != "links" && value.kind != "milestones" && value.kind != "deleted" && value.kind != "restored" && value.kind != "permanently_deleted" && value.kind != "merged") throw std::invalid_argument("invalid kind");
    result["kind"] = codec<std::string>::encode(value.kind);
    result["summary"] = codec<std::string>::encode(value.summary);
    result["commit_count"] = codec<int>::encode(value.commit_count);
    result["is_merge"] = codec<bool>::encode(value.is_merge);
    result["saves"] = codec<std::vector<holder::api::CardHistoryEntrySavesItem>>::encode(value.saves);
    return result;
}
inline holder::api::CardHistoryEntry codec<holder::api::CardHistoryEntry>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryEntry");
    if (!value.contains("first_oid")) throw std::invalid_argument("missing first_oid");
    if (!value.contains("last_oid")) throw std::invalid_argument("missing last_oid");
    if (!value.contains("parent_oids")) throw std::invalid_argument("missing parent_oids");
    if (!value.contains("visible_parent_oids")) throw std::invalid_argument("missing visible_parent_oids");
    if (!value.contains("author")) throw std::invalid_argument("missing author");
    if (!value.contains("started_at")) throw std::invalid_argument("missing started_at");
    if (!value.contains("ended_at")) throw std::invalid_argument("missing ended_at");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("summary")) throw std::invalid_argument("missing summary");
    if (!value.contains("commit_count")) throw std::invalid_argument("missing commit_count");
    if (!value.contains("is_merge")) throw std::invalid_argument("missing is_merge");
    if (!value.contains("saves")) throw std::invalid_argument("missing saves");
    if (value.at("kind") != "created" && value.at("kind") != "updated" && value.at("kind") != "moved" && value.at("kind") != "links" && value.at("kind") != "milestones" && value.at("kind") != "deleted" && value.at("kind") != "restored" && value.at("kind") != "permanently_deleted" && value.at("kind") != "merged") throw std::invalid_argument("invalid kind");
    return holder::api::CardHistoryEntry{
        codec<std::string>::decode(value.at("first_oid")),
        codec<std::string>::decode(value.at("last_oid")),
        codec<std::vector<std::string>>::decode(value.at("parent_oids")),
        codec<std::vector<std::string>>::decode(value.at("visible_parent_oids")),
        codec<holder::api::CardHistoryAuthor>::decode(value.at("author")),
        codec<long long>::decode(value.at("started_at")),
        codec<long long>::decode(value.at("ended_at")),
        codec<std::string>::decode(value.at("kind")),
        codec<std::string>::decode(value.at("summary")),
        codec<int>::decode(value.at("commit_count")),
        codec<bool>::decode(value.at("is_merge")),
        codec<std::vector<holder::api::CardHistoryEntrySavesItem>>::decode(value.at("saves")),
    };
}

inline json codec<holder::api::CardHistoryPageData>::encode(const holder::api::CardHistoryPageData& value) {
    json result = json::object();
    result["head_oid"] = codec<std::optional<std::string>>::encode(value.head_oid);
    result["entries"] = codec<std::vector<holder::api::CardHistoryEntry>>::encode(value.entries);
    result["next_cursor"] = codec<std::optional<std::string>>::encode(value.next_cursor);
    result["scan_limited"] = codec<bool>::encode(value.scan_limited);
    return result;
}
inline holder::api::CardHistoryPageData codec<holder::api::CardHistoryPageData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryPageData");
    if (!value.contains("head_oid")) throw std::invalid_argument("missing head_oid");
    if (!value.contains("entries")) throw std::invalid_argument("missing entries");
    if (!value.contains("next_cursor")) throw std::invalid_argument("missing next_cursor");
    if (!value.contains("scan_limited")) throw std::invalid_argument("missing scan_limited");
    return holder::api::CardHistoryPageData{
        codec<std::optional<std::string>>::decode(value.at("head_oid")),
        codec<std::vector<holder::api::CardHistoryEntry>>::decode(value.at("entries")),
        codec<std::optional<std::string>>::decode(value.at("next_cursor")),
        codec<bool>::decode(value.at("scan_limited")),
    };
}

inline json codec<holder::api::CardHistoryPageResponse>::encode(const holder::api::CardHistoryPageResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardHistoryPageData>::encode(value.data);
    return result;
}
inline holder::api::CardHistoryPageResponse codec<holder::api::CardHistoryPageResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryPageResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardHistoryPageResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardHistoryPageData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardHistoryVersion>::encode(const holder::api::CardHistoryVersion& value) {
    json result = json::object();
    result["exists"] = codec<bool>::encode(value.exists);
    result["oid"] = codec<std::string>::encode(value.oid);
    result["title"] = codec<std::string>::encode(value.title);
    result["body"] = codec<std::string>::encode(value.body);
    return result;
}
inline holder::api::CardHistoryVersion codec<holder::api::CardHistoryVersion>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryVersion");
    if (!value.contains("exists")) throw std::invalid_argument("missing exists");
    if (!value.contains("oid")) throw std::invalid_argument("missing oid");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("body")) throw std::invalid_argument("missing body");
    return holder::api::CardHistoryVersion{
        codec<bool>::decode(value.at("exists")),
        codec<std::string>::decode(value.at("oid")),
        codec<std::string>::decode(value.at("title")),
        codec<std::string>::decode(value.at("body")),
    };
}

inline json codec<holder::api::CardHistoryDiffLine>::encode(const holder::api::CardHistoryDiffLine& value) {
    json result = json::object();
    if (value.origin != " " && value.origin != "+" && value.origin != "-") throw std::invalid_argument("invalid origin");
    result["origin"] = codec<std::string>::encode(value.origin);
    result["text"] = codec<std::string>::encode(value.text);
    result["old_line"] = codec<std::optional<long long>>::encode(value.old_line);
    result["new_line"] = codec<std::optional<long long>>::encode(value.new_line);
    return result;
}
inline holder::api::CardHistoryDiffLine codec<holder::api::CardHistoryDiffLine>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryDiffLine");
    if (!value.contains("origin")) throw std::invalid_argument("missing origin");
    if (!value.contains("text")) throw std::invalid_argument("missing text");
    if (!value.contains("old_line")) throw std::invalid_argument("missing old_line");
    if (!value.contains("new_line")) throw std::invalid_argument("missing new_line");
    if (value.at("origin") != " " && value.at("origin") != "+" && value.at("origin") != "-") throw std::invalid_argument("invalid origin");
    return holder::api::CardHistoryDiffLine{
        codec<std::string>::decode(value.at("origin")),
        codec<std::string>::decode(value.at("text")),
        codec<std::optional<long long>>::decode(value.at("old_line")),
        codec<std::optional<long long>>::decode(value.at("new_line")),
    };
}

inline json codec<holder::api::CardHistoryComparisonData>::encode(const holder::api::CardHistoryComparisonData& value) {
    json result = json::object();
    result["from"] = codec<holder::api::CardHistoryVersion>::encode(value.from);
    result["to"] = codec<holder::api::CardHistoryVersion>::encode(value.to);
    result["summary"] = codec<std::string>::encode(value.summary);
    result["lines"] = codec<std::vector<holder::api::CardHistoryDiffLine>>::encode(value.lines);
    result["truncated"] = codec<bool>::encode(value.truncated);
    return result;
}
inline holder::api::CardHistoryComparisonData codec<holder::api::CardHistoryComparisonData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryComparisonData");
    if (!value.contains("from")) throw std::invalid_argument("missing from");
    if (!value.contains("to")) throw std::invalid_argument("missing to");
    if (!value.contains("summary")) throw std::invalid_argument("missing summary");
    if (!value.contains("lines")) throw std::invalid_argument("missing lines");
    if (!value.contains("truncated")) throw std::invalid_argument("missing truncated");
    return holder::api::CardHistoryComparisonData{
        codec<holder::api::CardHistoryVersion>::decode(value.at("from")),
        codec<holder::api::CardHistoryVersion>::decode(value.at("to")),
        codec<std::string>::decode(value.at("summary")),
        codec<std::vector<holder::api::CardHistoryDiffLine>>::decode(value.at("lines")),
        codec<bool>::decode(value.at("truncated")),
    };
}

inline json codec<holder::api::CardHistoryComparisonResponse>::encode(const holder::api::CardHistoryComparisonResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardHistoryComparisonData>::encode(value.data);
    return result;
}
inline holder::api::CardHistoryComparisonResponse codec<holder::api::CardHistoryComparisonResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryComparisonResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardHistoryComparisonResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardHistoryComparisonData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::RevisionReference>::encode(const holder::api::RevisionReference& value) {
    return codec<std::string>::encode(value.value);
}
inline holder::api::RevisionReference codec<holder::api::RevisionReference>::decode(const json& value) {
    if (value.is_string()) {
        const auto length = value.get_ref<const std::string&>().size();
        if (length < 8 || length > 40)
            throw std::invalid_argument("invalid length: value");
        static const std::regex pattern("^[0-9A-Fa-f]{8,40}$");
        if (!std::regex_search(value.get_ref<const std::string&>(), pattern))
            throw std::invalid_argument("invalid format: value");
    }
    return holder::api::RevisionReference{codec<std::string>::decode(value)};
}

inline json codec<holder::api::CardHistorySnapshotResponseData>::encode(const holder::api::CardHistorySnapshotResponseData& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["snapshot"] = codec<holder::api::CardHistoryVersion>::encode(value.snapshot);
    return result;
}
inline holder::api::CardHistorySnapshotResponseData codec<holder::api::CardHistorySnapshotResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistorySnapshotResponseData");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("snapshot")) throw std::invalid_argument("missing snapshot");
    return holder::api::CardHistorySnapshotResponseData{
        codec<std::string>::decode(value.at("card_id")),
        codec<holder::api::CardHistoryVersion>::decode(value.at("snapshot")),
    };
}

inline json codec<holder::api::CardHistorySnapshotResponse>::encode(const holder::api::CardHistorySnapshotResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardHistorySnapshotResponseData>::encode(value.data);
    return result;
}
inline holder::api::CardHistorySnapshotResponse codec<holder::api::CardHistorySnapshotResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistorySnapshotResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardHistorySnapshotResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardHistorySnapshotResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardHistoryRestoreResponseData>::encode(const holder::api::CardHistoryRestoreResponseData& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["restored_from_oid"] = codec<std::string>::encode(value.restored_from_oid);
    result["result_oid"] = codec<std::string>::encode(value.result_oid);
    result["title"] = codec<std::string>::encode(value.title);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    result["deleted_at"] = codec<std::optional<long long>>::encode(value.deleted_at);
    return result;
}
inline holder::api::CardHistoryRestoreResponseData codec<holder::api::CardHistoryRestoreResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryRestoreResponseData");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("restored_from_oid")) throw std::invalid_argument("missing restored_from_oid");
    if (!value.contains("result_oid")) throw std::invalid_argument("missing result_oid");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    if (!value.contains("deleted_at")) throw std::invalid_argument("missing deleted_at");
    return holder::api::CardHistoryRestoreResponseData{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("restored_from_oid")),
        codec<std::string>::decode(value.at("result_oid")),
        codec<std::string>::decode(value.at("title")),
        codec<long long>::decode(value.at("updated_at")),
        codec<std::optional<long long>>::decode(value.at("deleted_at")),
    };
}

inline json codec<holder::api::CardHistoryRestoreResponse>::encode(const holder::api::CardHistoryRestoreResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardHistoryRestoreResponseData>::encode(value.data);
    return result;
}
inline holder::api::CardHistoryRestoreResponse codec<holder::api::CardHistoryRestoreResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardHistoryRestoreResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardHistoryRestoreResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardHistoryRestoreResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectGitTestRemoteResponseData>::encode(const holder::api::ProjectGitTestRemoteResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    if (value.remote_url) {
        result["remote_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.remote_url);
    }
    result["branch"] = codec<std::string>::encode(value.branch);
    if (value.status != "reachable" && value.status != "auth_failed" && value.status != "not_found" && value.status != "network_error" && value.status != "invalid_remote_url" && value.status != "remote_unset" && value.status != "unknown_error") throw std::invalid_argument("invalid status");
    result["status"] = codec<std::string>::encode(value.status);
    result["remote_has_head"] = codec<bool>::encode(value.remote_has_head);
    if (value.error_code) {
        result["error_code"] = codec<std::optional<std::optional<std::string>>>::encode(value.error_code);
    }
    if (value.error_message) {
        result["error_message"] = codec<std::optional<std::optional<std::string>>>::encode(value.error_message);
    }
    return result;
}
inline holder::api::ProjectGitTestRemoteResponseData codec<holder::api::ProjectGitTestRemoteResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitTestRemoteResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("branch")) throw std::invalid_argument("missing branch");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    if (!value.contains("remote_has_head")) throw std::invalid_argument("missing remote_has_head");
    if (value.at("status") != "reachable" && value.at("status") != "auth_failed" && value.at("status") != "not_found" && value.at("status") != "network_error" && value.at("status") != "invalid_remote_url" && value.at("status") != "remote_unset" && value.at("status") != "unknown_error") throw std::invalid_argument("invalid status");
    return holder::api::ProjectGitTestRemoteResponseData{
        codec<std::string>::decode(value.at("project_id")),
        value.contains("remote_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("remote_url"))} : std::nullopt,
        codec<std::string>::decode(value.at("branch")),
        codec<std::string>::decode(value.at("status")),
        codec<bool>::decode(value.at("remote_has_head")),
        value.contains("error_code") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("error_code"))} : std::nullopt,
        value.contains("error_message") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("error_message"))} : std::nullopt,
    };
}

inline json codec<holder::api::ProjectGitTestRemoteResponse>::encode(const holder::api::ProjectGitTestRemoteResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectGitTestRemoteResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectGitTestRemoteResponse codec<holder::api::ProjectGitTestRemoteResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitTestRemoteResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectGitTestRemoteResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectGitTestRemoteResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectGitTestRemoteRequest>::encode(const holder::api::ProjectGitTestRemoteRequest& value) {
    json result = json::object();
    if (value.remote_url) {
        result["remote_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.remote_url);
    }
    if (value.branch) {
        result["branch"] = codec<std::optional<std::optional<std::string>>>::encode(value.branch);
    }
    return result;
}
inline holder::api::ProjectGitTestRemoteRequest codec<holder::api::ProjectGitTestRemoteRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitTestRemoteRequest");
    return holder::api::ProjectGitTestRemoteRequest{
        value.contains("remote_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("remote_url"))} : std::nullopt,
        value.contains("branch") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("branch"))} : std::nullopt,
    };
}

inline json codec<holder::api::ProjectGitPushResponseData>::encode(const holder::api::ProjectGitPushResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    if (value.remote_url) {
        result["remote_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.remote_url);
    }
    result["branch"] = codec<std::string>::encode(value.branch);
    if (value.status != "pushed" && value.status != "up_to_date" && value.status != "auth_failed" && value.status != "not_found" && value.status != "network_error" && value.status != "non_fast_forward" && value.status != "remote_unset" && value.status != "unknown_error") throw std::invalid_argument("invalid status");
    result["status"] = codec<std::string>::encode(value.status);
    result["ahead_count"] = codec<int>::encode(value.ahead_count);
    result["behind_count"] = codec<int>::encode(value.behind_count);
    if (value.local_head_commit) {
        result["local_head_commit"] = codec<std::optional<std::optional<std::string>>>::encode(value.local_head_commit);
    }
    if (value.error_code) {
        result["error_code"] = codec<std::optional<std::optional<std::string>>>::encode(value.error_code);
    }
    if (value.error_message) {
        result["error_message"] = codec<std::optional<std::optional<std::string>>>::encode(value.error_message);
    }
    if (value.next_action) {
        result["next_action"] = codec<std::optional<std::optional<std::string>>>::encode(value.next_action);
    }
    return result;
}
inline holder::api::ProjectGitPushResponseData codec<holder::api::ProjectGitPushResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitPushResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("branch")) throw std::invalid_argument("missing branch");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    if (!value.contains("ahead_count")) throw std::invalid_argument("missing ahead_count");
    if (!value.contains("behind_count")) throw std::invalid_argument("missing behind_count");
    if (value.at("status") != "pushed" && value.at("status") != "up_to_date" && value.at("status") != "auth_failed" && value.at("status") != "not_found" && value.at("status") != "network_error" && value.at("status") != "non_fast_forward" && value.at("status") != "remote_unset" && value.at("status") != "unknown_error") throw std::invalid_argument("invalid status");
    return holder::api::ProjectGitPushResponseData{
        codec<std::string>::decode(value.at("project_id")),
        value.contains("remote_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("remote_url"))} : std::nullopt,
        codec<std::string>::decode(value.at("branch")),
        codec<std::string>::decode(value.at("status")),
        codec<int>::decode(value.at("ahead_count")),
        codec<int>::decode(value.at("behind_count")),
        value.contains("local_head_commit") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("local_head_commit"))} : std::nullopt,
        value.contains("error_code") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("error_code"))} : std::nullopt,
        value.contains("error_message") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("error_message"))} : std::nullopt,
        value.contains("next_action") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("next_action"))} : std::nullopt,
    };
}

inline json codec<holder::api::ProjectGitPushResponse>::encode(const holder::api::ProjectGitPushResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectGitPushResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectGitPushResponse codec<holder::api::ProjectGitPushResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitPushResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectGitPushResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectGitPushResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectGitPushRequest>::encode(const holder::api::ProjectGitPushRequest& value) {
    json result = json::object();
    if (value.branch) {
        result["branch"] = codec<std::optional<std::optional<std::string>>>::encode(value.branch);
    }
    if (value.set_upstream) {
        result["set_upstream"] = codec<std::optional<std::optional<bool>>>::encode(value.set_upstream);
    }
    return result;
}
inline holder::api::ProjectGitPushRequest codec<holder::api::ProjectGitPushRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitPushRequest");
    return holder::api::ProjectGitPushRequest{
        value.contains("branch") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("branch"))} : std::nullopt,
        value.contains("set_upstream") ? std::optional<std::optional<bool>>{std::in_place, codec<std::optional<bool>>::decode(value.at("set_upstream"))} : std::nullopt,
    };
}

inline json codec<holder::api::ProjectGitSyncOperationResponseDataPull>::encode(const holder::api::ProjectGitSyncOperationResponseDataPull& value) {
    json result = json::object();
    result["attempted"] = codec<bool>::encode(value.attempted);
    if (value.status != "not_attempted" && value.status != "succeeded" && value.status != "remote_unset" && value.status != "failed") throw std::invalid_argument("invalid status");
    result["status"] = codec<std::string>::encode(value.status);
    result["conflicts_resolved"] = codec<int>::encode(value.conflicts_resolved);
    result["error_message"] = codec<std::optional<std::string>>::encode(value.error_message);
    return result;
}
inline holder::api::ProjectGitSyncOperationResponseDataPull codec<holder::api::ProjectGitSyncOperationResponseDataPull>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitSyncOperationResponseDataPull");
    if (!value.contains("attempted")) throw std::invalid_argument("missing attempted");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    if (!value.contains("conflicts_resolved")) throw std::invalid_argument("missing conflicts_resolved");
    if (!value.contains("error_message")) throw std::invalid_argument("missing error_message");
    if (value.at("status") != "not_attempted" && value.at("status") != "succeeded" && value.at("status") != "remote_unset" && value.at("status") != "failed") throw std::invalid_argument("invalid status");
    return holder::api::ProjectGitSyncOperationResponseDataPull{
        codec<bool>::decode(value.at("attempted")),
        codec<std::string>::decode(value.at("status")),
        codec<int>::decode(value.at("conflicts_resolved")),
        codec<std::optional<std::string>>::decode(value.at("error_message")),
    };
}

inline json codec<holder::api::ProjectGitSyncOperationResponseDataPushStatus>::encode(const holder::api::ProjectGitSyncOperationResponseDataPushStatus& value) {
    switch (value) {
    case holder::api::ProjectGitSyncOperationResponseDataPushStatus::pushed: return "pushed";
    case holder::api::ProjectGitSyncOperationResponseDataPushStatus::up_to_date: return "up_to_date";
    case holder::api::ProjectGitSyncOperationResponseDataPushStatus::auth_failed: return "auth_failed";
    case holder::api::ProjectGitSyncOperationResponseDataPushStatus::not_found: return "not_found";
    case holder::api::ProjectGitSyncOperationResponseDataPushStatus::network_error: return "network_error";
    case holder::api::ProjectGitSyncOperationResponseDataPushStatus::non_fast_forward: return "non_fast_forward";
    case holder::api::ProjectGitSyncOperationResponseDataPushStatus::remote_unset: return "remote_unset";
    case holder::api::ProjectGitSyncOperationResponseDataPushStatus::unknown_error: return "unknown_error";
    }
    throw std::invalid_argument("invalid ProjectGitSyncOperationResponseDataPushStatus");
}
inline holder::api::ProjectGitSyncOperationResponseDataPushStatus codec<holder::api::ProjectGitSyncOperationResponseDataPushStatus>::decode(const json& value) {
    if (!value.is_string()) throw std::invalid_argument("expected ProjectGitSyncOperationResponseDataPushStatus");
    if (value == "pushed") return holder::api::ProjectGitSyncOperationResponseDataPushStatus::pushed;
    if (value == "up_to_date") return holder::api::ProjectGitSyncOperationResponseDataPushStatus::up_to_date;
    if (value == "auth_failed") return holder::api::ProjectGitSyncOperationResponseDataPushStatus::auth_failed;
    if (value == "not_found") return holder::api::ProjectGitSyncOperationResponseDataPushStatus::not_found;
    if (value == "network_error") return holder::api::ProjectGitSyncOperationResponseDataPushStatus::network_error;
    if (value == "non_fast_forward") return holder::api::ProjectGitSyncOperationResponseDataPushStatus::non_fast_forward;
    if (value == "remote_unset") return holder::api::ProjectGitSyncOperationResponseDataPushStatus::remote_unset;
    if (value == "unknown_error") return holder::api::ProjectGitSyncOperationResponseDataPushStatus::unknown_error;
    throw std::invalid_argument("unknown ProjectGitSyncOperationResponseDataPushStatus");
}

inline json codec<holder::api::ProjectGitSyncOperationResponseDataPush>::encode(const holder::api::ProjectGitSyncOperationResponseDataPush& value) {
    json result = json::object();
    result["attempted"] = codec<bool>::encode(value.attempted);
    result["status"] = codec<std::optional<holder::api::ProjectGitSyncOperationResponseDataPushStatus>>::encode(value.status);
    result["ahead_count"] = codec<int>::encode(value.ahead_count);
    result["behind_count"] = codec<int>::encode(value.behind_count);
    result["local_head_commit"] = codec<std::optional<std::string>>::encode(value.local_head_commit);
    result["error_message"] = codec<std::optional<std::string>>::encode(value.error_message);
    return result;
}
inline holder::api::ProjectGitSyncOperationResponseDataPush codec<holder::api::ProjectGitSyncOperationResponseDataPush>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitSyncOperationResponseDataPush");
    if (!value.contains("attempted")) throw std::invalid_argument("missing attempted");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    if (!value.contains("ahead_count")) throw std::invalid_argument("missing ahead_count");
    if (!value.contains("behind_count")) throw std::invalid_argument("missing behind_count");
    if (!value.contains("local_head_commit")) throw std::invalid_argument("missing local_head_commit");
    if (!value.contains("error_message")) throw std::invalid_argument("missing error_message");
    return holder::api::ProjectGitSyncOperationResponseDataPush{
        codec<bool>::decode(value.at("attempted")),
        codec<std::optional<holder::api::ProjectGitSyncOperationResponseDataPushStatus>>::decode(value.at("status")),
        codec<int>::decode(value.at("ahead_count")),
        codec<int>::decode(value.at("behind_count")),
        codec<std::optional<std::string>>::decode(value.at("local_head_commit")),
        codec<std::optional<std::string>>::decode(value.at("error_message")),
    };
}

inline json codec<holder::api::ProjectGitSyncOperationResponseData>::encode(const holder::api::ProjectGitSyncOperationResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    if (value.remote_url) {
        result["remote_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.remote_url);
    }
    result["branch"] = codec<std::string>::encode(value.branch);
    if (value.status != "succeeded" && value.status != "failed") throw std::invalid_argument("invalid status");
    result["status"] = codec<std::string>::encode(value.status);
    result["error_code"] = codec<std::optional<std::string>>::encode(value.error_code);
    result["error_message"] = codec<std::optional<std::string>>::encode(value.error_message);
    result["pull"] = codec<holder::api::ProjectGitSyncOperationResponseDataPull>::encode(value.pull);
    result["push"] = codec<holder::api::ProjectGitSyncOperationResponseDataPush>::encode(value.push);
    return result;
}
inline holder::api::ProjectGitSyncOperationResponseData codec<holder::api::ProjectGitSyncOperationResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitSyncOperationResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("branch")) throw std::invalid_argument("missing branch");
    if (!value.contains("status")) throw std::invalid_argument("missing status");
    if (!value.contains("error_code")) throw std::invalid_argument("missing error_code");
    if (!value.contains("error_message")) throw std::invalid_argument("missing error_message");
    if (!value.contains("pull")) throw std::invalid_argument("missing pull");
    if (!value.contains("push")) throw std::invalid_argument("missing push");
    if (value.at("status") != "succeeded" && value.at("status") != "failed") throw std::invalid_argument("invalid status");
    return holder::api::ProjectGitSyncOperationResponseData{
        codec<std::string>::decode(value.at("project_id")),
        value.contains("remote_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("remote_url"))} : std::nullopt,
        codec<std::string>::decode(value.at("branch")),
        codec<std::string>::decode(value.at("status")),
        codec<std::optional<std::string>>::decode(value.at("error_code")),
        codec<std::optional<std::string>>::decode(value.at("error_message")),
        codec<holder::api::ProjectGitSyncOperationResponseDataPull>::decode(value.at("pull")),
        codec<holder::api::ProjectGitSyncOperationResponseDataPush>::decode(value.at("push")),
    };
}

inline json codec<holder::api::ProjectGitSyncOperationResponse>::encode(const holder::api::ProjectGitSyncOperationResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectGitSyncOperationResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectGitSyncOperationResponse codec<holder::api::ProjectGitSyncOperationResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitSyncOperationResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectGitSyncOperationResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectGitSyncOperationResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectGitSyncRequest>::encode(const holder::api::ProjectGitSyncRequest& value) {
    json result = json::object();
    if (value.branch) {
        result["branch"] = codec<std::optional<std::optional<std::string>>>::encode(value.branch);
    }
    if (value.set_upstream) {
        result["set_upstream"] = codec<std::optional<std::optional<bool>>>::encode(value.set_upstream);
    }
    return result;
}
inline holder::api::ProjectGitSyncRequest codec<holder::api::ProjectGitSyncRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectGitSyncRequest");
    return holder::api::ProjectGitSyncRequest{
        value.contains("branch") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("branch"))} : std::nullopt,
        value.contains("set_upstream") ? std::optional<std::optional<bool>>{std::in_place, codec<std::optional<bool>>::decode(value.at("set_upstream"))} : std::nullopt,
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

inline json codec<holder::api::MilestoneDateValue>::encode(const holder::api::MilestoneDateValue& value) {
    json result = json::object();
    result["role"] = codec<std::string>::encode(value.role);
    if (value.date) {
        result["date"] = codec<std::optional<std::string>>::encode(value.date);
    }
    if (value.datetime) {
        result["datetime"] = codec<std::optional<std::string>>::encode(value.datetime);
    }
    return result;
}
inline holder::api::MilestoneDateValue codec<holder::api::MilestoneDateValue>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneDateValue");
    if (!value.contains("role")) throw std::invalid_argument("missing role");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "role" && it.key() != "date" && it.key() != "datetime") throw std::invalid_argument("unexpected MilestoneDateValue field");
    if (value.contains("role") && value.at("role").is_string()) {
        const auto length = value.at("role").get_ref<const std::string&>().size();
        if (length < 1 || length > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid length: role");
    }
    return holder::api::MilestoneDateValue{
        codec<std::string>::decode(value.at("role")),
        value.contains("date") ? codec<std::optional<std::string>>::decode(value.at("date")) : std::nullopt,
        value.contains("datetime") ? codec<std::optional<std::string>>::decode(value.at("datetime")) : std::nullopt,
    };
}

inline json codec<holder::api::MilestoneWithDates>::encode(const holder::api::MilestoneWithDates& value) {
    json result = json::object();
    result["milestone_id"] = codec<std::string>::encode(value.milestone_id);
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["dates"] = codec<std::vector<holder::api::MilestoneDateValue>>::encode(value.dates);
    result["kind"] = codec<std::optional<std::string>>::encode(value.kind);
    result["description"] = codec<std::optional<std::string>>::encode(value.description);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::MilestoneWithDates codec<holder::api::MilestoneWithDates>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneWithDates");
    if (!value.contains("milestone_id")) throw std::invalid_argument("missing milestone_id");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("dates")) throw std::invalid_argument("missing dates");
    if (!value.contains("kind")) throw std::invalid_argument("missing kind");
    if (!value.contains("description")) throw std::invalid_argument("missing description");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::MilestoneWithDates{
        codec<std::string>::decode(value.at("milestone_id")),
        codec<std::string>::decode(value.at("card_id")),
        codec<std::vector<holder::api::MilestoneDateValue>>::decode(value.at("dates")),
        codec<std::optional<std::string>>::decode(value.at("kind")),
        codec<std::optional<std::string>>::decode(value.at("description")),
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::MilestoneWithDatesListResponse>::encode(const holder::api::MilestoneWithDatesListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::MilestoneWithDates>>::encode(value.data);
    return result;
}
inline holder::api::MilestoneWithDatesListResponse codec<holder::api::MilestoneWithDatesListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneWithDatesListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::MilestoneWithDatesListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::MilestoneWithDates>>::decode(value.at("data")),
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

inline json codec<holder::api::MilestoneWithDatesResponse>::encode(const holder::api::MilestoneWithDatesResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::MilestoneWithDates>::encode(value.data);
    return result;
}
inline holder::api::MilestoneWithDatesResponse codec<holder::api::MilestoneWithDatesResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneWithDatesResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::MilestoneWithDatesResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::MilestoneWithDates>::decode(value.at("data")),
    };
}

inline json codec<holder::api::MilestoneWithDatesCreateRequest>::encode(const holder::api::MilestoneWithDatesCreateRequest& value) {
    json result = json::object();
    result["dates"] = codec<std::vector<holder::api::MilestoneDateValue>>::encode(value.dates);
    if (value.kind) {
        result["kind"] = codec<std::optional<std::optional<std::string>>>::encode(value.kind);
    }
    if (value.description) {
        result["description"] = codec<std::optional<std::optional<std::string>>>::encode(value.description);
    }
    return result;
}
inline holder::api::MilestoneWithDatesCreateRequest codec<holder::api::MilestoneWithDatesCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneWithDatesCreateRequest");
    if (!value.contains("dates")) throw std::invalid_argument("missing dates");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "dates" && it.key() != "kind" && it.key() != "description") throw std::invalid_argument("unexpected MilestoneWithDatesCreateRequest field");
    return holder::api::MilestoneWithDatesCreateRequest{
        codec<std::vector<holder::api::MilestoneDateValue>>::decode(value.at("dates")),
        value.contains("kind") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("kind"))} : std::nullopt,
        value.contains("description") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("description"))} : std::nullopt,
    };
}

inline json codec<holder::api::MilestoneWithDatesUpdateRequest>::encode(const holder::api::MilestoneWithDatesUpdateRequest& value) {
    json result = json::object();
    if (value.dates) {
        result["dates"] = codec<std::optional<std::vector<holder::api::MilestoneDateValue>>>::encode(value.dates);
    }
    if (value.kind) {
        result["kind"] = codec<std::optional<std::optional<std::string>>>::encode(value.kind);
    }
    if (value.description) {
        result["description"] = codec<std::optional<std::optional<std::string>>>::encode(value.description);
    }
    return result;
}
inline holder::api::MilestoneWithDatesUpdateRequest codec<holder::api::MilestoneWithDatesUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneWithDatesUpdateRequest");
    if (value.size() < 1) throw std::invalid_argument("too few MilestoneWithDatesUpdateRequest fields");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "dates" && it.key() != "kind" && it.key() != "description") throw std::invalid_argument("unexpected MilestoneWithDatesUpdateRequest field");
    return holder::api::MilestoneWithDatesUpdateRequest{
        value.contains("dates") ? codec<std::optional<std::vector<holder::api::MilestoneDateValue>>>::decode(value.at("dates")) : std::nullopt,
        value.contains("kind") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("kind"))} : std::nullopt,
        value.contains("description") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("description"))} : std::nullopt,
    };
}

inline json codec<holder::api::MilestoneRemoveResult>::encode(const holder::api::MilestoneRemoveResult& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["milestone_id"] = codec<std::string>::encode(value.milestone_id);
    result["removed"] = codec<bool>::encode(value.removed);
    return result;
}
inline holder::api::MilestoneRemoveResult codec<holder::api::MilestoneRemoveResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneRemoveResult");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("milestone_id")) throw std::invalid_argument("missing milestone_id");
    if (!value.contains("removed")) throw std::invalid_argument("missing removed");
    return holder::api::MilestoneRemoveResult{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("milestone_id")),
        codec<bool>::decode(value.at("removed")),
    };
}

inline json codec<holder::api::MilestoneRemoveResponse>::encode(const holder::api::MilestoneRemoveResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::MilestoneRemoveResult>::encode(value.data);
    return result;
}
inline holder::api::MilestoneRemoveResponse codec<holder::api::MilestoneRemoveResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneRemoveResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::MilestoneRemoveResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::MilestoneRemoveResult>::decode(value.at("data")),
    };
}

inline json codec<holder::api::Milestone>::encode(const holder::api::Milestone& value) {
    json result = json::object();
    result["milestone_id"] = codec<std::string>::encode(value.milestone_id);
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["start_at"] = codec<long long>::encode(value.start_at);
    if (value.end_at) {
        result["end_at"] = codec<std::optional<std::optional<long long>>>::encode(value.end_at);
    }
    result["all_day"] = codec<bool>::encode(value.all_day);
    if (value.kind) {
        result["kind"] = codec<std::optional<std::optional<std::string>>>::encode(value.kind);
    }
    if (value.description) {
        result["description"] = codec<std::optional<std::optional<std::string>>>::encode(value.description);
    }
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    if (value.card_title) {
        result["card_title"] = codec<std::optional<std::string>>::encode(value.card_title);
    }
    return result;
}
inline holder::api::Milestone codec<holder::api::Milestone>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected Milestone");
    if (!value.contains("milestone_id")) throw std::invalid_argument("missing milestone_id");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("start_at")) throw std::invalid_argument("missing start_at");
    if (!value.contains("all_day")) throw std::invalid_argument("missing all_day");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::Milestone{
        codec<std::string>::decode(value.at("milestone_id")),
        codec<std::string>::decode(value.at("card_id")),
        codec<long long>::decode(value.at("start_at")),
        value.contains("end_at") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("end_at"))} : std::nullopt,
        codec<bool>::decode(value.at("all_day")),
        value.contains("kind") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("kind"))} : std::nullopt,
        value.contains("description") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("description"))} : std::nullopt,
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
        value.contains("card_title") ? codec<std::optional<std::string>>::decode(value.at("card_title")) : std::nullopt,
    };
}

inline json codec<holder::api::MilestoneListResponse>::encode(const holder::api::MilestoneListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::Milestone>>::encode(value.data);
    return result;
}
inline holder::api::MilestoneListResponse codec<holder::api::MilestoneListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::MilestoneListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::Milestone>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::MilestoneResponse>::encode(const holder::api::MilestoneResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::Milestone>::encode(value.data);
    return result;
}
inline holder::api::MilestoneResponse codec<holder::api::MilestoneResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::MilestoneResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::Milestone>::decode(value.at("data")),
    };
}

inline json codec<holder::api::MilestoneCreateRequest>::encode(const holder::api::MilestoneCreateRequest& value) {
    json result = json::object();
    result["start_at"] = codec<long long>::encode(value.start_at);
    if (value.end_at) {
        result["end_at"] = codec<std::optional<std::optional<long long>>>::encode(value.end_at);
    }
    result["all_day"] = codec<bool>::encode(value.all_day);
    if (value.kind) {
        result["kind"] = codec<std::optional<std::optional<std::string>>>::encode(value.kind);
    }
    if (value.description) {
        result["description"] = codec<std::optional<std::optional<std::string>>>::encode(value.description);
    }
    return result;
}
inline holder::api::MilestoneCreateRequest codec<holder::api::MilestoneCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneCreateRequest");
    if (!value.contains("start_at")) throw std::invalid_argument("missing start_at");
    return holder::api::MilestoneCreateRequest{
        codec<long long>::decode(value.at("start_at")),
        value.contains("end_at") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("end_at"))} : std::nullopt,
        value.contains("all_day") ? codec<bool>::decode(value.at("all_day")) : bool(false),
        value.contains("kind") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("kind"))} : std::nullopt,
        value.contains("description") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("description"))} : std::nullopt,
    };
}

inline json codec<holder::api::MilestoneUpdateRequest>::encode(const holder::api::MilestoneUpdateRequest& value) {
    json result = json::object();
    if (value.start_at) {
        result["start_at"] = codec<std::optional<long long>>::encode(value.start_at);
    }
    if (value.end_at) {
        result["end_at"] = codec<std::optional<std::optional<long long>>>::encode(value.end_at);
    }
    if (value.all_day) {
        result["all_day"] = codec<std::optional<bool>>::encode(value.all_day);
    }
    if (value.kind) {
        result["kind"] = codec<std::optional<std::optional<std::string>>>::encode(value.kind);
    }
    if (value.description) {
        result["description"] = codec<std::optional<std::optional<std::string>>>::encode(value.description);
    }
    return result;
}
inline holder::api::MilestoneUpdateRequest codec<holder::api::MilestoneUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected MilestoneUpdateRequest");
    if (value.size() < 1) throw std::invalid_argument("too few MilestoneUpdateRequest fields");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (it.key() != "start_at" && it.key() != "end_at" && it.key() != "all_day" && it.key() != "kind" && it.key() != "description") throw std::invalid_argument("unexpected MilestoneUpdateRequest field");
    return holder::api::MilestoneUpdateRequest{
        value.contains("start_at") ? codec<std::optional<long long>>::decode(value.at("start_at")) : std::nullopt,
        value.contains("end_at") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("end_at"))} : std::nullopt,
        value.contains("all_day") ? codec<std::optional<bool>>::decode(value.at("all_day")) : std::nullopt,
        value.contains("kind") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("kind"))} : std::nullopt,
        value.contains("description") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("description"))} : std::nullopt,
    };
}

inline json codec<holder::api::CardListResponse>::encode(const holder::api::CardListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::CardListItem>>::encode(value.data);
    return result;
}
inline holder::api::CardListResponse codec<holder::api::CardListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::CardListItem>>::decode(value.at("data")),
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

inline json codec<holder::api::CardContextDataProject>::encode(const holder::api::CardContextDataProject& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["name"] = codec<std::string>::encode(value.name);
    return result;
}
inline holder::api::CardContextDataProject codec<holder::api::CardContextDataProject>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardContextDataProject");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("name")) throw std::invalid_argument("missing name");
    return holder::api::CardContextDataProject{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("name")),
    };
}

inline json codec<holder::api::CardContextBreadcrumb>::encode(const holder::api::CardContextBreadcrumb& value) {
    json result = json::object();
    if (value.type != "project" && value.type != "card") throw std::invalid_argument("invalid type");
    result["type"] = codec<std::string>::encode(value.type);
    result["title"] = codec<std::string>::encode(value.title);
    if (value.project_id) {
        result["project_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.project_id);
    }
    if (value.card_id) {
        result["card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.card_id);
    }
    return result;
}
inline holder::api::CardContextBreadcrumb codec<holder::api::CardContextBreadcrumb>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardContextBreadcrumb");
    if (!value.contains("type")) throw std::invalid_argument("missing type");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (value.at("type") != "project" && value.at("type") != "card") throw std::invalid_argument("invalid type");
    return holder::api::CardContextBreadcrumb{
        codec<std::string>::decode(value.at("type")),
        codec<std::string>::decode(value.at("title")),
        value.contains("project_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("project_id"))} : std::nullopt,
        value.contains("card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("card_id"))} : std::nullopt,
    };
}

inline json codec<holder::api::CardContextCard>::encode(const holder::api::CardContextCard& value) {
    json result = codec<holder::api::CardListItem>::encode(value);
    return result;
}
inline holder::api::CardContextCard codec<holder::api::CardContextCard>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardContextCard");
    json base = value;
    return holder::api::CardContextCard{
        codec<holder::api::CardListItem>::decode(base),
    };
}

inline json codec<holder::api::CardContextData>::encode(const holder::api::CardContextData& value) {
    json result = json::object();
    result["project"] = codec<holder::api::CardContextDataProject>::encode(value.project);
    result["current_parent_card_id"] = codec<std::optional<std::string>>::encode(value.current_parent_card_id);
    result["breadcrumbs"] = codec<std::vector<holder::api::CardContextBreadcrumb>>::encode(value.breadcrumbs);
    result["cards"] = codec<std::vector<holder::api::CardContextCard>>::encode(value.cards);
    return result;
}
inline holder::api::CardContextData codec<holder::api::CardContextData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardContextData");
    if (!value.contains("project")) throw std::invalid_argument("missing project");
    if (!value.contains("current_parent_card_id")) throw std::invalid_argument("missing current_parent_card_id");
    if (!value.contains("breadcrumbs")) throw std::invalid_argument("missing breadcrumbs");
    if (!value.contains("cards")) throw std::invalid_argument("missing cards");
    return holder::api::CardContextData{
        codec<holder::api::CardContextDataProject>::decode(value.at("project")),
        codec<std::optional<std::string>>::decode(value.at("current_parent_card_id")),
        codec<std::vector<holder::api::CardContextBreadcrumb>>::decode(value.at("breadcrumbs")),
        codec<std::vector<holder::api::CardContextCard>>::decode(value.at("cards")),
    };
}

inline json codec<holder::api::CardContextResponse>::encode(const holder::api::CardContextResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardContextData>::encode(value.data);
    return result;
}
inline holder::api::CardContextResponse codec<holder::api::CardContextResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardContextResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardContextResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardContextData>::decode(value.at("data")),
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

inline json codec<holder::api::CardMoveResult>::encode(const holder::api::CardMoveResult& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["parent_card_id"] = codec<std::optional<std::string>>::encode(value.parent_card_id);
    result["sort_key"] = codec<double>::encode(value.sort_key);
    result["revision"] = codec<long long>::encode(value.revision);
    if (value.moved_into_title) {
        result["moved_into_title"] = codec<std::optional<std::optional<std::string>>>::encode(value.moved_into_title);
    }
    return result;
}
inline holder::api::CardMoveResult codec<holder::api::CardMoveResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardMoveResult");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("parent_card_id")) throw std::invalid_argument("missing parent_card_id");
    if (!value.contains("sort_key")) throw std::invalid_argument("missing sort_key");
    if (!value.contains("revision")) throw std::invalid_argument("missing revision");
    return holder::api::CardMoveResult{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::optional<std::string>>::decode(value.at("parent_card_id")),
        codec<double>::decode(value.at("sort_key")),
        codec<long long>::decode(value.at("revision")),
        value.contains("moved_into_title") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("moved_into_title"))} : std::nullopt,
    };
}

inline json codec<holder::api::CardMoveResponse>::encode(const holder::api::CardMoveResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardMoveResult>::encode(value.data);
    return result;
}
inline holder::api::CardMoveResponse codec<holder::api::CardMoveResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardMoveResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardMoveResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardMoveResult>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardMoveIntent>::encode(const holder::api::CardMoveIntent& value) {
    switch (value) {
    case holder::api::CardMoveIntent::into: return "into";
    case holder::api::CardMoveIntent::before: return "before";
    case holder::api::CardMoveIntent::after: return "after";
    case holder::api::CardMoveIntent::left: return "left";
    case holder::api::CardMoveIntent::right: return "right";
    case holder::api::CardMoveIntent::to_start: return "to_start";
    case holder::api::CardMoveIntent::to_end: return "to_end";
    case holder::api::CardMoveIntent::up_level: return "up_level";
    }
    throw std::invalid_argument("invalid CardMoveIntent");
}
inline holder::api::CardMoveIntent codec<holder::api::CardMoveIntent>::decode(const json& value) {
    if (!value.is_string()) throw std::invalid_argument("expected CardMoveIntent");
    if (value == "into") return holder::api::CardMoveIntent::into;
    if (value == "before") return holder::api::CardMoveIntent::before;
    if (value == "after") return holder::api::CardMoveIntent::after;
    if (value == "left") return holder::api::CardMoveIntent::left;
    if (value == "right") return holder::api::CardMoveIntent::right;
    if (value == "to_start") return holder::api::CardMoveIntent::to_start;
    if (value == "to_end") return holder::api::CardMoveIntent::to_end;
    if (value == "up_level") return holder::api::CardMoveIntent::up_level;
    throw std::invalid_argument("unknown CardMoveIntent");
}

inline json codec<holder::api::CardMoveRequest>::encode(const holder::api::CardMoveRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["intent"] = codec<holder::api::CardMoveIntent>::encode(value.intent);
    if (value.target_card_id) {
        result["target_card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.target_card_id);
    }
    if (value.parent_card_id) {
        result["parent_card_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.parent_card_id);
    }
    if (value.if_revision) {
        result["if_revision"] = codec<std::optional<std::optional<long long>>>::encode(value.if_revision);
    }
    return result;
}
inline holder::api::CardMoveRequest codec<holder::api::CardMoveRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardMoveRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("intent")) throw std::invalid_argument("missing intent");
    return holder::api::CardMoveRequest{
        codec<std::string>::decode(value.at("project_id")),
        codec<holder::api::CardMoveIntent>::decode(value.at("intent")),
        value.contains("target_card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("target_card_id"))} : std::nullopt,
        value.contains("parent_card_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("parent_card_id"))} : std::nullopt,
        value.contains("if_revision") ? std::optional<std::optional<long long>>{std::in_place, codec<std::optional<long long>>::decode(value.at("if_revision"))} : std::nullopt,
    };
}

inline json codec<holder::api::CardTagMutationResult>::encode(const holder::api::CardTagMutationResult& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["tag"] = codec<std::string>::encode(value.tag);
    if (value.outcome != "added" && value.outcome != "already_present" && value.outcome != "removed" && value.outcome != "not_present" && value.outcome != "present_outside_editable_tag_line") throw std::invalid_argument("invalid outcome");
    result["outcome"] = codec<std::string>::encode(value.outcome);
    result["changed"] = codec<bool>::encode(value.changed);
    return result;
}
inline holder::api::CardTagMutationResult codec<holder::api::CardTagMutationResult>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardTagMutationResult");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("tag")) throw std::invalid_argument("missing tag");
    if (!value.contains("outcome")) throw std::invalid_argument("missing outcome");
    if (!value.contains("changed")) throw std::invalid_argument("missing changed");
    if (value.at("outcome") != "added" && value.at("outcome") != "already_present" && value.at("outcome") != "removed" && value.at("outcome") != "not_present" && value.at("outcome") != "present_outside_editable_tag_line") throw std::invalid_argument("invalid outcome");
    return holder::api::CardTagMutationResult{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("tag")),
        codec<std::string>::decode(value.at("outcome")),
        codec<bool>::decode(value.at("changed")),
    };
}

inline json codec<holder::api::CardTagMutationResponse>::encode(const holder::api::CardTagMutationResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardTagMutationResult>::encode(value.data);
    return result;
}
inline holder::api::CardTagMutationResponse codec<holder::api::CardTagMutationResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardTagMutationResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardTagMutationResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardTagMutationResult>::decode(value.at("data")),
    };
}

inline json codec<holder::api::CardTagMutationRequest>::encode(const holder::api::CardTagMutationRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["tag"] = codec<std::string>::encode(value.tag);
    return result;
}
inline holder::api::CardTagMutationRequest codec<holder::api::CardTagMutationRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardTagMutationRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("tag")) throw std::invalid_argument("missing tag");
    if (value.contains("project_id") && value.at("project_id").is_string()) {
        const auto length = value.at("project_id").get_ref<const std::string&>().size();
        if (length < 1 || length > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid length: project_id");
    }
    if (value.contains("tag") && value.at("tag").is_string()) {
        const auto length = value.at("tag").get_ref<const std::string&>().size();
        if (length < 1 || length > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid length: tag");
    }
    return holder::api::CardTagMutationRequest{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("tag")),
    };
}

inline json codec<holder::api::ResourceAttachmentResponseData>::encode(const holder::api::ResourceAttachmentResponseData& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["resource_id"] = codec<std::string>::encode(value.resource_id);
    result["changed"] = codec<bool>::encode(value.changed);
    if (value.outcome != "attached" && value.outcome != "already_attached" && value.outcome != "detached" && value.outcome != "not_attached") throw std::invalid_argument("invalid outcome");
    result["outcome"] = codec<std::string>::encode(value.outcome);
    return result;
}
inline holder::api::ResourceAttachmentResponseData codec<holder::api::ResourceAttachmentResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceAttachmentResponseData");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("resource_id")) throw std::invalid_argument("missing resource_id");
    if (!value.contains("changed")) throw std::invalid_argument("missing changed");
    if (!value.contains("outcome")) throw std::invalid_argument("missing outcome");
    if (value.at("outcome") != "attached" && value.at("outcome") != "already_attached" && value.at("outcome") != "detached" && value.at("outcome") != "not_attached") throw std::invalid_argument("invalid outcome");
    return holder::api::ResourceAttachmentResponseData{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("resource_id")),
        codec<bool>::decode(value.at("changed")),
        codec<std::string>::decode(value.at("outcome")),
    };
}

inline json codec<holder::api::ResourceAttachmentResponse>::encode(const holder::api::ResourceAttachmentResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ResourceAttachmentResponseData>::encode(value.data);
    return result;
}
inline holder::api::ResourceAttachmentResponse codec<holder::api::ResourceAttachmentResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceAttachmentResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ResourceAttachmentResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ResourceAttachmentResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::PostCardsCardIdResourcesRequest>::encode(const holder::api::PostCardsCardIdResourcesRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["resource_id"] = codec<std::string>::encode(value.resource_id);
    return result;
}
inline holder::api::PostCardsCardIdResourcesRequest codec<holder::api::PostCardsCardIdResourcesRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected PostCardsCardIdResourcesRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("resource_id")) throw std::invalid_argument("missing resource_id");
    if (value.contains("project_id") && value.at("project_id").is_string()) {
        const auto length = value.at("project_id").get_ref<const std::string&>().size();
        if (length < 1 || length > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid length: project_id");
    }
    if (value.contains("resource_id") && value.at("resource_id").is_string()) {
        const auto length = value.at("resource_id").get_ref<const std::string&>().size();
        if (length < 1 || length > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid length: resource_id");
    }
    return holder::api::PostCardsCardIdResourcesRequest{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("resource_id")),
    };
}

inline json codec<holder::api::DeleteCardsCardIdResourcesRequest>::encode(const holder::api::DeleteCardsCardIdResourcesRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["resource_id"] = codec<std::string>::encode(value.resource_id);
    return result;
}
inline holder::api::DeleteCardsCardIdResourcesRequest codec<holder::api::DeleteCardsCardIdResourcesRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected DeleteCardsCardIdResourcesRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("resource_id")) throw std::invalid_argument("missing resource_id");
    if (value.contains("project_id") && value.at("project_id").is_string()) {
        const auto length = value.at("project_id").get_ref<const std::string&>().size();
        if (length < 1 || length > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid length: project_id");
    }
    if (value.contains("resource_id") && value.at("resource_id").is_string()) {
        const auto length = value.at("resource_id").get_ref<const std::string&>().size();
        if (length < 1 || length > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid length: resource_id");
    }
    return holder::api::DeleteCardsCardIdResourcesRequest{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("resource_id")),
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

inline json codec<holder::api::ResourceMetadata>::encode(const holder::api::ResourceMetadata& value) {
    return codec<std::map<std::string, std::vector<std::string>>>::encode(value.value);
}
inline holder::api::ResourceMetadata codec<holder::api::ResourceMetadata>::decode(const json& value) {
    return holder::api::ResourceMetadata{codec<std::map<std::string, std::vector<std::string>>>::decode(value)};
}

inline json codec<holder::api::AssetPlacement>::encode(const holder::api::AssetPlacement& value) {
    json result = json::object();
    result["placement_id"] = codec<std::string>::encode(value.placement_id);
    result["location_id"] = codec<std::string>::encode(value.location_id);
    if (value.encoding != "plain" && value.encoding != "holder_asset_v1") throw std::invalid_argument("invalid encoding");
    result["encoding"] = codec<std::string>::encode(value.encoding);
    result["stored_byte_size"] = codec<long long>::encode(value.stored_byte_size);
    result["created_at"] = codec<long long>::encode(value.created_at);
    return result;
}
inline holder::api::AssetPlacement codec<holder::api::AssetPlacement>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected AssetPlacement");
    if (!value.contains("placement_id")) throw std::invalid_argument("missing placement_id");
    if (!value.contains("location_id")) throw std::invalid_argument("missing location_id");
    if (!value.contains("encoding")) throw std::invalid_argument("missing encoding");
    if (!value.contains("stored_byte_size")) throw std::invalid_argument("missing stored_byte_size");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (value.at("encoding") != "plain" && value.at("encoding") != "holder_asset_v1") throw std::invalid_argument("invalid encoding");
    return holder::api::AssetPlacement{
        codec<std::string>::decode(value.at("placement_id")),
        codec<std::string>::decode(value.at("location_id")),
        codec<std::string>::decode(value.at("encoding")),
        codec<long long>::decode(value.at("stored_byte_size")),
        codec<long long>::decode(value.at("created_at")),
    };
}

inline json codec<holder::api::ResourceAsset>::encode(const holder::api::ResourceAsset& value) {
    json result = json::object();
    result["asset_id"] = codec<std::string>::encode(value.asset_id);
    result["resource_id"] = codec<std::string>::encode(value.resource_id);
    result["original_filename"] = codec<std::string>::encode(value.original_filename);
    result["media_type"] = codec<std::string>::encode(value.media_type);
    result["byte_size"] = codec<long long>::encode(value.byte_size);
    result["plaintext_sha256"] = codec<std::string>::encode(value.plaintext_sha256);
    result["placements"] = codec<std::vector<holder::api::AssetPlacement>>::encode(value.placements);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::ResourceAsset codec<holder::api::ResourceAsset>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceAsset");
    if (!value.contains("asset_id")) throw std::invalid_argument("missing asset_id");
    if (!value.contains("resource_id")) throw std::invalid_argument("missing resource_id");
    if (!value.contains("original_filename")) throw std::invalid_argument("missing original_filename");
    if (!value.contains("media_type")) throw std::invalid_argument("missing media_type");
    if (!value.contains("byte_size")) throw std::invalid_argument("missing byte_size");
    if (!value.contains("plaintext_sha256")) throw std::invalid_argument("missing plaintext_sha256");
    if (!value.contains("placements")) throw std::invalid_argument("missing placements");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::ResourceAsset{
        codec<std::string>::decode(value.at("asset_id")),
        codec<std::string>::decode(value.at("resource_id")),
        codec<std::string>::decode(value.at("original_filename")),
        codec<std::string>::decode(value.at("media_type")),
        codec<long long>::decode(value.at("byte_size")),
        codec<std::string>::decode(value.at("plaintext_sha256")),
        codec<std::vector<holder::api::AssetPlacement>>::decode(value.at("placements")),
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::ResourceReferencedByCardsItem>::encode(const holder::api::ResourceReferencedByCardsItem& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    result["link_kinds"] = codec<std::vector<std::string>>::encode(value.link_kinds);
    return result;
}
inline holder::api::ResourceReferencedByCardsItem codec<holder::api::ResourceReferencedByCardsItem>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceReferencedByCardsItem");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    if (!value.contains("link_kinds")) throw std::invalid_argument("missing link_kinds");
    return holder::api::ResourceReferencedByCardsItem{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("title")),
        codec<long long>::decode(value.at("updated_at")),
        codec<std::vector<std::string>>::decode(value.at("link_kinds")),
    };
}

inline json codec<holder::api::Resource>::encode(const holder::api::Resource& value) {
    json result = json::object();
    result["resource_id"] = codec<std::string>::encode(value.resource_id);
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["type"] = codec<std::string>::encode(value.type);
    result["label"] = codec<std::string>::encode(value.label);
    result["metadata"] = codec<holder::api::ResourceMetadata>::encode(value.metadata);
    result["assets"] = codec<std::vector<holder::api::ResourceAsset>>::encode(value.assets);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    if (value.referenced_by_cards) {
        result["referenced_by_cards"] = codec<std::optional<std::vector<holder::api::ResourceReferencedByCardsItem>>>::encode(value.referenced_by_cards);
    }
    return result;
}
inline holder::api::Resource codec<holder::api::Resource>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected Resource");
    if (!value.contains("resource_id")) throw std::invalid_argument("missing resource_id");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("type")) throw std::invalid_argument("missing type");
    if (!value.contains("label")) throw std::invalid_argument("missing label");
    if (!value.contains("metadata")) throw std::invalid_argument("missing metadata");
    if (!value.contains("assets")) throw std::invalid_argument("missing assets");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::Resource{
        codec<std::string>::decode(value.at("resource_id")),
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("type")),
        codec<std::string>::decode(value.at("label")),
        codec<holder::api::ResourceMetadata>::decode(value.at("metadata")),
        codec<std::vector<holder::api::ResourceAsset>>::decode(value.at("assets")),
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
        value.contains("referenced_by_cards") ? codec<std::optional<std::vector<holder::api::ResourceReferencedByCardsItem>>>::decode(value.at("referenced_by_cards")) : std::nullopt,
    };
}

inline json codec<holder::api::ResourceListResponse>::encode(const holder::api::ResourceListResponse& value) {
    json result = json::object();
    if (value.card_id) {
        result["card_id"] = codec<std::optional<std::string>>::encode(value.card_id);
    }
    if (value.limit) {
        result["limit"] = codec<std::optional<int>>::encode(value.limit);
    }
    if (value.offset) {
        result["offset"] = codec<std::optional<int>>::encode(value.offset);
    }
    if (value.next_offset) {
        result["next_offset"] = codec<std::optional<std::optional<int>>>::encode(value.next_offset);
    }
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::Resource>>::encode(value.data);
    return result;
}
inline holder::api::ResourceListResponse codec<holder::api::ResourceListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ResourceListResponse{
        value.contains("card_id") ? codec<std::optional<std::string>>::decode(value.at("card_id")) : std::nullopt,
        value.contains("limit") ? codec<std::optional<int>>::decode(value.at("limit")) : std::nullopt,
        value.contains("offset") ? codec<std::optional<int>>::decode(value.at("offset")) : std::nullopt,
        value.contains("next_offset") ? std::optional<std::optional<int>>{std::in_place, codec<std::optional<int>>::decode(value.at("next_offset"))} : std::nullopt,
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::Resource>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ResourceResponse>::encode(const holder::api::ResourceResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::Resource>::encode(value.data);
    return result;
}
inline holder::api::ResourceResponse codec<holder::api::ResourceResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ResourceResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::Resource>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ResourceCreateRequest>::encode(const holder::api::ResourceCreateRequest& value) {
    json result = json::object();
    if (value.resource_id) {
        result["resource_id"] = codec<std::optional<std::string>>::encode(value.resource_id);
    }
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["type"] = codec<std::string>::encode(value.type);
    result["label"] = codec<std::string>::encode(value.label);
    if (value.metadata) {
        result["metadata"] = codec<std::optional<holder::api::ResourceMetadata>>::encode(value.metadata);
    }
    if (value.created_at) {
        result["created_at"] = codec<std::optional<long long>>::encode(value.created_at);
    }
    if (value.updated_at) {
        result["updated_at"] = codec<std::optional<long long>>::encode(value.updated_at);
    }
    return result;
}
inline holder::api::ResourceCreateRequest codec<holder::api::ResourceCreateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceCreateRequest");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("type")) throw std::invalid_argument("missing type");
    if (!value.contains("label")) throw std::invalid_argument("missing label");
    return holder::api::ResourceCreateRequest{
        value.contains("resource_id") ? codec<std::optional<std::string>>::decode(value.at("resource_id")) : std::nullopt,
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("type")),
        codec<std::string>::decode(value.at("label")),
        value.contains("metadata") ? codec<std::optional<holder::api::ResourceMetadata>>::decode(value.at("metadata")) : std::nullopt,
        value.contains("created_at") ? codec<std::optional<long long>>::decode(value.at("created_at")) : std::nullopt,
        value.contains("updated_at") ? codec<std::optional<long long>>::decode(value.at("updated_at")) : std::nullopt,
    };
}

inline json codec<holder::api::ResourceUpdateRequest>::encode(const holder::api::ResourceUpdateRequest& value) {
    json result = json::object();
    if (value.type) {
        result["type"] = codec<std::optional<std::string>>::encode(value.type);
    }
    if (value.label) {
        result["label"] = codec<std::optional<std::string>>::encode(value.label);
    }
    if (value.metadata) {
        result["metadata"] = codec<std::optional<holder::api::ResourceMetadata>>::encode(value.metadata);
    }
    if (value.updated_at) {
        result["updated_at"] = codec<std::optional<long long>>::encode(value.updated_at);
    }
    return result;
}
inline holder::api::ResourceUpdateRequest codec<holder::api::ResourceUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ResourceUpdateRequest");
    return holder::api::ResourceUpdateRequest{
        value.contains("type") ? codec<std::optional<std::string>>::decode(value.at("type")) : std::nullopt,
        value.contains("label") ? codec<std::optional<std::string>>::decode(value.at("label")) : std::nullopt,
        value.contains("metadata") ? codec<std::optional<holder::api::ResourceMetadata>>::decode(value.at("metadata")) : std::nullopt,
        value.contains("updated_at") ? codec<std::optional<long long>>::decode(value.at("updated_at")) : std::nullopt,
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

inline json codec<holder::api::EventCheckpoint>::encode(const holder::api::EventCheckpoint& value) {
    json result = json::object();
    result["cursor"] = codec<std::string>::encode(value.cursor);
    result["git_revisions"] = codec<std::map<std::string, std::optional<std::string>>>::encode(value.git_revisions);
    result["history_urls"] = codec<std::map<std::string, std::string>>::encode(value.history_urls);
    return result;
}
inline holder::api::EventCheckpoint codec<holder::api::EventCheckpoint>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected EventCheckpoint");
    if (!value.contains("cursor")) throw std::invalid_argument("missing cursor");
    if (!value.contains("git_revisions")) throw std::invalid_argument("missing git_revisions");
    if (!value.contains("history_urls")) throw std::invalid_argument("missing history_urls");
    return holder::api::EventCheckpoint{
        codec<std::string>::decode(value.at("cursor")),
        codec<std::map<std::string, std::optional<std::string>>>::decode(value.at("git_revisions")),
        codec<std::map<std::string, std::string>>::decode(value.at("history_urls")),
    };
}

inline json codec<holder::api::EventCheckpointResponse>::encode(const holder::api::EventCheckpointResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::EventCheckpoint>::encode(value.data);
    return result;
}
inline holder::api::EventCheckpointResponse codec<holder::api::EventCheckpointResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected EventCheckpointResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::EventCheckpointResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::EventCheckpoint>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ChangeEventData>::encode(const holder::api::ChangeEventData& value) {
    json result = json::object();
    if (value.entity != "project" && value.entity != "card" && value.entity != "resource" && value.entity != "location" && value.entity != "thread" && value.entity != "message" && value.entity != "run") throw std::invalid_argument("invalid entity");
    result["entity"] = codec<std::string>::encode(value.entity);
    result["entity_id"] = codec<std::string>::encode(value.entity_id);
    result["project_id"] = codec<std::optional<std::string>>::encode(value.project_id);
    result["deleted"] = codec<bool>::encode(value.deleted);
    result["git_revision"] = codec<std::optional<std::string>>::encode(value.git_revision);
    return result;
}
inline holder::api::ChangeEventData codec<holder::api::ChangeEventData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ChangeEventData");
    if (!value.contains("entity")) throw std::invalid_argument("missing entity");
    if (!value.contains("entity_id")) throw std::invalid_argument("missing entity_id");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("deleted")) throw std::invalid_argument("missing deleted");
    if (!value.contains("git_revision")) throw std::invalid_argument("missing git_revision");
    if (value.at("entity") != "project" && value.at("entity") != "card" && value.at("entity") != "resource" && value.at("entity") != "location" && value.at("entity") != "thread" && value.at("entity") != "message" && value.at("entity") != "run") throw std::invalid_argument("invalid entity");
    return holder::api::ChangeEventData{
        codec<std::string>::decode(value.at("entity")),
        codec<std::string>::decode(value.at("entity_id")),
        codec<std::optional<std::string>>::decode(value.at("project_id")),
        codec<bool>::decode(value.at("deleted")),
        codec<std::optional<std::string>>::decode(value.at("git_revision")),
    };
}

inline json codec<holder::api::EventResyncData>::encode(const holder::api::EventResyncData& value) {
    json result = codec<holder::api::EventCheckpoint>::encode(value);
    if (value.reason != "history_unavailable" && value.reason != "observer_unavailable") throw std::invalid_argument("invalid reason");
    result["reason"] = codec<std::string>::encode(value.reason);
    result["last_revision"] = codec<std::optional<std::string>>::encode(value.last_revision);
    return result;
}
inline holder::api::EventResyncData codec<holder::api::EventResyncData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected EventResyncData");
    if (!value.contains("reason")) throw std::invalid_argument("missing reason");
    if (!value.contains("last_revision")) throw std::invalid_argument("missing last_revision");
    if (value.at("reason") != "history_unavailable" && value.at("reason") != "observer_unavailable") throw std::invalid_argument("invalid reason");
    json base = value;
    base.erase("reason");
    base.erase("last_revision");
    return holder::api::EventResyncData{
        codec<holder::api::EventCheckpoint>::decode(base),
        codec<std::string>::decode(value.at("reason")),
        codec<std::optional<std::string>>::decode(value.at("last_revision")),
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

inline json codec<holder::api::RecoveryTokenImportAutoResponseData>::encode(const holder::api::RecoveryTokenImportAutoResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["project_created"] = codec<bool>::encode(value.project_created);
    result["remote_hint_present"] = codec<bool>::encode(value.remote_hint_present);
    result["remote_configured"] = codec<bool>::encode(value.remote_configured);
    if (value.remote_error) {
        result["remote_error"] = codec<std::optional<std::optional<std::string>>>::encode(value.remote_error);
    }
    if (value.pull_status != "not_attempted" && value.pull_status != "succeeded" && value.pull_status != "failed") throw std::invalid_argument("invalid pull_status");
    result["pull_status"] = codec<std::string>::encode(value.pull_status);
    if (value.pull_error) {
        result["pull_error"] = codec<std::optional<std::optional<std::string>>>::encode(value.pull_error);
    }
    return result;
}
inline holder::api::RecoveryTokenImportAutoResponseData codec<holder::api::RecoveryTokenImportAutoResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RecoveryTokenImportAutoResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("project_created")) throw std::invalid_argument("missing project_created");
    if (!value.contains("remote_hint_present")) throw std::invalid_argument("missing remote_hint_present");
    if (!value.contains("remote_configured")) throw std::invalid_argument("missing remote_configured");
    if (!value.contains("pull_status")) throw std::invalid_argument("missing pull_status");
    if (value.at("pull_status") != "not_attempted" && value.at("pull_status") != "succeeded" && value.at("pull_status") != "failed") throw std::invalid_argument("invalid pull_status");
    return holder::api::RecoveryTokenImportAutoResponseData{
        codec<std::string>::decode(value.at("project_id")),
        codec<bool>::decode(value.at("project_created")),
        codec<bool>::decode(value.at("remote_hint_present")),
        codec<bool>::decode(value.at("remote_configured")),
        value.contains("remote_error") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("remote_error"))} : std::nullopt,
        codec<std::string>::decode(value.at("pull_status")),
        value.contains("pull_error") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("pull_error"))} : std::nullopt,
    };
}

inline json codec<holder::api::RecoveryTokenImportAutoResponse>::encode(const holder::api::RecoveryTokenImportAutoResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::RecoveryTokenImportAutoResponseData>::encode(value.data);
    return result;
}
inline holder::api::RecoveryTokenImportAutoResponse codec<holder::api::RecoveryTokenImportAutoResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected RecoveryTokenImportAutoResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::RecoveryTokenImportAutoResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::RecoveryTokenImportAutoResponseData>::decode(value.at("data")),
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

inline json codec<holder::api::HealthDataPrivacy>::encode(const holder::api::HealthDataPrivacy& value) {
    json result = json::object();
    result["backend"] = codec<std::string>::encode(value.backend);
    result["project_mode_supported"] = codec<bool>::encode(value.project_mode_supported);
    return result;
}
inline holder::api::HealthDataPrivacy codec<holder::api::HealthDataPrivacy>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected HealthDataPrivacy");
    if (!value.contains("backend")) throw std::invalid_argument("missing backend");
    if (!value.contains("project_mode_supported")) throw std::invalid_argument("missing project_mode_supported");
    return holder::api::HealthDataPrivacy{
        codec<std::string>::decode(value.at("backend")),
        codec<bool>::decode(value.at("project_mode_supported")),
    };
}

inline json codec<holder::api::HealthData>::encode(const holder::api::HealthData& value) {
    json result = json::object();
    result["db_ok"] = codec<bool>::encode(value.db_ok);
    result["uptime_ms"] = codec<long long>::encode(value.uptime_ms);
    result["api_version"] = codec<std::string>::encode(value.api_version);
    result["server_version"] = codec<std::string>::encode(value.server_version);
    result["pid"] = codec<int>::encode(value.pid);
    result["privacy"] = codec<holder::api::HealthDataPrivacy>::encode(value.privacy);
    return result;
}
inline holder::api::HealthData codec<holder::api::HealthData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected HealthData");
    if (!value.contains("db_ok")) throw std::invalid_argument("missing db_ok");
    if (!value.contains("uptime_ms")) throw std::invalid_argument("missing uptime_ms");
    if (!value.contains("api_version")) throw std::invalid_argument("missing api_version");
    if (!value.contains("server_version")) throw std::invalid_argument("missing server_version");
    if (!value.contains("pid")) throw std::invalid_argument("missing pid");
    if (!value.contains("privacy")) throw std::invalid_argument("missing privacy");
    return holder::api::HealthData{
        codec<bool>::decode(value.at("db_ok")),
        codec<long long>::decode(value.at("uptime_ms")),
        codec<std::string>::decode(value.at("api_version")),
        codec<std::string>::decode(value.at("server_version")),
        codec<int>::decode(value.at("pid")),
        codec<holder::api::HealthDataPrivacy>::decode(value.at("privacy")),
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

inline json codec<holder::api::ProjectListResponse>::encode(const holder::api::ProjectListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::Project>>::encode(value.data);
    return result;
}
inline holder::api::ProjectListResponse codec<holder::api::ProjectListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::Project>>::decode(value.at("data")),
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

inline json codec<holder::api::ProjectCreateResponse>::encode(const holder::api::ProjectCreateResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::Project>::encode(value.data);
    return result;
}
inline holder::api::ProjectCreateResponse codec<holder::api::ProjectCreateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectCreateResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectCreateResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::Project>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectUpdateResponseData>::encode(const holder::api::ProjectUpdateResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    if (value.git_remote_changed) {
        result["git_remote_changed"] = codec<std::optional<bool>>::encode(value.git_remote_changed);
    }
    return result;
}
inline holder::api::ProjectUpdateResponseData codec<holder::api::ProjectUpdateResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectUpdateResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    return holder::api::ProjectUpdateResponseData{
        codec<std::string>::decode(value.at("project_id")),
        value.contains("git_remote_changed") ? codec<std::optional<bool>>::decode(value.at("git_remote_changed")) : std::nullopt,
    };
}

inline json codec<holder::api::ProjectUpdateResponse>::encode(const holder::api::ProjectUpdateResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectUpdateResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectUpdateResponse codec<holder::api::ProjectUpdateResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectUpdateResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectUpdateResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectUpdateResponseData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectUpdateRequestPrivacyMode>::encode(const holder::api::ProjectUpdateRequestPrivacyMode& value) {
    switch (value) {
    case holder::api::ProjectUpdateRequestPrivacyMode::encrypted_git: return "encrypted_git";
    case holder::api::ProjectUpdateRequestPrivacyMode::plain: return "plain";
    }
    throw std::invalid_argument("invalid ProjectUpdateRequestPrivacyMode");
}
inline holder::api::ProjectUpdateRequestPrivacyMode codec<holder::api::ProjectUpdateRequestPrivacyMode>::decode(const json& value) {
    if (!value.is_string()) throw std::invalid_argument("expected ProjectUpdateRequestPrivacyMode");
    if (value == "encrypted_git") return holder::api::ProjectUpdateRequestPrivacyMode::encrypted_git;
    if (value == "plain") return holder::api::ProjectUpdateRequestPrivacyMode::plain;
    throw std::invalid_argument("unknown ProjectUpdateRequestPrivacyMode");
}

inline json codec<holder::api::ProjectUpdateRequest>::encode(const holder::api::ProjectUpdateRequest& value) {
    json result = json::object();
    if (value.name) {
        result["name"] = codec<std::optional<std::optional<std::string>>>::encode(value.name);
    }
    if (value.root_path) {
        result["root_path"] = codec<std::optional<std::optional<std::string>>>::encode(value.root_path);
    }
    if (value.git_remote_url) {
        result["git_remote_url"] = codec<std::optional<std::optional<std::string>>>::encode(value.git_remote_url);
    }
    if (value.git_provider) {
        result["git_provider"] = codec<std::optional<std::optional<std::string>>>::encode(value.git_provider);
    }
    if (value.privacy_mode) {
        result["privacy_mode"] = codec<std::optional<std::optional<holder::api::ProjectUpdateRequestPrivacyMode>>>::encode(value.privacy_mode);
    }
    if (value.project_key_id) {
        result["project_key_id"] = codec<std::optional<std::optional<std::string>>>::encode(value.project_key_id);
    }
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::ProjectUpdateRequest codec<holder::api::ProjectUpdateRequest>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectUpdateRequest");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::ProjectUpdateRequest{
        value.contains("name") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("name"))} : std::nullopt,
        value.contains("root_path") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("root_path"))} : std::nullopt,
        value.contains("git_remote_url") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("git_remote_url"))} : std::nullopt,
        value.contains("git_provider") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("git_provider"))} : std::nullopt,
        value.contains("privacy_mode") ? std::optional<std::optional<holder::api::ProjectUpdateRequestPrivacyMode>>{std::in_place, codec<std::optional<holder::api::ProjectUpdateRequestPrivacyMode>>::decode(value.at("privacy_mode"))} : std::nullopt,
        value.contains("project_key_id") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("project_key_id"))} : std::nullopt,
        codec<long long>::decode(value.at("updated_at")),
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

inline json codec<holder::api::CalendarCardActivity>::encode(const holder::api::CalendarCardActivity& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["created_at"] = codec<long long>::encode(value.created_at);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::CalendarCardActivity codec<holder::api::CalendarCardActivity>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CalendarCardActivity");
    if (!value.contains("card_id")) throw std::invalid_argument("missing card_id");
    if (!value.contains("title")) throw std::invalid_argument("missing title");
    if (!value.contains("created_at")) throw std::invalid_argument("missing created_at");
    if (!value.contains("updated_at")) throw std::invalid_argument("missing updated_at");
    return holder::api::CalendarCardActivity{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("title")),
        codec<long long>::decode(value.at("created_at")),
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::ProjectCalendarResponseData>::encode(const holder::api::ProjectCalendarResponseData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["from"] = codec<long long>::encode(value.from);
    result["to"] = codec<long long>::encode(value.to);
    result["milestones"] = codec<std::vector<holder::api::Milestone>>::encode(value.milestones);
    result["created_cards"] = codec<std::vector<holder::api::CalendarCardActivity>>::encode(value.created_cards);
    result["updated_cards"] = codec<std::vector<holder::api::CalendarCardActivity>>::encode(value.updated_cards);
    return result;
}
inline holder::api::ProjectCalendarResponseData codec<holder::api::ProjectCalendarResponseData>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectCalendarResponseData");
    if (!value.contains("project_id")) throw std::invalid_argument("missing project_id");
    if (!value.contains("from")) throw std::invalid_argument("missing from");
    if (!value.contains("to")) throw std::invalid_argument("missing to");
    if (!value.contains("milestones")) throw std::invalid_argument("missing milestones");
    if (!value.contains("created_cards")) throw std::invalid_argument("missing created_cards");
    if (!value.contains("updated_cards")) throw std::invalid_argument("missing updated_cards");
    return holder::api::ProjectCalendarResponseData{
        codec<std::string>::decode(value.at("project_id")),
        codec<long long>::decode(value.at("from")),
        codec<long long>::decode(value.at("to")),
        codec<std::vector<holder::api::Milestone>>::decode(value.at("milestones")),
        codec<std::vector<holder::api::CalendarCardActivity>>::decode(value.at("created_cards")),
        codec<std::vector<holder::api::CalendarCardActivity>>::decode(value.at("updated_cards")),
    };
}

inline json codec<holder::api::ProjectCalendarResponse>::encode(const holder::api::ProjectCalendarResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::ProjectCalendarResponseData>::encode(value.data);
    return result;
}
inline holder::api::ProjectCalendarResponse codec<holder::api::ProjectCalendarResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected ProjectCalendarResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectCalendarResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::ProjectCalendarResponseData>::decode(value.at("data")),
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

inline json codec<holder::api::routes::CardBacklinkListResponse>::encode(const holder::api::routes::CardBacklinkListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::routes::CardLink>>::encode(value.data);
    if (value.scope != "local" && value.scope != "external" && value.scope != "all_available") throw std::invalid_argument("invalid scope");
    result["scope"] = codec<std::string>::encode(value.scope);
    if (value.coverage != "target_project" && value.coverage != "available_projects") throw std::invalid_argument("invalid coverage");
    result["coverage"] = codec<std::string>::encode(value.coverage);
    return result;
}
inline holder::api::routes::CardBacklinkListResponse codec<holder::api::routes::CardBacklinkListResponse>::decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("expected CardBacklinkListResponse");
    if (!value.contains("ok")) throw std::invalid_argument("missing ok");
    if (!value.contains("data")) throw std::invalid_argument("missing data");
    if (!value.contains("scope")) throw std::invalid_argument("missing scope");
    if (!value.contains("coverage")) throw std::invalid_argument("missing coverage");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    if (value.at("scope") != "local" && value.at("scope") != "external" && value.at("scope") != "all_available") throw std::invalid_argument("invalid scope");
    if (value.at("coverage") != "target_project" && value.at("coverage") != "available_projects") throw std::invalid_argument("invalid coverage");
    return holder::api::routes::CardBacklinkListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::routes::CardLink>>::decode(value.at("data")),
        codec<std::string>::decode(value.at("scope")),
        codec<std::string>::decode(value.at("coverage")),
    };
}

template <typename T> json encode(const T& value) { return codec<T>::encode(value); }
template <typename T> T decode(const json& value) { return codec<T>::decode(value); }
} // namespace rig_json
