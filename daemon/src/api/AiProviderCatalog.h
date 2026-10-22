#pragma once

#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCatalogModelLimits {
  long long rpm RIG_FIELD_DESCRIPTION("Requests per minute; 0 when unlimited or unknown.");
  long long tpm RIG_FIELD_DESCRIPTION("Tokens per minute; 0 when unlimited or unknown.");
  long long rpd RIG_FIELD_DESCRIPTION("Requests per day; 0 when unlimited or unknown.");
};

struct RIG_OPEN_RECORD AiProviderCatalogModel {
  std::string id;
  std::string endpoint;
  std::optional<std::string> role RIG_NULLABLE;
  std::optional<std::string> cost_tier RIG_NULLABLE;
  bool default_for_low_budget;
  AiProviderCatalogModelLimits limits;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCatalogApi {
  std::optional<std::string> base_url RIG_OMIT_EMPTY;
  std::optional<std::string> kind RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCatalogAuth {
  std::optional<std::string> type RIG_OMIT_EMPTY;
  std::optional<std::string> key_param RIG_OMIT_EMPTY;
  std::optional<std::string> header_name RIG_OMIT_EMPTY;
  std::optional<std::string> bearer_prefix RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiProviderCatalogProvider {
  std::string id;
  std::string display_name;
  bool enabled RIG_FIELD_DESCRIPTION("This device's setting, or the catalog default without one.");
  bool configured RIG_FIELD_DESCRIPTION("Whether this device has an API key for the provider.");
  std::optional<std::string> cost_tier RIG_NULLABLE;
  std::optional<std::string> setup_url RIG_NULLABLE;
  std::optional<std::string> docs_url RIG_NULLABLE;
  std::optional<std::string> api_key_label RIG_NULLABLE;
  std::optional<std::string> api_key_hint RIG_NULLABLE;
  AiProviderCatalogApi api;
  AiProviderCatalogAuth auth;
  std::vector<AiProviderCatalogModel> models;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCatalogResponseData {
  std::optional<std::string> default_provider RIG_NULLABLE;
  std::vector<AiProviderCatalogProvider> providers;
};

struct RIG_OPEN_RECORD AiProviderCatalogResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderCatalogResponseData data;
};

struct AiProviderCatalogMissing : std::runtime_error {
  AiProviderCatalogMissing() : std::runtime_error("ai_catalog.yaml models runtime/catalog not found.") {}
};

RIG_ROUTE(GET, "/ai/providers/catalog")
RIG_SUMMARY("Get provider-agnostic cloud catalog with configured flags")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "The AI catalog could not be found", AiProviderCatalogMissing)
AiProviderCatalogResponse get_ai_provider_catalog(holder::platform::Db& db RIG_CONTEXT);

} // namespace holder::api
