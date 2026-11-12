#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "card/CardFrontMatter.h"
#include "card/CardPaths.h"
#include "card/CardStore.h"
#include "card/MilestoneRepo.h"
#include "core_test_helpers.h"
#include "git/GitRepo.h"
#include "index/FtsIndexer.h"
#include "model/Card.h"
#include "model/Milestone.h"
#include "model/Project.h"
#include "project/ProjectRepo.h"
#include "project/Rebuilder.h"
#include "sqlite_read_race_test_helpers.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

holder::model::Project create_project(
    holder::platform::Db& db,
    const std::string& project_id,
    const std::filesystem::path& root
) {
  holder::model::Project project;
  project.project_id = project_id;
  project.name = project_id;
  project.root_path = root.string();
  project.privacy_mode = "plain";
  project.created_at = 1;
  project.updated_at = 1;
  holder::project::ProjectRepo(db).create(project);
  return project;
}

holder::model::Card create_card(
    holder::card::CardStore& store,
    const std::string& project_id,
    const std::string& card_id
) {
  holder::model::Card card;
  card.project_id = project_id;
  card.card_id = card_id;
  card.title = card_id;
  card.created_at = 1;
  card.updated_at = 1;
  store.create(card, "Card body\n");
  return *store.get(card_id);
}

holder::model::Milestone milestone_for(
    const std::string& project_id,
    const std::string& card_id,
    const std::string& milestone_id
) {
  holder::model::Milestone milestone;
  milestone.project_id = project_id;
  milestone.card_id = card_id;
  milestone.milestone_id = milestone_id;
  milestone.dates = {
      holder::model::milestone_date_from_legacy("start", 100, false),
      holder::model::milestone_date_from_legacy("end", 200, false)
  };
  milestone.kind = "Review";
  milestone.description = "Original description";
  milestone.created_at = 10;
  milestone.updated_at = 10;
  return milestone;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.is_open());
  return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

} // namespace

TEST_CASE(
    "Milestone edits recheck whether the card became trashed",
    "[cardstore][broad-milestone-race]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project = create_project(db, "proj-1", dir / "project");
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const auto card = create_card(store, project.project_id, "aaaaaaaa-1111-4111-8111-111111111111");
  const auto original =
      milestone_for(project.project_id, card.card_id, "11111111-1111-4111-8111-111111111111");
  store.add_milestone(original, 10);
  holder::git::GitRepo git;
  git.open_existing(project.root_path);
  const auto head = git.head_oid();
  const auto path = std::filesystem::path(project.root_path) /
                    holder::core::card_rel_path(card.card_id);
  const auto bytes = read_file(path);
  bool updating = false;
  SECTION("adding reports a missing live card") {}
  SECTION("updating reports no matching live milestone") { updating = true; }
  holder::test::SqliteReadRace race(
      db.handle(),
      "FROM cards WHERE card_id",
      card.card_id,
      updating ? 3 : 2,
      "UPDATE cards SET deleted_at=20 WHERE card_id='" + card.card_id + "';"
  );
  if (updating) {
    holder::card::MilestoneUpdate update;
    update.has_kind = true;
    update.kind = "Changed";
    CHECK_FALSE(
        store.update_milestone(project.project_id, card.card_id, original.milestone_id, update, 30)
    );
  } else {
    auto next = original;
    next.milestone_id = "22222222-2222-4222-8222-222222222222";
    REQUIRE_THROWS_WITH(
        store.add_milestone(next, 30),
        Catch::Matchers::ContainsSubstring("live card not found")
    );
  }
  REQUIRE(race.fired);
  REQUIRE(race.result == SQLITE_OK);
  CHECK(git.head_oid() == head);
  CHECK(read_file(path) == bytes);
  const auto saved =
      holder::card::MilestoneRepo(db).list_for_card(project.project_id, card.card_id);
  REQUIRE(saved.size() == 1);
  CHECK(saved[0].kind == original.kind);
  CHECK(saved[0].updated_at == 10);
}

TEST_CASE(
    "Milestone additions reject duplicates and damaged authoritative cards atomically",
    "[cardstore][milestone-guards]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project = create_project(db, "proj-1", dir / "project");
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const auto card = create_card(store, project.project_id, "aaaaaaaa-1111-4111-8111-111111111111");
  const auto original =
      milestone_for(project.project_id, card.card_id, "11111111-1111-4111-8111-111111111111");
  store.add_milestone(original, 10);
  auto request = original;
  const auto path = std::filesystem::path(project.root_path) /
                    holder::core::card_rel_path(card.card_id);
  const auto original_bytes = read_file(path);
  std::string expected;
  bool missing_file = false;
  SECTION("duplicate milestone identity") { expected = "milestone ID already exists"; }
  SECTION("empty owner identity") {
    request.project_id.clear();
    expected = "milestone project_id/card_id must not be empty";
  }
  SECTION("card content is missing") {
    std::filesystem::rename(path, path.string() + ".saved");
    missing_file = true;
    expected = "card content missing";
  }
  SECTION("stored path does not match card identity") {
    db.exec("UPDATE cards SET rel_path='cards/wrong.md';");
    expected = "card rel_path does not match card_id";
  }
  SECTION("front matter belongs to a different card") {
    auto wrong = original_bytes;
    const auto at = wrong.find(card.card_id);
    REQUIRE(at != std::string::npos);
    wrong.replace(at, card.card_id.size(), "bbbbbbbb-2222-4222-8222-222222222222");
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    REQUIRE(output.is_open());
    output << wrong;
    expected = "card front matter identity mismatch";
  }
  holder::git::GitRepo git;
  git.open_existing(project.root_path);
  const auto head = git.head_oid();
  const auto before = missing_file ? std::string() : read_file(path);
  REQUIRE_THROWS_WITH(
      store.add_milestone(request, 30),
      Catch::Matchers::ContainsSubstring(expected)
  );
  CHECK(git.head_oid() == head);
  const auto rows = holder::card::MilestoneRepo(db).list_for_card(project.project_id, card.card_id);
  REQUIRE(rows.size() == 1);
  CHECK(rows[0].milestone_id == original.milestone_id);
  CHECK(rows[0].dates == original.dates);
  CHECK(rows[0].updated_at == 10);
  if (missing_file) {
    CHECK_FALSE(std::filesystem::exists(path));
    CHECK(read_file(path.string() + ".saved") == original_bytes);
  } else {
    CHECK(read_file(path) == before);
  }
}

TEST_CASE(
    "Legacy milestone edits preserve unrelated dates and reject ambiguous roles atomically",
    "[cardstore][milestone-update][legacy-dates]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project = create_project(db, "proj-1", dir / "project");
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const auto card = create_card(store, project.project_id, "aaaaaaaa-1111-4111-8111-111111111111");
  auto original =
      milestone_for(project.project_id, card.card_id, "11111111-1111-4111-8111-111111111111");
  const holder::model::MilestoneDate due{"due", std::string("2026-11-02"), std::nullopt};
  original.dates.push_back(due);
  holder::card::MilestoneUpdate update;
  std::string error;
  auto expected = original.dates;
  SECTION("introduce a start alongside a custom date") {
    original.dates = {due};
    update.start_at = 300;
    expected = {due, holder::model::milestone_date_from_legacy("start", 300, false)};
  }
  SECTION("introduce an end") {
    original.dates.erase(original.dates.begin() + 1);
    update.has_end_at = true;
    update.end_at = 400;
    expected = original.dates;
    expected.push_back(holder::model::milestone_date_from_legacy("end", 400, false));
  }
  SECTION("explicitly remove an end") {
    update.has_end_at = true;
    update.end_at.reset();
    expected.erase(expected.begin() + 1);
  }
  SECTION("convert existing calendar dates back to instants") {
    original.dates = {
        holder::model::milestone_date_from_legacy("start", 86400, true),
        holder::model::milestone_date_from_legacy("end", 172800, true),
        due
    };
    update.all_day = false;
    expected = {
        holder::model::milestone_date_from_legacy("start", 86400, false),
        holder::model::milestone_date_from_legacy("end", 172800, false),
        due
    };
  }
  SECTION("start cannot be inferred from a custom date") {
    original.dates = {due};
    update.all_day = true;
    error = "legacy milestone update requires a single start value";
  }
  SECTION("duplicate start roles are ambiguous") {
    original.dates.push_back(holder::model::milestone_date_from_legacy("start", 150, false));
    update.start_at = 120;
    error = "legacy milestone update requires a unique start";
  }
  SECTION("duplicate end roles are ambiguous") {
    original.dates.push_back(holder::model::milestone_date_from_legacy("end", 250, false));
    update.has_end_at = true;
    update.end_at = 300;
    error = "legacy milestone update requires a unique end";
  }
  SECTION("explicit dates cannot be mixed with legacy fields") {
    update.dates = std::vector<holder::model::MilestoneDate>{due};
    update.start_at = 120;
    error = "dates cannot be mixed with legacy milestone fields";
  }
  store.add_milestone(original, 10);
  const auto path = std::filesystem::path(project.root_path) /
                    holder::core::card_rel_path(card.card_id);
  const auto before = read_file(path);
  holder::git::GitRepo git;
  git.open_existing(project.root_path);
  const auto head = git.head_oid();
  REQUIRE(head.has_value());
  if (!error.empty()) {
    REQUIRE_THROWS_WITH(
        store.update_milestone(project.project_id, card.card_id, original.milestone_id, update, 30),
        Catch::Matchers::ContainsSubstring(error)
    );
    CHECK(read_file(path) == before);
    CHECK(git.head_oid() == head);
    const auto projected =
        holder::card::MilestoneRepo(db).list_for_card(project.project_id, card.card_id);
    REQUIRE(projected.size() == 1);
    CHECK(projected[0].dates == original.dates);
    CHECK(projected[0].updated_at == 10);
  } else {
    const auto result =
        store.update_milestone(project.project_id, card.card_id, original.milestone_id, update, 30);
    REQUIRE(result.has_value());
    CHECK(result->dates == expected);
    CHECK(result->updated_at == 30);
    CHECK(result->created_at == 10);
    CHECK(result->kind == original.kind);
    CHECK(result->description == original.description);
    CHECK(git.head_oid() != head);
    const auto parsed = holder::core::parse_card_file(read_file(path));
    REQUIRE(parsed.milestones.size() == 1);
    CHECK(parsed.milestones[0].dates == expected);
    const auto stats = holder::store::Rebuilder(db, &fts).rebuild_project(project);
    CHECK(stats.milestones == 1);
    const auto rebuilt =
        holder::card::MilestoneRepo(db).list_for_card(project.project_id, card.card_id);
    REQUIRE(rebuilt.size() == 1);
    CHECK(rebuilt[0].dates == expected);
    CHECK(rebuilt[0].updated_at == 30);
  }
}

TEST_CASE(
    "CardStore milestone update preserves fields and survives projection "
    "rebuild",
    "[cardstore][milestone-update][rebuild]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project = create_project(db, "proj-1", dir / "project");
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const auto card = create_card(store, project.project_id, "aaaaaaaa-1111-4111-8111-111111111111");
  holder::card::MilestoneRepo milestones(db);
  const auto original =
      milestone_for(project.project_id, card.card_id, "11111111-1111-4111-8111-111111111111");
  auto second =
      milestone_for(project.project_id, card.card_id, "22222222-2222-4222-8222-222222222222");
  second.dates = {holder::model::milestone_date_from_legacy("start", 300, false)};
  store.add_milestone(original, 10);
  store.add_milestone(second, 10);

  holder::git::GitRepo git;
  git.open_existing(project.root_path);
  const auto before = git.head_oid();
  REQUIRE(before.has_value());

  holder::card::MilestoneUpdate update;
  update.has_kind = true;
  update.kind = "Renewal";
  const auto result =
      store.update_milestone(project.project_id, card.card_id, original.milestone_id, update, 30);
  REQUIRE(result.has_value());
  CHECK(result->milestone_id == original.milestone_id);
  CHECK(result->project_id == original.project_id);
  CHECK(result->card_id == original.card_id);
  CHECK(result->created_at == original.created_at);
  CHECK(result->updated_at == 30);
  CHECK(result->dates == original.dates);
  CHECK(result->kind == std::optional<std::string>("Renewal"));
  CHECK(result->description == original.description);

  git.open_existing(project.root_path);
  const auto after = git.head_oid();
  REQUIRE(after.has_value());
  CHECK(*after != *before);

  const auto projected = milestones.list_for_card(project.project_id, card.card_id);
  REQUIRE(projected.size() == 2);
  CHECK(projected[0].milestone_id == original.milestone_id);
  CHECK(projected[0].kind == std::optional<std::string>("Renewal"));
  CHECK(projected[1].milestone_id == second.milestone_id);
  CHECK(projected[1].kind == second.kind);

  const auto parsed = holder::core::parse_card_file(read_file(
      std::filesystem::path(project.root_path) / holder::core::card_rel_path(card.card_id)
  ));
  REQUIRE(parsed.milestones.size() == 2);
  CHECK(parsed.milestones[0].milestone_id == original.milestone_id);
  CHECK(parsed.milestones[0].kind == std::optional<std::string>("Renewal"));

  const auto unchanged =
      store.update_milestone(project.project_id, card.card_id, original.milestone_id, update, 40);
  REQUIRE(unchanged.has_value());
  CHECK(unchanged->updated_at == 30);
  git.open_existing(project.root_path);
  CHECK(git.head_oid() == after);

  const auto stats = holder::store::Rebuilder(db, &fts).rebuild_project(project);
  CHECK(stats.milestones == 2);
  const auto rebuilt = milestones.list_for_card(project.project_id, card.card_id);
  REQUIRE(rebuilt.size() == 2);
  CHECK(rebuilt[0].milestone_id == original.milestone_id);
  CHECK(rebuilt[0].created_at == original.created_at);
  CHECK(rebuilt[0].updated_at == 30);
  CHECK(rebuilt[0].kind == std::optional<std::string>("Renewal"));
  CHECK(rebuilt[0].description == original.description);
}

TEST_CASE(
    "CardStore milestone update validates the result and enforces ownership",
    "[cardstore][milestone-update]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto first_project = create_project(db, "proj-1", dir / "first");
  const auto second_project = create_project(db, "proj-2", dir / "second");
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const auto first_card =
      create_card(store, first_project.project_id, "aaaaaaaa-1111-4111-8111-111111111111");
  const auto second_card =
      create_card(store, second_project.project_id, "bbbbbbbb-2222-4222-8222-222222222222");
  holder::card::MilestoneRepo milestones(db);
  const auto original = milestone_for(
      first_project.project_id,
      first_card.card_id,
      "11111111-1111-4111-8111-111111111111"
  );
  store.add_milestone(original, 10);

  holder::card::MilestoneUpdate update;
  update.start_at = 120;
  update.has_end_at = true;
  update.end_at = 300;
  update.all_day = true;
  update.has_kind = true;
  update.kind.reset();
  update.has_description = true;
  update.description.reset();
  const auto changed = store.update_milestone(
      first_project.project_id,
      first_card.card_id,
      original.milestone_id,
      update,
      30
  );
  REQUIRE(changed.has_value());
  CHECK(holder::model::milestone_epoch_for_role(*changed, "start") == 0);
  CHECK(holder::model::milestone_epoch_for_role(*changed, "end") == 0);
  CHECK(holder::model::milestone_unique_date_for_role(*changed, "start")->date == "1970-01-01");
  CHECK_FALSE(changed->kind.has_value());
  CHECK_FALSE(changed->description.has_value());
  CHECK(changed->milestone_id == original.milestone_id);
  CHECK(changed->created_at == original.created_at);

  holder::card::MilestoneUpdate invalid_start;
  invalid_start.start_at = 86'400;
  CHECK_THROWS_AS(
      store.update_milestone(
          first_project.project_id,
          first_card.card_id,
          original.milestone_id,
          invalid_start,
          40
      ),
      std::invalid_argument
  );
  holder::card::MilestoneUpdate invalid_end;
  invalid_end.has_end_at = true;
  invalid_end.end_at = -86'400;
  CHECK_THROWS_AS(
      store.update_milestone(
          first_project.project_id,
          first_card.card_id,
          original.milestone_id,
          invalid_end,
          40
      ),
      std::invalid_argument
  );
  const auto after_invalid = milestones.list_for_card(first_project.project_id, first_card.card_id);
  REQUIRE(after_invalid.size() == 1);
  CHECK(holder::model::milestone_epoch_for_role(after_invalid[0], "start") == 0);
  CHECK(holder::model::milestone_epoch_for_role(after_invalid[0], "end") == 0);
  CHECK(after_invalid[0].updated_at == 30);

  CHECK_FALSE(store
                  .update_milestone(
                      second_project.project_id,
                      first_card.card_id,
                      original.milestone_id,
                      update,
                      50
                  )
                  .has_value());
  CHECK_FALSE(store
                  .update_milestone(
                      second_project.project_id,
                      second_card.card_id,
                      original.milestone_id,
                      update,
                      50
                  )
                  .has_value());
  CHECK_FALSE(
      store.update_milestone(first_project.project_id, first_card.card_id, "missing", update, 50)
          .has_value()
  );

  store.trash(first_card.card_id, 60);
  CHECK_FALSE(store
                  .update_milestone(
                      first_project.project_id,
                      first_card.card_id,
                      original.milestone_id,
                      update,
                      70
                  )
                  .has_value());
}
