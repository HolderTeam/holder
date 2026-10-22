#include "api/CardMilestones.h"

#include "card/CardRepo.h"
#include "card/MilestoneRepo.h"
#include "model/Milestone.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace holder::api {

bool has_timestamp_view(const holder::model::Milestone& milestone) {
  if (!holder::model::milestone_epoch_for_role(milestone, "start")) return false;
  const bool has_end = std::any_of(milestone.dates.begin(), milestone.dates.end(),
                                   [](const auto& value) { return value.role == "end"; });
  return !has_end || holder::model::milestone_unique_date_for_role(milestone, "end");
}

Milestone timestamp_milestone(const holder::model::Milestone& milestone,
                              std::optional<std::string> card_title) {
  const auto start_at = holder::model::milestone_epoch_for_role(milestone, "start");
  if (!start_at) throw std::invalid_argument("milestone has no timestamp start");
  const auto* start = holder::model::milestone_unique_date_for_role(milestone, "start");
  return {
      milestone.milestone_id,
      milestone.card_id,
      *start_at,
      holder::model::milestone_epoch_for_role(milestone, "end"),
      start->date.has_value(),
      milestone.kind,
      milestone.description,
      milestone.created_at,
      milestone.updated_at,
      std::move(card_title),
  };
}

MilestoneListResponse list_card_milestones(holder::platform::Db& db, std::string card_id) {
  const auto card = holder::card::CardRepo(db).get(card_id);
  if (!card || card->deleted_at) throw MilestoneCardNotFound{};
  MilestoneListResponse response{true, {}};
  for (const auto& milestone :
       holder::card::MilestoneRepo(db).list_for_card(card->project_id, card_id)) {
    if (has_timestamp_view(milestone)) response.data.push_back(timestamp_milestone(milestone));
  }
  std::stable_sort(response.data.begin(), response.data.end(),
                   [](const auto& a, const auto& b) { return a.start_at < b.start_at; });
  return response;
}

} // namespace holder::api
