#include "api/AiRuns.h"

#include "ai/AiRunRepo.h"
#include "llm/RunnerModelRef.h"

#include <exception>
#include <optional>
#include <string>
#include <vector>

namespace holder::api {
namespace {

template <typename T>
std::optional<std::optional<T>> present(std::optional<T> value) {
  return std::optional<std::optional<T>>{std::in_place, std::move(value)};
}

std::optional<nlohmann::json> parse_json(const std::optional<std::string>& raw) {
  if (!raw || raw->empty()) return std::nullopt;
  try {
    auto value = nlohmann::json::parse(*raw);
    if (value.is_null()) return std::nullopt;
    return value;
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

AiRunItem run_item(const holder::model::AiRun& run) {
  auto policy_trace = parse_json(run.policy_trace_json);
  if (!policy_trace && run.ranked_json) {
    // Older rows stored cloud policy traces in ranked_json.
    auto ranked = parse_json(run.ranked_json);
    if (ranked && ranked->is_object()) policy_trace = std::move(ranked);
  }
  return {
      run.run_id,
      present(run.project_id),
      present(run.thread_id),
      present(run.message_id),
      run.mode,
      run.prompt,
      present(run.context_json),
      present(run.router_model),
      present(holder::llm::runner_id_from_ref(run.router_model)),
      present(run.ranked_json),
      present(run.policy_trace_json),
      present(std::move(policy_trace)),
      present(run.chosen_model),
      present(holder::llm::runner_id_from_ref(run.chosen_model)),
      run.status,
      present(run.error),
      run.created_at,
      run.updated_at,
  };
}

} // namespace

AiRunListResponse get_ai_runs(holder::platform::Db& db,
                              std::optional<std::string> project_id,
                              std::optional<std::string> thread_id) {
  const bool by_thread = thread_id && !thread_id->empty();
  if (!by_thread && (!project_id || project_id->empty())) throw AiRunsMissingScope{};
  holder::ai::AiRunRepo repo(db);
  const auto runs = by_thread ? repo.list_by_thread(*thread_id) : repo.list_by_project(*project_id);
  AiRunListResponse response{true, {}};
  response.data.reserve(runs.size());
  for (const auto& run : runs) response.data.push_back(run_item(run));
  return response;
}

AiRunGetResponse get_ai_run(holder::platform::Db& db, std::string run_id) {
  const auto run = holder::ai::AiRunRepo(db).get(run_id);
  if (!run) throw AiRunNotFound{};
  return {true, run_item(*run)};
}

} // namespace holder::api
