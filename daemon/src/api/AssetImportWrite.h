#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"
#include "api/support/StorageErrors.h"

#include <functional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }
namespace holder::git { class GitOps; }

namespace holder::api {

struct RIG_OPEN_RECORD AssetImportRequest {
  std::string project_id;
  std::string card_id;
  std::string location_id;
  std::string source_path RIG_FIELD_DESCRIPTION("Local readable regular file; never persisted into project metadata.");
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

RIG_ROUTE(POST, "/imports")
RIG_SUMMARY("Start a local-file asset import")
RIG_DESCRIPTION("The location and its credentials are checked before the job is queued; failures after that are reported by polling GET /imports/{job_id}.")
RIG_RESPONSE_DESCRIPTION("Import queued")
RIG_SUCCESS_STATUS(202)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, or the source is not a readable regular file", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Storage location not found in the project", AssetImportLocationNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "The storage location has no credentials on this device (storage_binding_required)", support::StorageBindingRequired)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon cannot import assets", AssetImportUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Secret store unavailable", SecretStoreUnavailable)
AssetImportJobResponse post_imports(holder::platform::Db& db RIG_CONTEXT,
                                    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
                                    holder::git::GitOps* git_ops RIG_CONTEXT,
                                    const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
                                    AssetImportRequest request);

} // namespace holder::api
