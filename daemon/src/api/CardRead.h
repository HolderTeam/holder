#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::card { class CardStore; }

namespace holder::api {

struct RIG_OPEN_RECORD CardListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<CardListItem> data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardContextDataProject {
  std::string project_id;
  std::string name;
};

struct RIG_OPEN_RECORD CardContextBreadcrumb {
  std::string type RIG_STRING_VALUES("project", "card");
  std::string title;
  std::optional<std::string> project_id RIG_FIELD_DESCRIPTION("Present on the project breadcrumb.") RIG_OMIT_EMPTY;
  std::optional<std::string> card_id RIG_FIELD_DESCRIPTION("Present on card breadcrumbs.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD CardContextCard : CardListItem {
};

struct RIG_OPEN_RECORD CardContextData {
  CardContextDataProject project;
  std::optional<std::string> current_parent_card_id RIG_NULLABLE;
  std::vector<CardContextBreadcrumb> breadcrumbs;
  std::vector<CardContextCard> cards;
};

struct RIG_OPEN_RECORD CardContextResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardContextData data;
};

struct RIG_OPEN_RECORD CardTagOccurrence {
  std::string tag;
  long long byte_start RIG_FIELD_DESCRIPTION("Inclusive byte offset of the leading # in content.");
  long long byte_end RIG_FIELD_DESCRIPTION("Exclusive byte offset after the tag name.");
};

struct RIG_OPEN_RECORD CardData {
  std::string card_id;
  std::string project_id;
  std::string title;
  std::string rel_path;
  double sort_key;
  long long created_at;
  long long updated_at;
  std::optional<std::string> parent_card_id RIG_NULLABLE;
  std::optional<long long> deleted_at RIG_NULLABLE;
  std::string content;
  std::vector<std::string> tags RIG_FIELD_DESCRIPTION("The card's normalized tags.");
  std::vector<CardTagOccurrence> tag_occurrences RIG_FIELD_DESCRIPTION("Each tag written in content, in order.");
};

struct RIG_OPEN_RECORD CardGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardData data;
};

struct CardReadNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

RIG_ROUTE(GET, "/cards")
RIG_SUMMARY("List cards")
RIG_DESCRIPTION("With tag, returns the project's live cards carrying that tag and ignores the other filters.")
RIG_RESPONSE_DESCRIPTION("OK")
CardListResponse get_cards(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::optional<std::string> tag RIG_QUERY RIG_PARAMETER_DESCRIPTION("Exact normalized tag to match, without the leading"),
    std::string view RIG_QUERY RIG_STRING_VALUES("tree", "recent") RIG_DEFAULT("tree") RIG_PARAMETER_DESCRIPTION("Card list mode. tree returns one hierarchy level (roots or a parent's children); recent returns recently updated cards."),
    std::optional<std::string> parent_card_id RIG_QUERY RIG_PARAMETER_DESCRIPTION("Optional when view=tree. Omit for roots; set to return that parent card's children."),
    std::optional<std::string> order RIG_QUERY RIG_STRING_VALUES("tree_default", "updated_desc", "title_asc") RIG_PARAMETER_DESCRIPTION("Card ordering strategy. Defaults are view-aware: tree uses tree_default\n(sort_key asc, updated_at desc, title asc), recent uses updated_desc.\n"),
    bool count RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include count fields (child_count) in returned card items."),
    int limit RIG_QUERY RIG_RANGE(1, 5000) RIG_DEFAULT(200) RIG_PARAMETER_DESCRIPTION("Maximum number of cards when view=recent."),
    std::optional<int> include_deleted RIG_QUERY RIG_INT_VALUES(0, 1) RIG_PARAMETER_DESCRIPTION("Set to 1 to include deleted cards when view=tree."));

RIG_ROUTE(GET, "/cards/context")
RIG_SUMMARY("Read card context and breadcrumb path for a project level")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or parent card not found", CardReadNotFound)
CardContextResponse get_cards_context(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::optional<std::string> parent_card_id RIG_QUERY RIG_PARAMETER_DESCRIPTION("Optional parent card id. Omit for project root level."),
    std::string order RIG_QUERY RIG_STRING_VALUES("tree_default", "updated_desc", "title_asc") RIG_DEFAULT("tree_default") RIG_PARAMETER_DESCRIPTION("Ordering for the returned level cards."),
    bool count RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include count fields (child_count) in context cards."));

RIG_ROUTE(GET, "/cards/{card_id}")
RIG_SUMMARY("Fetch a card")
RIG_DESCRIPTION("A card in the trash is not found.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", CardReadNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
CardGetResponse get_card(holder::platform::Db& db RIG_CONTEXT,
                         holder::card::CardStore* card_store RIG_CONTEXT,
                         std::string card_id);

} // namespace holder::api
