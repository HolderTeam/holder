#include "api/ProjectCards.h"

#include "api/support/Time.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "project/ProjectRepo.h"

#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace holder::api {

CardBreadcrumbsResponse get_projects_project_id_cards_card_id_breadcrumbs(
    holder::platform::Db& db, std::string project_id, std::string card_id) {
  const auto project = holder::project::ProjectRepo(db).get(project_id);
  if (!project) throw ProjectCardsNotFound("Project not found.");
  holder::card::CardRepo cards(db);
  std::vector<holder::model::Card> chain;
  std::unordered_set<std::string> visited;
  std::optional<std::string> current = card_id;
  while (current) {
    if (chain.size() >= 1024 || !visited.insert(*current).second)
      throw CardHierarchyInvalid("Card ancestry is cyclic or too deep.");
    const auto card = cards.find_by_id(project_id, *current, holder::model::CardScope::Live);
    if (!card) {
      if (chain.empty()) throw ProjectCardsNotFound("Card not found in this project.");
      throw CardHierarchyInvalid("Card ancestry is broken.");
    }
    chain.push_back(*card);
    current = card->parent_card_id;
    if (current && current->empty()) current = std::nullopt;
  }
  CardBreadcrumbsResponse response{true, {}};
  response.data.push_back({"project", project_id, project->name});
  for (auto it = chain.rbegin(); it != chain.rend(); ++it)
    response.data.push_back({"card", it->card_id, it->title});
  return response;
}

HierarchyRepairResponse post_projects_project_id_repair_hierarchy(
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    std::string project_id,
    std::optional<HierarchyRepairRequest> request) {
  const bool apply = request && request->apply;
  if (!holder::project::ProjectRepo(db).get(project_id)) throw ProjectCardsNotFound("Project not found.");
  if (card_store == nullptr) throw CardStoreUnavailable{};
  holder::card::HierarchyRepairPlan plan;
  try {
    plan = apply ? card_store->repair_hierarchy(project_id, support::now_epoch_seconds())
                 : card_store->plan_hierarchy_repair(project_id);
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  }
  HierarchyRepairResponse response{
      true,
      {apply && !plan.placements.empty(), {}, static_cast<int>(plan.placements.size()),
       plan.cycle_card_ids}};
  for (const auto& card_id : plan.moved_card_ids) {
    std::optional<std::string> parent;
    for (const auto& card : plan.placements) {
      if (card.card_id == card_id && card.parent_card_id) parent = *card.parent_card_id;
    }
    response.data.moved.push_back({card_id, std::move(parent)});
  }
  return response;
}

} // namespace holder::api
