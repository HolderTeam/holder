#pragma once

#include "api/AiRuntimeTypes.h"
#include "api/ApiService.h"
#include "api/HttpError.h"

namespace holder::api {

struct ByeResponse {
  bool ok;
};

ByeResponse say_goodbye();

} // namespace holder::api
