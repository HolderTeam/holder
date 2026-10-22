#include "api/AiNudges.h"

#include "ai/NudgeService.h"

namespace holder::api {
namespace {

AiNudge to_api(const holder::ai::Nudge& nudge) {
  AiNudge item{
      nudge.nudge_id,
      nudge.kind,
      nudge.project_id,
      nudge.card_id,
      nudge.title,
      nudge.body,
      nudge.meta_json,
      nudge.basis_fingerprint,
      nudge.basis_commit,
      nudge.created_at,
      std::nullopt,
  };
  if (nudge.meta_json.is_object() && nudge.meta_json.contains("suggestions")) {
    item.suggestions = nudge.meta_json["suggestions"].get<std::vector<std::string>>();
  }
  return item;
}

} // namespace

AiNudgeListResponse get_ai_nudges(holder::platform::Db& db,
                                  holder::llm::RunnerRegistry* registry,
                                  std::string project_id,
                                  std::optional<std::string> card_id) {
  if (card_id.has_value() && card_id->empty()) card_id.reset();
  AiNudgeListResponse response{true, {}};
  for (const auto& nudge : holder::ai::NudgeService(db, registry).list(project_id, card_id)) {
    response.data.nudges.push_back(to_api(nudge));
  }
  return response;
}

AiNudgeDismissResponse post_ai_nudges_nudge_id_dismiss(holder::platform::Db& db,
                                                       holder::llm::RunnerRegistry* registry,
                                                       std::string nudge_id) {
  if (!holder::ai::NudgeService(db, registry).dismiss(nudge_id)) throw AiNudgeNotFound();
  return {true, {std::move(nudge_id), true}};
}

AiNudgeEvaluateResponse post_ai_nudges_evaluate(holder::platform::Db& db,
                                                holder::llm::RunnerRegistry* registry,
                                                AiNudgeCandidate candidate) {
  const auto decision = holder::ai::NudgeService(db, registry).evaluate_and_record({
      .kind = candidate.kind,
      .project_id = std::move(candidate.project_id),
      .card_id = std::move(candidate.card_id),
      .created_at = candidate.created_at,
      .basis_fingerprint = std::move(candidate.basis_fingerprint),
      .basis_commit = std::move(candidate.basis_commit),
      .facts = std::move(candidate.facts),
  });
  return {true,
          {std::move(candidate.kind), decision.accepted, decision.should_nudge, decision.reason,
           decision.nudge.has_value() ? std::optional<AiNudge>(to_api(*decision.nudge))
                                      : std::nullopt}};
}

} // namespace holder::api
