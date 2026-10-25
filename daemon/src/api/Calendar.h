#pragma once

#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD CalendarCardActivity {
  std::string card_id;
  std::string title;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectCalendarResponseData {
  std::string project_id;
  long long from RIG_FIELD_DESCRIPTION("Inclusive lower bound in Unix epoch seconds.");
  long long to RIG_FIELD_DESCRIPTION("Inclusive upper bound in Unix epoch seconds.");
  std::vector<Milestone> milestones;
  std::vector<CalendarCardActivity> created_cards;
  std::vector<CalendarCardActivity> updated_cards;
};

struct RIG_OPEN_RECORD ProjectCalendarResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectCalendarResponseData data;
};

struct CalendarInvalidRange : std::invalid_argument {
  CalendarInvalidRange() : std::invalid_argument("from must not be after to.") {}
};

struct CalendarProjectNotFound : std::runtime_error {
  CalendarProjectNotFound() : std::runtime_error("Project not found.") {}
};

RIG_ROUTE(GET, "/calendar")
RIG_SUMMARY("List project milestones and card activity in a time range")
RIG_DESCRIPTION("This legacy calendar view selects milestone start values and card activity by epoch timestamps; both from and to are inclusive. Calendar-day values use UTC midnight in this view. Undated milestones are omitted. Date parsing and timezone conversion are client concerns because this endpoint accepts epoch seconds.")
RIG_RESPONSE_DESCRIPTION("Calendar activity in the requested inclusive epoch-second range.")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", CalendarInvalidRange)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", CalendarProjectNotFound)
ProjectCalendarResponse get_calendar(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1),
    long long from RIG_QUERY RIG_PARAMETER_DESCRIPTION("Inclusive lower bound in Unix epoch seconds."),
    long long to RIG_QUERY RIG_PARAMETER_DESCRIPTION("Inclusive upper bound in Unix epoch seconds."));

} // namespace holder::api
