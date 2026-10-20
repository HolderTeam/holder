// Generated from RIG IR; do not edit.
#pragma once
#include "api/Bye.h"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace rig_json {
using json = nlohmann::json;
template <typename T> struct codec;

template <> struct codec<int> {
    static json encode(int value) { return value; }
    static int decode(const json& value) {
        if (!value.is_number_integer()) throw std::invalid_argument("expected int");
        if (value.is_number_unsigned()) {
            if (value.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
                throw std::invalid_argument("int out of range");
        } else {
            const auto number = value.get<std::int64_t>();
            if (number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max())
                throw std::invalid_argument("int out of range");
        }
        return value.get<int>();
    }
};
template <> struct codec<bool> {
    static json encode(bool value) { return value; }
    static bool decode(const json& value) {
        if (!value.is_boolean()) throw std::invalid_argument("expected bool");
        return value.get<bool>();
    }
};
template <> struct codec<long long> {
    static json encode(long long value) { return value; }
    static long long decode(const json& value) {
        if (!value.is_number_integer()) throw std::invalid_argument("expected int64");
        if (value.is_number_unsigned() &&
            value.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<long long>::max()))
            throw std::invalid_argument("int64 out of range");
        return value.get<long long>();
    }
};
template <> struct codec<double> {
    static json encode(double value) { return value; }
    static double decode(const json& value) {
        if (!value.is_number()) throw std::invalid_argument("expected double");
        return value.get<double>();
    }
};
template <> struct codec<std::string> {
    static json encode(const std::string& value) { return value; }
    static std::string decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("expected string");
        return value.get<std::string>();
    }
};
template <typename T> struct codec<std::vector<T>> {
    static json encode(const std::vector<T>& value) {
        json result = json::array();
        for (const auto& item : value) result.push_back(codec<T>::encode(item));
        return result;
    }
    static std::vector<T> decode(const json& value) {
        if (!value.is_array()) throw std::invalid_argument("expected array");
        std::vector<T> result;
        for (const auto& item : value) result.push_back(codec<T>::decode(item));
        return result;
    }
};
template <typename T> struct codec<std::optional<T>> {
    static json encode(const std::optional<T>& value) {
        return value ? codec<T>::encode(*value) : json(nullptr);
    }
    static std::optional<T> decode(const json& value) {
        return value.is_null() ? std::nullopt : std::optional<T>(codec<T>::decode(value));
    }
};

template <> struct codec<holder::api::Error> {
    static json encode(const holder::api::Error& value);
    static holder::api::Error decode(const json& value);
};
template <> struct codec<holder::api::ErrorResponse> {
    static json encode(const holder::api::ErrorResponse& value);
    static holder::api::ErrorResponse decode(const json& value);
};
template <> struct codec<holder::api::ByeResponse> {
    static json encode(const holder::api::ByeResponse& value);
    static holder::api::ByeResponse decode(const json& value);
};

inline json codec<holder::api::Error>::encode(const holder::api::Error& value) {
    json result = json::object();
    result["code"] = codec<std::string>::encode(value.code);
    result["message"] = codec<std::string>::encode(value.message);
    return result;
}
inline holder::api::Error codec<holder::api::Error>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("code")
        || !value.contains("message")
) throw std::invalid_argument("expected Error");
    return holder::api::Error{
        codec<std::string>::decode(value.at("code")),
        codec<std::string>::decode(value.at("message")),
    };
}

inline json codec<holder::api::ErrorResponse>::encode(const holder::api::ErrorResponse& value) {
    json result = json::object();
    if (value.ok != false) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["error"] = codec<holder::api::Error>::encode(value.error);
    return result;
}
inline holder::api::ErrorResponse codec<holder::api::ErrorResponse>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("ok")
        || !value.contains("error")
) throw std::invalid_argument("expected ErrorResponse");
    if (value.at("ok") != false) throw std::invalid_argument("invalid ok");
    return holder::api::ErrorResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::Error>::decode(value.at("error")),
    };
}

inline json codec<holder::api::ByeResponse>::encode(const holder::api::ByeResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    return result;
}
inline holder::api::ByeResponse codec<holder::api::ByeResponse>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("ok")
) throw std::invalid_argument("expected ByeResponse");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ByeResponse{
        codec<bool>::decode(value.at("ok")),
    };
}

template <typename T> json encode(const T& value) { return codec<T>::encode(value); }
template <typename T> T decode(const json& value) { return codec<T>::decode(value); }
} // namespace rig_json
