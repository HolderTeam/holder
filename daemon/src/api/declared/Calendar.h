#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(GET, "/calendar")
RIG_SUMMARY("List project milestones and card activity in a time range")
RIG_DESCRIPTION("This legacy calendar view selects milestone start values and card activity by epoch timestamps; both from and to are inclusive. Calendar-day values use UTC midnight in this view. Undated milestones are omitted. Date parsing and timezone conversion are client concerns because this endpoint accepts epoch seconds.")
RIG_RESPONSE_DESCRIPTION("Calendar activity in the requested inclusive epoch-second range.")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
ProjectCalendarResponse get_calendar(
    std::string project_id RIG_QUERY,
    long long from RIG_QUERY RIG_PARAMETER_DESCRIPTION("Inclusive lower bound in Unix epoch seconds."),
    long long to RIG_QUERY RIG_PARAMETER_DESCRIPTION("Inclusive upper bound in Unix epoch seconds."));

} // namespace holder::api
