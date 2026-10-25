#include "api/AiStatus.h"

#include "ai/AiRunRepo.h"
#include "api/AiLocalModelConfig.h"
#include "api/AiRunnerRead.h"
#include "api/support/LocalModelRouting.h"
#include "api/support/Time.h"
#include "llm/RunnerClient.h"
#include "llm/RunnerRegistry.h"

#include <nlohmann/json.hpp>

#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace holder::api {
namespace {

std::optional<std::string> nullable_string(const nlohmann::json& value) {
  return value.is_string() ? std::optional<std::string>(value.get<std::string>()) : std::nullopt;
}

CasteRecommendedModel recommended_model(const nlohmann::json& item) {
  return {
      item.at("tag").get<std::string>(),
      nullable_string(item.at("provider")),
      nullable_string(item.at("engine")),
      nullable_string(item.at("category")),
      item.at("required_caste").get<std::string>(),
      item.at("installed").get<bool>(),
  };
}

} // namespace

RunnerCapabilitiesResponse get_ai_capabilities(holder::platform::Db& db,
                                               holder::llm::RunnerRegistry* registry,
                                               std::optional<std::string> runner_id) {
  RunnerCapabilitiesResponse response{true, {}};
  auto& data = response.data;
  try {
    data.local_model_config = get_local_model_config(db).data;
  } catch (const std::exception&) {
    // The capabilities view stays available without the saved model preferences.
  }
  const auto caste = support::detect_machine_caste();
  data.caste.emplace();  // Always sent, as null when the caste cannot be detected.
  if (caste) {
    data.caste->emplace(CasteInfo{caste->name, caste->reason.empty()
                                                   ? std::nullopt
                                                   : std::optional<std::string>(caste->reason)});
  }
  data.runners = runners_with_runtime(registry);
  if (!caste) return response;

  const auto selected = runner_id && !runner_id->empty()
                            ? *runner_id
                            : std::string(holder::llm::RunnerRegistry::kAutoLocalRunnerId);
  auto* client = registry ? registry->get_client(selected) : nullptr;
  const auto installed = client ? client->status().models : std::vector<holder::llm::LocalModel>{};
  for (const auto& item : support::build_caste_recommendations(
           installed, support::load_local_model_meta(), caste->name)) {
    auto model = recommended_model(item);
    if (!model.installed) data.recommended_install.push_back(model);
    data.recommended_models.push_back(std::move(model));
  }
  return response;
}

AiStatusResponse get_ai_status(holder::platform::Db& db, holder::llm::RunnerRegistry* registry) {
  AiStatusResponse response{true, {}};
  auto& data = response.data;
  data.checked_at = support::now_epoch_seconds();
  try {
    data.active_runs = holder::ai::AiRunRepo(db).count_started();
  } catch (const std::exception&) {
    data.active_runs = 0;
  }
  data.runners = runners_with_runtime(registry);
  for (const auto& runner : data.runners) {
    for (const auto& pull : runner.runtime.pulls) {
      if (pull.active) ++data.active_pull_jobs;
    }
  }
  data.cloud = list_provider_credentials(db).data.providers;
  data.cloud_configured_providers = static_cast<long long>(data.cloud.size());
  return response;
}

} // namespace holder::api
