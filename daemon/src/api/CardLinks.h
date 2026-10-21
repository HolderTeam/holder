#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::index { class FtsIndexer; }

namespace holder::api::routes {

struct RIG_OPEN_RECORD CardLink {
  std::string from_card_id;
  std::string to_card_id;
  std::optional<std::string> target_project_id RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("Present for a card target in another project; omitted for a local target. The link grants no access to the target.");
  std::optional<std::string> source_project_id RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("Present on a cross-project backlink; identifies the available project that owns the link.");
  std::string to_type
      RIG_FIELD_DESCRIPTION("Target type (card, ai_message, ai_thread, resource).");
  std::string kind;
  std::optional<std::optional<std::string>> label RIG_OMIT_EMPTY RIG_NULLABLE;
  long long created_at;
};

struct RIG_OPEN_RECORD CardLinkListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<CardLink> data;
};

struct CardLinkSourceNotFound : std::runtime_error {
  CardLinkSourceNotFound() : std::runtime_error("Card not found.") {}
};

RIG_ROUTE(GET, "/cards/{card_id}/links")
RIG_SUMMARY("List outgoing links for a card")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(401, holder::api::ErrorResponse, "Unauthorized")
RIG_ERROR_RESPONSE(404, holder::api::ErrorResponse, "Not found", CardLinkSourceNotFound)
CardLinkListResponse list_card_links(holder::platform::Db& db RIG_CONTEXT,
                                     holder::index::FtsIndexer* fts RIG_CONTEXT,
                                     std::string card_id,
                                     std::optional<int> include_deleted RIG_QUERY
                                         RIG_INT_VALUES(0, 1)
                                         RIG_PARAMETER_DESCRIPTION(
                                             "Include links to deleted local targets. External card links remain listed even when their targets are unavailable or trashed."));

} // namespace holder::api::routes
