#pragma once

#include "api/AiRuntimeTypes.h"
#include "api/Schemas.h"

namespace holder::platform { class Db; }

namespace holder::api {

struct AiLocalModelConfigUpdateRequest {
  std::optional<std::optional<std::string>> fast_model;
  std::optional<std::optional<std::string>> strong_model;
  std::optional<std::optional<std::string>> deep_model;
  std::optional<std::optional<long long>> updated_at;
};

AiLocalModelConfigResponse get_local_model_config(holder::platform::Db& db);

AiLocalModelConfigResponse put_local_model_config(holder::platform::Db& db,
                                                  AiLocalModelConfigUpdateRequest request);

} // namespace holder::api
