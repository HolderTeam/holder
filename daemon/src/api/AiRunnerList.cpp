#include "api/AiRunnerList.h"

#include "llm/RunnerClient.h"
#include "llm/RunnerModelRef.h"
#include "llm/RunnerRegistry.h"

#include <optional>
#include <string>

namespace holder::api {
namespace {

std::optional<std::string> unless_empty(const std::string& value) {
  return value.empty() ? std::nullopt : std::optional<std::string>(value);
}

AiRunnerRuntime runtime(const std::string& runner_id, holder::llm::RunnerClient* client) {
  if (client == nullptr) {
    return {false, false, false, 0, std::nullopt, "runner_not_configured", {}, {}};
  }
  const auto status = client->status();
  AiRunnerRuntime value{
      true,
      status.available,
      status.spawn_attempted,
      status.last_checked,
      unless_empty(status.version),
      unless_empty(status.error),
      {},
      {},
  };
  for (const auto& model : status.models) {
    value.models.push_back({
        model.name,
        runner_id,
        holder::llm::make_runner_model_ref(runner_id, model.name),
        model.digest,
        model.size,
        model.modified_at,
    });
  }
  // This listing has always sent a pull's error as a string, empty when there is none, and no
  // model_ref; GET /ai/runner/pull/{job_id} sends both differently.
  for (const auto& pull : client->list_pulls()) {
    value.pulls.push_back({
        pull.job_id,
        runner_id,
        std::nullopt,
        pull.model,
        pull.status,
        pull.updated_at,
        std::optional<std::optional<std::string>>{std::in_place, pull.error},
        {pull.progress.completed, pull.progress.total, pull.progress.percent, pull.progress.stage},
    });
  }
  return value;
}

} // namespace

AiRunnerListResponse get_ai_runners(holder::llm::RunnerRegistry* registry) {
  AiRunnerListResponse response{true, {{}}};
  if (registry == nullptr) return response;
  for (const auto& runner : registry->list_runners()) {
    response.data.runners.push_back({
        runner.runner_id,
        runner.name,
        runner.kind,
        runner.base_url,
        runner.source,
        runner.enabled,
        runner.created_at,
        runner.updated_at,
        runtime(runner.runner_id, registry->get_client(runner.runner_id)),
    });
  }
  return response;
}

} // namespace holder::api
