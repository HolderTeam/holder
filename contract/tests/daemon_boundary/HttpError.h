#pragma once
#include <stdexcept>
#include <string>
#include <type_traits>

namespace holder::api {
struct Error {
  std::string code;
  std::string message;
};
struct ErrorResponse {
  bool ok;
  Error error;
};
struct CodedError : std::runtime_error {
  std::string code;
  CodedError(const char* code, const char* message)
      : std::runtime_error(message),
        code(code) {}
};
inline ErrorResponse api_error_body(
    std::type_identity<ErrorResponse>,
    int status,
    const std::exception& error
) {
  return {false, {status == 404 ? "not_found" : "error", error.what()}};
}
inline ErrorResponse api_error_body(
    std::type_identity<ErrorResponse>,
    int,
    const CodedError& error
) {
  return {false, {error.code, error.what()}};
}
} // namespace holder::api
