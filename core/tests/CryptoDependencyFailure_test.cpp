// Isolated dependency-return-value injection: production code still calls libsodium.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"
#include "git/GitRepo.h"
#include "privacy/CryptoService.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"

#include <holder/holder_api.h>
#include <sodium.h>

#include <filesystem>
#include <memory>
#include <string>

namespace {
enum class Failure { None, Initialization, Encryption };
thread_local Failure pending = Failure::None;

struct InjectFailure {
  explicit InjectFailure(Failure failure) { pending = failure; }
  ~InjectFailure() { pending = Failure::None; }
  bool consumed() const { return pending == Failure::None; }
};

struct Host {
  std::filesystem::path dir = holder::test::make_temp_dir();
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena{
      holder_arena_new(),
      holder_arena_free
  };
  holder_error* error = nullptr;

  Host() {
    context.projects_root = dir / "projects";
    context.registry_path = dir / "projects.json";
  }
  ~Host() { holder_error_destroy(error); }

  holder::contract::Project make_project() {
    holder::contract::NewProject request;
    request.name = "Crypto failure";
    request.privacy_mode = holder::contract::NewProjectPrivacyMode::EncryptedGit;
    return holder::contract::create_project(context, request);
  }
  std::string export_token(const std::string& id) {
    const holder_project_recovery_token_export_result* result = nullptr;
    const holder_recovery_token_export_request request{"1234"};
    REQUIRE(
        holder_export_project_recovery_token(
            &context,
            id.c_str(),
            &request,
            arena.get(),
            &result,
            &error
        ) == HOLDER_OK
    );
    REQUIRE(result != nullptr);
    return result->recovery_token;
  }
};
} // namespace

extern "C" int __real_sodium_init();
extern "C" int __wrap_sodium_init() {
  if (pending == Failure::Initialization) {
    pending = Failure::None;
    return -1;
  }
  return __real_sodium_init();
}

extern "C" int
__real_crypto_aead_xchacha20poly1305_ietf_encrypt(unsigned char*, unsigned long long*, const unsigned char*, unsigned long long, const unsigned char*, unsigned long long, const unsigned char*, const unsigned char*, const unsigned char*);
extern "C" int __wrap_crypto_aead_xchacha20poly1305_ietf_encrypt(
    unsigned char* output,
    unsigned long long* output_size,
    const unsigned char* message,
    unsigned long long message_size,
    const unsigned char* additional,
    unsigned long long additional_size,
    const unsigned char* secret_nonce,
    const unsigned char* nonce,
    const unsigned char* key
) {
  if (pending == Failure::Encryption) {
    pending = Failure::None;
    return -1;
  }
  return __real_crypto_aead_xchacha20poly1305_ietf_encrypt(
      output,
      output_size,
      message,
      message_size,
      additional,
      additional_size,
      secret_nonce,
      nonce,
      key
  );
}

TEST_CASE(
    "Crypto initialization failure raises the typed privacy error before returning a key",
    "[crypto-dependency]"
) {
  InjectFailure fault(Failure::Initialization);
  try {
    (void)holder::privacy::generate_random_key();
    FAIL("key generation succeeded after initialization failure");
  } catch (const holder::privacy::PrivacyError& error) {
    CHECK(fault.consumed());
    CHECK(error.code() == holder::privacy::PrivacyErrorCode::PrivacyCryptoFailed);
    CHECK(std::string(error.what()) == "failed to initialize libsodium");
  }
}

TEST_CASE(
    "Recovery export translates crypto failures without changing the project or key",
    "[crypto-dependency]"
) {
  Failure failure = Failure::Initialization;
  SECTION("initialization failure") { failure = Failure::Initialization; }
  SECTION("encryption failure") { failure = Failure::Encryption; }
  Host host;
  const auto project = host.make_project();
  REQUIRE(project.project_key_id.has_value());
  const auto key =
      holder::privacy::load_project_key_bytes(project.project_id, *project.project_key_id);
  const auto original_token = host.export_token(project.project_id);
  holder::git::GitRepo repository;
  repository.open_existing(project.root_path);
  const auto head = repository.head_oid();
  const holder_recovery_token_export_request request{"1234"};
  const holder_project_recovery_token_export_result* result = nullptr;
  {
    InjectFailure fault(failure);
    CHECK(
        holder_export_project_recovery_token(
            &host.context,
            project.project_id.c_str(),
            &request,
            host.arena.get(),
            &result,
            &host.error
        ) == HOLDER_ERROR_DATA_CORRUPTED
    );
    CHECK(fault.consumed());
  }
  CHECK(result == nullptr);
  REQUIRE(host.error != nullptr);
  CHECK(std::string(holder_error_message(host.error)) == "recovery token cryptography failed");
  const auto stored = holder::project::ProjectRepo(host.db).get(project.project_id);
  REQUIRE(stored.has_value());
  CHECK(stored->project_key_id == project.project_key_id);
  CHECK(stored->updated_at == project.updated_at);
  CHECK(
      holder::privacy::load_project_key_bytes(project.project_id, *project.project_key_id) == key
  );
  CHECK(repository.head_oid() == head);
  CHECK(
      holder::privacy::inspect_recovery_token("1234", original_token).project_id ==
      project.project_id
  );
  holder_error_destroy(host.error);
  host.error = nullptr;
  CHECK_FALSE(host.export_token(project.project_id).empty());
}

TEST_CASE(
    "Recovery imports reject crypto initialization failure before changing project state",
    "[crypto-dependency]"
) {
  bool global = false;
  SECTION("existing project import") { global = false; }
  SECTION("global import into an empty target") { global = true; }
  Host source;
  const auto project = source.make_project();
  REQUIRE(project.project_key_id.has_value());
  const auto key =
      holder::privacy::load_project_key_bytes(project.project_id, *project.project_key_id);
  const auto token = source.export_token(project.project_id);
  holder::git::GitRepo repository;
  repository.open_existing(project.root_path);
  const auto head = repository.head_oid();
  Host target;
  const holder_recovery_token_import_request request{"1234", token.c_str()};
  const holder_recovery_token_global_import_result* imported = nullptr;
  const holder_project_recovery_token_import_result* local = nullptr;
  Host& host = global ? target : source;
  {
    InjectFailure fault(Failure::Initialization);
    const int status = global ? holder_import_recovery_token_global(
                                    &host.context,
                                    &request,
                                    host.arena.get(),
                                    &imported,
                                    &host.error
                                )
                              : holder_import_project_recovery_token(
                                    &host.context,
                                    project.project_id.c_str(),
                                    &request,
                                    host.arena.get(),
                                    &local,
                                    &host.error
                                );
    CHECK(status == HOLDER_ERROR_RECOVERY_TOKEN_INVALID);
    CHECK(fault.consumed());
  }
  CHECK(imported == nullptr);
  CHECK(local == nullptr);
  REQUIRE(host.error != nullptr);
  const auto stored = holder::project::ProjectRepo(source.db).get(project.project_id);
  REQUIRE(stored.has_value());
  CHECK(stored->project_key_id == project.project_key_id);
  CHECK(stored->updated_at == project.updated_at);
  CHECK(
      holder::privacy::load_project_key_bytes(project.project_id, *project.project_key_id) == key
  );
  CHECK(repository.head_oid() == head);
  CHECK(holder::project::ProjectRepo(target.db).list().empty());
  holder_error_destroy(host.error);
  host.error = nullptr;
  if (global) {
    REQUIRE(
        holder_import_recovery_token_global(
            &host.context,
            &request,
            host.arena.get(),
            &imported,
            &host.error
        ) == HOLDER_OK
    );
    REQUIRE(imported != nullptr);
    CHECK(std::string(imported->project_id) == project.project_id);
    CHECK(imported->project_created);
  } else {
    REQUIRE(
        holder_import_project_recovery_token(
            &host.context,
            project.project_id.c_str(),
            &request,
            host.arena.get(),
            &local,
            &host.error
        ) == HOLDER_OK
    );
    REQUIRE(local != nullptr);
    CHECK(std::string(local->project_id) == project.project_id);
  }
  CHECK(host.error == nullptr);
  const auto restored = holder::project::ProjectRepo(host.db).get(project.project_id);
  REQUIRE(restored.has_value());
  CHECK(restored->project_key_id == project.project_key_id);
  CHECK(
      holder::privacy::load_project_key_bytes(project.project_id, *restored->project_key_id) == key
  );
}
