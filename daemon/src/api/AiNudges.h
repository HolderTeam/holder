#pragma once

#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::llm { class RunnerRegistry; }
namespace holder::platform { class Db; }

namespace holder::api {

struct AiNudgeNotFound : std::runtime_error {
  AiNudgeNotFound() : std::runtime_error("Nudge not found.") {}
};

struct AiNudge {
  std::string nudge_id;
  std::string kind;
  std::string project_id;
  std::optional<std::string> card_id;
  std::string title;
  std::string body;
  nlohmann::json meta_json;
  std::optional<std::string> basis_fingerprint;
  std::optional<std::string> basis_commit;
  long long created_at;
  std::optional<std::vector<std::string>> suggestions;
};

struct AiNudgeListData {
  std::vector<AiNudge> nudges;
};

struct AiNudgeListResponse {
  bool ok;
  AiNudgeListData data;
};

struct AiNudgeDismissData {
  std::string nudge_id;
  bool dismissed;
};

struct AiNudgeDismissResponse {
  bool ok;
  AiNudgeDismissData data;
};

struct AiNudgeCandidate {
  std::string kind;
  std::string project_id;
  std::optional<std::string> card_id;
  long long created_at;
  std::optional<std::string> basis_fingerprint;
  std::optional<std::string> basis_commit;
  nlohmann::json facts;
};

struct AiNudgeEvaluateData {
  std::string kind;
  bool accepted;
  bool should_nudge;
  std::string reason;
  std::optional<AiNudge> nudge;
};

struct AiNudgeEvaluateResponse {
  bool ok;
  AiNudgeEvaluateData data;
};

AiNudgeListResponse get_ai_nudges(
    holder::platform::Db& db,
    holder::llm::RunnerRegistry* registry,
    std::string project_id,
    std::optional<std::string> card_id);

AiNudgeDismissResponse post_ai_nudges_nudge_id_dismiss(
    holder::platform::Db& db,
    holder::llm::RunnerRegistry* registry,
    std::string nudge_id);

AiNudgeEvaluateResponse post_ai_nudges_evaluate(
    holder::platform::Db& db,
    holder::llm::RunnerRegistry* registry,
    AiNudgeCandidate candidate);

} // namespace holder::api
