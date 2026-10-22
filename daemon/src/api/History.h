#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::privacy { class PrivacyError; }
namespace holder::card { class CardStore; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardHistoryRestoreResponseData {
  std::string card_id;
  std::string restored_from_oid RIG_FIELD_DESCRIPTION("Canonical full commit object ID of the selected historical snapshot.");
  std::string result_oid RIG_FIELD_DESCRIPTION("Canonical full commit object ID created by the restoration.");
  std::string title;
  long long updated_at;
  std::optional<long long> deleted_at RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardHistoryRestoreResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardHistoryRestoreResponseData data;
};

// History failures keep their own codes; each type is one response status.
struct HistoryNotFound : CodedError {
  using CodedError::CodedError;
};
struct HistoryConflict : CodedError {
  using CodedError::CodedError;
};
struct HistoryResponseTooLarge : CodedError {
  HistoryResponseTooLarge()
      : CodedError("history_response_too_large", "History response exceeds the 2 MiB limit.") {}
};
struct HistoryUnavailable : CodedError {
  explicit HistoryUnavailable(const std::string& message)
      : CodedError("history_unavailable", message) {}
};

// Whether a privacy failure reading history means this device lacks the project key (409),
// rather than history being unavailable (503).
bool history_key_unavailable(const holder::privacy::PrivacyError& error);

RIG_ROUTE(GET, "/projects/{project_id}/history")
RIG_SUMMARY("List project activity history")
RIG_DESCRIPTION("Returns bounded, cursor-paginated Git activities for the project. Each activity groups its changed paths by recognised Holder object kind; unknown paths remain visible as unknown.")
RIG_RESPONSE_DESCRIPTION("Project activity history")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid limit, cursor, or object-kind filter", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", HistoryNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "The encrypted project key is unavailable on this device", HistoryConflict)
RIG_ERROR_RESPONSE(413, ErrorResponse, "The bounded History response would exceed 2 MiB", HistoryResponseTooLarge)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Project history is temporarily unavailable", HistoryUnavailable)
ProjectHistoryPageResponse get_project_history(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id,
    int limit RIG_QUERY RIG_RANGE(1, 200) RIG_DEFAULT(50),
    std::optional<std::string> cursor RIG_QUERY RIG_PARAMETER_DESCRIPTION("A full commit OID from next_cursor."),
    std::optional<std::string> kind RIG_QUERY RIG_STRING_VALUES("card", "resource", "location", "ai_data", "project_settings", "unknown") RIG_PARAMETER_DESCRIPTION("Return only activities affecting this Holder object kind."));

RIG_ROUTE(GET, "/projects/{project_id}/history/cards/{card_id}")
RIG_SUMMARY("List saved history for a card")
RIG_DESCRIPTION("Returns commits reachable from the captured project HEAD that changed the card. Adjacent autosaves may be grouped into one entry.")
RIG_RESPONSE_DESCRIPTION("Saved card history")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid card identifier, limit, or cursor", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", HistoryNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "The encrypted project key is unavailable on this device", HistoryConflict)
RIG_ERROR_RESPONSE(413, ErrorResponse, "The bounded History response would exceed 2 MiB", HistoryResponseTooLarge)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Project history is temporarily unavailable", HistoryUnavailable)
CardHistoryPageResponse get_card_history(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id,
    std::string card_id,
    int limit RIG_QUERY RIG_RANGE(1, 200) RIG_DEFAULT(50),
    std::optional<std::string> cursor RIG_QUERY RIG_PARAMETER_DESCRIPTION("A full commit OID from next_cursor."));

RIG_ROUTE(GET, "/projects/{project_id}/history/cards/{card_id}/compare")
RIG_SUMMARY("Compare saved card versions or the change introduced by one revision")
RIG_DESCRIPTION("In since mode, compares the explicitly selected from and to snapshots. In change mode, compares the first parent of to with to; a root commit is compared with a previously nonexistent card. Merge commits use only their first parent, not a combined multi-parent diff.")
RIG_RESPONSE_DESCRIPTION("A line-oriented card comparison")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid or missing comparison parameters", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or revision not found", HistoryNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "A revision reference is ambiguous or the encrypted project key is unavailable", HistoryConflict)
RIG_ERROR_RESPONSE(413, ErrorResponse, "The bounded History response would exceed 2 MiB", HistoryResponseTooLarge)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Project history is temporarily unavailable", HistoryUnavailable)
CardHistoryComparisonResponse compare_card_history(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id,
    std::string card_id,
    std::optional<RevisionReference> from RIG_QUERY RIG_PARAMETER_DESCRIPTION("Required and used in since mode; change mode derives its base from to."),
    RevisionReference to RIG_QUERY RIG_PARAMETER_DESCRIPTION("The ending revision, or the revision whose introduced change is requested."),
    std::string mode RIG_QUERY RIG_STRING_VALUES("since", "change") RIG_DEFAULT("since") RIG_PARAMETER_DESCRIPTION("Since compares two explicit snapshots; change compares to with its first parent."));

RIG_ROUTE(GET, "/projects/{project_id}/history/cards/{card_id}/snapshot")
RIG_SUMMARY("Read a card snapshot at a selected revision")
RIG_RESPONSE_DESCRIPTION("The card snapshot, including an explicit absent state")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Missing revision reference or invalid card identifier", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or revision not found", HistoryNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "A revision reference is ambiguous or the encrypted project key is unavailable", HistoryConflict)
RIG_ERROR_RESPONSE(413, ErrorResponse, "The bounded snapshot response would exceed 2 MiB", HistoryResponseTooLarge)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The historical snapshot is temporarily unavailable", HistoryUnavailable)
CardHistorySnapshotResponse get_card_history_snapshot(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id,
    std::string card_id,
    RevisionReference oid RIG_QUERY);

RIG_ROUTE(POST, "/projects/{project_id}/history/cards/{card_id}/restore")
RIG_SUMMARY("Restore a card to a saved snapshot")
RIG_DESCRIPTION("Restores the complete card snapshot at the selected commit and records the result as a new commit without rewriting history. Lifecycle rules still apply. Restoring a snapshot in which the card was trashed promotes its current live children, as trashing does. Restoring a live snapshot places the card under a reachable parent, as restoring from Trash does.")
RIG_RESPONSE_DESCRIPTION("Card snapshot restored")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project, card, or revision not found", HistoryNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "A revision reference is ambiguous or the encrypted project key is unavailable", HistoryConflict)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The historical snapshot cannot be read or restored", HistoryUnavailable)
CardHistoryRestoreResponse post_projects_project_id_history_cards_card_id_restore(
    holder::platform::Db& db RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string project_id,
    std::string card_id,
    RevisionReference oid RIG_QUERY RIG_PARAMETER_DESCRIPTION("Revision containing the snapshot to restore."));

} // namespace holder::api
