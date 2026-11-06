#pragma once

#include "platform/Db.h"

namespace holder::test {

// The current schema cannot be made into an older fixture by changing schema_version alone:
// v9 replaced the milestone table. These upgrade tests contain no milestones, so restore the
// v8 table shape before stamping an older version.
inline void use_v8_milestone_schema(holder::platform::Db& db) {
  db.exec(R"sql(
DROP TABLE IF EXISTS milestone_dates;
DROP INDEX IF EXISTS idx_milestones_project_card;
DROP INDEX IF EXISTS idx_milestones_project_start;
DROP TABLE IF EXISTS milestones;
CREATE TABLE milestones (
  milestone_id TEXT PRIMARY KEY,
  project_id TEXT NOT NULL,
  card_id TEXT NOT NULL,
  start_at INTEGER NOT NULL,
  end_at INTEGER NULL,
  all_day INTEGER NOT NULL DEFAULT 0,
  kind TEXT NULL,
  description TEXT NULL,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY(project_id) REFERENCES projects(project_id) ON DELETE CASCADE,
  FOREIGN KEY(card_id) REFERENCES cards(card_id) ON DELETE CASCADE
);
CREATE INDEX idx_milestones_project_card ON milestones(project_id, card_id);
CREATE INDEX idx_milestones_project_start ON milestones(project_id, start_at);
)sql");
}

} // namespace holder::test
