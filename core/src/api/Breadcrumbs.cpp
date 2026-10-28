#include "api/Contract.h"

#include "capi/Context.h"
#include "card/CardRepo.h"
#include "project/ProjectRepo.h"

#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace holder::contract {

std::vector<CardBreadcrumb> get_card_breadcrumbs(holder_context& context, std::string project_id,
                                                 std::string card_id) {
  const auto project = holder::project::ProjectRepo(context.db).get(project_id);
  if (!project) throw NotFound("Project not found.");

  holder::card::CardRepo cards(context.db);
  std::vector<holder::model::Card> chain;
  std::unordered_set<std::string> visited;
  std::optional<std::string> current = std::move(card_id);
  while (current) {
    if (chain.size() >= 1024 || !visited.insert(*current).second)
      throw HierarchyInvalid("Card ancestry is cyclic or too deep.");
    const auto card = cards.find_by_id(project_id, *current, holder::model::CardScope::Live);
    if (!card) {
      if (chain.empty()) throw NotFound("Card not found in this project.");
      throw HierarchyInvalid("Card ancestry is broken.");
    }
    chain.push_back(*card);
    current = card->parent_card_id;
    if (current && current->empty()) current = std::nullopt;
  }

  std::vector<CardBreadcrumb> path;
  path.push_back({CardBreadcrumbKind::Project, project_id, project->name});
  for (auto it = chain.rbegin(); it != chain.rend(); ++it)
    path.push_back({CardBreadcrumbKind::Card, it->card_id, it->title});
  return path;
}

} // namespace holder::contract
