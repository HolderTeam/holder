#pragma once

#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct AiProviderSetting {
  std::string provider;
  bool enabled;
  long long updated_at;
};

struct AiProviderSettingUpdateRequest {
  std::string provider;
  bool enabled;
  std::optional<long long> updated_at;
};

struct AiProviderSettingResponse {
  bool ok;
  AiProviderSetting data;
};

struct AiProviderSettingsData {
  std::vector<AiProviderSetting> providers;
};

struct AiProviderSettingsResponse {
  bool ok;
  AiProviderSettingsData data;
};

struct AiProviderSettingDeleteData {
  std::string provider;
};

struct AiProviderSettingDeleteResponse {
  bool ok;
  AiProviderSettingDeleteData data;
};

AiProviderSettingsResponse list_provider_settings(holder::platform::Db& db);

AiProviderSettingResponse set_provider_setting(holder::platform::Db& db,
                                               AiProviderSettingUpdateRequest request);

AiProviderSettingDeleteResponse delete_provider_setting(holder::platform::Db& db,
                                                         std::string provider);

} // namespace holder::api
