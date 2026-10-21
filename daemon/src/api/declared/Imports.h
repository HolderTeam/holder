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

} // namespace holder::api
