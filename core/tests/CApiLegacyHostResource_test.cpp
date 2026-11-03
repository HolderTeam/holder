#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#else
#include <catch2/catch.hpp>
#endif

// Host registrations remain until #364.
#include "capi_asset_helpers.h"
#include "core_test_helpers.h"
#include "model/Location.h"
#include "model/Project.h"
#include "platform/Db.h"
#include "privacy/PlatformKeyring.h"
#include "project/ProjectRepo.h"
#include "resource/LocationStore.h"
#include "resource/ResourceRepo.h"

#include <git2.h>
#include <holder/holder.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

std::string read_schema_sql() {
  std::ifstream in(SCHEMA_SQL_PATH);
  REQUIRE(in.is_open());
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// Seeds a project whose root_path is a real, writable directory (unlike
// seed_project's placeholder /tmp/home), for tests that need to actually
// perform git operations against it.
void seed_git_project(
    const std::filesystem::path& data_dir,
    const std::string& project_id,
    const std::filesystem::path& root_path,
    const std::optional<std::string>& remote_url,
    holder::model::IdScheme id_scheme = holder::model::IdScheme::Uuid4
) {
  const auto db_path = data_dir / "server" / "holder.db";
  std::filesystem::create_directories(db_path.parent_path());
  holder::platform::Db db;
  db.open(db_path);
  db.exec(read_schema_sql());

  holder::model::Project project;
  project.project_id = project_id;
  project.name = "Git Project";
  project.root_path = root_path.string();
  project.git_remote_url = remote_url;
  project.id_scheme = id_scheme;
  // Tests using this helper write plain (unencrypted) markdown directly to
  // the working tree, so this must not be the default "encrypted_git" --
  // that would (correctly) trip assert_encryption_push_safe.
  project.privacy_mode = "plain";
  project.created_at = 10;
  project.updated_at = 20;

  holder::project::ProjectRepo repo(db);
  repo.create(project);
}

void seed_location(
    const std::filesystem::path& data_dir,
    const std::string& project_id,
    const std::string& location_id,
    const std::string& name,
    const std::string& provider,
    std::map<std::string, std::string> configuration = {}
) {
  holder::platform::Db db;
  db.open(data_dir / "server" / "holder.db");
  holder::model::Location location;
  location.location_id = location_id;
  location.project_id = project_id;
  location.name = name;
  location.provider = provider;
  location.configuration = std::move(configuration);
  location.created_at = 10;
  location.updated_at = 10;
  holder::resource::LocationStore(db).put(location);
}

struct CreatedPhotoCard {
  std::string project_id;
  std::string card_id;
  std::string project_root;
};

CreatedPhotoCard create_photo_project_and_card(holder_context* context, holder_error** error) {
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  holder_new_project project_request{};
  project_request.name = "Photos";
  project_request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
  const holder_project* project = nullptr;
  REQUIRE(
      holder_create_project(context, &project_request, arena.get(), &project, error) == HOLDER_OK
  );
  CreatedPhotoCard created{project->project_id, {}, project->root_path};
  holder_new_card card_request{};
  card_request.title = "Trip";
  const holder_card* card = nullptr;
  REQUIRE(
      holder_create_card(
          context,
          created.project_id.c_str(),
          &card_request,
          arena.get(),
          &card,
          error
      ) == HOLDER_OK
  );
  created.card_id = card->card_id;
  return created;
}

std::string asset_placement_id(
    holder_context* context,
    const std::string& project_id,
    const std::string& resource_id
) {
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  holder_error* error = nullptr;
  const holder_resource* resource = nullptr;
  const int rc = holder_get_resource(
      context,
      project_id.c_str(),
      resource_id.c_str(),
      arena.get(),
      &resource,
      &error
  );
  INFO((error ? holder_error_message(error) : "no C API error"));
  REQUIRE(rc == HOLDER_OK);
  REQUIRE(resource->assets.count == 1);
  REQUIRE(resource->assets.items[0].placements.count == 1);
  const std::string id = resource->assets.items[0].placements.items[0].placement_id;
  holder_error_destroy(error);
  return id;
}

// A bare repo, unlike GitRepo::open_or_init, accepts a non-force push to its
// currently checked-out branch -- git refuses that against a working-tree repo.
void init_bare_repo(const std::filesystem::path& repo_path) {
  git_libgit2_init();
  std::filesystem::create_directories(repo_path.parent_path());
  git_repository* repo = nullptr;
  git_repository_init_options opts{};
  REQUIRE(git_repository_init_options_init(&opts, GIT_REPOSITORY_INIT_OPTIONS_VERSION) == 0);
  opts.flags = GIT_REPOSITORY_INIT_BARE | GIT_REPOSITORY_INIT_MKPATH;
  REQUIRE(git_repository_init_ext(&repo, repo_path.string().c_str(), &opts) == 0);
  git_repository_free(repo);
  git_libgit2_shutdown();
}

} // namespace

TEST_CASE("C API git_set_homedir validates its argument and applies process-wide", "[capi][git]") {
  holder_error* error = nullptr;
  REQUIRE(holder_git_set_homedir("", &error) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(error != nullptr);
  holder_error_destroy(error);

  error = nullptr;
  const auto dir = holder::test::make_temp_dir();
  REQUIRE(holder_git_set_homedir(dir.string().c_str(), &error) == HOLDER_OK);
  REQUIRE(error == nullptr);
}

namespace {

int fake_sign_ok(
    void*,
    const unsigned char*,
    size_t,
    unsigned char** out_der_sig,
    size_t* out_der_sig_len
) {
  auto* buf = static_cast<unsigned char*>(std::malloc(1));
  buf[0] = 0x00;
  *out_der_sig = buf;
  *out_der_sig_len = 1;
  return 0;
}

} // namespace

TEST_CASE(
    "C API git_set_ssh_signer validates arguments and destroys user_data exactly once on replace/destroy",
    "[capi][git]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  seed_git_project(data_dir, "project-1", data_dir / "repo", std::nullopt);

  holder_context* context = nullptr;
  holder_error* error = nullptr;
  const auto schema = read_schema_sql();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );

  const unsigned char pubkey[] = {0x01, 0x02, 0x03};

  SECTION("rejects a null sign_fn, still destroying user_data exactly once") {
    int destroy_count = 0;
    auto destroy = [](void* user_data) {
      *static_cast<int*>(user_data) += 1;
    };
    REQUIRE(
        holder_git_set_ssh_signer(
            context,
            "git",
            pubkey,
            sizeof(pubkey),
            nullptr,
            &destroy_count,
            destroy,
            &error
        ) == HOLDER_ERROR_INVALID_ARGUMENT
    );
    REQUIRE(destroy_count == 1);
  }

  SECTION("rejects an empty username, still destroying user_data exactly once") {
    int destroy_count = 0;
    auto destroy = [](void* user_data) {
      *static_cast<int*>(user_data) += 1;
    };
    REQUIRE(
        holder_git_set_ssh_signer(
            context,
            "",
            pubkey,
            sizeof(pubkey),
            fake_sign_ok,
            &destroy_count,
            destroy,
            &error
        ) == HOLDER_ERROR_INVALID_ARGUMENT
    );
    REQUIRE(destroy_count == 1);
  }

  SECTION(
      "destroy_user_data fires once when replaced, and once more for the replacement at context destroy"
  ) {
    int first_destroy_count = 0;
    int second_destroy_count = 0;

    auto destroy_first = [](void* user_data) {
      *static_cast<int*>(user_data) += 1;
    };
    auto destroy_second = [](void* user_data) {
      *static_cast<int*>(user_data) += 1;
    };

    REQUIRE(
        holder_git_set_ssh_signer(
            context,
            "git",
            pubkey,
            sizeof(pubkey),
            fake_sign_ok,
            &first_destroy_count,
            destroy_first,
            &error
        ) == HOLDER_OK
    );
    REQUIRE(first_destroy_count == 0);

    // Installing a second signer drops the last reference to the first,
    // running its destroy_user_data exactly once.
    REQUIRE(
        holder_git_set_ssh_signer(
            context,
            "git",
            pubkey,
            sizeof(pubkey),
            fake_sign_ok,
            &second_destroy_count,
            destroy_second,
            &error
        ) == HOLDER_OK
    );
    REQUIRE(first_destroy_count == 1);
    REQUIRE(second_destroy_count == 0);

    holder_context_destroy(context);
    context = nullptr;
    REQUIRE(second_destroy_count == 1);
    REQUIRE(first_destroy_count == 1); // unchanged
  }

  holder_error_destroy(error);
  if (context != nullptr) {
    holder_context_destroy(context);
  }
}

TEST_CASE(
    "C API git_set_ssh_signer's provider is actually wired into git operations",
    "[capi][git]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  const auto remote_dir = data_dir / "remote";
  init_bare_repo(remote_dir);
  const char* project_id = "4b25e0e4-a460-4f48-8e3d-756c392724b7";
  seed_git_project(data_dir, project_id, data_dir / "repo", remote_dir.string());

  holder_context* context = nullptr;
  holder_error* error = nullptr;
  const auto schema = read_schema_sql();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );

  const unsigned char pubkey[] = {0x01, 0x02, 0x03};
  REQUIRE(
      holder_git_set_ssh_signer(
          context,
          "git",
          pubkey,
          sizeof(pubkey),
          fake_sign_ok,
          nullptr,
          nullptr,
          &error
      ) == HOLDER_OK
  );

  // A local-path remote never calls the signer, but probes the installed provider path.
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  const holder_remote_test remote{};
  const holder_remote_test_result* result = nullptr;
  const int rc =
      holder_test_project_remote(context, project_id, &remote, arena.get(), &result, &error);
  INFO((error ? holder_error_message(error) : "no C API error"));
  REQUIRE(rc == HOLDER_OK);
  REQUIRE(result->status == HOLDER_REMOTE_TEST_RESULT_STATUS_REACHABLE);

  holder_context_destroy(context);
}

namespace {

// A fake external keyring provider for the C ABI, backed by plain in-memory
// state passed as user_data -- exactly the shape Android's JNI bridge to
// EncryptedSharedPreferences plays in production.
struct FakeKeyringState {
  std::map<std::string, std::string> secrets;
  std::vector<std::string> observed_project_ids;
};

void record_project_id(FakeKeyringState& state, const char* project_id) {
  state.observed_project_ids.emplace_back(project_id != nullptr ? project_id : "");
}

int fake_keyring_lookup(
    void* user_data,
    int /*kind*/,
    const char* /*service*/,
    const char* account,
    const char* project_id,
    int* out_found,
    char** out_secret,
    char** /*out_error*/
) {
  auto* state = static_cast<FakeKeyringState*>(user_data);
  record_project_id(*state, project_id);
  const auto it = state->secrets.find(account);
  if (it == state->secrets.end()) {
    *out_found = 0;
    return 0;
  }
  *out_found = 1;
  auto* buf = static_cast<char*>(std::malloc(it->second.size() + 1));
  std::memcpy(buf, it->second.c_str(), it->second.size() + 1);
  *out_secret = buf;
  return 0;
}

int fake_keyring_store(
    void* user_data,
    int /*kind*/,
    const char* /*service*/,
    const char* account,
    const char* project_id,
    const char* /*label*/,
    const char* secret,
    char** /*out_error*/
) {
  auto* state = static_cast<FakeKeyringState*>(user_data);
  record_project_id(*state, project_id);
  state->secrets[account] = secret;
  return 0;
}

int fake_keyring_remove(
    void* user_data,
    int /*kind*/,
    const char* /*service*/,
    const char* account,
    const char* project_id,
    char** /*out_error*/
) {
  auto* state = static_cast<FakeKeyringState*>(user_data);
  record_project_id(*state, project_id);
  state->secrets.erase(account);
  return 0;
}

void noop_keyring_destroy(void*) {}

char* malloc_copy(const char* text) {
  const size_t len = std::strlen(text);
  auto* buf = static_cast<char*>(std::malloc(len + 1));
  std::memcpy(buf, text, len + 1);
  return buf;
}

// "_with_message" variants set *out_error; "_silent" variants report failure (rc != 0) without
// setting it, forcing the CApiKeyringProviderHandle wrapper's fallback message.
int failing_keyring_lookup_with_message(
    void*,
    int,
    const char*,
    const char*,
    const char*,
    int*,
    char**,
    char** out_error
) {
  *out_error = malloc_copy("lookup exploded");
  return 1;
}
int failing_keyring_lookup_silent(
    void*,
    int,
    const char*,
    const char*,
    const char*,
    int*,
    char**,
    char**
) {
  return 1;
}
int failing_keyring_store_with_message(
    void*,
    int,
    const char*,
    const char*,
    const char*,
    const char*,
    const char*,
    char** out_error
) {
  *out_error = malloc_copy("store exploded");
  return 1;
}
int failing_keyring_store_silent(
    void*,
    int,
    const char*,
    const char*,
    const char*,
    const char*,
    const char*,
    char**
) {
  return 1;
}
int failing_keyring_remove_with_message(
    void*,
    int,
    const char*,
    const char*,
    const char*,
    char** out_error
) {
  *out_error = malloc_copy("remove exploded");
  return 1;
}
int failing_keyring_remove_silent(void*, int, const char*, const char*, const char*, char**) {
  return 1;
}

} // namespace

TEST_CASE(
    "C API keyring_set_provider validates arguments, still destroying user_data exactly once",
    "[capi][privacy]"
) {
  int destroy_count = 0;
  auto destroy = [](void* user_data) {
    *static_cast<int*>(user_data) += 1;
  };
  holder_error* error = nullptr;

  REQUIRE(
      holder_keyring_set_provider(
          nullptr,
          fake_keyring_store,
          fake_keyring_remove,
          &destroy_count,
          destroy,
          &error
      ) == HOLDER_ERROR_INVALID_ARGUMENT
  );
  REQUIRE(destroy_count == 1);
  holder_error_destroy(error);

  holder::privacy::platform_keyring_clear_external_provider();
}

TEST_CASE(
    "C API keyring_set_provider destroys user_data exactly once on replace and on clear",
    "[capi][privacy]"
) {
  int first_destroy_count = 0;
  int second_destroy_count = 0;
  auto destroy_first = [](void* user_data) {
    *static_cast<int*>(user_data) += 1;
  };
  auto destroy_second = [](void* user_data) {
    *static_cast<int*>(user_data) += 1;
  };
  holder_error* error = nullptr;

  // lookup_fn/store_fn/remove_fn are never actually invoked in this test (nothing here
  // calls platform_keyring_lookup/store/remove_secret), so passing fake_keyring_lookup/
  // store/remove here is only about satisfying holder_keyring_set_provider's non-null
  // validation -- user_data is exercised solely by the destroy_* callbacks below.
  REQUIRE(
      holder_keyring_set_provider(
          fake_keyring_lookup,
          fake_keyring_store,
          fake_keyring_remove,
          &first_destroy_count,
          destroy_first,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(first_destroy_count == 0);

  REQUIRE(
      holder_keyring_set_provider(
          fake_keyring_lookup,
          fake_keyring_store,
          fake_keyring_remove,
          &second_destroy_count,
          destroy_second,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(first_destroy_count == 1);
  REQUIRE(second_destroy_count == 0);

  holder::privacy::platform_keyring_clear_external_provider();
  REQUIRE(second_destroy_count == 1);
  REQUIRE(first_destroy_count == 1); // unchanged
}

TEST_CASE(
    "C API keyring_set_provider round-trips lookup/store/remove through the registered callbacks",
    "[capi][privacy]"
) {
  FakeKeyringState state;
  holder_error* error = nullptr;
  REQUIRE(
      holder_keyring_set_provider(
          fake_keyring_lookup,
          fake_keyring_store,
          fake_keyring_remove,
          &state,
          noop_keyring_destroy,
          &error
      ) == HOLDER_OK
  );

  const holder::privacy::PlatformKeyringSecretRef ref{
      .kind = holder::privacy::PlatformKeyringSecretKind::GenericSecret,
      .service = "holder.test",
      .account = "acct",
      .project_id = "project-1",
  };

  REQUIRE_FALSE(holder::privacy::platform_keyring_lookup_secret(ref).secret.has_value());

  holder::privacy::platform_keyring_store_secret(ref, "label", "secret-value");
  REQUIRE(state.secrets.at("acct") == "secret-value");
  REQUIRE(holder::privacy::platform_keyring_lookup_secret(ref).secret == "secret-value");

  holder::privacy::platform_keyring_remove_secret(ref);
  REQUIRE(state.secrets.find("acct") == state.secrets.end());
  REQUIRE(state.observed_project_ids == std::vector<std::string>(4, "project-1"));

  holder::privacy::platform_keyring_clear_external_provider();
}

TEST_CASE(
    "C API keyring_set_provider surfaces lookup/store/remove failures, with and without a message",
    "[capi][privacy]"
) {
  const holder::privacy::PlatformKeyringSecretRef ref{
      .kind = holder::privacy::PlatformKeyringSecretKind::GenericSecret,
      .service = "holder.test",
      .account = "acct",
      .project_id = std::nullopt,
  };
  holder_error* error = nullptr;

  SECTION("lookup failure with a message") {
    REQUIRE(
        holder_keyring_set_provider(
            failing_keyring_lookup_with_message,
            fake_keyring_store,
            fake_keyring_remove,
            nullptr,
            noop_keyring_destroy,
            &error
        ) == HOLDER_OK
    );
    const auto result = holder::privacy::platform_keyring_lookup_secret(ref);
    REQUIRE(result.error_message == "lookup exploded");
  }

  SECTION("lookup failure without a message falls back to a default") {
    REQUIRE(
        holder_keyring_set_provider(
            failing_keyring_lookup_silent,
            fake_keyring_store,
            fake_keyring_remove,
            nullptr,
            noop_keyring_destroy,
            &error
        ) == HOLDER_OK
    );
    const auto result = holder::privacy::platform_keyring_lookup_secret(ref);
    REQUIRE(result.error_message == "keyring lookup failed");
  }

  SECTION("store failure with a message throws with that message") {
    REQUIRE(
        holder_keyring_set_provider(
            fake_keyring_lookup,
            failing_keyring_store_with_message,
            fake_keyring_remove,
            nullptr,
            noop_keyring_destroy,
            &error
        ) == HOLDER_OK
    );
    REQUIRE_THROWS_WITH(
        holder::privacy::platform_keyring_store_secret(ref, "label", "secret"),
        Catch::Matchers::Equals("store exploded")
    );
  }

  SECTION("store failure without a message falls back to a default") {
    REQUIRE(
        holder_keyring_set_provider(
            fake_keyring_lookup,
            failing_keyring_store_silent,
            fake_keyring_remove,
            nullptr,
            noop_keyring_destroy,
            &error
        ) == HOLDER_OK
    );
    REQUIRE_THROWS_WITH(
        holder::privacy::platform_keyring_store_secret(ref, "label", "secret"),
        Catch::Matchers::Equals("keyring store failed")
    );
  }

  SECTION("remove failure with a message throws with that message") {
    REQUIRE(
        holder_keyring_set_provider(
            fake_keyring_lookup,
            fake_keyring_store,
            failing_keyring_remove_with_message,
            nullptr,
            noop_keyring_destroy,
            &error
        ) == HOLDER_OK
    );
    REQUIRE_THROWS_WITH(
        holder::privacy::platform_keyring_remove_secret(ref),
        Catch::Matchers::Equals("remove exploded")
    );
  }

  SECTION("remove failure without a message falls back to a default") {
    REQUIRE(
        holder_keyring_set_provider(
            fake_keyring_lookup,
            fake_keyring_store,
            failing_keyring_remove_silent,
            nullptr,
            noop_keyring_destroy,
            &error
        ) == HOLDER_OK
    );
    REQUIRE_THROWS_WITH(
        holder::privacy::platform_keyring_remove_secret(ref),
        Catch::Matchers::Equals("keyring remove failed")
    );
  }

  holder::privacy::platform_keyring_clear_external_provider();
}

namespace {

// A minimal StorageProvider stand-in for the tests below: objects are just files under
// `root`, named by their object_key (which already contains '/', so it doubles as a
// relative path). Exercises the same C callback shape a real Kotlin/JNI-backed provider
// would implement.
struct FakeStorageBackend {
  std::filesystem::path root;
  int put_calls = 0;
  int get_calls = 0;
  int exists_calls = 0;
  int remove_calls = 0;
};

std::filesystem::path fake_storage_object_path(
    FakeStorageBackend* backend,
    const std::string& object_key
) {
  const auto path = backend->root / object_key;
  std::filesystem::create_directories(path.parent_path());
  return path;
}

int fake_storage_put(
    void* user_data,
    const char* object_key,
    const char* staged_file_path,
    long long /*stored_size*/,
    const char* /*stored_sha256*/,
    int* /*out_error_code*/,
    char** /*out_error*/
) {
  auto* backend = static_cast<FakeStorageBackend*>(user_data);
  backend->put_calls++;
  std::filesystem::copy_file(
      staged_file_path,
      fake_storage_object_path(backend, object_key),
      std::filesystem::copy_options::overwrite_existing
  );
  return 0;
}

int fake_storage_get(
    void* user_data,
    const char* object_key,
    const char* destination_file_path,
    int* out_error_code,
    char** out_error
) {
  auto* backend = static_cast<FakeStorageBackend*>(user_data);
  backend->get_calls++;
  const auto path = backend->root / object_key;
  if (!std::filesystem::is_regular_file(path)) {
    *out_error_code = HOLDER_STORAGE_ERROR_INTEGRITY;
    *out_error = malloc_copy("object not found");
    return 1;
  }
  std::filesystem::copy_file(
      path,
      destination_file_path,
      std::filesystem::copy_options::overwrite_existing
  );
  return 0;
}

int fake_storage_exists(
    void* user_data,
    const char* object_key,
    int* out_exists,
    int* /*out_error_code*/,
    char** /*out_error*/
) {
  auto* backend = static_cast<FakeStorageBackend*>(user_data);
  backend->exists_calls++;
  *out_exists = std::filesystem::is_regular_file(backend->root / object_key) ? 1 : 0;
  return 0;
}

int fake_storage_remove(
    void* user_data,
    const char* object_key,
    int* /*out_error_code*/,
    char** /*out_error*/
) {
  auto* backend = static_cast<FakeStorageBackend*>(user_data);
  backend->remove_calls++;
  std::error_code ec;
  std::filesystem::remove(backend->root / object_key, ec);
  return 0;
}

int failing_storage_put_with_message(
    void* user_data,
    const char*,
    const char*,
    long long,
    const char*,
    int* out_error_code,
    char** out_error
) {
  *out_error_code = user_data == nullptr ? HOLDER_STORAGE_ERROR_CAPACITY
                                         : *static_cast<int*>(user_data);
  *out_error = malloc_copy("storage failure");
  return 1;
}

int fake_storage_exists_but_block_git_write(
    void* user_data,
    const char* object_key,
    int* out_exists,
    int*,
    char**
) {
  auto* backend = static_cast<FakeStorageBackend*>(user_data);
  backend->exists_calls++;
  *out_exists = std::filesystem::is_regular_file(backend->root / object_key) ? 1 : 0;
  return 0;
}

void noop_storage_destroy(void*) {}

std::string write_source_file(const std::filesystem::path& path, const std::string& content) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  out << content;
  out.close();
  return path.string();
}

} // namespace

TEST_CASE(
    "C API storage_provider_register validates arguments, still destroying user_data exactly once",
    "[capi][resource]"
) {
  int destroy_count = 0;
  auto destroy = [](void* user_data) {
    *static_cast<int*>(user_data) += 1;
  };
  holder_error* error = nullptr;

  SECTION("empty provider_name") {
    REQUIRE(
        holder_storage_provider_register(
            "",
            fake_storage_put,
            fake_storage_get,
            fake_storage_exists,
            fake_storage_remove,
            &destroy_count,
            destroy,
            &error
        ) == HOLDER_ERROR_INVALID_ARGUMENT
    );
  }

  SECTION("'local_directory' cannot be overridden") {
    REQUIRE(
        holder_storage_provider_register(
            "local_directory",
            fake_storage_put,
            fake_storage_get,
            fake_storage_exists,
            fake_storage_remove,
            &destroy_count,
            destroy,
            &error
        ) == HOLDER_ERROR_INVALID_ARGUMENT
    );
  }

  SECTION("null callback") {
    REQUIRE(
        holder_storage_provider_register(
            "google-drive",
            nullptr,
            fake_storage_get,
            fake_storage_exists,
            fake_storage_remove,
            &destroy_count,
            destroy,
            &error
        ) == HOLDER_ERROR_INVALID_ARGUMENT
    );
  }

  REQUIRE(destroy_count == 1);
  holder_error_destroy(error);
}

TEST_CASE(
    "C API storage_provider_register destroys user_data exactly once on replace",
    "[capi][resource]"
) {
  // The registry can retain a provider until process exit, even if an assertion fails.
  // Give each provider shared ownership of its counter instead of a pointer into this stack.
  auto first_destroy_count = std::make_shared<int>(0);
  auto second_destroy_count = std::make_shared<int>(0);
  auto destroy = [](void* user_data) {
    const std::unique_ptr<std::shared_ptr<int>> count(
        static_cast<std::shared_ptr<int>*>(user_data)
    );
    ++**count;
  };
  holder_error* error = nullptr;

  REQUIRE(
      holder_storage_provider_register(
          "storage-replace-test",
          fake_storage_put,
          fake_storage_get,
          fake_storage_exists,
          fake_storage_remove,
          new std::shared_ptr<int>(first_destroy_count),
          destroy,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(*first_destroy_count == 0);

  REQUIRE(
      holder_storage_provider_register(
          "storage-replace-test",
          fake_storage_put,
          fake_storage_get,
          fake_storage_exists,
          fake_storage_remove,
          new std::shared_ptr<int>(second_destroy_count),
          destroy,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(*first_destroy_count == 1);
  REQUIRE(*second_destroy_count == 0);
}

TEST_CASE(
    "C API asset_import_file/asset_retrieve round-trip through the built-in local provider",
    "[capi][resource]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = read_schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );

  const auto created = create_photo_project_and_card(context, &error);
  const auto& project_id = created.project_id;
  const auto& card_id = created.card_id;

  seed_location(data_dir, project_id, "loc-local-1", "Local", "local_directory");

  const auto source_path = write_source_file(data_dir / "source" / "photo.jpg", "fake jpeg bytes");

  nlohmann::json import_result;
  REQUIRE(
      holder::test::import_asset_file(
          context,
          project_id.c_str(),
          card_id.c_str(),
          "loc-local-1",
          source_path.c_str(),
          &import_result,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(import_result.at("duplicate_reused").get<bool>() == false);
  REQUIRE(import_result.at("link_created").get<bool>() == true);
  const auto resource_id = import_result.at("resource_id").get<std::string>();
  const auto asset_id = import_result.at("asset_id").get<std::string>();

  const auto placement_id = asset_placement_id(context, project_id, resource_id);

  const auto destination = (data_dir / "downloaded" / "photo.jpg").string();
  REQUIRE(
      holder::test::retrieve_asset_file(
          context,
          resource_id.c_str(),
          asset_id.c_str(),
          placement_id.c_str(),
          destination.c_str(),
          &error
      ) == HOLDER_OK
  );
  std::ifstream downloaded(destination, std::ios::binary);
  const std::string downloaded_content(
      (std::istreambuf_iterator<char>(downloaded)),
      std::istreambuf_iterator<char>()
  );
  REQUIRE(downloaded_content == "fake jpeg bytes");

  holder_context_destroy(context);
}

TEST_CASE(
    "C API asset_import_file/asset_retrieve round-trip through a registered storage provider",
    "[capi][resource]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = read_schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );

  FakeStorageBackend backend{.root = data_dir / "fake-drive"};
  REQUIRE(
      holder_storage_provider_register(
          "google-drive",
          fake_storage_put,
          fake_storage_get,
          fake_storage_exists,
          fake_storage_remove,
          &backend,
          noop_storage_destroy,
          &error
      ) == HOLDER_OK
  );

  const auto created = create_photo_project_and_card(context, &error);
  const auto& project_id = created.project_id;
  const auto& card_id = created.card_id;

  seed_location(
      data_dir,
      project_id,
      "loc-drive-1",
      "My Drive",
      "google-drive",
      {{"folder_id", "drive-folder-abc"}}
  );

  const auto source_path =
      write_source_file(data_dir / "source" / "photo.jpg", "drive-backed bytes");

  nlohmann::json import_result;
  REQUIRE(
      holder::test::import_asset_file(
          context,
          project_id.c_str(),
          card_id.c_str(),
          "loc-drive-1",
          source_path.c_str(),
          &import_result,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(backend.put_calls == 1);
  REQUIRE(backend.exists_calls >= 1);
  const auto resource_id = import_result.at("resource_id").get<std::string>();
  const auto asset_id = import_result.at("asset_id").get<std::string>();

  const auto placement_id = asset_placement_id(context, project_id, resource_id);

  const auto destination = (data_dir / "downloaded" / "photo.jpg").string();
  REQUIRE(
      holder::test::retrieve_asset_file(
          context,
          resource_id.c_str(),
          asset_id.c_str(),
          placement_id.c_str(),
          destination.c_str(),
          &error
      ) == HOLDER_OK
  );
  REQUIRE(backend.get_calls == 1);
  std::ifstream downloaded(destination, std::ios::binary);
  const std::string downloaded_content(
      (std::istreambuf_iterator<char>(downloaded)),
      std::istreambuf_iterator<char>()
  );
  REQUIRE(downloaded_content == "drive-backed bytes");

  holder_context_destroy(context);
}

TEST_CASE(
    "C API asset_import_file surfaces a registered provider's put failure",
    "[capi][resource]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = read_schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );

  int failure_code = HOLDER_STORAGE_ERROR_CAPACITY;
  REQUIRE(
      holder_storage_provider_register(
          "quota-limited",
          failing_storage_put_with_message,
          fake_storage_get,
          fake_storage_exists,
          fake_storage_remove,
          &failure_code,
          noop_storage_destroy,
          &error
      ) == HOLDER_OK
  );

  const auto created = create_photo_project_and_card(context, &error);
  const auto& project_id = created.project_id;
  const auto& card_id = created.card_id;

  seed_location(data_dir, project_id, "loc-quota-1", "Full Drive", "quota-limited");

  const auto source_path = write_source_file(data_dir / "source" / "photo.jpg", "won't fit");

  const int rc = holder::test::import_asset_file(
      context,
      project_id.c_str(),
      card_id.c_str(),
      "loc-quota-1",
      source_path.c_str(),
      nullptr,
      &error
  );
  REQUIRE(rc == HOLDER_ERROR_RUNTIME);
  REQUIRE(error != nullptr);
  REQUIRE(std::string(holder_error_message(error)).find("storage failure") != std::string::npos);
  holder_error_destroy(error);

  for (const int code : std::array{
           static_cast<int>(HOLDER_STORAGE_ERROR_AUTHENTICATION),
           static_cast<int>(HOLDER_STORAGE_ERROR_PERMISSION),
           static_cast<int>(HOLDER_STORAGE_ERROR_INTEGRITY),
           static_cast<int>(HOLDER_STORAGE_ERROR_CONFLICT),
           static_cast<int>(HOLDER_STORAGE_ERROR_INVALID_CONFIGURATION),
           static_cast<int>(HOLDER_STORAGE_ERROR_TRANSIENT),
           999,
       }) {
    failure_code = code;
    error = nullptr;
    REQUIRE(
        holder::test::import_asset_file(
            context,
            project_id.c_str(),
            card_id.c_str(),
            "loc-quota-1",
            source_path.c_str(),
            nullptr,
            &error
        ) == (code == HOLDER_STORAGE_ERROR_INTEGRITY ? HOLDER_ERROR_DATA_CORRUPTED
                                                     : HOLDER_ERROR_RUNTIME)
    );
    REQUIRE(error != nullptr);
    holder_error_destroy(error);
  }

  // Nothing should have been left behind for a resource that never actually got stored.
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  const holder_resource_list* resources = nullptr;
  error = nullptr;
  REQUIRE(
      holder_list_resources(context, project_id.c_str(), arena.get(), &resources, &error) ==
      HOLDER_OK
  );
  REQUIRE(resources->count == 0);

  holder_context_destroy(context);
}

TEST_CASE(
    "C API asset import removes a newly stored object when the Git write fails",
    "[capi][resource]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = read_schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );

  FakeStorageBackend backend{.root = data_dir / "rollback-provider"};
  REQUIRE(
      holder_storage_provider_register(
          "rollback-provider",
          fake_storage_put,
          fake_storage_get,
          fake_storage_exists_but_block_git_write,
          fake_storage_remove,
          &backend,
          noop_storage_destroy,
          &error
      ) == HOLDER_OK
  );

  const auto created = create_photo_project_and_card(context, &error);
  const auto& project_id = created.project_id;
  const auto& card_id = created.card_id;
  const auto& project_root = created.project_root;

  seed_location(data_dir, project_id, "loc-rollback-1", "Rollback provider", "rollback-provider");

  // A regular file where the resource directory must be makes the Git-backed manifest write
  // fail after the provider has accepted the bytes.
  write_source_file(std::filesystem::path(project_root) / "resources", "blocks resource directory");
  const auto source_path =
      write_source_file(data_dir / "source" / "photo.jpg", "stored then rolled back");
  nlohmann::json import_result;
  REQUIRE(
      holder::test::import_asset_file(
          context,
          project_id.c_str(),
          card_id.c_str(),
          "loc-rollback-1",
          source_path.c_str(),
          nullptr,
          &error
      ) == HOLDER_ERROR_RUNTIME
  );
  REQUIRE(backend.put_calls == 1);
  REQUIRE(backend.exists_calls == 1);
  REQUIRE(backend.remove_calls == 1);
  holder_error_destroy(error);

  holder_context_destroy(context);
}

TEST_CASE("C API asset_import_file reports an unregistered provider by name", "[capi][resource]") {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = read_schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );

  const auto created = create_photo_project_and_card(context, &error);
  const auto& project_id = created.project_id;
  const auto& card_id = created.card_id;

  seed_location(
      data_dir,
      project_id,
      "loc-unregistered-1",
      "Dropbox (not wired up yet)",
      "dropbox"
  );

  const auto source_path = write_source_file(data_dir / "source" / "photo.jpg", "bytes");

  const int rc = holder::test::import_asset_file(
      context,
      project_id.c_str(),
      card_id.c_str(),
      "loc-unregistered-1",
      source_path.c_str(),
      nullptr,
      &error
  );
  REQUIRE(rc == HOLDER_ERROR_RUNTIME);
  REQUIRE(error != nullptr);
  REQUIRE(std::string(holder_error_message(error)).find("dropbox") != std::string::npos);
  holder_error_destroy(error);

  holder_context_destroy(context);
}

// A seeded bundle exercises generated reads and asset retrieval error paths.
TEST_CASE(
    "C API generated resource reads and asset retrieval report missing data",
    "[capi][resource]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  seed_git_project(data_dir, "project-1", data_dir / "repo", std::nullopt);

  holder_context* context = nullptr;
  holder_error* error = nullptr;
  const auto schema = read_schema_sql();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  seed_location(data_dir, "project-1", "location-1234", "Family Assets", "local_directory");
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );

  holder::model::ResourceBundle bundle;
  bundle.resource = {
      "resource-1234",
      "project-1",
      "document",
      "Homework.pdf",
      {{"description", {"Revision notes"}}},
      20,
      20
  };
  holder::model::Asset asset;
  asset.asset_id = "asset-1234";
  asset.resource_id = "resource-1234";
  asset.original_filename = "Homework.pdf";
  asset.media_type = "application/pdf";
  asset.byte_size = 42;
  asset.plaintext_sha256 = std::string(64, 'a');
  asset.created_at = 20;
  asset.updated_at = 20;
  asset.placements.push_back(
      {"placement-1234",
       "asset-1234",
       "location-1234",
       "project-1/asset-1234.holderasset",
       "plain",
       42,
       std::string(64, 'a'),
       20}
  );
  bundle.assets.push_back(asset);
  {
    holder::platform::Db db;
    db.open(data_dir / "server" / "holder.db");
    holder::resource::ResourceRepo(db).put_bundle(bundle);
  }

  const holder_resource_list* resources = nullptr;
  REQUIRE(
      holder_list_resources(context, "project-1", arena.get(), &resources, &error) == HOLDER_OK
  );
  REQUIRE(resources->count == 1);
  const holder_resource* resource = nullptr;
  REQUIRE(
      holder_get_resource(context, "project-1", "resource-1234", arena.get(), &resource, &error) ==
      HOLDER_OK
  );
  REQUIRE(resource->assets.count == 1);
  REQUIRE(std::string(resource->assets.items[0].original_filename) == "Homework.pdf");

  REQUIRE(
      holder::test::retrieve_asset_file(
          context,
          "resource-1234",
          "missing-asset",
          "placement-1234",
          "/tmp/not-written",
          &error
      ) == HOLDER_ERROR_NOT_FOUND
  );
  holder_error_destroy(error);
  error = nullptr;
  REQUIRE(
      holder::test::retrieve_asset_file(
          context,
          "resource-1234",
          "asset-1234",
          "missing-placement",
          "/tmp/not-written",
          &error
      ) == HOLDER_ERROR_NOT_FOUND
  );
  holder_error_destroy(error);
  error = nullptr;

  {
    // A damaged projection may retain a placement whose location has disappeared.
    holder::platform::Db damaged;
    damaged.open(data_dir / "server" / "holder.db");
    damaged.exec("PRAGMA foreign_keys=OFF;");
    damaged.exec(
        "UPDATE asset_placements SET location_id = 'missing-location' WHERE placement_id = 'placement-1234';"
    );
    const auto destination = data_dir / "missing-location-download";
    REQUIRE(
        holder::test::retrieve_asset_file(
            context,
            "resource-1234",
            "asset-1234",
            "placement-1234",
            destination.string().c_str(),
            &error
        ) == HOLDER_ERROR_NOT_FOUND
    );
    REQUIRE(
        std::string(holder_error_message(error)) ==
        "storage location not found in project: missing-location"
    );
    REQUIRE_FALSE(std::filesystem::exists(destination));
    holder_error_destroy(error);
    error = nullptr;
    damaged.exec(
        "UPDATE asset_placements SET location_id = 'location-1234' WHERE placement_id = 'placement-1234';"
    );
  }

  REQUIRE(std::filesystem::exists(data_dir / "repo" / ".git"));

  holder_context_destroy(context);
}
