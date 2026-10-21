#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

struct RIG_OPEN_RECORD TrashItem {
  std::string type;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> message_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> thread_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::string> project_id RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> title RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> role RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> deleted_at RIG_OMIT_EMPTY;
  std::optional<long long> created_at RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> rel_path RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD TrashListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<TrashItem> data;
};

struct RIG_OPEN_RECORD TrashDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  nlohmann::json data;
};

RIG_ROUTE(GET, "/trash")
RIG_SUMMARY("List trashed items by project")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
TrashListResponse get_trash(
    std::string project_id RIG_QUERY,
    std::optional<std::string> type RIG_QUERY RIG_PARAMETER_DESCRIPTION("card | ai_message | all"));

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
