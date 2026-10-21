#include "api/AiLocalModelConfig.h"

#include "ai/AiLocalModelConfigRepo.h"

#include <optional>

namespace holder::api {

AiLocalModelConfigResponse get_local_model_config(holder::platform::Db& db) {
  const auto config = holder::ai::AiLocalModelConfigRepo(db).get();
  if (!config) return {true, {}};
  return {true,
          {config->fast_model, config->strong_model, config->deep_model, config->updated_at}};
}

} // namespace holder::api
