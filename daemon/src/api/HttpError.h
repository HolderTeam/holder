#pragma once

#include "api/AiRuntimeTypes.h"

#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace holder::privacy { class PrivacyError; }

namespace holder::api {

struct Error {
  std::string code;
  std::string message;
};

struct ErrorResponse {
  bool ok;
  Error error;
};

// A failure that names its own error code instead of the status's generic one.
struct CodedError : std::runtime_error {
  CodedError(std::string error_code, const std::string& message)
      : std::runtime_error(message), code(std::move(error_code)) {}
  std::string code;
};

// The daemon was started without a card store, so card bodies cannot be read or written.
struct CardStoreUnavailable : CodedError {
  CardStoreUnavailable() : CodedError("not_implemented", "Card store unavailable.") {}
};

// The daemon has no secret store, so device-private values cannot be read or written.
struct SecretStoreUnavailable : CodedError {
  SecretStoreUnavailable() : CodedError("secret_store_unavailable", "Secret store unavailable.") {}
};

// Generated dispatch builds the body for each exception named by an
// operation's declared error response; the PrivacyError and CodedError overloads keep their codes.
ErrorResponse api_error_body(std::type_identity<ErrorResponse>, int status,
                             const std::exception& error);
ErrorResponse api_error_body(std::type_identity<ErrorResponse>, int status,
                             const holder::privacy::PrivacyError& error);
ErrorResponse api_error_body(std::type_identity<ErrorResponse>, int status,
                             const CodedError& error);

} // namespace holder::api
