#pragma once

#include "api/RigAnnotations.h"

#include <string>

namespace holder::api {

struct Error {
  std::string code;
  std::string message;
};

struct ErrorResponse {
  bool ok RIG_BOOL_VALUE(false);
  Error error;
};

} // namespace holder::api
