#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"

namespace holder::api {

struct ByeResponse {
  bool ok;
};

ByeResponse say_goodbye();

} // namespace holder::api
