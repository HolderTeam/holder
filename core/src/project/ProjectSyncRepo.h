#pragma once

#include "model/ProjectSyncState.h"
#include "platform/Db.h"

#include <optional>
#include <string>

namespace holder::project {

struct ProjectSyncActivityUpdate {
  int uncommitted_changes_count = 0;
  int unpushed_commits_count = 0;
  long long updated_at = 0;
};

class ProjectSyncRepo {
 public:
  explicit ProjectSyncRepo(holder::platform::Db& db);

  std::optional<holder::model::ProjectSyncState> get(const std::string& project_id) const;

  void record_push_result(
      const std::string& project_id,
      const std::string& status,
      bool success,
      const std::optional<std::string>& error_message,
      long long now
  );
  // A successful pull whose changes are not empty also becomes the project's last pull changes.
  // Pulls that change nothing leave the previous ones in place.
  void record_pull_result(
      const std::string& project_id,
      const std::string& status,
      bool success,
      const std::optional<std::string>& error_message,
      long long now,
      const holder::model::PullChanges* changes = nullptr
  );
  void update_activity_counts(
      const std::string& project_id,
      const ProjectSyncActivityUpdate& update
  );
  void remove(const std::string& project_id);

 private:
  void ensure_table();
  void upsert(const holder::model::ProjectSyncState& state);

  holder::platform::Db& db_;
};

} // namespace holder::project
