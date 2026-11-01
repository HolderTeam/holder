#pragma once

#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"
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

struct AiLocalModelConfigData {
  std::optional<std::string> fast_model;
  std::optional<std::string> strong_model;
  std::optional<std::string> deep_model;
  std::optional<long long> updated_at;
};

struct LocalModel {
  std::string name;
  std::optional<std::string> runner_id;
  std::optional<std::string> model_ref;
  std::optional<std::string> digest;
  std::optional<long long> size;
  std::optional<std::string> modified_at;
};

struct AiRunnerRuntime {
  bool configured;
  bool available;
  bool spawn_attempted;
  long long last_checked;
  std::optional<std::string> version;
  std::optional<std::string> error;
  std::vector<LocalModel> models;
  std::vector<holder::api::RunnerPullJob> pulls;
};

struct AiRunner {
  std::string runner_id;
  std::string name;
  std::string kind;
  std::optional<std::string> base_url;
  std::string source;
  bool enabled;
  long long created_at;
  long long updated_at;
  AiRunnerRuntime runtime;
};

struct AiRunnerListResponseData {
  std::vector<AiRunner> runners;
};

struct AiRunnerListResponse {
  bool ok;
  AiRunnerListResponseData data;
};

struct AiRunnerResponse {
  bool ok;
  AiRunner data;
};

struct AiLocalModelConfigResponse {
  bool ok;
  AiLocalModelConfigData data;
};

struct AiRunEventData {
  // support::append_run_event fills run_id, so producers name only their own fields.
  std::string run_id = {};
  std::optional<std::string> status {};
  std::optional<long long> created_at {};
  std::optional<std::string> delta {};
  std::optional<std::string> message {};
  std::optional<std::string> error {};
  std::optional<std::string> runner_id {};
  std::optional<std::string> model_ref {};
  std::optional<std::string> model {};
  std::optional<std::string> provider {};
  std::optional<bool> context_compacted  {};
  std::optional<long long> cooldown_until  {};
  std::optional<std::string> router_model
      {};
  std::optional<std::vector<std::string>> ranked
      {};
  std::optional<bool> partial  {};
};

struct AiRunCreateRequest {
  std::string prompt;
  std::optional<std::string> mode;
  std::optional<std::optional<std::string>> model;
  std::optional<std::optional<std::string>> provider;
  std::optional<std::optional<std::string>> project_id;
  std::optional<std::optional<std::string>> thread_id;
  std::optional<std::optional<nlohmann::json>> context;
  std::optional<std::optional<std::string>> runner_id;
};

struct AiThread {
  std::string thread_id;
  std::string project_id;
  std::optional<std::optional<std::string>> card_id;
  std::string title;
  long long created_at;
  long long updated_at;
};

struct EventCheckpoint {
  std::string cursor;
  std::map<std::string, std::optional<std::string>> git_revisions;
  std::map<std::string, std::string> history_urls;
};

struct EventCheckpointResponse {
  bool ok;
  EventCheckpoint data;
};

struct ChangeEventData {
  std::string entity;
  std::string entity_id;
  std::optional<std::string> project_id;
  bool deleted;
  std::optional<std::string> git_revision;
};

struct EventResyncData : EventCheckpoint {
  std::string reason;
  std::optional<std::string> last_revision;
};

struct AssetImportJob {
  std::string job_id;
  std::string status;
  std::optional<std::string> resource_id;
  std::optional<std::string> asset_id;
  bool duplicate_reused;
  bool link_created;
  std::optional<std::string> error;
};

struct AssetImportJobResponse {
  bool ok;
  AssetImportJob data;
};

} // namespace holder::api
