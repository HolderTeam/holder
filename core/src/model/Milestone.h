#pragma once

#include "model/MilestoneDate.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace holder::model {

struct Milestone {
  std::string milestone_id;
  std::string project_id;
  std::string card_id;
  std::vector<MilestoneDate> dates;
  std::optional<std::string> kind;
  std::optional<std::string> description;
  long long created_at = 0;
  long long updated_at = 0;
};

inline const MilestoneDate* milestone_date_for_role(
    const Milestone& milestone,
    const std::string& role
) {
  const auto it =
      std::find_if(milestone.dates.begin(), milestone.dates.end(), [&](const auto& value) {
        return value.role == role;
      });
  return it == milestone.dates.end() ? nullptr : &*it;
}

inline std::optional<long long> milestone_epoch_for_role(
    const Milestone& milestone,
    const std::string& role
) {
  const auto* value = milestone_date_for_role(milestone, role);
  return value ? milestone_date_epoch(*value) : std::nullopt;
}

inline void set_milestone_date(Milestone& milestone, MilestoneDate value) {
  validate_milestone_date(value);
  const auto it =
      std::find_if(milestone.dates.begin(), milestone.dates.end(), [&](const auto& current) {
        return current.role == value.role;
      });
  if (it == milestone.dates.end())
    milestone.dates.push_back(std::move(value));
  else
    *it = std::move(value);
}

inline void clear_milestone_date(Milestone& milestone, const std::string& role) {
  std::erase_if(milestone.dates, [&](const auto& value) {
    return value.role == role;
  });
}

} // namespace holder::model
