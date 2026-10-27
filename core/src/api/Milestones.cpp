// The contract's milestone and calendar operations (contract/interface/milestones.py). The generated
// glue checks their arguments first; these do the work.
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"

#include "card/CardRepo.h"
#include "card/MilestoneRepo.h"
#include "model/Milestone.h"
#include "model/MilestoneDate.h"
#include "project/ProjectRepo.h"

#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace holder::contract {
namespace {

Milestone to_milestone(const holder::model::Milestone& milestone) {
  Milestone out{milestone.milestone_id, milestone.project_id, milestone.card_id, {},
                milestone.kind,         milestone.description, milestone.created_at, milestone.updated_at};
  for (const auto& date : milestone.dates) out.dates.push_back({date.role, date.date, date.datetime});
  return out;
}

std::vector<Milestone> to_milestones(const std::vector<holder::model::Milestone>& milestones) {
  std::vector<Milestone> out;
  out.reserve(milestones.size());
  for (const auto& milestone : milestones) out.push_back(to_milestone(milestone));
  return out;
}

// The contract states a date's form in words, so core checks it.
std::vector<holder::model::MilestoneDate> checked_dates(const std::vector<MilestoneDate>& dates) {
  std::vector<holder::model::MilestoneDate> out;
  out.reserve(dates.size());
  for (const auto& date : dates) {
    holder::model::MilestoneDate value{date.role, date.date, date.datetime};
    try {
      holder::model::validate_milestone_date(value);
    } catch (const std::invalid_argument& error) {
      throw InvalidArgument(error.what());
    }
    out.push_back(std::move(value));
  }
  return out;
}

std::string checked_day(const std::string& day, const char* name) {
  try {
    holder::model::milestone_calendar_day(day);
  } catch (const std::invalid_argument&) {
    throw InvalidArgument(std::string(name) + " must be a day, YYYY-MM-DD");
  }
  return day;
}

void require_project(holder_context& context, const std::string& project_id) {
  if (!holder::project::ProjectRepo(context.db).get(project_id)) throw NotFound("project not found: " + project_id);
}

std::vector<Milestone> card_milestones(holder_context& context, const std::string& project_id,
                                       const std::string& card_id) {
  return to_milestones(holder::card::MilestoneRepo(context.db).list_for_card(project_id, card_id));
}

std::vector<Milestone> list(holder_context& context, const std::string& project_id, const std::string& card_id) {
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  return card_milestones(context, project_id, card_id);
}

Milestone add(holder_context& context, const std::string& project_id, const std::string& card_id,
              const NewMilestone& request) {
  holder::model::Milestone milestone;
  milestone.project_id = project_id;
  milestone.card_id = card_id;
  milestone.dates = checked_dates(request.dates);
  milestone.kind = request.kind;
  milestone.description = request.description;
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  return to_milestone(store.add_milestone(std::move(milestone), now_epoch_seconds()));
}

Milestone update(holder_context& context, const std::string& project_id, const std::string& card_id,
                 const std::string& milestone_id, const MilestoneUpdate& request) {
  holder::card::MilestoneUpdate change;
  if (request.dates) change.dates = checked_dates(*request.dates);
  change.has_kind = request.kind.has_value();
  change.kind = request.kind.value_or(std::nullopt);
  change.has_description = request.description.has_value();
  change.description = request.description.value_or(std::nullopt);
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  const auto updated = store.update_milestone(project_id, card_id, milestone_id, change, now_epoch_seconds());
  if (!updated) throw NotFound("milestone not found: " + milestone_id);
  return to_milestone(*updated);
}

std::vector<Milestone> remove(holder_context& context, const std::string& project_id, const std::string& card_id,
                              const std::string& milestone_id) {
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  store.remove_milestone(project_id, card_id, milestone_id, now_epoch_seconds());
  return card_milestones(context, project_id, card_id);
}

std::vector<Milestone> by_day(holder_context& context, const std::string& project_id, const std::string& first_day,
                              const std::string& last_day, const std::optional<std::string>& role) {
  checked_day(first_day, "first_day");
  checked_day(last_day, "last_day");
  require_project(context, project_id);
  return to_milestones(holder::card::MilestoneRepo(context.db).list_days_in_range(project_id, role, first_day, last_day));
}

std::vector<Milestone> by_instant(holder_context& context, const std::string& project_id, long long first_instant,
                                  long long last_instant, const std::optional<std::string>& role) {
  require_project(context, project_id);
  return to_milestones(
      holder::card::MilestoneRepo(context.db).list_instants_in_range(project_id, role, first_instant, last_instant));
}

Calendar calendar(holder_context& context, const std::string& project_id, long long first_instant,
                  long long last_instant, const std::string& first_day, const std::string& last_day) {
  checked_day(first_day, "first_day");
  checked_day(last_day, "last_day");
  require_project(context, project_id);
  Calendar out;
  std::map<std::string, std::string> titles;
  for (auto& card : holder::card::CardRepo(context.db).list_all(project_id)) {
    if (card.deleted_at) continue;
    titles.emplace(card.card_id, card.title);
    if (card.created_at >= first_instant && card.created_at <= last_instant) out.created_cards.push_back(card);
    if (card.updated_at != card.created_at && card.updated_at >= first_instant && card.updated_at <= last_instant)
      out.updated_cards.push_back(std::move(card));
  }
  std::ranges::sort(out.created_cards, {}, [](const auto& card) { return std::tie(card.created_at, card.card_id); });
  std::ranges::sort(out.updated_cards, {}, [](const auto& card) { return std::tie(card.updated_at, card.card_id); });

  holder::card::MilestoneRepo repo(context.db);
  auto milestones = repo.list_days_in_range(project_id, std::nullopt, first_day, last_day);
  for (auto& milestone : repo.list_instants_in_range(project_id, std::nullopt, first_instant, last_instant)) {
    if (std::ranges::none_of(milestones, [&](const auto& m) { return m.milestone_id == milestone.milestone_id; }))
      milestones.push_back(std::move(milestone));
  }
  std::ranges::sort(milestones, {}, [](const auto& m) { return std::tie(m.created_at, m.milestone_id); });
  for (const auto& milestone : milestones) {
    const auto title = titles.find(milestone.card_id);
    if (title == titles.end()) continue;
    out.milestones.push_back({to_milestone(milestone), title->second});
  }
  return out;
}

} // namespace

std::vector<Milestone> list_card_milestones(holder_context& context, std::string project_id, std::string card_id) {
  return with_contract_errors([&] { return list(context, project_id, card_id); });
}

Milestone add_card_milestone(holder_context& context, std::string project_id, std::string card_id,
                             const NewMilestone& milestone) {
  return with_contract_errors([&] { return add(context, project_id, card_id, milestone); });
}

Milestone update_card_milestone(holder_context& context, std::string project_id, std::string card_id,
                                std::string milestone_id, const MilestoneUpdate& request) {
  return with_contract_errors([&] { return update(context, project_id, card_id, milestone_id, request); });
}

std::vector<Milestone> remove_card_milestone(holder_context& context, std::string project_id, std::string card_id,
                                             std::string milestone_id) {
  return with_contract_errors([&] { return remove(context, project_id, card_id, milestone_id); });
}

std::vector<Milestone> list_milestones_by_day(holder_context& context, std::string project_id, std::string first_day,
                                              std::string last_day, std::optional<std::string> role) {
  return with_contract_errors([&] { return by_day(context, project_id, first_day, last_day, role); });
}

std::vector<Milestone> list_milestones_by_instant(holder_context& context, std::string project_id,
                                                  long long first_instant, long long last_instant,
                                                  std::optional<std::string> role) {
  return with_contract_errors([&] { return by_instant(context, project_id, first_instant, last_instant, role); });
}

Calendar get_calendar(holder_context& context, std::string project_id, long long first_instant,
                      long long last_instant, std::string first_day, std::string last_day) {
  return with_contract_errors(
      [&] { return calendar(context, project_id, first_instant, last_instant, first_day, last_day); });
}

} // namespace holder::contract
