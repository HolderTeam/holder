#pragma once

// Core's own failures as the contract's error kinds (Contract.h), for the operations in src/api/.

#include "api/Contract.h"
#include "model/ConflictError.h"
#include "privacy/PrivacyError.h"

#include <utility>

namespace holder::contract {

// Runs body, turning the failures the contract names into its error kinds. Anything else,
// including a privacy failure the contract does not name, is left as an unexpected failure.
template <class Body>
decltype(auto) with_contract_errors(Body&& body) {
  using holder::privacy::PrivacyErrorCode;
  try {
    return std::forward<Body>(body)();
  } catch (const holder::model::ConflictError& error) {
    throw Conflict(error.what());
  } catch (const holder::privacy::PrivacyError& error) {
    switch (error.code()) {
    case PrivacyErrorCode::KeyringUnavailable: throw KeyringUnavailable(error.what());
    case PrivacyErrorCode::KeyMaterialMissing: throw KeyMissing(error.what());
    case PrivacyErrorCode::EnvelopeInvalid:
    case PrivacyErrorCode::EnvelopeMetadataMismatch:
    case PrivacyErrorCode::CryptMetadataMissing:
    // Plaintext where only encrypted data belongs: Holder refuses to write it.
    case PrivacyErrorCode::EncryptionSafetyCheckFailed: throw DataCorrupted(error.what());
    case PrivacyErrorCode::RecoveryTokenInvalid:
    case PrivacyErrorCode::PrivacyCryptoFailed: break;
    }
    throw;
  }
}

} // namespace holder::contract
