#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

struct RIG_OPEN_RECORD AssetImportJob {
  std::string job_id;
  std::string status RIG_STRING_VALUES("queued", "staging", "storing", "committing", "completed", "failed");
  std::optional<std::optional<std::string>> resource_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> asset_id RIG_OMIT_EMPTY RIG_NULLABLE;
  bool duplicate_reused;
  std::optional<bool> link_created RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> error RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AssetImportJobResponse {
  bool ok RIG_BOOL_VALUE(true);
  AssetImportJob data;
};

struct RIG_OPEN_RECORD AssetImportRequest {
  std::string project_id;
  std::string card_id;
  std::string location_id;
  std::string source_path RIG_FIELD_DESCRIPTION("Local readable regular file; never persisted into project metadata.");
};

RIG_ROUTE(POST, "/imports")
RIG_SUMMARY("Start a local-file asset import")
RIG_RESPONSE_DESCRIPTION("Import queued")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(202)
AssetImportJobResponse post_imports(AssetImportRequest request);

RIG_ROUTE(GET, "/imports/{job_id}")
RIG_SUMMARY("Poll an asset import job")
RIG_RESPONSE_DESCRIPTION("Current job state")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Job not found")
AssetImportJobResponse get_imports_job_id(std::string job_id);

} // namespace holder::api
