#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD AiProviderSetting {
  std::string provider;
  bool enabled;
  long long updated_at;
};

struct RIG_OPEN_RECORD AiProviderSettingUpdateRequest {
  std::string provider;
  bool enabled;
  std::optional<long long> updated_at RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiProviderSettingResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderSetting data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderSettingsData {
  std::vector<AiProviderSetting> providers;
};

struct RIG_OPEN_RECORD AiProviderSettingsResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderSettingsData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderSettingDeleteData {
  std::string provider;
};

struct RIG_OPEN_RECORD AiProviderSettingDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderSettingDeleteData data;
};

RIG_ROUTE(GET, "/ai/providers/settings")
RIG_SUMMARY("List cloud provider enabled settings")
RIG_RESPONSE_DESCRIPTION("OK")
AiProviderSettingsResponse list_provider_settings(holder::platform::Db& db RIG_CONTEXT);

RIG_ROUTE(PUT, "/ai/providers/settings")
RIG_SUMMARY("Set whether a cloud provider is enabled")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request or an invalid provider name", std::invalid_argument)
AiProviderSettingResponse set_provider_setting(holder::platform::Db& db RIG_CONTEXT,
                                               AiProviderSettingUpdateRequest request);

RIG_ROUTE(DELETE, "/ai/providers/settings/{provider}")
RIG_SUMMARY("Remove a cloud provider enabled override")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid provider name", std::invalid_argument)
AiProviderSettingDeleteResponse delete_provider_setting(holder::platform::Db& db RIG_CONTEXT,
                                                         std::string provider);

} // namespace holder::api
