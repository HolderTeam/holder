#include "api/CardReferenceResolve.h"

#include "card/CardReferenceResolver.h"
#include "card/CardRepo.h"
#include "model/Card.h"
#include "project/ProjectRepo.h"

#include <utility>

namespace holder::api {
namespace {

holder::model::CardScope parse_scope(const std::string& value) {
  if (value == "live") return holder::model::CardScope::Live;
  if (value == "trashed") return holder::model::CardScope::Trashed;
  if (value == "either") return holder::model::CardScope::Either;
  throw CardReferenceInvalidRequest("scope must be one of: live, trashed, either.");
}

std::string match_kind_name(holder::card::CardReferenceMatchKind value) {
  switch (value) {
  case holder::card::CardReferenceMatchKind::FullId: return "full_id";
  case holder::card::CardReferenceMatchKind::IdPrefix: return "id_prefix";
  case holder::card::CardReferenceMatchKind::ExactTitle: return "exact_title";
  }
  return {}; // LCOV_EXCL_LINE
}

CardReferenceCandidate summarize(const holder::model::Card& card) {
  return {card.card_id, card.title, card.deleted_at};
}

} // namespace

CardReferenceResolveResponse resolve_card_reference(holder::platform::Db& db,
                                                     CardReferenceResolveRequest request) {
  if (request.project_id.empty() || request.reference.empty()) {
    throw CardReferenceInvalidRequest("project_id and reference must not be empty.");
  }
  const auto scope = parse_scope(request.scope);
  if (!holder::project::ProjectRepo(db).get(request.project_id)) {
    throw CardReferenceProjectNotFound{};
  }

  holder::card::CardRepo cards(db);
  const auto result = holder::card::CardReferenceResolver(cards).resolve(
      request.project_id, request.reference, scope);
  CardReferenceResolveData data;
  switch (result.status) {
  case holder::card::CardReferenceStatus::Resolved:
    data.status = "resolved";
    data.match_kind = match_kind_name(*result.match_kind);
    data.card = summarize(*result.card);
    break;
  case holder::card::CardReferenceStatus::Ambiguous:
    data.status = "ambiguous";
    data.match_kind = match_kind_name(*result.match_kind);
    data.candidates.emplace();
    for (const auto& candidate : result.candidates) {
      data.candidates->push_back(summarize(candidate));
    }
    break;
  case holder::card::CardReferenceStatus::NotFound:
    data.status = "not_found";
    break;
  }
  return {true, std::move(data)};
}

} // namespace holder::api
