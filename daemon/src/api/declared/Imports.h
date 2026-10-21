#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

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
