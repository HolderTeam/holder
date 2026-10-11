#pragma once

#include "platform/Signal.h"

#include <filesystem>

namespace holder::sync {

struct ProjectSyncWorkerIntervals {
  int push_interval_seconds = 1200;
  int pull_interval_seconds = 300;
  int poll_interval_seconds = 30;
};

class ProjectSyncWorker {
 public:
  explicit ProjectSyncWorker(
      std::filesystem::path db_path,
      ProjectSyncWorkerIntervals intervals = {}
  );

  void run(const holder::core::SignalHandler& signals);

  // Pushes, now, every project with commits that have not been pushed. For the way out of an
  // idle exit. It ignores the push interval but still respects the back-off after a failed
  // push, so an offline machine is not held up retrying. Returns how many pushes it attempted.
  int run_final_push();

  static void set_fail_post_pull_metrics_for_tests(bool enabled);
  static void set_fail_post_push_metrics_for_tests(bool enabled);

 private:
  long long now_epoch_seconds() const;
  void run_startup_pull_pass();
  // Makes hidden cards reachable again in every project. Pulls repair what they bring in, so this
  // catches what changed while the daemon was not running: a card file removed or edited by hand,
  // or a project last opened by an older version.
  void run_startup_repair_pass();
  int run_push_cycle(bool final_push = false);

  std::filesystem::path db_path_;
  int push_interval_seconds_ = 1200;
  int pull_interval_seconds_ = 300;
  int poll_interval_seconds_ = 30;
};

} // namespace holder::sync
