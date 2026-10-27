#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/RunnerPull.h"
#include "api/ProviderCredentials.h"
#include "api/CardLinks.h"

#include <nlohmann/json.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace holder::api {

// Schemas of the declared operations, shared across their path groups.

struct RIG_OPEN_RECORD AiLocalModelConfigData {
  std::optional<std::string> fast_model RIG_NULLABLE;
  std::optional<std::string> strong_model RIG_NULLABLE;
  std::optional<std::string> deep_model RIG_NULLABLE;
  std::optional<long long> updated_at RIG_NULLABLE;
};

struct RIG_OPEN_RECORD LocalModel {
  std::string name;
  std::optional<std::string> runner_id RIG_OMIT_EMPTY RIG_FIELD_DESCRIPTION("The runner that has this model.");
  std::optional<std::string> model_ref RIG_OMIT_EMPTY RIG_FIELD_DESCRIPTION("The runner-qualified model reference to request it by.");
  std::optional<std::string> digest RIG_OMIT_EMPTY;
  std::optional<long long> size RIG_OMIT_EMPTY;
  std::optional<std::string> modified_at RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiRunnerRuntime {
  bool configured;
  bool available;
  bool spawn_attempted;
  long long last_checked;
  std::optional<std::string> version RIG_NULLABLE;
  std::optional<std::string> error RIG_NULLABLE;
  std::vector<LocalModel> models;
  std::vector<holder::api::RunnerPullJob> pulls;
};

struct RIG_OPEN_RECORD AiRunner {
  std::string runner_id;
  std::string name;
  std::string kind;
  std::optional<std::string> base_url RIG_NULLABLE;
  std::string source;
  bool enabled;
  long long created_at;
  long long updated_at;
  AiRunnerRuntime runtime;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiRunnerListResponseData {
  std::vector<AiRunner> runners;
};

struct RIG_OPEN_RECORD AiRunnerListResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunnerListResponseData data;
};

struct RIG_OPEN_RECORD AiRunnerResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunner data;
};

struct RIG_OPEN_RECORD AiLocalModelConfigResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiLocalModelConfigData data;
};

struct RIG_OPEN_RECORD RIG_DESCRIPTION("Named run events share run_id. Fields vary by event and provider.") AiRunEventData {
  // support::append_run_event fills run_id, so producers name only their own fields.
  std::string run_id = {};
  std::optional<std::string> status RIG_OMIT_EMPTY{};
  std::optional<long long> created_at RIG_OMIT_EMPTY{};
  std::optional<std::string> delta RIG_OMIT_EMPTY{};
  std::optional<std::string> message RIG_OMIT_EMPTY{};
  std::optional<std::string> error RIG_OMIT_EMPTY{};
  std::optional<std::string> runner_id RIG_OMIT_EMPTY{};
  std::optional<std::string> model_ref RIG_OMIT_EMPTY{};
  std::optional<std::string> model RIG_OMIT_EMPTY{};
  std::optional<std::string> provider RIG_OMIT_EMPTY{};
  std::optional<bool> context_compacted RIG_OMIT_EMPTY RIG_FIELD_DESCRIPTION(
      "On a cloud progress event: whether the thread was summarized to fit the model's context."
  ){};
  std::optional<long long> cooldown_until RIG_OMIT_EMPTY RIG_FIELD_DESCRIPTION(
      "On a cloud fallback: when the failed model may be tried again, in epoch seconds."
  ){};
  std::optional<std::string> router_model RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("On router_result: the model that ranked the candidates."){};
  std::optional<std::vector<std::string>> ranked RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("On router_result: the candidate models, best first."){};
  std::optional<bool> partial RIG_OMIT_EMPTY RIG_FIELD_DESCRIPTION(
      "On a local fallback: whether the failed model had already streamed some output."
  ){};
};

struct RIG_OPEN_RECORD AiRunCreateRequest {
  std::string prompt;
  std::optional<std::string> mode RIG_FIELD_DESCRIPTION("auto or model") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> model RIG_FIELD_DESCRIPTION("Optional explicit model id; interpreted for local or cloud selected provider.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> provider RIG_FIELD_DESCRIPTION("Optional cloud provider id when local runner is unavailable.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> project_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> thread_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<nlohmann::json>> context RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> runner_id RIG_FIELD_DESCRIPTION("The local runner to use; defaults to the automatic local runner.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiThread {
  std::string thread_id;
  std::string project_id;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string title;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD ProjectSync {
  std::optional<long long> last_commit_at RIG_NULLABLE;
  std::optional<long long> last_push_at RIG_NULLABLE;
  std::optional<long long> last_pull_at RIG_NULLABLE;
  int uncommitted_changes_count;
  int unpushed_commits_count;
  std::optional<std::string> last_push_status RIG_NULLABLE;
  std::optional<std::string> last_pull_status RIG_NULLABLE;
  std::optional<std::string> last_sync_error RIG_NULLABLE;
  std::optional<long long> last_sync_error_at RIG_NULLABLE;
  int retry_count;
  std::optional<long long> next_retry_at RIG_NULLABLE;
  int pull_retry_count;
  std::optional<long long> next_pull_retry_at RIG_NULLABLE;
  std::optional<long long> updated_at RIG_NULLABLE;
};

struct RIG_OPEN_RECORD Project {
  std::string project_id;
  std::string name;
  std::string root_path;
  std::optional<std::optional<std::string>> git_remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> git_provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string privacy_mode RIG_STRING_VALUES("encrypted_git", "plain");
  std::optional<std::optional<std::string>> project_key_id RIG_OMIT_EMPTY RIG_NULLABLE;
  long long created_at;
  long long updated_at;
  std::optional<int> card_count RIG_FIELD_DESCRIPTION("Present when count=true.") RIG_MINIMUM(0) RIG_OMIT_EMPTY;
  std::optional<int> root_card_count RIG_FIELD_DESCRIPTION("Present when count=true.") RIG_MINIMUM(0) RIG_OMIT_EMPTY;
  ProjectSync sync;
};

// A card in GET /cards and GET /cards/context. The contract's CardListItem replaces it when those
// routes move to the contract.
struct RIG_OPEN_RECORD CardViewItem {
  std::string card_id;
  std::string project_id;
  std::string title;
  std::string rel_path;
  double sort_key;
  long long created_at;
  long long updated_at;
  std::optional<std::string> parent_card_id RIG_NULLABLE;
  std::optional<long long> deleted_at RIG_NULLABLE;
  std::optional<int> child_count RIG_FIELD_DESCRIPTION("Live children, present when requested (count=true, or include_child_count=true on project card pages).") RIG_MINIMUM(0) RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD CardHistoryAuthor {
  std::string name;
  std::string email;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectHistoryAffectedObjectItemsItem {
  std::string path;
  std::optional<std::string> title RIG_OMIT_EMPTY;
  std::optional<std::string> detail RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD ProjectHistoryAffectedObject {
  std::string kind RIG_STRING_VALUES("card", "resource", "location", "ai_data", "project_settings", "unknown");
  std::vector<std::string> paths;
  std::vector<ProjectHistoryAffectedObjectItemsItem> items RIG_FIELD_DESCRIPTION("Historical display details for the affected paths, when available.");
};

struct RIG_OPEN_RECORD ProjectHistoryActivity {
  std::string oid;
  std::vector<std::string> parent_oids;
  CardHistoryAuthor author;
  long long authored_at;
  long long committed_at;
  std::string message;
  std::vector<ProjectHistoryAffectedObject> affected_objects;
  bool is_merge;
};

struct RIG_OPEN_RECORD ProjectHistoryPageData {
  std::optional<std::string> head_oid RIG_NULLABLE;
  std::vector<ProjectHistoryActivity> activities;
  std::optional<std::string> next_cursor RIG_NULLABLE;
  bool scan_limited;
};

struct RIG_OPEN_RECORD ProjectHistoryPageResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectHistoryPageData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardHistoryEntrySavesItem {
  std::string oid;
  std::vector<std::string> parent_oids;
  long long authored_at;
  long long committed_at;
  std::string message;
};

struct RIG_OPEN_RECORD CardHistoryEntry {
  std::string first_oid;
  std::string last_oid;
  std::vector<std::string> parent_oids;
  std::vector<std::string> visible_parent_oids RIG_FIELD_DESCRIPTION("Direct parent entries visible on this history page; omitted connections must not be inferred across filtered commits or page boundaries.");
  CardHistoryAuthor author;
  long long started_at;
  long long ended_at;
  std::string kind RIG_STRING_VALUES("created", "updated", "moved", "links", "milestones", "deleted", "restored", "permanently_deleted", "merged");
  std::string summary;
  int commit_count RIG_MINIMUM(1);
  bool is_merge;
  std::vector<CardHistoryEntrySavesItem> saves RIG_FIELD_DESCRIPTION("Exact saved commits in chronological order; one item for an ungrouped entry.");
};

struct RIG_OPEN_RECORD CardHistoryPageData {
  std::optional<std::string> head_oid RIG_NULLABLE;
  std::vector<CardHistoryEntry> entries;
  std::optional<std::string> next_cursor RIG_NULLABLE;
  bool scan_limited RIG_FIELD_DESCRIPTION("True when History examined its bounded number of revisions before reaching the end; next_cursor continues the scan.");
};

struct RIG_OPEN_RECORD CardHistoryPageResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardHistoryPageData data;
};

struct RIG_OPEN_RECORD CardHistoryVersion {
  bool exists;
  std::string oid;
  std::string title;
  std::string body;
};

struct RIG_OPEN_RECORD CardHistoryDiffLine {
  std::string origin RIG_STRING_VALUES(" ", "+", "-");
  std::string text;
  std::optional<long long> old_line RIG_NULLABLE;
  std::optional<long long> new_line RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardHistoryComparisonData {
  CardHistoryVersion from;
  CardHistoryVersion to;
  std::string summary;
  std::vector<CardHistoryDiffLine> lines;
  bool truncated;
};

struct RIG_OPEN_RECORD CardHistoryComparisonResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardHistoryComparisonData data;
};

struct RIG_VALUE_TYPE RIG_DESCRIPTION("Full Git commit object ID or a unique leading commit-ID prefix. Successful responses use the canonical full lowercase object ID.") RevisionReference {
  std::string value RIG_PATTERN("^[0-9A-Fa-f]{8,40}$") RIG_MIN_LENGTH(8) RIG_MAX_LENGTH(40);
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardHistorySnapshotResponseData {
  std::string card_id;
  CardHistoryVersion snapshot;
};

struct RIG_OPEN_RECORD CardHistorySnapshotResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardHistorySnapshotResponseData data;
};

struct RIG_OPEN_RECORD StorageLocation {
  std::string location_id;
  std::string project_id;
  std::string name;
  std::string provider RIG_STRING_VALUES("local_directory", "s3_compatible", "google-drive");
  std::map<std::string, std::string> configuration RIG_FIELD_DESCRIPTION("Portable non-secret provider configuration.");
  bool bound;
  std::optional<std::string> binding_preview RIG_FIELD_DESCRIPTION("Safe display text; credentials are never returned.") RIG_NULLABLE;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD StorageLocationResponse {
  bool ok RIG_BOOL_VALUE(true);
  StorageLocation data;
};

struct RIG_OPEN_RECORD EventCheckpoint {
  std::string cursor RIG_FIELD_DESCRIPTION("Opaque process-local SSE cursor. Persist it unchanged for Last-Event-ID.");
  std::map<std::string, std::optional<std::string>> git_revisions RIG_FIELD_DESCRIPTION("Observed project Git HEADs; null for missing or unborn repositories. Not a transaction marker for live database state.");
  std::map<std::string, std::string> history_urls RIG_FIELD_DESCRIPTION("Existing project history endpoints for durable recovery. Refresh live state after inspecting history.");
};

struct RIG_OPEN_RECORD EventCheckpointResponse {
  bool ok RIG_BOOL_VALUE(true);
  EventCheckpoint data;
};

struct RIG_OPEN_RECORD ChangeEventData {
  std::string entity RIG_STRING_VALUES("project", "card", "resource", "location", "thread", "message", "run");
  std::string entity_id;
  std::optional<std::string> project_id RIG_NULLABLE;
  bool deleted RIG_FIELD_DESCRIPTION("Entity disappeared from the observed database snapshot. Soft deletion remains an ordinary invalidation.");
  std::optional<std::string> git_revision RIG_FIELD_DESCRIPTION("Observed Git HEAD for this project; live changes may not yet be committed to Git.") RIG_NULLABLE;
};

struct RIG_OPEN_RECORD EventResyncData : EventCheckpoint {
  std::string reason RIG_STRING_VALUES("history_unavailable", "observer_unavailable");
  std::optional<std::string> last_revision RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AssetImportJob {
  std::string job_id;
  std::string status RIG_STRING_VALUES("queued", "staging", "storing", "committing", "completed", "failed");
  std::optional<std::string> resource_id RIG_NULLABLE;
  std::optional<std::string> asset_id RIG_NULLABLE;
  bool duplicate_reused;
  bool link_created;
  std::optional<std::string> error RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AssetImportJobResponse {
  bool ok RIG_BOOL_VALUE(true);
  AssetImportJob data;
};

} // namespace holder::api
