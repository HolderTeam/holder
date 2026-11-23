// Generated from contract/interface (ai, cards, daemon, documents, errors.py, events, git, history,
// links, locations, maintenance, milestones, pagination, projects, recovery, resources, runtime,
// search, trash) by compiler. Do not edit; change the contract, then run make -C contract update.
//
// Complete daemon boundary JSON codecs.
#pragma once
#include "api/HttpError.h"
#include "api/generated/DaemonTypes.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace daemon_json {
using json = nlohmann::json;
template <typename T> struct codec;

// The number of characters in text, or nothing when it is not well-formed UTF-8.
inline std::optional<std::size_t> utf8_length(std::string_view text) {
  std::size_t count = 0;
  for (std::size_t i = 0; i < text.size(); ++count) {
    const unsigned int lead = static_cast<unsigned char>(text[i]);
    std::size_t size = 1;
    std::uint32_t point = lead;
    if (lead >= 0xC2 && lead <= 0xDF) {
      size = 2, point = lead & 0x1FU;
    } else if (lead >= 0xE0 && lead <= 0xEF) {
      size = 3, point = lead & 0x0FU;
    } else if (lead >= 0xF0 && lead <= 0xF4) {
      size = 4, point = lead & 0x07U;
    } else if (lead >= 0x80) {
      return std::nullopt;
    }
    if (text.size() - i < size) return std::nullopt;
    for (std::size_t k = 1; k < size; ++k) {
      const unsigned int next = static_cast<unsigned char>(text[i + k]);
      if ((next & 0xC0U) != 0x80) return std::nullopt;
      point = (point << 6U) | (next & 0x3FU);
    }
    if ((size == 3 && (point < 0x800 || (point >= 0xD800 && point <= 0xDFFF))) ||
        (size == 4 && (point < 0x10000 || point > 0x10FFFF)))
      return std::nullopt;
    i += size;
  }
  return count;
}

// Text the contract accepts: UTF-8 without U+0000, its length in characters within bounds.
inline void validate_text(
    std::string_view value,
    const char* name,
    std::size_t minimum,
    std::size_t maximum
) {
  const auto length = utf8_length(value);
  if (!length) throw std::invalid_argument(std::string(name) + " must be UTF-8");
  if (value.find('\0') != std::string_view::npos)
    throw std::invalid_argument(std::string(name) + " must not contain U+0000");
  if (*length < minimum)
    throw std::invalid_argument(
        std::string(name) + (minimum == 1
                                 ? " must not be empty"
                                 : " must be at least " + std::to_string(minimum) + " characters")
    );
  if (*length > maximum)
    throw std::invalid_argument(
        std::string(name) + " must be at most " + std::to_string(maximum) + " characters"
    );
}

inline std::string check_text(
    std::string value,
    const char* name,
    std::size_t minimum,
    std::size_t maximum
) {
  validate_text(value, name, minimum, maximum);
  return value;
}

// Lists and maps of text: every item, key and value is text the contract accepts, of any length.
inline std::vector<std::string> check_text_items(std::vector<std::string> items, const char* name) {
  for (const auto& text : items)
    validate_text(text, name, 0, std::numeric_limits<std::size_t>::max());
  return items;
}

inline std::map<std::string, std::string> check_text_entries(
    std::map<std::string, std::string> entries,
    const char* name
) {
  const auto key_name = std::string(name) + " key";
  for (const auto& [key, text] : entries) {
    validate_text(key, key_name.c_str(), 0, std::numeric_limits<std::size_t>::max());
    validate_text(text, name, 0, std::numeric_limits<std::size_t>::max());
  }
  return entries;
}

inline std::map<std::string, std::vector<std::string>> check_text_entries(
    std::map<std::string, std::vector<std::string>> entries,
    const char* name
) {
  const auto key_name = std::string(name) + " key";
  for (const auto& [key, values] : entries) {
    validate_text(key, key_name.c_str(), 0, std::numeric_limits<std::size_t>::max());
    for (const auto& text : values)
      validate_text(text, name, 0, std::numeric_limits<std::size_t>::max());
  }
  return entries;
}

template <typename T> T check_range(T value, const char* name, T minimum, T maximum) {
  if (value < minimum || value > maximum)
    throw std::invalid_argument(
        std::string(name) + " must be from " + std::to_string(minimum) + " to " +
        std::to_string(maximum)
    );
  return value;
}

inline double check_finite(double value, const char* name) {
  if (!std::isfinite(value))
    throw std::invalid_argument(std::string(name) + " must be a finite number");
  return value;
}

// A field's value, with the field's name on any complaint about it.
template <typename T> T decode_field(const json& value, const char* name) {
  try {
    return codec<T>::decode(value);
  } catch (const std::invalid_argument& error) {
    throw std::invalid_argument(std::string(name) + " " + error.what());
  }
}

// An integer of type T. JSON has one kind of number, so an integer may be written with a fraction
// or an exponent, such as 691.0 or 1e3, as JSON Schema allows; it must be whole and in range.
template <typename T> T decode_integer(const json& value) {
  constexpr auto minimum = std::numeric_limits<T>::min();
  constexpr auto maximum = std::numeric_limits<T>::max();
  if (value.is_number_unsigned()) {
    if (value.get<std::uint64_t>() > static_cast<std::uint64_t>(maximum))
      throw std::invalid_argument("is out of range");
    return static_cast<T>(value.get<std::uint64_t>());
  }
  if (value.is_number_integer()) {
    const auto number = value.get<std::int64_t>();
    if (number < minimum || number > maximum) throw std::invalid_argument("is out of range");
    return static_cast<T>(number);
  }
  if (!value.is_number_float() || std::trunc(value.get<double>()) != value.get<double>())
    throw std::invalid_argument("must be an integer");
  // -minimum, a power of two, is the first double past maximum.
  const auto number = value.get<double>();
  if (number < static_cast<double>(minimum) || number >= -static_cast<double>(minimum))
    throw std::invalid_argument("is out of range");
  return static_cast<T>(number);
}

template <> struct codec<std::int32_t> {
  static json encode(std::int32_t value) { return value; }
  static std::int32_t decode(const json& value) { return decode_integer<std::int32_t>(value); }
};
template <> struct codec<bool> {
  static json encode(bool value) { return value; }
  static bool decode(const json& value) {
    if (!value.is_boolean()) throw std::invalid_argument("must be a boolean");
    return value.get<bool>();
  }
};
template <> struct codec<long long> {
  static json encode(long long value) { return value; }
  static long long decode(const json& value) { return decode_integer<long long>(value); }
};
template <> struct codec<double> {
  static json encode(double value) { return value; }
  static double decode(const json& value) {
    if (!value.is_number()) throw std::invalid_argument("must be a number");
    const double number = value.get<double>();
    if (!std::isfinite(number)) throw std::invalid_argument("must be a finite number");
    return number;
  }
};
template <> struct codec<std::string> {
  static json encode(const std::string& value) { return value; }
  static std::string decode(const json& value) {
    if (!value.is_string()) throw std::invalid_argument("must be a string");
    return value.get<std::string>();
  }
};
template <typename T> struct codec<std::vector<T>> {
  static json encode(const std::vector<T>& value) {
    json result = json::array();
    for (const auto& item : value)
      result.push_back(codec<T>::encode(item));
    return result;
  }
  static std::vector<T> decode(const json& value) {
    if (!value.is_array()) throw std::invalid_argument("must be an array");
    std::vector<T> result;
    for (const auto& item : value)
      result.push_back(codec<T>::decode(item));
    return result;
  }
};
// A map is a JSON object; JSON names its keys, so each is text and appears once.
template <typename T> struct codec<std::map<std::string, T>> {
  static json encode(const std::map<std::string, T>& value) {
    json result = json::object();
    for (const auto& [key, item] : value)
      result[key] = codec<T>::encode(item);
    return result;
  }
  static std::map<std::string, T> decode(const json& value) {
    if (!value.is_object()) throw std::invalid_argument("must be an object");
    std::map<std::string, T> result;
    for (auto it = value.begin(); it != value.end(); ++it) {
      try {
        result.emplace(it.key(), codec<T>::decode(it.value()));
      } catch (const std::invalid_argument& error) {
        throw std::invalid_argument("has " + it.key() + ", which " + error.what());
      }
    }
    return result;
  }
};

inline std::map<std::string, std::string> decode_text_map(
    const json& value,
    const char* name,
    const char* value_error,
    std::size_t minimum,
    std::size_t maximum
) {
  if (!value.is_object()) throw std::invalid_argument(std::string(name) + " must be an object");
  std::map<std::string, std::string> result;
  for (auto it = value.begin(); it != value.end(); ++it) {
    if (!it.value().is_string()) throw std::invalid_argument(value_error);
    const auto key_name = std::string(name) + " key";
    const auto key =
        check_text(it.key(), key_name.c_str(), 0, std::numeric_limits<std::size_t>::max());
    result.emplace(
        key,
        check_text(it.value().get_ref<const std::string&>(), name, minimum, maximum)
    );
  }
  return result;
}

template <> struct codec<holder::api::Error> {
  static json encode(const holder::api::Error& value);
  static holder::api::Error decode(const json& value);
};
template <> struct codec<holder::api::ErrorResponse> {
  static json encode(const holder::api::ErrorResponse& value);
  static holder::api::ErrorResponse decode(const json& value);
};
template <> struct codec<holder::api::LocationBindingRequest> {
  static json encode(const holder::api::LocationBindingRequest& value);
  static holder::api::LocationBindingRequest decode(const json& value);
};
template <> struct codec<holder::api::LocationAction> {
  static json encode(const holder::api::LocationAction& value);
  static holder::api::LocationAction decode(const json& value);
};
template <> struct codec<holder::api::LocationActionResponse> {
  static json encode(const holder::api::LocationActionResponse& value);
  static holder::api::LocationActionResponse decode(const json& value);
};
inline json codec<holder::api::Error>::encode(const holder::api::Error& value) {
  json result = json::object();
  result["code"] = codec<std::string>::encode(value.code);
  result["message"] = codec<std::string>::encode(value.message);
  return result;
}
inline holder::api::Error codec<holder::api::Error>::decode(const json& value) {
  if (!value.is_object()) throw std::invalid_argument("the request body must be an object");
  for (auto it = value.begin(); it != value.end(); ++it)
    if (it.key() != "code" && it.key() != "message")
      throw std::invalid_argument("there is no field " + it.key());
  holder::api::Error result{};
  if (!value.contains("code")) throw std::invalid_argument("code must be a string");
  if (value.contains("code")) {
    result.code = check_text(
        decode_field<std::string>(value.at("code"), "code"),
        "code",
        0,
        std::numeric_limits<std::size_t>::max()
    );
  }
  if (!value.contains("message")) throw std::invalid_argument("message must be a string");
  if (value.contains("message")) {
    result.message = check_text(
        decode_field<std::string>(value.at("message"), "message"),
        "message",
        0,
        std::numeric_limits<std::size_t>::max()
    );
  }
  return result;
}

inline json codec<holder::api::ErrorResponse>::encode(const holder::api::ErrorResponse& value) {
  json result = json::object();
  if (value.ok != false) throw std::logic_error("invalid error response constant");
  result["ok"] = codec<bool>::encode(value.ok);
  result["error"] = codec<holder::api::Error>::encode(value.error);
  return result;
}
inline holder::api::ErrorResponse codec<holder::api::ErrorResponse>::decode(const json& value) {
  if (!value.is_object()) throw std::invalid_argument("the request body must be an object");
  for (auto it = value.begin(); it != value.end(); ++it)
    if (it.key() != "ok" && it.key() != "error")
      throw std::invalid_argument("there is no field " + it.key());
  holder::api::ErrorResponse result{};
  if (!value.contains("ok")) throw std::invalid_argument("ok must be a boolean");
  if (value.contains("ok")) {
    result.ok = decode_field<bool>(value.at("ok"), "ok");
  }
  if (!value.contains("error")) throw std::invalid_argument("error must be an object");
  if (value.contains("error")) {
    result.error = decode_field<holder::api::Error>(value.at("error"), "error");
  }
  return result;
}

inline json codec<holder::api::LocationBindingRequest>::encode(
    const holder::api::LocationBindingRequest& value
) {
  json result = json::object();
  result["values"] = codec<std::map<std::string, std::string>>::encode(value.values);
  if (value.preview) {
    result["preview"] = codec<std::string>::encode(*value.preview);
  }
  return result;
}
inline holder::api::LocationBindingRequest codec<holder::api::LocationBindingRequest>::decode(
    const json& value
) {
  if (!value.is_object()) throw std::invalid_argument("the request body must be an object");
  for (auto it = value.begin(); it != value.end(); ++it)
    if (it.key() != "values" && it.key() != "preview")
      throw std::invalid_argument("there is no field " + it.key());
  holder::api::LocationBindingRequest result{};
  if (!value.contains("values")) throw std::invalid_argument("values must be an object");
  if (value.contains("values")) {
    result.values = decode_text_map(
        value.at("values"),
        "values",
        "binding values must be strings",
        0,
        std::numeric_limits<std::size_t>::max()
    );
  }
  if (value.contains("preview")) {
    result.preview = check_text(
        decode_field<std::string>(value.at("preview"), "preview"),
        "preview",
        0,
        std::numeric_limits<std::size_t>::max()
    );
  }
  return result;
}

inline json codec<holder::api::LocationAction>::encode(const holder::api::LocationAction& value) {
  json result = json::object();
  result["location_id"] = codec<std::string>::encode(value.location_id);
  if (value.bound) {
    result["bound"] = codec<bool>::encode(*value.bound);
  }
  if (value.binding_preview) {
    result["binding_preview"] = *value.binding_preview
                                    ? codec<std::string>::encode(**value.binding_preview)
                                    : json(nullptr);
  }
  return result;
}
inline holder::api::LocationAction codec<holder::api::LocationAction>::decode(const json& value) {
  if (!value.is_object()) throw std::invalid_argument("the request body must be an object");
  for (auto it = value.begin(); it != value.end(); ++it)
    if (it.key() != "location_id" && it.key() != "bound" && it.key() != "binding_preview")
      throw std::invalid_argument("there is no field " + it.key());
  holder::api::LocationAction result{};
  if (!value.contains("location_id")) throw std::invalid_argument("location_id must be a string");
  if (value.contains("location_id")) {
    result.location_id = check_text(
        decode_field<std::string>(value.at("location_id"), "location_id"),
        "location_id",
        0,
        std::numeric_limits<std::size_t>::max()
    );
  }
  if (value.contains("bound")) {
    result.bound = decode_field<bool>(value.at("bound"), "bound");
  }
  if (value.contains("binding_preview")) {
    if (value.at("binding_preview").is_null()) {
      result.binding_preview.emplace();
    } else {
      result.binding_preview = check_text(
          decode_field<std::string>(value.at("binding_preview"), "binding_preview"),
          "binding_preview",
          0,
          std::numeric_limits<std::size_t>::max()
      );
    }
  }
  return result;
}

inline json codec<holder::api::LocationActionResponse>::encode(
    const holder::api::LocationActionResponse& value
) {
  json result = json::object();
  result["ok"] = codec<bool>::encode(value.ok);
  result["data"] = codec<holder::api::LocationAction>::encode(value.data);
  return result;
}
inline holder::api::LocationActionResponse codec<holder::api::LocationActionResponse>::decode(
    const json& value
) {
  if (!value.is_object()) throw std::invalid_argument("the request body must be an object");
  for (auto it = value.begin(); it != value.end(); ++it)
    if (it.key() != "ok" && it.key() != "data")
      throw std::invalid_argument("there is no field " + it.key());
  holder::api::LocationActionResponse result{};
  if (!value.contains("ok")) throw std::invalid_argument("ok must be a boolean");
  if (value.contains("ok")) {
    result.ok = decode_field<bool>(value.at("ok"), "ok");
  }
  if (!value.contains("data")) throw std::invalid_argument("data must be an object");
  if (value.contains("data")) {
    result.data = decode_field<holder::api::LocationAction>(value.at("data"), "data");
  }
  return result;
}

template <typename T> json encode(const T& value) { return codec<T>::encode(value); }
template <typename T> T decode(const json& value) { return codec<T>::decode(value); }
} // namespace daemon_json
