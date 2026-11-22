#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "sync/CardMerge.h"

namespace {

using holder::core::ParsedCardFile;
using holder::sync::merge_body_lines;
using holder::sync::merge_card_versions;

constexpr const char* kCardId = "550e8400-e29b-41d4-a716-446655440000";

ParsedCardFile base_card() {
  ParsedCardFile card;
  card.has_front_matter = true;
  card.card.card_id = kCardId;
  card.card.project_id = "proj-1";
  card.card.rel_path = "cards/55/0e/550e8400-e29b-41d4-a716-446655440000.md";
  card.card.title = "Notes";
  card.card.sort_key = 1;
  card.card.created_at = 10;
  card.card.updated_at = 10;
  card.body = "First.\n\nSecond.\n";
  return card;
}

holder::model::CardLink link_to(const std::string& target, std::optional<std::string> label = {}) {
  holder::model::CardLink link;
  link.project_id = "proj-1";
  link.from_card_id = kCardId;
  link.to_card_id = target;
  link.to_type = "card";
  link.kind = "ref";
  link.label = std::move(label);
  link.created_at = 10;
  return link;
}

holder::model::Milestone milestone(const std::string& id, const std::string& description) {
  holder::model::Milestone value;
  value.milestone_id = id;
  value.project_id = "proj-1";
  value.card_id = kCardId;
  value.description = description;
  value.created_at = 10;
  value.updated_at = 10;
  return value;
}

} // namespace

TEST_CASE("merge_card_versions combines changes to different fields", "[sync][merge]") {
  const auto base = base_card();
  auto local = base;
  auto remote = base;

  SECTION("a move on one side and a body edit on the other") {
    local.card.parent_card_id = "parent-id";
    local.card.sort_key = 5;
    local.card.updated_at = 20;
    remote.body = "First.\n\nSecond, edited.\n";
    remote.card.updated_at = 15;
    const auto merged = merge_card_versions(base, local, remote);
    REQUIRE(merged.has_value());
    CHECK(merged->card.parent_card_id == "parent-id");
    CHECK(merged->card.sort_key == 5);
    CHECK(merged->body == remote.body);
    CHECK(merged->card.updated_at == 20);
  }

  SECTION("a title on one side and a body edit on the other") {
    local.body = "First, edited.\n\nSecond.\n";
    remote.card.title = "Meeting notes";
    const auto merged = merge_card_versions(base, local, remote);
    REQUIRE(merged.has_value());
    CHECK(merged->card.title == "Meeting notes");
    CHECK(merged->body == local.body);
  }

  SECTION("the same edit on both sides") {
    local.body = remote.body = "First.\n\nSecond, fixed.\n";
    local.card.updated_at = 30;
    remote.card.updated_at = 25;
    const auto merged = merge_card_versions(base, local, remote);
    REQUIRE(merged.has_value());
    CHECK(merged->body == "First.\n\nSecond, fixed.\n");
    CHECK(merged->card.updated_at == 30);
  }
}

TEST_CASE("merge_card_versions keeps both versions for clashing changes", "[sync][merge]") {
  const auto base = base_card();
  auto local = base;
  auto remote = base;

  SECTION("different edits to the same body line") {
    local.body = "First.\n\nSecond, edited here.\n";
    remote.body = "First.\n\nSecond, edited there.\n";
  }
  SECTION("different edits to neighbouring body lines") {
    local.body = "First, edited here.\n\nSecond.\n";
    remote.body = "First.\nAdded there.\n\nSecond.\n";
  }
  SECTION("different titles") {
    local.card.title = "Local title";
    remote.card.title = "Remote title";
  }
  SECTION("different moves, even when only the position differs") {
    local.card.sort_key = 2;
    remote.card.sort_key = 3;
  }
  SECTION("moving to Trash on one side and editing on the other") {
    local.card.deleted_at = 20;
    remote.body = "Remote.\n";
  }
  SECTION("Trash on both sides at different times") {
    local.card.deleted_at = 20;
    remote.card.deleted_at = 21;
  }
  SECTION("a version whose identity differs") { remote.card.created_at = 11; }
  SECTION("a version without front matter") { remote.has_front_matter = false; }
  CHECK_FALSE(merge_card_versions(base, local, remote).has_value());
}

TEST_CASE("merge_card_versions merges links one by one", "[sync][merge]") {
  auto base = base_card();
  base.links = {link_to("kept"), link_to("changed", "Old label")};
  auto local = base;
  auto remote = base;

  SECTION("additions on both sides are kept, remote order first") {
    local.links.push_back(link_to("local-new"));
    remote.links.push_back(link_to("remote-new"));
    const auto merged = merge_card_versions(base, local, remote);
    REQUIRE(merged.has_value());
    REQUIRE(merged->links.size() == 4);
    CHECK(merged->links[2].to_card_id == "remote-new");
    CHECK(merged->links[3].to_card_id == "local-new");
  }

  SECTION("a removal on one side applies when the other left the link alone") {
    std::erase_if(local.links, [](const auto& link) {
      return link.to_card_id == "kept";
    });
    remote.body = "Remote.\n";
    const auto merged = merge_card_versions(base, local, remote);
    REQUIRE(merged.has_value());
    REQUIRE(merged->links.size() == 1);
    CHECK(merged->links[0].to_card_id == "changed");
  }

  SECTION("a label change on one side applies") {
    remote.links[1].label = "New label";
    local.body = "Local.\n";
    const auto merged = merge_card_versions(base, local, remote);
    REQUIRE(merged.has_value());
    CHECK(merged->links[1].label == "New label");
  }

  SECTION("a removal against a label change keeps both versions") {
    std::erase_if(local.links, [](const auto& link) {
      return link.to_card_id == "changed";
    });
    remote.links[1].label = "New label";
    CHECK_FALSE(merge_card_versions(base, local, remote).has_value());
  }

  SECTION("the same link added twice with different labels keeps both versions") {
    local.links.push_back(link_to("new", "Local"));
    remote.links.push_back(link_to("new", "Remote"));
    CHECK_FALSE(merge_card_versions(base, local, remote).has_value());
  }

  SECTION("a version listing one link twice keeps both versions") {
    local.links.push_back(link_to("kept"));
    CHECK_FALSE(merge_card_versions(base, local, remote).has_value());
  }
}

TEST_CASE("merge_card_versions merges milestones one by one", "[sync][merge]") {
  auto base = base_card();
  base.milestones = {milestone("m1", "Draft"), milestone("m2", "Review")};
  auto local = base;
  auto remote = base;

  SECTION("an addition on one side and an edit on the other") {
    local.milestones.push_back(milestone("m3", "Publish"));
    remote.milestones[1].description = "Final review";
    remote.milestones[1].updated_at = 20;
    const auto merged = merge_card_versions(base, local, remote);
    REQUIRE(merged.has_value());
    REQUIRE(merged->milestones.size() == 3);
    CHECK(merged->milestones[1].description == "Final review");
    CHECK(merged->milestones[2].milestone_id == "m3");
  }

  SECTION("a date added on one side applies") {
    local.milestones[0].dates.push_back({"due", "2026-12-24", std::nullopt});
    const auto merged = merge_card_versions(base, local, remote);
    REQUIRE(merged.has_value());
    REQUIRE(merged->milestones[0].dates.size() == 1);
    CHECK(merged->milestones[0].dates[0].date == "2026-12-24");
  }

  SECTION("different edits to one milestone keep both versions") {
    local.milestones[0].description = "Local";
    remote.milestones[0].description = "Remote";
    CHECK_FALSE(merge_card_versions(base, local, remote).has_value());
  }

  SECTION("a removal against an edit keeps both versions") {
    local.milestones.erase(local.milestones.begin());
    remote.milestones[0].description = "Remote";
    CHECK_FALSE(merge_card_versions(base, local, remote).has_value());
  }
}

TEST_CASE("merge_card_versions merges body edits to different lines", "[sync][merge]") {
  auto base = base_card();
  base.body = "Intro.\n\nFirst.\n\nSecond.\n\nThird.\n";
  auto local = base;
  auto remote = base;
  local.body = "Intro.\n\nFirst, edited here.\n\nSecond.\n\nThird.\n";
  remote.body = "Intro.\n\nFirst.\n\nSecond.\n\nThird, edited there.\n\nFourth, added there.\n";
  local.card.title = "Renamed here";
  local.card.updated_at = 30;
  remote.card.updated_at = 20;

  const auto merged = merge_card_versions(base, local, remote);
  REQUIRE(merged.has_value());
  CHECK(
      merged->body ==
      "Intro.\n\nFirst, edited here.\n\nSecond.\n\nThird, edited there.\n\nFourth, added there.\n"
  );
  CHECK(merged->card.title == "Renamed here");
  CHECK(merged->card.updated_at == 30);
}

TEST_CASE("merge_body_lines merges as git merges a file", "[sync][merge]") {
  SECTION("a removal on one side and an edit elsewhere on the other") {
    CHECK(merge_body_lines("a\nb\nc\nd\n", "b\nc\nd\n", "a\nb\nc\nD\n") == "b\nc\nD\n");
  }
  SECTION("CRLF line endings are kept") {
    CHECK(
        merge_body_lines(
            "1\r\n2\r\n3\r\n4\r\n",
            "1!\r\n2\r\n3\r\n4\r\n",
            "1\r\n2\r\n3\r\n4!\r\n"
        ) == "1!\r\n2\r\n3\r\n4!\r\n"
    );
  }
  SECTION("a last line without a newline") {
    CHECK(merge_body_lines("a\nb\nc", "A\nb\nc", "a\nb\nC") == "A\nb\nC");
  }
  SECTION("a body written from empty on both sides cannot be merged") {
    CHECK_FALSE(merge_body_lines("", "Here.\n", "There.\n").has_value());
  }
  SECTION("text with a NUL byte is not merged") {
    const std::string base("a\nb\nc\n\0\n", 8);
    CHECK_FALSE(
        merge_body_lines(base, std::string("A\nb\nc\n\0\n", 8), std::string("a\nb\nC\n\0\n", 8))
            .has_value()
    );
  }
}
