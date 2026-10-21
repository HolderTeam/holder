#include "api/ProviderSettings.h"

#include "ai/AiProviderSettingRepo.h"
#include "api/support/ProviderUtils.h"
#include "api/support/Time.h"
#include "platform/DeviceConfigStore.h"

#include <exception>
#include <utility>

namespace holder::api {

AiProviderSettingsResponse list_provider_settings(holder::platform::Db& db) {
  try {
    AiProviderSettingsData data;
    for (const auto& setting : holder::ai::AiProviderSettingRepo(db).list()) {
      data.providers.push_back({setting.provider, setting.enabled, setting.updated_at});
    }
    return {true, std::move(data)};
  } catch (const std::exception& error) {
    throw ProviderSettingError(error.what());
  }
}

AiProviderSettingResponse set_provider_setting(holder::platform::Db& db,
                                               AiProviderSettingUpdateRequest request) {
  try {
    auto provider = support::normalize_provider_name(request.provider);
    if (provider.empty()) {
      throw ProviderSettingError("provider must be alphanumeric and may include '-', '_' or '.'.");
    }
    const long long updated_at = request.updated_at.value_or(support::now_epoch_seconds());
    holder::ai::AiProviderSettingRepo(db).upsert(provider, request.enabled, updated_at);
    holder::core::persist_device_config(db);
    return {true, {std::move(provider), request.enabled, updated_at}};
  } catch (const std::exception& error) {
    throw ProviderSettingError(error.what());
  }
}

} // namespace holder::api
