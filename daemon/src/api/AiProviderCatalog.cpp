#include "api/AiProviderCatalog.h"

#include "ai/AiProviderCredentialRepo.h"
#include "ai/AiProviderSettingRepo.h"
#include "api/support/CloudConfig.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace holder::api {
namespace {

std::optional<std::string> unless_empty(const std::string& value) {
  return value.empty() ? std::nullopt : std::optional<std::string>(value);
}

AiProviderCatalogModel catalog_model(const support::CloudModelConfig& model) {
  return {
      model.id,
      model.endpoint,
      unless_empty(model.role),
      unless_empty(model.cost_tier),
      model.default_for_low_budget,
      {model.rpm, model.tpm, model.rpd},
  };
}

} // namespace

AiProviderCatalogResponse get_ai_provider_catalog(holder::platform::Db& db) {
  const auto catalog = support::load_cloudproviders_config();
  if (!catalog) throw AiProviderCatalogMissing{};

  std::unordered_set<std::string> configured;
  for (const auto& credential : holder::ai::AiProviderCredentialRepo(db).list()) {
    configured.insert(credential.provider);
  }
  std::unordered_map<std::string, bool> enabled;
  for (const auto& setting : holder::ai::AiProviderSettingRepo(db).list()) {
    enabled[setting.provider] = setting.enabled;
  }

  AiProviderCatalogResponse response{true, {unless_empty(catalog->default_provider), {}}};
  for (const auto& provider : catalog->providers) {
    const auto setting = enabled.find(provider.id);
    AiProviderCatalogProvider item{
        provider.id,
        provider.display_name.empty() ? provider.id : provider.display_name,
        setting != enabled.end() ? setting->second : provider.enabled,
        configured.contains(provider.id),
        unless_empty(provider.cost_tier),
        unless_empty(provider.setup_url),
        unless_empty(provider.docs_url),
        unless_empty(provider.api_key_label),
        unless_empty(provider.api_key_hint),
        {unless_empty(provider.base_url), unless_empty(provider.kind)},
        {unless_empty(provider.auth_type), unless_empty(provider.key_param),
         unless_empty(provider.header_name), unless_empty(provider.bearer_prefix)},
        {},
    };
    for (const auto& model : provider.models) item.models.push_back(catalog_model(model));
    response.data.providers.push_back(std::move(item));
  }
  return response;
}

} // namespace holder::api
