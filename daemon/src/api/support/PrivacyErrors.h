#pragma once

#include "privacy/PrivacyError.h"

namespace holder::api::support {

// Generated routes name one exception per status, so a privacy failure is rethrown as the one
// for its status, keeping its code: 503 when the keyring is unavailable, 500 when the project's
// key material or crypt metadata is missing, otherwise 400.
struct KeyringUnavailable : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};
struct PrivacyRejected : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};
struct PrivacyBroken : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};

[[noreturn]] inline void throw_privacy_failure(const holder::privacy::PrivacyError& error) {
  switch (error.code()) {
  case holder::privacy::PrivacyErrorCode::KeyringUnavailable:
    throw KeyringUnavailable(error.code(), error.what());
  case holder::privacy::PrivacyErrorCode::KeyMaterialMissing:
  case holder::privacy::PrivacyErrorCode::CryptMetadataMissing:
    throw PrivacyBroken(error.code(), error.what());
  default:
    break;
  }
  throw PrivacyRejected(error.code(), error.what());
}

} // namespace holder::api::support
