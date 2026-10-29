#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

namespace holder::api {

struct RIG_OPEN_RECORD ByeResponse {
  bool ok RIG_BOOL_VALUE(true);
};

ByeResponse say_goodbye();

} // namespace holder::api
