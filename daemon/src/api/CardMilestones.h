#pragma once

#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::model { struct Milestone; }

namespace holder::api {

// Whether a milestone has the timestamp view: a timestamp start and at most one end.
bool has_timestamp_view(const holder::model::Milestone& milestone);

// A milestone's timestamp view, as the card milestone routes and the calendar report it.
Milestone timestamp_milestone(const holder::model::Milestone& milestone,
                              std::optional<std::string> card_title = std::nullopt);

struct RIG_OPEN_RECORD MilestoneListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<Milestone> data;
};

struct MilestoneCardNotFound : std::runtime_error {
  MilestoneCardNotFound() : std::runtime_error("Card not found.") {}
};

RIG_ROUTE(GET, "/cards/{card_id}/milestones")
RIG_SUMMARY("List milestones attached to a card")
RIG_DESCRIPTION("This compatibility timestamp view omits undated milestones and milestones with repeated start or end roles. Calendar-day values project to UTC midnight. Use /cards/{card_id}/milestones-with-dates for the complete model.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found", MilestoneCardNotFound)
MilestoneListResponse list_card_milestones(holder::platform::Db& db RIG_CONTEXT,
                                           std::string card_id);

} // namespace holder::api
