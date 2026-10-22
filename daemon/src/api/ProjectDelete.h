#pragma once

#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <exception>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectDeleteResponseData {
  std::string project_id;
};

struct RIG_OPEN_RECORD ProjectDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectDeleteResponseData data;
};

struct ProjectDeleteNotFound : std::runtime_error {
  ProjectDeleteNotFound() : std::runtime_error("Project not found.") {}
};

RIG_ROUTE(DELETE, "/projects/{project_id}")
RIG_SUMMARY("Delete a project")
RIG_DESCRIPTION("Removes the project from this device's database and records its removal so startup discovery and database reconstruction do not automatically import it again. Project files and encryption keys are retained for explicit recovery. Creating or importing the project explicitly registers it again.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", ProjectDeleteNotFound)
RIG_ERROR_RESPONSE(500, ErrorResponse, "Could not persist the project removal; the database deletion is rolled back.", std::exception)
ProjectDeleteResponse delete_project(holder::platform::Db& db RIG_CONTEXT,
                                     std::string project_id);

} // namespace holder::api
