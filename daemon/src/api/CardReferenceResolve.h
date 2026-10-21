#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD CardReferenceResolveRequest {
  std::string project_id;
  std::string reference RIG_FIELD_DESCRIPTION(
      "Full canonical UUID, eligible UUID prefix, or exact card title.");
  std::string scope RIG_STRING_VALUES("live", "trashed", "either");
};

struct RIG_OPEN_RECORD CardReferenceCandidate {
  std::string card_id RIG_FIELD_DESCRIPTION("Canonical full card UUID.");
  std::string title;
  std::optional<long long> deleted_at RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardReferenceResolveData {
  std::string status RIG_STRING_VALUES("resolved", "ambiguous", "not_found");
  std::optional<std::string> match_kind RIG_OMIT_EMPTY
      RIG_STRING_VALUES("full_id", "id_prefix", "exact_title")
      RIG_FIELD_DESCRIPTION("Present for resolved and ambiguous results.");
  std::optional<CardReferenceCandidate> card RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("Present for a resolved result.");
  std::optional<std::vector<CardReferenceCandidate>> candidates RIG_OMIT_EMPTY
      RIG_FIELD_DESCRIPTION("Present for an ambiguous result and contains two candidates.");
};

struct RIG_OPEN_RECORD CardReferenceResolveResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardReferenceResolveData data;
};

struct CardReferenceInvalidRequest : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct CardReferenceProjectNotFound : std::runtime_error {
  CardReferenceProjectNotFound() : std::runtime_error("Project not found.") {}
};

RIG_ROUTE(POST, "/card-references/resolve")
RIG_SUMMARY("Resolve a card reference within a project and deletion scope")
RIG_RESPONSE_DESCRIPTION("Resolved, ambiguous, or not-found result")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", CardReferenceInvalidRequest)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", CardReferenceProjectNotFound)
RIG_ERROR_RESPONSE(500, ErrorResponse, "Internal server error", std::exception)
CardReferenceResolveResponse resolve_card_reference(holder::platform::Db& db RIG_CONTEXT,
                                                     CardReferenceResolveRequest request);

} // namespace holder::api
