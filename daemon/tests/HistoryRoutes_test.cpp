#include "api/GeneratedRoutes.h"
#include "http_test_helpers.h"

#include "ai/AiMessageFrontMatter.h"
#include "ai/AiMessagePaths.h"
#include "ai/AiThreadManifest.h"
#include "card/CardFrontMatter.h"
#include "card/CardPaths.h"
#include "card/CardStore.h"
#include "git/GitRepo.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"
#include "resource/ResourceManifest.h"

#include <boost/beast/http.hpp>
#include <git2.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace http = boost::beast::http;

namespace {

// Sends a history request through the generated routes, with the query a route test's parameter
// function supplies; like the old handler's parameters, an empty value is left out.
template <typename Param>
bool dispatch_history(
    http::verb method,
    const std::string& path,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    const Param& param,
    holder::card::CardStore* card_store = nullptr
) {
  std::string target = path;
  char separator = '?';
  for (const char* key : {"limit", "cursor", "kind", "from_oid", "to_oid", "mode", "oid"}) {
    const std::string value = param(key);
    if (value.empty()) continue;
    target += separator;
    target += key;
    target += '=';
    for (const unsigned char ch : value) {
      if (std::isalnum(ch) != 0 || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
        target += static_cast<char>(ch);
      } else {
        static constexpr char hex[] = "0123456789ABCDEF";
        target += '%';
        target += hex[ch >> 4U];
        target += hex[ch & 0x0FU];
      }
    }
    separator = '&';
  }
  http::request<http::string_body> req{method, target, 11};
  return holder::api::dispatch_generated_routes(req, res, db, nullptr, nullptr, nullptr, nullptr,
                                                {}, {}, card_store);
}

template <typename Param>
bool dispatch_history_get(
    const std::string& path,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    const Param& param
) {
  return dispatch_history(http::verb::get, path, res, db, param);
}

// The generated routes answer a history path's undeclared methods with 405.
void require_method_not_allowed(http::verb method,
                                const std::string& path,
                                holder::platform::Db& db,
                                const std::string& allow) {
  http::request<http::string_body> req{method, path, 11};
  http::response<http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  CHECK(res.result() == http::status::method_not_allowed);
  CHECK(res[http::field::allow] == allow);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "method_not_allowed");
}

std::string history_card_file(
    const std::string& card_id,
    const std::string& body,
    const std::vector<holder::model::Milestone>& milestones = {}
) {
  holder::model::Card card;
  card.card_id = card_id;
  card.project_id = "history-project";
  card.title = "History card";
  card.rel_path = holder::core::card_rel_path(card_id);
  card.created_at = 1;
  card.updated_at = 2;
  return holder::core::render_card_front_matter(card, {}, milestones) + body;
}

void history_commit(
    holder::git::GitRepo& git,
    const std::string& card_id,
    const std::string& body,
    const std::string& message
) {
  const auto path = holder::core::card_rel_path(card_id);
  git.write_file(path, history_card_file(card_id, body));
  git.stage_path(path);
  git.commit(message);
}

std::string history_resource_manifest(const std::string& resource_id, const std::string& label) {
  holder::model::ResourceBundle bundle;
  bundle.resource.resource_id = resource_id;
  bundle.resource.project_id = "history-project";
  bundle.resource.type = "file";
  bundle.resource.label = label;
  bundle.resource.created_at = 1;
  bundle.resource.updated_at = 1;
  holder::model::Asset asset;
  asset.asset_id = "asset-" + resource_id;
  asset.resource_id = resource_id;
  asset.original_filename = "project-notes.pdf";
  asset.media_type = "application/pdf";
  asset.byte_size = 42;
  asset.plaintext_sha256 = std::string(64, 'a');
  bundle.assets.push_back(asset);
  return holder::resource::render_resource_manifest(bundle);
}

struct FileSnapshot {
  bool exists = false;
  std::string bytes;
};

FileSnapshot snapshot_file(const std::filesystem::path& path) {
  FileSnapshot snapshot;
  snapshot.exists = std::filesystem::exists(path);
  if (!snapshot.exists) return snapshot;
  std::ifstream input(path, std::ios::binary);
  snapshot.bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  return snapshot;
}

void check_file_unchanged(const std::filesystem::path& path, const FileSnapshot& before) {
  const auto after = snapshot_file(path);
  CHECK(after.exists == before.exists);
  CHECK(after.bytes == before.bytes);
}

std::string uppercase_hex(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
    if (character >= 'a' && character <= 'f') {
      return static_cast<char>(character - 'a' + 'A');
    }
    return static_cast<char>(character);
  });
  return value;
}

std::string collision_commit_content(const std::string& tree_oid, std::uint32_t nonce) {
  return "tree " + tree_oid +
         "\n"
         "author Holder <holder@example.invalid> 1 +0000\n"
         "committer Holder <holder@example.invalid> 1 +0000\n\n"
         "route revision collision " +
         std::to_string(nonce) + "\n";
}

std::pair<std::string, std::string> write_ambiguous_revision_prefix(
    const std::filesystem::path& root,
    const std::string& tree_oid
) {
  git_repository* raw = nullptr;
  REQUIRE(git_repository_open(&raw, root.string().c_str()) == 0);
  git_odb* odb = nullptr;
  REQUIRE(git_repository_odb(&odb, raw) == 0);

  std::unordered_map<std::uint32_t, std::uint32_t> seen;
  std::optional<std::pair<std::uint32_t, std::uint32_t>> collision;
  for (std::uint32_t nonce = 0; nonce < 500'000 && !collision.has_value(); ++nonce) {
    const auto content = collision_commit_content(tree_oid, nonce);
    git_oid oid{};
    if (git_odb_hash(&oid, content.data(), content.size(), GIT_OBJECT_COMMIT) != 0) {
      FAIL("git_odb_hash failed while constructing ambiguous revision prefixes");
    }
    const auto prefix = (static_cast<std::uint32_t>(oid.id[0]) << 24U) |
                        (static_cast<std::uint32_t>(oid.id[1]) << 16U) |
                        (static_cast<std::uint32_t>(oid.id[2]) << 8U) |
                        static_cast<std::uint32_t>(oid.id[3]);
    const auto [position, inserted] = seen.emplace(prefix, nonce);
    if (!inserted) collision = std::pair{position->second, nonce};
  }
  REQUIRE(collision.has_value());

  std::pair<std::string, std::string> oids;
  const std::uint32_t nonces[] = {collision->first, collision->second};
  std::string* outputs[] = {&oids.first, &oids.second};
  for (std::size_t index = 0; index < 2; ++index) {
    const auto content = collision_commit_content(tree_oid, nonces[index]);
    git_oid oid{};
    REQUIRE(git_odb_write(&oid, odb, content.data(), content.size(), GIT_OBJECT_COMMIT) == 0);
    *outputs[index] = git_oid_tostr_s(&oid);
  }

  git_odb_free(odb);
  git_repository_free(raw);
  REQUIRE(oids.first.substr(0, 8) == oids.second.substr(0, 8));
  REQUIRE(oids.first != oids.second);
  return oids;
}

} // namespace

TEST_CASE("HistoryRoutes lists and compares card versions", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());

  const std::string card_id = "abcd-route-history";
  holder::git::GitRepo git;
  git.open_or_init(project_root);
  history_commit(git, card_id, "Old body\n", "Add card History card");
  const auto old_oid = git.head_oid();
  REQUIRE(old_oid.has_value());
  history_commit(git, card_id, "New body\n", "Update card History card");

  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  std::unordered_map<std::string, std::string> query;
  auto param = [&](const std::string& key) {
    const auto found = query.find(key);
    return found == query.end() ? std::string{} : found->second;
  };
  const auto base = "/projects/history-project/history/cards/" + card_id;
  REQUIRE(dispatch_history_get(base, res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto list = nlohmann::json::parse(res.body())["data"];
  REQUIRE(list["entries"].size() == 2);
  REQUIRE(list["head_oid"].is_string());
  CHECK_FALSE(list["scan_limited"].get<bool>());
  REQUIRE(list["entries"][0]["saves"].size() == 1);
  REQUIRE(list["entries"][0]["visible_parent_oids"].is_array());
  REQUIRE(list["entries"][0]["visible_parent_oids"].size() == 1);
  CHECK(list["entries"][0]["visible_parent_oids"][0] == list["entries"][1]["last_oid"]);
  CHECK(list["entries"][0]["saves"][0]["oid"] == list["entries"][0]["last_oid"]);
  CHECK(list["entries"][0]["saves"][0]["parent_oids"].is_array());
  CHECK(list["entries"][0]["saves"][0]["message"] == "Update card History card");
  CHECK(list["entries"][0]["saves"][0]["authored_at"].is_number_integer());

  const auto head_oid = list["head_oid"].get<std::string>();
  query["from_oid"] = uppercase_hex(old_oid->substr(0, 8));
  query["to_oid"] = uppercase_hex(head_oid.substr(0, 8));
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto comparison = nlohmann::json::parse(res.body())["data"];
  CHECK(comparison["before"]["body"] == "Old body\n");
  CHECK(comparison["before"]["oid"] == *old_oid);
  CHECK(comparison["after"]["body"] == "New body\n");
  CHECK(comparison["after"]["oid"] == head_oid);

  query.clear();
  query["to_oid"] = uppercase_hex(head_oid.substr(0, 8));
  query["mode"] = "change";
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto edit = nlohmann::json::parse(res.body())["data"];
  CHECK(edit["before"]["exists"] == true);
  CHECK(edit["before"]["body"] == "Old body\n");
  CHECK(edit["before"]["oid"] == *old_oid);
  CHECK(edit["after"]["body"] == "New body\n");
  CHECK(edit["after"]["oid"] == head_oid);
  CHECK(std::none_of(edit["lines"].begin(), edit["lines"].end(), [](const auto& line) {
    return line["origin"] == "+" && line["text"] == "# History card";
  }));

  query.clear();
  query["to_oid"] = old_oid->substr(0, 8);
  query["mode"] = "change";
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto creation = nlohmann::json::parse(res.body())["data"];
  CHECK_FALSE(creation["before"]["exists"].get<bool>());
  CHECK(creation["after"]["body"] == "Old body\n");
  CHECK(creation["after"]["oid"] == *old_oid);

  query["mode"] = "unsupported";
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  CHECK(res.result() == http::status::bad_request);

  // A reference that is not 8 to 40 hex digits fails the contract before any lookup.
  query["mode"] = "since";
  query["from_oid"] = "not-an-oid";
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  CHECK(res.result() == http::status::bad_request);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "invalid_argument");

  query["from_oid"] = "deadbeef";
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  CHECK(res.result() == http::status::not_found);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "not_found");

  query = {{"mode", "change"}, {"to_oid", "deadbeef"}};
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  CHECK(res.result() == http::status::not_found);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "not_found");

  query.clear();
  query["to_oid"] = list["head_oid"].get<std::string>();
  query["mode"] = "since";
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  CHECK(res.result() == http::status::bad_request);

  query.clear();
  query["mode"] = "change";
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  CHECK(res.result() == http::status::bad_request);

  query.clear();
  query["cursor"] = "not-an-oid";
  res = {};
  REQUIRE(dispatch_history_get(base, res, db, param));
  CHECK(res.result() == http::status::bad_request);

  query.clear();
  query["limit"] = "999999999999999999999999999999999999";
  res = {};
  REQUIRE(dispatch_history_get(base, res, db, param));
  CHECK(res.result() == http::status::bad_request);
}

TEST_CASE("HistoryRoutes lists and filters project activities", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());

  holder::git::GitRepo git;
  git.open_or_init(project_root);
  holder::model::Milestone milestone;
  milestone.milestone_id = "route-milestone";
  milestone.project_id = "history-project";
  milestone.card_id = "abcd-project-route";
  milestone.dates = {holder::model::milestone_date_from_legacy("start", 1, false)};
  milestone.kind = "Review";
  milestone.description = "Project review";
  git.write_file(
      "cards/ab/cd/abcd-project-route.md",
      history_card_file("abcd-project-route", "Project history card", {milestone})
  );
  git.write_file(
      "resources/ef/gh/efgh-project-route.json",
      history_resource_manifest("efgh-project-route", "Project notes")
  );
  git.stage_paths({"cards/ab/cd/abcd-project-route.md", "resources/ef/gh/efgh-project-route.json"});
  git.commit("Attach project resource");
  git.write_file("notes/from-another-tool.txt", "external");
  git.stage_path("notes/from-another-tool.txt");
  git.commit("External project note");

  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  std::unordered_map<std::string, std::string> query;
  auto param = [&](const std::string& key) {
    const auto found = query.find(key);
    return found == query.end() ? std::string{} : found->second;
  };
  const std::string path = "/projects/history-project/history";

  REQUIRE(dispatch_history_get(path, res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto page = nlohmann::json::parse(res.body())["data"];
  REQUIRE(page["activities"].size() == 2);
  CHECK(page["activities"][0]["message"] == "External project note");
  REQUIRE(page["activities"][1]["affected_objects"].size() == 2);
  CHECK(page["activities"][1]["affected_objects"][0]["kind"] == "card");
  CHECK(
      page["activities"][1]["affected_objects"][0]["items"][0]["path"] ==
      "cards/ab/cd/abcd-project-route.md"
  );
  CHECK(page["activities"][1]["affected_objects"][0]["items"][0]["title"] == "History card");
  CHECK(
      page["activities"][1]["affected_objects"][0]["items"][0]["detail"] ==
      "Milestone: Review — Project review"
  );
  CHECK(page["activities"][1]["affected_objects"][1]["kind"] == "resource");
  CHECK(page["activities"][1]["affected_objects"][1]["items"][0]["title"] == "Project notes");
  CHECK(
      page["activities"][1]["affected_objects"][1]["items"][0]["detail"] ==
      "Attachment: project-notes.pdf"
  );

  query["kind"] = "resource";
  res = {};
  REQUIRE(dispatch_history_get(path, res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto resource_page = nlohmann::json::parse(res.body())["data"];
  REQUIRE(resource_page["activities"].size() == 1);
  CHECK(resource_page["activities"][0]["message"] == "Attach project resource");

  query["kind"] = "not-a-kind";
  res = {};
  REQUIRE(dispatch_history_get(path, res, db, param));
  CHECK(res.result() == http::status::bad_request);
}

TEST_CASE(
    "HistoryRoutes reads snapshots through canonical revision references",
    "[http][history]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  const auto other_root = dir / "other-project";
  holder::test::create_project(db, "history-project", project_root.string());
  holder::test::create_project(db, "other-project", other_root.string());

  const std::string card_id = "abcd-snapshot-route";
  holder::git::GitRepo git;
  git.open_or_init(project_root);
  history_commit(git, card_id, "Saved snapshot body\n", "Add card History card");
  const auto saved_oid = git.head_oid().value();
  history_commit(git, card_id, "Later body\n", "Update card History card");

  holder::git::GitRepo other_git;
  other_git.open_or_init(other_root);
  other_git.write_file("other.txt", "other project");
  other_git.stage_path("other.txt");
  other_git.commit("Other project revision");
  const auto other_oid = other_git.head_oid().value();

  const auto path = "/projects/history-project/history/cards/" + card_id + "/snapshot";
  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  std::unordered_map<std::string, std::string> query;
  auto param = [&](const std::string& key) {
    const auto found = query.find(key);
    return found == query.end() ? std::string{} : found->second;
  };

  query["oid"] = uppercase_hex(saved_oid.substr(0, 8));
  REQUIRE(dispatch_history_get(path, res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto data = nlohmann::json::parse(res.body())["data"];
  CHECK(data["card_id"] == card_id);
  CHECK(data["snapshot"]["exists"] == true);
  CHECK(data["snapshot"]["oid"] == saved_oid);
  CHECK(data["snapshot"]["title"] == "History card");
  CHECK(data["snapshot"]["body"] == "Saved snapshot body\n");

  query["oid"] = saved_oid;
  res = {};
  const auto missing_card_path =
      "/projects/history-project/history/cards/abcd-absent-snapshot/snapshot";
  REQUIRE(dispatch_history_get(missing_card_path, res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto absent = nlohmann::json::parse(res.body())["data"]["snapshot"];
  CHECK(absent["exists"] == false);
  CHECK(absent["oid"] == saved_oid);

  query["oid"].clear();
  res = {};
  REQUIRE(dispatch_history_get(path, res, db, param));
  CHECK(res.result() == http::status::bad_request);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "invalid_argument");

  query["oid"] = saved_oid.substr(0, 7);
  res = {};
  REQUIRE(dispatch_history_get(path, res, db, param));
  CHECK(res.result() == http::status::bad_request);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "invalid_argument");

  query["oid"] = std::string(40, '0');
  res = {};
  REQUIRE(dispatch_history_get(path, res, db, param));
  CHECK(res.result() == http::status::not_found);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "not_found");

  query["oid"] = other_oid;
  res = {};
  REQUIRE(dispatch_history_get(path, res, db, param));
  CHECK(res.result() == http::status::not_found);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "not_found");

  require_method_not_allowed(http::verb::post, path, db, "GET");
}

TEST_CASE(
    "HistoryRoutes maps ambiguous revision references on every revision route",
    "[http][history]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());

  holder::git::GitRepo git;
  git.open_or_init(project_root);
  git.write_file("seed.txt", "seed");
  git.stage_path("seed.txt");
  git.commit("seed");

  git_repository* raw = nullptr;
  REQUIRE(git_repository_open(&raw, project_root.string().c_str()) == 0);
  git_oid head_oid{};
  REQUIRE(git_reference_name_to_id(&head_oid, raw, "HEAD") == 0);
  git_commit* head = nullptr;
  REQUIRE(git_commit_lookup(&head, raw, &head_oid) == 0);
  const std::string tree_oid = git_oid_tostr_s(git_commit_tree_id(head));
  git_commit_free(head);
  git_repository_free(raw);
  const auto [first_oid, second_oid] = write_ambiguous_revision_prefix(project_root, tree_oid);
  REQUIRE(first_oid.substr(0, 8) == second_oid.substr(0, 8));

  std::unordered_map<std::string, std::string> query{{"oid", first_oid.substr(0, 8)}};
  auto param = [&](const std::string& key) {
    const auto found = query.find(key);
    return found == query.end() ? std::string{} : found->second;
  };
  const std::string base = "/projects/history-project/history/cards/abcd-ambiguous";

  http::request<http::string_body> get{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  REQUIRE(dispatch_history_get(base + "/snapshot", res, db, param));
  REQUIRE(res.result() == http::status::conflict);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "revision_ambiguous");

  query = {{"from_oid", first_oid}, {"to_oid", first_oid.substr(0, 8)}};
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  REQUIRE(res.result() == http::status::conflict);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "revision_ambiguous");

  query = {{"mode", "change"}, {"to_oid", first_oid.substr(0, 8)}};
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  REQUIRE(res.result() == http::status::conflict);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "revision_ambiguous");

  holder::card::CardStore store(db, nullptr);
  query = {{"oid", first_oid.substr(0, 8)}};
  res = {};
  REQUIRE(dispatch_history(http::verb::post, base + "/restore", res, db, param, &store));
  REQUIRE(res.result() == http::status::conflict);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "revision_ambiguous");
}

TEST_CASE("HistoryRoutes describes historical project AI data", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());

  holder::model::Project project;
  project.project_id = "history-project";
  project.root_path = project_root.string();
  project.privacy_mode = "plain";
  holder::model::AiThread thread;
  thread.thread_id = "route-thread-history";
  thread.project_id = project.project_id;
  thread.title = "Release review";
  thread.created_at = 1;
  thread.updated_at = 1;
  holder::model::AiMessage message;
  message.message_id = "route-message-history";
  message.thread_id = thread.thread_id;
  message.role = "user";
  message.source = "holder";
  message.created_at = 2;

  holder::git::GitRepo git;
  git.open_or_init(project_root);
  const auto thread_path = holder::ai::ai_thread_manifest_rel_path(thread.thread_id);
  const auto message_path = holder::core::ai_message_rel_path(message.message_id);
  git.write_file(thread_path, holder::ai::render_ai_thread_manifest(project, thread));
  git.write_file(
      message_path,
      holder::core::render_ai_message_front_matter(message, project.project_id, {}) +
          "Can you review the release notes?\n"
  );
  git.stage_paths({thread_path, message_path});
  git.commit("Capture AI review");

  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  const auto empty_param = [](const std::string&) {
    return std::string{};
  };
  REQUIRE(dispatch_history_get("/projects/history-project/history", res, db, empty_param));
  REQUIRE(res.result() == http::status::ok);
  const auto page = nlohmann::json::parse(res.body())["data"];
  REQUIRE(page["activities"].size() == 1);
  const auto& items = page["activities"][0]["affected_objects"][0]["items"];
  REQUIRE(items.size() == 2);
  bool found_message = false;
  bool found_thread = false;
  for (const auto& item : items) {
    if (item["path"] == message_path) {
      found_message = true;
      CHECK(item["title"] == "Release review");
      CHECK(item["detail"] == "user: Can you review the release notes?");
    }
    if (item["path"] == thread_path) {
      found_thread = true;
      CHECK(item["title"] == "Release review");
      CHECK_FALSE(item.contains("detail"));
    }
  }
  CHECK(found_message);
  CHECK(found_thread);
}

TEST_CASE("HistoryRoutes validates project and comparison parameters", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  auto empty_param = [](const std::string&) {
    return std::string{};
  };

  REQUIRE(dispatch_history_get("/projects/missing/history/cards/abcd-card", res, db, empty_param));
  CHECK(res.result() == http::status::not_found);
  CHECK_FALSE(dispatch_history_get("/cards/abcd-card/history", res, db, empty_param));

  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());
  res = {};
  REQUIRE(dispatch_history_get("/projects/history-project/history/cards/abc", res, db, empty_param));
  CHECK(res.result() == http::status::bad_request);
}

TEST_CASE(
    "HistoryRoutes handles an empty card history and card-absent revision",
    "[http][history]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());

  holder::git::GitRepo git;
  git.open_or_init(project_root);
  git.write_file("project-notes.md", "This repository revision has no card.\n");
  git.stage_path("project-notes.md");
  git.commit("Add project note");
  const auto unrelated_oid = git.head_oid();
  REQUIRE(unrelated_oid.has_value());

  const std::string card_id = "abcd-no-history";
  const auto base = "/projects/history-project/history/cards/" + card_id;
  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  std::unordered_map<std::string, std::string> query;
  auto param = [&](const std::string& key) {
    const auto found = query.find(key);
    return found == query.end() ? std::string{} : found->second;
  };

  REQUIRE(dispatch_history_get(base, res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto list = nlohmann::json::parse(res.body())["data"];
  CHECK(list["head_oid"] == *unrelated_oid);
  CHECK(list["entries"].empty());
  CHECK(list["next_cursor"].is_null());

  query["from_oid"] = *unrelated_oid;
  query["to_oid"] = *unrelated_oid;
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto comparison = nlohmann::json::parse(res.body())["data"];
  CHECK_FALSE(comparison["before"]["exists"].get<bool>());
  CHECK_FALSE(comparison["after"]["exists"].get<bool>());
  CHECK(comparison["lines"].empty());
}

TEST_CASE("HistoryRoutes compares captured revisions after a later autosave", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());

  const std::string card_id = "abcd-captured-history";
  holder::git::GitRepo git;
  git.open_or_init(project_root);
  history_commit(git, card_id, "First saved body\n", "Add card History card");
  const auto first_oid = git.head_oid();
  REQUIRE(first_oid.has_value());
  history_commit(git, card_id, "Captured saved body\n", "Update card History card");

  const auto base = "/projects/history-project/history/cards/" + card_id;
  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  std::unordered_map<std::string, std::string> query;
  auto param = [&](const std::string& key) {
    const auto found = query.find(key);
    return found == query.end() ? std::string{} : found->second;
  };

  REQUIRE(dispatch_history_get(base, res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto captured_oid = nlohmann::json::parse(res.body())["data"]["head_oid"].get<std::string>(
  );

  history_commit(git, card_id, "Later autosave body\n", "Update card History card");

  query["from_oid"] = *first_oid;
  query["to_oid"] = captured_oid;
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  REQUIRE(res.result() == http::status::ok);
  const auto comparison = nlohmann::json::parse(res.body())["data"];
  CHECK(comparison["before"]["oid"] == *first_oid);
  CHECK(comparison["before"]["body"] == "First saved body\n");
  CHECK(comparison["after"]["oid"] == captured_oid);
  CHECK(comparison["after"]["body"] == "Captured saved body\n");
}

TEST_CASE("HistoryRoutes rejects an oversized history comparison", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());

  const std::string card_id = "abcd-oversized-history";
  holder::git::GitRepo git;
  git.open_or_init(project_root);
  history_commit(git, card_id, "Small version\n", "Add card History card");
  const auto old_oid = git.head_oid();
  REQUIRE(old_oid.has_value());
  const std::string oversized(2 * 1024 * 1024, 'x');
  history_commit(git, card_id, oversized, "Update card History card");
  const auto head_oid = git.head_oid();
  REQUIRE(head_oid.has_value());

  const auto base = "/projects/history-project/history/cards/" + card_id;
  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  std::unordered_map<std::string, std::string> query;
  auto param = [&](const std::string& key) {
    const auto found = query.find(key);
    return found == query.end() ? std::string{} : found->second;
  };

  query["from_oid"] = *old_oid;
  query["to_oid"] = *head_oid;
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  REQUIRE(res.result() == http::status::payload_too_large);
  CHECK(nlohmann::json::parse(res.body())["error"]["code"] == "history_response_too_large");
  query.clear();
  query["oid"] = *head_oid;
  REQUIRE(dispatch_history_get(base + "/snapshot", res, db, param));
  CHECK(res.result() == http::status::payload_too_large);
  history_commit(git, card_id, "Another version\n", oversized);
  query.clear();
  for (const std::string path : {base, std::string("/projects/history-project/history")}) {
    REQUIRE(dispatch_history_get(path, res, db, param));
    CHECK(res.result() == http::status::payload_too_large);
  }
}

TEST_CASE("HistoryRoutes leave SQLite files unchanged", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = holder::test::open_db_with_schema(db_path);
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());

  const std::string card_id = "abcd-sqlite-history";
  holder::git::GitRepo git;
  git.open_or_init(project_root);
  history_commit(git, card_id, "First body\n", "Add card History card");
  const auto old_oid = git.head_oid();
  REQUIRE(old_oid.has_value());
  history_commit(git, card_id, "Second body\n", "Update card History card");
  const auto head_oid = git.head_oid();
  REQUIRE(head_oid.has_value());

  const auto db_before = snapshot_file(db_path);
  const auto wal_before = snapshot_file(db_path.string() + "-wal");
  const auto journal_before = snapshot_file(db_path.string() + "-journal");

  const auto base = "/projects/history-project/history/cards/" + card_id;
  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  std::unordered_map<std::string, std::string> query;
  auto param = [&](const std::string& key) {
    const auto found = query.find(key);
    return found == query.end() ? std::string{} : found->second;
  };

  REQUIRE(dispatch_history_get(base, res, db, param));
  REQUIRE(res.result() == http::status::ok);
  query["from_oid"] = *old_oid;
  query["to_oid"] = *head_oid;
  res = {};
  REQUIRE(dispatch_history_get(base + "/compare", res, db, param));
  REQUIRE(res.result() == http::status::ok);

  // SQLite can update the -shm file solely to record an active WAL reader.
  // That transient lock bookkeeping is not persistent database state, so this
  // proof deliberately checks the database and durable journal files instead.
  check_file_unchanged(db_path, db_before);
  check_file_unchanged(db_path.string() + "-wal", wal_before);
  check_file_unchanged(db_path.string() + "-journal", journal_before);
}

TEST_CASE("HistoryRoutes reports an unavailable encrypted project key", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "encrypted-project";
  holder::model::Project project;
  project.project_id = "encrypted-history-project";
  project.name = "Encrypted history";
  project.root_path = project_root.string();
  project.privacy_mode = "encrypted_git";
  project.project_key_id.reset();
  project.created_at = 1;
  project.updated_at = 1;
  holder::project::ProjectRepo(db).create(project);

  const std::string card_id = "abcd-encrypted-route";
  holder::git::GitRepo git;
  git.open_or_init(project_root);
  history_commit(git, card_id, "Encrypted body\n", "Add card Encrypted history");

  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  auto empty_param = [](const std::string&) {
    return std::string{};
  };
  REQUIRE(dispatch_history_get("/projects/encrypted-history-project/history/cards/" + card_id, res, db, empty_param));
  CHECK(res.result() == http::status::conflict);
  const auto error = nlohmann::json::parse(res.body())["error"];
  CHECK(error["code"] == "history_key_unavailable");
}

TEST_CASE("HistoryRoutes reports malformed historical card data", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());

  const std::string card_id = "abcd-malformed-route";
  holder::git::GitRepo git;
  git.open_or_init(project_root);
  const auto path = holder::core::card_rel_path(card_id);
  git.write_file(path, "Not a Holder card file\n");
  git.stage_path(path);
  git.commit("Update card History card");

  http::request<http::string_body> req{http::verb::get, "/", 11};
  http::response<http::string_body> res;
  auto empty_param = [](const std::string&) {
    return std::string{};
  };
  REQUIRE(dispatch_history_get("/projects/history-project/history/cards/" + card_id, res, db, empty_param));
  CHECK(res.result() == http::status::service_unavailable);
  const auto error = nlohmann::json::parse(res.body())["error"];
  CHECK(error["code"] == "history_unavailable");
  CHECK(error["message"] == "Historical card content is malformed");
}

TEST_CASE("HistoryRoutes restores a selected card version", "[http][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "project";
  holder::test::create_project(db, "history-project", project_root.string());
  holder::card::CardStore store(db, nullptr);
  holder::model::Card card;
  card.card_id = "abcd-restore-route";
  card.project_id = "history-project";
  card.title = "Original";
  card.created_at = 1;
  card.updated_at = 1;
  store.create(card, "original body");
  holder::git::GitRepo git;
  git.open_existing(project_root);
  const auto oid = git.head_oid();
  REQUIRE(oid.has_value());
  store.update_content(card.card_id, "changed body", std::string("Changed"), 2);
  store.trash(card.card_id, 3);
  REQUIRE(store.get(card.card_id)->deleted_at.has_value());

  http::response<http::string_body> res;
  const auto abbreviated_oid = uppercase_hex(oid->substr(0, 8));
  auto param = [&](const std::string& key) {
    return key == "oid" ? abbreviated_oid : std::string{};
  };
  REQUIRE(dispatch_history(http::verb::post,
                           "/projects/history-project/history/cards/" + card.card_id + "/restore",
                           res, db, param, &store));
  CHECK(res.result() == http::status::ok);
  const auto response = nlohmann::json::parse(res.body());
  REQUIRE(response["ok"] == true);
  const auto data = response["data"];
  CHECK(data["card_id"] == card.card_id);
  CHECK(data["restored_from_oid"] == *oid);
  CHECK(data["result_oid"].is_string());
  CHECK(data["result_oid"].get<std::string>().size() == 40);
  CHECK(data["result_oid"] != *oid);
  CHECK(data["title"] == "Original");
  CHECK(data["updated_at"].is_number_integer());
  CHECK(data["deleted_at"].is_null());
  const auto restored = store.get(card.card_id);
  REQUIRE(restored.has_value());
  CHECK(restored->title == "Original");
  CHECK_FALSE(restored->deleted_at.has_value());
  REQUIRE(store.get_content(*restored).value() == "original body");

  git.open_existing(project_root);
  CHECK(git.head_oid() == data["result_oid"].get<std::string>());
}

TEST_CASE(
    "HistoryRoutes rejects restore requests without required services or revisions",
    "[http][history]"
) {
  const auto root = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(root / "holder.db");
  holder::test::create_project(db, "history-project", (root / "project").string());
  holder::card::CardStore cards(db, nullptr);
  const auto path = "/projects/history-project/history/cards/card-one/restore";
  http::response<http::string_body> response;
  auto params = [](const std::string&) {
    return std::string();
  };
  CHECK_FALSE(dispatch_history(http::verb::post,
                               "/projects/history-project/history/cards/card-one/unknown",
                               response, db, params, &cards));
  require_method_not_allowed(http::verb::get, path, db, "POST");
  // The revision is required before the card is looked up.
  REQUIRE(dispatch_history(http::verb::post, path, response, db, params));
  CHECK(response.result() == http::status::bad_request);

  holder::git::GitRepo git;
  git.open_or_init(root / "project");
  history_commit(git, "different-card", "body\n", "Add a different card");
  const auto oid = git.head_oid();
  REQUIRE(oid.has_value());
  auto oid_param = [&](const std::string& key) {
    return key == "oid" ? *oid : std::string();
  };
  REQUIRE(dispatch_history(http::verb::post, path, response, db, oid_param));
  CHECK(response.result() == http::status::not_found);
  REQUIRE(dispatch_history(http::verb::post, path, response, db, oid_param, &cards));
  CHECK(response.result() == http::status::not_found);
  CHECK(nlohmann::json::parse(response.body())["error"]["message"] == "Card not found.");
  REQUIRE(dispatch_history(http::verb::post, "/projects/missing/history/cards/card-one/restore",
                           response, db, oid_param, &cards));
  CHECK(response.result() == http::status::not_found);
}
