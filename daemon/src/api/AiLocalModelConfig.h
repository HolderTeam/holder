#pragma once

#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

namespace holder::platform { class Db; }

namespace holder::api {

RIG_ROUTE(GET, "/ai/local-models/config")
RIG_SUMMARY("Get local model configuration")
RIG_RESPONSE_DESCRIPTION("OK")
AiLocalModelConfigResponse get_local_model_config(holder::platform::Db& db RIG_CONTEXT);

} // namespace holder::api
