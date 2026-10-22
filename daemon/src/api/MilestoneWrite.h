#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"
#include "api/support/PrivacyErrors.h"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::card { class CardStore; }

namespace holder::api {

struct RIG_OPEN_RECORD MilestoneWithDatesResponse {
  bool ok RIG_BOOL_VALUE(true);
  MilestoneWithDates data;
};

struct MilestoneWithDatesCreateRequest {
  std::vector<MilestoneDateValue> dates RIG_FIELD_DESCRIPTION("May be empty for an undated milestone.");
  std::optional<std::optional<std::string>> kind RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> description RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_DESCRIPTION("Partial update. Supplying dates replaces the entire ordered list; an empty array clears all dates. Omitted fields remain unchanged.") RIG_MIN_PROPERTIES(1) MilestoneWithDatesUpdateRequest {
  std::optional<std::vector<MilestoneDateValue>> dates RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> kind RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> description RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD MilestoneRemoveResult {
  std::string card_id;
  std::string milestone_id RIG_FIELD_DESCRIPTION("Canonical milestone identifier supplied in the request.") RIG_FORMAT("uuid");
  bool removed RIG_FIELD_DESCRIPTION("True when the milestone existed and was removed; false when it was already absent.");
};

struct RIG_OPEN_RECORD MilestoneRemoveResponse {
  bool ok RIG_BOOL_VALUE(true);
  MilestoneRemoveResult data;
};

struct RIG_OPEN_RECORD MilestoneResponse {
  bool ok RIG_BOOL_VALUE(true);
  Milestone data;
};

struct RIG_OPEN_RECORD MilestoneCreateRequest {
  long long start_at RIG_FIELD_DESCRIPTION("Unix epoch seconds. For all_day=true, encode the local calendar date at local midnight.");
  std::optional<std::optional<long long>> end_at RIG_FIELD_DESCRIPTION("Inclusive milestone end. For all_day=true, encode the inclusive ending local date at local midnight.") RIG_OMIT_EMPTY RIG_NULLABLE;
  bool all_day RIG_FIELD_DESCRIPTION("Marks start_at and end_at as local calendar dates rather than timed instants.") RIG_DEFAULT(false);
  std::optional<std::optional<std::string>> kind RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> description RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_DESCRIPTION("Partial milestone update. Omitted properties remain unchanged; null clears a nullable property.") RIG_MIN_PROPERTIES(1) MilestoneUpdateRequest {
  std::optional<long long> start_at RIG_FIELD_DESCRIPTION("Unix epoch seconds. When changing an all-day milestone, encode the local calendar date at local midnight.") RIG_OMIT_EMPTY;
  std::optional<std::optional<long long>> end_at RIG_FIELD_DESCRIPTION("Inclusive milestone end, or null to clear it. The resulting end must not precede start_at.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<bool> all_day RIG_FIELD_DESCRIPTION("Whether start_at and end_at carry local calendar-date semantics rather than timed-instant semantics.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> kind RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> description RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct MilestoneWriteNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

RIG_ROUTE(POST, "/cards/{card_id}/milestones-with-dates")
RIG_SUMMARY("Add a milestone with labelled dates to a live card")
RIG_RESPONSE_DESCRIPTION("Created milestone")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, invalid milestone dates, or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found", MilestoneWriteNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
MilestoneWithDatesResponse post_cards_card_id_milestones_with_dates(
    holder::platform::Db& db RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
    std::string card_id,
    MilestoneWithDatesCreateRequest request);

RIG_ROUTE(PATCH, "/cards/{card_id}/milestones-with-dates/{milestone_id}")
RIG_SUMMARY("Update a milestone's labelled dates and metadata")
RIG_DESCRIPTION("Supplying dates replaces the complete ordered array. Milestone identity and creation time remain unchanged.")
RIG_RESPONSE_DESCRIPTION("Updated milestone")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed or empty update, invalid milestone dates, or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card or milestone not found", MilestoneWriteNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
MilestoneWithDatesResponse patch_cards_card_id_milestones_with_dates_milestone_id(
    holder::platform::Db& db RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string card_id,
    std::string milestone_id RIG_FORMAT("uuid"),
    MilestoneWithDatesUpdateRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}/milestones-with-dates/{milestone_id}")
RIG_SUMMARY("Remove a milestone from a live card")
RIG_DESCRIPTION("Equivalent to deleting the same milestone through the scheduled timestamp route. Removing a milestone that is already absent succeeds with removed false.")
RIG_RESPONSE_DESCRIPTION("Removal result")
RIG_ERROR_RESPONSE(400, ErrorResponse, "A privacy error writing an encrypted card", support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found", MilestoneWriteNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
MilestoneRemoveResponse delete_cards_card_id_milestones_with_dates_milestone_id(
    holder::platform::Db& db RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string card_id,
    std::string milestone_id RIG_FORMAT("uuid"));

RIG_ROUTE(POST, "/cards/{card_id}/milestones")
RIG_SUMMARY("Add a milestone to a card")
RIG_DESCRIPTION("Creates a scheduled timestamp milestone. Milestones carry optional kind and description fields and have no independent title.")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, end_at before start_at, or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found", MilestoneWriteNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
MilestoneResponse post_cards_card_id_milestones(
    holder::platform::Db& db RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
    std::string card_id,
    MilestoneCreateRequest request);

RIG_ROUTE(PATCH, "/cards/{card_id}/milestones/{milestone_id}")
RIG_SUMMARY("Partially update a milestone on a live card")
RIG_DESCRIPTION("Updates only supplied fields while preserving milestone identity and creation metadata.")
RIG_RESPONSE_DESCRIPTION("Updated milestone")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed or empty update, an end before the start, a milestone without the timestamp view (use the labelled dates route), or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card or milestone not found", MilestoneWriteNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
MilestoneResponse patch_cards_card_id_milestones_milestone_id(
    holder::platform::Db& db RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string card_id,
    std::string milestone_id RIG_FORMAT("uuid") RIG_PARAMETER_DESCRIPTION("Canonical milestone identifier; it is not a card reference and is never abbreviated."),
    MilestoneUpdateRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}/milestones/{milestone_id}")
RIG_SUMMARY("Remove a milestone from a card")
RIG_DESCRIPTION("Removing a milestone that is already absent succeeds with removed false.")
RIG_RESPONSE_DESCRIPTION("Removal result")
RIG_ERROR_RESPONSE(400, ErrorResponse, "A privacy error writing an encrypted card", support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found", MilestoneWriteNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
MilestoneRemoveResponse delete_cards_card_id_milestones_milestone_id(
    holder::platform::Db& db RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string card_id,
    std::string milestone_id);

} // namespace holder::api
