#pragma once

// What the C API's opaque handles hold. Shared by the hand-written C API (holder.cpp) and the
// implementations of the contract's operations (src/api/), which receive a holder_context&.

#include "git/GitCredentialProvider.h"
#include "index/FtsIndexer.h"
#include "platform/Db.h"

#include <filesystem>
#include <memory>
#include <string>

struct holder_context {
  std::filesystem::path data_dir;
  std::filesystem::path db_path;
  holder::platform::Db db;
  holder::index::FtsIndexer fts{db};
  std::shared_ptr<holder::git::GitCredentialProvider> credential_provider; // nullable
};

struct holder_error {
  std::string message;
};
