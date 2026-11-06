#include "api/AiRunnerWrite.h"

#include "ai/AiRunnerRepo.h"
#include "api/support/LocalModelRouting.h"
#include "api/support/Time.h"
#include "llm/RunnerRegistry.h"
#include "platform/DeviceConfigStore.h"

#include <spdlog/spdlog.h>

#include <utility>

namespace holder::api {
namespace {

std::string runner_base_url(const std::string& raw) {
  auto value = support::trim_ascii(raw);
  if (value.empty()) throw std::invalid_argument("base_url cannot be empty.");
  const std::string scheme = "http://";
  const auto host_port = value.rfind(scheme, 0) == 0 ? value.substr(scheme.size()) : "";
  const auto colon = host_port.rfind(':');
  if (colon == std::string::npos || colon == 0 || colon == host_port.size() - 1)
    throw std::invalid_argument("base_url must use http://host:port format.");
  return value;
}

std::string runner_name(const std::string& raw) {
  auto name = support::trim_ascii(raw);
  if (name.empty()) throw std::invalid_argument("name cannot be empty.");
  return name;
}

void require_editable(const std::string& runner_id, const char* action) {
  if (runner_id == holder::llm::RunnerRegistry::kAutoLocalRunnerId)
    throw std::invalid_argument(std::string("auto-local runner is not ") + action + ".");
}

// Runner configuration is device configuration; the registry picks up the change.
void saved(holder::platform::Db& db, holder::llm::RunnerRegistry* registry) {
  holder::core::persist_device_config(db);
  if (registry != nullptr) registry->refresh();
}

} // namespace

AiRunnerResponse post_ai_runners(holder::platform::Db& db,
                                 holder::llm::RunnerRegistry* registry,
                                 const std::function<std::string()>& uuid_v4,
                                 AiRunnerCreateRequest request) {
  const long long now = support::now_epoch_seconds();
  holder::model::AiRunner runner;
  runner.name = runner_name(request.name);
  runner.base_url = runner_base_url(request.base_url);
  runner.runner_id = "manual-" + uuid_v4();
  runner.kind = std::move(request.kind);
  runner.source = "manual";
  runner.enabled = request.enabled.value_or(true);
  runner.created_at = now;
  runner.updated_at = now;
  holder::ai::AiRunnerRepo(db).upsert(runner);
  saved(db, registry);
  return {true, runner_with_runtime(runner, registry)};
}

AiRunnerResponse patch_ai_runners_runner_id(holder::platform::Db& db,
                                            holder::llm::RunnerRegistry* registry,
                                            std::string runner_id,
                                            AiRunnerUpdateRequest request) {
  require_editable(runner_id, "editable");
  holder::ai::AiRunnerRepo repo(db);
  auto runner = repo.get(runner_id);
  if (!runner) throw AiRunnerNotFound{};
  if (request.name) runner->name = runner_name(*request.name);
  if (request.enabled) runner->enabled = *request.enabled;
  if (request.base_url) {
    runner->base_url = *request.base_url ? std::optional(runner_base_url(**request.base_url))
                                         : std::nullopt;
  }
  runner->updated_at = support::now_epoch_seconds();
  repo.upsert(*runner);
  saved(db, registry);
  return {true, runner_with_runtime(*runner, registry)};
}

AiRunnerDeleteResponse delete_ai_runners_runner_id(holder::platform::Db& db,
                                                   holder::llm::RunnerRegistry* registry,
                                                   std::string runner_id) {
  require_editable(runner_id, "deletable");
  holder::ai::AiRunnerRepo repo(db);
  if (!repo.get(runner_id)) throw AiRunnerNotFound{};
  repo.remove(runner_id);
  saved(db, registry);
  return {true, {std::move(runner_id)}};
}

AiRunnerResponse post_ai_runners_runner_id_retry(holder::llm::RunnerRegistry* registry,
                                                 std::string runner_id) {
  if (registry == nullptr) throw AiRunnerNotFound{};
  auto* client = registry->get_client(runner_id);
  const auto runner = registry->get_runner(runner_id);
  if (!runner || client == nullptr) throw AiRunnerNotConfigured{};
  spdlog::info("AI runner retry requested runner_id=" + runner_id);
  (void)client->retry();
  registry->refresh();
  return {true, runner_with_runtime(registry->get_runner(runner_id).value_or(*runner), registry)};
}

} // namespace holder::api
