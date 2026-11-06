#pragma once

#include "api/support/LocalModelRouting.h"

#include <cctype>
#include <string>
#include <vector>

namespace holder::api::support {

inline std::string normalize_provider_name(const std::string& raw) {
  std::string key = lowercase_ascii(trim_ascii(raw));
  if (key.empty()) return {};
  for (const char ch : key) {
    const unsigned char c = static_cast<unsigned char>(ch);
    if (std::isalnum(c) || ch == '-' || ch == '_' || ch == '.') continue;
    return {};
  }
  return key;
}

inline std::string mask_api_key(const std::string& api_key) {
  if (api_key.empty()) return {};
  std::vector<std::size_t> boundaries;
  boundaries.reserve(api_key.size() + 1);
  for (std::size_t i = 0; i < api_key.size();) {
    boundaries.push_back(i);
    const auto lead = static_cast<unsigned char>(api_key[i]);
    const std::size_t width = lead < 0x80 ? 1 : lead >= 0xc2 && lead <= 0xdf ? 2
        : lead >= 0xe0 && lead <= 0xef ? 3 : lead >= 0xf0 && lead <= 0xf4 ? 4 : 0;
    if (!width || i + width > api_key.size()) return "****";
    for (std::size_t j = 1; j < width; ++j) {
      if ((static_cast<unsigned char>(api_key[i + j]) & 0xc0) != 0x80) return "****";
    }
    i += width;
  }
  if (boundaries.size() <= 8) return "****";
  return api_key.substr(0, boundaries[4]) + "..." + api_key.substr(boundaries[boundaries.size() - 2]);
}

} // namespace holder::api::support
