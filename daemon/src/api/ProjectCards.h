#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"
#include "api/support/PrivacyErrors.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::card { class CardStore; }

namespace holder::api {

struct RIG_OPEN_RECORD CardPageItem : CardListItem {
  std::optional<std::string> content RIG_FIELD_DESCRIPTION("The card's body. Present only when include_content is true.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD CardPage {
  std::vector<CardPageItem> items;
  std::optional<std::string> next_cursor RIG_FIELD_DESCRIPTION("Continue with this cursor, or null when there are no more results.") RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardPageResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardPage data;
};

struct RIG_VALUE_TYPE RIG_INLINE_RECORD ParentRoots {
  std::string value RIG_STRING_VALUES("roots");
};

struct RIG_VALUE_TYPE RIG_INLINE_RECORD ParentUuid {
  std::string value RIG_PATTERN("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[47][0-9a-fA-F]{3}-[89aAbB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}$") RIG_FORMAT("uuid");
};

struct RIG_OPEN_RECORD CardBreadcrumb {
  std::string kind RIG_STRING_VALUES("project", "card");
  std::string id;
  std::string title;
};

struct RIG_OPEN_RECORD CardBreadcrumbsResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<CardBreadcrumb> data RIG_FIELD_DESCRIPTION("Project first, then live ancestor cards from root to the requested card.");
};

struct ProjectCardsNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct CardHierarchyInvalid : CodedError {
  explicit CardHierarchyInvalid(const std::string& message)
      : CodedError("hierarchy_invalid", message) {}
};


RIG_ROUTE(GET, "/projects/{project_id}/cards")
RIG_SUMMARY("Page through a project's live cards")
RIG_DESCRIPTION("Filters combine. Each page reads current data; iteration is not a snapshot.")
RIG_RESPONSE_DESCRIPTION("A page of matching cards")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid parameter or cursor, including a cursor from a different query, or a privacy error reading content", support::PrivacyRejected, std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", ProjectCardsNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "Content pages unavailable because the daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to read an encrypted project's content is unavailable", support::KeyringUnavailable)
CardPageResponse get_projects_project_id_cards(
    holder::platform::Db& db RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string project_id,
    std::optional<std::string> tag RIG_QUERY RIG_MIN_LENGTH(1) RIG_PARAMETER_DESCRIPTION("Exact tag match, case-insensitive, without the leading"),
    std::optional<std::variant<ParentRoots, ParentUuid>> parent RIG_QUERY RIG_PARAMETER_DESCRIPTION("Omit for all hierarchy levels; roots selects root cards; a card ID selects its immediate children."),
    std::string order RIG_QUERY RIG_STRING_VALUES("card_id_asc", "updated_desc", "sort_key_asc") RIG_DEFAULT("card_id_asc") RIG_PARAMETER_DESCRIPTION("updated_desc breaks ties by descending card ID. sort_key_asc follows sibling order and breaks ties by ascending card ID; use it with parent=roots or a parent card ID."),
    int limit RIG_QUERY RIG_RANGE(1, 5000) RIG_DEFAULT(200),
    std::optional<std::string> cursor RIG_QUERY RIG_MIN_LENGTH(1) RIG_MAX_LENGTH(8192) RIG_PARAMETER_DESCRIPTION("Continuation from next_cursor. Keep the project, filters and ordering unchanged."),
    bool include_content RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include each card's body. The page is selected and every body read under one project lock, and the whole page fails if a body cannot be read (for example a locked encrypted project). limit is at most 1000 when true. The cursor is the same with or without content."),
    bool include_child_count RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include each returned live card's immediate live child count. The cursor is the same with or without counts."));

RIG_ROUTE(GET, "/projects/{project_id}/cards/{card_id}/breadcrumbs")
RIG_SUMMARY("Read a live card's project-to-card breadcrumb trail")
RIG_DESCRIPTION("Traverses only the requested card's live ancestors. A missing or trashed card returns 404; a broken or cyclic ancestor chain returns 409 rather than a partial trail.")
RIG_RESPONSE_DESCRIPTION("Project and live card path")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or card not found", ProjectCardsNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "Card's ancestor chain is broken or cyclic", CardHierarchyInvalid)
CardBreadcrumbsResponse get_projects_project_id_cards_card_id_breadcrumbs(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id,
    std::string card_id);

} // namespace holder::api
