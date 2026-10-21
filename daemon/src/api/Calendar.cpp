#include "api/Calendar.h"

#include "api/CardMilestones.h"

#include "card/CardRepo.h"
#include "card/MilestoneRepo.h"
#include "model/Card.h"
#include "model/Milestone.h"
#include "project/ProjectRepo.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace holder::api {
namespace {

CalendarCardActivity card_activity(const holder::model::Card& card) {
  return {card.card_id, card.title, card.created_at, card.updated_at};
}

} // namespace

ProjectCalendarResponse get_calendar(holder::platform::Db& db,
                                     std::string project_id,
                                     long long from,
                                     long long to) {
  if (from > to) throw CalendarInvalidRange{};
  if (!holder::project::ProjectRepo(db).get(project_id)) throw CalendarProjectNotFound{};

  ProjectCalendarResponse response{true, {project_id, from, to, {}, {}, {}}};
  auto& data = response.data;
  std::unordered_map<std::string, std::string> active_titles;
  for (const auto& card : holder::card::CardRepo(db).list_all(project_id)) {
    if (card.deleted_at) continue;
    active_titles.emplace(card.card_id, card.title);
    if (card.created_at >= from && card.created_at <= to) {
      data.created_cards.push_back(card_activity(card));
    }
    if (card.updated_at != card.created_at && card.updated_at >= from && card.updated_at <= to) {
      data.updated_cards.push_back(card_activity(card));
    }
  }

  for (const auto& milestone :
       holder::card::MilestoneRepo(db).list_in_range(project_id, from, to)) {
    const auto title = active_titles.find(milestone.card_id);
    if (title == active_titles.end()) continue;
    data.milestones.push_back(timestamp_milestone(milestone, title->second));
  }
  return response;
}

} // namespace holder::api
