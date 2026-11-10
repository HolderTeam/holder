#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "core_test_helpers.h"
#include "capi/Context.h"
#include "card/CardRepo.h"
#include "git/GitOps.h"
#include "model/Project.h"
#include "platform/Db.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"
#include "project/ProjectRegistry.h"

#include <git2.h>
#include <holder/holder_api.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace {

std::string schema_sql() {
  std::ifstream in(SCHEMA_SQL_PATH);
  REQUIRE(in.is_open());
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void init_bare_repo(const std::filesystem::path& path) {
  git_libgit2_init();
  git_repository* repo = nullptr;
  REQUIRE(git_repository_init(&repo, path.string().c_str(), 1) == 0);
  git_repository_free(repo);
  git_libgit2_shutdown();
}

void seed_encrypted_project(const std::filesystem::path& data_dir, const std::filesystem::path& root,
                            const std::filesystem::path& remote) {
  const auto db_path = data_dir / "server" / "holder.db";
  std::filesystem::create_directories(db_path.parent_path());
  holder::platform::Db db;
  db.open(db_path);
  db.exec(schema_sql());
  holder::model::Project project;
  project.project_id = "project-1";
  project.name = "Synced Notes";
  project.root_path = root.string();
  project.git_remote_url = remote.string();
  project.privacy_mode = "encrypted_git";
  project.created_at = 10;
  project.updated_at = 20;
  holder::project::ProjectRepo repo(db);
  repo.create(project);
  holder::git::RealGitOps git;
  holder::privacy::ensure_encrypted_project_ready(git, repo, project.project_id, project.root_path,
                                                   std::nullopt, 20, [] { return "key-project-1"; });
}

struct Fixture {
  std::filesystem::path data_dir = holder::test::make_temp_dir();
  holder::test::EnvGuard keystore{"HOLDER_TEST_KEYSTORE_DIR", (data_dir / "keystore").string()};
  std::filesystem::path remote = data_dir / "remote";
  holder_context* source = nullptr;
  holder_context* target = nullptr;
  holder_arena* arena = holder_arena_new();
  holder_error* error = nullptr;
  std::string project_id;
  std::string token;

  Fixture() {
    init_bare_repo(remote);
    const auto schema = schema_sql();
    seed_encrypted_project(data_dir, data_dir / "repo", remote);
    REQUIRE(holder_context_open(data_dir.string().c_str(), schema.c_str(), &source, &error) == HOLDER_OK);
    project_id = "project-1";
    holder_recovery_token_export_request export_request{"1234"};
    const holder_project_recovery_token_export_result* exported = nullptr;
    REQUIRE(holder_export_project_recovery_token(source, project_id.c_str(), &export_request, arena,
                                                 &exported, &error) == HOLDER_OK);
    token = exported->recovery_token;
    const auto target_dir = holder::test::make_temp_dir();
    REQUIRE(holder_context_open(target_dir.string().c_str(), schema.c_str(), &target, &error) == HOLDER_OK);
  }

  ~Fixture() {
    holder_context_destroy(target);
    holder_context_destroy(source);
    holder_error_destroy(error);
    holder_arena_free(arena);
  }

  const holder_recovery_token_global_import_result* restore() {
    holder_recovery_token_import_request request{"1234", token.c_str()};
    const holder_recovery_token_global_import_result* result = nullptr;
    const auto status = holder_import_recovery_token_global(target, &request, arena, &result, &error);
    INFO((error ? holder_error_message(error) : "no import error"));
    REQUIRE(status == HOLDER_OK);
    return result;
  }

  std::string add_card() {
    holder_new_card request{};
    request.title = "Secret";
    request.content = "hidden";
    const holder_card* card = nullptr;
    REQUIRE(holder_create_card(source, project_id.c_str(), &request, arena, &card, &error) == HOLDER_OK);
    return card->card_id;
  }

  void push() {
    holder_push_options options{};
    const holder_sync_result* result = nullptr;
    REQUIRE(holder_push_project(source, project_id.c_str(), &options, arena, &result, &error) == HOLDER_OK);
    REQUIRE(result->push.status == HOLDER_PUSH_PHASE_STATUS_PUSHED);
  }
};

} // namespace

TEST_CASE("Recovery token import registers a recovered project and restores a forgotten identity",
          "[capi][recovery-registry]") {
  Fixture f;
  const auto id = f.add_card();
  f.push();
  f.target->registry_path = f.target->data_dir / "recovered-projects.json";
  holder::core::ProjectRegistry registry(f.target->registry_path);
  CHECK(registry.roots().empty());
  const auto* imported = f.restore();
  CHECK(imported->project_created);
  const auto project = holder::project::ProjectRepo(f.target->db).get(f.project_id);
  REQUIRE(project.has_value());
  auto roots = registry.roots();
  REQUIRE(roots.size() == 1);
  CHECK(std::filesystem::equivalent(roots[0], project->root_path));
  registry.forget(*project);
  REQUIRE(registry.roots().empty());
  // An ordinary refresh may not undo a deliberate removal, but explicit token
  // import is authorized to restore the identity to this device's registry.
  registry.remember({*project});
  CHECK(registry.roots().empty());
  const auto* retried = f.restore();
  CHECK_FALSE(retried->project_created);
  roots = registry.roots();
  REQUIRE(roots.size() == 1);
  CHECK(std::filesystem::equivalent(roots[0], project->root_path));
  const holder_card* card = nullptr;
  REQUIRE(holder_get_card(f.target, f.project_id.c_str(), id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  CHECK(std::string(card->content) == "hidden");
}

TEST_CASE("Generated sync exposes failed pulls and up-to-date pushes",
          "[capi][broad-sync-results]") {
  Fixture f;
  f.add_card();
  holder_push_options options{};
  const holder_sync_result* result = nullptr;
  SECTION("a repeated push reports up to date") {
    f.push();
    REQUIRE(holder_push_project(f.source, f.project_id.c_str(), &options, f.arena, &result, &f.error) == HOLDER_OK);
    REQUIRE(result != nullptr);
    CHECK(result->push.status == HOLDER_PUSH_PHASE_STATUS_UP_TO_DATE);
    CHECK(result->status == HOLDER_SYNC_RESULT_STATUS_SUCCEEDED);
  }
  SECTION("an empty remote reports a failed pull") {
    REQUIRE(holder_sync_project(f.source, f.project_id.c_str(), &options, f.arena, &result, &f.error) == HOLDER_OK);
    REQUIRE(result != nullptr);
    CHECK(result->pull.status == HOLDER_PULL_PHASE_STATUS_FAILED);
    REQUIRE(result->pull.error_message != nullptr);
    CHECK(std::string(result->pull.error_message).size() > 0);
  }
}

TEST_CASE("Recovery import reports a corrupt Git index and can be retried after repair",
          "[capi][broad-recovery-metrics]") {
  Fixture f;
  const auto id = f.add_card();
  f.push();
  f.restore();
  const auto project = holder::project::ProjectRepo(f.target->db).get(f.project_id);
  REQUIRE(project.has_value());
  const auto index = std::filesystem::path(project->root_path) / ".git/index";
  std::filesystem::rename(index, index.string() + ".saved");
  {
    std::ofstream corrupt(index, std::ios::binary);
    corrupt << "not an index";
  }
  holder_recovery_token_import_request request{"1234", f.token.c_str()};
  const holder_recovery_token_global_import_result* result = nullptr;
  REQUIRE(holder_import_recovery_token_global(f.target, &request, f.arena, &result, &f.error) == HOLDER_ERROR_RUNTIME);
  REQUIRE(f.error != nullptr);
  CHECK(std::string(holder_error_message(f.error)).find("index") != std::string::npos);
  CHECK(result == nullptr);
  holder_error_destroy(f.error);
  f.error = nullptr;
  REQUIRE(std::filesystem::remove(index));
  std::filesystem::rename(index.string() + ".saved", index);
  const auto* retried = f.restore();
  CHECK(retried->pull_status == HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED);
  const holder_card* card = nullptr;
  REQUIRE(holder_get_card(f.target, f.project_id.c_str(), id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  CHECK(std::string(card->content) == "hidden");
}

TEST_CASE("Recovery token import reports a locked remote config while keeping the restored key usable",
          "[capi][recovery-import-failures]") {
  Fixture f;
  const auto id = f.add_card();
  f.push();
  f.restore();
  const auto project = holder::project::ProjectRepo(f.target->db).get(f.project_id);
  REQUIRE(project.has_value());
  holder::git::GitRepo git;
  git.open_existing(project->root_path);
  // Force a config update: an already-correct remote needs no write or lock.
  git.set_remote("origin", (f.data_dir / "old-remote.git").string());
  const auto head = git.head_oid();
  const auto lock_path = std::filesystem::path(project->root_path) / ".git/config.lock";
  {
    std::ofstream lock(lock_path);
    REQUIRE(lock.is_open());
    lock << "another Git writer\n";
  }
  const auto* result = f.restore();
  CHECK_FALSE(result->project_created);
  CHECK(result->remote_hint_present);
  CHECK_FALSE(result->remote_configured);
  REQUIRE(result->remote_error != nullptr);
  CHECK(std::string(result->remote_error).size() > 0);
  CHECK(result->pull_status == HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_NOT_ATTEMPTED);
  CHECK(result->pull_error == nullptr);
  CHECK(git.head_oid() == head);
  const holder_card* card = nullptr;
  REQUIRE(holder_get_card(f.target, f.project_id.c_str(), id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  CHECK(std::string(card->content) == "hidden");
  REQUIRE(std::filesystem::remove(lock_path));
  const auto* retried = f.restore();
  CHECK_FALSE(retried->project_created);
  CHECK(retried->remote_configured);
  CHECK(retried->remote_error == nullptr);
  CHECK(retried->pull_status == HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED);
}

TEST_CASE("Recovery token import preserves a divergent local encrypted edit as a conflicted copy",
          "[capi][recovery-import-failures]") {
  Fixture f;
  const auto id = f.add_card();
  f.push();
  f.restore();
  const auto edit = [&](holder_context* context, const char* content) {
    holder_card_update update{};
    update.content = content;
    const holder_card* result = nullptr;
    REQUIRE(holder_update_card(context, f.project_id.c_str(), id.c_str(), &update,
                                f.arena, &result, &f.error) == HOLDER_OK);
    REQUIRE(std::string(result->content) == content);
  };
  edit(f.target, "Local edit to preserve");
  edit(f.source, "Remote edit to retain");
  f.push();
  const auto* imported = f.restore();
  INFO((imported->pull_error ? imported->pull_error : "no pull error"));
  CHECK_FALSE(imported->project_created);
  CHECK(imported->remote_configured);
  REQUIRE(imported->pull_status == HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED);
  const auto cards = holder::card::CardRepo(f.target->db).list_all(f.project_id);
  REQUIRE(cards.size() == 2);
  bool original = false;
  bool copy = false;
  for (const auto& record : cards) {
    const holder_card* card = nullptr;
    REQUIRE(holder_get_card(f.target, f.project_id.c_str(), record.card_id.c_str(),
                            f.arena, &card, &f.error) == HOLDER_OK);
    if (record.card_id == id) {
      CHECK(std::string(card->content) == "Remote edit to retain");
      CHECK(std::string(card->title) == "Secret");
      original = true;
    } else {
      CHECK(std::string(card->content) == "Local edit to preserve");
      CHECK(std::string(card->title) == "Secret (conflicted copy)");
      copy = true;
    }
  }
  CHECK(original);
  CHECK(copy);
  const holder_project_encryption_check* safety = nullptr;
  REQUIRE(holder_check_project_encryption(f.target, f.project_id.c_str(), f.arena,
                                           &safety, &f.error) == HOLDER_OK);
  REQUIRE(safety != nullptr);
  INFO(safety->check.message);
  CHECK(safety->check.ok);
  // Importing again does not create a second copy of the already-resolved edit.
  REQUIRE(f.restore()->pull_status == HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED);
  CHECK(holder::card::CardRepo(f.target->db).list_all(f.project_id).size() == 2);
}

TEST_CASE("Generated recovery token import configures and pulls its remote hint", "[capi][privacy]") {
  Fixture f;
  const std::string card_id = f.add_card();
  f.push();
  const auto* result = f.restore();
  INFO("recovery pull: " << (result->pull_error ? result->pull_error : "no error"));
  REQUIRE(result->project_created);
  REQUIRE(result->remote_hint_present);
  REQUIRE(result->remote_configured);
  REQUIRE(result->pull_status == HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED);
  const holder_card* card = nullptr;
  REQUIRE(holder_get_card(f.target, f.project_id.c_str(), card_id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  REQUIRE(std::string(card->content) == "hidden");
}

TEST_CASE("Generated recovery token import retries after an empty remote gains content", "[capi][privacy]") {
  Fixture f;
  const auto* result = f.restore();
  REQUIRE(result->remote_configured);
  REQUIRE(result->pull_status == HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_FAILED);
  REQUIRE(result->pull_error != nullptr);

  const std::string card_id = f.add_card();
  f.push();
  const holder_sync_result* pulled = nullptr;
  REQUIRE(holder_pull_project(f.target, f.project_id.c_str(), f.arena, &pulled, &f.error) == HOLDER_OK);
  INFO("retry pull: " << (pulled->pull.error_message ? pulled->pull.error_message : "no error"));
  REQUIRE(pulled->pull.status == HOLDER_PULL_PHASE_STATUS_SUCCEEDED);
  const holder_card* card = nullptr;
  REQUIRE(holder_get_card(f.target, f.project_id.c_str(), card_id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  REQUIRE(std::string(card->content) == "hidden");
}

TEST_CASE(
    "Generated project token import restores its key and rejects a wrong PIN",
    "[capi][privacy]"
) {
  Fixture f;
  const auto id = f.add_card();
  holder_recovery_token_import_request request{"1234", f.token.c_str()};
  const holder_project_recovery_token_import_result* result = nullptr;
  REQUIRE(
      holder_import_project_recovery_token(
          f.source,
          f.project_id.c_str(),
          &request,
          f.arena,
          &result,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(result->project_id) == f.project_id);
  request.pin = "wrong-pin";
  REQUIRE(
      holder_import_project_recovery_token(
          f.source,
          f.project_id.c_str(),
          &request,
          f.arena,
          &result,
          &f.error
      ) == HOLDER_ERROR_RECOVERY_TOKEN_INVALID
  );
  REQUIRE(f.error != nullptr);
  holder_error_destroy(f.error);
  f.error = nullptr;
  // A rejected import leaves the existing key usable.
  const holder_card* card = nullptr;
  REQUIRE(
      holder_get_card(f.source, f.project_id.c_str(), id.c_str(), f.arena, &card, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(card->content) == "hidden");
}

TEST_CASE("Generated explicit sync pulls and pushes through a local remote", "[capi][privacy]") {
  Fixture f;
  const auto id = f.add_card();
  f.push();
  f.restore();
  holder_card_update update{};
  update.content = "Edited on the restored device";
  const holder_card* card = nullptr;
  REQUIRE(
      holder_update_card(
          f.target,
          f.project_id.c_str(),
          id.c_str(),
          &update,
          f.arena,
          &card,
          &f.error
      ) == HOLDER_OK
  );
  holder_push_options options{};
  const holder_sync_result* synced = nullptr;
  REQUIRE(
      holder_sync_project(f.target, f.project_id.c_str(), &options, f.arena, &synced, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(synced->project_id) == f.project_id);
  REQUIRE(std::string(synced->remote_url) == f.remote.string());
  REQUIRE(synced->pull.status == HOLDER_PULL_PHASE_STATUS_SUCCEEDED);
  REQUIRE(synced->push.status == HOLDER_PUSH_PHASE_STATUS_PUSHED);
  // A third device must recover the pushed edit from the remote, independently
  // of either writer's working tree or database.
  holder_context* opened = nullptr;
  const auto observer_dir = holder::test::make_temp_dir();
  const auto schema = schema_sql();
  REQUIRE(
      holder_context_open(observer_dir.string().c_str(), schema.c_str(), &opened, &f.error) ==
      HOLDER_OK
  );
  const std::unique_ptr<holder_context, decltype(&holder_context_destroy)> observer(
      opened,
      holder_context_destroy
  );
  holder_recovery_token_import_request request{"1234", f.token.c_str()};
  const holder_recovery_token_global_import_result* recovered = nullptr;
  REQUIRE(
      holder_import_recovery_token_global(
          observer.get(),
          &request,
          f.arena,
          &recovered,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(
      recovered->pull_status == HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED
  );
  REQUIRE(
      holder_get_card(observer.get(), f.project_id.c_str(), id.c_str(), f.arena, &card, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(card->content) == "Edited on the restored device");
}

TEST_CASE(
    "Generated encrypted history distinguishes a missing key from an unavailable revision",
    "[capi][history-key]"
) {
  Fixture f;
  const auto id = f.add_card();
  const holder_card_history_page* history = nullptr;
  REQUIRE(
      holder_list_card_history(
          f.source,
          f.project_id.c_str(),
          id.c_str(),
          nullptr,
          nullptr,
          f.arena,
          &history,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(history->entries.count > 0);
  const std::string revision = history->entries.items[0].last_oid;
  {
    holder::test::EnvGuard missing_key(
        "HOLDER_TEST_KEYSTORE_DIR",
        (f.data_dir / "empty-keystore").string()
    );
    const holder_card_history_snapshot* snapshot = nullptr;
    REQUIRE(
        holder_get_card_history_snapshot(
            f.source,
            f.project_id.c_str(),
            id.c_str(),
            revision.c_str(),
            f.arena,
            &snapshot,
            &f.error
        ) == HOLDER_ERROR_HISTORY_KEY_UNAVAILABLE
    );
    REQUIRE(f.error != nullptr);
    REQUIRE(snapshot == nullptr);
    holder_error_destroy(f.error);
    f.error = nullptr;
  }
  // Restoring access to the key makes the same committed revision readable.
  const holder_card_history_snapshot* snapshot = nullptr;
  REQUIRE(
      holder_get_card_history_snapshot(
          f.source,
          f.project_id.c_str(),
          id.c_str(),
          revision.c_str(),
          f.arena,
          &snapshot,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(snapshot->snapshot.exists);
  REQUIRE(std::string(snapshot->snapshot.body) == "hidden");
}
