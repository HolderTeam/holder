#pragma once

// What the C API's opaque handles hold. Shared by the hand-written C API (holder.cpp) and the
// implementations of the contract's operations (src/api/), which receive a holder_context&.

#include "git/GitCredentialProvider.h"
#include "index/FtsIndexer.h"
#include "platform/Db.h"

#include <filesystem>
#include <memory>
#include <string>

namespace holder::git { class GitOps; }
namespace holder::privacy { class SecretStore; }

// The services operations run with. holder_context_open makes a context that owns its database
// and indexer; a host with its own, such as a daemon worker, makes one on them instead, and they
// must outlive it.
struct holder_context {
  holder_context()
      : owned_db(std::make_unique<holder::platform::Db>()),
        owned_fts(std::make_unique<holder::index::FtsIndexer>(*owned_db)),
        db(*owned_db),
        fts(*owned_fts) {}

  holder_context(holder::platform::Db& host_db, holder::index::FtsIndexer& host_fts,
                 holder::git::GitOps* host_git = nullptr)
      : db(host_db), fts(host_fts), git(host_git) {}

  holder_context(const holder_context&) = delete;
  holder_context& operator=(const holder_context&) = delete;

  std::filesystem::path data_dir;
  std::filesystem::path db_path;
  // Where new projects go when their caller names no directory: data_dir/projects for
  // holder_context_open; a host gives its own.
  std::filesystem::path projects_root;
  // The host's project registry (project/ProjectRegistry.h), which finds projects outside
  // projects_root and remembers removals; empty for none.
  std::filesystem::path registry_path;

 private:
  std::unique_ptr<holder::platform::Db> owned_db;
  std::unique_ptr<holder::index::FtsIndexer> owned_fts;

 public:
  holder::platform::Db& db;
  holder::index::FtsIndexer& fts;
  // Git work for cards; null for core's own. A daemon passes the one its workers share.
  holder::git::GitOps* git = nullptr;
  std::shared_ptr<holder::git::GitCredentialProvider> credential_provider; // nullable
  // A host can supply its device keyring for location binding metadata.
  holder::privacy::SecretStore* secret_store = nullptr;
};

namespace holder::capi {

// Marks a data directory whose database may be rebuilt from its managed projects alone.
inline std::filesystem::path rebuild_readiness_path(const std::filesystem::path& data_dir) {
  return data_dir / "server" / "database-rebuild-ready.json";
}

} // namespace holder::capi

struct holder_error {
  std::string message;
};
