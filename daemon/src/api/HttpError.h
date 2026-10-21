#pragma once

#include "api/RigAnnotations.h"

#include <exception>
#include <string>
#include <type_traits>

namespace holder::privacy { class PrivacyError; }

namespace holder::api {

struct RIG_OPEN_RECORD Error {
  std::string code;
  std::string message;
};

struct RIG_OPEN_RECORD ErrorResponse {
  bool ok RIG_BOOL_VALUE(false);
  Error error;
};

// Generated dispatch builds the body for each exception named by an
// operation's RIG_ERROR_RESPONSE; the PrivacyError overload keeps its code.
ErrorResponse rig_error_body(std::type_identity<ErrorResponse>, int status,
                             const std::exception& error);
ErrorResponse rig_error_body(std::type_identity<ErrorResponse>, int status,
                             const holder::privacy::PrivacyError& error);

} // namespace holder::api
