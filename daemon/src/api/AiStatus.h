#pragma once

#include "api/ProviderCredentials.h"
#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <optional>
#include <string>
#include <vector>

namespace holder::llm { class RunnerRegistry; }
namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD CasteInfo {
  std::string name RIG_STRING_VALUES("mini", "user", "developer", "workstation", "rig");
  std::optional<std::string> reason RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CasteRecommendedModel {
  std::string tag;
  std::optional<std::string> provider RIG_NULLABLE;
  std::optional<std::string> engine RIG_NULLABLE;
  std::optional<std::string> category RIG_NULLABLE;
  std::string required_caste RIG_STRING_VALUES("mini", "user", "developer", "workstation", "rig");
  bool installed;
};

struct RIG_OPEN_RECORD RunnerCapabilitiesData {
  AiLocalModelConfigData local_model_config;
  std::optional<std::optional<CasteInfo>> caste RIG_OMIT_EMPTY RIG_NULLABLE RIG_FIELD_DESCRIPTION("This machine's hardware caste, or null when it cannot be detected.");
  std::vector<AiRunner> runners;
  std::vector<CasteRecommendedModel> recommended_models RIG_FIELD_DESCRIPTION("Catalog models this machine's caste can run, best first.");
  std::vector<CasteRecommendedModel> recommended_install RIG_FIELD_DESCRIPTION("The recommended models that the selected runner has not installed.");
};

struct RIG_OPEN_RECORD RunnerCapabilitiesResponse {
  bool ok RIG_BOOL_VALUE(true);
  RunnerCapabilitiesData data;
};

struct RIG_OPEN_RECORD AiStatusData {
  long long checked_at;
  long long active_runs RIG_FIELD_DESCRIPTION("Runs still in progress across every project.");
  long long active_pull_jobs RIG_FIELD_DESCRIPTION("Active pulls across every runner.");
  std::vector<AiRunner> runners;
  std::vector<AiProviderCredential> cloud;
  long long cloud_configured_providers;
};

struct RIG_OPEN_RECORD AiStatusResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiStatusData data;
};

RIG_ROUTE(GET, "/ai/capabilities")
RIG_SUMMARY("Get local model runner capabilities")
RIG_RESPONSE_DESCRIPTION("OK")
RunnerCapabilitiesResponse get_ai_capabilities(
    holder::platform::Db& db RIG_CONTEXT,
    holder::llm::RunnerRegistry* registry RIG_CONTEXT,
    std::optional<std::string> runner_id RIG_QUERY RIG_PARAMETER_DESCRIPTION("The runner whose installed models decide recommended_install; defaults to auto-local."));

RIG_ROUTE(GET, "/ai/status")
RIG_SUMMARY("Get live AI runtime status")
RIG_RESPONSE_DESCRIPTION("OK")
AiStatusResponse get_ai_status(holder::platform::Db& db RIG_CONTEXT,
                               holder::llm::RunnerRegistry* registry RIG_CONTEXT);

} // namespace holder::api
