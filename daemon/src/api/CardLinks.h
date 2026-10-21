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

struct RIG_OPEN_RECORD CardLinkCreateRequest {
  std::string to_card_id;
  std::optional<std::string> target_project_id RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("Optional for card targets. A named external target project need not be loaded locally.");
  std::optional<std::string> to_type RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("Optional; defaults to \"card\".");
  std::optional<std::string> kind RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("Optional; defaults to \"ref\".");
  std::optional<std::optional<std::string>> label RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> created_at RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.");
};

struct RIG_OPEN_RECORD CardLinkCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardLink data;
};

struct RIG_OPEN_RECORD CardBacklinkListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<CardLink> data;
  std::string scope RIG_STRING_VALUES("local", "external", "all_available");
  std::string coverage RIG_FIELD_DESCRIPTION("External backlinks cover only source projects available to this daemon request, never all possible projects.") RIG_STRING_VALUES("target_project", "available_projects");
};

struct CardLinkSourceNotFound : std::runtime_error {
  CardLinkSourceNotFound() : std::runtime_error("Card not found.") {}
};

RIG_ROUTE(GET, "/cards/{card_id}/links")
RIG_SUMMARY("List outgoing links for a card")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, holder::api::ErrorResponse, "Not found", CardLinkSourceNotFound)
CardLinkListResponse list_card_links(holder::platform::Db& db RIG_CONTEXT,
                                     holder::index::FtsIndexer* fts RIG_CONTEXT,
                                     std::string card_id,
                                     std::optional<int> include_deleted RIG_QUERY
                                         RIG_INT_VALUES(0, 1)
                                         RIG_PARAMETER_DESCRIPTION(
                                             "Include links to deleted local targets. External card links remain listed even when their targets are unavailable or trashed."));

RIG_ROUTE(GET, "/cards/{card_id}/backlinks")
RIG_SUMMARY("List backlinks to a card from selected available projects")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, holder::api::ErrorResponse, "Not found", CardLinkSourceNotFound)
CardBacklinkListResponse list_card_backlinks(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string card_id,
    std::optional<int> include_deleted RIG_QUERY RIG_INT_VALUES(0, 1) RIG_PARAMETER_DESCRIPTION("Include backlinks from deleted sources."),
    std::string scope RIG_QUERY RIG_STRING_VALUES("local", "external", "all_available") RIG_DEFAULT("local") RIG_PARAMETER_DESCRIPTION("Local sources only (default), external available projects only, or both. External results are best effort and exclude unavailable projects."));

// routes::handle_card_routes still writes this response.
RIG_ROUTE(POST, "/cards/{card_id}/links")
RIG_SUMMARY("Create or update a link from a card")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, holder::api::ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, holder::api::ErrorResponse, "Not found")
CardLinkCreateResponse create_card_link(std::string card_id, CardLinkCreateRequest request);

} // namespace holder::api::routes
