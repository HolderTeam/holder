#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::index { class FtsIndexer; }
namespace holder::card { class CardStore; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD TrashEmptyResult {
  std::string project_id;
};

struct RIG_OPEN_RECORD TrashEmptyResponse {
  bool ok RIG_BOOL_VALUE(true);
  TrashEmptyResult data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD TrashDeleteResult {
  std::optional<std::string> card_id RIG_FIELD_DESCRIPTION("Present when a card was removed.") RIG_OMIT_EMPTY;
  std::optional<std::string> message_id RIG_FIELD_DESCRIPTION("Present when an AI message was removed.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD TrashDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  TrashDeleteResult data;
};

struct TrashItemNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct TrashItemNotTrashed : CodedError {
  explicit TrashItemNotTrashed(const std::string& message) : CodedError("not_trashed", message) {}
};

RIG_ROUTE(DELETE, "/trash")
RIG_SUMMARY("Empty trash by project")
RIG_DESCRIPTION("Permanently removes the project's trashed cards, AI messages, or both (the default). A project with nothing in Trash, or one that is not registered, succeeds without changes.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Missing project_id or unknown type", std::invalid_argument)
RIG_ERROR_RESPONSE(501, ErrorResponse, "Removing cards needs the card store, which the daemon does not have", CardStoreUnavailable)
TrashEmptyResponse delete_trash(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::string type RIG_QUERY RIG_STRING_VALUES("card", "ai_message", "all") RIG_DEFAULT("all"));

RIG_ROUTE(DELETE, "/trash/{type}/{id}")
RIG_SUMMARY("Permanently delete a trashed item (legacy global-ID route)")
RIG_DESCRIPTION("The item must still be in Trash. Use the project-scoped route for new clients.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Item not found", TrashItemNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "Item is not in Trash (not_trashed)", TrashItemNotTrashed)
RIG_ERROR_RESPONSE(501, ErrorResponse, "Card store unavailable", CardStoreUnavailable)
TrashDeleteResponse delete_trash_type_id(holder::platform::Db& db RIG_CONTEXT,
                                         holder::index::FtsIndexer* fts RIG_CONTEXT,
                                         holder::card::CardStore* card_store RIG_CONTEXT,
                                         std::string type RIG_STRING_VALUES("card", "ai_message"),
                                         std::string id);

RIG_ROUTE(DELETE, "/projects/{project_id}/trash/{type}/{id}")
RIG_SUMMARY("Permanently remove a trashed item from one project")
RIG_DESCRIPTION("The item must belong to the named project and still be in Trash. For cards, older Git revisions remain available; this operation does not erase repository history.")
RIG_RESPONSE_DESCRIPTION("Trashed item removed")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Item not found in the named project", TrashItemNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "Item is not in Trash (not_trashed)", TrashItemNotTrashed)
RIG_ERROR_RESPONSE(501, ErrorResponse, "Card store unavailable", CardStoreUnavailable)
TrashDeleteResponse delete_projects_project_id_trash_type_id(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string project_id,
    std::string type RIG_STRING_VALUES("card", "ai_message"),
    std::string id);

} // namespace holder::api
