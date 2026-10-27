#pragma once

#include "model/Milestone.h"
#include "platform/Db.h"

#include <optional>
#include <string>
#include <vector>

namespace holder::card {

class MilestoneRepo {
 public:
  explicit MilestoneRepo(holder::platform::Db& db);

  // Replaces card_id's whole milestone set with `milestones` (delete-then-insert), matching
  // TagRepo::set_tags_for_card -- milestones are rebuilt from front matter each write, so a
  // milestone removed from the file must disappear from the index too, not just get upserted.
  void replace_for_card(
      const std::string& project_id,
      const std::string& card_id,
      const std::vector<holder::model::Milestone>& milestones
  );

  void delete_for_card(const std::string& project_id, const std::string& card_id);

  std::vector<holder::model::Milestone> list_for_card(
      const std::string& project_id,
      const std::string& card_id
  ) const;

  // Legacy Calendar query: the `start` role as an instant in [from, to].
  std::vector<holder::model::Milestone> list_in_range(
      const std::string& project_id,
      long long from,
      long long to
  ) const;

  // Query one free-form role, or any role when it is nullopt. Day values and instants are
  // deliberately separate ranges. Both exclude milestones on trashed cards.
  std::vector<holder::model::Milestone> list_days_in_range(
      const std::string& project_id,
      const std::optional<std::string>& role,
      const std::string& from,
      const std::string& to
  ) const;
  std::vector<holder::model::Milestone> list_instants_in_range(
      const std::string& project_id,
      const std::optional<std::string>& role,
      long long from,
      long long to
  ) const;

 private:
  holder::platform::Db& db_;
};

} // namespace holder::card
