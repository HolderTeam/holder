#pragma once

// The link records the AI message routes use. A card's links are the contract's
// (contract/interface/links.py).

#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"
#include "api/support/PrivacyErrors.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::api {

struct CardLinkDeleteRequest {
  std::optional<std::string> to_card_id;
  std::optional<std::string> target_project_id;
  std::optional<std::string> to_type;
  std::optional<std::string> kind;
};

} // namespace holder::api

namespace holder::api::routes {

struct CardLink {
  std::string from_card_id;
  std::string to_card_id;
  std::optional<std::string> target_project_id
;
  std::optional<std::string> source_project_id
;
  std::string to_type
;
  std::string kind;
  std::optional<std::optional<std::string>> label;
  long long created_at;
};

struct CardLinkListResponse {
  bool ok;
  std::vector<CardLink> data;
};

struct CardLinkCreateRequest {
  std::string to_card_id;
  std::optional<std::string> target_project_id
;
  std::optional<std::string> to_type
;
  std::optional<std::string> kind
;
  std::optional<std::optional<std::string>> label;
  std::optional<long long> created_at
;
};

struct CardLinkCreateResponse {
  bool ok;
  CardLink data;
};

} // namespace holder::api::routes
