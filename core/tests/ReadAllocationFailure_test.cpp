// Isolated sweeps of allocations issued by core-owned repository/parser paths.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "card/CardFrontMatter.h"
#include "card/CardRepo.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"
#include "project/ProjectRepo.h"
#include "project/Rebuilder.h"
#include <holder/holder_api.h>

#include <yaml-cpp/yaml.h>

#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string>

namespace {
thread_local std::size_t allocations_until_failure = 0;
thread_local bool allocation_failed = false;
thread_local bool fail_yaml_load = false;
thread_local std::optional<holder::store::Rebuilder::RebuildStats> injected_stats;
struct InjectStats {
  explicit InjectStats(holder::store::Rebuilder::RebuildStats value) { injected_stats = value; }
  ~InjectStats() { injected_stats.reset(); }
  bool consumed() const { return !injected_stats.has_value(); }
};
struct FailYamlLoad {
  FailYamlLoad() { fail_yaml_load = true; }
  ~FailYamlLoad() { fail_yaml_load = false; }
  bool consumed() const { return !fail_yaml_load; }
};
int statement_count(sqlite3* db) {
  int count = 0;
  for (auto* statement = sqlite3_next_stmt(db, nullptr); statement;
       statement = sqlite3_next_stmt(db, statement))
    ++count;
  return count;
}

struct FailAllocation {
  explicit FailAllocation(std::size_t at) {
    allocation_failed = false;
    allocations_until_failure = at;
  }
  ~FailAllocation() { allocations_until_failure = 0; }
};

template <class Invoke, class Validate> void sweep(Invoke&& invoke, Validate&& validate) {
  using Result = decltype(invoke());
  for (std::size_t at = 1; at <= 512; ++at) {
    INFO("Allocation position: " << at);
    std::optional<Result> result;
    bool threw = false;
    {
      FailAllocation fault(at);
      try {
        result.emplace(invoke());
      } catch (const std::bad_alloc&) {
        threw = true;
      }
    }
    if (!allocation_failed) {
      REQUIRE_FALSE(threw);
      REQUIRE(result.has_value());
      validate(*result);
      return;
    }
    REQUIRE(threw);
    REQUIRE_FALSE(result.has_value());
    // The same data and service must support a complete retry after the failure.
    validate(invoke());
  }
  FAIL("Read did not exhaust its allocation positions within the sweep bound");
}
} // namespace

extern "C" void* __real__Znwm(std::size_t);
extern "C" void* __wrap__Znwm(std::size_t size) {
  if (allocations_until_failure && --allocations_until_failure == 0) {
    allocation_failed = true;
    throw std::bad_alloc();
  }
  return __real__Znwm(size);
}

// YAML::Load(const std::string&) with the GNU C++11 string ABI. CMake checks it.
YAML::Node real_yaml_load(const std::string&) __asm__(
    "__real__ZN4YAML4LoadERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE"
);
YAML::Node wrap_yaml_load(const std::string&) __asm__(
    "__wrap__ZN4YAML4LoadERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE"
);
YAML::Node wrap_yaml_load(const std::string& text) {
  if (fail_yaml_load) {
    fail_yaml_load = false;
    throw std::bad_alloc();
  }
  return real_yaml_load(text);
}

TEST_CASE(
    "Card collection allocation failures release SQLite statements and preserve query results",
    "[read-allocation]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::model::Project project;
  project.project_id = "allocation-project";
  project.name = "Allocation query test";
  project.root_path = (dir / "project").string();
  project.privacy_mode = "plain";
  holder::project::ProjectRepo(db).create(project);
  holder::card::CardRepo cards(db);
  holder::model::Card first;
  first.card_id = "allocation-card-one";
  first.project_id = project.project_id;
  first.title = "First title longer than small string storage";
  first.rel_path = "cards/allocation-card-one.md";
  cards.create(first);
  auto second = first;
  second.card_id = "allocation-card-two";
  second.title = "Second title longer than small string storage";
  second.rel_path = "cards/allocation-card-two.md";
  cards.create(second);
  db.exec(
      "INSERT INTO card_tags VALUES ('allocation-project', 'allocation-card-one', 'selected', 1);"
  );
  holder::card::CardPageQuery query;
  std::size_t expected_count = 2;
  SECTION("unfiltered page") {}
  SECTION("indexed tag page") {
    query.tag = "selected";
    expected_count = 1;
  }
  const auto changes = sqlite3_total_changes64(db.handle());
  const auto statements = statement_count(db.handle());
  sweep(
      [&] {
        return cards.list_collection_page(project.project_id, query, 10);
      },
      [&](const std::vector<holder::model::Card>& page) {
        CHECK(sqlite3_total_changes64(db.handle()) == changes);
        CHECK(statement_count(db.handle()) == statements);
        REQUIRE(page.size() == expected_count);
        CHECK(page[0].card_id == first.card_id);
        CHECK(page[0].title == first.title);
        if (expected_count == 2) {
          CHECK(page[1].card_id == second.card_id);
          CHECK(page[1].title == second.title);
        }
      }
  );
}

TEST_CASE(
    "Card parser allocation failure must not masquerade as malformed front matter",
    "[read-allocation]"
) {
  const std::string raw = "---\n"
                          "card_id: allocation-card-original\n"
                          "project_id: allocation-project-original\n"
                          "title: A complete title which uses heap storage\n"
                          "created_at: 10\n"
                          "updated_at: 20\n"
                          "sort_key: 1.5\n"
                          "links:\n"
                          "  - to: allocation-target-card\n"
                          "    to_type: card\n"
                          "    kind: related\n"
                          "    created_at: 15\n"
                          "    label: A complete link label which uses heap storage\n"
                          "---\n"
                          "Original body whose front matter must never become visible content.\n";
  const auto validate = [&](const holder::core::ParsedCardFile& parsed) {
    REQUIRE(parsed.has_front_matter);
    CHECK(parsed.card.card_id == "allocation-card-original");
    CHECK(parsed.card.project_id == "allocation-project-original");
    CHECK(parsed.card.title == "A complete title which uses heap storage");
    REQUIRE(parsed.links.size() == 1);
    CHECK(parsed.links[0].to_card_id == "allocation-target-card");
    CHECK(parsed.links[0].label == "A complete link label which uses heap storage");
    CHECK(parsed.body == "Original body whose front matter must never become visible content.\n");
  };
  validate(holder::core::parse_card_file(raw));
  std::optional<holder::core::ParsedCardFile> result;
  bool threw = false;
  bool injected = false;
  {
    FailYamlLoad fault;
    try {
      result.emplace(holder::core::parse_card_file(raw));
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    injected = fault.consumed();
  }
  CHECK(injected);
  CHECK(threw);
  CHECK_FALSE(result.has_value());
  validate(holder::core::parse_card_file(raw));
  // Malformed input keeps the established fallback behavior once injection is disabled.
  const std::string malformed = "---\ncard_id: [unterminated\n---\nbody\n";
  const auto rejected = holder::core::parse_card_file(malformed);
  CHECK_FALSE(rejected.has_front_matter);
  CHECK(rejected.body == malformed);
}

// The adapter must validate a large backend result without constructing billions of rows.
using Rebuilder = holder::store::Rebuilder;
Rebuilder::RebuildStats real_rebuild(Rebuilder*, const holder::model::Project&) __asm__(
    "__real__ZN6holder5store9Rebuilder15rebuild_projectERKNS_5model7ProjectE"
);
Rebuilder::RebuildStats wrap_rebuild(Rebuilder*, const holder::model::Project&) __asm__(
    "__wrap__ZN6holder5store9Rebuilder15rebuild_projectERKNS_5model7ProjectE"
);
Rebuilder::RebuildStats wrap_rebuild(Rebuilder* self, const holder::model::Project& project) {
  if (injected_stats) {
    const auto value = *injected_stats;
    injected_stats.reset();
    return value;
  }
  return real_rebuild(self, project);
}

TEST_CASE(
    "Rebuild response counts accept INT32_MAX and reject overflow without partial results",
    "[read-allocation][rebuild-boundary]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  context.projects_root = dir / "projects";
  context.registry_path = dir / "projects.json";
  holder::contract::NewProject make;
  make.name = "Rebuild count boundaries";
  make.privacy_mode = holder::contract::NewProjectPrivacyMode::Plain;
  const auto project = holder::contract::create_project(context, make);
  const holder_rebuild_request request{project.project_id.c_str()};
  const auto changes = sqlite3_total_changes64(db.handle());
  using Stats = Rebuilder::RebuildStats;
  const std::array<std::size_t Stats::*, 4> fields{
      &Stats::cards,
      &Stats::ai_messages,
      &Stats::ai_threads,
      &Stats::links
  };
  const std::array<int32_t holder_rebuild_result::*, 4> outputs{
      &holder_rebuild_result::cards,
      &holder_rebuild_result::ai_messages,
      &holder_rebuild_result::ai_threads,
      &holder_rebuild_result::links
  };
  for (std::size_t field = 0; field < fields.size(); ++field) {
    for (bool overflow : {false, true}) {
      INFO("Field " << field << ", overflow " << overflow);
      Stats stats;
      stats.*fields[field] = static_cast<std::size_t>(std::numeric_limits<int32_t>::max()) +
                             static_cast<std::size_t>(overflow);
      std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
          holder_arena_new(),
          holder_arena_free
      );
      const holder_rebuild_result* result = nullptr;
      holder_error* raw_error = nullptr;
      int status;
      bool consumed;
      {
        InjectStats backend(stats);
        status = holder_rebuild_project(&context, &request, arena.get(), &result, &raw_error);
        consumed = backend.consumed();
      }
      std::unique_ptr<holder_error, decltype(&holder_error_destroy)> error(
          raw_error,
          holder_error_destroy
      );
      CHECK(consumed);
      CHECK(sqlite3_total_changes64(db.handle()) == changes);
      if (overflow) {
        CHECK(status == HOLDER_ERROR_REBUILD_FAILED);
        CHECK(result == nullptr);
        REQUIRE(error != nullptr);
        CHECK(std::string(holder_error_message(error.get())) == "rebuild count exceeds API range");
      } else {
        CHECK(status == HOLDER_OK);
        CHECK(error == nullptr);
        REQUIRE(result != nullptr);
        CHECK(result->*outputs[field] == std::numeric_limits<int32_t>::max());
      }
    }
  }
  // The wrapper's default path still performs a real rebuild.
  const auto normal = holder::contract::rebuild_project(context, {project.project_id});
  CHECK(normal.cards == 0);
}
