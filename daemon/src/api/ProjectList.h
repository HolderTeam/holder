#pragma once

#include "api/ApiService.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD ProjectListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<Project> data;
};

RIG_ROUTE(GET, "/projects")
RIG_SUMMARY("List projects")
RIG_RESPONSE_DESCRIPTION("OK")
ProjectListResponse get_projects(
    holder::platform::Db& db RIG_CONTEXT,
    std::optional<std::string> name RIG_QUERY RIG_PARAMETER_DESCRIPTION("Substring match on project name."),
    std::optional<long long> updated_after RIG_QUERY RIG_PARAMETER_DESCRIPTION("Return projects with updated_at >= updated_after."),
    std::optional<long long> updated_before RIG_QUERY RIG_PARAMETER_DESCRIPTION("Return projects with updated_at <= updated_before."),
    std::optional<std::string> order RIG_QUERY RIG_STRING_VALUES("updated_at_desc", "updated_at_asc", "created_at_desc", "created_at_asc", "name_desc", "name_asc") RIG_PARAMETER_DESCRIPTION("Order by updated_at (default), created_at, or name."),
    std::optional<int> limit RIG_QUERY RIG_MINIMUM(0) RIG_FORMAT("int32") RIG_PARAMETER_DESCRIPTION("Max results (default 100, capped at 1000)."),
    std::optional<int> offset RIG_QUERY RIG_MINIMUM(0) RIG_FORMAT("int32") RIG_PARAMETER_DESCRIPTION("Offset into ordered results (default 0)."),
    bool count RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include count fields (card_count/root_card_count) per project."));

} // namespace holder::api
