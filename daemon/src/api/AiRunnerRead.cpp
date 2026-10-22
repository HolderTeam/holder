#include "api/AiRunnerRead.h"

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
  // The same shape as GET /ai/runner/pull/{job_id}.
  for (const auto& pull : client->list_pulls()) {
    value.pulls.push_back({
        pull.job_id,
        runner_id,
        holder::llm::make_runner_model_ref(runner_id, pull.model),
        pull.model,
        pull.status,
        holder::llm::pull_job_active(pull),
        pull.updated_at,
        std::optional<std::optional<std::string>>{std::in_place, unless_empty(pull.error)},
        {pull.progress.completed, pull.progress.total, pull.progress.percent, pull.progress.stage},
    });
  }
  return value;
}

} // namespace

AiRunner runner_with_runtime(const holder::model::AiRunner& runner,
                             holder::llm::RunnerRegistry* registry) {
  return {
      runner.runner_id,
      runner.name,
      runner.kind,
      runner.base_url,
      runner.source,
      runner.enabled,
      runner.created_at,
      runner.updated_at,
      runtime(runner.runner_id, registry ? registry->get_client(runner.runner_id) : nullptr),
  };
}

std::vector<AiRunner> runners_with_runtime(holder::llm::RunnerRegistry* registry) {
  std::vector<AiRunner> runners;
  if (registry == nullptr) return runners;
  for (const auto& runner : registry->list_runners()) {
    runners.push_back(runner_with_runtime(runner, registry));
  }
  return runners;
}

AiRunnerListResponse get_ai_runners(holder::llm::RunnerRegistry* registry) {
  return {true, {runners_with_runtime(registry)}};
}

AiRunnerResponse get_ai_runner(holder::llm::RunnerRegistry* registry, std::string runner_id) {
  if (registry == nullptr) throw AiRunnerNotFound{};
  const auto runner = registry->get_runner(runner_id);
  if (!runner) throw AiRunnerNotFound{};
  return {true, runner_with_runtime(*runner, registry)};
}

} // namespace holder::api
