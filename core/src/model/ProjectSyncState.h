#pragma once

#include <optional>
#include <string>
#include <vector>

namespace holder::model {

// What a pull changed on this device to bring two devices' versions of the same cards together.
struct PullChanges {
  std::vector<std::string> merged_card_ids;
  std::vector<std::string> kept_out_of_trash_card_ids;
  std::vector<std::string> restored_from_trash_card_ids;
  std::vector<std::string> conflict_copy_ids;
  int hidden_cards_repaired = 0;

  bool empty() const {
    return merged_card_ids.empty() && kept_out_of_trash_card_ids.empty() &&
           restored_from_trash_card_ids.empty() && conflict_copy_ids.empty() &&
           hidden_cards_repaired == 0;
  }
};

// The most recent pull that changed anything. Only a notice for this device: the changes
// themselves are commits, so losing this with a rebuilt database loses nothing else.
struct LastPullChanges {
  long long pulled_at = 0;
  PullChanges changes;
};

struct ProjectSyncState {
  std::string project_id;
  std::optional<long long> last_commit_at;
  std::optional<long long> last_push_at;
  std::optional<long long> last_pull_at;
  int uncommitted_changes_count = 0;
  int unpushed_commits_count = 0;
  std::optional<std::string> last_push_status;
  std::optional<std::string> last_pull_status;
  std::optional<std::string> last_sync_error;
  std::optional<long long> last_sync_error_at;
  int retry_count = 0;
  std::optional<long long> next_retry_at;
  int pull_retry_count = 0;
  std::optional<long long> next_pull_retry_at;
  long long updated_at = 0;
  std::optional<LastPullChanges> last_pull_changes;
};

} // namespace holder::model
