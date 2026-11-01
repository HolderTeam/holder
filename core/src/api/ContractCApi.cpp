// Generated from contract/interface/ai/models.py, contract/interface/ai/operations.py, contract/interface/cards/errors.py, contract/interface/cards/models.py, contract/interface/cards/operations.py, contract/interface/cards/references.py, contract/interface/daemon/models.py, contract/interface/daemon/operations.py, contract/interface/documents/operations.py, contract/interface/errors.py, contract/interface/events/models.py, contract/interface/events/operations.py, contract/interface/git/models.py, contract/interface/git/operations.py, contract/interface/history/errors.py, contract/interface/history/models.py, contract/interface/history/operations.py, contract/interface/links/models.py, contract/interface/links/operations.py, contract/interface/locations/models.py, contract/interface/locations/operations.py, contract/interface/maintenance/errors.py, contract/interface/maintenance/models.py, contract/interface/maintenance/operations.py, contract/interface/maintenance/rebuild.py, contract/interface/milestones/models.py, contract/interface/milestones/operations.py, contract/interface/pagination/models.py, contract/interface/projects/ids.py, contract/interface/projects/models.py, contract/interface/projects/operations.py, contract/interface/recovery/errors.py, contract/interface/recovery/models.py, contract/interface/recovery/operations.py, contract/interface/resources/models.py, contract/interface/resources/operations.py, contract/interface/search/models.py, contract/interface/search/operations.py, contract/interface/trash/errors.py, contract/interface/trash/models.py, contract/interface/trash/operations.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// The C entry points: argument checks, conversions and error codes.
#include "Contract.h"
#include "holder/holder.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace contract = holder::contract;

struct holder_arena {
  std::vector<std::unique_ptr<std::byte[]>> blocks;
};

namespace {

template <class T>
T* make(holder_arena* arena, std::size_t count = 1) {
  if (count == 0) return nullptr;
  auto block = std::make_unique<std::byte[]>(sizeof(T) * count);
  T* items = reinterpret_cast<T*>(block.get());
  std::uninitialized_value_construct_n(items, count);
  arena->blocks.push_back(std::move(block));
  return items;
}

const char* copy(holder_arena* arena, const std::string& text) {
  char* out = make<char>(arena, text.size() + 1);
  std::memcpy(out, text.data(), text.size());
  return out;
}

template <class C, class T>
const C* copy_value(holder_arena* arena, const std::optional<T>& value) {
  if (!value) return nullptr;
  C* out = make<C>(arena);
  *out = static_cast<C>(*value);
  return out;
}

template <class C, class T, class Convert>
const C* copy_optional(holder_arena* arena, const std::optional<T>& value, Convert convert) {
  if (!value) return nullptr;
  C* out = make<C>(arena);
  *out = convert(*value);
  return out;
}

const char* copy_text(holder_arena* arena, const std::optional<std::string>& value) {
  return value ? copy(arena, *value) : nullptr;
}

holder_string_list copy_strings(holder_arena* arena, const std::vector<std::string>& values) {
  auto* items = make<const char*>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = copy(arena, values[i]);
  return {items, values.size()};
}

struct ArgumentError : std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

void require(bool ok, const std::string& message) {
  if (!ok) throw ArgumentError(message);
}

// The number of characters in text, or nothing when it is not well-formed UTF-8: no overlong
// forms, surrogates or code points above U+10FFFF.
std::optional<std::size_t> utf8_length(const std::string& text) {
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

// Lengths count characters, as the contract does.
std::string text(const char* value, const char* name, std::size_t min_length, std::size_t max_length) {
  require(value != nullptr, std::string(name) + " must not be null");
  std::string out(value);
  const auto length = utf8_length(out);
  require(length.has_value(), std::string(name) + " must be UTF-8");
  require(*length >= min_length, std::string(name) + (min_length == 1 ? " must not be empty"
                                                                       : " must be at least " + std::to_string(min_length) + " characters"));
  require(*length <= max_length, std::string(name) + " must be at most " + std::to_string(max_length) + " characters");
  return out;
}

std::optional<std::string> optional_text(const char* value, const char* name, std::size_t min_length,
                                         std::size_t max_length) {
  if (value == nullptr) return std::nullopt;
  return text(value, name, min_length, max_length);
}

template <class T>
T in_range(T value, const char* name, T minimum, T maximum) {
  require(value >= minimum && value <= maximum, std::string(name) + " must be from " + std::to_string(minimum) +
                                                    " to " + std::to_string(maximum));
  return value;
}

double finite(double value, const char* name) {
  require(std::isfinite(value), std::string(name) + " must be a finite number");
  return value;
}

template <class Body>
int guarded(holder_arena* arena, holder_error** out_error, Body&& body) {
  if (out_error != nullptr) *out_error = nullptr;
  if (arena == nullptr) return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "arena must not be null");
  try {
    body();
    return HOLDER_OK;
  } catch (const ArgumentError& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, e.what());
  } catch (const contract::InvalidArgument& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, e.what());
  } catch (const contract::NotFound& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_NOT_FOUND, e.what());
  } catch (const contract::Conflict& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_CONFLICT, e.what());
  } catch (const contract::KeyringUnavailable& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_KEYRING_UNAVAILABLE, e.what());
  } catch (const contract::KeyMissing& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_KEY_MISSING, e.what());
  } catch (const contract::DataCorrupted& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_DATA_CORRUPTED, e.what());
  } catch (const contract::InvalidMove& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_MOVE, e.what());
  } catch (const contract::HierarchyInvalid& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_HIERARCHY_INVALID, e.what());
  } catch (const contract::HistoryUnavailable& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_HISTORY_UNAVAILABLE, e.what());
  } catch (const contract::HistoryKeyUnavailable& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_HISTORY_KEY_UNAVAILABLE, e.what());
  } catch (const contract::RevisionAmbiguous& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_REVISION_AMBIGUOUS, e.what());
  } catch (const contract::HistoryResponseTooLarge& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_HISTORY_RESPONSE_TOO_LARGE, e.what());
  } catch (const contract::RebuildFailed& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_REBUILD_FAILED, e.what());
  } catch (const contract::RecoveryTokenInvalid& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_RECOVERY_TOKEN_INVALID, e.what());
  } catch (const contract::NotTrashed& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_NOT_TRASHED, e.what());
  } catch (const std::bad_alloc&) {
    return contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  } catch (const std::exception& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, e.what());
  } catch (...) {
    return contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, "unknown failure");
  }
}

// A partial update's text field: kept, set to its value, or cleared.
contract::Change<std::string> changed_text(const holder_string_change& value, const char* name,
                                           std::size_t min_length, std::size_t max_length) {
  switch (value.change) {
  case HOLDER_KEEP: return std::nullopt;
  case HOLDER_SET: return text(value.value, (std::string(name) + ".value").c_str(), min_length, max_length);
  case HOLDER_CLEAR: return std::optional<std::string>{};
  default: throw ArgumentError(std::string(name) + ".change is not a holder_change value");
  }
}

holder_location_configuration to_c_location_configuration(holder_arena* arena, const std::map<std::string, std::string>& value) {
  auto* items = make<holder_location_configuration_entry>(arena, value.size());
  std::size_t i = 0;
  for (const auto& [key, text] : value) items[i++] = {copy(arena, key), copy(arena, text)};
  return {items, value.size()};
}

std::map<std::string, std::string> from_c_location_configuration(const holder_location_configuration& value, const char* name) {
  require(value.items != nullptr || value.count == 0, std::string(name) + ".items must not be null");
  std::map<std::string, std::string> out;
  for (std::size_t i = 0; i < value.count; ++i) {
    const auto& entry = value.items[i];
    auto key = text(entry.key, (std::string(name) + " key").c_str(), 0, SIZE_MAX);
    auto value_text = text(entry.values, (std::string(name) + "." + key).c_str(), 0, SIZE_MAX);
    require(out.emplace(std::move(key), std::move(value_text)).second, std::string(name) + " repeats a key");
  }
  return out;
}

holder_resource_metadata to_c_resource_metadata(holder_arena* arena, const std::map<std::string, std::vector<std::string>>& value) {
  auto* items = make<holder_resource_metadata_entry>(arena, value.size());
  std::size_t i = 0;
  for (const auto& [key, values] : value) items[i++] = {copy(arena, key), copy_strings(arena, values)};
  return {items, value.size()};
}

std::map<std::string, std::vector<std::string>> from_c_resource_metadata(const holder_resource_metadata& value, const char* name) {
  require(value.items != nullptr || value.count == 0, std::string(name) + ".items must not be null");
  std::map<std::string, std::vector<std::string>> out;
  for (std::size_t i = 0; i < value.count; ++i) {
    const auto& entry = value.items[i];
    auto key = text(entry.key, (std::string(name) + " key").c_str(), 0, SIZE_MAX);
    const auto& list = entry.values;
    require(list.items != nullptr || list.count == 0, std::string(name) + "." + key + ".items must not be null");
    std::vector<std::string> values;
    for (std::size_t k = 0; k < list.count; ++k)
      values.push_back(text(list.items[k], (std::string(name) + "." + key).c_str(), 0, SIZE_MAX));
    require(out.emplace(std::move(key), std::move(values)).second, std::string(name) + " repeats a key");
  }
  return out;
}

holder_project_storage_location_provider to_c_enum(contract::ProjectStorageLocationProvider value) {
  switch (value) {
  case contract::ProjectStorageLocationProvider::LocalDirectory: return HOLDER_PROJECT_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY;
  case contract::ProjectStorageLocationProvider::S3Compatible: return HOLDER_PROJECT_STORAGE_LOCATION_PROVIDER_S3_COMPATIBLE;
  case contract::ProjectStorageLocationProvider::GoogleDrive: return HOLDER_PROJECT_STORAGE_LOCATION_PROVIDER_GOOGLE_DRIVE;
  }
  throw std::logic_error("not a contract::ProjectStorageLocationProvider value");
}

holder_card_breadcrumb_kind to_c_enum(contract::CardBreadcrumbKind value) {
  switch (value) {
  case contract::CardBreadcrumbKind::Project: return HOLDER_CARD_BREADCRUMB_KIND_PROJECT;
  case contract::CardBreadcrumbKind::Card: return HOLDER_CARD_BREADCRUMB_KIND_CARD;
  }
  throw std::logic_error("not a contract::CardBreadcrumbKind value");
}

holder_card_reference_resolve_data_status to_c_enum(contract::CardReferenceResolveDataStatus value) {
  switch (value) {
  case contract::CardReferenceResolveDataStatus::Resolved: return HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_RESOLVED;
  case contract::CardReferenceResolveDataStatus::Ambiguous: return HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_AMBIGUOUS;
  case contract::CardReferenceResolveDataStatus::NotFound: return HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_NOT_FOUND;
  }
  throw std::logic_error("not a contract::CardReferenceResolveDataStatus value");
}

holder_link_source_type to_c_enum(contract::LinkSourceType value) {
  switch (value) {
  case contract::LinkSourceType::Card: return HOLDER_LINK_SOURCE_TYPE_CARD;
  case contract::LinkSourceType::AiMessage: return HOLDER_LINK_SOURCE_TYPE_AI_MESSAGE;
  }
  throw std::logic_error("not a contract::LinkSourceType value");
}

holder_link_target_type to_c_enum(contract::LinkTargetType value) {
  switch (value) {
  case contract::LinkTargetType::Card: return HOLDER_LINK_TARGET_TYPE_CARD;
  case contract::LinkTargetType::AiMessage: return HOLDER_LINK_TARGET_TYPE_AI_MESSAGE;
  case contract::LinkTargetType::AiThread: return HOLDER_LINK_TARGET_TYPE_AI_THREAD;
  case contract::LinkTargetType::Resource: return HOLDER_LINK_TARGET_TYPE_RESOURCE;
  }
  throw std::logic_error("not a contract::LinkTargetType value");
}

holder_asset_placement_encoding to_c_enum(contract::AssetPlacementEncoding value) {
  switch (value) {
  case contract::AssetPlacementEncoding::Plain: return HOLDER_ASSET_PLACEMENT_ENCODING_PLAIN;
  case contract::AssetPlacementEncoding::HolderAssetV1: return HOLDER_ASSET_PLACEMENT_ENCODING_HOLDER_ASSET_V1;
  }
  throw std::logic_error("not a contract::AssetPlacementEncoding value");
}

holder_project_privacy_mode to_c_enum(contract::ProjectPrivacyMode value) {
  switch (value) {
  case contract::ProjectPrivacyMode::EncryptedGit: return HOLDER_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT;
  case contract::ProjectPrivacyMode::Plain: return HOLDER_PROJECT_PRIVACY_MODE_PLAIN;
  }
  throw std::logic_error("not a contract::ProjectPrivacyMode value");
}

holder_project_encryption_check_privacy_mode to_c_enum(contract::ProjectEncryptionCheckPrivacyMode value) {
  switch (value) {
  case contract::ProjectEncryptionCheckPrivacyMode::EncryptedGit: return HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_ENCRYPTED_GIT;
  case contract::ProjectEncryptionCheckPrivacyMode::Plain: return HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_PLAIN;
  }
  throw std::logic_error("not a contract::ProjectEncryptionCheckPrivacyMode value");
}

holder_recovery_token_global_import_result_pull_status to_c_enum(contract::RecoveryTokenGlobalImportResultPullStatus value) {
  switch (value) {
  case contract::RecoveryTokenGlobalImportResultPullStatus::NotAttempted: return HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_NOT_ATTEMPTED;
  case contract::RecoveryTokenGlobalImportResultPullStatus::Succeeded: return HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED;
  case contract::RecoveryTokenGlobalImportResultPullStatus::Failed: return HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_FAILED;
  }
  throw std::logic_error("not a contract::RecoveryTokenGlobalImportResultPullStatus value");
}

holder_pull_phase_status to_c_enum(contract::PullPhaseStatus value) {
  switch (value) {
  case contract::PullPhaseStatus::NotAttempted: return HOLDER_PULL_PHASE_STATUS_NOT_ATTEMPTED;
  case contract::PullPhaseStatus::Succeeded: return HOLDER_PULL_PHASE_STATUS_SUCCEEDED;
  case contract::PullPhaseStatus::RemoteUnset: return HOLDER_PULL_PHASE_STATUS_REMOTE_UNSET;
  case contract::PullPhaseStatus::Failed: return HOLDER_PULL_PHASE_STATUS_FAILED;
  }
  throw std::logic_error("not a contract::PullPhaseStatus value");
}

holder_push_phase_status to_c_enum(contract::PushPhaseStatus value) {
  switch (value) {
  case contract::PushPhaseStatus::NotAttempted: return HOLDER_PUSH_PHASE_STATUS_NOT_ATTEMPTED;
  case contract::PushPhaseStatus::Pushed: return HOLDER_PUSH_PHASE_STATUS_PUSHED;
  case contract::PushPhaseStatus::UpToDate: return HOLDER_PUSH_PHASE_STATUS_UP_TO_DATE;
  case contract::PushPhaseStatus::AuthFailed: return HOLDER_PUSH_PHASE_STATUS_AUTH_FAILED;
  case contract::PushPhaseStatus::NotFound: return HOLDER_PUSH_PHASE_STATUS_NOT_FOUND;
  case contract::PushPhaseStatus::NetworkError: return HOLDER_PUSH_PHASE_STATUS_NETWORK_ERROR;
  case contract::PushPhaseStatus::NonFastForward: return HOLDER_PUSH_PHASE_STATUS_NON_FAST_FORWARD;
  case contract::PushPhaseStatus::RemoteUnset: return HOLDER_PUSH_PHASE_STATUS_REMOTE_UNSET;
  case contract::PushPhaseStatus::UnknownError: return HOLDER_PUSH_PHASE_STATUS_UNKNOWN_ERROR;
  }
  throw std::logic_error("not a contract::PushPhaseStatus value");
}

holder_sync_result_status to_c_enum(contract::SyncResultStatus value) {
  switch (value) {
  case contract::SyncResultStatus::Succeeded: return HOLDER_SYNC_RESULT_STATUS_SUCCEEDED;
  case contract::SyncResultStatus::Failed: return HOLDER_SYNC_RESULT_STATUS_FAILED;
  }
  throw std::logic_error("not a contract::SyncResultStatus value");
}

holder_remote_test_result_status to_c_enum(contract::RemoteTestResultStatus value) {
  switch (value) {
  case contract::RemoteTestResultStatus::Reachable: return HOLDER_REMOTE_TEST_RESULT_STATUS_REACHABLE;
  case contract::RemoteTestResultStatus::AuthFailed: return HOLDER_REMOTE_TEST_RESULT_STATUS_AUTH_FAILED;
  case contract::RemoteTestResultStatus::NotFound: return HOLDER_REMOTE_TEST_RESULT_STATUS_NOT_FOUND;
  case contract::RemoteTestResultStatus::NetworkError: return HOLDER_REMOTE_TEST_RESULT_STATUS_NETWORK_ERROR;
  case contract::RemoteTestResultStatus::InvalidRemoteUrl: return HOLDER_REMOTE_TEST_RESULT_STATUS_INVALID_REMOTE_URL;
  case contract::RemoteTestResultStatus::RemoteUnset: return HOLDER_REMOTE_TEST_RESULT_STATUS_REMOTE_UNSET;
  case contract::RemoteTestResultStatus::UnknownError: return HOLDER_REMOTE_TEST_RESULT_STATUS_UNKNOWN_ERROR;
  }
  throw std::logic_error("not a contract::RemoteTestResultStatus value");
}

holder_history_affected_object_kind to_c_enum(contract::HistoryAffectedObjectKind value) {
  switch (value) {
  case contract::HistoryAffectedObjectKind::Card: return HOLDER_HISTORY_AFFECTED_OBJECT_KIND_CARD;
  case contract::HistoryAffectedObjectKind::Resource: return HOLDER_HISTORY_AFFECTED_OBJECT_KIND_RESOURCE;
  case contract::HistoryAffectedObjectKind::Location: return HOLDER_HISTORY_AFFECTED_OBJECT_KIND_LOCATION;
  case contract::HistoryAffectedObjectKind::AiData: return HOLDER_HISTORY_AFFECTED_OBJECT_KIND_AI_DATA;
  case contract::HistoryAffectedObjectKind::ProjectSettings: return HOLDER_HISTORY_AFFECTED_OBJECT_KIND_PROJECT_SETTINGS;
  case contract::HistoryAffectedObjectKind::Unknown: return HOLDER_HISTORY_AFFECTED_OBJECT_KIND_UNKNOWN;
  }
  throw std::logic_error("not a contract::HistoryAffectedObjectKind value");
}

holder_trash_item_type to_c_enum(contract::TrashItemType value) {
  switch (value) {
  case contract::TrashItemType::Card: return HOLDER_TRASH_ITEM_TYPE_CARD;
  case contract::TrashItemType::AiMessage: return HOLDER_TRASH_ITEM_TYPE_AI_MESSAGE;
  }
  throw std::logic_error("not a contract::TrashItemType value");
}

holder_project_storage_location to_c(holder_arena* arena, const contract::ProjectStorageLocation& value);
holder_location_delete_result to_c(holder_arena* arena, const contract::LocationDeleteResult& value);
holder_tag_occurrence to_c(holder_arena* arena, const contract::TagOccurrence& value);
holder_tag_occurrence_list to_c_list(holder_arena* arena, const std::vector<contract::TagOccurrence>& values);
holder_card to_c(holder_arena* arena, const contract::Card& value);
holder_card_metadata to_c(holder_arena* arena, const contract::CardMetadata& value);
holder_card_metadata_list to_c_list(holder_arena* arena, const std::vector<contract::CardMetadata>& values);
holder_card_breadcrumb to_c(holder_arena* arena, const contract::CardBreadcrumb& value);
holder_card_breadcrumb_list to_c_list(holder_arena* arena, const std::vector<contract::CardBreadcrumb>& values);
holder_card_reference_candidate to_c(holder_arena* arena, const contract::CardReferenceCandidate& value);
holder_card_reference_candidate_list to_c_list(holder_arena* arena, const std::vector<contract::CardReferenceCandidate>& values);
holder_card_reference_resolve_data to_c(holder_arena* arena, const contract::CardReferenceResolveData& value);
holder_card_list_item to_c(holder_arena* arena, const contract::CardListItem& value);
holder_card_list_item_list to_c_list(holder_arena* arena, const std::vector<contract::CardListItem>& values);
holder_card_list_item_page to_c(holder_arena* arena, const contract::Page<contract::CardListItem>& value);
holder_tag_count to_c(holder_arena* arena, const contract::TagCount& value);
holder_tag_count_list to_c_list(holder_arena* arena, const std::vector<contract::TagCount>& values);
holder_link to_c(holder_arena* arena, const contract::Link& value);
holder_link_list to_c_list(holder_arena* arena, const std::vector<contract::Link>& values);
holder_milestone_date to_c(holder_arena* arena, const contract::MilestoneDate& value);
holder_milestone_date_list to_c_list(holder_arena* arena, const std::vector<contract::MilestoneDate>& values);
holder_milestone to_c(holder_arena* arena, const contract::Milestone& value);
holder_milestone_list to_c_list(holder_arena* arena, const std::vector<contract::Milestone>& values);
holder_calendar_milestone to_c(holder_arena* arena, const contract::CalendarMilestone& value);
holder_calendar_milestone_list to_c_list(holder_arena* arena, const std::vector<contract::CalendarMilestone>& values);
holder_calendar to_c(holder_arena* arena, const contract::Calendar& value);
holder_asset_placement to_c(holder_arena* arena, const contract::AssetPlacement& value);
holder_asset_placement_list to_c_list(holder_arena* arena, const std::vector<contract::AssetPlacement>& values);
holder_resource_asset to_c(holder_arena* arena, const contract::ResourceAsset& value);
holder_resource_asset_list to_c_list(holder_arena* arena, const std::vector<contract::ResourceAsset>& values);
holder_resource to_c(holder_arena* arena, const contract::Resource& value);
holder_resource_list to_c_list(holder_arena* arena, const std::vector<contract::Resource>& values);
holder_project_list_item to_c(holder_arena* arena, const contract::ProjectListItem& value);
holder_project_list_item_list to_c_list(holder_arena* arena, const std::vector<contract::ProjectListItem>& values);
holder_project to_c(holder_arena* arena, const contract::Project& value);
holder_encryption_safety_check to_c(holder_arena* arena, const contract::EncryptionSafetyCheck& value);
holder_project_encryption_check to_c(holder_arena* arena, const contract::ProjectEncryptionCheck& value);
holder_hierarchy_repair_move to_c(holder_arena* arena, const contract::HierarchyRepairMove& value);
holder_hierarchy_repair_move_list to_c_list(holder_arena* arena, const std::vector<contract::HierarchyRepairMove>& values);
holder_hierarchy_repair to_c(holder_arena* arena, const contract::HierarchyRepair& value);
holder_rebuild_result to_c(holder_arena* arena, const contract::RebuildResult& value);
holder_reindex_result to_c(holder_arena* arena, const contract::ReindexResult& value);
holder_project_recovery_token_export_result to_c(holder_arena* arena, const contract::ProjectRecoveryTokenExportResult& value);
holder_project_recovery_token_import_result to_c(holder_arena* arena, const contract::ProjectRecoveryTokenImportResult& value);
holder_recovery_token_global_import_result to_c(holder_arena* arena, const contract::RecoveryTokenGlobalImportResult& value);
holder_sync_state to_c(holder_arena* arena, const contract::SyncState& value);
holder_pull_phase to_c(holder_arena* arena, const contract::PullPhase& value);
holder_push_phase to_c(holder_arena* arena, const contract::PushPhase& value);
holder_sync_result to_c(holder_arena* arena, const contract::SyncResult& value);
holder_due_sync_result to_c(holder_arena* arena, const contract::DueSyncResult& value);
holder_remote_test_result to_c(holder_arena* arena, const contract::RemoteTestResult& value);
holder_history_author to_c(holder_arena* arena, const contract::HistoryAuthor& value);
holder_history_affected_path to_c(holder_arena* arena, const contract::HistoryAffectedPath& value);
holder_history_affected_path_list to_c_list(holder_arena* arena, const std::vector<contract::HistoryAffectedPath>& values);
holder_history_affected_object to_c(holder_arena* arena, const contract::HistoryAffectedObject& value);
holder_history_affected_object_list to_c_list(holder_arena* arena, const std::vector<contract::HistoryAffectedObject>& values);
holder_project_activity to_c(holder_arena* arena, const contract::ProjectActivity& value);
holder_project_activity_list to_c_list(holder_arena* arena, const std::vector<contract::ProjectActivity>& values);
holder_project_history_page to_c(holder_arena* arena, const contract::ProjectHistoryPage& value);
holder_card_history_save to_c(holder_arena* arena, const contract::CardHistorySave& value);
holder_card_history_save_list to_c_list(holder_arena* arena, const std::vector<contract::CardHistorySave>& values);
holder_card_history_entry to_c(holder_arena* arena, const contract::CardHistoryEntry& value);
holder_card_history_entry_list to_c_list(holder_arena* arena, const std::vector<contract::CardHistoryEntry>& values);
holder_card_history_page to_c(holder_arena* arena, const contract::CardHistoryPage& value);
holder_card_history_version to_c(holder_arena* arena, const contract::CardHistoryVersion& value);
holder_card_history_diff_line to_c(holder_arena* arena, const contract::CardHistoryDiffLine& value);
holder_card_history_diff_line_list to_c_list(holder_arena* arena, const std::vector<contract::CardHistoryDiffLine>& values);
holder_card_history_comparison to_c(holder_arena* arena, const contract::CardHistoryComparison& value);
holder_card_history_snapshot to_c(holder_arena* arena, const contract::CardHistorySnapshot& value);
holder_card_history_restore_result to_c(holder_arena* arena, const contract::CardHistoryRestoreResult& value);
holder_trash_item to_c(holder_arena* arena, const contract::TrashItem& value);
holder_trash_item_list to_c_list(holder_arena* arena, const std::vector<contract::TrashItem>& values);
holder_trash_empty_result to_c(holder_arena* arena, const contract::TrashEmptyResult& value);
holder_trash_delete_result to_c(holder_arena* arena, const contract::TrashDeleteResult& value);
holder_search_card to_c(holder_arena* arena, const contract::SearchCard& value);
holder_search_card_list to_c_list(holder_arena* arena, const std::vector<contract::SearchCard>& values);
holder_search_message to_c(holder_arena* arena, const contract::SearchMessage& value);
holder_search_message_list to_c_list(holder_arena* arena, const std::vector<contract::SearchMessage>& values);

holder_project_storage_location to_c(holder_arena* arena, const contract::ProjectStorageLocation& value) {
  holder_project_storage_location out{};
  out.location_id = copy(arena, value.location_id);
  out.project_id = copy(arena, value.project_id);
  out.name = copy(arena, value.name);
  out.provider = to_c_enum(value.provider);
  out.configuration = to_c_location_configuration(arena, value.configuration);
  out.bound = value.bound;
  out.binding_preview = copy_text(arena, value.binding_preview);
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  return out;
}

holder_location_delete_result to_c(holder_arena* arena, const contract::LocationDeleteResult& value) {
  holder_location_delete_result out{};
  out.location_id = copy(arena, value.location_id);
  return out;
}

holder_tag_occurrence to_c(holder_arena* arena, const contract::TagOccurrence& value) {
  holder_tag_occurrence out{};
  out.tag = copy(arena, value.tag);
  out.byte_start = value.byte_start;
  out.byte_end = value.byte_end;
  return out;
}

holder_tag_occurrence_list to_c_list(holder_arena* arena, const std::vector<contract::TagOccurrence>& values) {
  auto* items = make<holder_tag_occurrence>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card to_c(holder_arena* arena, const contract::Card& value) {
  holder_card out{};
  out.card_id = copy(arena, value.card_id);
  out.project_id = copy(arena, value.project_id);
  out.title = copy(arena, value.title);
  out.rel_path = copy(arena, value.rel_path);
  out.parent_card_id = copy_text(arena, value.parent_card_id);
  out.sort_key = value.sort_key;
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  out.deleted_at = copy_value<int64_t>(arena, value.deleted_at);
  out.content = copy(arena, value.content);
  out.tags = copy_strings(arena, value.tags);
  out.tag_occurrences = to_c_list(arena, value.tag_occurrences);
  return out;
}

holder_card_metadata to_c(holder_arena* arena, const contract::CardMetadata& value) {
  holder_card_metadata out{};
  out.card_id = copy(arena, value.card_id);
  out.project_id = copy(arena, value.project_id);
  out.title = copy(arena, value.title);
  out.rel_path = copy(arena, value.rel_path);
  out.parent_card_id = copy_text(arena, value.parent_card_id);
  out.sort_key = value.sort_key;
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  out.deleted_at = copy_value<int64_t>(arena, value.deleted_at);
  return out;
}

holder_card_metadata_list to_c_list(holder_arena* arena, const std::vector<contract::CardMetadata>& values) {
  auto* items = make<holder_card_metadata>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card_breadcrumb to_c(holder_arena* arena, const contract::CardBreadcrumb& value) {
  holder_card_breadcrumb out{};
  out.kind = to_c_enum(value.kind);
  out.id = copy(arena, value.id);
  out.title = copy(arena, value.title);
  return out;
}

holder_card_breadcrumb_list to_c_list(holder_arena* arena, const std::vector<contract::CardBreadcrumb>& values) {
  auto* items = make<holder_card_breadcrumb>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card_reference_candidate to_c(holder_arena* arena, const contract::CardReferenceCandidate& value) {
  holder_card_reference_candidate out{};
  out.card_id = copy(arena, value.card_id);
  out.title = copy(arena, value.title);
  out.deleted_at = copy_value<int64_t>(arena, value.deleted_at);
  return out;
}

holder_card_reference_candidate_list to_c_list(holder_arena* arena, const std::vector<contract::CardReferenceCandidate>& values) {
  auto* items = make<holder_card_reference_candidate>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card_reference_resolve_data to_c(holder_arena* arena, const contract::CardReferenceResolveData& value) {
  holder_card_reference_resolve_data out{};
  out.status = to_c_enum(value.status);
  out.match_kind = copy_text(arena, value.match_kind);
  out.card = copy_optional<holder_card_reference_candidate>(arena, value.card, [&](const auto& item) { return to_c(arena, item); });
  out.candidates = copy_optional<holder_card_reference_candidate_list>(arena, value.candidates, [&](const auto& items) { return to_c_list(arena, items); });
  return out;
}

holder_card_list_item to_c(holder_arena* arena, const contract::CardListItem& value) {
  holder_card_list_item out{};
  out.card_id = copy(arena, value.card_id);
  out.project_id = copy(arena, value.project_id);
  out.title = copy(arena, value.title);
  out.rel_path = copy(arena, value.rel_path);
  out.parent_card_id = copy_text(arena, value.parent_card_id);
  out.sort_key = value.sort_key;
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  out.deleted_at = copy_value<int64_t>(arena, value.deleted_at);
  out.child_count = copy_value<int32_t>(arena, value.child_count);
  out.content = copy_text(arena, value.content);
  return out;
}

holder_card_list_item_list to_c_list(holder_arena* arena, const std::vector<contract::CardListItem>& values) {
  auto* items = make<holder_card_list_item>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card_list_item_page to_c(holder_arena* arena, const contract::Page<contract::CardListItem>& value) {
  holder_card_list_item_page out{};
  out.items = to_c_list(arena, value.items);
  out.next_cursor = copy_text(arena, value.next_cursor);
  return out;
}

holder_tag_count to_c(holder_arena* arena, const contract::TagCount& value) {
  holder_tag_count out{};
  out.tag = copy(arena, value.tag);
  out.card_count = value.card_count;
  return out;
}

holder_tag_count_list to_c_list(holder_arena* arena, const std::vector<contract::TagCount>& values) {
  auto* items = make<holder_tag_count>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_link to_c(holder_arena* arena, const contract::Link& value) {
  holder_link out{};
  out.source_project_id = copy(arena, value.source_project_id);
  out.source_type = to_c_enum(value.source_type);
  out.source_id = copy(arena, value.source_id);
  out.target_project_id = copy(arena, value.target_project_id);
  out.target_type = to_c_enum(value.target_type);
  out.target_id = copy(arena, value.target_id);
  out.kind = copy(arena, value.kind);
  out.label = copy_text(arena, value.label);
  out.created_at = value.created_at;
  return out;
}

holder_link_list to_c_list(holder_arena* arena, const std::vector<contract::Link>& values) {
  auto* items = make<holder_link>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_milestone_date to_c(holder_arena* arena, const contract::MilestoneDate& value) {
  holder_milestone_date out{};
  out.role = copy(arena, value.role);
  out.date = copy_text(arena, value.date);
  out.datetime = copy_text(arena, value.datetime);
  return out;
}

holder_milestone_date_list to_c_list(holder_arena* arena, const std::vector<contract::MilestoneDate>& values) {
  auto* items = make<holder_milestone_date>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_milestone to_c(holder_arena* arena, const contract::Milestone& value) {
  holder_milestone out{};
  out.milestone_id = copy(arena, value.milestone_id);
  out.project_id = copy(arena, value.project_id);
  out.card_id = copy(arena, value.card_id);
  out.dates = to_c_list(arena, value.dates);
  out.kind = copy_text(arena, value.kind);
  out.description = copy_text(arena, value.description);
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  return out;
}

holder_milestone_list to_c_list(holder_arena* arena, const std::vector<contract::Milestone>& values) {
  auto* items = make<holder_milestone>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_calendar_milestone to_c(holder_arena* arena, const contract::CalendarMilestone& value) {
  holder_calendar_milestone out{};
  out.milestone_id = copy(arena, value.milestone_id);
  out.project_id = copy(arena, value.project_id);
  out.card_id = copy(arena, value.card_id);
  out.dates = to_c_list(arena, value.dates);
  out.kind = copy_text(arena, value.kind);
  out.description = copy_text(arena, value.description);
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  out.card_title = copy(arena, value.card_title);
  return out;
}

holder_calendar_milestone_list to_c_list(holder_arena* arena, const std::vector<contract::CalendarMilestone>& values) {
  auto* items = make<holder_calendar_milestone>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_calendar to_c(holder_arena* arena, const contract::Calendar& value) {
  holder_calendar out{};
  out.milestones = to_c_list(arena, value.milestones);
  out.created_cards = to_c_list(arena, value.created_cards);
  out.updated_cards = to_c_list(arena, value.updated_cards);
  return out;
}

holder_asset_placement to_c(holder_arena* arena, const contract::AssetPlacement& value) {
  holder_asset_placement out{};
  out.placement_id = copy(arena, value.placement_id);
  out.location_id = copy(arena, value.location_id);
  out.encoding = to_c_enum(value.encoding);
  out.stored_byte_size = value.stored_byte_size;
  out.created_at = value.created_at;
  return out;
}

holder_asset_placement_list to_c_list(holder_arena* arena, const std::vector<contract::AssetPlacement>& values) {
  auto* items = make<holder_asset_placement>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_resource_asset to_c(holder_arena* arena, const contract::ResourceAsset& value) {
  holder_resource_asset out{};
  out.asset_id = copy(arena, value.asset_id);
  out.original_filename = copy(arena, value.original_filename);
  out.media_type = copy(arena, value.media_type);
  out.byte_size = value.byte_size;
  out.plaintext_sha256 = copy(arena, value.plaintext_sha256);
  out.placements = to_c_list(arena, value.placements);
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  return out;
}

holder_resource_asset_list to_c_list(holder_arena* arena, const std::vector<contract::ResourceAsset>& values) {
  auto* items = make<holder_resource_asset>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_resource to_c(holder_arena* arena, const contract::Resource& value) {
  holder_resource out{};
  out.resource_id = copy(arena, value.resource_id);
  out.project_id = copy(arena, value.project_id);
  out.type = copy(arena, value.type);
  out.label = copy(arena, value.label);
  out.metadata = to_c_resource_metadata(arena, value.metadata);
  out.assets = to_c_list(arena, value.assets);
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  return out;
}

holder_resource_list to_c_list(holder_arena* arena, const std::vector<contract::Resource>& values) {
  auto* items = make<holder_resource>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_project_list_item to_c(holder_arena* arena, const contract::ProjectListItem& value) {
  holder_project_list_item out{};
  out.project_id = copy(arena, value.project_id);
  out.name = copy(arena, value.name);
  out.root_path = copy(arena, value.root_path);
  out.git_remote_url = copy_text(arena, value.git_remote_url);
  out.git_provider = copy_text(arena, value.git_provider);
  out.privacy_mode = to_c_enum(value.privacy_mode);
  out.project_key_id = copy_text(arena, value.project_key_id);
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  out.card_count = copy_value<int32_t>(arena, value.card_count);
  out.root_card_count = copy_value<int32_t>(arena, value.root_card_count);
  return out;
}

holder_project_list_item_list to_c_list(holder_arena* arena, const std::vector<contract::ProjectListItem>& values) {
  auto* items = make<holder_project_list_item>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_project to_c(holder_arena* arena, const contract::Project& value) {
  holder_project out{};
  out.project_id = copy(arena, value.project_id);
  out.name = copy(arena, value.name);
  out.root_path = copy(arena, value.root_path);
  out.git_remote_url = copy_text(arena, value.git_remote_url);
  out.git_provider = copy_text(arena, value.git_provider);
  out.privacy_mode = to_c_enum(value.privacy_mode);
  out.project_key_id = copy_text(arena, value.project_key_id);
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  return out;
}

holder_encryption_safety_check to_c(holder_arena* arena, const contract::EncryptionSafetyCheck& value) {
  holder_encryption_safety_check out{};
  out.ok = value.ok;
  out.checked_files = value.checked_files;
  out.unsafe_files = value.unsafe_files;
  out.unsafe_paths = copy_strings(arena, value.unsafe_paths);
  out.message = copy(arena, value.message);
  return out;
}

holder_project_encryption_check to_c(holder_arena* arena, const contract::ProjectEncryptionCheck& value) {
  holder_project_encryption_check out{};
  out.project_id = copy(arena, value.project_id);
  out.privacy_mode = to_c_enum(value.privacy_mode);
  out.check = to_c(arena, value.check);
  return out;
}

holder_hierarchy_repair_move to_c(holder_arena* arena, const contract::HierarchyRepairMove& value) {
  holder_hierarchy_repair_move out{};
  out.card_id = copy(arena, value.card_id);
  out.parent_card_id = copy_text(arena, value.parent_card_id);
  return out;
}

holder_hierarchy_repair_move_list to_c_list(holder_arena* arena, const std::vector<contract::HierarchyRepairMove>& values) {
  auto* items = make<holder_hierarchy_repair_move>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_hierarchy_repair to_c(holder_arena* arena, const contract::HierarchyRepair& value) {
  holder_hierarchy_repair out{};
  out.applied = value.applied;
  out.moved = to_c_list(arena, value.moved);
  out.changed_card_count = value.changed_card_count;
  out.cycle_card_ids = copy_strings(arena, value.cycle_card_ids);
  return out;
}

holder_rebuild_result to_c(holder_arena* arena, const contract::RebuildResult& value) {
  holder_rebuild_result out{};
  out.project_id = copy(arena, value.project_id);
  out.cards = value.cards;
  out.ai_messages = value.ai_messages;
  out.ai_threads = value.ai_threads;
  out.links = value.links;
  return out;
}

holder_reindex_result to_c(holder_arena* arena, const contract::ReindexResult& value) {
  holder_reindex_result out{};
  out.message = copy(arena, value.message);
  return out;
}

holder_project_recovery_token_export_result to_c(holder_arena* arena, const contract::ProjectRecoveryTokenExportResult& value) {
  holder_project_recovery_token_export_result out{};
  out.project_id = copy(arena, value.project_id);
  out.key_id = copy(arena, value.key_id);
  out.recovery_token = copy(arena, value.recovery_token);
  return out;
}

holder_project_recovery_token_import_result to_c(holder_arena* arena, const contract::ProjectRecoveryTokenImportResult& value) {
  holder_project_recovery_token_import_result out{};
  out.project_id = copy(arena, value.project_id);
  return out;
}

holder_recovery_token_global_import_result to_c(holder_arena* arena, const contract::RecoveryTokenGlobalImportResult& value) {
  holder_recovery_token_global_import_result out{};
  out.project_id = copy(arena, value.project_id);
  out.project_created = value.project_created;
  out.remote_hint_present = value.remote_hint_present;
  out.remote_configured = value.remote_configured;
  out.remote_error = copy_text(arena, value.remote_error);
  out.pull_status = to_c_enum(value.pull_status);
  out.pull_error = copy_text(arena, value.pull_error);
  return out;
}

holder_sync_state to_c(holder_arena* arena, const contract::SyncState& value) {
  holder_sync_state out{};
  out.project_id = copy(arena, value.project_id);
  out.last_commit_at = copy_value<int64_t>(arena, value.last_commit_at);
  out.last_push_at = copy_value<int64_t>(arena, value.last_push_at);
  out.last_pull_at = copy_value<int64_t>(arena, value.last_pull_at);
  out.uncommitted_changes_count = value.uncommitted_changes_count;
  out.unpushed_commits_count = value.unpushed_commits_count;
  out.last_push_status = copy_text(arena, value.last_push_status);
  out.last_pull_status = copy_text(arena, value.last_pull_status);
  out.last_sync_error = copy_text(arena, value.last_sync_error);
  out.last_sync_error_at = copy_value<int64_t>(arena, value.last_sync_error_at);
  out.retry_count = value.retry_count;
  out.next_retry_at = copy_value<int64_t>(arena, value.next_retry_at);
  out.pull_retry_count = value.pull_retry_count;
  out.next_pull_retry_at = copy_value<int64_t>(arena, value.next_pull_retry_at);
  out.updated_at = copy_value<int64_t>(arena, value.updated_at);
  return out;
}

holder_pull_phase to_c(holder_arena* arena, const contract::PullPhase& value) {
  holder_pull_phase out{};
  out.attempted = value.attempted;
  out.status = to_c_enum(value.status);
  out.conflicts_resolved = value.conflicts_resolved;
  out.hidden_cards_repaired = value.hidden_cards_repaired;
  out.error_message = copy_text(arena, value.error_message);
  return out;
}

holder_push_phase to_c(holder_arena* arena, const contract::PushPhase& value) {
  holder_push_phase out{};
  out.attempted = value.attempted;
  out.status = to_c_enum(value.status);
  out.ahead_count = value.ahead_count;
  out.behind_count = value.behind_count;
  out.local_head_commit = copy_text(arena, value.local_head_commit);
  out.error_message = copy_text(arena, value.error_message);
  return out;
}

holder_sync_result to_c(holder_arena* arena, const contract::SyncResult& value) {
  holder_sync_result out{};
  out.project_id = copy(arena, value.project_id);
  out.status = to_c_enum(value.status);
  out.remote_url = copy_text(arena, value.remote_url);
  out.pull = to_c(arena, value.pull);
  out.push = to_c(arena, value.push);
  return out;
}

holder_due_sync_result to_c(holder_arena* arena, const contract::DueSyncResult& value) {
  holder_due_sync_result out{};
  out.project_id = copy(arena, value.project_id);
  out.remote_url = copy_text(arena, value.remote_url);
  out.pull = to_c(arena, value.pull);
  out.push = to_c(arena, value.push);
  return out;
}

holder_remote_test_result to_c(holder_arena* arena, const contract::RemoteTestResult& value) {
  holder_remote_test_result out{};
  out.project_id = copy(arena, value.project_id);
  out.status = to_c_enum(value.status);
  out.remote_url = copy_text(arena, value.remote_url);
  out.remote_has_head = value.remote_has_head;
  out.error_message = copy_text(arena, value.error_message);
  return out;
}

holder_history_author to_c(holder_arena* arena, const contract::HistoryAuthor& value) {
  holder_history_author out{};
  out.name = copy(arena, value.name);
  out.email = copy(arena, value.email);
  return out;
}

holder_history_affected_path to_c(holder_arena* arena, const contract::HistoryAffectedPath& value) {
  holder_history_affected_path out{};
  out.path = copy(arena, value.path);
  out.title = copy_text(arena, value.title);
  out.detail = copy_text(arena, value.detail);
  return out;
}

holder_history_affected_path_list to_c_list(holder_arena* arena, const std::vector<contract::HistoryAffectedPath>& values) {
  auto* items = make<holder_history_affected_path>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_history_affected_object to_c(holder_arena* arena, const contract::HistoryAffectedObject& value) {
  holder_history_affected_object out{};
  out.kind = to_c_enum(value.kind);
  out.paths = copy_strings(arena, value.paths);
  out.items = to_c_list(arena, value.items);
  return out;
}

holder_history_affected_object_list to_c_list(holder_arena* arena, const std::vector<contract::HistoryAffectedObject>& values) {
  auto* items = make<holder_history_affected_object>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_project_activity to_c(holder_arena* arena, const contract::ProjectActivity& value) {
  holder_project_activity out{};
  out.oid = copy(arena, value.oid);
  out.parent_oids = copy_strings(arena, value.parent_oids);
  out.author = to_c(arena, value.author);
  out.authored_at = value.authored_at;
  out.committed_at = value.committed_at;
  out.message = copy(arena, value.message);
  out.affected_objects = to_c_list(arena, value.affected_objects);
  out.is_merge = value.is_merge;
  return out;
}

holder_project_activity_list to_c_list(holder_arena* arena, const std::vector<contract::ProjectActivity>& values) {
  auto* items = make<holder_project_activity>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_project_history_page to_c(holder_arena* arena, const contract::ProjectHistoryPage& value) {
  holder_project_history_page out{};
  out.head_oid = copy_text(arena, value.head_oid);
  out.activities = to_c_list(arena, value.activities);
  out.next_cursor = copy_text(arena, value.next_cursor);
  out.scan_limited = value.scan_limited;
  return out;
}

holder_card_history_save to_c(holder_arena* arena, const contract::CardHistorySave& value) {
  holder_card_history_save out{};
  out.oid = copy(arena, value.oid);
  out.parent_oids = copy_strings(arena, value.parent_oids);
  out.authored_at = value.authored_at;
  out.committed_at = value.committed_at;
  out.message = copy(arena, value.message);
  return out;
}

holder_card_history_save_list to_c_list(holder_arena* arena, const std::vector<contract::CardHistorySave>& values) {
  auto* items = make<holder_card_history_save>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card_history_entry to_c(holder_arena* arena, const contract::CardHistoryEntry& value) {
  holder_card_history_entry out{};
  out.first_oid = copy(arena, value.first_oid);
  out.last_oid = copy(arena, value.last_oid);
  out.parent_oids = copy_strings(arena, value.parent_oids);
  out.visible_parent_oids = copy_strings(arena, value.visible_parent_oids);
  out.author = to_c(arena, value.author);
  out.started_at = value.started_at;
  out.ended_at = value.ended_at;
  out.kind = copy(arena, value.kind);
  out.summary = copy(arena, value.summary);
  out.commit_count = value.commit_count;
  out.is_merge = value.is_merge;
  out.saves = to_c_list(arena, value.saves);
  return out;
}

holder_card_history_entry_list to_c_list(holder_arena* arena, const std::vector<contract::CardHistoryEntry>& values) {
  auto* items = make<holder_card_history_entry>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card_history_page to_c(holder_arena* arena, const contract::CardHistoryPage& value) {
  holder_card_history_page out{};
  out.head_oid = copy_text(arena, value.head_oid);
  out.entries = to_c_list(arena, value.entries);
  out.next_cursor = copy_text(arena, value.next_cursor);
  out.scan_limited = value.scan_limited;
  return out;
}

holder_card_history_version to_c(holder_arena* arena, const contract::CardHistoryVersion& value) {
  holder_card_history_version out{};
  out.exists = value.exists;
  out.oid = copy(arena, value.oid);
  out.title = copy(arena, value.title);
  out.body = copy(arena, value.body);
  return out;
}

holder_card_history_diff_line to_c(holder_arena* arena, const contract::CardHistoryDiffLine& value) {
  holder_card_history_diff_line out{};
  out.origin = copy(arena, value.origin);
  out.text = copy(arena, value.text);
  out.old_line = copy_value<int64_t>(arena, value.old_line);
  out.new_line = copy_value<int64_t>(arena, value.new_line);
  return out;
}

holder_card_history_diff_line_list to_c_list(holder_arena* arena, const std::vector<contract::CardHistoryDiffLine>& values) {
  auto* items = make<holder_card_history_diff_line>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card_history_comparison to_c(holder_arena* arena, const contract::CardHistoryComparison& value) {
  holder_card_history_comparison out{};
  out.before = to_c(arena, value.before);
  out.after = to_c(arena, value.after);
  out.summary = copy(arena, value.summary);
  out.lines = to_c_list(arena, value.lines);
  out.truncated = value.truncated;
  return out;
}

holder_card_history_snapshot to_c(holder_arena* arena, const contract::CardHistorySnapshot& value) {
  holder_card_history_snapshot out{};
  out.card_id = copy(arena, value.card_id);
  out.snapshot = to_c(arena, value.snapshot);
  return out;
}

holder_card_history_restore_result to_c(holder_arena* arena, const contract::CardHistoryRestoreResult& value) {
  holder_card_history_restore_result out{};
  out.card_id = copy(arena, value.card_id);
  out.restored_from_oid = copy(arena, value.restored_from_oid);
  out.result_oid = copy(arena, value.result_oid);
  out.title = copy(arena, value.title);
  out.updated_at = value.updated_at;
  out.deleted_at = copy_value<int64_t>(arena, value.deleted_at);
  return out;
}

holder_trash_item to_c(holder_arena* arena, const contract::TrashItem& value) {
  holder_trash_item out{};
  out.type = to_c_enum(value.type);
  out.project_id = copy(arena, value.project_id);
  out.card_id = copy_text(arena, value.card_id);
  out.message_id = copy_text(arena, value.message_id);
  out.thread_id = copy_text(arena, value.thread_id);
  out.title = copy_text(arena, value.title);
  out.role = copy_text(arena, value.role);
  out.deleted_at = value.deleted_at;
  out.created_at = copy_value<int64_t>(arena, value.created_at);
  out.rel_path = copy_text(arena, value.rel_path);
  return out;
}

holder_trash_item_list to_c_list(holder_arena* arena, const std::vector<contract::TrashItem>& values) {
  auto* items = make<holder_trash_item>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_trash_empty_result to_c(holder_arena* arena, const contract::TrashEmptyResult& value) {
  holder_trash_empty_result out{};
  out.project_id = copy(arena, value.project_id);
  return out;
}

holder_trash_delete_result to_c(holder_arena* arena, const contract::TrashDeleteResult& value) {
  holder_trash_delete_result out{};
  out.card_id = copy_text(arena, value.card_id);
  out.message_id = copy_text(arena, value.message_id);
  return out;
}

holder_search_card to_c(holder_arena* arena, const contract::SearchCard& value) {
  holder_search_card out{};
  out.card_id = copy(arena, value.card_id);
  out.title = copy(arena, value.title);
  out.updated_at = value.updated_at;
  out.created_at = value.created_at;
  out.snippet = copy(arena, value.snippet);
  out.rank = value.rank;
  return out;
}

holder_search_card_list to_c_list(holder_arena* arena, const std::vector<contract::SearchCard>& values) {
  auto* items = make<holder_search_card>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_search_message to_c(holder_arena* arena, const contract::SearchMessage& value) {
  holder_search_message out{};
  out.message_id = copy(arena, value.message_id);
  out.created_at = value.created_at;
  out.snippet = copy(arena, value.snippet);
  out.rank = value.rank;
  return out;
}

holder_search_message_list to_c_list(holder_arena* arena, const std::vector<contract::SearchMessage>& values) {
  auto* items = make<holder_search_message>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

contract::NewStorageLocation from_c(const holder_new_storage_location& value) {
  contract::NewStorageLocation out;
  out.location_id = optional_text(value.location_id, "new_storage_location.location_id", 0, SIZE_MAX);
  out.project_id = text(value.project_id, "new_storage_location.project_id", 1, SIZE_MAX);
  out.name = text(value.name, "new_storage_location.name", 1, SIZE_MAX);
  switch (value.provider) {
  case HOLDER_NEW_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY: out.provider = contract::NewStorageLocationProvider::LocalDirectory; break;
  case HOLDER_NEW_STORAGE_LOCATION_PROVIDER_S3_COMPATIBLE: out.provider = contract::NewStorageLocationProvider::S3Compatible; break;
  case HOLDER_NEW_STORAGE_LOCATION_PROVIDER_GOOGLE_DRIVE: out.provider = contract::NewStorageLocationProvider::GoogleDrive; break;
  default: throw ArgumentError("new_storage_location.provider is not a holder_new_storage_location_provider value");
  }
  out.configuration = (value.configuration ? std::optional<std::map<std::string, std::string>>(from_c_location_configuration(*value.configuration, "new_storage_location.configuration")) : std::nullopt);
  out.created_at = (value.created_at ? std::optional<long long>(*value.created_at) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(*value.updated_at) : std::nullopt);
  return out;
}

contract::StorageLocationUpdate from_c(const holder_storage_location_update& value) {
  contract::StorageLocationUpdate out;
  out.name = optional_text(value.name, "storage_location_update.name", 1, SIZE_MAX);
  out.configuration = (value.configuration ? std::optional<std::map<std::string, std::string>>(from_c_location_configuration(*value.configuration, "storage_location_update.configuration")) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(*value.updated_at) : std::nullopt);
  return out;
}

contract::CardReferenceResolveRequest from_c(const holder_card_reference_resolve_request& value) {
  contract::CardReferenceResolveRequest out;
  out.project_id = text(value.project_id, "card_reference_resolve_request.project_id", 1, SIZE_MAX);
  out.reference = text(value.reference, "card_reference_resolve_request.reference", 1, SIZE_MAX);
  switch (value.scope) {
  case HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_LIVE: out.scope = contract::CardReferenceResolveRequestScope::Live; break;
  case HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_TRASHED: out.scope = contract::CardReferenceResolveRequestScope::Trashed; break;
  case HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_EITHER: out.scope = contract::CardReferenceResolveRequestScope::Either; break;
  default: throw ArgumentError("card_reference_resolve_request.scope is not a holder_card_reference_resolve_request_scope value");
  }
  return out;
}

contract::NewCard from_c(const holder_new_card& value) {
  contract::NewCard out;
  out.title = text(value.title, "new_card.title", 1, SIZE_MAX);
  out.content = optional_text(value.content, "new_card.content", 0, SIZE_MAX).value_or("");
  out.parent_card_id = optional_text(value.parent_card_id, "new_card.parent_card_id", 1, SIZE_MAX);
  out.sort_key = (value.sort_key ? std::optional<double>(finite(*value.sort_key, "new_card.sort_key")) : std::nullopt);
  out.created_at = (value.created_at ? std::optional<long long>(*value.created_at) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(*value.updated_at) : std::nullopt);
  return out;
}

contract::CardUpdate from_c(const holder_card_update& value) {
  contract::CardUpdate out;
  out.title = optional_text(value.title, "card_update.title", 1, SIZE_MAX);
  out.content = optional_text(value.content, "card_update.content", 0, SIZE_MAX);
  out.parent_card_id = changed_text(value.parent_card_id, "card_update.parent_card_id", 1, SIZE_MAX);
  out.sort_key = (value.sort_key ? std::optional<double>(finite(*value.sort_key, "card_update.sort_key")) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(*value.updated_at) : std::nullopt);
  return out;
}

contract::CardMove from_c(const holder_card_move& value) {
  contract::CardMove out;
  switch (value.intent) {
  case HOLDER_CARD_MOVE_INTENT_INTO: out.intent = contract::CardMoveIntent::Into; break;
  case HOLDER_CARD_MOVE_INTENT_BEFORE: out.intent = contract::CardMoveIntent::Before; break;
  case HOLDER_CARD_MOVE_INTENT_AFTER: out.intent = contract::CardMoveIntent::After; break;
  case HOLDER_CARD_MOVE_INTENT_LEFT: out.intent = contract::CardMoveIntent::Left; break;
  case HOLDER_CARD_MOVE_INTENT_RIGHT: out.intent = contract::CardMoveIntent::Right; break;
  case HOLDER_CARD_MOVE_INTENT_TO_START: out.intent = contract::CardMoveIntent::ToStart; break;
  case HOLDER_CARD_MOVE_INTENT_TO_END: out.intent = contract::CardMoveIntent::ToEnd; break;
  case HOLDER_CARD_MOVE_INTENT_UP_LEVEL: out.intent = contract::CardMoveIntent::UpLevel; break;
  default: throw ArgumentError("card_move.intent is not a holder_card_move_intent value");
  }
  out.target_card_id = optional_text(value.target_card_id, "card_move.target_card_id", 1, SIZE_MAX);
  out.parent_card_id = changed_text(value.parent_card_id, "card_move.parent_card_id", 1, SIZE_MAX);
  return out;
}

contract::NewLink from_c(const holder_new_link& value) {
  contract::NewLink out;
  switch (value.target_type) {
  case HOLDER_NEW_LINK_TARGET_TYPE_CARD: out.target_type = contract::NewLinkTargetType::Card; break;
  case HOLDER_NEW_LINK_TARGET_TYPE_AI_MESSAGE: out.target_type = contract::NewLinkTargetType::AiMessage; break;
  case HOLDER_NEW_LINK_TARGET_TYPE_AI_THREAD: out.target_type = contract::NewLinkTargetType::AiThread; break;
  case HOLDER_NEW_LINK_TARGET_TYPE_RESOURCE: out.target_type = contract::NewLinkTargetType::Resource; break;
  default: throw ArgumentError("new_link.target_type is not a holder_new_link_target_type value");
  }
  out.target_id = text(value.target_id, "new_link.target_id", 1, SIZE_MAX);
  out.target_project_id = optional_text(value.target_project_id, "new_link.target_project_id", 1, SIZE_MAX);
  out.kind = optional_text(value.kind, "new_link.kind", 1, SIZE_MAX).value_or("ref");
  out.label = optional_text(value.label, "new_link.label", 0, SIZE_MAX);
  out.created_at = (value.created_at ? std::optional<long long>(*value.created_at) : std::nullopt);
  return out;
}

contract::MilestoneDate from_c(const holder_milestone_date& value) {
  contract::MilestoneDate out;
  out.role = text(value.role, "milestone_date.role", 1, SIZE_MAX);
  out.date = optional_text(value.date, "milestone_date.date", 10, 10);
  out.datetime = optional_text(value.datetime, "milestone_date.datetime", 20, 25);
  return out;
}

std::vector<contract::MilestoneDate> from_c_list(const holder_milestone_date_list& values, const char* name) {
  require(values.items != nullptr || values.count == 0, std::string(name) + ".items must not be null");
  std::vector<contract::MilestoneDate> out;
  out.reserve(values.count);
  for (std::size_t i = 0; i < values.count; ++i) out.push_back(from_c(values.items[i]));
  return out;
}

contract::NewMilestone from_c(const holder_new_milestone& value) {
  contract::NewMilestone out;
  out.dates = from_c_list(value.dates, "new_milestone.dates");
  out.kind = optional_text(value.kind, "new_milestone.kind", 1, SIZE_MAX);
  out.description = optional_text(value.description, "new_milestone.description", 1, SIZE_MAX);
  return out;
}

contract::MilestoneUpdate from_c(const holder_milestone_update& value) {
  contract::MilestoneUpdate out;
  out.dates = (value.dates ? std::optional<std::vector<contract::MilestoneDate>>(from_c_list(*value.dates, "milestone_update.dates")) : std::nullopt);
  out.kind = changed_text(value.kind, "milestone_update.kind", 1, SIZE_MAX);
  out.description = changed_text(value.description, "milestone_update.description", 1, SIZE_MAX);
  return out;
}

contract::NewResource from_c(const holder_new_resource& value) {
  contract::NewResource out;
  out.resource_id = optional_text(value.resource_id, "new_resource.resource_id", 4, SIZE_MAX);
  out.type = text(value.type, "new_resource.type", 1, SIZE_MAX);
  out.label = text(value.label, "new_resource.label", 1, SIZE_MAX);
  out.metadata = (value.metadata ? std::optional<std::map<std::string, std::vector<std::string>>>(from_c_resource_metadata(*value.metadata, "new_resource.metadata")) : std::nullopt);
  out.created_at = (value.created_at ? std::optional<long long>(*value.created_at) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(*value.updated_at) : std::nullopt);
  return out;
}

contract::ResourceUpdate from_c(const holder_resource_update& value) {
  contract::ResourceUpdate out;
  out.type = optional_text(value.type, "resource_update.type", 1, SIZE_MAX);
  out.label = optional_text(value.label, "resource_update.label", 1, SIZE_MAX);
  out.metadata = (value.metadata ? std::optional<std::map<std::string, std::vector<std::string>>>(from_c_resource_metadata(*value.metadata, "resource_update.metadata")) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(*value.updated_at) : std::nullopt);
  return out;
}

contract::NewProject from_c(const holder_new_project& value) {
  contract::NewProject out;
  out.project_id = optional_text(value.project_id, "new_project.project_id", 1, SIZE_MAX);
  out.name = text(value.name, "new_project.name", 1, SIZE_MAX);
  out.root_path = optional_text(value.root_path, "new_project.root_path", 0, SIZE_MAX);
  out.git_remote_url = optional_text(value.git_remote_url, "new_project.git_remote_url", 1, SIZE_MAX);
  out.git_provider = optional_text(value.git_provider, "new_project.git_provider", 0, SIZE_MAX);
  switch (value.privacy_mode) {
  case HOLDER_NEW_PROJECT_PRIVACY_MODE_DEFAULT: break;
  case HOLDER_NEW_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT: out.privacy_mode = contract::NewProjectPrivacyMode::EncryptedGit; break;
  case HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN: out.privacy_mode = contract::NewProjectPrivacyMode::Plain; break;
  default: throw ArgumentError("new_project.privacy_mode is not a holder_new_project_privacy_mode value");
  }
  out.created_at = (value.created_at ? std::optional<long long>(in_range<long long>(*value.created_at, "new_project.created_at", 1, std::numeric_limits<long long>::max())) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(in_range<long long>(*value.updated_at, "new_project.updated_at", 1, std::numeric_limits<long long>::max())) : std::nullopt);
  return out;
}

contract::ProjectUpdate from_c(const holder_project_update& value) {
  contract::ProjectUpdate out;
  out.name = optional_text(value.name, "project_update.name", 1, SIZE_MAX);
  out.git_remote_url = changed_text(value.git_remote_url, "project_update.git_remote_url", 1, SIZE_MAX);
  out.git_provider = changed_text(value.git_provider, "project_update.git_provider", 0, SIZE_MAX);
  return out;
}

contract::HierarchyRepairRequest from_c(const holder_hierarchy_repair_request& value) {
  contract::HierarchyRepairRequest out;
  out.apply = (value.apply ? *value.apply : bool{false});
  return out;
}

contract::RebuildRequest from_c(const holder_rebuild_request& value) {
  contract::RebuildRequest out;
  out.project_id = text(value.project_id, "rebuild_request.project_id", 1, SIZE_MAX);
  return out;
}

contract::RecoveryTokenExportRequest from_c(const holder_recovery_token_export_request& value) {
  contract::RecoveryTokenExportRequest out;
  out.pin = text(value.pin, "recovery_token_export_request.pin", 1, SIZE_MAX);
  return out;
}

contract::RecoveryTokenImportRequest from_c(const holder_recovery_token_import_request& value) {
  contract::RecoveryTokenImportRequest out;
  out.pin = text(value.pin, "recovery_token_import_request.pin", 1, SIZE_MAX);
  out.recovery_token = text(value.recovery_token, "recovery_token_import_request.recovery_token", 1, SIZE_MAX);
  return out;
}

contract::PushOptions from_c(const holder_push_options& value) {
  contract::PushOptions out;
  out.branch = optional_text(value.branch, "push_options.branch", 1, SIZE_MAX);
  out.set_upstream = (value.set_upstream ? *value.set_upstream : bool{true});
  return out;
}

contract::RemoteTest from_c(const holder_remote_test& value) {
  contract::RemoteTest out;
  out.remote_url = optional_text(value.remote_url, "remote_test.remote_url", 1, SIZE_MAX);
  return out;
}

}  // namespace

extern "C" {

holder_arena* holder_arena_new(void) { return new (std::nothrow) holder_arena; }
void holder_arena_free(holder_arena* arena) { delete arena; }

int holder_get_storage_location(holder_context* context, const char* location_id, holder_arena* arena, const holder_project_storage_location** out_project_storage_location, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project_storage_location != nullptr, "out_project_storage_location must not be null");
    *out_project_storage_location = nullptr;
    const auto location_id_value = text(location_id, "location_id", 1, SIZE_MAX);
    auto* out = make<holder_project_storage_location>(arena);
    *out = to_c(arena, contract::get_storage_location(*context, location_id_value));
    *out_project_storage_location = out;
  });
}

int holder_create_storage_location(holder_context* context, const holder_new_storage_location* request, holder_arena* arena, const holder_project_storage_location** out_project_storage_location, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project_storage_location != nullptr, "out_project_storage_location must not be null");
    *out_project_storage_location = nullptr;
    require(request != nullptr, "request must not be null");
    const auto request_value = from_c(*request);
    auto* out = make<holder_project_storage_location>(arena);
    *out = to_c(arena, contract::create_storage_location(*context, request_value));
    *out_project_storage_location = out;
  });
}

int holder_update_storage_location(holder_context* context, const char* location_id, const holder_storage_location_update* request, holder_arena* arena, const holder_project_storage_location** out_project_storage_location, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project_storage_location != nullptr, "out_project_storage_location must not be null");
    *out_project_storage_location = nullptr;
    const auto location_id_value = text(location_id, "location_id", 1, SIZE_MAX);
    require(request != nullptr, "request must not be null");
    const auto request_value = from_c(*request);
    auto* out = make<holder_project_storage_location>(arena);
    *out = to_c(arena, contract::update_storage_location(*context, location_id_value, request_value));
    *out_project_storage_location = out;
  });
}

int holder_delete_storage_location(holder_context* context, const char* location_id, holder_arena* arena, const holder_location_delete_result** out_location_delete_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_location_delete_result != nullptr, "out_location_delete_result must not be null");
    *out_location_delete_result = nullptr;
    const auto location_id_value = text(location_id, "location_id", 1, SIZE_MAX);
    auto* out = make<holder_location_delete_result>(arena);
    *out = to_c(arena, contract::delete_storage_location(*context, location_id_value));
    *out_location_delete_result = out;
  });
}

int holder_get_card(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::get_card(*context, project_id_value, card_id_value));
    *out_card = out;
  });
}

int holder_get_card_metadata(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card_metadata** out_card_metadata, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card_metadata != nullptr, "out_card_metadata must not be null");
    *out_card_metadata = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    auto* out = make<holder_card_metadata>(arena);
    *out = to_c(arena, contract::get_card_metadata(*context, project_id_value, card_id_value));
    *out_card_metadata = out;
  });
}

int holder_get_card_breadcrumbs(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card_breadcrumb_list** out_card_breadcrumb_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card_breadcrumb_list != nullptr, "out_card_breadcrumb_list must not be null");
    *out_card_breadcrumb_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    auto* out = make<holder_card_breadcrumb_list>(arena);
    *out = to_c_list(arena, contract::get_card_breadcrumbs(*context, project_id_value, card_id_value));
    *out_card_breadcrumb_list = out;
  });
}

int holder_resolve_card_reference(holder_context* context, const holder_card_reference_resolve_request* request, holder_arena* arena, const holder_card_reference_resolve_data** out_card_reference_resolve_data, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card_reference_resolve_data != nullptr, "out_card_reference_resolve_data must not be null");
    *out_card_reference_resolve_data = nullptr;
    require(request != nullptr, "request must not be null");
    const auto request_value = from_c(*request);
    auto* out = make<holder_card_reference_resolve_data>(arena);
    *out = to_c(arena, contract::resolve_card_reference(*context, request_value));
    *out_card_reference_resolve_data = out;
  });
}

int holder_list_cards(holder_context* context, const char* project_id, holder_list_cards_parent parent, const char* tag, holder_list_cards_order order, const int32_t* limit, const char* cursor, bool include_content, bool include_child_count, holder_arena* arena, const holder_card_list_item_page** out_page, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_page != nullptr, "out_page must not be null");
    *out_page = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    std::optional<contract::ListCardsParent> parent_value;
    switch (parent.kind) {
    case HOLDER_LIST_CARDS_PARENT_NONE: break;
    case HOLDER_LIST_CARDS_PARENT_ROOTS: parent_value = contract::ListCardsParentRoots{}; break;
    case HOLDER_LIST_CARDS_PARENT_CARD_ID: parent_value = text(parent.card_id, "parent.card_id", 1, SIZE_MAX); break;
    default: throw ArgumentError("parent.kind is not a holder_list_cards_parent_kind value");
    }
    const auto tag_value = optional_text(tag, "tag", 1, SIZE_MAX);
    contract::ListCardsOrder order_value;
    switch (order) {
    case HOLDER_LIST_CARDS_ORDER_DEFAULT: order_value = contract::ListCardsOrder::CardIdAsc; break;
    case HOLDER_LIST_CARDS_ORDER_CARD_ID_ASC: order_value = contract::ListCardsOrder::CardIdAsc; break;
    case HOLDER_LIST_CARDS_ORDER_UPDATED_DESC: order_value = contract::ListCardsOrder::UpdatedDesc; break;
    case HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC: order_value = contract::ListCardsOrder::SortKeyAsc; break;
    default: throw ArgumentError("order is not a holder_list_cards_order value");
    }
    const auto limit_value = (limit ? in_range<std::int32_t>(*limit, "limit", 1, 5000) : std::int32_t{200});
    const auto cursor_value = optional_text(cursor, "cursor", 1, 8192);
    const auto include_content_value = include_content;
    const auto include_child_count_value = include_child_count;
    auto* out = make<holder_card_list_item_page>(arena);
    *out = to_c(arena, contract::list_cards(*context, project_id_value, parent_value, tag_value, order_value, limit_value, cursor_value, include_content_value, include_child_count_value));
    *out_page = out;
  });
}

int holder_create_card(holder_context* context, const char* project_id, const holder_new_card* card, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(card != nullptr, "card must not be null");
    const auto card_value = from_c(*card);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::create_card(*context, project_id_value, card_value));
    *out_card = out;
  });
}

int holder_update_card(holder_context* context, const char* project_id, const char* card_id, const holder_card_update* update, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    require(update != nullptr, "update must not be null");
    const auto update_value = from_c(*update);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::update_card(*context, project_id_value, card_id_value, update_value));
    *out_card = out;
  });
}

int holder_trash_card(holder_context* context, const char* project_id, const char* card_id, holder_trash_card_children children, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    contract::TrashCardChildren children_value;
    switch (children) {
    case HOLDER_TRASH_CARD_CHILDREN_DEFAULT: children_value = contract::TrashCardChildren::Promote; break;
    case HOLDER_TRASH_CARD_CHILDREN_PROMOTE: children_value = contract::TrashCardChildren::Promote; break;
    case HOLDER_TRASH_CARD_CHILDREN_TRASH_SUBTREE: children_value = contract::TrashCardChildren::TrashSubtree; break;
    default: throw ArgumentError("children is not a holder_trash_card_children value");
    }
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::trash_card(*context, project_id_value, card_id_value, children_value));
    *out_card = out;
  });
}

int holder_restore_card(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::restore_card(*context, project_id_value, card_id_value));
    *out_card = out;
  });
}

int holder_move_card(holder_context* context, const char* project_id, const char* card_id, const holder_card_move* move, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    require(move != nullptr, "move must not be null");
    const auto move_value = from_c(*move);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::move_card(*context, project_id_value, card_id_value, move_value));
    *out_card = out;
  });
}

int holder_add_card_tag(holder_context* context, const char* project_id, const char* card_id, const char* tag, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto tag_value = text(tag, "tag", 1, SIZE_MAX);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::add_card_tag(*context, project_id_value, card_id_value, tag_value));
    *out_card = out;
  });
}

int holder_remove_card_tag(holder_context* context, const char* project_id, const char* card_id, const char* tag, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto tag_value = text(tag, "tag", 1, SIZE_MAX);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::remove_card_tag(*context, project_id_value, card_id_value, tag_value));
    *out_card = out;
  });
}

int holder_list_project_tags(holder_context* context, const char* project_id, holder_arena* arena, const holder_tag_count_list** out_tag_count_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_tag_count_list != nullptr, "out_tag_count_list must not be null");
    *out_tag_count_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    auto* out = make<holder_tag_count_list>(arena);
    *out = to_c_list(arena, contract::list_project_tags(*context, project_id_value));
    *out_tag_count_list = out;
  });
}

int holder_list_card_links(holder_context* context, const char* project_id, const char* card_id, bool include_deleted, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_link_list != nullptr, "out_link_list must not be null");
    *out_link_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto include_deleted_value = include_deleted;
    auto* out = make<holder_link_list>(arena);
    *out = to_c_list(arena, contract::list_card_links(*context, project_id_value, card_id_value, include_deleted_value));
    *out_link_list = out;
  });
}

int holder_list_card_backlinks(holder_context* context, const char* project_id, const char* card_id, holder_list_card_backlinks_scope scope, bool include_deleted, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_link_list != nullptr, "out_link_list must not be null");
    *out_link_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    contract::ListCardBacklinksScope scope_value;
    switch (scope) {
    case HOLDER_LIST_CARD_BACKLINKS_SCOPE_DEFAULT: scope_value = contract::ListCardBacklinksScope::Local; break;
    case HOLDER_LIST_CARD_BACKLINKS_SCOPE_LOCAL: scope_value = contract::ListCardBacklinksScope::Local; break;
    case HOLDER_LIST_CARD_BACKLINKS_SCOPE_EXTERNAL: scope_value = contract::ListCardBacklinksScope::External; break;
    case HOLDER_LIST_CARD_BACKLINKS_SCOPE_ALL_AVAILABLE: scope_value = contract::ListCardBacklinksScope::AllAvailable; break;
    default: throw ArgumentError("scope is not a holder_list_card_backlinks_scope value");
    }
    const auto include_deleted_value = include_deleted;
    auto* out = make<holder_link_list>(arena);
    *out = to_c_list(arena, contract::list_card_backlinks(*context, project_id_value, card_id_value, scope_value, include_deleted_value));
    *out_link_list = out;
  });
}

int holder_add_card_link(holder_context* context, const char* project_id, const char* card_id, const holder_new_link* link, holder_arena* arena, const holder_link** out_link, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_link != nullptr, "out_link must not be null");
    *out_link = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    require(link != nullptr, "link must not be null");
    const auto link_value = from_c(*link);
    auto* out = make<holder_link>(arena);
    *out = to_c(arena, contract::add_card_link(*context, project_id_value, card_id_value, link_value));
    *out_link = out;
  });
}

int holder_remove_card_link(holder_context* context, const char* project_id, const char* card_id, holder_remove_card_link_target_type target_type, const char* target_id, const char* kind, const char* target_project_id, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_link_list != nullptr, "out_link_list must not be null");
    *out_link_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    contract::RemoveCardLinkTargetType target_type_value;
    switch (target_type) {
    case HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_CARD: target_type_value = contract::RemoveCardLinkTargetType::Card; break;
    case HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_AI_MESSAGE: target_type_value = contract::RemoveCardLinkTargetType::AiMessage; break;
    case HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_AI_THREAD: target_type_value = contract::RemoveCardLinkTargetType::AiThread; break;
    case HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_RESOURCE: target_type_value = contract::RemoveCardLinkTargetType::Resource; break;
    default: throw ArgumentError("target_type is not a holder_remove_card_link_target_type value");
    }
    const auto target_id_value = text(target_id, "target_id", 1, SIZE_MAX);
    const auto kind_value = optional_text(kind, "kind", 1, SIZE_MAX).value_or("ref");
    const auto target_project_id_value = optional_text(target_project_id, "target_project_id", 1, SIZE_MAX);
    auto* out = make<holder_link_list>(arena);
    *out = to_c_list(arena, contract::remove_card_link(*context, project_id_value, card_id_value, target_type_value, target_id_value, kind_value, target_project_id_value));
    *out_link_list = out;
  });
}

int holder_list_card_milestones(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_milestone_list** out_milestone_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_milestone_list != nullptr, "out_milestone_list must not be null");
    *out_milestone_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    auto* out = make<holder_milestone_list>(arena);
    *out = to_c_list(arena, contract::list_card_milestones(*context, project_id_value, card_id_value));
    *out_milestone_list = out;
  });
}

int holder_add_card_milestone(holder_context* context, const char* project_id, const char* card_id, const holder_new_milestone* milestone, holder_arena* arena, const holder_milestone** out_milestone, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_milestone != nullptr, "out_milestone must not be null");
    *out_milestone = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    require(milestone != nullptr, "milestone must not be null");
    const auto milestone_value = from_c(*milestone);
    auto* out = make<holder_milestone>(arena);
    *out = to_c(arena, contract::add_card_milestone(*context, project_id_value, card_id_value, milestone_value));
    *out_milestone = out;
  });
}

int holder_update_card_milestone(holder_context* context, const char* project_id, const char* card_id, const char* milestone_id, const holder_milestone_update* update, holder_arena* arena, const holder_milestone** out_milestone, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_milestone != nullptr, "out_milestone must not be null");
    *out_milestone = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto milestone_id_value = text(milestone_id, "milestone_id", 1, SIZE_MAX);
    require(update != nullptr, "update must not be null");
    const auto update_value = from_c(*update);
    auto* out = make<holder_milestone>(arena);
    *out = to_c(arena, contract::update_card_milestone(*context, project_id_value, card_id_value, milestone_id_value, update_value));
    *out_milestone = out;
  });
}

int holder_remove_card_milestone(holder_context* context, const char* project_id, const char* card_id, const char* milestone_id, holder_arena* arena, const holder_milestone_list** out_milestone_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_milestone_list != nullptr, "out_milestone_list must not be null");
    *out_milestone_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto milestone_id_value = text(milestone_id, "milestone_id", 1, SIZE_MAX);
    auto* out = make<holder_milestone_list>(arena);
    *out = to_c_list(arena, contract::remove_card_milestone(*context, project_id_value, card_id_value, milestone_id_value));
    *out_milestone_list = out;
  });
}

int holder_list_milestones_by_day(holder_context* context, const char* project_id, const char* first_day, const char* last_day, const char* role, holder_arena* arena, const holder_milestone_list** out_milestone_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_milestone_list != nullptr, "out_milestone_list must not be null");
    *out_milestone_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto first_day_value = text(first_day, "first_day", 10, 10);
    const auto last_day_value = text(last_day, "last_day", 10, 10);
    const auto role_value = optional_text(role, "role", 1, SIZE_MAX);
    auto* out = make<holder_milestone_list>(arena);
    *out = to_c_list(arena, contract::list_milestones_by_day(*context, project_id_value, first_day_value, last_day_value, role_value));
    *out_milestone_list = out;
  });
}

int holder_list_milestones_by_instant(holder_context* context, const char* project_id, int64_t first_instant, int64_t last_instant, const char* role, holder_arena* arena, const holder_milestone_list** out_milestone_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_milestone_list != nullptr, "out_milestone_list must not be null");
    *out_milestone_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto first_instant_value = first_instant;
    const auto last_instant_value = last_instant;
    const auto role_value = optional_text(role, "role", 1, SIZE_MAX);
    auto* out = make<holder_milestone_list>(arena);
    *out = to_c_list(arena, contract::list_milestones_by_instant(*context, project_id_value, first_instant_value, last_instant_value, role_value));
    *out_milestone_list = out;
  });
}

int holder_get_calendar(holder_context* context, const char* project_id, int64_t first_instant, int64_t last_instant, const char* first_day, const char* last_day, holder_arena* arena, const holder_calendar** out_calendar, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_calendar != nullptr, "out_calendar must not be null");
    *out_calendar = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto first_instant_value = first_instant;
    const auto last_instant_value = last_instant;
    const auto first_day_value = text(first_day, "first_day", 10, 10);
    const auto last_day_value = text(last_day, "last_day", 10, 10);
    auto* out = make<holder_calendar>(arena);
    *out = to_c(arena, contract::get_calendar(*context, project_id_value, first_instant_value, last_instant_value, first_day_value, last_day_value));
    *out_calendar = out;
  });
}

int holder_list_resources(holder_context* context, const char* project_id, holder_arena* arena, const holder_resource_list** out_resource_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_resource_list != nullptr, "out_resource_list must not be null");
    *out_resource_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    auto* out = make<holder_resource_list>(arena);
    *out = to_c_list(arena, contract::list_resources(*context, project_id_value));
    *out_resource_list = out;
  });
}

int holder_get_resource(holder_context* context, const char* project_id, const char* resource_id, holder_arena* arena, const holder_resource** out_resource, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_resource != nullptr, "out_resource must not be null");
    *out_resource = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto resource_id_value = text(resource_id, "resource_id", 4, SIZE_MAX);
    auto* out = make<holder_resource>(arena);
    *out = to_c(arena, contract::get_resource(*context, project_id_value, resource_id_value));
    *out_resource = out;
  });
}

int holder_create_resource(holder_context* context, const char* project_id, const holder_new_resource* resource, holder_arena* arena, const holder_resource** out_resource, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_resource != nullptr, "out_resource must not be null");
    *out_resource = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(resource != nullptr, "resource must not be null");
    const auto resource_value = from_c(*resource);
    auto* out = make<holder_resource>(arena);
    *out = to_c(arena, contract::create_resource(*context, project_id_value, resource_value));
    *out_resource = out;
  });
}

int holder_update_resource(holder_context* context, const char* project_id, const char* resource_id, const holder_resource_update* update, holder_arena* arena, const holder_resource** out_resource, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_resource != nullptr, "out_resource must not be null");
    *out_resource = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto resource_id_value = text(resource_id, "resource_id", 4, SIZE_MAX);
    require(update != nullptr, "update must not be null");
    const auto update_value = from_c(*update);
    auto* out = make<holder_resource>(arena);
    *out = to_c(arena, contract::update_resource(*context, project_id_value, resource_id_value, update_value));
    *out_resource = out;
  });
}

int holder_delete_resource(holder_context* context, const char* project_id, const char* resource_id, holder_arena* arena, const holder_resource** out_resource, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_resource != nullptr, "out_resource must not be null");
    *out_resource = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto resource_id_value = text(resource_id, "resource_id", 4, SIZE_MAX);
    auto* out = make<holder_resource>(arena);
    *out = to_c(arena, contract::delete_resource(*context, project_id_value, resource_id_value));
    *out_resource = out;
  });
}

int holder_list_resource_backlinks(holder_context* context, const char* project_id, const char* resource_id, bool include_deleted, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_link_list != nullptr, "out_link_list must not be null");
    *out_link_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto resource_id_value = text(resource_id, "resource_id", 4, SIZE_MAX);
    const auto include_deleted_value = include_deleted;
    auto* out = make<holder_link_list>(arena);
    *out = to_c_list(arena, contract::list_resource_backlinks(*context, project_id_value, resource_id_value, include_deleted_value));
    *out_link_list = out;
  });
}

int holder_list_card_resources(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_resource_list** out_resource_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_resource_list != nullptr, "out_resource_list must not be null");
    *out_resource_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    auto* out = make<holder_resource_list>(arena);
    *out = to_c_list(arena, contract::list_card_resources(*context, project_id_value, card_id_value));
    *out_resource_list = out;
  });
}

int holder_attach_card_resource(holder_context* context, const char* project_id, const char* card_id, const char* resource_id, holder_arena* arena, const holder_resource_list** out_resource_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_resource_list != nullptr, "out_resource_list must not be null");
    *out_resource_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto resource_id_value = text(resource_id, "resource_id", 4, SIZE_MAX);
    auto* out = make<holder_resource_list>(arena);
    *out = to_c_list(arena, contract::attach_card_resource(*context, project_id_value, card_id_value, resource_id_value));
    *out_resource_list = out;
  });
}

int holder_detach_card_resource(holder_context* context, const char* project_id, const char* card_id, const char* resource_id, holder_arena* arena, const holder_resource_list** out_resource_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_resource_list != nullptr, "out_resource_list must not be null");
    *out_resource_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto resource_id_value = text(resource_id, "resource_id", 4, SIZE_MAX);
    auto* out = make<holder_resource_list>(arena);
    *out = to_c_list(arena, contract::detach_card_resource(*context, project_id_value, card_id_value, resource_id_value));
    *out_resource_list = out;
  });
}

int holder_list_projects(holder_context* context, const char* name, const int64_t* updated_after, const int64_t* updated_before, holder_list_projects_order order, const int32_t* limit, int32_t offset, bool include_card_counts, holder_arena* arena, const holder_project_list_item_list** out_project_list_item_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project_list_item_list != nullptr, "out_project_list_item_list must not be null");
    *out_project_list_item_list = nullptr;
    const auto name_value = optional_text(name, "name", 1, SIZE_MAX);
    const auto updated_after_value = (updated_after ? std::optional<long long>(*updated_after) : std::nullopt);
    const auto updated_before_value = (updated_before ? std::optional<long long>(*updated_before) : std::nullopt);
    std::optional<contract::ListProjectsOrder> order_value;
    switch (order) {
    case HOLDER_LIST_PROJECTS_ORDER_NONE: break;
    case HOLDER_LIST_PROJECTS_ORDER_UPDATED_AT_DESC: order_value = contract::ListProjectsOrder::UpdatedAtDesc; break;
    case HOLDER_LIST_PROJECTS_ORDER_UPDATED_AT_ASC: order_value = contract::ListProjectsOrder::UpdatedAtAsc; break;
    case HOLDER_LIST_PROJECTS_ORDER_CREATED_AT_DESC: order_value = contract::ListProjectsOrder::CreatedAtDesc; break;
    case HOLDER_LIST_PROJECTS_ORDER_CREATED_AT_ASC: order_value = contract::ListProjectsOrder::CreatedAtAsc; break;
    case HOLDER_LIST_PROJECTS_ORDER_NAME_DESC: order_value = contract::ListProjectsOrder::NameDesc; break;
    case HOLDER_LIST_PROJECTS_ORDER_NAME_ASC: order_value = contract::ListProjectsOrder::NameAsc; break;
    default: throw ArgumentError("order is not a holder_list_projects_order value");
    }
    const auto limit_value = (limit ? in_range<std::int32_t>(*limit, "limit", 1, 1000) : std::int32_t{100});
    const auto offset_value = in_range<std::int32_t>(offset, "offset", 0, std::numeric_limits<std::int32_t>::max());
    const auto include_card_counts_value = include_card_counts;
    auto* out = make<holder_project_list_item_list>(arena);
    *out = to_c_list(arena, contract::list_projects(*context, name_value, updated_after_value, updated_before_value, order_value, limit_value, offset_value, include_card_counts_value));
    *out_project_list_item_list = out;
  });
}

int holder_get_project(holder_context* context, const char* project_id, holder_arena* arena, const holder_project** out_project, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project != nullptr, "out_project must not be null");
    *out_project = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    auto* out = make<holder_project>(arena);
    *out = to_c(arena, contract::get_project(*context, project_id_value));
    *out_project = out;
  });
}

int holder_create_project(holder_context* context, const holder_new_project* project, holder_arena* arena, const holder_project** out_project, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project != nullptr, "out_project must not be null");
    *out_project = nullptr;
    require(project != nullptr, "project must not be null");
    const auto project_value = from_c(*project);
    auto* out = make<holder_project>(arena);
    *out = to_c(arena, contract::create_project(*context, project_value));
    *out_project = out;
  });
}

int holder_update_project(holder_context* context, const char* project_id, const holder_project_update* update, holder_arena* arena, const holder_project** out_project, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project != nullptr, "out_project must not be null");
    *out_project = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(update != nullptr, "update must not be null");
    const auto update_value = from_c(*update);
    auto* out = make<holder_project>(arena);
    *out = to_c(arena, contract::update_project(*context, project_id_value, update_value));
    *out_project = out;
  });
}

int holder_delete_project(holder_context* context, const char* project_id, holder_arena* arena, const holder_project** out_project, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project != nullptr, "out_project must not be null");
    *out_project = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    auto* out = make<holder_project>(arena);
    *out = to_c(arena, contract::delete_project(*context, project_id_value));
    *out_project = out;
  });
}

int holder_check_project_encryption(holder_context* context, const char* project_id, holder_arena* arena, const holder_project_encryption_check** out_project_encryption_check, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project_encryption_check != nullptr, "out_project_encryption_check must not be null");
    *out_project_encryption_check = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    auto* out = make<holder_project_encryption_check>(arena);
    *out = to_c(arena, contract::check_project_encryption(*context, project_id_value));
    *out_project_encryption_check = out;
  });
}

int holder_repair_project_hierarchy(holder_context* context, const char* project_id, const holder_hierarchy_repair_request* request, holder_arena* arena, const holder_hierarchy_repair** out_hierarchy_repair, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_hierarchy_repair != nullptr, "out_hierarchy_repair must not be null");
    *out_hierarchy_repair = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(request != nullptr, "request must not be null");
    const auto request_value = from_c(*request);
    auto* out = make<holder_hierarchy_repair>(arena);
    *out = to_c(arena, contract::repair_project_hierarchy(*context, project_id_value, request_value));
    *out_hierarchy_repair = out;
  });
}

int holder_rebuild_project(holder_context* context, const holder_rebuild_request* request, holder_arena* arena, const holder_rebuild_result** out_rebuild_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_rebuild_result != nullptr, "out_rebuild_result must not be null");
    *out_rebuild_result = nullptr;
    require(request != nullptr, "request must not be null");
    const auto request_value = from_c(*request);
    auto* out = make<holder_rebuild_result>(arena);
    *out = to_c(arena, contract::rebuild_project(*context, request_value));
    *out_rebuild_result = out;
  });
}

int holder_reindex_search(holder_context* context, holder_arena* arena, const holder_reindex_result** out_reindex_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_reindex_result != nullptr, "out_reindex_result must not be null");
    *out_reindex_result = nullptr;
    auto* out = make<holder_reindex_result>(arena);
    *out = to_c(arena, contract::reindex_search(*context));
    *out_reindex_result = out;
  });
}

int holder_export_project_recovery_token(holder_context* context, const char* project_id, const holder_recovery_token_export_request* request, holder_arena* arena, const holder_project_recovery_token_export_result** out_project_recovery_token_export_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project_recovery_token_export_result != nullptr, "out_project_recovery_token_export_result must not be null");
    *out_project_recovery_token_export_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(request != nullptr, "request must not be null");
    const auto request_value = from_c(*request);
    auto* out = make<holder_project_recovery_token_export_result>(arena);
    *out = to_c(arena, contract::export_project_recovery_token(*context, project_id_value, request_value));
    *out_project_recovery_token_export_result = out;
  });
}

int holder_import_project_recovery_token(holder_context* context, const char* project_id, const holder_recovery_token_import_request* request, holder_arena* arena, const holder_project_recovery_token_import_result** out_project_recovery_token_import_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project_recovery_token_import_result != nullptr, "out_project_recovery_token_import_result must not be null");
    *out_project_recovery_token_import_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(request != nullptr, "request must not be null");
    const auto request_value = from_c(*request);
    auto* out = make<holder_project_recovery_token_import_result>(arena);
    *out = to_c(arena, contract::import_project_recovery_token(*context, project_id_value, request_value));
    *out_project_recovery_token_import_result = out;
  });
}

int holder_import_recovery_token_global(holder_context* context, const holder_recovery_token_import_request* request, holder_arena* arena, const holder_recovery_token_global_import_result** out_recovery_token_global_import_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_recovery_token_global_import_result != nullptr, "out_recovery_token_global_import_result must not be null");
    *out_recovery_token_global_import_result = nullptr;
    require(request != nullptr, "request must not be null");
    const auto request_value = from_c(*request);
    auto* out = make<holder_recovery_token_global_import_result>(arena);
    *out = to_c(arena, contract::import_recovery_token_global(*context, request_value));
    *out_recovery_token_global_import_result = out;
  });
}

int holder_get_project_sync_state(holder_context* context, const char* project_id, holder_arena* arena, const holder_sync_state** out_sync_state, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_sync_state != nullptr, "out_sync_state must not be null");
    *out_sync_state = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    auto* out = make<holder_sync_state>(arena);
    *out = to_c(arena, contract::get_project_sync_state(*context, project_id_value));
    *out_sync_state = out;
  });
}

int holder_pull_project(holder_context* context, const char* project_id, holder_arena* arena, const holder_sync_result** out_sync_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_sync_result != nullptr, "out_sync_result must not be null");
    *out_sync_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    auto* out = make<holder_sync_result>(arena);
    *out = to_c(arena, contract::pull_project(*context, project_id_value));
    *out_sync_result = out;
  });
}

int holder_push_project(holder_context* context, const char* project_id, const holder_push_options* options, holder_arena* arena, const holder_sync_result** out_sync_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_sync_result != nullptr, "out_sync_result must not be null");
    *out_sync_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(options != nullptr, "options must not be null");
    const auto options_value = from_c(*options);
    auto* out = make<holder_sync_result>(arena);
    *out = to_c(arena, contract::push_project(*context, project_id_value, options_value));
    *out_sync_result = out;
  });
}

int holder_sync_project(holder_context* context, const char* project_id, const holder_push_options* options, holder_arena* arena, const holder_sync_result** out_sync_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_sync_result != nullptr, "out_sync_result must not be null");
    *out_sync_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(options != nullptr, "options must not be null");
    const auto options_value = from_c(*options);
    auto* out = make<holder_sync_result>(arena);
    *out = to_c(arena, contract::sync_project(*context, project_id_value, options_value));
    *out_sync_result = out;
  });
}

int holder_sync_project_if_due(holder_context* context, const char* project_id, int32_t push_interval_seconds, int32_t pull_interval_seconds, holder_arena* arena, const holder_due_sync_result** out_due_sync_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_due_sync_result != nullptr, "out_due_sync_result must not be null");
    *out_due_sync_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto push_interval_seconds_value = in_range<std::int32_t>(push_interval_seconds, "push_interval_seconds", 0, std::numeric_limits<std::int32_t>::max());
    const auto pull_interval_seconds_value = in_range<std::int32_t>(pull_interval_seconds, "pull_interval_seconds", 0, std::numeric_limits<std::int32_t>::max());
    auto* out = make<holder_due_sync_result>(arena);
    *out = to_c(arena, contract::sync_project_if_due(*context, project_id_value, push_interval_seconds_value, pull_interval_seconds_value));
    *out_due_sync_result = out;
  });
}

int holder_test_project_remote(holder_context* context, const char* project_id, const holder_remote_test* remote, holder_arena* arena, const holder_remote_test_result** out_remote_test_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_remote_test_result != nullptr, "out_remote_test_result must not be null");
    *out_remote_test_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(remote != nullptr, "remote must not be null");
    const auto remote_value = from_c(*remote);
    auto* out = make<holder_remote_test_result>(arena);
    *out = to_c(arena, contract::test_project_remote(*context, project_id_value, remote_value));
    *out_remote_test_result = out;
  });
}

int holder_list_project_history(holder_context* context, const char* project_id, const int32_t* limit, const char* cursor, holder_list_project_history_kind kind, holder_arena* arena, const holder_project_history_page** out_project_history_page, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_project_history_page != nullptr, "out_project_history_page must not be null");
    *out_project_history_page = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto limit_value = (limit ? in_range<std::int32_t>(*limit, "limit", 1, 200) : std::int32_t{50});
    const auto cursor_value = optional_text(cursor, "cursor", 0, SIZE_MAX);
    std::optional<contract::ListProjectHistoryKind> kind_value;
    switch (kind) {
    case HOLDER_LIST_PROJECT_HISTORY_KIND_NONE: break;
    case HOLDER_LIST_PROJECT_HISTORY_KIND_CARD: kind_value = contract::ListProjectHistoryKind::Card; break;
    case HOLDER_LIST_PROJECT_HISTORY_KIND_RESOURCE: kind_value = contract::ListProjectHistoryKind::Resource; break;
    case HOLDER_LIST_PROJECT_HISTORY_KIND_LOCATION: kind_value = contract::ListProjectHistoryKind::Location; break;
    case HOLDER_LIST_PROJECT_HISTORY_KIND_AI_DATA: kind_value = contract::ListProjectHistoryKind::AiData; break;
    case HOLDER_LIST_PROJECT_HISTORY_KIND_PROJECT_SETTINGS: kind_value = contract::ListProjectHistoryKind::ProjectSettings; break;
    case HOLDER_LIST_PROJECT_HISTORY_KIND_UNKNOWN: kind_value = contract::ListProjectHistoryKind::Unknown; break;
    default: throw ArgumentError("kind is not a holder_list_project_history_kind value");
    }
    auto* out = make<holder_project_history_page>(arena);
    *out = to_c(arena, contract::list_project_history(*context, project_id_value, limit_value, cursor_value, kind_value));
    *out_project_history_page = out;
  });
}

int holder_list_card_history(holder_context* context, const char* project_id, const char* card_id, const int32_t* limit, const char* cursor, holder_arena* arena, const holder_card_history_page** out_card_history_page, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card_history_page != nullptr, "out_card_history_page must not be null");
    *out_card_history_page = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto limit_value = (limit ? in_range<std::int32_t>(*limit, "limit", 1, 200) : std::int32_t{50});
    const auto cursor_value = optional_text(cursor, "cursor", 0, SIZE_MAX);
    auto* out = make<holder_card_history_page>(arena);
    *out = to_c(arena, contract::list_card_history(*context, project_id_value, card_id_value, limit_value, cursor_value));
    *out_card_history_page = out;
  });
}

int holder_compare_card_history(holder_context* context, const char* project_id, const char* card_id, const char* to_oid, const char* from_oid, holder_compare_card_history_mode mode, holder_arena* arena, const holder_card_history_comparison** out_card_history_comparison, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card_history_comparison != nullptr, "out_card_history_comparison must not be null");
    *out_card_history_comparison = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto to_oid_value = text(to_oid, "to_oid", 8, 40);
    const auto from_oid_value = optional_text(from_oid, "from_oid", 8, 40);
    contract::CompareCardHistoryMode mode_value;
    switch (mode) {
    case HOLDER_COMPARE_CARD_HISTORY_MODE_DEFAULT: mode_value = contract::CompareCardHistoryMode::Since; break;
    case HOLDER_COMPARE_CARD_HISTORY_MODE_SINCE: mode_value = contract::CompareCardHistoryMode::Since; break;
    case HOLDER_COMPARE_CARD_HISTORY_MODE_CHANGE: mode_value = contract::CompareCardHistoryMode::Change; break;
    default: throw ArgumentError("mode is not a holder_compare_card_history_mode value");
    }
    auto* out = make<holder_card_history_comparison>(arena);
    *out = to_c(arena, contract::compare_card_history(*context, project_id_value, card_id_value, to_oid_value, from_oid_value, mode_value));
    *out_card_history_comparison = out;
  });
}

int holder_get_card_history_snapshot(holder_context* context, const char* project_id, const char* card_id, const char* oid, holder_arena* arena, const holder_card_history_snapshot** out_card_history_snapshot, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card_history_snapshot != nullptr, "out_card_history_snapshot must not be null");
    *out_card_history_snapshot = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto oid_value = text(oid, "oid", 8, 40);
    auto* out = make<holder_card_history_snapshot>(arena);
    *out = to_c(arena, contract::get_card_history_snapshot(*context, project_id_value, card_id_value, oid_value));
    *out_card_history_snapshot = out;
  });
}

int holder_restore_card_history_snapshot(holder_context* context, const char* project_id, const char* card_id, const char* oid, holder_arena* arena, const holder_card_history_restore_result** out_card_history_restore_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card_history_restore_result != nullptr, "out_card_history_restore_result must not be null");
    *out_card_history_restore_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto oid_value = text(oid, "oid", 8, 40);
    auto* out = make<holder_card_history_restore_result>(arena);
    *out = to_c(arena, contract::restore_card_history_snapshot(*context, project_id_value, card_id_value, oid_value));
    *out_card_history_restore_result = out;
  });
}

int holder_list_trash(holder_context* context, const char* project_id, const char* type, holder_arena* arena, const holder_trash_item_list** out_trash_item_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_trash_item_list != nullptr, "out_trash_item_list must not be null");
    *out_trash_item_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto type_value = optional_text(type, "type", 0, SIZE_MAX);
    auto* out = make<holder_trash_item_list>(arena);
    *out = to_c_list(arena, contract::list_trash(*context, project_id_value, type_value));
    *out_trash_item_list = out;
  });
}

int holder_empty_trash(holder_context* context, const char* project_id, holder_empty_trash_type type, holder_arena* arena, const holder_trash_empty_result** out_trash_empty_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_trash_empty_result != nullptr, "out_trash_empty_result must not be null");
    *out_trash_empty_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    contract::EmptyTrashType type_value;
    switch (type) {
    case HOLDER_EMPTY_TRASH_TYPE_DEFAULT: type_value = contract::EmptyTrashType::All; break;
    case HOLDER_EMPTY_TRASH_TYPE_CARD: type_value = contract::EmptyTrashType::Card; break;
    case HOLDER_EMPTY_TRASH_TYPE_AI_MESSAGE: type_value = contract::EmptyTrashType::AiMessage; break;
    case HOLDER_EMPTY_TRASH_TYPE_ALL: type_value = contract::EmptyTrashType::All; break;
    default: throw ArgumentError("type is not a holder_empty_trash_type value");
    }
    auto* out = make<holder_trash_empty_result>(arena);
    *out = to_c(arena, contract::empty_trash(*context, project_id_value, type_value));
    *out_trash_empty_result = out;
  });
}

int holder_delete_trash_item(holder_context* context, holder_delete_trash_item_type type, const char* id, holder_arena* arena, const holder_trash_delete_result** out_trash_delete_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_trash_delete_result != nullptr, "out_trash_delete_result must not be null");
    *out_trash_delete_result = nullptr;
    contract::DeleteTrashItemType type_value;
    switch (type) {
    case HOLDER_DELETE_TRASH_ITEM_TYPE_CARD: type_value = contract::DeleteTrashItemType::Card; break;
    case HOLDER_DELETE_TRASH_ITEM_TYPE_AI_MESSAGE: type_value = contract::DeleteTrashItemType::AiMessage; break;
    default: throw ArgumentError("type is not a holder_delete_trash_item_type value");
    }
    const auto id_value = text(id, "id", 0, SIZE_MAX);
    auto* out = make<holder_trash_delete_result>(arena);
    *out = to_c(arena, contract::delete_trash_item(*context, type_value, id_value));
    *out_trash_delete_result = out;
  });
}

int holder_delete_project_trash_item(holder_context* context, const char* project_id, holder_delete_project_trash_item_type type, const char* id, holder_arena* arena, const holder_trash_delete_result** out_trash_delete_result, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_trash_delete_result != nullptr, "out_trash_delete_result must not be null");
    *out_trash_delete_result = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    contract::DeleteProjectTrashItemType type_value;
    switch (type) {
    case HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_CARD: type_value = contract::DeleteProjectTrashItemType::Card; break;
    case HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_AI_MESSAGE: type_value = contract::DeleteProjectTrashItemType::AiMessage; break;
    default: throw ArgumentError("type is not a holder_delete_project_trash_item_type value");
    }
    const auto id_value = text(id, "id", 0, SIZE_MAX);
    auto* out = make<holder_trash_delete_result>(arena);
    *out = to_c(arena, contract::delete_project_trash_item(*context, project_id_value, type_value, id_value));
    *out_trash_delete_result = out;
  });
}

int holder_search_cards(holder_context* context, const char* project_id, const char* q, const int32_t* limit, int32_t offset, holder_arena* arena, const holder_search_card_list** out_search_card_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_search_card_list != nullptr, "out_search_card_list must not be null");
    *out_search_card_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto q_value = text(q, "q", 1, SIZE_MAX);
    const auto limit_value = (limit ? in_range<std::int32_t>(*limit, "limit", 1, std::numeric_limits<std::int32_t>::max()) : std::int32_t{20});
    const auto offset_value = in_range<std::int32_t>(offset, "offset", 0, std::numeric_limits<std::int32_t>::max());
    auto* out = make<holder_search_card_list>(arena);
    *out = to_c_list(arena, contract::search_cards(*context, project_id_value, q_value, limit_value, offset_value));
    *out_search_card_list = out;
  });
}

int holder_search_ai_messages(holder_context* context, const char* project_id, const char* q, const int32_t* limit, int32_t offset, holder_arena* arena, const holder_search_message_list** out_search_message_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_search_message_list != nullptr, "out_search_message_list must not be null");
    *out_search_message_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto q_value = text(q, "q", 1, SIZE_MAX);
    const auto limit_value = (limit ? in_range<std::int32_t>(*limit, "limit", 1, std::numeric_limits<std::int32_t>::max()) : std::int32_t{20});
    const auto offset_value = in_range<std::int32_t>(offset, "offset", 0, std::numeric_limits<std::int32_t>::max());
    auto* out = make<holder_search_message_list>(arena);
    *out = to_c_list(arena, contract::search_ai_messages(*context, project_id_value, q_value, limit_value, offset_value));
    *out_search_message_list = out;
  });
}

}  // extern "C"
