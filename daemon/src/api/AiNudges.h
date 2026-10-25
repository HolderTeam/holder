#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

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

struct RIG_OPEN_RECORD AiNudge {
  std::string nudge_id;
  std::string kind;
  std::string project_id;
  std::optional<std::string> card_id RIG_NULLABLE;
  std::string title;
  std::string body;
  nlohmann::json meta_json RIG_FIELD_DESCRIPTION("The evidence and details the nudge was built from.");
  std::optional<std::string> basis_fingerprint RIG_NULLABLE;
  std::optional<std::string> basis_commit RIG_NULLABLE;
  long long created_at;
  std::optional<std::vector<std::string>> suggestions RIG_OMIT_EMPTY RIG_FIELD_DESCRIPTION("Suggested titles, for a card.title_suggestion nudge.");
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiNudgeListData {
  std::vector<AiNudge> nudges;
};

struct RIG_OPEN_RECORD AiNudgeListResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiNudgeListData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiNudgeDismissData {
  std::string nudge_id;
  bool dismissed RIG_BOOL_VALUE(true);
};

struct RIG_OPEN_RECORD AiNudgeDismissResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiNudgeDismissData data;
};

struct RIG_OPEN_RECORD AiNudgeCandidate {
  std::string kind RIG_MIN_LENGTH(1) RIG_FIELD_DESCRIPTION("The candidate's kind, such as card.title_only; an unknown kind is not accepted.");
  std::string project_id RIG_MIN_LENGTH(1);
  std::optional<std::string> card_id RIG_OMIT_EMPTY;
  long long created_at;
  std::optional<std::string> basis_fingerprint RIG_OMIT_EMPTY;
  std::optional<std::string> basis_commit RIG_OMIT_EMPTY;
  nlohmann::json facts RIG_FIELD_DESCRIPTION("The evidence for the candidate, which depends on its kind.");
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiNudgeEvaluateData {
  std::string kind;
  bool accepted;
  bool should_nudge;
  std::string reason;
  std::optional<AiNudge> nudge RIG_FIELD_DESCRIPTION("The recorded nudge, or null when there is nothing to show.");
};

struct RIG_OPEN_RECORD AiNudgeEvaluateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiNudgeEvaluateData data;
};

RIG_ROUTE(GET, "/ai/nudges")
RIG_SUMMARY("List a project's current nudges")
RIG_DESCRIPTION("Nudges whose evidence no longer holds are removed before listing, and dismissed nudges are left out.")
RIG_RESPONSE_DESCRIPTION("OK")
AiNudgeListResponse get_ai_nudges(
    holder::platform::Db& db RIG_CONTEXT,
    holder::llm::RunnerRegistry* registry RIG_CONTEXT,
    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::optional<std::string> card_id RIG_QUERY RIG_PARAMETER_DESCRIPTION("Only this card's nudges; empty means every card's."));

RIG_ROUTE(POST, "/ai/nudges/{nudge_id}/dismiss")
RIG_SUMMARY("Dismiss a nudge")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Nudge not found", AiNudgeNotFound)
AiNudgeDismissResponse post_ai_nudges_nudge_id_dismiss(
    holder::platform::Db& db RIG_CONTEXT,
    holder::llm::RunnerRegistry* registry RIG_CONTEXT,
    std::string nudge_id);

RIG_ROUTE(POST, "/ai/nudges/evaluate")
RIG_SUMMARY("Evaluate a nudge candidate and record the nudge it warrants")
RIG_DESCRIPTION("Answers whether the candidate is accepted and whether it warrants a nudge, with the reason. A candidate the daemon does not know is answered, not rejected.")
RIG_RESPONSE_DESCRIPTION("OK")
AiNudgeEvaluateResponse post_ai_nudges_evaluate(
    holder::platform::Db& db RIG_CONTEXT,
    holder::llm::RunnerRegistry* registry RIG_CONTEXT,
    AiNudgeCandidate candidate);

} // namespace holder::api
