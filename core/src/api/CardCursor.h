#pragma once

// The opaque cursors of the contract's card pages.

#include "api/Contract.h"

#include "card/CardRepo.h"

#include <nlohmann/json.hpp>

#include <limits>
#include <string>

namespace holder::contract {

// Cursors are the hex of a JSON object naming the query they continue, so a cursor cannot be
// used with a different project, filter or order. The daemon's card page uses the same format,
// and so does a recovery snapshot's.
inline std::string encode_cursor(const nlohmann::json& value) {
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (const char character : value.dump()) {
    const auto byte = static_cast<unsigned char>(character);
    result += hex[byte >> 4];
    result += hex[byte & 15];
  }
  return result;
}

inline holder::card::CardPageCursor decode_cursor(const std::string& cursor, const nlohmann::json& scope, bool sort_key) {
  if (cursor.size() % 2) throw InvalidArgument("invalid cursor");
  const auto digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    throw InvalidArgument("invalid cursor");
  };
  try {
    std::string text;
    for (std::size_t i = 0; i < cursor.size(); i += 2)
      text += static_cast<char>((digit(cursor[i]) << 4) | digit(cursor[i + 1]));
    const auto decoded = nlohmann::json::parse(text);
    if (decoded.at("version") != 1 || decoded.at("query") != scope)
      throw InvalidArgument("the cursor belongs to a different query");
    const auto id = decoded.at("card_id").get<std::string>();
    const auto& timestamp = decoded.at("updated_at");
    if (id.empty() || !timestamp.is_number_integer() ||
        (timestamp.is_number_unsigned() &&
         timestamp.get<unsigned long long>() >
             static_cast<unsigned long long>(std::numeric_limits<long long>::max())))
      throw InvalidArgument("invalid cursor");
    holder::card::CardPageCursor result{id, timestamp.get<long long>()};
    if (sort_key) {
      const auto& key = decoded.at("sort_key");
      if (!key.is_number()) throw InvalidArgument("invalid cursor");
      result.sort_key = key.get<double>();
    }
    return result;
  } catch (const nlohmann::json::exception&) {
    throw InvalidArgument("invalid cursor");
  }
}

} // namespace holder::contract
