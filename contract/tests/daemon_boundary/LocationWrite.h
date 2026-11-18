#pragma once
#include "api/HttpError.h"
#include "api/generated/DaemonTypes.h"

namespace holder::platform {
class Db {
 public:
  int calls = 0;
};
} // namespace holder::platform
namespace holder::privacy {
class SecretStore {};
} // namespace holder::privacy
namespace holder::api {
struct LocationNotFound : std::runtime_error {
  LocationNotFound()
      : std::runtime_error("missing") {}
};
struct SecretStoreUnavailable : CodedError {
  SecretStoreUnavailable()
      : CodedError("secret_store_unavailable", "unavailable") {}
};
} // namespace holder::api
