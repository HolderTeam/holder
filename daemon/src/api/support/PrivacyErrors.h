#pragma once

#include "privacy/PrivacyError.h"

namespace holder::api::support {

// Generated routes name one exception per status, so a privacy failure is rethrown as the one
// for its status, keeping its code: 503 when the keyring is unavailable, otherwise 400.
struct KeyringUnavailable : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};
struct PrivacyRejected : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};

[[noreturn]] inline void throw_privacy_failure(const holder::privacy::PrivacyError& error) {
  if (error.code() == holder::privacy::PrivacyErrorCode::KeyringUnavailable)
    throw KeyringUnavailable(error.code(), error.what());
  throw PrivacyRejected(error.code(), error.what());
}

} // namespace holder::api::support
