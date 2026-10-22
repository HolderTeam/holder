#include "api/AiLocalModelConfig.h"

#include "ai/AiLocalModelConfigRepo.h"
#include "api/support/Time.h"
#include "platform/DeviceConfigStore.h"

#include <optional>
#include <string>

namespace holder::api {

AiLocalModelConfigResponse get_local_model_config(holder::platform::Db& db) {
  const auto config = holder::ai::AiLocalModelConfigRepo(db).get();
  if (!config) return {true, {}};
  return {true,
          {config->fast_model, config->strong_model, config->deep_model, config->updated_at}};
}

namespace {

// An empty model name clears that model.
std::optional<std::string> model(const std::optional<std::optional<std::string>>& value) {
  auto name = value.value_or(std::nullopt);
  if (name && name->empty()) name.reset();
  return name;
}

} // namespace

AiLocalModelConfigResponse put_local_model_config(holder::platform::Db& db,
                                                  AiLocalModelConfigUpdateRequest request) {
  holder::ai::AiLocalModelConfigRepo(db).set(
      model(request.fast_model), model(request.strong_model), model(request.deep_model),
      request.updated_at.value_or(std::nullopt).value_or(support::now_epoch_seconds()));
  holder::core::persist_device_config(db);
  return get_local_model_config(db);
}

} // namespace holder::api
