#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(POST, "/cards/{card_id}/milestones-with-dates")
RIG_SUMMARY("Add a milestone with labelled dates to a live card")
RIG_RESPONSE_DESCRIPTION("Created milestone")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid milestone dates")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found")
MilestoneWithDatesResponse post_cards_card_id_milestones_with_dates(
    std::string card_id,
    MilestoneWithDatesCreateRequest request);

RIG_ROUTE(PATCH, "/cards/{card_id}/milestones-with-dates/{milestone_id}")
RIG_SUMMARY("Update a milestone's labelled dates and metadata")
RIG_DESCRIPTION("Supplying dates replaces the complete ordered array. Milestone identity and creation time remain unchanged.")
RIG_RESPONSE_DESCRIPTION("Updated milestone")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid update")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card or milestone not found")
MilestoneWithDatesResponse patch_cards_card_id_milestones_with_dates_milestone_id(
    std::string card_id,
    std::string milestone_id RIG_FORMAT("uuid"),
    MilestoneWithDatesUpdateRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}/milestones-with-dates/{milestone_id}")
RIG_SUMMARY("Remove a milestone from a live card")
RIG_DESCRIPTION("Equivalent to deleting the same milestone through the scheduled timestamp route.")
RIG_RESPONSE_DESCRIPTION("Removal result")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found")
MilestoneRemoveResponse delete_cards_card_id_milestones_with_dates_milestone_id(
    std::string card_id,
    std::string milestone_id RIG_FORMAT("uuid"));

RIG_ROUTE(POST, "/cards/{card_id}/milestones")
RIG_SUMMARY("Add a milestone to a card")
RIG_DESCRIPTION("Creates a scheduled timestamp milestone. Milestones carry optional kind and description fields and have no independent title.")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found")
MilestoneResponse post_cards_card_id_milestones(
    std::string card_id,
    MilestoneCreateRequest request);

RIG_ROUTE(PATCH, "/cards/{card_id}/milestones/{milestone_id}")
RIG_SUMMARY("Partially update a milestone on a live card")
RIG_DESCRIPTION("Updates only supplied fields while preserving milestone identity and creation metadata.")
RIG_RESPONSE_DESCRIPTION("Updated milestone")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid or empty update")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card or milestone not found")
MilestoneResponse patch_cards_card_id_milestones_milestone_id(
    std::string card_id,
    std::string milestone_id RIG_FORMAT("uuid") RIG_PARAMETER_DESCRIPTION("Canonical milestone identifier; it is not a card reference and is never abbreviated."),
    MilestoneUpdateRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}/milestones/{milestone_id}")
RIG_SUMMARY("Remove a milestone from a card")
RIG_RESPONSE_DESCRIPTION("Removal result")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found")
MilestoneRemoveResponse delete_cards_card_id_milestones_milestone_id(
    std::string card_id,
    std::string milestone_id);

} // namespace holder::api
