#pragma once

#include "api/RigAnnotations.h"

#include <string>

namespace holder::api {

struct RIG_OPEN_RECORD Error {
  std::string code;
  std::string message;
};

struct RIG_OPEN_RECORD ErrorResponse {
  bool ok RIG_BOOL_VALUE(false);
  Error error;
};

} // namespace holder::api
