#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "card/CardRepo.h"
#include "card/LinkRepo.h"
#include "model/Card.h"
#include "model/CardLink.h"
#include "model/Project.h"
#include "platform/Db.h"
#include "project/ProjectRepo.h"

#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

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
  auto dir = base / ("holder_link_test_" + suffix);
  std::filesystem::create_directories(dir);
  return dir;
}

void apply_schema(holder::platform::Db& db) {
  const auto schema_path = find_schema_sql();
  std::ifstream in(schema_path);
  REQUIRE(in.is_open());
  std::string sql((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  db.exec(sql);
}

void create_project(holder::platform::Db& db, const std::string& project_id) {
  holder::project::ProjectRepo repo(db);
  holder::model::Project project;
  project.project_id = project_id;
  project.name = "Project";
  project.root_path = "/tmp/project";
  project.created_at = 1;
  project.updated_at = 1;
  repo.create(project);
}

void create_card(
    holder::platform::Db& db,
    const std::string& card_id,
    const std::string& project_id
) {
  holder::card::CardRepo repo(db);
  holder::model::Card card;
  card.card_id = card_id;
  card.project_id = project_id;
  card.title = card_id;
  card.rel_path = "cards/" + card_id + ".md";
  card.sort_key = 0.0;
  card.created_at = 1;
  card.updated_at = 1;
  repo.create(card);
}

} // namespace

TEST_CASE("LinkRepo upsert/list/delete", "[linkrepo]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  apply_schema(db);
  create_project(db, "proj-1");
  create_card(db, "card-a", "proj-1");
  create_card(db, "card-b", "proj-1");
  create_card(db, "card-c", "proj-1");

  holder::card::LinkRepo repo(db);

  holder::model::CardLink link1;
  link1.project_id = "proj-1";
  link1.from_card_id = "card-a";
  link1.to_card_id = "card-b";
  link1.to_type = "card";
  link1.kind = "wiki";
  link1.label = "Label";
  link1.created_at = 10;

  holder::model::CardLink link2;
  link2.project_id = "proj-1";
  link2.from_card_id = "card-a";
  link2.to_card_id = "res-1";
  link2.to_type = "resource";
  link2.kind = "ref";
  link2.created_at = 11;

  repo.upsert_links("proj-1", "card-a", {link1, link2});

  auto outgoing = repo.list_outgoing("proj-1", "card-a");
  REQUIRE(outgoing.size() == 2);
  REQUIRE_FALSE(outgoing[0].target_project_id.has_value());
  REQUIRE_FALSE(outgoing[1].target_project_id.has_value());

  auto backlinks = repo.list_backlinks("proj-1", "card-b");
  REQUIRE(backlinks.size() == 1);
  REQUIRE(backlinks[0].from_card_id == "card-a");
  REQUIRE(backlinks[0].label.has_value());
  REQUIRE_FALSE(backlinks[0].target_project_id.has_value());

  const auto incoming_resources = repo.list_incoming_typed("proj-1", "resource");
  REQUIRE(incoming_resources.size() == 1);
  REQUIRE(incoming_resources[0].to_card_id == "res-1");
  REQUIRE(incoming_resources[0].from_card_id == "card-a");

  link1.label = "New Label";
  link1.created_at = 12;
  repo.upsert_links("proj-1", "card-a", {link1});

  outgoing = repo.list_outgoing("proj-1", "card-a");
  REQUIRE(outgoing.size() == 2);

  repo.delete_links_from("proj-1", "card-a");
  outgoing = repo.list_outgoing("proj-1", "card-a");
  REQUIRE(outgoing.empty());
}

TEST_CASE("LinkRepo preserves an explicit target project ID", "[linkrepo]") {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  apply_schema(db);
  create_project(db, "proj-1");
  create_card(db, "card-a", "proj-1");
  create_card(db, "card-b", "proj-1");

  holder::model::CardLink link;
  link.project_id = "proj-1";
  link.from_card_id = "card-a";
  link.to_card_id = "card-b";
  link.target_project_id = "proj-2";
  link.to_type = "card";
  link.kind = "ref";
  link.created_at = 10;
  holder::card::LinkRepo repo(db);
  repo.upsert_links("proj-1", "card-a", {link});

  const auto outgoing = repo.list_outgoing("proj-1", "card-a");
  REQUIRE(outgoing.size() == 1);
  REQUIRE(outgoing[0].target_project_id == "proj-2");
  REQUIRE(repo.list_backlinks("proj-1", "card-b").empty());
  repo.delete_links_to_typed("proj-1", "card-b", "card");
  REQUIRE(repo.list_outgoing("proj-1", "card-a").size() == 1);
}

TEST_CASE("LinkRepo external backlinks require an allowed source and exact target", "[linkrepo]") {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  apply_schema(db);
  create_project(db, "source-a");
  create_project(db, "source-b");
  create_project(db, "target");
  create_card(db, "from-a", "source-a");
  create_card(db, "from-b", "source-b");
  create_card(db, "target-card", "target");

  holder::card::LinkRepo repo(db);
  holder::model::CardLink link;
  link.project_id = "source-a";
  link.from_card_id = "from-a";
  link.to_card_id = "target-card";
  link.target_project_id = "target";
  link.to_type = "card";
  link.kind = "ref";
  link.created_at = 10;
  repo.upsert_links("source-a", "from-a", {link});

  auto other_source = link;
  other_source.project_id = "source-b";
  other_source.from_card_id = "from-b";
  other_source.created_at = 20;
  repo.upsert_links("source-b", "from-b", {other_source});

  auto other_target = link;
  other_target.target_project_id = "other-target";
  other_target.created_at = 30;
  repo.upsert_links("source-a", "from-a", {other_target});

  auto other_type = link;
  other_type.to_type = "resource";
  other_type.created_at = 40;
  repo.upsert_links("source-a", "from-a", {other_type});

  auto local = link;
  local.target_project_id.reset();
  local.created_at = 50;
  repo.upsert_links("source-a", "from-a", {local});

  REQUIRE(repo.list_external_backlinks_typed("target", "target-card", "card", {}).empty());
  REQUIRE(repo.list_external_backlinks_typed("target", "target-card", "card", {"target"}).empty());
  const auto allowed = repo.list_external_backlinks_typed(
      "target", "target-card", "card", {"source-a", "source-a"}
  );
  REQUIRE(allowed.size() == 1);
  REQUIRE(allowed[0].project_id == "source-a");
  REQUIRE(allowed[0].target_project_id == "target");

  const auto both = repo.list_external_backlinks_typed(
      "target", "target-card", "card", {"source-a", "source-b"}
  );
  REQUIRE(both.size() == 2);
  REQUIRE(both[0].project_id == "source-b");
  REQUIRE(both[1].project_id == "source-a");
  REQUIRE(repo.list_external_backlinks_typed(
      "other-target", "target-card", "card", {"source-a"}
  ).size() == 1);
  REQUIRE(repo.list_external_backlinks_typed(
      "target", "target-card", "resource", {"source-a"}
  ).size() == 1);
  REQUIRE(repo.list_external_backlinks_typed(
      "target", "different-card", "card", {"source-a", "source-b"}
  ).empty());
  REQUIRE(repo.list_backlinks("source-a", "target-card").size() == 1);
}

TEST_CASE("LinkRepo distinguishes target projects sharing a card ID", "[linkrepo]") {
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  apply_schema(db);
  create_project(db, "proj-1");
  create_card(db, "card-a", "proj-1");

  holder::model::CardLink link;
  link.project_id = "proj-1";
  link.from_card_id = "card-a";
  link.to_card_id = "shared-card-id";
  link.to_type = "card";
  link.kind = "ref";
  link.created_at = 10;
  auto external = link;
  external.target_project_id = "proj-2";
  external.label = "Other project";
  auto explicit_local = link;
  explicit_local.target_project_id = "proj-1";
  explicit_local.label = "Local project";

  holder::card::LinkRepo repo(db);
  repo.upsert_links("proj-1", "card-a", {link, external, explicit_local});
  const auto outgoing = repo.list_outgoing("proj-1", "card-a");
  REQUIRE(outgoing.size() == 2);
  const auto external_it = std::find_if(outgoing.begin(), outgoing.end(), [](const auto& item) {
    return item.target_project_id == "proj-2";
  });
  const auto local_it = std::find_if(outgoing.begin(), outgoing.end(), [](const auto& item) {
    return !item.target_project_id.has_value();
  });
  REQUIRE(external_it != outgoing.end());
  REQUIRE(local_it != outgoing.end());
  REQUIRE(local_it->label == "Local project");

  repo.delete_link_exact("proj-1", "card-a", "shared-card-id", "card", "ref", "proj-2");
  const auto remaining = repo.list_outgoing("proj-1", "card-a");
  REQUIRE(remaining.size() == 1);
  REQUIRE_FALSE(remaining[0].target_project_id.has_value());
}

TEST_CASE("LinkRepo upsert rejects project/from mismatch", "[linkrepo]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  apply_schema(db);
  create_project(db, "proj-1");
  create_project(db, "proj-2");
  create_card(db, "card-a", "proj-1");
  create_card(db, "card-b", "proj-1");

  holder::card::LinkRepo repo(db);

  holder::model::CardLink bad;
  bad.project_id = "proj-2";
  bad.from_card_id = "card-a";
  bad.to_card_id = "card-b";
  bad.to_type = "card";
  bad.kind = "ref";
  bad.created_at = 1;

  REQUIRE_THROWS_WITH(
      repo.upsert_links("proj-1", "card-a", {bad}),
      Catch::Matchers::ContainsSubstring("link project_id/from_card_id mismatch")
  );
}

TEST_CASE("LinkRepo delete_link supports type-only and type+kind filters", "[linkrepo]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  apply_schema(db);
  create_project(db, "proj-1");
  create_card(db, "card-a", "proj-1");
  create_card(db, "card-b", "proj-1");

  holder::card::LinkRepo repo(db);

  holder::model::CardLink l1;
  l1.project_id = "proj-1";
  l1.from_card_id = "card-a";
  l1.to_card_id = "card-b";
  l1.to_type = "card";
  l1.kind = "wiki";
  l1.created_at = 1;

  holder::model::CardLink l2;
  l2.project_id = "proj-1";
  l2.from_card_id = "card-a";
  l2.to_card_id = "card-b";
  l2.to_type = "resource";
  l2.kind = "ref";
  l2.created_at = 2;

  holder::model::CardLink l3;
  l3.project_id = "proj-1";
  l3.from_card_id = "card-a";
  l3.to_card_id = "card-b";
  l3.to_type = "resource";
  l3.kind = "embed";
  l3.created_at = 3;

  repo.upsert_links("proj-1", "card-a", {l1, l2, l3});
  REQUIRE(repo.list_outgoing("proj-1", "card-a").size() == 3);

  // has_type only branch
  repo.delete_link(
      "proj-1",
      "card-a",
      "card-b",
      std::optional<std::string>("resource"),
      std::nullopt
  );
  auto outgoing = repo.list_outgoing("proj-1", "card-a");
  REQUIRE(outgoing.size() == 1);
  REQUIRE(outgoing[0].to_type == "card");

  // Reinsert resource links and delete exact type+kind
  repo.upsert_links("proj-1", "card-a", {l2, l3});
  REQUIRE(repo.list_outgoing("proj-1", "card-a").size() == 3);
  repo.delete_link(
      "proj-1",
      "card-a",
      "card-b",
      std::optional<std::string>("resource"),
      std::optional<std::string>("embed")
  );
  outgoing = repo.list_outgoing("proj-1", "card-a");
  REQUIRE(outgoing.size() == 2);

  auto typed = repo.list_backlinks_typed("proj-1", "card-b", "resource");
  REQUIRE(typed.size() == 1);
  REQUIRE(typed[0].kind == "ref");

  // has_kind only branch
  repo.delete_link("proj-1", "card-a", "card-b", std::nullopt, std::optional<std::string>("wiki"));
  outgoing = repo.list_outgoing("proj-1", "card-a");
  REQUIRE(outgoing.size() == 1);
  REQUIRE(outgoing[0].kind == "ref");
}

TEST_CASE("LinkRepo methods throw sqlite errors when DB is closed", "[linkrepo]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  apply_schema(db);
  create_project(db, "proj-1");
  create_card(db, "card-a", "proj-1");
  create_card(db, "card-b", "proj-1");

  holder::card::LinkRepo repo(db);
  db.close();

  holder::model::CardLink link;
  link.project_id = "proj-1";
  link.from_card_id = "card-a";
  link.to_card_id = "card-b";
  link.to_type = "card";
  link.kind = "ref";
  link.created_at = 1;

  REQUIRE_THROWS_WITH(
      repo.upsert_links("proj-1", "card-a", {link}),
      Catch::Matchers::ContainsSubstring("prepare upsert links failed: unknown sqlite error")
  );
  REQUIRE_THROWS_WITH(
      repo.list_outgoing("proj-1", "card-a"),
      Catch::Matchers::ContainsSubstring("prepare list outgoing links failed: unknown sqlite error")
  );
  REQUIRE_THROWS_WITH(
      repo.list_backlinks("proj-1", "card-b"),
      Catch::Matchers::ContainsSubstring("prepare list backlinks failed: unknown sqlite error")
  );
  REQUIRE_THROWS_WITH(
      repo.list_backlinks_typed("proj-1", "card-b", "card"),
      Catch::Matchers::ContainsSubstring("prepare list backlinks typed failed: unknown sqlite error"
      )
  );
  REQUIRE_THROWS_WITH(
      repo.list_incoming_typed("proj-1", "card"),
      Catch::Matchers::ContainsSubstring(
          "prepare list incoming typed links failed: unknown sqlite error"
      )
  );
  REQUIRE_THROWS_WITH(
      repo.delete_link("proj-1", "card-a", "card-b", std::nullopt, std::nullopt),
      Catch::Matchers::ContainsSubstring("prepare delete link failed: unknown sqlite error")
  );
  REQUIRE_THROWS_WITH(
      repo.delete_links_to_typed("proj-1", "card-b", "card"),
      Catch::Matchers::ContainsSubstring("prepare delete links to failed: unknown sqlite error")
  );
  REQUIRE_THROWS_WITH(
      repo.delete_links_from("proj-1", "card-a"),
      Catch::Matchers::ContainsSubstring("prepare delete links failed: unknown sqlite error")
  );
}

TEST_CASE("LinkRepo upsert throws when sqlite step fails", "[linkrepo]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  apply_schema(db);
  create_project(db, "proj-1");
  create_card(db, "card-a", "proj-1");
  create_card(db, "card-b", "proj-1");

  // Force INSERT/UPSERT step failure while still allowing statement prepare.
  db.exec("CREATE TRIGGER block_link_insert "
          "BEFORE INSERT ON card_links "
          "BEGIN SELECT RAISE(ABORT, 'blocked insert'); END;");

  holder::card::LinkRepo repo(db);
  holder::model::CardLink link;
  link.project_id = "proj-1";
  link.from_card_id = "card-a";
  link.to_card_id = "card-b";
  link.to_type = "card";
  link.kind = "ref";
  link.created_at = 1;

  REQUIRE_THROWS_WITH(
      repo.upsert_links("proj-1", "card-a", {link}),
      Catch::Matchers::ContainsSubstring("upsert link failed: blocked insert")
  );
}

namespace {
int interrupt_progress(void*) { return 1; }
} // namespace

TEST_CASE("LinkRepo list methods throw when sqlite step is interrupted", "[linkrepo]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  apply_schema(db);
  create_project(db, "proj-1");
  create_card(db, "card-a", "proj-1");
  create_card(db, "card-b", "proj-1");

  holder::card::LinkRepo repo(db);
  holder::model::CardLink link;
  link.project_id = "proj-1";
  link.from_card_id = "card-a";
  link.to_card_id = "card-b";
  link.to_type = "card";
  link.kind = "ref";
  link.created_at = 1;
  repo.upsert_links("proj-1", "card-a", {link});

  sqlite3_progress_handler(db.handle(), 1, interrupt_progress, nullptr);
  // SQLite 3.41+ can report the interrupt while preparing the statement, earlier versions while
  // stepping it. The operation name and "interrupted" are the stable part; the phase is not.
  REQUIRE_THROWS_WITH(
      repo.list_outgoing("proj-1", "card-a"),
      Catch::Matchers::ContainsSubstring("list outgoing links failed: interrupted")
  );
  REQUIRE_THROWS_WITH(
      repo.list_backlinks("proj-1", "card-b"),
      Catch::Matchers::ContainsSubstring("list backlinks failed: interrupted")
  );
  REQUIRE_THROWS_WITH(
      repo.list_backlinks_typed("proj-1", "card-b", "card"),
      Catch::Matchers::ContainsSubstring("list backlinks typed failed: interrupted")
  );
  REQUIRE_THROWS_WITH(
      repo.list_incoming_typed("proj-1", "card"),
      Catch::Matchers::ContainsSubstring("list incoming typed links failed: interrupted")
  );
  sqlite3_progress_handler(db.handle(), 0, nullptr, nullptr);
}

TEST_CASE("LinkRepo delete methods throw when sqlite delete step fails", "[linkrepo]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);
  apply_schema(db);
  create_project(db, "proj-1");
  create_card(db, "card-a", "proj-1");
  create_card(db, "card-b", "proj-1");

  holder::card::LinkRepo repo(db);
  holder::model::CardLink link;
  link.project_id = "proj-1";
  link.from_card_id = "card-a";
  link.to_card_id = "card-b";
  link.to_type = "card";
  link.kind = "ref";
  link.created_at = 1;
  repo.upsert_links("proj-1", "card-a", {link});

  // Force DELETE step failure while still allowing prepare/bind.
  db.exec("CREATE TRIGGER block_link_delete "
          "BEFORE DELETE ON card_links "
          "BEGIN SELECT RAISE(ABORT, 'blocked delete'); END;");

  REQUIRE_THROWS_WITH(
      repo.delete_link(
          "proj-1",
          "card-a",
          "card-b",
          std::optional<std::string>("card"),
          std::nullopt
      ),
      Catch::Matchers::ContainsSubstring("delete link failed: blocked delete")
  );
  REQUIRE_THROWS_WITH(
      repo.delete_links_to_typed("proj-1", "card-b", "card"),
      Catch::Matchers::ContainsSubstring("delete links to failed: blocked delete")
  );
  REQUIRE_THROWS_WITH(
      repo.delete_links_from("proj-1", "card-a"),
      Catch::Matchers::ContainsSubstring("delete links failed: blocked delete")
  );
}
