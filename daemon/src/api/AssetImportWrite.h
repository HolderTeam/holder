#pragma once

#include "api/HttpError.h"
#include "api/Schemas.h"
#include "api/support/StorageErrors.h"

#include <functional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }
namespace holder::git { class GitOps; }

namespace holder::api {

struct AssetImportRequest {
  std::string project_id;
  std::string card_id;
  std::string location_id;
  std::string source_path;
};

struct AssetImportLocationNotFound : std::runtime_error {
  AssetImportLocationNotFound() : std::runtime_error("storage location not found in project") {}
};

// The daemon was started without the Git executor imports commit through.
struct AssetImportUnavailable : CodedError {
  AssetImportUnavailable() : CodedError("not_implemented", "Asset import unavailable.") {}
};

// Listener shutdown calls this before destroying the serialized Git executor used by imports.
void wait_for_asset_import_jobs();

AssetImportJobResponse post_imports(holder::platform::Db& db,
                                    holder::privacy::SecretStore* secret_store,
                                    holder::git::GitOps* git_ops,
                                    const std::function<std::string()>& uuid_v4,
                                    AssetImportRequest request);

} // namespace holder::api
