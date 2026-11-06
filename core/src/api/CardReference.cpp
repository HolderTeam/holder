#include "api/Contract.h"

#include "capi/Context.h"
#include "card/CardReferenceResolver.h"
#include "card/CardRepo.h"
#include "model/Card.h"
#include "project/ProjectRepo.h"

#include <string>
#include <utility>

namespace holder::contract {
namespace {

holder::model::CardScope scope_for(CardReferenceResolveRequestScope value) {
  switch (value) {
  case CardReferenceResolveRequestScope::Live: return holder::model::CardScope::Live;
  case CardReferenceResolveRequestScope::Trashed: return holder::model::CardScope::Trashed;
  case CardReferenceResolveRequestScope::Either: return holder::model::CardScope::Either;
  }
  throw InvalidArgument("scope must be live, trashed or either");
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

CardReferenceResolveData resolve_card_reference(holder_context& context,
                                                const CardReferenceResolveRequest& request) {
  if (request.project_id.empty() || request.reference.empty())
    throw InvalidArgument("project_id and reference must not be empty.");
  const auto scope = scope_for(request.scope);
  if (!holder::project::ProjectRepo(context.db).get(request.project_id))
    throw NotFound("Project not found.");

  holder::card::CardRepo cards(context.db);
  const auto result = holder::card::CardReferenceResolver(cards).resolve(
      request.project_id, request.reference, scope);
  CardReferenceResolveData data;
  switch (result.status) {
  case holder::card::CardReferenceStatus::Resolved:
    data.status = CardReferenceResolveDataStatus::Resolved;
    data.match_kind = match_kind_name(*result.match_kind);
    data.card = summarize(*result.card);
    break;
  case holder::card::CardReferenceStatus::Ambiguous:
    data.status = CardReferenceResolveDataStatus::Ambiguous;
    data.match_kind = match_kind_name(*result.match_kind);
    data.candidates.emplace();
    for (const auto& candidate : result.candidates)
      data.candidates->push_back(summarize(candidate));
    break;
  case holder::card::CardReferenceStatus::NotFound:
    data.status = CardReferenceResolveDataStatus::NotFound;
    break;
  }
  return data;
}

} // namespace holder::contract
