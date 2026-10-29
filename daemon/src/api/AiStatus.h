#pragma once

#include "api/ProviderCredentials.h"
#include "api/AiRuntimeTypes.h"
#include "api/Schemas.h"

#include <optional>
#include <string>
#include <vector>

namespace holder::llm { class RunnerRegistry; }
namespace holder::platform { class Db; }

namespace holder::api {

struct CasteInfo {
  std::string name;
  std::optional<std::string> reason;
};

struct CasteRecommendedModel {
  std::string tag;
  std::optional<std::string> provider;
  std::optional<std::string> engine;
  std::optional<std::string> category;
  std::string required_caste;
  bool installed;
};

struct RunnerCapabilitiesData {
  AiLocalModelConfigData local_model_config;
  std::optional<std::optional<CasteInfo>> caste;
  std::vector<AiRunner> runners;
  std::vector<CasteRecommendedModel> recommended_models;
  std::vector<CasteRecommendedModel> recommended_install;
};

struct RunnerCapabilitiesResponse {
  bool ok;
  RunnerCapabilitiesData data;
};

struct AiStatusData {
  long long checked_at;
  long long active_runs;
  long long active_pull_jobs;
  std::vector<AiRunner> runners;
  std::vector<AiProviderCredential> cloud;
  long long cloud_configured_providers;
};

struct AiStatusResponse {
  bool ok;
  AiStatusData data;
};

RunnerCapabilitiesResponse get_ai_capabilities(
    holder::platform::Db& db,
    holder::llm::RunnerRegistry* registry,
    std::optional<std::string> runner_id);

AiStatusResponse get_ai_status(holder::platform::Db& db,
                               holder::llm::RunnerRegistry* registry);

} // namespace holder::api
