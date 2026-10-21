// Generated from RIG IR; do not edit.
#pragma once
#include "api/RigRuntime.h"
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
template <> struct codec<holder::api::CardReferenceResolveRequest> {
    static json encode(const holder::api::CardReferenceResolveRequest& value);
    static holder::api::CardReferenceResolveRequest decode(const json& value);
};
template <> struct codec<holder::api::CardReferenceCandidate> {
    static json encode(const holder::api::CardReferenceCandidate& value);
    static holder::api::CardReferenceCandidate decode(const json& value);
};
template <> struct codec<holder::api::CardReferenceResolveData> {
    static json encode(const holder::api::CardReferenceResolveData& value);
    static holder::api::CardReferenceResolveData decode(const json& value);
};
template <> struct codec<holder::api::CardReferenceResolveResponse> {
    static json encode(const holder::api::CardReferenceResolveResponse& value);
    static holder::api::CardReferenceResolveResponse decode(const json& value);
};
template <> struct codec<holder::api::ProjectTag> {
    static json encode(const holder::api::ProjectTag& value);
    static holder::api::ProjectTag decode(const json& value);
};
template <> struct codec<holder::api::ProjectTagListResponse> {
    static json encode(const holder::api::ProjectTagListResponse& value);
    static holder::api::ProjectTagListResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSetting> {
    static json encode(const holder::api::AiProviderSetting& value);
    static holder::api::AiProviderSetting decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingUpdateRequest> {
    static json encode(const holder::api::AiProviderSettingUpdateRequest& value);
    static holder::api::AiProviderSettingUpdateRequest decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingResponse> {
    static json encode(const holder::api::AiProviderSettingResponse& value);
    static holder::api::AiProviderSettingResponse decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingsData> {
    static json encode(const holder::api::AiProviderSettingsData& value);
    static holder::api::AiProviderSettingsData decode(const json& value);
};
template <> struct codec<holder::api::AiProviderSettingsResponse> {
    static json encode(const holder::api::AiProviderSettingsResponse& value);
    static holder::api::AiProviderSettingsResponse decode(const json& value);
};
template <> struct codec<holder::api::RebuildRequest> {
    static json encode(const holder::api::RebuildRequest& value);
    static holder::api::RebuildRequest decode(const json& value);
};
template <> struct codec<holder::api::RebuildData> {
    static json encode(const holder::api::RebuildData& value);
    static holder::api::RebuildData decode(const json& value);
};
template <> struct codec<holder::api::RebuildResponse> {
    static json encode(const holder::api::RebuildResponse& value);
    static holder::api::RebuildResponse decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullProgress> {
    static json encode(const holder::api::RunnerPullProgress& value);
    static holder::api::RunnerPullProgress decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullJob> {
    static json encode(const holder::api::RunnerPullJob& value);
    static holder::api::RunnerPullJob decode(const json& value);
};
template <> struct codec<holder::api::RunnerPullStatusResponse> {
    static json encode(const holder::api::RunnerPullStatusResponse& value);
    static holder::api::RunnerPullStatusResponse decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLink> {
    static json encode(const holder::api::routes::CardLink& value);
    static holder::api::routes::CardLink decode(const json& value);
};
template <> struct codec<holder::api::routes::CardLinkListResponse> {
    static json encode(const holder::api::routes::CardLinkListResponse& value);
    static holder::api::routes::CardLinkListResponse decode(const json& value);
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

inline json codec<holder::api::CardReferenceResolveRequest>::encode(const holder::api::CardReferenceResolveRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["reference"] = codec<std::string>::encode(value.reference);
    if (value.scope != "live" && value.scope != "trashed" && value.scope != "either") throw std::invalid_argument("invalid scope");
    result["scope"] = codec<std::string>::encode(value.scope);
    return result;
}
inline holder::api::CardReferenceResolveRequest codec<holder::api::CardReferenceResolveRequest>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("project_id")
        || !value.contains("reference")
        || !value.contains("scope")
) throw std::invalid_argument("expected CardReferenceResolveRequest");
    if (value.at("scope") != "live" && value.at("scope") != "trashed" && value.at("scope") != "either") throw std::invalid_argument("invalid scope");
    return holder::api::CardReferenceResolveRequest{
        codec<std::string>::decode(value.at("project_id")),
        codec<std::string>::decode(value.at("reference")),
        codec<std::string>::decode(value.at("scope")),
    };
}

inline json codec<holder::api::CardReferenceCandidate>::encode(const holder::api::CardReferenceCandidate& value) {
    json result = json::object();
    result["card_id"] = codec<std::string>::encode(value.card_id);
    result["title"] = codec<std::string>::encode(value.title);
    result["deleted_at"] = codec<std::optional<long long>>::encode(value.deleted_at);
    return result;
}
inline holder::api::CardReferenceCandidate codec<holder::api::CardReferenceCandidate>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("card_id")
        || !value.contains("title")
        || !value.contains("deleted_at")
) throw std::invalid_argument("expected CardReferenceCandidate");
    return holder::api::CardReferenceCandidate{
        codec<std::string>::decode(value.at("card_id")),
        codec<std::string>::decode(value.at("title")),
        codec<std::optional<long long>>::decode(value.at("deleted_at")),
    };
}

inline json codec<holder::api::CardReferenceResolveData>::encode(const holder::api::CardReferenceResolveData& value) {
    json result = json::object();
    if (value.status != "resolved" && value.status != "ambiguous" && value.status != "not_found") throw std::invalid_argument("invalid status");
    result["status"] = codec<std::string>::encode(value.status);
    if (value.match_kind && *value.match_kind != "full_id" && *value.match_kind != "id_prefix" && *value.match_kind != "exact_title") throw std::invalid_argument("invalid match_kind");
    if (value.match_kind) {
        result["match_kind"] = codec<std::optional<std::string>>::encode(value.match_kind);
    }
    if (value.card) {
        result["card"] = codec<std::optional<holder::api::CardReferenceCandidate>>::encode(value.card);
    }
    if (value.candidates) {
        result["candidates"] = codec<std::optional<std::vector<holder::api::CardReferenceCandidate>>>::encode(value.candidates);
    }
    return result;
}
inline holder::api::CardReferenceResolveData codec<holder::api::CardReferenceResolveData>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("status")
) throw std::invalid_argument("expected CardReferenceResolveData");
    if (value.at("status") != "resolved" && value.at("status") != "ambiguous" && value.at("status") != "not_found") throw std::invalid_argument("invalid status");
    if (value.contains("match_kind") && !value.at("match_kind").is_null() && value.at("match_kind") != "full_id" && value.at("match_kind") != "id_prefix" && value.at("match_kind") != "exact_title") throw std::invalid_argument("invalid match_kind");
    return holder::api::CardReferenceResolveData{
        codec<std::string>::decode(value.at("status")),
        value.contains("match_kind") ? codec<std::optional<std::string>>::decode(value.at("match_kind")) : std::nullopt,
        value.contains("card") ? codec<std::optional<holder::api::CardReferenceCandidate>>::decode(value.at("card")) : std::nullopt,
        value.contains("candidates") ? codec<std::optional<std::vector<holder::api::CardReferenceCandidate>>>::decode(value.at("candidates")) : std::nullopt,
    };
}

inline json codec<holder::api::CardReferenceResolveResponse>::encode(const holder::api::CardReferenceResolveResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::CardReferenceResolveData>::encode(value.data);
    return result;
}
inline holder::api::CardReferenceResolveResponse codec<holder::api::CardReferenceResolveResponse>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("ok")
        || !value.contains("data")
) throw std::invalid_argument("expected CardReferenceResolveResponse");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::CardReferenceResolveResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::CardReferenceResolveData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::ProjectTag>::encode(const holder::api::ProjectTag& value) {
    json result = json::object();
    result["tag"] = codec<std::string>::encode(value.tag);
    result["card_count"] = codec<int>::encode(value.card_count);
    return result;
}
inline holder::api::ProjectTag codec<holder::api::ProjectTag>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("tag")
        || !value.contains("card_count")
) throw std::invalid_argument("expected ProjectTag");
    return holder::api::ProjectTag{
        codec<std::string>::decode(value.at("tag")),
        codec<int>::decode(value.at("card_count")),
    };
}

inline json codec<holder::api::ProjectTagListResponse>::encode(const holder::api::ProjectTagListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::ProjectTag>>::encode(value.data);
    return result;
}
inline holder::api::ProjectTagListResponse codec<holder::api::ProjectTagListResponse>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("ok")
        || !value.contains("data")
) throw std::invalid_argument("expected ProjectTagListResponse");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::ProjectTagListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::ProjectTag>>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiProviderSetting>::encode(const holder::api::AiProviderSetting& value) {
    json result = json::object();
    result["provider"] = codec<std::string>::encode(value.provider);
    result["enabled"] = codec<bool>::encode(value.enabled);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    return result;
}
inline holder::api::AiProviderSetting codec<holder::api::AiProviderSetting>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("provider")
        || !value.contains("enabled")
        || !value.contains("updated_at")
) throw std::invalid_argument("expected AiProviderSetting");
    return holder::api::AiProviderSetting{
        codec<std::string>::decode(value.at("provider")),
        codec<bool>::decode(value.at("enabled")),
        codec<long long>::decode(value.at("updated_at")),
    };
}

inline json codec<holder::api::AiProviderSettingUpdateRequest>::encode(const holder::api::AiProviderSettingUpdateRequest& value) {
    json result = json::object();
    result["provider"] = codec<std::string>::encode(value.provider);
    result["enabled"] = codec<bool>::encode(value.enabled);
    if (value.updated_at) {
        result["updated_at"] = codec<std::optional<long long>>::encode(value.updated_at);
    }
    return result;
}
inline holder::api::AiProviderSettingUpdateRequest codec<holder::api::AiProviderSettingUpdateRequest>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("provider")
        || !value.contains("enabled")
) throw std::invalid_argument("expected AiProviderSettingUpdateRequest");
    return holder::api::AiProviderSettingUpdateRequest{
        codec<std::string>::decode(value.at("provider")),
        codec<bool>::decode(value.at("enabled")),
        value.contains("updated_at") ? codec<std::optional<long long>>::decode(value.at("updated_at")) : std::nullopt,
    };
}

inline json codec<holder::api::AiProviderSettingResponse>::encode(const holder::api::AiProviderSettingResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiProviderSetting>::encode(value.data);
    return result;
}
inline holder::api::AiProviderSettingResponse codec<holder::api::AiProviderSettingResponse>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("ok")
        || !value.contains("data")
) throw std::invalid_argument("expected AiProviderSettingResponse");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiProviderSettingResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiProviderSetting>::decode(value.at("data")),
    };
}

inline json codec<holder::api::AiProviderSettingsData>::encode(const holder::api::AiProviderSettingsData& value) {
    json result = json::object();
    result["providers"] = codec<std::vector<holder::api::AiProviderSetting>>::encode(value.providers);
    return result;
}
inline holder::api::AiProviderSettingsData codec<holder::api::AiProviderSettingsData>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("providers")
) throw std::invalid_argument("expected AiProviderSettingsData");
    return holder::api::AiProviderSettingsData{
        codec<std::vector<holder::api::AiProviderSetting>>::decode(value.at("providers")),
    };
}

inline json codec<holder::api::AiProviderSettingsResponse>::encode(const holder::api::AiProviderSettingsResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::AiProviderSettingsData>::encode(value.data);
    return result;
}
inline holder::api::AiProviderSettingsResponse codec<holder::api::AiProviderSettingsResponse>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("ok")
        || !value.contains("data")
) throw std::invalid_argument("expected AiProviderSettingsResponse");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::AiProviderSettingsResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::AiProviderSettingsData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::RebuildRequest>::encode(const holder::api::RebuildRequest& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    return result;
}
inline holder::api::RebuildRequest codec<holder::api::RebuildRequest>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("project_id")
) throw std::invalid_argument("expected RebuildRequest");
    return holder::api::RebuildRequest{
        codec<std::string>::decode(value.at("project_id")),
    };
}

inline json codec<holder::api::RebuildData>::encode(const holder::api::RebuildData& value) {
    json result = json::object();
    result["project_id"] = codec<std::string>::encode(value.project_id);
    result["cards"] = codec<int>::encode(value.cards);
    result["ai_messages"] = codec<int>::encode(value.ai_messages);
    result["ai_threads"] = codec<int>::encode(value.ai_threads);
    result["links"] = codec<int>::encode(value.links);
    return result;
}
inline holder::api::RebuildData codec<holder::api::RebuildData>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("project_id")
        || !value.contains("cards")
        || !value.contains("ai_messages")
        || !value.contains("ai_threads")
        || !value.contains("links")
) throw std::invalid_argument("expected RebuildData");
    return holder::api::RebuildData{
        codec<std::string>::decode(value.at("project_id")),
        codec<int>::decode(value.at("cards")),
        codec<int>::decode(value.at("ai_messages")),
        codec<int>::decode(value.at("ai_threads")),
        codec<int>::decode(value.at("links")),
    };
}

inline json codec<holder::api::RebuildResponse>::encode(const holder::api::RebuildResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::RebuildData>::encode(value.data);
    return result;
}
inline holder::api::RebuildResponse codec<holder::api::RebuildResponse>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("ok")
        || !value.contains("data")
) throw std::invalid_argument("expected RebuildResponse");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::RebuildResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::RebuildData>::decode(value.at("data")),
    };
}

inline json codec<holder::api::RunnerPullProgress>::encode(const holder::api::RunnerPullProgress& value) {
    json result = json::object();
    result["completed"] = codec<long long>::encode(value.completed);
    result["total"] = codec<long long>::encode(value.total);
    result["percent"] = codec<double>::encode(value.percent);
    result["stage"] = codec<std::string>::encode(value.stage);
    return result;
}
inline holder::api::RunnerPullProgress codec<holder::api::RunnerPullProgress>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("completed")
        || !value.contains("total")
        || !value.contains("percent")
        || !value.contains("stage")
) throw std::invalid_argument("expected RunnerPullProgress");
    return holder::api::RunnerPullProgress{
        codec<long long>::decode(value.at("completed")),
        codec<long long>::decode(value.at("total")),
        codec<double>::decode(value.at("percent")),
        codec<std::string>::decode(value.at("stage")),
    };
}

inline json codec<holder::api::RunnerPullJob>::encode(const holder::api::RunnerPullJob& value) {
    json result = json::object();
    result["job_id"] = codec<std::string>::encode(value.job_id);
    if (value.runner_id) {
        result["runner_id"] = codec<std::optional<std::string>>::encode(value.runner_id);
    }
    if (value.model_ref) {
        result["model_ref"] = codec<std::optional<std::string>>::encode(value.model_ref);
    }
    result["model"] = codec<std::string>::encode(value.model);
    result["status"] = codec<std::string>::encode(value.status);
    result["updated_at"] = codec<long long>::encode(value.updated_at);
    if (value.error) {
        result["error"] = codec<std::optional<std::optional<std::string>>>::encode(value.error);
    }
    result["progress"] = codec<holder::api::RunnerPullProgress>::encode(value.progress);
    return result;
}
inline holder::api::RunnerPullJob codec<holder::api::RunnerPullJob>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("job_id")
        || !value.contains("model")
        || !value.contains("status")
        || !value.contains("updated_at")
        || !value.contains("progress")
) throw std::invalid_argument("expected RunnerPullJob");
    return holder::api::RunnerPullJob{
        codec<std::string>::decode(value.at("job_id")),
        value.contains("runner_id") ? codec<std::optional<std::string>>::decode(value.at("runner_id")) : std::nullopt,
        value.contains("model_ref") ? codec<std::optional<std::string>>::decode(value.at("model_ref")) : std::nullopt,
        codec<std::string>::decode(value.at("model")),
        codec<std::string>::decode(value.at("status")),
        codec<long long>::decode(value.at("updated_at")),
        value.contains("error") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("error"))} : std::nullopt,
        codec<holder::api::RunnerPullProgress>::decode(value.at("progress")),
    };
}

inline json codec<holder::api::RunnerPullStatusResponse>::encode(const holder::api::RunnerPullStatusResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<holder::api::RunnerPullJob>::encode(value.data);
    return result;
}
inline holder::api::RunnerPullStatusResponse codec<holder::api::RunnerPullStatusResponse>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("ok")
        || !value.contains("data")
) throw std::invalid_argument("expected RunnerPullStatusResponse");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::RunnerPullStatusResponse{
        codec<bool>::decode(value.at("ok")),
        codec<holder::api::RunnerPullJob>::decode(value.at("data")),
    };
}

inline json codec<holder::api::routes::CardLink>::encode(const holder::api::routes::CardLink& value) {
    json result = json::object();
    result["from_card_id"] = codec<std::string>::encode(value.from_card_id);
    result["to_card_id"] = codec<std::string>::encode(value.to_card_id);
    if (value.target_project_id) {
        result["target_project_id"] = codec<std::optional<std::string>>::encode(value.target_project_id);
    }
    if (value.source_project_id) {
        result["source_project_id"] = codec<std::optional<std::string>>::encode(value.source_project_id);
    }
    result["to_type"] = codec<std::string>::encode(value.to_type);
    result["kind"] = codec<std::string>::encode(value.kind);
    if (value.label) {
        result["label"] = codec<std::optional<std::optional<std::string>>>::encode(value.label);
    }
    result["created_at"] = codec<long long>::encode(value.created_at);
    return result;
}
inline holder::api::routes::CardLink codec<holder::api::routes::CardLink>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("from_card_id")
        || !value.contains("to_card_id")
        || !value.contains("to_type")
        || !value.contains("kind")
        || !value.contains("created_at")
) throw std::invalid_argument("expected CardLink");
    return holder::api::routes::CardLink{
        codec<std::string>::decode(value.at("from_card_id")),
        codec<std::string>::decode(value.at("to_card_id")),
        value.contains("target_project_id") ? codec<std::optional<std::string>>::decode(value.at("target_project_id")) : std::nullopt,
        value.contains("source_project_id") ? codec<std::optional<std::string>>::decode(value.at("source_project_id")) : std::nullopt,
        codec<std::string>::decode(value.at("to_type")),
        codec<std::string>::decode(value.at("kind")),
        value.contains("label") ? std::optional<std::optional<std::string>>{std::in_place, codec<std::optional<std::string>>::decode(value.at("label"))} : std::nullopt,
        codec<long long>::decode(value.at("created_at")),
    };
}

inline json codec<holder::api::routes::CardLinkListResponse>::encode(const holder::api::routes::CardLinkListResponse& value) {
    json result = json::object();
    if (value.ok != true) throw std::invalid_argument("invalid ok");
    result["ok"] = codec<bool>::encode(value.ok);
    result["data"] = codec<std::vector<holder::api::routes::CardLink>>::encode(value.data);
    return result;
}
inline holder::api::routes::CardLinkListResponse codec<holder::api::routes::CardLinkListResponse>::decode(const json& value) {
    if (!value.is_object()
        || !value.contains("ok")
        || !value.contains("data")
) throw std::invalid_argument("expected CardLinkListResponse");
    if (value.at("ok") != true) throw std::invalid_argument("invalid ok");
    return holder::api::routes::CardLinkListResponse{
        codec<bool>::decode(value.at("ok")),
        codec<std::vector<holder::api::routes::CardLink>>::decode(value.at("data")),
    };
}

template <typename T> json encode(const T& value) { return codec<T>::encode(value); }
template <typename T> T decode(const json& value) { return codec<T>::decode(value); }
} // namespace rig_json
