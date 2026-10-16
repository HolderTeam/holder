#pragma once

#include "model/MilestoneDate.h"

#include <algorithm>
#include <iterator>
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

// Timestamp-only adapters can represent a role only when it has one value.
inline const MilestoneDate* milestone_unique_date_for_role(
    const Milestone& milestone,
    const std::string& role
) {
  const auto it =
      std::find_if(milestone.dates.begin(), milestone.dates.end(), [&](const auto& value) {
        return value.role == role;
      });
  if (it == milestone.dates.end()) return nullptr;
  return std::find_if(std::next(it), milestone.dates.end(), [&](const auto& value) {
           return value.role == role;
         }) == milestone.dates.end()
             ? &*it
             : nullptr;
}

inline std::optional<long long> milestone_epoch_for_role(
    const Milestone& milestone,
    const std::string& role
) {
  const auto* value = milestone_unique_date_for_role(milestone, role);
  return value ? milestone_legacy_epoch(*value) : std::nullopt;
}

} // namespace holder::model
