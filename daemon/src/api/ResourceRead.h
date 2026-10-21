#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::model { struct ResourceBundle; }

namespace holder::api {

// A resource as every resource response reports it, with the live cards that link to it.
Resource resource_item(const holder::model::ResourceBundle& bundle,
                       std::vector<ResourceReferencedByCardsItem> referenced_by_cards = {});

// resource_item with the live cards that currently link to the resource.
Resource resource_with_references(holder::platform::Db& db,
                                  const holder::model::ResourceBundle& bundle);

struct RIG_OPEN_RECORD ResourceListResponse {
  std::optional<std::string> card_id RIG_FIELD_DESCRIPTION("Canonical live card ID, present only for card attachment lists.") RIG_OMIT_EMPTY;
  std::optional<int> limit RIG_OMIT_EMPTY;
  std::optional<int> offset RIG_OMIT_EMPTY;
  std::optional<std::optional<long long>> next_offset RIG_FIELD_DESCRIPTION("Present for card lists; null when exhausted, otherwise continue here (the next page may be empty).") RIG_OMIT_EMPTY RIG_NULLABLE;
  bool ok RIG_BOOL_VALUE(true);
  std::vector<Resource> data;
};

struct ResourceListBadRequest : std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

struct ResourceCardNotFound : std::runtime_error {
  ResourceCardNotFound() : std::runtime_error("Live card not found.") {}
};

struct ResourceCardInOtherProject : CodedError {
  ResourceCardInOtherProject()
      : CodedError("cross_project_resource_forbidden", "Card is in a different project.") {}
};

struct ResourceNotFound : std::runtime_error {
  ResourceNotFound() : std::runtime_error("Resource not found.") {}
};

RIG_ROUTE(GET, "/resources")
RIG_SUMMARY("List project resources or attachments to a live card")
RIG_DESCRIPTION("With card_id, returns an authoritative live-card attachment join in updated_at descending, resource_id ascending order. Without card_id, preserves the unpaginated project resource list. Detach preserves project resources; delete removes them globally.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", ResourceListBadRequest)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found", ResourceCardNotFound)
RIG_ERROR_RESPONSE(422, ErrorResponse, "Card is in a different project", ResourceCardInOtherProject)
ResourceListResponse get_resources(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::optional<std::string> card_id RIG_QUERY RIG_PARAMETER_DESCRIPTION("Complete resolved live card ID; must belong to project_id."),
    std::optional<int> limit RIG_QUERY RIG_RANGE(1, 1000) RIG_PARAMETER_DESCRIPTION("Requires card_id; defaults to 100."),
    std::optional<int> offset RIG_QUERY RIG_MINIMUM(0) RIG_PARAMETER_DESCRIPTION("Requires card_id; defaults to 0."));

RIG_ROUTE(GET, "/resources/{resource_id}")
RIG_SUMMARY("Get a resource with its assets and placements")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", ResourceNotFound)
ResourceResponse get_resource(holder::platform::Db& db RIG_CONTEXT, std::string resource_id);

} // namespace holder::api
