#include "api/ProjectCards.h"

#include "api/support/Time.h"
#include "card/CardStore.h"
#include "project/ProjectRepo.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace holder::api {

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
