#pragma once

#include "api/RigAnnotations.h"
#include "api/Schemas.h"

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_DESCRIPTION("Replaces the whole configuration: an omitted, null or empty model clears it.") AiLocalModelConfigUpdateRequest {
  std::optional<std::optional<std::string>> fast_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> strong_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> deep_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<long long>> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or null.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

RIG_ROUTE(GET, "/ai/local-models/config")
RIG_SUMMARY("Get local model configuration")
RIG_RESPONSE_DESCRIPTION("OK")
AiLocalModelConfigResponse get_local_model_config(holder::platform::Db& db RIG_CONTEXT);

RIG_ROUTE(PUT, "/ai/local-models/config")
RIG_SUMMARY("Set or clear local model configuration")
RIG_RESPONSE_DESCRIPTION("The stored configuration")
AiLocalModelConfigResponse put_local_model_config(holder::platform::Db& db RIG_CONTEXT,
                                                  AiLocalModelConfigUpdateRequest request);

} // namespace holder::api
