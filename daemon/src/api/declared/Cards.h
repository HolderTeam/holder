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

RIG_ROUTE(GET, "/cards/{card_id}/milestones")
RIG_SUMMARY("List milestones attached to a card")
RIG_DESCRIPTION("This compatibility timestamp view omits undated milestones and milestones with repeated start or end roles. Calendar-day values project to UTC midnight. Use /cards/{card_id}/milestones-with-dates for the complete model.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found")
MilestoneListResponse get_cards_card_id_milestones(std::string card_id);

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

RIG_ROUTE(GET, "/cards")
RIG_SUMMARY("List cards")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
CardListResponse get_cards(
    std::string project_id RIG_QUERY,
    std::optional<std::string> tag RIG_QUERY RIG_PARAMETER_DESCRIPTION("Exact normalized tag to match, without the leading"),
    std::string view RIG_QUERY RIG_STRING_VALUES("tree", "recent") RIG_DEFAULT("tree") RIG_PARAMETER_DESCRIPTION("Card list mode. tree returns one hierarchy level (roots or a parent's children); recent returns recently updated cards."),
    std::optional<std::string> parent_card_id RIG_QUERY RIG_PARAMETER_DESCRIPTION("Optional when view=tree. Omit for roots; set to return that parent card's children."),
    std::optional<std::string> order RIG_QUERY RIG_STRING_VALUES("tree_default", "updated_desc", "title_asc") RIG_PARAMETER_DESCRIPTION("Card ordering strategy. Defaults are view-aware: tree uses tree_default\n(sort_key asc, updated_at desc, title asc), recent uses updated_desc.\n"),
    bool count RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include count fields (child_count) in returned card items."),
    int limit RIG_QUERY RIG_RANGE(1, 5000) RIG_DEFAULT(200) RIG_PARAMETER_DESCRIPTION("Maximum number of cards when view=recent."),
    std::optional<int> include_deleted RIG_QUERY RIG_PARAMETER_DESCRIPTION("Set to 1 to include deleted cards."));

RIG_ROUTE(POST, "/cards")
RIG_SUMMARY("Create a card")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Conflict")
CardCreateResponse post_cards(CardCreateRequest request);

RIG_ROUTE(GET, "/cards/context")
RIG_SUMMARY("Read card context and breadcrumb path for a project level")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or parent card not found")
CardContextResponse get_cards_context(
    std::string project_id RIG_QUERY,
    std::optional<std::string> parent_card_id RIG_QUERY RIG_PARAMETER_DESCRIPTION("Optional parent card id. Omit for project root level."),
    std::string order RIG_QUERY RIG_STRING_VALUES("tree_default", "updated_desc", "title_asc") RIG_DEFAULT("tree_default") RIG_PARAMETER_DESCRIPTION("Ordering for the returned level cards."),
    bool count RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include count fields (child_count) in context cards."));

RIG_ROUTE(GET, "/cards/{card_id}")
RIG_SUMMARY("Fetch a card")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
CardGetResponse get_cards_card_id(std::string card_id);

RIG_ROUTE(PATCH, "/cards/{card_id}")
RIG_SUMMARY("Update card content and/or title")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
CardPatchResponse patch_cards_card_id(std::string card_id, CardUpdateRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}")
RIG_SUMMARY("Move card to trash")
RIG_DESCRIPTION("The card and every changed card are written in one commit. With children=promote (the default), the card's immediate live children take its place among its siblings (or the project's top level) in their existing order, keeping their own descendants; children already in Trash stay there. With children=trash_subtree, every live descendant goes to Trash with the card and keeps its parent there, including live cards beneath descendants that were already trashed, which keep their own trash time. Restoring is always single-card: see POST /cards/{card_id}/restore.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid children value, or the card cannot be trashed (for example it is already in Trash)")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
CardPatchResponse delete_cards_card_id(
    std::string card_id,
    std::string children RIG_QUERY RIG_STRING_VALUES("promote", "trash_subtree") RIG_DEFAULT("promote") RIG_PARAMETER_DESCRIPTION("What happens to the card's live children. Any other value is rejected before anything changes."));

RIG_ROUTE(POST, "/cards/{card_id}/move")
RIG_SUMMARY("Move/reparent/reorder a card using card move intent")
RIG_DESCRIPTION("Placement is resolved and written under the project lock. When the card's new neighbours leave no room between their sort keys (tied keys, or a gap used up by repeated moves), nearby siblings are re-spaced in the same commit, keeping their order. The response describes only the moved card, so a client holding other cards' sort_key values should refresh the listing.")
RIG_RESPONSE_DESCRIPTION("Move applied")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request (missing/invalid intent fields)")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card or target not found")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Stale revision (if_revision mismatch)")
RIG_ERROR_RESPONSE(422, ErrorResponse, "Invalid move (cycle/cross-project/invalid target)")
CardMoveResponse post_cards_card_id_move(std::string card_id, CardMoveRequest request);

RIG_ROUTE(POST, "/cards/{card_id}/tags")
RIG_SUMMARY("Add a tag to a live card")
RIG_RESPONSE_DESCRIPTION("Added or already present")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Missing fields or invalid tag")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found")
RIG_ERROR_RESPONSE(422, ErrorResponse, "Card belongs to a different project")
CardTagMutationResponse post_cards_card_id_tags(
    std::string card_id,
    CardTagMutationRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}/tags")
RIG_SUMMARY("Remove a tag from a live card's editable trailing tag line")
RIG_RESPONSE_DESCRIPTION("Removed, not present, or present only outside the editable tag line")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Missing fields or invalid tag")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found")
RIG_ERROR_RESPONSE(422, ErrorResponse, "Card belongs to a different project")
CardTagMutationResponse delete_cards_card_id_tags(
    std::string card_id,
    CardTagMutationRequest request);

RIG_ROUTE(POST, "/cards/{card_id}/resources")
RIG_SUMMARY("Attach a project resource to a live card (idempotent)")
RIG_RESPONSE_DESCRIPTION("Attachment outcome; repeated requests do not change state")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card or resource not found")
RIG_ERROR_RESPONSE(422, ErrorResponse, "Card or resource belongs to a different project")
ResourceAttachmentResponse post_cards_card_id_resources(
    std::string card_id,
    PostCardsCardIdResourcesRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}/resources")
RIG_SUMMARY("Detach one attachment without deleting the project resource (idempotent)")
RIG_RESPONSE_DESCRIPTION("Attachment outcome; repeated requests do not change state")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card or resource not found")
RIG_ERROR_RESPONSE(422, ErrorResponse, "Card or resource belongs to a different project")
ResourceAttachmentResponse delete_cards_card_id_resources(
    std::string card_id,
    DeleteCardsCardIdResourcesRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}/links")
RIG_SUMMARY("Delete links from a card")
RIG_DESCRIPTION("Provide to_card_id (and optional to_type/kind) to delete matching links. A fully specified selector without target_project_id is rejected if more than one target project matches. Provide target_project_id together with to_card_id, to_type and kind to select one qualified target. If to_card_id is omitted, deletes all outgoing links.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
CardPatchResponse delete_cards_card_id_links(
    std::string card_id,
    std::optional<CardLinkDeleteRequest> request);

RIG_ROUTE(POST, "/cards/{card_id}/restore")
RIG_SUMMARY("Restore a card from trash")
RIG_DESCRIPTION("Restores this card only. It returns to its saved parent when that parent is live and reachable, otherwise to its nearest live, reachable ancestor or the project's top level. Children promoted when it was trashed stay where they are; descendants trashed with it stay in Trash, and reattach to it if restored afterwards.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
CardPatchResponse post_cards_card_id_restore(std::string card_id);

} // namespace holder::api
