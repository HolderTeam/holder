#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD ProjectTag {
  std::string tag RIG_FIELD_DESCRIPTION("Normalized tag without the leading");
  int card_count RIG_MINIMUM(1)
      RIG_FIELD_DESCRIPTION("Number of live cards in the project carrying this tag.");
};

struct RIG_OPEN_RECORD ProjectTagListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<ProjectTag> data
      RIG_FIELD_DESCRIPTION("Tags ordered by descending card count, then ascending tag.");
};

struct ProjectNotFound : std::runtime_error {
  ProjectNotFound() : std::runtime_error("Project not found.") {}
};

RIG_ROUTE(GET, "/projects/{project_id}/tags")
RIG_SUMMARY("List project tags with live-card counts")
RIG_RESPONSE_DESCRIPTION("Project tags ordered by descending card count, then ascending tag")
RIG_ERROR_RESPONSE(401, ErrorResponse, "Unauthorized")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
RIG_ERROR_RESPONSE(500, ErrorResponse, "Internal server error")
ProjectTagListResponse list_project_tags(holder::platform::Db& db RIG_CONTEXT,
                                         std::string project_id);

} // namespace holder::api
