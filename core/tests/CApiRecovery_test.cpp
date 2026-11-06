#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "core_test_helpers.h"
#include "git/GitOps.h"
#include "model/Project.h"
#include "platform/Db.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"

#include <git2.h>
#include <holder/holder_api.h>

#include <filesystem>
#include <fstream>
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
    REQUIRE(holder_import_recovery_token_global(target, &request, arena, &result, &error) == HOLDER_OK);
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
