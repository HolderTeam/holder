#pragma once

#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::model { struct Milestone; }

namespace holder::api {

// A milestone with its complete ordered dates, as the labelled dates routes report it.
MilestoneWithDates milestone_with_dates(const holder::model::Milestone& milestone);

struct ProjectMilestonesInvalidQuery : std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

struct ProjectMilestonesNotFound : std::runtime_error {
  ProjectMilestonesNotFound() : std::runtime_error("Project not found.") {}
};

struct CardMilestonesNotFound : std::runtime_error {
  CardMilestonesNotFound() : std::runtime_error("Card not found.") {}
};

RIG_ROUTE(GET, "/cards/{card_id}/milestones-with-dates")
RIG_SUMMARY("List all milestones on a live card with their labelled dates")
RIG_DESCRIPTION("Includes undated milestones and repeated roles. Dates remain in stored order. The existing /cards/{card_id}/milestones route remains a scheduled timestamp view.")
RIG_RESPONSE_DESCRIPTION("All milestones on the card")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found", CardMilestonesNotFound)
MilestoneWithDatesListResponse get_card_milestones_with_dates(holder::platform::Db& db RIG_CONTEXT,
                                                              std::string card_id);

RIG_ROUTE(GET, "/projects/{project_id}/milestones-with-dates")
RIG_SUMMARY("Query project milestones by a labelled date role")
RIG_DESCRIPTION("Returns each matching milestone once with its complete ordered dates array. Calendar days and instants are queried separately; both range bounds are inclusive. Milestones on trashed cards are excluded. Roles are free-form, and this endpoint does not infer outstanding or completion state.")
RIG_RESPONSE_DESCRIPTION("Matching milestones")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid role or range", ProjectMilestonesInvalidQuery)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", ProjectMilestonesNotFound)
MilestoneWithDatesListResponse get_project_milestones_with_dates(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id,
    std::string role RIG_QUERY RIG_MIN_LENGTH(1),
    std::string value_type RIG_QUERY RIG_STRING_VALUES("date", "datetime"),
    std::string from RIG_QUERY RIG_PARAMETER_DESCRIPTION("YYYY-MM-DD when value_type=date; Unix epoch seconds when value_type=datetime."),
    std::string to RIG_QUERY RIG_PARAMETER_DESCRIPTION("Inclusive upper bound in the same form as from."));

} // namespace holder::api
