#include "api/MilestonesWithDates.h"

#include "card/CardRepo.h"
#include "card/MilestoneRepo.h"
#include "model/Milestone.h"
#include "model/MilestoneDate.h"
#include "project/ProjectRepo.h"

#include <cstddef>
#include <exception>
#include <string>
#include <vector>

namespace holder::api {
namespace {

long long parse_epoch(const std::string& raw, const std::string& name) {
  if (raw.empty()) throw ProjectMilestonesInvalidQuery("Missing " + name + ".");
  const auto invalid = [&] {
    return ProjectMilestonesInvalidQuery(name + " must be an epoch-second integer.");
  };
  std::size_t consumed = 0;
  long long value = 0;
  try {
    value = std::stoll(raw, &consumed);
  } catch (const std::exception&) {
    throw invalid();
  }
  if (consumed != raw.size()) throw invalid();
  return value;
}

MilestoneWithDatesListResponse list_response(const std::vector<holder::model::Milestone>& milestones) {
  MilestoneWithDatesListResponse response{true, {}};
  response.data.reserve(milestones.size());
  for (const auto& milestone : milestones) response.data.push_back(milestone_with_dates(milestone));
  return response;
}

std::vector<holder::model::Milestone> matching_milestones(holder::platform::Db& db,
                                                          const std::string& project_id,
                                                          const std::string& role,
                                                          const std::string& value_type,
                                                          const std::string& from,
                                                          const std::string& to) {
  holder::card::MilestoneRepo repo(db);
  if (value_type == "date") {
    if (from.size() != 10 || to.size() != 10)
      throw ProjectMilestonesInvalidQuery("from and to must use YYYY-MM-DD for date queries.");
    holder::model::milestone_calendar_day(from);
    holder::model::milestone_calendar_day(to);
    if (from > to) throw ProjectMilestonesInvalidQuery("from must not be after to.");
    return repo.list_days_in_range(project_id, role, from, to);
  }
  const auto from_epoch = parse_epoch(from, "from");
  const auto to_epoch = parse_epoch(to, "to");
  if (from_epoch > to_epoch) throw ProjectMilestonesInvalidQuery("from must not be after to.");
  return repo.list_instants_in_range(project_id, role, from_epoch, to_epoch);
}

} // namespace

MilestoneWithDates milestone_with_dates(const holder::model::Milestone& milestone) {
  MilestoneWithDates value{
      milestone.milestone_id, milestone.card_id, {}, milestone.kind, milestone.description,
      milestone.created_at, milestone.updated_at,
  };
  value.dates.reserve(milestone.dates.size());
  for (const auto& date : milestone.dates) {
    value.dates.push_back({date.role, date.date, date.datetime});
  }
  return value;
}

MilestoneWithDatesListResponse get_project_milestones_with_dates(holder::platform::Db& db,
                                                                 std::string project_id,
                                                                 std::string role,
                                                                 std::string value_type,
                                                                 std::string from,
                                                                 std::string to) {
  if (!holder::project::ProjectRepo(db).get(project_id)) throw ProjectMilestonesNotFound{};
  std::vector<holder::model::Milestone> matches;
  try {
    matches = matching_milestones(db, project_id, role, value_type, from, to);
  } catch (const ProjectMilestonesInvalidQuery&) {
    throw;
  } catch (const std::invalid_argument& error) {
    // Core rejects malformed calendar days and ranges this way.
    throw ProjectMilestonesInvalidQuery(error.what());
  }
  return list_response(matches);
}

MilestoneWithDatesListResponse get_card_milestones_with_dates(holder::platform::Db& db,
                                                              std::string card_id) {
  const auto card = holder::card::CardRepo(db).get(card_id);
  if (!card || card->deleted_at) throw CardMilestonesNotFound{};
  return list_response(holder::card::MilestoneRepo(db).list_for_card(card->project_id, card_id));
}

} // namespace holder::api
