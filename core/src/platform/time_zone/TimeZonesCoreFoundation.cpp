// The built-in provider on macOS, whose C++ library lacks the time-zone database: Core Foundation,
// which reads the system's own zone data.
#include "platform/time_zone/TimeZones.h"

#include <CoreFoundation/CoreFoundation.h>

#include <memory>
#include <type_traits>

namespace holder::platform {
namespace {

template <class T> using Owned = std::unique_ptr<std::remove_pointer_t<T>, decltype(&CFRelease)>;

template <class T> Owned<T> own(T value) { return Owned<T>(value, &CFRelease); }

std::string text(CFStringRef value) {
  const auto length =
      CFStringGetMaximumSizeForEncoding(CFStringGetLength(value), kCFStringEncodingUTF8) + 1;
  std::string out(static_cast<std::size_t>(length), '\0');
  if (!CFStringGetCString(value, out.data(), length, kCFStringEncodingUTF8)) return {};
  out.resize(std::char_traits<char>::length(out.c_str()));
  return out;
}

class CoreFoundationTimeZones final : public TimeZoneProvider {
 public:
  std::optional<long long> utc_offset(const std::string& zone, long long utc_seconds) override {
    const auto name = own(
        CFStringCreateWithCString(kCFAllocatorDefault, zone.c_str(), kCFStringEncodingUTF8)
    );
    if (!name) return std::nullopt;
    const auto found = own(CFTimeZoneCreateWithName(kCFAllocatorDefault, name.get(), true));
    if (!found) return std::nullopt;
    // Core Foundation counts from 2001-01-01T00:00:00Z.
    const auto at = static_cast<CFAbsoluteTime>(utc_seconds) - kCFAbsoluteTimeIntervalSince1970;
    return static_cast<long long>(CFTimeZoneGetSecondsFromGMT(found.get(), at));
  }

  std::string current_zone() override {
    CFTimeZoneResetSystem();
    const auto system = own(CFTimeZoneCopySystem());
    return text(CFTimeZoneGetName(system.get()));
  }
};

} // namespace

std::shared_ptr<TimeZoneProvider> built_in_time_zone_provider() {
  return std::make_shared<CoreFoundationTimeZones>();
}

} // namespace holder::platform
