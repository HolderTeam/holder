// Platforms without a built-in provider: Android, whose C++ library lacks the time-zone database.
// The app registers one with holder_register_time_zone_provider.
#include "platform/time_zone/TimeZones.h"

namespace holder::platform {

std::shared_ptr<TimeZoneProvider> built_in_time_zone_provider() { return nullptr; }

} // namespace holder::platform
