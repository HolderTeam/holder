#pragma once

#include <cstring>
#include <sqlite3.h>
#include <string>
#include <utility>

namespace holder::test {

// Apply another writer's state change immediately before a selected re-read.
// This exercises lock-time validation without thread scheduling or sleeps.
class SqliteReadRace {
 public:
  SqliteReadRace(sqlite3* db, std::string query, std::string id, int read, std::string mutation)
      : db_(db),
        query_(std::move(query)),
        id_("'" + id + "'"),
        read_(read),
        mutation_(std::move(mutation)) {
    sqlite3_trace_v2(db_, SQLITE_TRACE_STMT, trace, this);
  }
  ~SqliteReadRace() { sqlite3_trace_v2(db_, 0, nullptr, nullptr); }
  SqliteReadRace(const SqliteReadRace&) = delete;
  SqliteReadRace& operator=(const SqliteReadRace&) = delete;
  bool fired = false;
  int result = SQLITE_OK;

 private:
  static int trace(unsigned, void* raw, void* statement, void*) noexcept {
    auto& state = *static_cast<SqliteReadRace*>(raw);
    if (state.fired) return 0;
    auto* stmt = static_cast<sqlite3_stmt*>(statement);
    const char* sql = sqlite3_sql(stmt);
    if (!sql || !std::strstr(sql, state.query_.c_str())) return 0;
    char* expanded = sqlite3_expanded_sql(stmt);
    const bool matches = expanded && std::strstr(expanded, state.id_.c_str());
    sqlite3_free(expanded);
    if (matches && --state.read_ == 0) {
      state.fired = true;
      state.result = sqlite3_exec(state.db_, state.mutation_.c_str(), nullptr, nullptr, nullptr);
    }
    return 0;
  }
  sqlite3* db_;
  std::string query_;
  std::string id_;
  int read_;
  std::string mutation_;
};

} // namespace holder::test
