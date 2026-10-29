#pragma once

#include "api/AiRuntimeTypes.h"
#include "api/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct AiProviderCatalogModelLimits {
  long long rpm;
  long long tpm;
  long long rpd;
};

struct AiProviderCatalogModel {
  std::string id;
  std::string endpoint;
  std::optional<std::string> role;
  std::optional<std::string> cost_tier;
  bool default_for_low_budget;
  AiProviderCatalogModelLimits limits;
};

struct AiProviderCatalogApi {
  std::optional<std::string> base_url;
  std::optional<std::string> kind;
};

struct AiProviderCatalogAuth {
  std::optional<std::string> type;
  std::optional<std::string> key_param;
  std::optional<std::string> header_name;
  std::optional<std::string> bearer_prefix;
};

struct AiProviderCatalogProvider {
  std::string id;
  std::string display_name;
  bool enabled;
  bool configured;
  std::optional<std::string> cost_tier;
  std::optional<std::string> setup_url;
  std::optional<std::string> docs_url;
  std::optional<std::string> api_key_label;
  std::optional<std::string> api_key_hint;
  AiProviderCatalogApi api;
  AiProviderCatalogAuth auth;
  std::vector<AiProviderCatalogModel> models;
};

struct AiProviderCatalogResponseData {
  std::optional<std::string> default_provider;
  std::vector<AiProviderCatalogProvider> providers;
};

struct AiProviderCatalogResponse {
  bool ok;
  AiProviderCatalogResponseData data;
};

struct AiProviderCatalogMissing : std::runtime_error {
  AiProviderCatalogMissing() : std::runtime_error("ai_catalog.yaml models runtime/catalog not found.") {}
};

AiProviderCatalogResponse get_ai_provider_catalog(holder::platform::Db& db);

} // namespace holder::api
