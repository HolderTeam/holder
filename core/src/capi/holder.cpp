#include "holder/holder_api.h"

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

} // namespace

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
                       (health.health == holder::platform::DatabaseHealth::Missing && !roots.empty()
                       );
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

// The generated runtime error operations share core's holder_error type.
const char* holder::contract::error_message(const holder_error* error) noexcept {
  return error == nullptr ? "" : error->message.c_str();
}

void holder::contract::error_destroy(holder_error* error) noexcept { delete error; }
