#include "api/support/StorageErrors.h"

#include "resource/StorageProvider.h"

namespace holder::api::support {

StorageFailureStatus storage_failure_status(const holder::resource::StorageError& error) {
  using Code = holder::resource::StorageErrorCode;
  switch (error.code()) {
  case Code::Unavailable: return {503, "storage_unavailable"};
  case Code::Authentication: return {502, "storage_authentication_failed"};
  case Code::Permission: return {502, "storage_permission_denied"};
  case Code::Capacity: return {507, "storage_capacity_exceeded"};
  case Code::Integrity: return {422, "storage_integrity_failed"};
  case Code::Conflict: return {409, "storage_conflict"};
  case Code::InvalidConfiguration: return {400, "storage_configuration_invalid"};
  case Code::Transient: return {503, "storage_transient_failure"};
  }
  return {503, "storage_unavailable"}; // LCOV_EXCL_LINE: every category is listed.
}

void throw_storage_failure(const holder::resource::StorageError& error) {
  const auto [status, code] = storage_failure_status(error);
  switch (status) {
  case 400: throw StorageBadRequest(code, error.what());
  case 409: throw StorageConflict(code, error.what());
  case 422: throw StorageIntegrityFailed(code, error.what());
  case 502: throw StorageGatewayFailure(code, error.what());
  case 507: throw StorageCapacityExceeded(code, error.what());
  default: throw StorageUnavailable(code, error.what());
  }
}

} // namespace holder::api::support
