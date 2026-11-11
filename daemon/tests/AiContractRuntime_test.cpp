#include "api/generated/AiJson.h"
#include "api/generated/AiRoutes.h"

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <map>
#include <string>
#include <vector>

TEST_CASE("AI contract codec rejects a non-object nudge facts field") {
  const nlohmann::json body = {
      {"kind", "unknown"}, {"project_id", "p1"}, {"created_at", 0}, {"facts", nlohmann::json::array()}};
  REQUIRE_THROWS_AS(ai_json::decode<holder::api::AiNudgeCandidate>(body), std::invalid_argument);
}

TEST_CASE("AI contract preserves omitted and explicit null update fields") {
  const auto omitted = ai_json::decode<holder::api::AiRunnerUpdateRequest>(nlohmann::json::object());
  REQUIRE_FALSE(omitted.base_url.has_value());

  const auto cleared = ai_json::decode<holder::api::AiRunnerUpdateRequest>(
      nlohmann::json{{"base_url", nullptr}});
  REQUIRE(cleared.base_url.has_value());
  REQUIRE_FALSE(cleared.base_url->has_value());
}

TEST_CASE("AI request body rejects negative int64 overflow without rejecting quoted text") {
  REQUIRE_THROWS_AS(ai_http::check_negative_integer_tokens(
      R"({"updated_at":-9223372036854775809})"), std::invalid_argument);
  REQUIRE_NOTHROW(ai_http::check_negative_integer_tokens(
      R"({"updated_at":-9223372036854775808.0,"note":"-9223372036854775809"})"));
}

TEST_CASE("Generated codecs count Unicode characters and reject malformed UTF-8", "[generated]") {
  const std::string text = "\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
  REQUIRE(ai_json::utf8_length(text) == 3);
  REQUIRE(contract_json::utf8_length(text) == 3);
  CHECK(ai_json::check_text(text, "text", 3, 3) == text);
  CHECK(contract_json::check_text(text, "text", 3, 3) == text);
  CHECK_THROWS_AS(ai_json::check_text(text, "text", 4, 8), std::invalid_argument);
  CHECK_THROWS_AS(contract_json::check_text(text, "text", 1, 2), std::invalid_argument);
  for (const std::string invalid : {
           "\x80", "\xC0\xAF", "\xC2", "\xE2\x28\xA1", "\xED\xA0\x80",
           "\xE0\x80\x80", "\xF0\x80\x80\x80", "\xF4\x90\x80\x80", "\xFF",
       }) {
    CAPTURE(invalid.size());
    CHECK_FALSE(ai_json::utf8_length(invalid));
    CHECK_FALSE(contract_json::utf8_length(invalid));
    CHECK_THROWS_AS(ai_json::check_text(invalid, "text", 0, 100), std::invalid_argument);
    CHECK_THROWS_AS(contract_json::check_text(invalid, "text", 0, 100), std::invalid_argument);
  }
  const std::string nul("a\0b", 3);
  CHECK_THROWS_AS(ai_json::check_text(nul, "text", 0, 100), std::invalid_argument);
  CHECK_THROWS_AS(contract_json::check_text(nul, "text", 0, 100), std::invalid_argument);
  CHECK_THROWS_AS(ai_json::check_text("", "text", 1, 100), std::invalid_argument);
  CHECK_THROWS_AS(contract_json::check_text("", "text", 1, 100), std::invalid_argument);
}

TEST_CASE("Generated integer codecs accept whole JSON numbers and enforce signed limits", "[generated]") {
  using json = nlohmann::json;
  for (const auto& value : {json(17), json(17U), json(17.0), json::parse("1.7e1")}) {
    CHECK(ai_json::decode<std::int32_t>(value) == 17);
    CHECK(contract_json::decode<std::int32_t>(value) == 17);
    CHECK(ai_json::decode<long long>(value) == 17);
    CHECK(contract_json::decode<long long>(value) == 17);
  }
  for (const auto& value : {
           json(-2147483649LL), json(2147483648ULL), json(2147483648.0),
           json(-2147483649.0), json(0.5), json("17"), json(true), json(nullptr),
       }) {
    CHECK_THROWS_AS(ai_json::decode<std::int32_t>(value), std::invalid_argument);
    CHECK_THROWS_AS(contract_json::decode<std::int32_t>(value), std::invalid_argument);
  }
  CHECK(ai_json::decode<long long>(json::parse("-9223372036854775808")) ==
        std::numeric_limits<long long>::min());
  CHECK(contract_json::decode<long long>(json::parse("9223372036854775807")) ==
        std::numeric_limits<long long>::max());
  for (const auto& value : {json::parse("9223372036854775808"), json::parse("1e20")}) {
    CHECK_THROWS_AS(ai_json::decode<long long>(value), std::invalid_argument);
    CHECK_THROWS_AS(contract_json::decode<long long>(value), std::invalid_argument);
  }
}

TEST_CASE("Generated codecs reject non-finite numbers and incorrect container types", "[generated]") {
  using json = nlohmann::json;
  for (const double invalid : {
           std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
           std::numeric_limits<double>::quiet_NaN(),
       }) {
    CHECK_THROWS_AS(ai_json::decode<double>(json(invalid)), std::invalid_argument);
    CHECK_THROWS_AS(contract_json::decode<double>(json(invalid)), std::invalid_argument);
    CHECK_THROWS_AS(ai_json::check_finite(invalid, "number"), std::invalid_argument);
    CHECK_THROWS_AS(contract_json::check_finite(invalid, "number"), std::invalid_argument);
  }
  CHECK_THROWS_AS(ai_json::decode<double>(json("1")), std::invalid_argument);
  CHECK_THROWS_AS(contract_json::decode<double>(json("1")), std::invalid_argument);
  CHECK(ai_json::decode<double>(json(1)) == 1.0);
  CHECK(contract_json::decode<double>(json(1)) == 1.0);
  CHECK_THROWS_AS(ai_json::decode<std::vector<std::string>>(json::object()), std::invalid_argument);
  CHECK_THROWS_AS(contract_json::decode<std::vector<std::string>>(json::object()), std::invalid_argument);
  using Metadata = std::map<std::string, std::vector<std::string>>;
  CHECK_THROWS_AS(ai_json::decode<Metadata>(json::array()), std::invalid_argument);
  CHECK_THROWS_AS(contract_json::decode<Metadata>(json::array()), std::invalid_argument);
  CHECK_THROWS_AS(ai_json::decode<Metadata>(json{{"topic", 1}}), std::invalid_argument);
  CHECK_THROWS_AS(contract_json::decode<Metadata>(json{{"topic", 1}}), std::invalid_argument);
  const json valid{{"topic", {"work", "home"}}};
  CHECK(ai_json::encode(ai_json::decode<Metadata>(valid)) == valid);
  CHECK(contract_json::encode(contract_json::decode<Metadata>(valid)) == valid);
}

TEST_CASE("Generated card requests preserve omitted and explicitly cleared parents", "[generated]") {
  using holder::contract::CardUpdate;
  using json = nlohmann::json;
  const auto omitted = contract_json::decode<CardUpdate>(json::object());
  CHECK_FALSE(omitted.parent_card_id);
  const auto cleared = contract_json::decode<CardUpdate>({{"parent_card_id", nullptr}});
  REQUIRE(cleared.parent_card_id);
  CHECK_FALSE(*cleared.parent_card_id);
  CHECK(contract_json::encode(cleared) == json{{"parent_card_id", nullptr}});
  const auto changed = contract_json::decode<CardUpdate>(
      {{"parent_card_id", "parent"}, {"content", ""}, {"sort_key", 1.5}});
  REQUIRE(changed.parent_card_id);
  CHECK(*changed.parent_card_id == "parent");
  CHECK(changed.content == "");
  CHECK(changed.sort_key == 1.5);
  CHECK(contract_json::encode(changed) ==
        json{{"parent_card_id", "parent"}, {"content", ""}, {"sort_key", 1.5}});

  for (const std::string field : {"title", "content", "sort_key", "updated_at"}) {
    CAPTURE(field);
    CHECK_THROWS_AS(contract_json::decode<CardUpdate>(json{{field, nullptr}}), std::invalid_argument);
  }
}

TEST_CASE("Generated request codecs reject unknown fields and null required values", "[generated]") {
  using json = nlohmann::json;
  const auto check = []<typename T>(const json& valid, const std::vector<std::string>& required) {
    REQUIRE_NOTHROW(contract_json::decode<T>(valid));
    CHECK_THROWS_AS(contract_json::decode<T>(json::array()), std::invalid_argument);
    auto unknown = valid;
    unknown["unknown_field"] = true;
    CHECK_THROWS_AS(contract_json::decode<T>(unknown), std::invalid_argument);
    for (const auto& field : required) {
      CAPTURE(field);
      auto missing = valid;
      missing.erase(field);
      CHECK_THROWS_AS(contract_json::decode<T>(missing), std::invalid_argument);
      auto null = valid;
      null[field] = nullptr;
      CHECK_THROWS_AS(contract_json::decode<T>(null), std::invalid_argument);
    }
  };
  check.operator()<holder::contract::NewCard>({{"title", "Card"}}, {"title"});
  check.operator()<holder::contract::NewResource>({{"type", "file"}, {"label", "Asset"}},
                                                {"type", "label"});
  check.operator()<holder::contract::NewStorageLocation>(
      {{"project_id", "p"}, {"name", "Storage"}, {"provider", "local_directory"}},
      {"project_id", "name", "provider"});
  check.operator()<holder::contract::NewLink>(
      {{"target_type", "card"}, {"target_id", "target"}}, {"target_type", "target_id"});
  check.operator()<holder::contract::MilestoneDate>(
      {{"role", "due"}, {"date", "2026-12-01"}}, {"role"});
  check.operator()<holder::contract::RecoveryTokenImportRequest>(
      {{"pin", "1234"}, {"recovery_token", "token"}}, {"pin", "recovery_token"});
  check.operator()<holder::contract::LinkKindLabels>(
      {{"kind", "relates_to"}, {"forward_label", "Related"}, {"reverse_label", "Related"}},
      {"kind", "forward_label", "reverse_label"});
  check.operator()<holder::contract::CardReferenceResolveRequest>(
      {{"project_id", "project"}, {"reference", "card"}, {"scope", "live"}},
      {"project_id", "reference", "scope"});
  check.operator()<holder::contract::CardMove>({{"intent", "into"}}, {"intent"});
  check.operator()<holder::contract::NewMilestone>({{"dates", json::array()}}, {"dates"});
  check.operator()<holder::contract::NewProject>({{"name", "Project"}}, {"name"});
  check.operator()<holder::contract::RecoveryTokenExportRequest>({{"pin", "1234"}}, {"pin"});
}

TEST_CASE("Generated optional request fields distinguish omission from forbidden nulls", "[generated]") {
  using json = nlohmann::json;
  const auto check = []<typename T>(const json& valid, const std::vector<std::string>& fields) {
    REQUIRE_NOTHROW(contract_json::decode<T>(valid));
    for (const auto& field : fields) {
      CAPTURE(field);
      auto invalid = valid;
      invalid[field] = nullptr;
      CHECK_THROWS_AS(contract_json::decode<T>(invalid), std::invalid_argument);
    }
  };
  check.operator()<holder::contract::NewCard>(
      {{"title", "Card"}}, {"content", "sort_key", "created_at", "updated_at"});
  check.operator()<holder::contract::NewResource>(
      {{"type", "file"}, {"label", "Asset"}}, {"resource_id", "metadata", "created_at", "updated_at"});
  check.operator()<holder::contract::NewStorageLocation>(
      {{"project_id", "p"}, {"name", "Storage"}, {"provider", "local_directory"}},
      {"location_id", "configuration", "created_at", "updated_at"});
  check.operator()<holder::contract::StorageLocationUpdate>(
      json::object(), {"name", "configuration", "updated_at"});
  check.operator()<holder::contract::MilestoneDate>(
      {{"role", "due"}}, {"date", "datetime"});
  check.operator()<holder::contract::HierarchyRepairRequest>(json::object(), {"apply"});
  check.operator()<holder::contract::CardMove>({{"intent", "into"}}, {"target_card_id"});
  check.operator()<holder::contract::NewLink>(
      {{"target_type", "card"}, {"target_id", "target"}}, {"target_project_id", "kind", "created_at"});
  check.operator()<holder::contract::MilestoneUpdate>(json::object(), {"dates"});
  check.operator()<holder::contract::ResourceUpdate>(json::object(), {"type", "updated_at"});
  check.operator()<holder::contract::NewProject>(
      {{"name", "Project"}}, {"project_id", "root_path", "privacy_mode", "created_at", "updated_at"});
  check.operator()<holder::contract::PushOptions>(json::object(), {"branch", "set_upstream"});
}

TEST_CASE("Generated AI update fields preserve clear operations and reject incorrect types", "[generated]") {
  using Request = holder::api::AiRunnerUpdateRequest;
  using json = nlohmann::json;
  const auto cleared = ai_json::decode<Request>(
      {{"name", nullptr}, {"base_url", nullptr}, {"enabled", nullptr}});
  CHECK_FALSE(cleared.name);
  REQUIRE(cleared.base_url);
  CHECK_FALSE(cleared.enabled);
  CHECK_FALSE(*cleared.base_url);
  CHECK(ai_json::encode(cleared) == json{{"base_url", nullptr}});

  const json values{{"name", "Office"}, {"base_url", "http://127.0.0.1:11434"}, {"enabled", false}};
  const auto decoded = ai_json::decode<Request>(values);
  CHECK(ai_json::encode(decoded) == values);
  for (const auto& invalid : {
           json{{"name", 1}}, json{{"base_url", true}}, json{{"enabled", "false"}},
       }) {
    CHECK_THROWS_AS(ai_json::decode<Request>(invalid), std::invalid_argument);
  }
}

TEST_CASE("Generated query decoding preserves URI and query escaping semantics", "[generated]") {
  CHECK(contract_http::decode_uri_component("a+b%2Fc", false) == "a+b/c");
  CHECK(contract_http::decode_uri_component("a+b%2fc", true) == "a b/c");
  CHECK(contract_http::decode_query("name=a+b&path=%2Ftmp%2Ffile&flag") ==
        std::map<std::string, std::string>{{"name", "a b"}, {"path", "/tmp/file"}, {"flag", ""}});
  for (const std::string query : {"key=one&key=two", "bad=%", "bad=%XZ"}) {
    CHECK_THROWS_AS(contract_http::decode_query(query), std::invalid_argument);
  }
  for (const std::string body : {
           R"({"timestamp":-9223372036854775809})",
           R"({"timestamp":-99999999999999999999999999999999999999})",
       }) {
    CHECK_THROWS_AS(ai_http::decode_body<holder::api::AiThreadCreateRequest>(body),
                    std::invalid_argument);
  }
  CHECK_NOTHROW(ai_http::check_negative_integer_tokens(
      R"({"text":"quoted \"-9223372036854775809\"","number":-1e4})"));
}

TEST_CASE("Generated LinkKindLabels encoding keeps forward and reverse labels distinct", "[generated]") {
  const holder::contract::LinkKindLabels labels{
      .kind = "depends_on",
      .forward_label = "Depends on",
      .reverse_label = "Required by",
  };
  const nlohmann::json expected{
      {"kind", "depends_on"},
      {"forward_label", "Depends on"},
      {"reverse_label", "Required by"},
  };

  const auto encoded = contract_json::encode(labels);
  REQUIRE(encoded == expected);
  const auto decoded = contract_json::decode<holder::contract::LinkKindLabels>(encoded);
  CHECK(decoded.kind == labels.kind);
  CHECK(decoded.forward_label == labels.forward_label);
  CHECK(decoded.reverse_label == labels.reverse_label);
}

TEST_CASE("Generated enum codecs preserve every wire value and reject invalid enum values", "[generated]") {
  const auto check = []<typename T>(std::initializer_list<std::pair<T, const char*>> values) {
    for (const auto& [value, wire] : values) {
      CAPTURE(wire);
      CHECK(contract_json::encode(value) == wire);
      CHECK(contract_json::decode<T>(wire) == value);
    }
    CHECK_THROWS_AS(contract_json::encode(static_cast<T>(-1)), std::invalid_argument);
    CHECK_THROWS_AS(contract_json::decode<T>("not-a-contract-value"), std::invalid_argument);
    CHECK_THROWS_AS(contract_json::decode<T>(123), std::invalid_argument);
  };
  using namespace holder::contract;
  check.operator()<ProjectStorageLocationProvider>({
      {ProjectStorageLocationProvider::LocalDirectory, "local_directory"},
      {ProjectStorageLocationProvider::S3Compatible, "s3_compatible"},
      {ProjectStorageLocationProvider::GoogleDrive, "google-drive"},
  });
  check.operator()<CardBreadcrumbKind>({
      {CardBreadcrumbKind::Project, "project"},
      {CardBreadcrumbKind::Card, "card"},
  });
  check.operator()<CardReferenceResolveDataStatus>({
      {CardReferenceResolveDataStatus::Resolved, "resolved"},
      {CardReferenceResolveDataStatus::Ambiguous, "ambiguous"},
      {CardReferenceResolveDataStatus::NotFound, "not_found"},
  });
  check.operator()<LinkSourceType>({
      {LinkSourceType::Card, "card"},
      {LinkSourceType::AiMessage, "ai_message"},
  });
  check.operator()<LinkTargetType>({
      {LinkTargetType::Card, "card"},
      {LinkTargetType::AiMessage, "ai_message"},
      {LinkTargetType::AiThread, "ai_thread"},
      {LinkTargetType::Resource, "resource"},
  });
  check.operator()<AssetPlacementEncoding>({
      {AssetPlacementEncoding::Plain, "plain"},
      {AssetPlacementEncoding::HolderAssetV1, "holder_asset_v1"},
  });
  check.operator()<ProjectPrivacyMode>({
      {ProjectPrivacyMode::EncryptedGit, "encrypted_git"},
      {ProjectPrivacyMode::Plain, "plain"},
  });
  check.operator()<ProjectEncryptionCheckPrivacyMode>({
      {ProjectEncryptionCheckPrivacyMode::EncryptedGit, "encrypted_git"},
      {ProjectEncryptionCheckPrivacyMode::Plain, "plain"},
  });
  check.operator()<RecoverySnapshotLinkTargetType>({
      {RecoverySnapshotLinkTargetType::Card, "card"},
      {RecoverySnapshotLinkTargetType::AiMessage, "ai_message"},
      {RecoverySnapshotLinkTargetType::AiThread, "ai_thread"},
      {RecoverySnapshotLinkTargetType::Resource, "resource"},
  });
  check.operator()<RecoverySnapshotPlacementEncoding>({
      {RecoverySnapshotPlacementEncoding::Plain, "plain"},
      {RecoverySnapshotPlacementEncoding::HolderAssetV1, "holder_asset_v1"},
  });
  check.operator()<RecoverySnapshotLocationProvider>({
      {RecoverySnapshotLocationProvider::LocalDirectory, "local_directory"},
      {RecoverySnapshotLocationProvider::S3Compatible, "s3_compatible"},
      {RecoverySnapshotLocationProvider::GoogleDrive, "google-drive"},
  });
  check.operator()<RecoverySnapshotCardPrivacyMode>({
      {RecoverySnapshotCardPrivacyMode::EncryptedGit, "encrypted_git"},
      {RecoverySnapshotCardPrivacyMode::Plain, "plain"},
  });
  check.operator()<RecoveryTokenGlobalImportResultPullStatus>({
      {RecoveryTokenGlobalImportResultPullStatus::NotAttempted, "not_attempted"},
      {RecoveryTokenGlobalImportResultPullStatus::Succeeded, "succeeded"},
      {RecoveryTokenGlobalImportResultPullStatus::Failed, "failed"},
  });
  check.operator()<PullPhaseStatus>({
      {PullPhaseStatus::NotAttempted, "not_attempted"},
      {PullPhaseStatus::Succeeded, "succeeded"},
      {PullPhaseStatus::RemoteUnset, "remote_unset"},
      {PullPhaseStatus::Failed, "failed"},
  });
  check.operator()<PushPhaseStatus>({
      {PushPhaseStatus::NotAttempted, "not_attempted"},
      {PushPhaseStatus::Pushed, "pushed"},
      {PushPhaseStatus::UpToDate, "up_to_date"},
      {PushPhaseStatus::AuthFailed, "auth_failed"},
      {PushPhaseStatus::NotFound, "not_found"},
      {PushPhaseStatus::NetworkError, "network_error"},
      {PushPhaseStatus::NonFastForward, "non_fast_forward"},
      {PushPhaseStatus::RemoteUnset, "remote_unset"},
      {PushPhaseStatus::UnknownError, "unknown_error"},
  });
  check.operator()<SyncResultStatus>({
      {SyncResultStatus::Succeeded, "succeeded"},
      {SyncResultStatus::Failed, "failed"},
  });
  check.operator()<RemoteTestResultStatus>({
      {RemoteTestResultStatus::Reachable, "reachable"},
      {RemoteTestResultStatus::AuthFailed, "auth_failed"},
      {RemoteTestResultStatus::NotFound, "not_found"},
      {RemoteTestResultStatus::NetworkError, "network_error"},
      {RemoteTestResultStatus::InvalidRemoteUrl, "invalid_remote_url"},
      {RemoteTestResultStatus::RemoteUnset, "remote_unset"},
      {RemoteTestResultStatus::UnknownError, "unknown_error"},
  });
  check.operator()<HistoryAffectedObjectKind>({
      {HistoryAffectedObjectKind::Card, "card"},
      {HistoryAffectedObjectKind::Resource, "resource"},
      {HistoryAffectedObjectKind::Location, "location"},
      {HistoryAffectedObjectKind::AiData, "ai_data"},
      {HistoryAffectedObjectKind::ProjectSettings, "project_settings"},
      {HistoryAffectedObjectKind::Unknown, "unknown"},
  });
  check.operator()<TrashItemType>({
      {TrashItemType::Card, "card"},
      {TrashItemType::AiMessage, "ai_message"},
  });
  check.operator()<NewStorageLocationProvider>({
      {NewStorageLocationProvider::LocalDirectory, "local_directory"},
      {NewStorageLocationProvider::S3Compatible, "s3_compatible"},
      {NewStorageLocationProvider::GoogleDrive, "google-drive"},
  });
  check.operator()<CardReferenceResolveRequestScope>({
      {CardReferenceResolveRequestScope::Live, "live"},
      {CardReferenceResolveRequestScope::Trashed, "trashed"},
      {CardReferenceResolveRequestScope::Either, "either"},
  });
  check.operator()<CardMoveIntent>({
      {CardMoveIntent::Into, "into"},
      {CardMoveIntent::Before, "before"},
      {CardMoveIntent::After, "after"},
      {CardMoveIntent::Left, "left"},
      {CardMoveIntent::Right, "right"},
      {CardMoveIntent::ToStart, "to_start"},
      {CardMoveIntent::ToEnd, "to_end"},
      {CardMoveIntent::UpLevel, "up_level"},
  });
  check.operator()<NewLinkTargetType>({
      {NewLinkTargetType::Card, "card"},
      {NewLinkTargetType::AiMessage, "ai_message"},
      {NewLinkTargetType::AiThread, "ai_thread"},
      {NewLinkTargetType::Resource, "resource"},
  });
  check.operator()<NewProjectPrivacyMode>({
      {NewProjectPrivacyMode::EncryptedGit, "encrypted_git"},
      {NewProjectPrivacyMode::Plain, "plain"},
  });
  check.operator()<RecoverySnapshotRestorePrivacyMode>({
      {RecoverySnapshotRestorePrivacyMode::EncryptedGit, "encrypted_git"},
      {RecoverySnapshotRestorePrivacyMode::Plain, "plain"},
  });
}

TEST_CASE("Generated scalar checks accept finite values and reject overlong Unicode text", "[generated]") {
  CHECK(ai_json::check_finite(1.25, "number") == 1.25);
  CHECK(contract_json::check_finite(-1.25, "number") == -1.25);
  const std::string text = "\xC3\xA9\xC3\xA9";
  CHECK(ai_json::check_text(text, "label", 0, 2) == text);
  CHECK(contract_json::check_text(text, "label", 0, 2) == text);
  CHECK_THROWS_AS(ai_json::check_text(text, "label", 0, 1), std::invalid_argument);
  CHECK_THROWS_AS(contract_json::check_text(text, "label", 0, 1), std::invalid_argument);
  CHECK_NOTHROW(ai_http::check_negative_integer_tokens(R"({"first":-12,"second":-34})"));
}

TEST_CASE("Generated request codecs retain explicit timestamps and move parent values", "[generated]") {
  using namespace holder::contract;
  const auto location = contract_json::decode<NewStorageLocation>({
      {"project_id", "project"}, {"name", "Disk"}, {"provider", "local_directory"},
      {"created_at", 12}, {"updated_at", 34}});
  CHECK(location.created_at == 12);
  CHECK(location.updated_at == 34);
  const auto resource = contract_json::decode<NewResource>({
      {"type", "file"}, {"label", "Asset"}, {"created_at", 56}, {"updated_at", 78}});
  CHECK(resource.created_at == 56);
  CHECK(resource.updated_at == 78);
  const auto update = contract_json::decode<ResourceUpdate>({{"updated_at", 90}});
  CHECK(update.updated_at == 90);
  const auto link = contract_json::decode<NewLink>({
      {"target_type", "card"}, {"target_id", "target"}, {"created_at", 123}});
  CHECK(link.created_at == 123);
  const auto move = contract_json::decode<CardMove>({
      {"intent", "to_end"}, {"parent_card_id", "parent"}});
  REQUIRE(move.parent_card_id);
  REQUIRE(*move.parent_card_id);
  CHECK(**move.parent_card_id == "parent");
  for (const std::string kind : {"ai_message", "ai_thread", "resource"}) {
    CAPTURE(kind);
    const auto value = contract_json::decode<NewLink>({{"target_type", kind}, {"target_id", "target"}});
    CHECK(contract_json::encode(value).at("target_type") == kind);
  }
  CHECK_THROWS_AS(contract_json::decode<NewLink>({
      {"target_type", "unknown"}, {"target_id", "target"}}), std::invalid_argument);
}
