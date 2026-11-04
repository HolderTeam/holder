#include "holder/holder.h"

#include "api/Contract.h"
#include "capi/Context.h"
#include "capi/StorageProviders.h"
#include "git/EcdsaDerSigningCredentialProvider.h"
#include "platform/DatabaseRebuild.h"
#include "platform/Migrations.h"
#include "platform/time_zone/TimeZones.h"
#include "privacy/PlatformKeyring.h"
#include "project/ProjectRepo.h"
#include "resource/LocalDirectoryProvider.h"
#include "resource/StorageProvider.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using holder::capi::rebuild_readiness_path;

bool looks_like_project(const std::filesystem::path& root) {
  return std::filesystem::is_directory(root) &&
         std::filesystem::is_regular_file(root / ".holder" / "project.json");
}

std::vector<std::filesystem::path> discover_managed_project_roots(
    const std::filesystem::path& data_dir
) {
  std::vector<std::filesystem::path> roots;
  const auto projects_dir = data_dir / "projects";
  if (!std::filesystem::is_directory(projects_dir)) return roots;
  for (const auto& entry : std::filesystem::directory_iterator(projects_dir)) {
    if (looks_like_project(entry.path())) roots.push_back(entry.path());
  }
  return roots;
}

holder::platform::DatabaseRebuildReport rebuild_managed_database(
    const std::filesystem::path& data_dir,
    const std::string& schema_sql,
    bool dry_run
) {
  holder::platform::DatabaseRebuildRequest request;
  request.database_path = data_dir / "server" / "holder.db";
  request.backup_root = data_dir / "server" / "database-backups";
  request.schema_sql = schema_sql;
  request.expected_schema_version = holder::platform::Migrations::latest_schema_version;
  request.project_roots = discover_managed_project_roots(data_dir);
  request.durable_ownership_ready = holder::platform::database_rebuild_is_ready(
      rebuild_readiness_path(data_dir)
  );
  request.dry_run = dry_run;
  auto report = holder::platform::rebuild_database_projection(request);
  if (!dry_run) {
    holder::platform::mark_database_rebuild_ready(rebuild_readiness_path(data_dir));
  }
  return report;
}

void clear_error(holder_error** out_error) {
  if (out_error != nullptr) {
    *out_error = nullptr;
  }
}

int set_error(holder_error** out_error, int code, std::string message) {
  if (out_error != nullptr) {
    try {
      *out_error = new holder_error{std::move(message)};
      // LCOV_EXCL_START
    } catch (...) {
      *out_error = nullptr;
      return HOLDER_ERROR_ALLOCATION;
    }
    // LCOV_EXCL_STOP
  }
  return code;
}

int set_exception(holder_error** out_error, const std::exception& e) {
  return set_error(out_error, HOLDER_ERROR_RUNTIME, e.what());
}

// LCOV_EXCL_START -- all production throw sites use std::exception-derived types; this is the
// C ABI's final defense against a foreign/non-standard exception crossing the boundary.
int set_unknown_exception(holder_error** out_error) {
  return set_error(out_error, HOLDER_ERROR_RUNTIME, "unknown holder error");
}
// LCOV_EXCL_STOP

holder::resource::StorageErrorCode storage_error_code_from_int(int value) {
  switch (value) {
  case HOLDER_STORAGE_ERROR_AUTHENTICATION:
    return holder::resource::StorageErrorCode::Authentication;
  case HOLDER_STORAGE_ERROR_PERMISSION:
    return holder::resource::StorageErrorCode::Permission;
  case HOLDER_STORAGE_ERROR_CAPACITY:
    return holder::resource::StorageErrorCode::Capacity;
  case HOLDER_STORAGE_ERROR_INTEGRITY:
    return holder::resource::StorageErrorCode::Integrity;
  case HOLDER_STORAGE_ERROR_CONFLICT:
    return holder::resource::StorageErrorCode::Conflict;
  case HOLDER_STORAGE_ERROR_INVALID_CONFIGURATION:
    return holder::resource::StorageErrorCode::InvalidConfiguration;
  case HOLDER_STORAGE_ERROR_TRANSIENT:
    return holder::resource::StorageErrorCode::Transient;
  default:
    return holder::resource::StorageErrorCode::Unavailable;
  }
}

// Owns a C-ABI storage provider's user_data/destroy_user_data pair while registered or
// retained by an in-flight operation -- see holder_storage_provider_register's ownership
// contract in holder.h. Implements holder::resource::StorageProvider by calling the C
// callbacks and translating a nonzero return into a typed StorageError, the same shape
// CApiKeyringProviderHandle uses for the keyring seam.
class CApiStorageProviderHandle final : public holder::resource::StorageProvider {
 public:
  CApiStorageProviderHandle(
      holder_storage_put_fn put_fn,
      holder_storage_get_fn get_fn,
      holder_storage_exists_fn exists_fn,
      holder_storage_remove_fn remove_fn,
      void* user_data,
      holder_destroy_fn destroy_user_data
  )
      : put_fn_(put_fn),
        get_fn_(get_fn),
        exists_fn_(exists_fn),
        remove_fn_(remove_fn),
        user_data_(user_data),
        destroy_user_data_(destroy_user_data) {}

  // The body is exercised; exclude GCC's separate, non-addressable deleting-destructor alias.
  ~CApiStorageProviderHandle() override { // LCOV_EXCL_LINE
    if (destroy_user_data_ != nullptr) destroy_user_data_(user_data_); // LCOV_EXCL_LINE
  } // LCOV_EXCL_LINE

  CApiStorageProviderHandle(const CApiStorageProviderHandle&) = delete;
  CApiStorageProviderHandle& operator=(const CApiStorageProviderHandle&) = delete;

  void put(
      const std::string& object_key,
      const std::filesystem::path& staged_file,
      long long stored_size,
      const std::string& stored_sha256
  ) override {
    int error_code = HOLDER_STORAGE_ERROR_UNAVAILABLE;
    char* error_ptr = nullptr;
    const int rc = put_fn_(
        user_data_,
        object_key.c_str(),
        staged_file.string().c_str(),
        stored_size,
        stored_sha256.c_str(),
        &error_code,
        &error_ptr
    );
    throw_if_failed(rc, error_code, error_ptr, "put");
  }

  void get(const std::string& object_key, const std::filesystem::path& destination_file) override {
    int error_code = HOLDER_STORAGE_ERROR_UNAVAILABLE;
    char* error_ptr = nullptr;
    const int rc = get_fn_(
        user_data_,
        object_key.c_str(),
        destination_file.string().c_str(),
        &error_code,
        &error_ptr
    );
    throw_if_failed(rc, error_code, error_ptr, "get");
  }

  bool exists(const std::string& object_key) override {
    int found = 0;
    int error_code = HOLDER_STORAGE_ERROR_UNAVAILABLE;
    char* error_ptr = nullptr;
    const int rc = exists_fn_(user_data_, object_key.c_str(), &found, &error_code, &error_ptr);
    throw_if_failed(rc, error_code, error_ptr, "exists");
    return found != 0;
  }

  void remove(const std::string& object_key) override {
    int error_code = HOLDER_STORAGE_ERROR_UNAVAILABLE;
    char* error_ptr = nullptr;
    const int rc = remove_fn_(user_data_, object_key.c_str(), &error_code, &error_ptr);
    throw_if_failed(rc, error_code, error_ptr, "remove");
  }

 private:
  static void throw_if_failed(int rc, int error_code, char* error_ptr, const char* op) {
    if (rc == 0) {
      if (error_ptr != nullptr) std::free(error_ptr); // LCOV_EXCL_LINE
      return;
    }
    std::string message =
        error_ptr != nullptr
            ? std::string(
                  error_ptr
              ) // LCOV_EXCL_LINE - callback failures without text use the documented fallback.
            : (std::string("storage provider ") + op + " failed");
    if (error_ptr != nullptr) std::free(error_ptr);
    throw holder::resource::StorageError(storage_error_code_from_int(error_code), message);
  }

  holder_storage_put_fn put_fn_;
  holder_storage_get_fn get_fn_;
  holder_storage_exists_fn exists_fn_;
  holder_storage_remove_fn remove_fn_;
  void* user_data_;
  holder_destroy_fn destroy_user_data_;
};

std::mutex& storage_provider_registry_mutex() {
  static std::mutex mutex;
  return mutex;
}

std::map<std::string, std::shared_ptr<CApiStorageProviderHandle>>& storage_provider_registry() {
  static std::map<std::string, std::shared_ptr<CApiStorageProviderHandle>> registry;
  return registry;
}

// "local_directory" is always available with no registration required, rooted per
// data_dir; every other provider name (e.g. "google-drive", "s3_compatible") must have
// been registered first via holder_storage_provider_register. Named to match
// holder-desktop's own local-storage Location provider string (see
// resources_tool_view.vala) -- Locations are portable/git-synced, so a client-specific
// name here would silently fail to resolve a Location another client created.
std::shared_ptr<holder::resource::StorageProvider> resolve_storage_provider(
    holder_context* context,
    const std::string& provider_name
) {
  if (provider_name == "local_directory") {
    return std::make_shared<holder::resource::LocalDirectoryProvider>(
        context->data_dir / "resource-store"
    );
  }
  std::lock_guard<std::mutex> lock(storage_provider_registry_mutex());
  auto it = storage_provider_registry().find(provider_name);
  if (it == storage_provider_registry().end()) {
    throw std::runtime_error("no storage provider registered for: " + provider_name);
  }
  return it->second;
}

} // namespace

namespace holder::capi {

std::shared_ptr<holder::resource::StorageProvider> storage_provider(
    holder_context& context,
    const std::string& provider_name
) {
  return resolve_storage_provider(&context, provider_name);
}

} // namespace holder::capi

// The generated C API (src/api/ContractCApi.cpp) reports errors through the same holder_error as
// the rest of the C API.
int holder::contract::detail::fail(holder_error** out_error, int code, const std::string& message) {
  return set_error(out_error, code, message);
}

// The contract's runtime operations (contract/interface/runtime): opening and releasing a context,
// and rebuilding a data directory's database with no context open.
holder_context* holder::contract::context_open(
    std::string data_dir,
    std::optional<std::string> schema_sql
) {
  auto context = std::make_unique<holder_context>();
  context->data_dir = std::filesystem::path(data_dir);
  context->db_path = context->data_dir / "server" / "holder.db";
  context->projects_root = context->data_dir / "projects";
  std::filesystem::create_directories(context->db_path.parent_path());

  const auto health = holder::platform::inspect_database_health(context->db_path);
  const auto roots = discover_managed_project_roots(context->data_dir);
  if (health.health == holder::platform::DatabaseHealth::IoError) {
    throw std::runtime_error("database I/O failure: " + health.detail);
  }
  bool needs_rebuild = health.health == holder::platform::DatabaseHealth::Corrupt ||
                       (health.health == holder::platform::DatabaseHealth::Missing &&
                        !roots.empty());
  // quick_check (what `health` reflects) only catches corruption, not staleness: a
  // structurally intact database left behind by an older build can still be on an
  // older schema_version than this one expects -- the one case this whole codebase's
  // callers actually hit is an in-place app update on Android, which has no other
  // migration path wired up (unlike the desktop daemon, which calls
  // Migrations::migrate_to_latest() directly from its own startup). Reuse the same
  // rebuild-from-durable-git-source-of-truth path already used for Corrupt/Missing
  // rather than running incremental migrations in place here: every future schema
  // bump gets this for free with no migration script of its own to write or forget,
  // and rebuild_database_projection's durable-object-count check means it fails loudly
  // instead of silently if anything doesn't come back the same.
  if (!needs_rebuild && health.health == holder::platform::DatabaseHealth::Healthy) {
    holder::platform::Db probe;
    probe.open(context->db_path);
    const bool stale = holder::platform::Migrations::read_schema_version(probe) <
                       holder::platform::Migrations::latest_schema_version;
    probe.close();
    needs_rebuild = stale;
  }
  if (needs_rebuild) {
    if (!schema_sql || schema_sql->empty()) {
      throw std::runtime_error("schema SQL is required to reconstruct the database");
    }
    rebuild_managed_database(context->data_dir, *schema_sql, false);
  }

  context->db.open(context->db_path);
  if (schema_sql && !schema_sql->empty()) {
    context->db.exec(*schema_sql);
  }
  try {
    if (holder::project::ProjectRepo(context->db).list().size() != roots.size()) {
      throw std::runtime_error("not every project is under the managed project directory");
    }
    holder::platform::audit_core_durable_ownership(
        context->db,
        context->db_path.parent_path() / "quarantined-cards.json"
    );
    holder::platform::mark_database_rebuild_ready(rebuild_readiness_path(context->data_dir));
  } catch (const std::exception&) {
    // A healthy legacy profile remains usable, but is deliberately not marked
    // reconstructible until its durable owners have been backfilled and audited.
    if (health.health != holder::platform::DatabaseHealth::Healthy) throw;
  }

  return context.release();
}

void holder::contract::context_destroy(holder_context* context) noexcept { delete context; }

holder::contract::DatabaseRebuildReport holder::contract::database_rebuild(
    std::string data_dir,
    std::string schema_sql,
    bool dry_run
) {
  const auto report =
      rebuild_managed_database(std::filesystem::path(data_dir), schema_sql, dry_run);
  const auto count = [](std::size_t value) {
    return static_cast<std::int32_t>(value);
  };
  return {
      report.dry_run,
      report.previous_health,
      count(report.projects),
      count(report.cards),
      count(report.ai_threads),
      count(report.ai_messages),
      count(report.resources),
      count(report.assets),
      count(report.placements),
      count(report.locations),
      count(report.quarantined_cards),
      report.backup_path.empty() ? std::nullopt
                                 : std::optional<std::string>(report.backup_path.string())
  };
}

int holder_storage_provider_register(
    const char* provider_name,
    holder_storage_put_fn put_fn,
    holder_storage_get_fn get_fn,
    holder_storage_exists_fn exists_fn,
    holder_storage_remove_fn remove_fn,
    void* user_data,
    holder_destroy_fn destroy_user_data,
    holder_error** out_error
) {
  clear_error(out_error);

  // Takes ownership of user_data unconditionally from this point on, exactly like
  // other host registrations -- constructing the RAII handle before any validation is
  // what makes every early return below (including a validation failure) still run
  // destroy_user_data exactly once, via normal scope-exit destruction.
  std::shared_ptr<CApiStorageProviderHandle> handle;
  try {
    handle = std::make_shared<CApiStorageProviderHandle>(
        put_fn,
        get_fn,
        exists_fn,
        remove_fn,
        user_data,
        destroy_user_data
    );
  } catch (const std::bad_alloc&) {
    // Allocation failed before the handle could take the unconditionally transferred data.
    if (destroy_user_data != nullptr) destroy_user_data(user_data);
    return set_error(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  }

  if (provider_name == nullptr || provider_name[0] == '\0') {
    return set_error(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "provider_name must not be empty");
  }
  if (std::strcmp(provider_name, "local_directory") == 0) {
    return set_error(
        out_error,
        HOLDER_ERROR_INVALID_ARGUMENT,
        "'local_directory' is a built-in provider name and cannot be overridden"
    );
  }
  if (put_fn == nullptr || get_fn == nullptr || exists_fn == nullptr || remove_fn == nullptr) {
    return set_error(
        out_error,
        HOLDER_ERROR_INVALID_ARGUMENT,
        "put_fn, get_fn, exists_fn, and remove_fn must not be null"
    );
  }

  try {
    {
      std::lock_guard<std::mutex> lock(storage_provider_registry_mutex());
      storage_provider_registry()[provider_name].swap(handle);
    }
    // Retire the previous provider outside the lock: its cleanup callback may reenter
    // registration. Active operations keep their own shared ownership until completion.
    return HOLDER_OK;
    // LCOV_EXCL_START
  } catch (const std::bad_alloc&) {
    return set_error(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  } catch (const std::exception& e) {
    return set_exception(out_error, e);
  } catch (...) {
    return set_unknown_exception(out_error);
  }
  // LCOV_EXCL_STOP
}

// The generated runtime error operations share core's holder_error type.
const char* holder::contract::error_message(const holder_error* error) noexcept {
  return error == nullptr ? "" : error->message.c_str();
}

void holder::contract::error_destroy(holder_error* error) noexcept { delete error; }
