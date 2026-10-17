#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "card/CardRepo.h"
#include "card/LinkRepo.h"
#include "card/MilestoneRepo.h"
#include "platform/Db.h"
#include "platform/Migrations.h"
#include "project/ProjectRepo.h"

#include <sqlite3.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::filesystem::path find_schema_sql() {
#ifdef SCHEMA_SQL_PATH
  std::filesystem::path p = SCHEMA_SQL_PATH;
  if (std::filesystem::exists(p)) return p;
#endif
  namespace fs = std::filesystem;
  fs::path p1 = fs::current_path() / "schema" / "schema.sql";
  if (fs::exists(p1)) return p1;
  fs::path p2 = fs::current_path().parent_path() / "schema" / "schema.sql";
  if (fs::exists(p2)) return p2;
  throw std::runtime_error("schema.sql not found for tests");
}

std::filesystem::path make_temp_dir() {
  const auto base = std::filesystem::temp_directory_path();
  const auto suffix = std::to_string(
      static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count())
  );
  auto dir = base / ("holder_migrations_test_" + suffix);
  std::filesystem::create_directories(dir);
  return dir;
}

// Older tests start with the current schema and stamp an earlier version. Restore the
// milestone shape those versions really had before exercising the migration chain.
void use_v8_milestones(holder::platform::Db& db) {
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

} // namespace

TEST_CASE("ensure_schema_version accepts expected version", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);

  const auto schema_path = find_schema_sql();
  holder::platform::Migrations::ensure_schema(db, schema_path);

  REQUIRE_NOTHROW(holder::platform::Migrations::ensure_schema_version(
      db,
      holder::platform::Migrations::latest_schema_version
  ));
  REQUIRE_FALSE(holder::platform::Migrations::migrate_to_latest(db));
}

TEST_CASE("ensure_schema_version rejects mismatch", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);

  const auto schema_path = find_schema_sql();
  holder::platform::Migrations::ensure_schema(db, schema_path);

  REQUIRE_THROWS_WITH(
      holder::platform::Migrations::ensure_schema_version(db, 1),
      Catch::Matchers::ContainsSubstring("Schema version mismatch")
  );
}

TEST_CASE("migrate_to_latest upgrades v1 databases with card tags", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  holder::platform::Migrations::ensure_schema(db, find_schema_sql());
  use_v8_milestones(db);
  db.exec("DROP TABLE card_tags;");
  db.exec("UPDATE schema_version SET version = 1;");

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  REQUIRE_NOTHROW(holder::platform::Migrations::ensure_schema_version(
      db,
      holder::platform::Migrations::latest_schema_version
  ));
  REQUIRE_NOTHROW(db.exec("SELECT project_id, card_id, tag, created_at FROM card_tags;"));
  REQUIRE_FALSE(holder::platform::Migrations::migrate_to_latest(db));
}

TEST_CASE("migrate_to_latest tolerates a v1 database that already has card tags", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  holder::platform::Migrations::ensure_schema(db, find_schema_sql());
  use_v8_milestones(db);
  db.exec("UPDATE schema_version SET version = 1;");

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  REQUIRE_NOTHROW(holder::platform::Migrations::ensure_schema_version(
      db,
      holder::platform::Migrations::latest_schema_version
  ));
}

TEST_CASE(
    "migrate_to_latest upgrades v2 databases with milestones and drops alerts",
    "[migrations]"
) {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  holder::platform::Migrations::ensure_schema(db, find_schema_sql());
  use_v8_milestones(db);
  db.exec("DROP TABLE milestones;");
  db.exec("CREATE TABLE alerts ("
          "  alert_id TEXT PRIMARY KEY,"
          "  project_id TEXT NOT NULL,"
          "  card_id TEXT NULL,"
          "  title TEXT NOT NULL,"
          "  due_at INTEGER NOT NULL,"
          "  created_at INTEGER NOT NULL,"
          "  updated_at INTEGER NOT NULL"
          ");");
  db.exec("UPDATE schema_version SET version = 2;");

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  REQUIRE_NOTHROW(holder::platform::Migrations::ensure_schema_version(
      db,
      holder::platform::Migrations::latest_schema_version
  ));
  REQUIRE_NOTHROW(db.exec("SELECT milestone_id, project_id, card_id, kind, description, "
                          "created_at, updated_at FROM milestones;"));
  REQUIRE_THROWS_WITH(
      db.exec("SELECT 1 FROM alerts;"),
      Catch::Matchers::ContainsSubstring("sqlite exec failed: no such table: alerts")
  );
  REQUIRE_FALSE(holder::platform::Migrations::migrate_to_latest(db));
}

TEST_CASE("migrate_to_latest tolerates a v2 database that already has milestones", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  holder::platform::Migrations::ensure_schema(db, find_schema_sql());
  use_v8_milestones(db);
  db.exec("UPDATE schema_version SET version = 2;");

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  REQUIRE_NOTHROW(holder::platform::Migrations::ensure_schema_version(
      db,
      holder::platform::Migrations::latest_schema_version
  ));
}

TEST_CASE("migrate_to_latest adds UUID4 ID scheme to v4 projects", "[migrations]") {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  db.exec("CREATE TABLE projects ("
          "project_id TEXT PRIMARY KEY,"
          "name TEXT NOT NULL,"
          "root_path TEXT NOT NULL,"
          "git_remote_url TEXT NULL,"
          "git_provider TEXT NULL,"
          "privacy_mode TEXT NOT NULL DEFAULT 'encrypted_git',"
          "project_key_id TEXT NULL,"
          "created_at INTEGER NOT NULL,"
          "updated_at INTEGER NOT NULL,"
          "CHECK(privacy_mode IN ('encrypted_git', 'plain'))"
          ");"
          "CREATE TABLE cards ("
          "card_id TEXT PRIMARY KEY,"
          "project_id TEXT NOT NULL,"
          "parent_card_id TEXT NULL,"
          "updated_at INTEGER NOT NULL"
          ");"
          "CREATE TABLE card_tags ("
          "project_id TEXT NOT NULL,"
          "card_id TEXT NOT NULL,"
          "tag TEXT NOT NULL"
          ");"
          "CREATE TABLE card_links ("
          "project_id TEXT NOT NULL,"
          "from_card_id TEXT NOT NULL,"
          "to_card_id TEXT NOT NULL,"
          "to_type TEXT NOT NULL,"
          "kind TEXT NOT NULL,"
          "label TEXT NULL,"
          "created_at INTEGER NOT NULL"
          ");"
          "CREATE TABLE schema_version(version INTEGER NOT NULL);"
          "INSERT INTO schema_version VALUES(4);"
          "INSERT INTO projects(project_id,name,root_path,privacy_mode,created_at,updated_at) "
          "VALUES('legacy','Legacy','/tmp/legacy','plain',1,2);");
  use_v8_milestones(db);

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  REQUIRE_NOTHROW(holder::platform::Migrations::ensure_schema_version(
      db,
      holder::platform::Migrations::latest_schema_version
  ));
  const auto project = holder::project::ProjectRepo(db).get("legacy");
  REQUIRE(project.has_value());
  REQUIRE(project->id_scheme == holder::model::IdScheme::Uuid4);
  REQUIRE_THROWS_WITH(
      db.exec("UPDATE projects SET id_scheme = 'uuid8' WHERE project_id = 'legacy';"),
      Catch::Matchers::ContainsSubstring(
          "sqlite exec failed: CHECK constraint failed: id_scheme IN ('uuid4', 'uuid7')"
      )
  );
}

TEST_CASE("migrate_to_latest adds the project card ID lookup index to v5", "[migrations]") {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  holder::platform::Migrations::ensure_schema(db, find_schema_sql());
  use_v8_milestones(db);
  db.exec("DROP INDEX idx_cards_project_card_id;");
  db.exec("UPDATE schema_version SET version = 5;");

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  REQUIRE_NOTHROW(holder::platform::Migrations::ensure_schema_version(
      db,
      holder::platform::Migrations::latest_schema_version
  ));
  REQUIRE_NOTHROW(db.exec("SELECT card_id FROM cards INDEXED BY idx_cards_project_card_id "
                          "WHERE project_id = 'project' AND card_id >= '';"));
}

TEST_CASE("migrate_to_latest adds the collection page indexes to v6", "[migrations]") {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  holder::platform::Migrations::ensure_schema(db, find_schema_sql());
  use_v8_milestones(db);
  // Recreate the v6 indexes.
  db.exec("DROP INDEX idx_cards_project_parent_card_id;"
          "DROP INDEX idx_cards_project_parent_updated;"
          "DROP INDEX idx_cards_project_updated;"
          "CREATE INDEX idx_cards_project_updated ON cards(project_id, updated_at);"
          "DROP INDEX idx_card_tags_tag;"
          "CREATE INDEX idx_card_tags_tag ON card_tags(project_id, tag);"
          "UPDATE schema_version SET version = 6;");

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  REQUIRE_NOTHROW(holder::platform::Migrations::ensure_schema_version(
      db,
      holder::platform::Migrations::latest_schema_version
  ));
  const auto columns = [&](const std::string& index) {
    std::vector<std::string> names;
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(
        sqlite3_prepare_v2(
            db.handle(),
            ("SELECT name FROM pragma_index_info('" + index + "') ORDER BY seqno;").c_str(),
            -1,
            &stmt,
            nullptr
        ) == SQLITE_OK
    );
    while (sqlite3_step(stmt) == SQLITE_ROW)
      names.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    sqlite3_finalize(stmt);
    return names;
  };
  using Names = std::vector<std::string>;
  REQUIRE(columns("idx_cards_project_updated") == Names{"project_id", "updated_at", "card_id"});
  REQUIRE(
      columns("idx_cards_project_parent_card_id") ==
      Names{"project_id", "parent_card_id", "card_id"}
  );
  REQUIRE(
      columns("idx_cards_project_parent_updated") ==
      Names{"project_id", "parent_card_id", "updated_at", "card_id"}
  );
  REQUIRE(columns("idx_card_tags_tag") == Names{"project_id", "tag", "card_id"});
}

TEST_CASE("migrate_to_latest gives v7 links qualified target identity", "[migrations]") {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  db.exec("CREATE TABLE projects(project_id TEXT PRIMARY KEY);"
          "INSERT INTO projects VALUES('source');"
          "CREATE TABLE cards(card_id TEXT PRIMARY KEY, project_id TEXT NOT NULL);"
          "CREATE TABLE card_links ("
          "project_id TEXT NOT NULL, from_card_id TEXT NOT NULL, to_card_id TEXT NOT NULL, "
          "to_type TEXT NOT NULL, kind TEXT NOT NULL, label TEXT NULL, "
          "created_at INTEGER NOT NULL, "
          "PRIMARY KEY(project_id, from_card_id, to_card_id, to_type, kind));"
          "CREATE TABLE schema_version(version INTEGER NOT NULL);"
          "INSERT INTO schema_version VALUES(7);"
          "INSERT INTO card_links VALUES('source', 'card-a', 'card-b', 'card', 'ref', "
          "'old label', 10);");
  use_v8_milestones(db);

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  REQUIRE(
      holder::platform::Migrations::read_schema_version(db) ==
      holder::platform::Migrations::latest_schema_version
  );
  holder::card::LinkRepo links(db);
  const auto migrated = links.list_outgoing("source", "card-a");
  REQUIRE(migrated.size() == 1);
  REQUIRE_FALSE(migrated[0].target_project_id.has_value());
  REQUIRE(migrated[0].label == "old label");

  auto other = migrated[0];
  other.target_project_id = "other";
  other.label = "external label";
  links.upsert_links("source", "card-a", {other});
  REQUIRE(links.list_outgoing("source", "card-a").size() == 2);
  REQUIRE_FALSE(holder::platform::Migrations::migrate_to_latest(db));
}

TEST_CASE("v9 migration indexes existing external links", "[migrations]") {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  holder::platform::Migrations::ensure_schema(db, find_schema_sql());
  db.exec("DROP INDEX idx_card_links_external_target;"
          "UPDATE schema_version SET version = 9;");

  holder::model::Project source;
  source.project_id = "source";
  source.name = "Source";
  source.root_path = (dir / "source").string();
  source.created_at = source.updated_at = 1;
  holder::project::ProjectRepo(db).create(source);

  holder::model::CardLink link;
  link.project_id = "source";
  link.from_card_id = "from-card";
  link.to_card_id = "target-card";
  link.target_project_id = "target";
  link.to_type = "card";
  link.kind = "ref";
  link.created_at = 1;
  holder::card::LinkRepo links(db);
  links.upsert_links("source", "from-card", {link});

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  REQUIRE(holder::platform::Migrations::read_schema_version(db) == 10);
  const auto matches = links.list_external_backlinks_typed(
      "target", "target-card", "card", {"source"}
  );
  REQUIRE(matches.size() == 1);
  REQUIRE(matches[0].from_card_id == "from-card");

  sqlite3_stmt* stmt = nullptr;
  REQUIRE(sqlite3_prepare_v2(
      db.handle(),
      "EXPLAIN QUERY PLAN SELECT * FROM card_links WHERE target_project_id = ? "
      "AND to_card_id = ? AND to_type = ? AND project_id = ?;",
      -1, &stmt, nullptr
  ) == SQLITE_OK);
  REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
  const std::string plan = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
  sqlite3_finalize(stmt);
  REQUIRE(plan.find("idx_card_links_external_target") != std::string::npos);
}

TEST_CASE(
    "v8 milestone migration preserves IDs and converts all-day epochs to UTC dates",
    "[migrations]"
) {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  holder::platform::Migrations::ensure_schema(db, find_schema_sql());
  use_v8_milestones(db);
  holder::model::Project project;
  project.project_id = "project";
  project.name = "Project";
  project.root_path = (dir / "project").string();
  project.created_at = project.updated_at = 1;
  holder::project::ProjectRepo(db).create(project);
  holder::model::Card card;
  card.card_id = "card";
  card.project_id = project.project_id;
  card.title = "Card";
  card.rel_path = "cards/card.md";
  card.created_at = card.updated_at = 1;
  holder::card::CardRepo(db).create(card);
  db.exec("INSERT INTO milestones VALUES"
          "('timed','project','card',100,200,0,'Review','Timed',1,2),"
          "('day','project','card',300,NULL,1,'Deadline','All day',3,4);");
  db.exec("UPDATE schema_version SET version = 8;");

  REQUIRE(holder::platform::Migrations::migrate_to_latest(db));
  const auto all = holder::card::MilestoneRepo(db).list_for_card("project", "card");
  REQUIRE(all.size() == 2);
  CHECK(all[0].milestone_id == "timed");
  CHECK(
      holder::model::milestone_unique_date_for_role(all[0], "start")->datetime == "1970-01-01T00:01:40Z"
  );
  CHECK(holder::model::milestone_epoch_for_role(all[0], "end") == 200);
  CHECK(all[1].milestone_id == "day");
  CHECK(holder::model::milestone_unique_date_for_role(all[1], "start")->date == "1970-01-01");
}

TEST_CASE("migrate_to_latest rejects databases newer than this build", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  holder::platform::Migrations::ensure_schema(db, find_schema_sql());
  db.exec(
      "UPDATE schema_version SET version = " +
      std::to_string(holder::platform::Migrations::latest_schema_version + 1) + ";"
  );

  REQUIRE_THROWS_WITH(
      holder::platform::Migrations::migrate_to_latest(db),
      Catch::Matchers::ContainsSubstring(
          "Expected at most " + std::to_string(holder::platform::Migrations::latest_schema_version)
      )
  );
}

TEST_CASE("ensure_schema fails when schema file is missing", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);

  const auto missing_schema = dir / "missing-schema.sql";
  REQUIRE_FALSE(std::filesystem::exists(missing_schema));

  REQUIRE_THROWS_WITH(
      holder::platform::Migrations::ensure_schema(db, missing_schema),
      Catch::Matchers::ContainsSubstring("Failed to open schema file")
  );
}

TEST_CASE("ensure_schema skips schema file when tables already exist", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  db.exec("CREATE TABLE existing_table(id INTEGER NOT NULL);");

  const auto missing_schema = dir / "missing-schema.sql";
  REQUIRE_FALSE(std::filesystem::exists(missing_schema));
  REQUIRE_NOTHROW(holder::platform::Migrations::ensure_schema(db, missing_schema));
}

TEST_CASE("ensure_schema throws when sqlite prepare fails", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  db.close();

  const auto schema_path = find_schema_sql();
  REQUIRE_THROWS_WITH(
      holder::platform::Migrations::ensure_schema(db, schema_path),
      Catch::Matchers::ContainsSubstring("sqlite prepare failed")
  );
}

TEST_CASE("ensure_schema_version throws when sqlite prepare fails", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  db.close();

  REQUIRE_THROWS_WITH(
      holder::platform::Migrations::ensure_schema_version(db, 1),
      Catch::Matchers::ContainsSubstring("sqlite prepare failed")
  );
}

TEST_CASE("ensure_schema_version throws when schema_version row is missing", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  db.exec("CREATE TABLE schema_version(version INTEGER NOT NULL);");

  REQUIRE_THROWS_WITH(
      holder::platform::Migrations::ensure_schema_version(db, 1),
      Catch::Matchers::ContainsSubstring("schema_version row missing")
  );
}

TEST_CASE("ensure_schema_version throws when sqlite step fails", "[migrations]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);

  auto fail_fn = [](sqlite3_context* ctx, int /*argc*/, sqlite3_value** /*argv*/) {
    sqlite3_result_error(ctx, "forced-step-error", -1);
  };
  REQUIRE(
      sqlite3_create_function_v2(
          db.handle(),
          "always_fail",
          0,
          SQLITE_UTF8,
          nullptr,
          fail_fn,
          nullptr,
          nullptr,
          nullptr
      ) == SQLITE_OK
  );

  db.exec("CREATE VIEW schema_version AS SELECT always_fail() AS version;");

  REQUIRE_THROWS_WITH(
      holder::platform::Migrations::ensure_schema_version(db, 1),
      Catch::Matchers::ContainsSubstring("sqlite step failed")
  );
}

TEST_CASE(
    "migrate_to_latest rejects a schema version outside its migration graph",
    "[migrations]"
) {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  db.exec(
      "CREATE TABLE schema_version(version INTEGER NOT NULL); INSERT INTO schema_version VALUES(0);"
  );
  REQUIRE_THROWS_WITH(
      holder::platform::Migrations::migrate_to_latest(db),
      Catch::Matchers::ContainsSubstring("Unsupported schema version")
  );
}
