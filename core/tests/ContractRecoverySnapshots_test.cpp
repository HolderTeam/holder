// Recovery snapshots: reading a project's cards a page at a time, and restoring the lines read
// into a new project with the card tree, links, milestones and resources kept.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "card/CardRepo.h"
#include "ai/AiThreadRepo.h"
#include "ai/AiMessageRepo.h"
#include "card/CardPaths.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"
#include "resource/LocationStore.h"
#include "resource/ResourceStore.h"

#include <git2.h>
#include <holder/holder_api.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <filesystem>
#include <map>
#include <memory>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace {

namespace contract = holder::contract;
using nlohmann::json;

// Git that fails a commit with a given message, to fail a restore after its project is made.
class Git final : public holder::git::GitOps {
 public:
  std::string fail_commit;

  void open_or_init(const std::filesystem::path& dir) override { real_.open_or_init(dir); }
  void write_file(const std::filesystem::path& path, const std::string& content) override {
    real_.write_file(path, content);
  }
  void stage_path(const std::filesystem::path& path) override { real_.stage_path(path); }
  void stage_paths(const std::vector<std::filesystem::path>& paths) override { real_.stage_paths(paths); }
  void remove_path(const std::filesystem::path& path) override { real_.remove_path(path); }
  void commit(const std::string& message) override {
    if (message == fail_commit) throw std::runtime_error("commit failed");
    real_.commit(message);
  }
  void set_remote(const std::string& name, const std::string& url) override { real_.set_remote(name, url); }
  void remove_remote(const std::string& name) override { real_.remove_remote(name); }
  void pull_remote_ff_only(const std::string& name) override { real_.pull_remote_ff_only(name); }
  holder::git::RemoteProbeResult probe_remote(const std::string& name) override { return real_.probe_remote(name); }
  holder::git::PushResult push_branch(const std::string& name, const std::string& branch, bool upstream) override {
    return real_.push_branch(name, branch, upstream);
  }
  std::filesystem::path repo_dir() const override { return real_.repo_dir(); }

 private:
  holder::git::RealGitOps real_;
};

struct Host {
  std::filesystem::path dir = holder::test::make_temp_dir();
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  Git git;
  holder_context context{db, fts, &git};
  std::string project_id;

  Host() {
    context.projects_root = dir / "projects";
    context.registry_path = dir / "projects.json";
    project_id = make_project("Source").project_id;
  }

  contract::Project make_project(const std::string& name) {
    contract::NewProject request;
    request.name = name;
    request.privacy_mode = contract::NewProjectPrivacyMode::Plain;
    return contract::create_project(context, request);
  }

  std::string card(const std::string& title, std::optional<std::string> parent = std::nullopt,
                   std::optional<double> sort_key = std::nullopt, const std::string& content = "body") {
    contract::NewCard request;
    request.title = title;
    request.content = content;
    request.parent_card_id = std::move(parent);
    request.sort_key = sort_key;
    return contract::create_card(context, project_id, request).card_id;
  }

  void link(const std::string& from, contract::NewLinkTargetType type, const std::string& to,
            const std::string& kind, std::optional<std::string> target_project = std::nullopt) {
    contract::NewLink request;
    request.target_type = type;
    request.target_id = to;
    request.kind = kind;
    request.label = kind + " label";
    request.target_project_id = std::move(target_project);
    contract::add_card_link(context, project_id, from, request);
  }

  contract::RecoverySnapshotPage read(const std::string& project, std::optional<std::string> cursor = std::nullopt,
                                      std::int32_t limit = 500) {
    return contract::read_recovery_snapshot(context, project, std::move(cursor), limit);
  }

  // Every line of the project's snapshot, read a page at a time.
  std::vector<std::string> lines(const std::string& project, std::int32_t limit = 500) {
    std::vector<std::string> out;
    std::optional<std::string> cursor;
    do {
      auto page = read(project, cursor, limit);
      out.insert(out.end(), page.lines.begin(), page.lines.end());
      cursor = page.next_cursor;
    } while (cursor);
    return out;
  }

  contract::Project restore(const std::vector<std::string>& lines, const std::string& name = "Restored") {
    contract::RecoverySnapshotRestore request;
    request.project_name = name;
    request.privacy_mode = contract::RecoverySnapshotRestorePrivacyMode::Plain;
    for (const auto& line : lines) request.snapshot += line + "\n";
    request.commit_message = "Restored from a recovery snapshot";
    return contract::restore_recovery_snapshot(context, request);
  }

  // The project's cards by title.
  std::map<std::string, contract::RecoverySnapshotCard> by_title(const std::string& project) {
    std::map<std::string, contract::RecoverySnapshotCard> out;
    for (auto& card : read(project).cards) out.emplace(card.title, std::move(card));
    return out;
  }

  std::size_t project_count() {
    return contract::list_projects(context, std::nullopt, std::nullopt, std::nullopt, std::nullopt, 1000, 0, false)
        .size();
  }
};

int count_commits(const std::filesystem::path& repo_dir) {
  git_libgit2_init();
  git_repository* repo = nullptr;
  REQUIRE(git_repository_open(&repo, repo_dir.string().c_str()) == 0);
  git_revwalk* walk = nullptr;
  REQUIRE(git_revwalk_new(&walk, repo) == 0);
  int count = 0;
  if (git_revwalk_push_head(walk) == 0) {
    git_oid oid{};
    while (git_revwalk_next(&oid, walk) == 0) ++count;
  }
  git_revwalk_free(walk);
  git_repository_free(repo);
  git_libgit2_shutdown();
  return count;
}

// A line's card, to change before restoring it.
json card_of(const std::string& line) { return json::parse(line).at("card"); }

std::string line_of(const json& card) {
  return json{{"format", "holder-recovery-snapshot"}, {"version", 1}, {"card", card}}.dump();
}

json bare_card(const std::string& card_id, const std::string& title, json parent = nullptr, double sort_key = 1) {
  return {{"card_id", card_id}, {"project_id", "source"},  {"project_name", "Source"},
          {"privacy_mode", "plain"}, {"title", title},     {"content", title + " body"},
          {"parent_card_id", parent}, {"sort_key", sort_key}, {"created_at", 1},
          {"updated_at", 1},         {"links", json::array()}, {"milestones", json::array()},
          {"resources", json::array()}, {"locations", json::array()}};
}

} // namespace

TEST_CASE("A recovery snapshot pages a project's live cards, most recently updated first", "[recovery]") {
  Host host;
  const auto oldest = host.card("Oldest");
  const auto middle = host.card("Middle");
  const auto newest = host.card("Newest");
  const auto trashed = host.card("Trashed");
  holder::card::CardRepo cards(host.db);
  cards.touch_updated(oldest, 100);
  cards.touch_updated(middle, 200);
  cards.touch_updated(newest, 300);
  contract::trash_card(host.context, host.project_id, trashed, contract::TrashCardChildren::Promote);

  const auto first = host.read(host.project_id, std::nullopt, 2);
  REQUIRE(first.cards.size() == 2);
  REQUIRE(first.lines.size() == 2);
  REQUIRE(first.cards[0].title == "Newest");
  REQUIRE(first.cards[1].title == "Middle");
  REQUIRE(first.cards[0].content == "body");
  REQUIRE(first.cards[0].project_id == host.project_id);
  REQUIRE(first.cards[0].project_name == "Source");
  REQUIRE(first.cards[0].privacy_mode == contract::RecoverySnapshotCardPrivacyMode::Plain);
  REQUIRE(card_of(first.lines[0]).at("card_id") == newest);
  REQUIRE(first.next_cursor);

  const auto second = host.read(host.project_id, first.next_cursor, 2);
  REQUIRE(second.cards.size() == 1);
  REQUIRE(second.cards[0].title == "Oldest");
  REQUIRE_FALSE(second.next_cursor);

  // A cursor continues only the snapshot of the project it came from.
  const auto other = host.make_project("Other").project_id;
  REQUIRE_THROWS_AS(host.read(other, first.next_cursor, 2), contract::InvalidArgument);
  REQUIRE_THROWS_AS(host.read(host.project_id, std::string("zz"), 2), contract::InvalidArgument);
  REQUIRE_THROWS_AS(host.read("missing-project"), contract::NotFound);
}

TEST_CASE("Restoring a recovery snapshot keeps the card tree, links, milestones and resources", "[recovery]") {
  Host host;
  const auto root = host.card("Root", std::nullopt, 1.0);
  const auto second_root = host.card("Second root", std::nullopt, 2.0);
  const auto child_b = host.card("Child B", root, 20.0);
  const auto child_a = host.card("Child A", root, 10.0);
  const auto grandchild = host.card("Grandchild", child_a, 5.0, "![notes](holder://resource/resource-original)");

  holder::model::Location location{"location-original", host.project_id, "Files", "local_directory", {{"path", "/files"}},
                                   10, 10};
  holder::resource::LocationStore(host.db, nullptr, &host.git).put(location);
  holder::model::ResourceBundle bundle;
  bundle.resource = {"resource-original", host.project_id, "document", "Notes.pdf", {{"pages", {"4"}}}, 10, 10};
  bundle.assets.push_back({"asset-original", "resource-original", "Notes.pdf", "application/pdf", 4,
                           std::string(64, 'a'), 10, 10,
                           {{"placement-original", "asset-original", "location-original", "remote/notes",
                             "holder_asset_v1", 40, std::string(64, 'b'), 10}}});
  holder::resource::ResourceStore(host.db, nullptr, &host.git).put(bundle);

  host.link(grandchild, contract::NewLinkTargetType::Resource, "resource-original", "attachment");
  host.link(grandchild, contract::NewLinkTargetType::Card, second_root, "related");
  host.link(grandchild, contract::NewLinkTargetType::Card, child_b, "elsewhere", "project-2");
  host.link(grandchild, contract::NewLinkTargetType::Card, "absent-card", "offline", "project-3");
  contract::NewMilestone milestone;
  milestone.dates = {{"due", "2026-11-02", std::nullopt}};
  milestone.kind = "Deadline";
  milestone.description = "ship it";
  contract::add_card_milestone(host.context, host.project_id, grandchild, milestone);

  const auto source = host.by_title(host.project_id);
  const auto& read_grandchild = source.at("Grandchild");
  REQUIRE(read_grandchild.parent_card_id == child_a);
  REQUIRE(read_grandchild.sort_key == 5.0);
  REQUIRE(read_grandchild.links.size() == 4);
  REQUIRE(read_grandchild.resources.size() == 1);
  REQUIRE(read_grandchild.resources[0].assets[0].placements[0].object_key == "remote/notes");
  REQUIRE(read_grandchild.resources[0].assets[0].placements[0].encoding ==
          contract::RecoverySnapshotPlacementEncoding::HolderAssetV1);
  REQUIRE(read_grandchild.locations.size() == 1);
  REQUIRE(read_grandchild.locations[0].configuration.at("path") == "/files");
  REQUIRE(read_grandchild.milestones.size() == 1);

  // Children's lines come before their parents', as a page newest first can put them.
  auto lines = host.lines(host.project_id, 2);
  REQUIRE(lines.size() == 5);
  std::reverse(lines.begin(), lines.end());
  const auto restored = host.restore(lines);
  REQUIRE(restored.project_id != host.project_id);
  REQUIRE(restored.name == "Restored");
  REQUIRE(restored.privacy_mode == contract::ProjectPrivacyMode::Plain);
  // The project's own commit, one each for the storage location and the resource, as their stores
  // make, and one for every card.
  REQUIRE(count_commits(restored.root_path) == 4);

  const auto cards = host.by_title(restored.project_id);
  REQUIRE(cards.size() == 5);
  for (const auto& [title, card] : cards) {
    REQUIRE(card.card_id != source.at(title).card_id);
    REQUIRE(card.sort_key == source.at(title).sort_key);
    REQUIRE(card.created_at == source.at(title).created_at);
  }
  REQUIRE_FALSE(cards.at("Root").parent_card_id);
  REQUIRE_FALSE(cards.at("Second root").parent_card_id);
  REQUIRE(cards.at("Child A").parent_card_id == cards.at("Root").card_id);
  REQUIRE(cards.at("Child B").parent_card_id == cards.at("Root").card_id);
  const auto& copy = cards.at("Grandchild");
  REQUIRE(copy.parent_card_id == cards.at("Child A").card_id);

  // The card tree as list_cards sees it, siblings in order.
  const auto children = contract::list_cards(host.context, restored.project_id,
                                             contract::ListCardsParent{cards.at("Root").card_id}, std::nullopt,
                                             contract::ListCardsOrder::SortKeyAsc, 10, std::nullopt, false, false);
  REQUIRE(children.items.size() == 2);
  REQUIRE(children.items[0].title == "Child A");
  REQUIRE(children.items[1].title == "Child B");

  std::map<std::string, contract::RecoverySnapshotLink> links;
  for (const auto& link : copy.links) links.emplace(link.kind, link);
  REQUIRE(links.size() == 4);
  REQUIRE(links.at("related").target_id == cards.at("Second root").card_id);
  REQUIRE(links.at("related").label == "related label");
  REQUIRE_FALSE(links.at("related").target_project_id);
  REQUIRE(links.at("elsewhere").target_id == child_b);
  REQUIRE(links.at("elsewhere").target_project_id == "project-2");
  REQUIRE(links.at("offline").target_id == "absent-card");
  REQUIRE(links.at("offline").target_project_id == "project-3");

  const auto& resource_id = links.at("attachment").target_id;
  REQUIRE(resource_id != "resource-original");
  REQUIRE(copy.content == "![notes](holder://resource/" + resource_id + ")");
  REQUIRE(copy.resources.size() == 1);
  const auto& resource = copy.resources[0];
  REQUIRE(resource.resource_id == resource_id);
  REQUIRE(resource.label == "Notes.pdf");
  REQUIRE(resource.metadata.at("pages") == std::vector<std::string>{"4"});
  REQUIRE(resource.assets[0].asset_id != "asset-original");
  const auto& placement = resource.assets[0].placements[0];
  REQUIRE(placement.placement_id != "placement-original");
  REQUIRE(placement.object_key == "remote/notes");
  REQUIRE(placement.stored_sha256 == std::string(64, 'b'));
  REQUIRE(copy.locations.size() == 1);
  REQUIRE(copy.locations[0].location_id == placement.location_id);
  REQUIRE(placement.location_id != "location-original");
  REQUIRE(copy.locations[0].name == "Files");

  REQUIRE(copy.milestones.size() == 1);
  REQUIRE(copy.milestones[0].kind == "Deadline");
  REQUIRE(copy.milestones[0].description == "ship it");
  REQUIRE(copy.milestones[0].dates[0].date == "2026-11-02");
}

TEST_CASE("Restore makes a card whose parent is not in the snapshot a root, and breaks a parent loop",
          "[recovery]") {
  Host host;
  const auto restored = host.restore({
      line_of(bare_card("orphan", "Orphan", "not-restored")),
      line_of(bare_card("loop-a", "Loop A", "loop-b")),
      line_of(bare_card("loop-b", "Loop B", "loop-c")),
      line_of(bare_card("loop-c", "Loop C", "loop-a")),
      line_of(bare_card("self", "Self", "self")),
  });
  const auto cards = host.by_title(restored.project_id);
  REQUIRE(cards.size() == 5);
  REQUIRE_FALSE(cards.at("Orphan").parent_card_id);
  REQUIRE_FALSE(cards.at("Self").parent_card_id);
  // Exactly one card of the loop becomes a root; the other two keep their parents.
  int roots = 0;
  for (const char* title : {"Loop A", "Loop B", "Loop C"}) roots += cards.at(title).parent_card_id ? 0 : 1;
  REQUIRE(roots == 1);
}

TEST_CASE("Restore refuses a snapshot it cannot read and leaves no project behind", "[recovery]") {
  Host host;
  const auto before = host.project_count();
  const auto good = line_of(bare_card("card-1", "Card"));

  auto future = json::parse(good);
  future["version"] = 2;
  auto other_format = json::parse(good);
  other_format["format"] = "something-else";
  auto missing_field = bare_card("card-1", "Card");
  missing_field.erase("sort_key");
  auto bad_date = bare_card("card-1", "Card");
  bad_date["milestones"] = json::array({{{"dates", json::array({{{"role", "due"}, {"date", "2026-13-40"}}})},
                                         {"kind", nullptr},
                                         {"description", nullptr}}});
  auto stray_placement = bare_card("card-1", "Card");
  stray_placement["resources"] = json::array(
      {{{"resource_id", "resource-1"},
        {"type", "document"},
        {"label", "Doc"},
        {"metadata", json::object()},
        {"created_at", 1},
        {"updated_at", 1},
        {"assets", json::array({{{"asset_id", "asset-1"},
                                 {"original_filename", "doc"},
                                 {"media_type", "text/plain"},
                                 {"byte_size", 1},
                                 {"plaintext_sha256", "aa"},
                                 {"created_at", 1},
                                 {"updated_at", 1},
                                 {"placements", json::array({{{"placement_id", "placement-1"},
                                                              {"location_id", "not-in-snapshot"},
                                                              {"object_key", "key"},
                                                              {"encoding", "plain"},
                                                              {"stored_byte_size", 1},
                                                              {"stored_sha256", "aa"},
                                                              {"created_at", 1}}})}}})}}});

  for (const auto& lines : std::vector<std::vector<std::string>>{
           {"not json"},
           {future.dump()},
           {other_format.dump()},
           {line_of(missing_field)},
           {line_of(bad_date)},
           {line_of(stray_placement)},
           {good, good},
       }) {
    REQUIRE_THROWS_AS(host.restore(lines), contract::InvalidArgument);
  }
  REQUIRE(host.project_count() == before);
}

TEST_CASE("Restore removes the new project when writing the cards fails", "[recovery]") {
  Host host;
  const auto before = host.project_count();
  host.git.fail_commit = "Restored from a recovery snapshot";
  REQUIRE_THROWS(host.restore({line_of(bare_card("card-1", "Card"))}));
  REQUIRE(host.project_count() == before);
  // Nothing is left under the projects directory but the source project.
  REQUIRE(std::distance(std::filesystem::directory_iterator(host.context.projects_root),
                        std::filesystem::directory_iterator()) == 1);
}

TEST_CASE("Restore writes a large snapshot as one commit", "[recovery]") {
  Host host;
  std::vector<std::string> lines;
  for (int i = 0; i < 2000; ++i) {
    // Every tenth card is a root; the rest are its children.
    const auto parent = i % 10 == 0 ? json(nullptr) : json("card-" + std::to_string(i - i % 10));
    lines.push_back(line_of(bare_card("card-" + std::to_string(i), "Card " + std::to_string(i), parent, i)));
  }
  const auto restored = host.restore(lines);
  REQUIRE(host.lines(restored.project_id).size() == 2000);
  REQUIRE(count_commits(restored.root_path) == 2);
}

TEST_CASE(
    "Generated recovery snapshots preserve nested records through paging and restore",
    "[capi][recovery]"
) {
  Host host;
  const auto root = host.card("Parent");
  const auto child = host.card("Child", root, 7.0, "![notes](holder://resource/resource-original)");
  const auto trashed = host.card("Trashed");
  contract::trash_card(
      host.context,
      host.project_id,
      trashed,
      contract::TrashCardChildren::Promote
  );
  holder::resource::LocationStore(host.db, nullptr, &host.git)
      .put(
          {"location-original",
           host.project_id,
           "Files",
           "local_directory",
           {{"path", "/files"}},
           10,
           20}
      );
  holder::model::ResourceBundle bundle;
  bundle.resource = {
      "resource-original",
      host.project_id,
      "document",
      "Notes.pdf",
      {{"pages", {"4", "5"}}},
      10,
      20
  };
  bundle.assets.push_back(
      {"asset-original",
       "resource-original",
       "Notes.pdf",
       "application/pdf",
       4,
       std::string(64, 'a'),
       10,
       20,
       {{"placement-original",
         "asset-original",
         "location-original",
         "remote/notes",
         "holder_asset_v1",
         40,
         std::string(64, 'b'),
         10}}}
  );
  holder::resource::ResourceStore(host.db, nullptr, &host.git).put(bundle);
  host.link(child, contract::NewLinkTargetType::Resource, "resource-original", "attachment");
  host.link(child, contract::NewLinkTargetType::Card, root, "related");
  contract::NewMilestone milestone;
  milestone.dates = {{"due", "2026-11-02", std::nullopt}};
  milestone.kind = "Deadline";
  milestone.description = "ship it";
  contract::add_card_milestone(host.context, host.project_id, child, milestone);

  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  holder_error* error = nullptr;
  std::string cursor;
  std::string snapshot;
  const int32_t limit = 1;
  const holder_recovery_snapshot_card* read_child = nullptr;
  std::set<std::string> seen;
  do {
    const holder_recovery_snapshot_page* page = nullptr;
    REQUIRE(
        holder_read_recovery_snapshot(
            &host.context,
            host.project_id.c_str(),
            cursor.empty() ? nullptr : cursor.c_str(),
            &limit,
            arena.get(),
            &page,
            &error
        ) == HOLDER_OK
    );
    REQUIRE(error == nullptr);
    REQUIRE(page->cards.count == 1);
    REQUIRE(page->lines.count == 1);
    const auto& card = page->cards.items[0];
    REQUIRE(seen.insert(card.card_id).second);
    REQUIRE(std::string(card.project_id) == host.project_id);
    REQUIRE(std::string(card.project_name) == "Source");
    REQUIRE(card.privacy_mode == HOLDER_RECOVERY_SNAPSHOT_CARD_PRIVACY_MODE_PLAIN);
    REQUIRE(json::parse(page->lines.items[0])["card"]["card_id"] == card.card_id);
    snapshot += std::string(page->lines.items[0]) + "\n";
    if (card.card_id == child) read_child = &card;
    cursor = page->next_cursor ? page->next_cursor : "";
    REQUIRE(seen.size() <= 2);
  } while (!cursor.empty());
  REQUIRE(seen == std::set<std::string>{root, child});
  REQUIRE(read_child != nullptr);
  REQUIRE(std::string(read_child->parent_card_id) == root);
  REQUIRE(read_child->sort_key == 7.0);
  REQUIRE(read_child->links.count == 2);
  REQUIRE(read_child->milestones.count == 1);
  const auto& due = read_child->milestones.items[0];
  REQUIRE(std::string(due.kind) == "Deadline");
  REQUIRE(std::string(due.description) == "ship it");
  REQUIRE(due.dates.count == 1);
  REQUIRE(std::string(due.dates.items[0].date) == "2026-11-02");
  REQUIRE(read_child->locations.count == 1);
  const auto& location = read_child->locations.items[0];
  REQUIRE(location.provider == HOLDER_RECOVERY_SNAPSHOT_LOCATION_PROVIDER_LOCAL_DIRECTORY);
  REQUIRE(location.configuration.count == 1);
  REQUIRE(std::string(location.configuration.items[0].key) == "path");
  REQUIRE(std::string(location.configuration.items[0].values) == "/files");
  REQUIRE(read_child->resources.count == 1);
  const auto& resource = read_child->resources.items[0];
  REQUIRE(std::string(resource.resource_id) == "resource-original");
  REQUIRE(resource.metadata.count == 1);
  REQUIRE(resource.metadata.items[0].values.count == 2);
  REQUIRE(std::string(resource.metadata.items[0].values.items[1]) == "5");
  REQUIRE(resource.assets.count == 1);
  const auto& asset = resource.assets.items[0];
  REQUIRE(std::string(asset.original_filename) == "Notes.pdf");
  REQUIRE(asset.byte_size == 4);
  REQUIRE(asset.placements.count == 1);
  REQUIRE(
      asset.placements.items[0].encoding ==
      HOLDER_RECOVERY_SNAPSHOT_PLACEMENT_ENCODING_HOLDER_ASSET_V1
  );
  REQUIRE(asset.placements.items[0].stored_byte_size == 40);
  REQUIRE(std::string(asset.placements.items[0].stored_sha256) == std::string(64, 'b'));

  holder_recovery_snapshot_restore request{};
  request.project_name = "Restored through C";
  request.privacy_mode = HOLDER_RECOVERY_SNAPSHOT_RESTORE_PRIVACY_MODE_PLAIN;
  request.snapshot = snapshot.c_str();
  request.commit_message = "Recover saved cards";
  const holder_project* restored = nullptr;
  REQUIRE(
      holder_restore_recovery_snapshot(&host.context, &request, arena.get(), &restored, &error) ==
      HOLDER_OK
  );
  REQUIRE(error == nullptr);
  REQUIRE(std::string(restored->project_id) != host.project_id);
  REQUIRE(std::string(restored->name) == "Restored through C");
  const auto cards = host.by_title(restored->project_id);
  REQUIRE(cards.size() == 2);
  const auto& copy = cards.at("Child");
  REQUIRE(copy.card_id != child);
  REQUIRE(copy.parent_card_id == cards.at("Parent").card_id);
  REQUIRE(copy.resources.size() == 1);
  REQUIRE(copy.resources[0].resource_id != "resource-original");
  REQUIRE(copy.content == "![notes](holder://resource/" + copy.resources[0].resource_id + ")");
  REQUIRE(copy.resources[0].assets[0].placements[0].location_id == copy.locations[0].location_id);
  REQUIRE(copy.milestones[0].description == "ship it");
  // Original arena-backed nested records survive subsequent calls.
  REQUIRE(std::string(asset.original_filename) == "Notes.pdf");
}

TEST_CASE(
    "Generated recovery rejects corrupt snapshots without creating projects",
    "[capi][recovery]"
) {
  Host host;
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  auto record = json::parse(line_of(bare_card("card-1", "Card")));
  SECTION("future version") { record["version"] = 2; }
  SECTION("wrong format") { record["format"] = "other"; }
  SECTION("missing required field") { record["card"].erase("sort_key"); }
  SECTION("unknown privacy mode") { record["card"]["privacy_mode"] = "unknown"; }
  SECTION("card is not an object") { record["card"] = json::array(); }
  SECTION("nonstring title") { record["card"]["title"] = 12; }
  SECTION("empty identity") { record["card"]["card_id"] = ""; }
  SECTION("empty parent") { record["card"]["parent_card_id"] = ""; }
  SECTION("fractional timestamp") { record["card"]["created_at"] = 1.5; }
  SECTION("overflowed timestamp") {
    record["card"]["updated_at"] = std::numeric_limits<unsigned long long>::max();
  }
  SECTION("invalid sort key") { record["card"]["sort_key"] = "first"; }
  SECTION("links are not an array") { record["card"]["links"] = json::object(); }
  SECTION("unsupported link type") {
    record["card"]["links"] = json::array({{{"target_type", "unknown"}}});
  }
  SECTION("unsupported location provider") {
    record["card"]["locations"] = json::array(
        {{{"location_id", "location-1"}, {"name", "Files"}, {"provider", "unknown"}}}
    );
  }
  SECTION("location configuration is not an object") {
    record["card"]["locations"] = json::array(
        {{{"location_id", "location-1"},
          {"name", "Files"},
          {"provider", "local_directory"},
          {"configuration", 3}}}
    );
  }
  SECTION("location configuration values are not strings") {
    record["card"]["locations"] = json::array(
        {{{"location_id", "location-1"},
          {"name", "Files"},
          {"provider", "local_directory"},
          {"configuration", {{"path", 3}}}}}
    );
  }
  SECTION("resource metadata is not an object") {
    record["card"]["resources"] = json::array(
        {{{"resource_id", "resource-1"}, {"type", "document"}, {"label", "Notes"}, {"metadata", 3}}}
    );
  }
  SECTION("resource metadata values are not arrays") {
    record["card"]["resources"] = json::array(
        {{{"resource_id", "resource-1"},
          {"type", "document"},
          {"label", "Notes"},
          {"metadata", {{"pages", "4"}}}}}
    );
  }
  SECTION("resource metadata array contains a nonstring") {
    record["card"]["resources"] = json::array(
        {{{"resource_id", "resource-1"},
          {"type", "document"},
          {"label", "Notes"},
          {"metadata", {{"pages", {4}}}}}}
    );
  }
  // A valid first record must not be partially restored when a later record is corrupt.
  const auto text = line_of(bare_card("valid-card", "Valid first record")) + "\n" + record.dump();
  holder_recovery_snapshot_restore request{};
  request.project_name = "Must not exist";
  request.privacy_mode = HOLDER_RECOVERY_SNAPSHOT_RESTORE_PRIVACY_MODE_PLAIN;
  request.snapshot = text.c_str();
  request.commit_message = "Rejected restore";
  const auto before = host.project_count();
  const holder_project* restored = nullptr;
  holder_error* error = nullptr;
  const auto status =
      holder_restore_recovery_snapshot(&host.context, &request, arena.get(), &restored, &error);
  const std::unique_ptr<holder_error, decltype(&holder_error_destroy)> owned_error(
      error,
      holder_error_destroy
  );
  REQUIRE(status == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(error != nullptr);
  REQUIRE(std::string(holder_error_message(error)).size() > 0);
  REQUIRE(restored == nullptr);
  REQUIRE(host.project_count() == before);
}

TEST_CASE(
    "Generated encrypted recovery snapshots restore readable cards and drop unrestorable AI links",
    "[capi][recovery-variants]"
) {
  Host host;
  holder::test::EnvGuard keystore("HOLDER_TEST_KEYSTORE_DIR", (host.dir / "keystore").string());
  contract::NewProject project_request;
  project_request.name = "Encrypted source";
  project_request.privacy_mode = contract::NewProjectPrivacyMode::EncryptedGit;
  const auto source = contract::create_project(host.context, project_request);
  host.project_id = source.project_id;
  const auto card =
      host.card("Private notes", std::nullopt, std::nullopt, "Private recovery body — café");
  holder::model::AiThread thread;
  thread.thread_id = "snapshot-thread";
  thread.project_id = source.project_id;
  thread.title = "Conversation";
  thread.created_at = thread.updated_at = 10;
  holder::ai::AiThreadRepo(host.db).create(thread);
  holder::model::AiMessage message;
  message.message_id = "snapshot-message";
  message.thread_id = thread.thread_id;
  message.role = "user";
  message.source = "manual";
  message.content = "Message";
  message.created_at = 11;
  holder::ai::AiMessageRepo(host.db, &host.fts).append(message);
  host.link(card, contract::NewLinkTargetType::AiThread, thread.thread_id, "conversation");
  host.link(card, contract::NewLinkTargetType::AiMessage, message.message_id, "citation");
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  holder_error* error = nullptr;
  const holder_recovery_snapshot_page* page = nullptr;
  REQUIRE(
      holder_read_recovery_snapshot(
          &host.context,
          source.project_id.c_str(),
          nullptr,
          nullptr,
          arena.get(),
          &page,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(error == nullptr);
  REQUIRE(page->cards.count == 1);
  const auto& saved = page->cards.items[0];
  REQUIRE(saved.privacy_mode == HOLDER_RECOVERY_SNAPSHOT_CARD_PRIVACY_MODE_ENCRYPTED_GIT);
  REQUIRE(std::string(saved.content) == "Private recovery body — café");
  REQUIRE(saved.links.count == 2);
  std::set<holder_recovery_snapshot_link_target_type> kinds;
  for (size_t i = 0; i < saved.links.count; ++i)
    kinds.insert(saved.links.items[i].target_type);
  REQUIRE(
      kinds == std::set<holder_recovery_snapshot_link_target_type>{
                   HOLDER_RECOVERY_SNAPSHOT_LINK_TARGET_TYPE_AI_THREAD,
                   HOLDER_RECOVERY_SNAPSHOT_LINK_TARGET_TYPE_AI_MESSAGE
               }
  );
  REQUIRE(page->lines.count == 1);
  REQUIRE(json::parse(page->lines.items[0])["card"]["privacy_mode"] == "encrypted_git");
  holder_recovery_snapshot_restore restore{};
  restore.project_name = "Recovered encrypted notes";
  restore.privacy_mode = HOLDER_RECOVERY_SNAPSHOT_RESTORE_PRIVACY_MODE_ENCRYPTED_GIT;
  restore.snapshot = page->lines.items[0];
  restore.commit_message = "Recover private notes";
  const holder_project* restored = nullptr;
  REQUIRE(
      holder_restore_recovery_snapshot(&host.context, &restore, arena.get(), &restored, &error) ==
      HOLDER_OK
  );
  REQUIRE(restored->privacy_mode == HOLDER_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT);
  REQUIRE(std::string(restored->project_id) != source.project_id);
  const auto cards = host.by_title(restored->project_id);
  REQUIRE(cards.size() == 1);
  const auto& copy = cards.at("Private notes");
  REQUIRE(copy.card_id != card);
  REQUIRE(copy.content == "Private recovery body — café");
  REQUIRE(copy.links.empty());
  const auto path = std::filesystem::path(restored->root_path) /
                    holder::core::card_rel_path(copy.card_id);
  std::ifstream file(path, std::ios::binary);
  REQUIRE(file.is_open());
  const std::string stored{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
  REQUIRE(stored.find("Private recovery body") == std::string::npos);
  REQUIRE(stored.find("Private notes") == std::string::npos);
}

TEST_CASE(
    "Generated recovery preserves cloud declarations and plain asset placements without network access",
    "[capi][recovery-variants]"
) {
  Host host;
  std::string provider;
  holder_recovery_snapshot_location_provider expected{};
  SECTION("S3 compatible") {
    provider = "s3_compatible";
    expected = HOLDER_RECOVERY_SNAPSHOT_LOCATION_PROVIDER_S3_COMPATIBLE;
  }
  SECTION("Google Drive") {
    provider = "google-drive";
    expected = HOLDER_RECOVERY_SNAPSHOT_LOCATION_PROVIDER_GOOGLE_DRIVE;
  }
  const auto card = host.card("Attachment");
  holder::resource::LocationStore(host.db, nullptr, &host.git)
      .put(
          {"portable-location",
           host.project_id,
           "Cloud files",
           provider,
           {{"folder", "portable-folder"}},
           10,
           20}
      );
  holder::model::ResourceBundle bundle;
  bundle.resource = {"portable-resource", host.project_id, "document", "Notes.txt", {}, 10, 20};
  bundle.assets.push_back(
      {"portable-asset",
       "portable-resource",
       "Notes.txt",
       "text/plain",
       4,
       std::string(64, 'a'),
       10,
       20,
       {{"portable-placement",
         "portable-asset",
         "portable-location",
         "object/notes",
         "plain",
         4,
         std::string(64, 'a'),
         10}}}
  );
  holder::resource::ResourceStore(host.db, nullptr, &host.git).put(bundle);
  host.link(card, contract::NewLinkTargetType::Resource, "portable-resource", "attachment");
  contract::NewMilestone milestone;
  milestone.dates = {{"due", std::nullopt, std::string("2026-11-02T12:30:00Z")}};
  contract::add_card_milestone(host.context, host.project_id, card, milestone);
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  holder_error* error = nullptr;
  const holder_recovery_snapshot_page* page = nullptr;
  REQUIRE(
      holder_read_recovery_snapshot(
          &host.context,
          host.project_id.c_str(),
          nullptr,
          nullptr,
          arena.get(),
          &page,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(error == nullptr);
  REQUIRE(page->cards.count == 1);
  const auto& saved = page->cards.items[0];
  REQUIRE(saved.locations.count == 1);
  REQUIRE(saved.locations.items[0].provider == expected);
  REQUIRE(saved.resources.count == 1);
  REQUIRE(saved.resources.items[0].assets.count == 1);
  const auto& asset = saved.resources.items[0].assets.items[0];
  REQUIRE(asset.placements.count == 1);
  REQUIRE(asset.placements.items[0].encoding == HOLDER_RECOVERY_SNAPSHOT_PLACEMENT_ENCODING_PLAIN);
  REQUIRE(saved.milestones.count == 1);
  REQUIRE(saved.milestones.items[0].dates.count == 1);
  REQUIRE(saved.milestones.items[0].dates.items[0].date == nullptr);
  REQUIRE(std::string(saved.milestones.items[0].dates.items[0].datetime) == "2026-11-02T12:30:00Z");
  holder_recovery_snapshot_restore request{};
  request.project_name = "Portable cloud restore";
  request.privacy_mode = HOLDER_RECOVERY_SNAPSHOT_RESTORE_PRIVACY_MODE_PLAIN;
  request.snapshot = page->lines.items[0];
  request.commit_message = "Restore declarations";
  const holder_project* restored = nullptr;
  REQUIRE(
      holder_restore_recovery_snapshot(&host.context, &request, arena.get(), &restored, &error) ==
      HOLDER_OK
  );
  const auto cards = host.by_title(restored->project_id);
  REQUIRE(cards.size() == 1);
  const auto& copy = cards.at("Attachment");
  REQUIRE(copy.locations.size() == 1);
  REQUIRE(copy.locations[0].configuration.at("folder") == "portable-folder");
  REQUIRE(
      copy.resources[0].assets[0].placements[0].encoding ==
      contract::RecoverySnapshotPlacementEncoding::Plain
  );
  REQUIRE(copy.milestones[0].dates[0].datetime == "2026-11-02T12:30:00Z");
}

TEST_CASE(
    "Generated recovery refuses unknown persisted record types instead of dropping them",
    "[capi][recovery-corruption]"
) {
  Host host;
  const auto card = host.card("Attachment");
  holder::resource::LocationStore(host.db, nullptr, &host.git)
      .put(
          {"location-1", host.project_id, "Files", "local_directory", {{"path", "/files"}}, 10, 10}
      );
  holder::model::ResourceBundle bundle;
  bundle.resource = {"resource-1", host.project_id, "document", "Notes", {}, 10, 10};
  bundle.assets.push_back(
      {"asset-1",
       "resource-1",
       "Notes",
       "text/plain",
       1,
       std::string(64, 'a'),
       10,
       10,
       {{"placement-1", "asset-1", "location-1", "notes", "plain", 1, std::string(64, 'a'), 10}}}
  );
  holder::resource::ResourceStore(host.db, nullptr, &host.git).put(bundle);
  host.link(card, contract::NewLinkTargetType::Resource, "resource-1", "attachment");
  std::string repair;
  SECTION("unknown storage provider") {
    host.db.exec("UPDATE storage_locations SET provider='future-provider';");
    repair = "UPDATE storage_locations SET provider='local_directory';";
  }
  SECTION("unknown asset encoding") {
    host.db.exec("UPDATE asset_placements SET encoding='future-encoding';");
    repair = "UPDATE asset_placements SET encoding='plain';";
  }
  SECTION("unknown link target type") {
    host.db.exec("UPDATE card_links SET to_type='future-target';");
    repair = "UPDATE card_links SET to_type='resource';";
  }
  SECTION("unknown persisted project privacy mode") {
    // Model a database from an unsupported writer: normal writes cannot bypass
    // this CHECK constraint. Readers must reject it rather than export plaintext
    // under a guessed privacy mode.
    host.db.exec("PRAGMA ignore_check_constraints=ON;");
    host.db.exec("UPDATE projects SET privacy_mode='future-privacy';");
    host.db.exec("PRAGMA ignore_check_constraints=OFF;");
    repair = "UPDATE projects SET privacy_mode='plain';";
    REQUIRE_THROWS_AS(contract::get_project(host.context, host.project_id), contract::Error);
  }
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  const holder_recovery_snapshot_page* page = nullptr;
  holder_error* error = nullptr;
  REQUIRE(
      holder_read_recovery_snapshot(
          &host.context,
          host.project_id.c_str(),
          nullptr,
          nullptr,
          arena.get(),
          &page,
          &error
      ) == HOLDER_ERROR_RUNTIME
  );
  const std::unique_ptr<holder_error, decltype(&holder_error_destroy)> owned_error(
      error,
      holder_error_destroy
  );
  REQUIRE(error != nullptr);
  REQUIRE(std::string(holder_error_message(error)).find("unknown") != std::string::npos);
  REQUIRE(page == nullptr);
  host.db.exec(repair);
  error = nullptr;
  REQUIRE(
      holder_read_recovery_snapshot(
          &host.context,
          host.project_id.c_str(),
          nullptr,
          nullptr,
          arena.get(),
          &page,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(error == nullptr);
  REQUIRE(page->cards.count == 1);
  REQUIRE(page->cards.items[0].resources.count == 1);
}
