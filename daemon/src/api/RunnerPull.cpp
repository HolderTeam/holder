#include "api/RunnerPull.h"

#include "llm/LocalModelRunner.h"
#include "llm/RunnerModelRef.h"
#include "llm/RunnerRegistry.h"

#include <spdlog/spdlog.h>

#include <string>

namespace holder::api {

RunnerPullJob pull_job_status(const holder::llm::RunnerPullJob& job, const std::string& runner_id) {
  std::optional<std::string> error;
  if (!job.error.empty()) error = job.error;
  return {
      job.job_id,
      runner_id,
      holder::llm::make_runner_model_ref(runner_id, job.model),
      job.model,
      job.status,
      holder::llm::pull_job_active(job),
      job.updated_at,
      std::optional<std::optional<std::string>>{std::in_place, error},
      {job.progress.completed, job.progress.total, job.progress.percent, job.progress.stage}
  };
}

RunnerPullStatusResponse get_pull_status(
    holder::llm::RunnerRegistry* registry,
    std::optional<std::string> requested_runner_id,
    std::string job_id
) {
  const std::string runner_id = requested_runner_id.has_value() && !requested_runner_id->empty()
                                    ? *requested_runner_id
                                    : holder::llm::RunnerRegistry::kAutoLocalRunnerId;
  auto* runner = registry ? registry->get_client(runner_id) : nullptr;
  if (!runner) throw RunnerNotConfigured{};
  if (job_id.empty()) throw PullJobNotFound{};

  const auto job = runner->get_pull(job_id);
  if (!job.has_value()) throw PullJobNotFound{};
  return {true, pull_job_status(*job, runner_id)};
}

RunnerPullStartResponse post_ai_runner_pull(holder::llm::RunnerRegistry* registry,
                                            PostAiRunnerPullRequest request) {
  const std::string runner_id = request.runner_id && !request.runner_id->empty()
                                    ? *request.runner_id
                                    : holder::llm::RunnerRegistry::kAutoLocalRunnerId;
  auto* runner = registry ? registry->get_client(runner_id) : nullptr;
  if (!runner) throw RunnerNotConfigured{};
  if (!runner->status().available) throw RunnerUnavailable{};
  spdlog::info("AI runner pull requested runner_id=" + runner_id + " model=" + request.model);
  const auto job = runner->start_pull(request.model);
  if (job.status == "failed")
    throw std::invalid_argument(job.error.empty() ? "Pull failed." : job.error);
  return {true,
          {job.job_id, runner_id, holder::llm::make_runner_model_ref(runner_id, job.model),
           job.model, job.status}};
}

} // namespace holder::api
