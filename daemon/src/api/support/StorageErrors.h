#pragma once

#include "api/HttpError.h"

#include <string>

namespace holder::resource { class StorageError; }

namespace holder::api::support {

// The HTTP status and error code for each storage failure category.
struct StorageFailureStatus {
  int status;
  const char* code;
};
StorageFailureStatus storage_failure_status(const holder::resource::StorageError& error);

// Generated routes name one exception per status, so a storage failure is rethrown as the one for
// its status, keeping its code and message.
struct StorageBadRequest : CodedError {
  using CodedError::CodedError;
};
struct StorageConflict : CodedError {
  using CodedError::CodedError;
};
struct StorageIntegrityFailed : CodedError {
  using CodedError::CodedError;
};
struct StorageGatewayFailure : CodedError {
  using CodedError::CodedError;
};
struct StorageUnavailable : CodedError {
  using CodedError::CodedError;
};
struct StorageCapacityExceeded : CodedError {
  using CodedError::CodedError;
};
[[noreturn]] void throw_storage_failure(const holder::resource::StorageError& error);

// A location has no private binding on this device, so it cannot be reached yet.
struct StorageBindingRequired : CodedError {
  explicit StorageBindingRequired(const std::string& message)
      : CodedError("storage_binding_required", message) {}
};

} // namespace holder::api::support
