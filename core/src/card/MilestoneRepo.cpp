#include "card/MilestoneRepo.h"

#include <sqlite3.h>

#include <stdexcept>
#include <unordered_set>

namespace holder::card {
namespace {

void throw_sqlite(sqlite3* db, const std::string& what) {
  const char* msg = db ? sqlite3_errmsg(db) : "unknown sqlite error";
  throw std::runtime_error(what + ": " + msg);
}

void bind_text(sqlite3_stmt* stmt, int idx, const std::string& value) {
  if (sqlite3_bind_text(stmt, idx, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    throw std::runtime_error("sqlite bind_text failed"); // LCOV_EXCL_LINE
  }
}

void bind_text_optional(sqlite3_stmt* stmt, int idx, const std::optional<std::string>& value) {
  if (value.has_value()) {
    bind_text(stmt, idx, value.value());
  } else if (sqlite3_bind_null(stmt, idx) != SQLITE_OK) {
    throw std::runtime_error("sqlite bind_null failed"); // LCOV_EXCL_LINE
  }
}

void bind_int64(sqlite3_stmt* stmt, int idx, long long value) {
  if (sqlite3_bind_int64(stmt, idx, static_cast<sqlite3_int64>(value)) != SQLITE_OK) {
    throw std::runtime_error("sqlite bind_int64 failed"); // LCOV_EXCL_LINE
  }
}

holder::model::Milestone read_milestone(sqlite3_stmt* stmt) {
  holder::model::Milestone milestone;
  milestone.milestone_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
  milestone.project_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
  milestone.card_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
  if (sqlite3_column_type(stmt, 3) == SQLITE_NULL) {
    milestone.kind.reset();
  } else {
    milestone.kind = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
  }
  if (sqlite3_column_type(stmt, 4) == SQLITE_NULL) {
    milestone.description.reset();
  } else {
    milestone.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
  }
  milestone.created_at = sqlite3_column_int64(stmt, 5);
  milestone.updated_at = sqlite3_column_int64(stmt, 6);
  return milestone;
} // LCOV_EXCL_LINE

void load_dates(sqlite3* db, holder::model::Milestone& milestone) {
  static constexpr const char* SQL =
      "SELECT role, date_value, datetime_value, legacy_all_day_at FROM milestone_dates "
      "WHERE milestone_id = ? ORDER BY role;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, SQL, -1, &stmt, nullptr) != SQLITE_OK)
    throw_sqlite(db, "prepare milestone dates failed");
  bind_text(stmt, 1, milestone.milestone_id);
  while (true) {
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) break;
    if (rc != SQLITE_ROW) {
      sqlite3_finalize(stmt);
      throw_sqlite(db, "list milestone dates failed");
    }
    holder::model::MilestoneDate value;
    value.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (sqlite3_column_type(stmt, 1) != SQLITE_NULL)
      value.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
      value.datetime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
      value.legacy_all_day_at = sqlite3_column_int64(stmt, 3);
    milestone.dates.push_back(std::move(value));
  }
  sqlite3_finalize(stmt);
}

} // namespace

MilestoneRepo::MilestoneRepo(holder::platform::Db& db)
    : db_(db) {}

void MilestoneRepo::replace_for_card(
    const std::string& project_id,
    const std::string& card_id,
    const std::vector<holder::model::Milestone>& milestones
) {
  for (const auto& milestone : milestones) {
    if (milestone.project_id != project_id || milestone.card_id != card_id)
      throw std::runtime_error("milestone project_id/card_id mismatch");
    std::unordered_set<std::string> roles;
    for (const auto& value : milestone.dates) {
      holder::model::validate_milestone_date(value);
      if (!roles.insert(value.role).second)
        throw std::invalid_argument("duplicate milestone date role: " + value.role);
    }
  }
  static constexpr const char* DELETE_DATES_SQL =
      "DELETE FROM milestone_dates WHERE milestone_id IN "
      "(SELECT milestone_id FROM milestones WHERE project_id = ? AND card_id = ?);";
  sqlite3_stmt* delete_dates_stmt = nullptr;
  if (sqlite3_prepare_v2(db_.handle(), DELETE_DATES_SQL, -1, &delete_dates_stmt, nullptr) !=
      SQLITE_OK)
    throw_sqlite(db_.handle(), "prepare delete milestone dates failed");
  bind_text(delete_dates_stmt, 1, project_id);
  bind_text(delete_dates_stmt, 2, card_id);
  const int delete_dates_rc = sqlite3_step(delete_dates_stmt);
  sqlite3_finalize(delete_dates_stmt);
  if (delete_dates_rc != SQLITE_DONE) throw_sqlite(db_.handle(), "delete milestone dates failed");

  static constexpr const char* DELETE_SQL =
      "DELETE FROM milestones WHERE project_id = ? AND card_id = ?;";
  sqlite3_stmt* delete_stmt = nullptr;
  if (sqlite3_prepare_v2(db_.handle(), DELETE_SQL, -1, &delete_stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(db_.handle(), "prepare delete milestones failed");
  }
  bind_text(delete_stmt, 1, project_id);
  bind_text(delete_stmt, 2, card_id);
  const int delete_rc = sqlite3_step(delete_stmt);
  sqlite3_finalize(delete_stmt);
  if (delete_rc != SQLITE_DONE) {
    throw_sqlite(db_.handle(), "delete milestones failed"); // LCOV_EXCL_LINE
  }

  if (milestones.empty()) {
    return;
  }

  static constexpr const char* INSERT_SQL =
      "INSERT INTO milestones(milestone_id, project_id, card_id, kind, description, "
      "created_at, updated_at) VALUES(?, ?, ?, ?, ?, ?, ?);";
  static constexpr const char* INSERT_DATE_SQL =
      "INSERT INTO milestone_dates(milestone_id, role, date_value, datetime_value, "
      "legacy_all_day_at, at_epoch) VALUES(?, ?, ?, ?, ?, ?);";
  sqlite3_stmt* insert_stmt = nullptr;
  if (sqlite3_prepare_v2(db_.handle(), INSERT_SQL, -1, &insert_stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(db_.handle(), "prepare insert milestone failed"); // LCOV_EXCL_LINE
  }

  sqlite3_stmt* insert_date_stmt = nullptr;
  if (sqlite3_prepare_v2(db_.handle(), INSERT_DATE_SQL, -1, &insert_date_stmt, nullptr) !=
      SQLITE_OK) {
    sqlite3_finalize(insert_stmt);
    throw_sqlite(db_.handle(), "prepare insert milestone date failed");
  }

  for (const auto& milestone : milestones) {

    sqlite3_reset(insert_stmt);
    sqlite3_clear_bindings(insert_stmt);

    bind_text(insert_stmt, 1, milestone.milestone_id);
    bind_text(insert_stmt, 2, milestone.project_id);
    bind_text(insert_stmt, 3, milestone.card_id);
    bind_text_optional(insert_stmt, 4, milestone.kind);
    bind_text_optional(insert_stmt, 5, milestone.description);
    bind_int64(insert_stmt, 6, milestone.created_at);
    bind_int64(insert_stmt, 7, milestone.updated_at);

    const int rc = sqlite3_step(insert_stmt);
    if (rc != SQLITE_DONE) {
      const std::string error = sqlite3_errmsg(db_.handle());
      sqlite3_finalize(insert_stmt);
      sqlite3_finalize(insert_date_stmt);
      throw std::runtime_error("insert milestone failed: " + error);
    }
    for (const auto& value : milestone.dates) {
      sqlite3_reset(insert_date_stmt);
      sqlite3_clear_bindings(insert_date_stmt);
      bind_text(insert_date_stmt, 1, milestone.milestone_id);
      bind_text(insert_date_stmt, 2, value.role);
      bind_text_optional(insert_date_stmt, 3, value.date);
      bind_text_optional(insert_date_stmt, 4, value.datetime);
      if (value.legacy_all_day_at)
        bind_int64(insert_date_stmt, 5, *value.legacy_all_day_at);
      else
        sqlite3_bind_null(insert_date_stmt, 5);
      if (value.datetime)
        bind_int64(insert_date_stmt, 6, holder::model::milestone_datetime_epoch(*value.datetime));
      else if (value.legacy_all_day_at)
        bind_int64(insert_date_stmt, 6, *value.legacy_all_day_at);
      else
        sqlite3_bind_null(insert_date_stmt, 6);
      if (sqlite3_step(insert_date_stmt) != SQLITE_DONE) {
        const std::string error = sqlite3_errmsg(db_.handle());
        sqlite3_finalize(insert_stmt);
        sqlite3_finalize(insert_date_stmt);
        throw std::runtime_error("insert milestone date failed: " + error);
      }
    }
  }

  sqlite3_finalize(insert_stmt);
  sqlite3_finalize(insert_date_stmt);
}

void MilestoneRepo::delete_for_card(const std::string& project_id, const std::string& card_id) {
  static constexpr const char* DATES_SQL =
      "DELETE FROM milestone_dates WHERE milestone_id IN "
      "(SELECT milestone_id FROM milestones WHERE project_id = ? AND card_id = ?);";
  sqlite3_stmt* dates_stmt = nullptr;
  if (sqlite3_prepare_v2(db_.handle(), DATES_SQL, -1, &dates_stmt, nullptr) != SQLITE_OK)
    throw_sqlite(db_.handle(), "prepare delete milestone dates failed");
  bind_text(dates_stmt, 1, project_id);
  bind_text(dates_stmt, 2, card_id);
  const int dates_rc = sqlite3_step(dates_stmt);
  sqlite3_finalize(dates_stmt);
  if (dates_rc != SQLITE_DONE) throw_sqlite(db_.handle(), "delete milestone dates failed");
  static constexpr const char* SQL = "DELETE FROM milestones WHERE project_id = ? AND card_id = ?;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_.handle(), SQL, -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(db_.handle(), "prepare delete milestones failed");
  }
  bind_text(stmt, 1, project_id);
  bind_text(stmt, 2, card_id);
  const int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  if (rc != SQLITE_DONE) {
    throw_sqlite(db_.handle(), "delete milestones failed"); // LCOV_EXCL_LINE
  }
}

std::vector<holder::model::Milestone> MilestoneRepo::list_for_card(
    const std::string& project_id,
    const std::string& card_id
) const {
  static constexpr const char* SQL =
      "SELECT milestone_id, project_id, card_id, kind, description, "
      "created_at, updated_at FROM milestones WHERE project_id = ? AND card_id = ? "
      "ORDER BY created_at ASC, milestone_id ASC;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_.handle(), SQL, -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(db_.handle(), "prepare list milestones for card failed");
  }
  bind_text(stmt, 1, project_id);
  bind_text(stmt, 2, card_id);

  std::vector<holder::model::Milestone> out;
  while (true) {
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
      auto milestone = read_milestone(stmt);
      load_dates(db_.handle(), milestone);
      out.push_back(std::move(milestone));
      continue;
    }
    if (rc == SQLITE_DONE) break;
    sqlite3_finalize(stmt); // LCOV_EXCL_LINE
    throw_sqlite(db_.handle(), "list milestones for card failed"); // LCOV_EXCL_LINE
  }
  sqlite3_finalize(stmt);
  return out;
} // LCOV_EXCL_LINE

std::vector<holder::model::Milestone> MilestoneRepo::list_in_range(
    const std::string& project_id,
    long long from,
    long long to
) const {
  return list_instants_in_range(project_id, "start", from, to);
}

std::vector<holder::model::Milestone> MilestoneRepo::list_instants_in_range(
    const std::string& project_id,
    const std::string& role,
    long long from,
    long long to
) const {
  if (role.empty()) throw std::invalid_argument("milestone date role must not be empty");
  static constexpr const char* SQL =
      "SELECT m.milestone_id, m.project_id, m.card_id, m.kind, m.description, "
      "m.created_at, m.updated_at "
      "FROM milestones m "
      "JOIN milestone_dates d ON d.milestone_id = m.milestone_id "
      "JOIN cards c ON c.card_id = m.card_id "
      "WHERE m.project_id = ? AND d.role = ? AND c.deleted_at IS NULL "
      "AND d.at_epoch >= ? AND d.at_epoch <= ? "
      "ORDER BY d.at_epoch ASC;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_.handle(), SQL, -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(db_.handle(), "prepare list milestones in range failed");
  }
  bind_text(stmt, 1, project_id);
  bind_text(stmt, 2, role);
  bind_int64(stmt, 3, from);
  bind_int64(stmt, 4, to);

  std::vector<holder::model::Milestone> out;
  while (true) {
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
      auto milestone = read_milestone(stmt);
      load_dates(db_.handle(), milestone);
      out.push_back(std::move(milestone));
      continue;
    }
    if (rc == SQLITE_DONE) break;
    sqlite3_finalize(stmt); // LCOV_EXCL_LINE
    throw_sqlite(db_.handle(), "list milestones in range failed"); // LCOV_EXCL_LINE
  }
  sqlite3_finalize(stmt);
  return out;
} // LCOV_EXCL_LINE

std::vector<holder::model::Milestone> MilestoneRepo::list_days_in_range(
    const std::string& project_id,
    const std::string& role,
    const std::string& from,
    const std::string& to
) const {
  if (role.empty()) throw std::invalid_argument("milestone date role must not be empty");
  if (from.size() != 10 || to.size() != 10)
    throw std::invalid_argument("milestone day range must use YYYY-MM-DD");
  holder::model::milestone_calendar_day(from);
  holder::model::milestone_calendar_day(to);
  static constexpr const char* SQL =
      "SELECT m.milestone_id, m.project_id, m.card_id, m.kind, m.description, "
      "m.created_at, m.updated_at "
      "FROM milestones m "
      "JOIN milestone_dates d ON d.milestone_id = m.milestone_id "
      "JOIN cards c ON c.card_id = m.card_id "
      "WHERE m.project_id = ? AND d.role = ? AND c.deleted_at IS NULL "
      "AND d.date_value >= ? AND d.date_value <= ? "
      "ORDER BY d.date_value ASC;";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_.handle(), SQL, -1, &stmt, nullptr) != SQLITE_OK)
    throw_sqlite(db_.handle(), "prepare list milestone days failed");
  bind_text(stmt, 1, project_id);
  bind_text(stmt, 2, role);
  bind_text(stmt, 3, from);
  bind_text(stmt, 4, to);
  std::vector<holder::model::Milestone> out;
  while (true) {
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) break;
    if (rc != SQLITE_ROW) {
      sqlite3_finalize(stmt);
      throw_sqlite(db_.handle(), "list milestone days failed");
    }
    auto milestone = read_milestone(stmt);
    load_dates(db_.handle(), milestone);
    out.push_back(std::move(milestone));
  }
  sqlite3_finalize(stmt);
  return out;
}

} // namespace holder::card
