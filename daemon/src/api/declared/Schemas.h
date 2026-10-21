#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <nlohmann/json.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace holder::api {

// Schemas of the declared operations, shared across their path groups.

struct RIG_OPEN_RECORD AiStatusPullJob : holder::api::RunnerPullJob {
  bool active;
};

struct RIG_OPEN_RECORD AiStatusData {
  long long checked_at;
  bool runner_available;
  long long runner_last_checked;
  std::optional<std::optional<std::string>> runner_version RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> runner_error RIG_OMIT_EMPTY RIG_NULLABLE;
  long long active_runs;
  long long active_pull_jobs;
  std::vector<AiStatusPullJob> pulls;
  std::vector<holder::api::AiProviderCredential> cloud;
  long long cloud_configured_providers;
};

struct RIG_OPEN_RECORD AiStatusResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiStatusData data;
};

struct RIG_OPEN_RECORD AiProviderCatalogProvider {
  std::string id;
  std::string display_name;
  bool enabled;
  bool configured;
  std::optional<nlohmann::json> api RIG_NULLABLE;
  std::optional<nlohmann::json> auth RIG_NULLABLE;
  std::vector<nlohmann::json> models;
  std::optional<std::optional<std::string>> setup_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> docs_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> api_key_label RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> api_key_hint RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCatalogResponseData {
  std::optional<std::optional<std::string>> default_provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::vector<AiProviderCatalogProvider> providers;
};

struct RIG_OPEN_RECORD AiProviderCatalogResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderCatalogResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCredentialDeleteResponseData {
  std::string provider;
};

struct RIG_OPEN_RECORD AiProviderCredentialDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderCredentialDeleteResponseData data;
};

struct RIG_OPEN_RECORD LocalModel {
  std::string name;
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

struct RIG_OPEN_RECORD AiRunnerCreateRequest {
  std::string name;
  std::string kind RIG_STRING_VALUES("ollama");
  std::string base_url;
  std::optional<bool> enabled RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiRunnerUpdateRequest {
  std::optional<std::string> name RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> base_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<bool> enabled RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiRunnerDeleteResponseData {
  std::string runner_id;
};

struct RIG_OPEN_RECORD AiRunnerDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunnerDeleteResponseData data;
};

struct RIG_OPEN_RECORD AiLocalModelConfigData {
  std::optional<std::string> fast_model RIG_NULLABLE;
  std::optional<std::string> strong_model RIG_NULLABLE;
  std::optional<std::string> deep_model RIG_NULLABLE;
  std::optional<long long> updated_at RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiLocalModelConfigResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiLocalModelConfigData data;
};

struct RIG_OPEN_RECORD AiLocalModelConfigUpdateRequest {
  std::optional<std::optional<std::string>> fast_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> strong_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> deep_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<long long>> updated_at RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiRunItem {
  std::string run_id;
  std::optional<std::optional<std::string>> project_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> thread_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> message_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string mode;
  std::string prompt;
  std::optional<std::optional<std::string>> context_json RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> router_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> ranked_json RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> policy_trace_json RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<nlohmann::json>> policy_trace RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> chosen_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string status;
  std::optional<std::optional<std::string>> error RIG_OMIT_EMPTY RIG_NULLABLE;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD AiRunListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiRunItem> data;
};

struct RIG_OPEN_RECORD AiRunGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunItem data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD RunnerPullStartResponseData {
  std::string job_id;
  std::optional<std::string> runner_id RIG_OMIT_EMPTY;
  std::optional<std::string> model_ref RIG_OMIT_EMPTY;
  std::string model;
  std::string status;
};

struct RIG_OPEN_RECORD RunnerPullStartResponse {
  bool ok RIG_BOOL_VALUE(true);
  RunnerPullStartResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD PostAiRunnerPullRequest {
  std::string model;
  std::optional<std::string> runner_id RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiMessage {
  std::string message_id;
  std::string thread_id;
  std::string role;
  std::string source;
  std::optional<std::optional<std::string>> provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string content;
  long long created_at;
  std::optional<std::optional<long long>> deleted_at RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> prompt_hash RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> meta_json RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiMessageListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiMessage> data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiMessageCreateResponseData {
  std::string message_id;
};

struct RIG_OPEN_RECORD AiMessageCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessageCreateResponseData data;
};

struct RIG_OPEN_RECORD AiMessageCreateRequest {
  std::optional<std::string> message_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string thread_id;
  std::string role;
  std::string source;
  std::optional<std::optional<std::string>> provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string content;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> prompt_hash RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> meta_json RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiMessageCaptureResponseData {
  std::string thread_id;
  std::string user_message_id;
  std::string assistant_message_id;
};

struct RIG_OPEN_RECORD AiMessageCaptureResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessageCaptureResponseData data;
};

struct RIG_OPEN_RECORD AiMessageCaptureRequest {
  std::string project_id;
  std::optional<std::optional<std::string>> thread_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string prompt;
  std::string response;
  std::optional<std::optional<std::string>> source RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<nlohmann::json>> context RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<long long>> created_at RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiThread {
  std::string thread_id;
  std::string project_id;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string title;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD AiThreadListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiThread> data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiThreadCreateResponseData {
  std::string thread_id;
};

struct RIG_OPEN_RECORD AiThreadCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiThreadCreateResponseData data;
};

struct RIG_OPEN_RECORD AiThreadCreateRequest {
  std::optional<std::string> thread_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string project_id;
  std::string title;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses created_at if omitted or 0.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiThreadGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiThread data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiThreadPatchResponseData {
  std::string thread_id;
};

struct RIG_OPEN_RECORD AiThreadPatchResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiThreadPatchResponseData data;
};

struct RIG_OPEN_RECORD AiThreadUpdateRequest {
  std::optional<std::optional<std::string>> title RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  long long updated_at;
};

struct RIG_OPEN_RECORD AiMessageGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessage data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiMessageUpdateResponseData {
  std::string message_id;
};

struct RIG_OPEN_RECORD AiMessageUpdateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessageUpdateResponseData data;
};

struct RIG_OPEN_RECORD AiMessageUpdateRequest {
  std::optional<std::optional<std::string>> role RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> source RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> content RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> created_at RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> prompt_hash RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> meta_json RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardLinkDeleteRequest {
  std::optional<std::string> to_card_id RIG_OMIT_EMPTY;
  std::optional<std::string> target_project_id RIG_FIELD_DESCRIPTION("Selects the target project. Requires to_card_id, to_type and kind; use it to distinguish equal card IDs in different projects.") RIG_OMIT_EMPTY;
  std::optional<std::string> to_type RIG_OMIT_EMPTY;
  std::optional<std::string> kind RIG_OMIT_EMPTY;
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

struct RIG_OPEN_RECORD ProjectListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<Project> data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectCreateResponseData {
  std::string project_id;
};

struct RIG_OPEN_RECORD ProjectCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectCreateResponseData data;
};

struct RIG_OPEN_RECORD ProjectCreateRequest {
  std::optional<std::string> project_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string name;
  std::optional<std::string> root_path RIG_FIELD_DESCRIPTION("Optional; server chooses a default if omitted.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> git_remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> git_provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::string> privacy_mode RIG_FIELD_DESCRIPTION("Optional; defaults to encrypted_git.") RIG_STRING_VALUES("encrypted_git", "plain") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> project_key_id RIG_FIELD_DESCRIPTION("Optional keyring identifier (metadata only).") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses created_at if omitted or 0.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD ProjectGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  Project data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectDeleteResponseData {
  std::string project_id;
};

struct RIG_OPEN_RECORD ProjectDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectDeleteResponseData data;
};

struct RIG_OPEN_RECORD HierarchyRepairMove {
  std::string card_id RIG_FIELD_DESCRIPTION("A hidden branch's top card. Its descendants stay attached and move with it.");
  std::optional<std::string> parent_card_id RIG_FIELD_DESCRIPTION("Where it goes, or null for the project's top level.") RIG_NULLABLE;
};

struct RIG_OPEN_RECORD HierarchyRepair {
  bool applied RIG_FIELD_DESCRIPTION("True when changes were written.");
  std::vector<HierarchyRepairMove> moved;
  int changed_card_count RIG_FIELD_DESCRIPTION("Cards whose placement changes, counting siblings re-spaced to make room.") RIG_MINIMUM(0);
  std::vector<std::string> cycle_card_ids RIG_FIELD_DESCRIPTION("Live cards whose parent chain loops. They are left alone and need moving by hand.");
};

struct RIG_OPEN_RECORD HierarchyRepairResponse {
  bool ok RIG_BOOL_VALUE(true);
  HierarchyRepair data;
};

struct RIG_OPEN_RECORD HierarchyRepairRequest {
  bool apply RIG_FIELD_DESCRIPTION("Apply the repair. Without it, only report.") RIG_DEFAULT(false);
};

struct RIG_OPEN_RECORD CardBreadcrumb {
  std::string kind RIG_STRING_VALUES("project", "card");
  std::string id;
  std::string title;
};

struct RIG_OPEN_RECORD CardBreadcrumbsResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<CardBreadcrumb> data RIG_FIELD_DESCRIPTION("Project first, then live ancestor cards from root to the requested card.");
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
  std::optional<std::vector<ProjectHistoryAffectedObjectItemsItem>> items RIG_FIELD_DESCRIPTION("Historical display details for the affected paths, when available.") RIG_OMIT_EMPTY;
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

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardHistoryRestoreResponseData {
  std::string card_id;
  std::string restored_from_oid RIG_FIELD_DESCRIPTION("Canonical full commit object ID of the selected historical snapshot.");
  std::string result_oid RIG_FIELD_DESCRIPTION("Canonical full commit object ID created by the restoration.");
  std::string title;
  long long updated_at;
  std::optional<long long> deleted_at RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardHistoryRestoreResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardHistoryRestoreResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitTestRemoteResponseData {
  std::string project_id;
  std::optional<std::optional<std::string>> remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string branch;
  std::string status RIG_STRING_VALUES("reachable", "auth_failed", "not_found", "network_error", "invalid_remote_url", "remote_unset", "unknown_error");
  bool remote_has_head;
  std::optional<std::optional<std::string>> error_code RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> error_message RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD ProjectGitTestRemoteResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectGitTestRemoteResponseData data;
};

struct RIG_OPEN_RECORD ProjectGitTestRemoteRequest {
  std::optional<std::optional<std::string>> remote_url RIG_FIELD_DESCRIPTION("Optional remote URL override for this read-only probe. Stored project configuration and Git configuration are unchanged. If omitted, uses the configured URL; null probes no remote.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> branch RIG_FIELD_DESCRIPTION("Optional branch hint, echoed in the result. If omitted, reports local_default. The probe lists remote refs without resolving or fetching a branch.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitPushResponseData {
  std::string project_id;
  std::optional<std::optional<std::string>> remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string branch;
  std::string status RIG_STRING_VALUES("pushed", "up_to_date", "auth_failed", "not_found", "network_error", "non_fast_forward", "remote_unset", "unknown_error");
  int ahead_count;
  int behind_count;
  std::optional<std::optional<std::string>> local_head_commit RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> error_code RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> error_message RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> next_action RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD ProjectGitPushResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectGitPushResponseData data;
};

struct RIG_OPEN_RECORD ProjectGitPushRequest {
  std::optional<std::optional<std::string>> branch RIG_FIELD_DESCRIPTION("Optional branch to push. If omitted, server resolves local default branch from HEAD/init.defaultBranch.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<bool>> set_upstream RIG_FIELD_DESCRIPTION("Optional; defaults to true.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitSyncStatusResponseData {
  std::string project_id;
  ProjectSync sync;
};

struct RIG_OPEN_RECORD ProjectGitSyncStatusResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectGitSyncStatusResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectRecoveryTokenExportResponseData {
  std::string project_id;
  std::string key_id;
  std::string recovery_token RIG_FIELD_DESCRIPTION("Serialized `.hrk` recovery token JSON payload.");
};

struct RIG_OPEN_RECORD ProjectRecoveryTokenExportResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectRecoveryTokenExportResponseData data;
};

struct RIG_OPEN_RECORD ProjectRecoveryTokenExportRequest {
  std::string pin RIG_FIELD_DESCRIPTION("User-provided PIN used to wrap the project key.");
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectRecoveryTokenImportResponseData {
  std::string project_id;
};

struct RIG_OPEN_RECORD ProjectRecoveryTokenImportResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectRecoveryTokenImportResponseData data;
};

struct RIG_OPEN_RECORD ProjectRecoveryTokenImportRequest {
  std::string pin;
  std::string recovery_token RIG_FIELD_DESCRIPTION("Serialized `.hrk` recovery token JSON payload.");
};

struct RIG_OPEN_RECORD ProjectEncryptionCheck {
  bool ok;
  int checked_files;
  int unsafe_files;
  std::vector<std::string> unsafe_paths;
  std::string message;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectEncryptionCheckResponseData {
  std::string project_id;
  std::string privacy_mode RIG_STRING_VALUES("encrypted_git", "plain");
  ProjectEncryptionCheck check;
};

struct RIG_OPEN_RECORD ProjectEncryptionCheckResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectEncryptionCheckResponseData data;
};

struct RIG_DESCRIPTION("Exactly one of date or datetime must be present. Roles are free-form and may repeat within a milestone. Array order is preserved.") MilestoneDateValue {
  std::string role RIG_MIN_LENGTH(1);
  std::optional<std::string> date RIG_FIELD_DESCRIPTION("Calendar day with no timezone; YYYY-MM-DD.") RIG_FORMAT("date") RIG_OMIT_EMPTY;
  std::optional<std::string> datetime RIG_FIELD_DESCRIPTION("RFC 3339 timestamp with seconds and an explicit Z or numeric offset.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD MilestoneWithDates {
  std::string milestone_id RIG_FORMAT("uuid");
  std::string card_id;
  std::vector<MilestoneDateValue> dates;
  std::optional<std::string> kind RIG_NULLABLE;
  std::optional<std::string> description RIG_NULLABLE;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD MilestoneWithDatesListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<MilestoneWithDates> data;
};

struct RIG_OPEN_RECORD TrashDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  nlohmann::json data;
};

struct RIG_OPEN_RECORD MilestoneWithDatesResponse {
  bool ok RIG_BOOL_VALUE(true);
  MilestoneWithDates data;
};

struct MilestoneWithDatesCreateRequest {
  std::vector<MilestoneDateValue> dates RIG_FIELD_DESCRIPTION("May be empty for an undated milestone.");
  std::optional<std::optional<std::string>> kind RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> description RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD MilestoneRemoveResult {
  std::string card_id;
  std::string milestone_id RIG_FIELD_DESCRIPTION("Canonical milestone identifier supplied in the request.") RIG_FORMAT("uuid");
  bool removed RIG_FIELD_DESCRIPTION("True when the milestone existed and was removed; false when it was already absent.");
};

struct RIG_OPEN_RECORD MilestoneRemoveResponse {
  bool ok RIG_BOOL_VALUE(true);
  MilestoneRemoveResult data;
};

struct RIG_OPEN_RECORD Milestone {
  std::string milestone_id RIG_FIELD_DESCRIPTION("Canonical milestone identifier; this is not a card ID and is never abbreviated.") RIG_FORMAT("uuid");
  std::string card_id;
  long long start_at RIG_FIELD_DESCRIPTION("Unix epoch seconds. For an all-day milestone, this encodes the originating local date at local midnight.");
  std::optional<std::optional<long long>> end_at RIG_FIELD_DESCRIPTION("Inclusive milestone end in Unix epoch seconds. For an all-day milestone, this encodes the inclusive ending local date at local midnight.") RIG_OMIT_EMPTY RIG_NULLABLE;
  bool all_day RIG_FIELD_DESCRIPTION("When true, start_at and end_at carry local calendar-date semantics and clients should render dates rather than ordinary timed instants.");
  std::optional<std::optional<std::string>> kind RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> description RIG_OMIT_EMPTY RIG_NULLABLE;
  long long created_at;
  long long updated_at;
  std::optional<std::string> card_title RIG_FIELD_DESCRIPTION("Included in project calendar responses.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD MilestoneListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<Milestone> data;
};

struct RIG_OPEN_RECORD MilestoneResponse {
  bool ok RIG_BOOL_VALUE(true);
  Milestone data;
};

struct RIG_OPEN_RECORD MilestoneCreateRequest {
  long long start_at RIG_FIELD_DESCRIPTION("Unix epoch seconds. For all_day=true, encode the local calendar date at local midnight.");
  std::optional<std::optional<long long>> end_at RIG_FIELD_DESCRIPTION("Inclusive milestone end. For all_day=true, encode the inclusive ending local date at local midnight.") RIG_OMIT_EMPTY RIG_NULLABLE;
  bool all_day RIG_FIELD_DESCRIPTION("Marks start_at and end_at as local calendar dates rather than timed instants.") RIG_DEFAULT(false);
  std::optional<std::optional<std::string>> kind RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> description RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardListItem {
  std::string card_id;
  std::string project_id;
  std::string title;
  std::string rel_path;
  double sort_key;
  long long created_at;
  long long updated_at;
  std::optional<std::optional<std::string>> parent_card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<long long>> deleted_at RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<int> child_count RIG_FIELD_DESCRIPTION("Present when requested by the listing route (include_child_count=true on project card pages).") RIG_MINIMUM(0) RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD CardListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<CardListItem> data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardCreateResponseData {
  std::string card_id;
  std::string rel_path;
};

struct RIG_OPEN_RECORD CardCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardCreateResponseData data;
};

struct RIG_OPEN_RECORD CardCreateRequest {
  std::string project_id;
  std::string title;
  std::string content;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses created_at if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> parent_card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<double> sort_key RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> rel_path RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardContextDataProject {
  std::string project_id;
  std::string name;
};

struct RIG_OPEN_RECORD CardContextBreadcrumb {
  std::string type RIG_STRING_VALUES("project", "card");
  std::string title;
  std::optional<std::optional<std::string>> project_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardContextCard : CardListItem {
};

struct RIG_OPEN_RECORD CardContextData {
  CardContextDataProject project;
  std::optional<std::string> current_parent_card_id RIG_NULLABLE;
  std::vector<CardContextBreadcrumb> breadcrumbs;
  std::vector<CardContextCard> cards;
};

struct RIG_OPEN_RECORD CardContextResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardContextData data;
};

struct RIG_OPEN_RECORD CardData {
  std::string card_id;
  std::string project_id;
  std::string title;
  std::string rel_path;
  double sort_key;
  long long created_at;
  long long updated_at;
  std::optional<std::optional<std::string>> parent_card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<long long>> deleted_at RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string content;
};

struct RIG_OPEN_RECORD CardGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardPatchResponseData {
  std::string card_id;
};

struct RIG_OPEN_RECORD CardPatchResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardPatchResponseData data;
};

struct RIG_OPEN_RECORD CardUpdateRequest {
  std::optional<std::string> content RIG_OMIT_EMPTY;
  long long updated_at;
  std::optional<std::optional<std::string>> title RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> parent_card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<double> sort_key RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD CardMoveResult {
  std::string card_id;
  std::optional<std::string> parent_card_id RIG_NULLABLE;
  double sort_key;
  long long revision;
  std::optional<std::optional<std::string>> moved_into_title RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardMoveResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardMoveResult data;
};

enum class CardMoveIntent {
  into,
  before,
  after,
  left,
  right,
  to_start,
  to_end,
  up_level,
};

struct RIG_OPEN_RECORD CardMoveRequest {
  std::string project_id;
  CardMoveIntent intent;
  std::optional<std::optional<std::string>> target_card_id RIG_FIELD_DESCRIPTION("Required for intents into/before/after.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> parent_card_id RIG_FIELD_DESCRIPTION("Destination parent scope for intents to_start/to_end/left/right. If omitted, source card's current parent scope is used. An explicit null is the project's top level.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<long long>> if_revision RIG_FIELD_DESCRIPTION("Optional optimistic-concurrency revision guard.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardTagMutationResult {
  std::string card_id;
  std::string tag RIG_FIELD_DESCRIPTION("Normalized tag without the leading");
  std::string outcome RIG_STRING_VALUES("added", "already_present", "removed", "not_present", "present_outside_editable_tag_line");
  bool changed RIG_FIELD_DESCRIPTION("True only when card content was changed.");
};

struct RIG_OPEN_RECORD CardTagMutationResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardTagMutationResult data;
};

struct RIG_OPEN_RECORD CardTagMutationRequest {
  std::string project_id RIG_FIELD_DESCRIPTION("Project that must own the live card.") RIG_MIN_LENGTH(1);
  std::string tag RIG_FIELD_DESCRIPTION("Tag without the leading") RIG_MIN_LENGTH(1);
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ResourceAttachmentResponseData {
  std::string card_id;
  std::string resource_id;
  bool changed;
  std::string outcome RIG_STRING_VALUES("attached", "already_attached", "detached", "not_attached");
};

struct RIG_OPEN_RECORD ResourceAttachmentResponse {
  bool ok RIG_BOOL_VALUE(true);
  ResourceAttachmentResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD PostCardsCardIdResourcesRequest {
  std::string project_id RIG_MIN_LENGTH(1);
  std::string resource_id RIG_FIELD_DESCRIPTION("Complete exact project resource ID; no prefix or label lookup.") RIG_MIN_LENGTH(1);
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD DeleteCardsCardIdResourcesRequest {
  std::string project_id RIG_MIN_LENGTH(1);
  std::string resource_id RIG_FIELD_DESCRIPTION("Complete exact project resource ID; no prefix or label lookup.") RIG_MIN_LENGTH(1);
};

struct RIG_OPEN_RECORD CardBacklinkListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<holder::api::routes::CardLink> data;
  std::string scope RIG_STRING_VALUES("local", "external", "all_available");
  std::string coverage RIG_FIELD_DESCRIPTION("External backlinks cover only source projects available to this daemon request, never all possible projects.") RIG_STRING_VALUES("target_project", "available_projects");
};

struct RIG_OPEN_RECORD StorageLocation {
  std::string location_id;
  std::string project_id;
  std::string name;
  std::string provider RIG_STRING_VALUES("local_directory", "s3_compatible");
  std::map<std::string, std::string> configuration RIG_FIELD_DESCRIPTION("Portable non-secret provider configuration.");
  bool bound;
  std::optional<std::optional<std::string>> binding_preview RIG_FIELD_DESCRIPTION("Safe display text; credentials are never returned.") RIG_OMIT_EMPTY RIG_NULLABLE;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD StorageLocationListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<StorageLocation> data;
  std::optional<std::string> preferred_location_id RIG_NULLABLE;
};

struct RIG_OPEN_RECORD StorageLocationResponse {
  bool ok RIG_BOOL_VALUE(true);
  StorageLocation data;
};

struct RIG_OPEN_RECORD StorageLocationRequest {
  std::optional<std::string> location_id RIG_OMIT_EMPTY;
  std::string project_id;
  std::string name;
  std::string provider RIG_STRING_VALUES("local_directory", "s3_compatible");
  std::optional<std::map<std::string, std::string>> configuration RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD PatchLocationsLocationIdRequest {
  std::optional<std::string> name RIG_OMIT_EMPTY;
  std::optional<std::map<std::string, std::string>> configuration RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD LocationActionResponseData {
  std::string location_id;
  std::optional<bool> bound RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> binding_preview RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD LocationActionResponse {
  bool ok RIG_BOOL_VALUE(true);
  LocationActionResponseData data;
};

struct RIG_OPEN_RECORD LocationBindingRequest {
  std::map<std::string, std::string> values RIG_WRITE_ONLY;
  std::optional<std::string> preview RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD LocationTestResponseData {
  bool available;
};

struct RIG_OPEN_RECORD LocationTestResponse {
  bool ok RIG_BOOL_VALUE(true);
  LocationTestResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD PutLocationsPreferredRequest {
  std::string project_id;
  std::string location_id;
};

struct RIG_VALUE_TYPE RIG_DESCRIPTION("Permissive metadata map. Repeated and custom values are preserved in order.") ResourceMetadata {
  std::map<std::string, std::vector<std::string>> value;
};

struct RIG_OPEN_RECORD AssetPlacement {
  std::string placement_id;
  std::string location_id;
  std::string encoding RIG_STRING_VALUES("plain", "holder_asset_v1");
  long long stored_byte_size;
  long long created_at;
};

struct RIG_OPEN_RECORD ResourceAsset {
  std::string asset_id;
  std::string resource_id;
  std::string original_filename;
  std::string media_type;
  long long byte_size;
  std::string plaintext_sha256;
  std::vector<AssetPlacement> placements;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ResourceReferencedByCardsItem {
  std::string card_id;
  std::string title;
  long long updated_at;
  std::vector<std::string> link_kinds;
};

struct RIG_OPEN_RECORD Resource {
  std::string resource_id;
  std::string project_id;
  std::string type RIG_FIELD_DESCRIPTION("Friendly, extensible type rather than a closed enum.");
  std::string label;
  ResourceMetadata metadata;
  std::vector<ResourceAsset> assets;
  long long created_at;
  long long updated_at;
  std::optional<std::vector<ResourceReferencedByCardsItem>> referenced_by_cards RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD ResourceListResponse {
  std::optional<std::string> card_id RIG_FIELD_DESCRIPTION("Canonical live card ID, present only for card attachment lists.") RIG_OMIT_EMPTY;
  std::optional<int> limit RIG_OMIT_EMPTY;
  std::optional<int> offset RIG_OMIT_EMPTY;
  std::optional<std::optional<int>> next_offset RIG_FIELD_DESCRIPTION("Present for card lists; null when exhausted, otherwise continue here (the next page may be empty).") RIG_OMIT_EMPTY RIG_NULLABLE;
  bool ok RIG_BOOL_VALUE(true);
  std::vector<Resource> data;
};

struct RIG_OPEN_RECORD ResourceResponse {
  bool ok RIG_BOOL_VALUE(true);
  Resource data;
};

struct RIG_OPEN_RECORD ResourceCreateRequest {
  std::optional<std::string> resource_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string project_id;
  std::string type;
  std::string label;
  std::optional<ResourceMetadata> metadata RIG_OMIT_EMPTY;
  std::optional<long long> created_at RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD ResourceUpdateRequest {
  std::optional<std::string> type RIG_OMIT_EMPTY;
  std::optional<std::string> label RIG_OMIT_EMPTY;
  std::optional<ResourceMetadata> metadata RIG_FIELD_DESCRIPTION("Partial metadata update. Empty arrays remove a property; omitted properties are preserved.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ResourceDeleteResponseData {
  std::string resource_id;
};

struct RIG_OPEN_RECORD ResourceDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  ResourceDeleteResponseData data;
};

struct RIG_OPEN_RECORD TrashItem {
  std::string type;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> message_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> thread_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::string> project_id RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> title RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> role RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> deleted_at RIG_OMIT_EMPTY;
  std::optional<long long> created_at RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> rel_path RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD TrashListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<TrashItem> data;
};

struct RIG_OPEN_RECORD AssetImportJob {
  std::string job_id;
  std::string status RIG_STRING_VALUES("queued", "staging", "storing", "committing", "completed", "failed");
  std::optional<std::optional<std::string>> resource_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> asset_id RIG_OMIT_EMPTY RIG_NULLABLE;
  bool duplicate_reused;
  std::optional<bool> link_created RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> error RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AssetImportJobResponse {
  bool ok RIG_BOOL_VALUE(true);
  AssetImportJob data;
};

struct RIG_OPEN_RECORD AssetImportRequest {
  std::string project_id;
  std::string card_id;
  std::string location_id;
  std::string source_path RIG_FIELD_DESCRIPTION("Local readable regular file; never persisted into project metadata.");
};

struct RIG_OPEN_RECORD SearchCardItem {
  std::string card_id;
  std::string title;
  long long updated_at;
  long long created_at;
  std::string snippet;
  double rank;
};

struct RIG_OPEN_RECORD SearchCardsResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<SearchCardItem> data;
};

struct RIG_OPEN_RECORD SearchMessageItem {
  std::string message_id;
  long long created_at;
  std::string snippet;
  double rank;
};

struct RIG_OPEN_RECORD SearchMessagesResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<SearchMessageItem> data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD HealthDataCryptFilter {
  std::string path;
  bool exists;
  bool executable;
};

struct RIG_OPEN_RECORD HealthData {
  bool db_ok;
  long long uptime_ms;
  std::string api_version;
  std::string server_version;
  int pid;
  HealthDataCryptFilter crypt_filter;
};

struct RIG_OPEN_RECORD HealthResponse {
  bool ok RIG_BOOL_VALUE(true);
  HealthData data;
};

struct RIG_OPEN_RECORD ReindexData {
  std::string message;
};

struct RIG_OPEN_RECORD ReindexResponse {
  bool ok RIG_BOOL_VALUE(true);
  ReindexData data;
};

struct RIG_OPEN_RECORD CalendarCardActivity {
  std::string card_id;
  std::string title;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectCalendarResponseData {
  std::string project_id;
  long long from RIG_FIELD_DESCRIPTION("Inclusive lower bound in Unix epoch seconds.");
  long long to RIG_FIELD_DESCRIPTION("Inclusive upper bound in Unix epoch seconds.");
  std::vector<Milestone> milestones;
  std::vector<CalendarCardActivity> created_cards;
  std::vector<CalendarCardActivity> updated_cards;
};

struct RIG_OPEN_RECORD ProjectCalendarResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectCalendarResponseData data;
};

} // namespace holder::api
