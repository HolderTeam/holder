// Transport conformance tests. This binary deliberately links a stand-in core,
// not the real core: it checks every declared operation error at the HTTP boundary.
// These definitions mirror Contract.h; changed operation signatures fail to compile/link.
#include "api/generated/ContractRoutes.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <functional>
#include <map>
#include <string>
#include <string_view>

struct holder_context {
  std::function<void(std::string_view)> invoke;
};

namespace holder::contract {
ProjectStorageLocation get_storage_location(holder_context& context, std::string) {
  context.invoke("get_storage_location");
  return {};
}
ProjectStorageLocation create_storage_location(holder_context& context, const NewStorageLocation&) {
  context.invoke("create_storage_location");
  return {};
}
ProjectStorageLocation
update_storage_location(holder_context& context, std::string, const StorageLocationUpdate&) {
  context.invoke("update_storage_location");
  return {};
}
LocationDeleteResult delete_storage_location(holder_context& context, std::string) {
  context.invoke("delete_storage_location");
  return {};
}
Card get_card(holder_context& context, std::string, std::string) {
  context.invoke("get_card");
  return {};
}
CardMetadata get_card_metadata(holder_context& context, std::string, std::string) {
  context.invoke("get_card_metadata");
  return {};
}
std::vector<CardBreadcrumb> get_card_breadcrumbs(
    holder_context& context,
    std::string,
    std::string
) {
  context.invoke("get_card_breadcrumbs");
  return {};
}
CardReferenceResolveData
resolve_card_reference(holder_context& context, const CardReferenceResolveRequest&) {
  context.invoke("resolve_card_reference");
  return {};
}
Page<CardListItem> list_cards(
    holder_context& context,
    std::string,
    std::optional<ListCardsParent>,
    std::optional<std::string>,
    ListCardsOrder,
    std::int32_t,
    std::optional<std::string>,
    bool,
    bool
) {
  context.invoke("list_cards");
  return {};
}
Card create_card(holder_context& context, std::string, const NewCard&) {
  context.invoke("create_card");
  return {};
}
Card update_card(holder_context& context, std::string, std::string, const CardUpdate&) {
  context.invoke("update_card");
  return {};
}
Card trash_card(holder_context& context, std::string, std::string, TrashCardChildren) {
  context.invoke("trash_card");
  return {};
}
Card restore_card(holder_context& context, std::string, std::string) {
  context.invoke("restore_card");
  return {};
}
Card move_card(holder_context& context, std::string, std::string, const CardMove&) {
  context.invoke("move_card");
  return {};
}
Card add_card_tag(holder_context& context, std::string, std::string, std::string) {
  context.invoke("add_card_tag");
  return {};
}
Card remove_card_tag(holder_context& context, std::string, std::string, std::string) {
  context.invoke("remove_card_tag");
  return {};
}
std::vector<TagCount> list_project_tags(holder_context& context, std::string) {
  context.invoke("list_project_tags");
  return {};
}
std::vector<LinkKindLabels> list_link_kinds(holder_context& context) {
  context.invoke("list_link_kinds");
  return {};
}
std::vector<Link> list_card_links(holder_context& context, std::string, std::string, bool) {
  context.invoke("list_card_links");
  return {};
}
std::vector<Link> list_card_backlinks(
    holder_context& context,
    std::string,
    std::string,
    ListCardBacklinksScope,
    bool
) {
  context.invoke("list_card_backlinks");
  return {};
}
Link add_card_link(holder_context& context, std::string, std::string, const NewLink&) {
  context.invoke("add_card_link");
  return {};
}
std::vector<Link>
remove_card_link(holder_context& context, std::string, std::string, RemoveCardLinkTargetType, std::string, std::string, std::optional<std::string>) {
  context.invoke("remove_card_link");
  return {};
}
std::vector<Milestone> list_card_milestones(holder_context& context, std::string, std::string) {
  context.invoke("list_card_milestones");
  return {};
}
Milestone
add_card_milestone(holder_context& context, std::string, std::string, const NewMilestone&) {
  context.invoke("add_card_milestone");
  return {};
}
Milestone
update_card_milestone(holder_context& context, std::string, std::string, std::string, const MilestoneUpdate&) {
  context.invoke("update_card_milestone");
  return {};
}
std::vector<Milestone> remove_card_milestone(
    holder_context& context,
    std::string,
    std::string,
    std::string
) {
  context.invoke("remove_card_milestone");
  return {};
}
std::vector<Milestone> list_milestones_by_day(
    holder_context& context,
    std::string,
    std::string,
    std::string,
    std::optional<std::string> role
) {
  REQUIRE(role);
  CHECK(*role == "due");
  context.invoke("list_milestones_by_day");
  return {};
}
std::vector<Milestone> list_milestones_by_instant(
    holder_context& context,
    std::string,
    long long,
    long long,
    std::optional<std::string> role
) {
  REQUIRE(role);
  CHECK(*role == "due");
  context.invoke("list_milestones_by_instant");
  return {};
}
Calendar
get_calendar(holder_context& context, std::string, std::string, std::string, std::optional<std::string>) {
  context.invoke("get_calendar");
  return {};
}
std::vector<Resource> list_resources(holder_context& context, std::string) {
  context.invoke("list_resources");
  return {};
}
Resource get_resource(holder_context& context, std::string, std::string) {
  context.invoke("get_resource");
  return {};
}
Resource create_resource(holder_context& context, std::string, const NewResource&) {
  context.invoke("create_resource");
  return {};
}
Resource update_resource(holder_context& context, std::string, std::string, const ResourceUpdate&) {
  context.invoke("update_resource");
  return {};
}
Resource delete_resource(holder_context& context, std::string, std::string) {
  context.invoke("delete_resource");
  return {};
}
std::vector<Link> list_resource_backlinks(holder_context& context, std::string, std::string, bool) {
  context.invoke("list_resource_backlinks");
  return {};
}
std::vector<Resource> list_card_resources(holder_context& context, std::string, std::string) {
  context.invoke("list_card_resources");
  return {};
}
std::vector<Resource> attach_card_resource(
    holder_context& context,
    std::string,
    std::string,
    std::string
) {
  context.invoke("attach_card_resource");
  return {};
}
std::vector<Resource> detach_card_resource(
    holder_context& context,
    std::string,
    std::string,
    std::string
) {
  context.invoke("detach_card_resource");
  return {};
}
std::vector<ProjectListItem> list_projects(
    holder_context& context,
    std::optional<std::string>,
    std::optional<long long>,
    std::optional<long long>,
    std::optional<ListProjectsOrder>,
    std::int32_t,
    std::int32_t,
    bool
) {
  context.invoke("list_projects");
  return {};
}
Project get_project(holder_context& context, std::string) {
  context.invoke("get_project");
  return {};
}
Project create_project(holder_context& context, const NewProject&) {
  context.invoke("create_project");
  return {};
}
Project update_project(holder_context& context, std::string, const ProjectUpdate&) {
  context.invoke("update_project");
  return {};
}
Project delete_project(holder_context& context, std::string) {
  context.invoke("delete_project");
  return {};
}
ProjectEncryptionCheck check_project_encryption(holder_context& context, std::string) {
  context.invoke("check_project_encryption");
  return {};
}
HierarchyRepair
repair_project_hierarchy(holder_context& context, std::string, const HierarchyRepairRequest&) {
  context.invoke("repair_project_hierarchy");
  return {};
}
RebuildResult rebuild_project(holder_context& context, const RebuildRequest&) {
  context.invoke("rebuild_project");
  return {};
}
ReindexResult reindex_search(holder_context& context) {
  context.invoke("reindex_search");
  return {};
}
ProjectRecoveryTokenExportResult
export_project_recovery_token(holder_context& context, std::string, const RecoveryTokenExportRequest&) {
  context.invoke("export_project_recovery_token");
  return {};
}
ProjectRecoveryTokenImportResult
import_project_recovery_token(holder_context& context, std::string, const RecoveryTokenImportRequest&) {
  context.invoke("import_project_recovery_token");
  return {};
}
RecoveryTokenGlobalImportResult
import_recovery_token_global(holder_context& context, const RecoveryTokenImportRequest&) {
  context.invoke("import_recovery_token_global");
  return {};
}
SyncState get_project_sync_state(holder_context& context, std::string) {
  context.invoke("get_project_sync_state");
  return {};
}
SyncResult pull_project(holder_context& context, std::string) {
  context.invoke("pull_project");
  return {};
}
SyncResult push_project(holder_context& context, std::string, const PushOptions&) {
  context.invoke("push_project");
  return {};
}
SyncResult sync_project(holder_context& context, std::string, const PushOptions&) {
  context.invoke("sync_project");
  return {};
}
DueSyncResult sync_project_if_due(
    holder_context& context,
    std::string,
    std::int32_t,
    std::int32_t
) {
  context.invoke("sync_project_if_due");
  return {};
}
RemoteTestResult test_project_remote(holder_context& context, std::string, const RemoteTest&) {
  context.invoke("test_project_remote");
  return {};
}
ProjectHistoryPage
list_project_history(holder_context& context, std::string, std::int32_t, std::optional<std::string>, std::optional<ListProjectHistoryKind>) {
  context.invoke("list_project_history");
  return {};
}
CardHistoryPage
list_card_history(holder_context& context, std::string, std::string, std::int32_t, std::optional<std::string>) {
  context.invoke("list_card_history");
  return {};
}
CardHistoryComparison compare_card_history(
    holder_context& context,
    std::string,
    std::string,
    std::string,
    std::optional<std::string>,
    CompareCardHistoryMode
) {
  context.invoke("compare_card_history");
  return {};
}
CardHistorySnapshot get_card_history_snapshot(
    holder_context& context,
    std::string,
    std::string,
    std::string
) {
  context.invoke("get_card_history_snapshot");
  return {};
}
CardHistoryRestoreResult restore_card_history_snapshot(
    holder_context& context,
    std::string,
    std::string,
    std::string
) {
  context.invoke("restore_card_history_snapshot");
  return {};
}
std::vector<TrashItem>
list_trash(holder_context& context, std::string, std::optional<std::string>) {
  context.invoke("list_trash");
  return {};
}
TrashEmptyResult empty_trash(holder_context& context, std::string, EmptyTrashType) {
  context.invoke("empty_trash");
  return {};
}
TrashDeleteResult delete_trash_item(holder_context& context, DeleteTrashItemType, std::string) {
  context.invoke("delete_trash_item");
  return {};
}
TrashDeleteResult delete_project_trash_item(
    holder_context& context,
    std::string,
    DeleteProjectTrashItemType,
    std::string
) {
  context.invoke("delete_project_trash_item");
  return {};
}
std::vector<SearchCard> search_cards(
    holder_context& context,
    std::string,
    std::string,
    std::int32_t,
    std::int32_t
) {
  context.invoke("search_cards");
  return {};
}
std::vector<SearchMessage> search_ai_messages(
    holder_context& context,
    std::string,
    std::string,
    std::int32_t,
    std::int32_t
) {
  context.invoke("search_ai_messages");
  return {};
}
} // namespace holder::contract

namespace {
struct ErrorCase {
  const char* code;
  int status;
  std::function<void()> raise;
};
struct RouteCase {
  const char* operation;
  const char* method;
  const char* path;
  const char* body;
  std::vector<ErrorCase> errors;
};
} // namespace

TEST_CASE(
    "Generated core dispatch maps declared failures for every operation",
    "[generated][dispatch]"
) {
  const std::vector<RouteCase> routes{
      {"get_storage_location",
       "GET",
       "/locations/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"create_storage_location",
       "POST",
       "/locations",
       "{\"project_id\":\"a\",\"name\":\"a\",\"provider\":\"local_directory\"}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"update_storage_location",
       "PATCH",
       "/locations/a",
       "{}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"delete_storage_location",
       "DELETE",
       "/locations/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"conflict",
            409,
            [] {
              throw holder::contract::Conflict("operation rejected");
            }},
       }},
      {"get_card",
       "GET",
       "/projects/a/cards/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"get_card_metadata",
       "GET",
       "/projects/a/cards/a/metadata",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"get_card_breadcrumbs",
       "GET",
       "/projects/a/cards/a/breadcrumbs",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"hierarchy_invalid",
            409,
            [] {
              throw holder::contract::HierarchyInvalid("operation rejected");
            }},
       }},
      {"resolve_card_reference",
       "POST",
       "/card-references/resolve",
       "{\"project_id\":\"a\",\"reference\":\"a\",\"scope\":\"live\"}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"list_cards",
       "GET",
       "/projects/a/cards",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"create_card",
       "POST",
       "/projects/a/cards",
       "{\"title\":\"a\",\"content\":\"a\",\"parent_card_id\":null}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"conflict",
            409,
            [] {
              throw holder::contract::Conflict("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"update_card",
       "PATCH",
       "/projects/a/cards/a",
       "{}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"invalid_move",
            422,
            [] {
              throw holder::contract::InvalidMove("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"trash_card",
       "DELETE",
       "/projects/a/cards/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"restore_card",
       "POST",
       "/projects/a/cards/a/restore",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"conflict",
            409,
            [] {
              throw holder::contract::Conflict("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"move_card",
       "POST",
       "/projects/a/cards/a/move",
       "{\"intent\":\"into\"}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"invalid_move",
            422,
            [] {
              throw holder::contract::InvalidMove("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"add_card_tag",
       "PUT",
       "/projects/a/cards/a/tags/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"remove_card_tag",
       "DELETE",
       "/projects/a/cards/a/tags/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"conflict",
            409,
            [] {
              throw holder::contract::Conflict("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"list_project_tags",
       "GET",
       "/projects/a/tags",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"list_link_kinds", "GET", "/link-kinds", "", {}},
      {"list_card_links",
       "GET",
       "/projects/a/cards/a/links",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"list_card_backlinks",
       "GET",
       "/projects/a/cards/a/backlinks",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"add_card_link",
       "POST",
       "/projects/a/cards/a/links",
       "{\"target_type\":\"card\",\"target_id\":\"a\",\"kind\":\"a\",\"label\":null}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"remove_card_link",
       "DELETE",
       "/projects/a/cards/a/links?target_type=card&target_id=a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"list_card_milestones",
       "GET",
       "/projects/a/cards/a/milestones",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"add_card_milestone",
       "POST",
       "/projects/a/cards/a/milestones",
       "{\"dates\":[],\"kind\":null,\"description\":null}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"update_card_milestone",
       "PATCH",
       "/projects/a/cards/a/milestones/a",
       "{}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"remove_card_milestone",
       "DELETE",
       "/projects/a/cards/a/milestones/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"list_milestones_by_day",
       "GET",
       "/projects/a/milestones/days?first_day=2026-01-01&last_day=2026-01-02&role=due",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"list_milestones_by_instant",
       "GET",
       "/projects/a/milestones/instants?first_instant=1&last_instant=1&role=due",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"get_calendar",
       "GET",
       "/projects/a/calendar?first_day=2026-01-01&last_day=2026-01-02",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"list_resources",
       "GET",
       "/projects/a/resources",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"get_resource",
       "GET",
       "/projects/a/resources/aaaa",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"create_resource",
       "POST",
       "/projects/a/resources",
       "{\"type\":\"a\",\"label\":\"a\"}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"conflict",
            409,
            [] {
              throw holder::contract::Conflict("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"update_resource",
       "PATCH",
       "/projects/a/resources/aaaa",
       "{}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"delete_resource",
       "DELETE",
       "/projects/a/resources/aaaa",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"list_resource_backlinks",
       "GET",
       "/projects/a/resources/aaaa/backlinks?include_deleted=true",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"list_card_resources",
       "GET",
       "/projects/a/cards/a/resources",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"attach_card_resource",
       "PUT",
       "/projects/a/cards/a/resources/aaaa",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"detach_card_resource",
       "DELETE",
       "/projects/a/cards/a/resources/aaaa",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"list_projects", "GET", "/projects", "", {}},
      {"get_project",
       "GET",
       "/projects/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"create_project",
       "POST",
       "/projects",
       "{\"name\":\"a\",\"git_remote_url\":null,\"git_provider\":null,\"privacy_mode\":\"encrypted_git\"}",
       {
           {"conflict",
            409,
            [] {
              throw holder::contract::Conflict("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"update_project",
       "PATCH",
       "/projects/a",
       "{}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"delete_project",
       "DELETE",
       "/projects/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"check_project_encryption",
       "GET",
       "/projects/a/encryption-check",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"repair_project_hierarchy",
       "POST",
       "/projects/a/repair-hierarchy",
       "{\"apply\":false}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"rebuild_project",
       "POST",
       "/rebuild",
       "{\"project_id\":\"a\"}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"rebuild_failed",
            400,
            [] {
              throw holder::contract::RebuildFailed("operation rejected");
            }},
       }},
      {"reindex_search", "POST", "/reindex", "", {}},
      {"export_project_recovery_token",
       "POST",
       "/projects/a/recovery-token/export",
       "{\"pin\":\"a\"}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"import_project_recovery_token",
       "POST",
       "/projects/a/recovery-token/import",
       "{\"pin\":\"1234\",\"recovery_token\":\"token\"}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"recovery_token_invalid",
            400,
            [] {
              throw holder::contract::RecoveryTokenInvalid("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"import_recovery_token_global",
       "POST",
       "/recovery-token/import",
       "{\"pin\":\"1234\",\"recovery_token\":\"token\"}",
       {
           {"recovery_token_invalid",
            400,
            [] {
              throw holder::contract::RecoveryTokenInvalid("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"get_project_sync_state",
       "GET",
       "/projects/a/git/sync-status",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"pull_project",
       "POST",
       "/projects/a/git/pull",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"push_project",
       "POST",
       "/projects/a/git/push",
       "{\"set_upstream\":false}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"sync_project",
       "POST",
       "/projects/a/git/sync",
       "{\"set_upstream\":false}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"sync_project_if_due",
       "POST",
       "/projects/a/git/sync-if-due",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"test_project_remote",
       "POST",
       "/projects/a/git/test-remote",
       "{}",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
       }},
      {"list_project_history",
       "GET",
       "/projects/a/history",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"history_key_unavailable",
            409,
            [] {
              throw holder::contract::HistoryKeyUnavailable("operation rejected");
            }},
           {"history_response_too_large",
            413,
            [] {
              throw holder::contract::HistoryResponseTooLarge("operation rejected");
            }},
           {"history_unavailable",
            503,
            [] {
              throw holder::contract::HistoryUnavailable("operation rejected");
            }},
       }},
      {"list_card_history",
       "GET",
       "/projects/a/history/cards/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"history_key_unavailable",
            409,
            [] {
              throw holder::contract::HistoryKeyUnavailable("operation rejected");
            }},
           {"history_response_too_large",
            413,
            [] {
              throw holder::contract::HistoryResponseTooLarge("operation rejected");
            }},
           {"history_unavailable",
            503,
            [] {
              throw holder::contract::HistoryUnavailable("operation rejected");
            }},
       }},
      {"compare_card_history",
       "GET",
       "/projects/a/history/cards/a/compare?to_oid=aaaaaaaa",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"revision_ambiguous",
            409,
            [] {
              throw holder::contract::RevisionAmbiguous("operation rejected");
            }},
           {"history_key_unavailable",
            409,
            [] {
              throw holder::contract::HistoryKeyUnavailable("operation rejected");
            }},
           {"history_response_too_large",
            413,
            [] {
              throw holder::contract::HistoryResponseTooLarge("operation rejected");
            }},
           {"history_unavailable",
            503,
            [] {
              throw holder::contract::HistoryUnavailable("operation rejected");
            }},
       }},
      {"get_card_history_snapshot",
       "GET",
       "/projects/a/history/cards/a/snapshot?oid=aaaaaaaa",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"revision_ambiguous",
            409,
            [] {
              throw holder::contract::RevisionAmbiguous("operation rejected");
            }},
           {"history_key_unavailable",
            409,
            [] {
              throw holder::contract::HistoryKeyUnavailable("operation rejected");
            }},
           {"history_response_too_large",
            413,
            [] {
              throw holder::contract::HistoryResponseTooLarge("operation rejected");
            }},
           {"history_unavailable",
            503,
            [] {
              throw holder::contract::HistoryUnavailable("operation rejected");
            }},
       }},
      {"restore_card_history_snapshot",
       "POST",
       "/projects/a/history/cards/a/restore?oid=aaaaaaaa",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"revision_ambiguous",
            409,
            [] {
              throw holder::contract::RevisionAmbiguous("operation rejected");
            }},
           {"history_key_unavailable",
            409,
            [] {
              throw holder::contract::HistoryKeyUnavailable("operation rejected");
            }},
           {"history_unavailable",
            503,
            [] {
              throw holder::contract::HistoryUnavailable("operation rejected");
            }},
       }},
      {"list_trash", "GET", "/trash?project_id=a", "", {}},
      {"empty_trash",
       "DELETE",
       "/trash?project_id=a",
       "",
       {
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"delete_trash_item",
       "DELETE",
       "/trash/card/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"not_trashed",
            409,
            [] {
              throw holder::contract::NotTrashed("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"delete_project_trash_item",
       "DELETE",
       "/projects/a/trash/card/a",
       "",
       {
           {"not_found",
            404,
            [] {
              throw holder::contract::NotFound("operation rejected");
            }},
           {"not_trashed",
            409,
            [] {
              throw holder::contract::NotTrashed("operation rejected");
            }},
           {"keyring_unavailable",
            503,
            [] {
              throw holder::contract::KeyringUnavailable("operation rejected");
            }},
           {"key_missing",
            409,
            [] {
              throw holder::contract::KeyMissing("operation rejected");
            }},
           {"data_corrupted",
            500,
            [] {
              throw holder::contract::DataCorrupted("operation rejected");
            }},
       }},
      {"search_cards", "GET", "/search/cards?project_id=a&q=a", "", {}},
      {"search_ai_messages", "GET", "/search/ai?project_id=a&q=a", "", {}},
  };
  for (const auto& route : routes) {
    CAPTURE(route.operation);
    auto errors = route.errors;
    errors.push_back({"invalid_argument", 400, [] {
                        throw holder::contract::InvalidArgument("operation rejected");
                      }});
    errors.push_back({"internal_error", 500, [] {
                        throw std::runtime_error("operation rejected");
                      }});
    for (const auto& error : errors) {
      CAPTURE(error.code);
      bool called = false;
      holder_context context{[&](std::string_view operation) {
        CHECK(operation == route.operation);
        called = true;
        error.raise();
      }};
      const auto response = contract_http::dispatch(route.method, route.path, route.body, context);
      INFO((response ? response->body.dump() : "unmatched route"));
      REQUIRE(called);
      REQUIRE(response);
      CHECK(response->operation == route.operation);
      CHECK(response->status == error.status);
      CHECK(
          response->body ==
          nlohmann::json{
              {"ok", false},
              {"error", {{"code", error.code}, {"message", "operation rejected"}}}
          }
      );
    }
    bool called = false;
    holder_context context{[&](std::string_view) {
      called = true;
    }};
    // Core answering is a success, whatever the route declares.
    const auto answered = contract_http::dispatch(route.method, route.path, route.body, context);
    REQUIRE(answered);
    CHECK(called);
    CHECK(answered->operation == route.operation);
    CHECK(answered->status >= 200);
    CHECK(answered->status < 300);
    CHECK(answered->body["ok"] == true);
    called = false;
    const auto invalid_path = std::string(route.path) +
                              (std::string_view(route.path).find('?') == std::string_view::npos
                                   ? "?"
                                   : "&") +
                              "unexpected=1";
    const auto invalid = contract_http::dispatch(route.method, invalid_path, route.body, context);
    REQUIRE(invalid);
    CHECK(invalid->status == 400);
    CHECK(invalid->operation == route.operation);
    CHECK_FALSE(called);
    // A body is checked against its declared fields too: one the contract does not declare is a
    // 400.
    if (std::string_view(route.body).starts_with('{')) {
      auto body = nlohmann::json::parse(route.body);
      body["unexpected"] = 1;
      const auto unknown = contract_http::dispatch(route.method, route.path, body.dump(), context);
      REQUIRE(unknown);
      CHECK(unknown->status == 400);
      CHECK(
          unknown->body["error"]["message"].get<std::string>().find("has no field unexpected") !=
          std::string::npos
      );
      CHECK_FALSE(called);
    }
  }
}

TEST_CASE(
    "Generated codecs carry the optional fields routes rarely send",
    "[generated][dispatch]"
) {
  // A location's replacement configuration is decoded entry by entry.
  const auto update = contract_json::decode<holder::contract::StorageLocationUpdate>(
      nlohmann::json{{"configuration", {{"bucket", "notes"}, {"region", "eu"}}}}
  );
  REQUIRE(update.configuration.has_value());
  CHECK(
      *update.configuration ==
      std::map<std::string, std::string>{{"bucket", "notes"}, {"region", "eu"}}
  );

  // A link to a card in another project names that project.
  holder::contract::NewLink link;
  link.target_type = holder::contract::NewLinkTargetType::Card;
  link.target_id = "card-2";
  link.target_project_id = "project-2";
  CHECK(contract_json::encode(link)["target_project_id"] == "project-2");

  // Decoding a response type rejects a field the contract does not declare, as requests do.
  CHECK_THROWS_WITH(
      contract_json::decode<holder::contract::CardMetadata>(nlohmann::json{{"unexpected", 1}}),
      "CardMetadata has no field unexpected"
  );

  // A new location's configuration is decoded entry by entry too.
  const auto location = contract_json::decode<holder::contract::NewStorageLocation>(nlohmann::json{
      {"project_id", "project-1"},
      {"name", "Notes"},
      {"provider", "local_directory"},
      {"configuration", {{"path", "/srv/notes"}}}
  });
  CHECK(location.configuration == std::map<std::string, std::string>{{"path", "/srv/notes"}});

  // An update leaves out a name it does not change; it cannot clear one.
  CHECK_THROWS_WITH(
      contract_json::decode<holder::contract::StorageLocationUpdate>(
          nlohmann::json{{"name", nullptr}}
      ),
      "name must not be null"
  );

  // Every declared order reaches core.
  for (const auto* order : {"card_id_asc", "updated_desc"}) {
    CAPTURE(order);
    bool called = false;
    holder_context context{[&](std::string_view operation) {
      CHECK(operation == "list_cards");
      called = true;
    }};
    const auto response = contract_http::dispatch(
        "GET",
        std::string("/projects/a/cards?order=") + order,
        "",
        context
    );
    REQUIRE(response);
    CHECK(response->status == 200);
    CHECK(called);
  }
}
