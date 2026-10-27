#pragma once

// The link records the AI message routes use. A card's links are the contract's
// (contract/interface/links.py).

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/support/PrivacyErrors.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::api {

struct RIG_OPEN_RECORD CardLinkDeleteRequest {
  std::optional<std::string> to_card_id RIG_OMIT_EMPTY;
  std::optional<std::string> target_project_id RIG_FIELD_DESCRIPTION("Selects the target project. Requires to_card_id, to_type and kind; use it to distinguish equal card IDs in different projects.") RIG_OMIT_EMPTY;
  std::optional<std::string> to_type RIG_OMIT_EMPTY;
  std::optional<std::string> kind RIG_OMIT_EMPTY;
};

} // namespace holder::api

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

} // namespace holder::api::routes
