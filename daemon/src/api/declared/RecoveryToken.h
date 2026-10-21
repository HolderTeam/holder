#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"


namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(POST, "/recovery-token/import")
RIG_SUMMARY("Import recovery token and auto-match/create project")
RIG_RESPONSE_DESCRIPTION("Imported and matched an existing local project_id")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_RESPONSE(201, "Imported and created a new local project_id from token metadata")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Keyring unavailable")
RecoveryTokenImportAutoResponse post_recovery_token_import(
    ProjectRecoveryTokenImportRequest request);

} // namespace holder::api
