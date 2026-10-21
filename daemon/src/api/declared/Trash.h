#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(DELETE, "/trash")
RIG_SUMMARY("Empty trash by project")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
TrashDeleteResponse delete_trash(
    std::string project_id RIG_QUERY,
    std::optional<std::string> type RIG_QUERY RIG_PARAMETER_DESCRIPTION("card | ai_message | all"));

RIG_ROUTE(DELETE, "/trash/{type}/{id}")
RIG_SUMMARY("Permanently delete a trashed item (legacy global-ID route)")
RIG_DESCRIPTION("The item must still be in Trash. Use the project-scoped route for new clients.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Item not found")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Item is not in Trash")
RIG_ERROR_RESPONSE(501, ErrorResponse, "Card store unavailable")
TrashDeleteResponse delete_trash_type_id(std::string type, std::string id);

} // namespace holder::api
