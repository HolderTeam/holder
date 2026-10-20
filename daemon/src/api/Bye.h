#pragma once

#include "api/RigAnnotations.h"

namespace holder::api {

struct ByeResponse {
  bool ok;
};

RIG_ROUTE(POST, "/bye")
ByeResponse say_goodbye();

} // namespace holder::api
