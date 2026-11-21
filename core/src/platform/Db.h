#pragma once
#include <sqlite3.h>

#include <filesystem>
#include <string>

namespace holder::platform {

// Whether opening a database may create its file. Only the owner that sets the database up may
// create it. Any other connection opened later must fail when the file is missing: SQLite would
// otherwise create an empty database at the path, next to the WAL and shared-memory files of the
// database that was there, which the owner's connection is still using.
enum class DbOpen { CreateIfMissing, ExistingOnly };

class Db {
 public:
  Db() = default;
  ~Db();

  Db(const Db&) = delete;
  Db& operator=(const Db&) = delete;

  Db(Db&& other) noexcept;
  Db& operator=(Db&& other) noexcept;

  // Opens the DB file, creating it when missing unless mode is ExistingOnly.
  void open(const std::filesystem::path& path, DbOpen mode = DbOpen::CreateIfMissing);

  // Close explicitly (optional; destructor does it).
  void close();

  // Execute a SQL string (may contain multiple statements).
  void exec(const std::string& sql);

  // Convenience: current DB path, if opened.
  const std::filesystem::path& path() const { return path_; }

  // Expose raw handle when needed.
  sqlite3* handle() const { return db_; }

 private:
  static void throw_sqlite(const std::string& what, int rc, sqlite3* db);

  sqlite3* db_ = nullptr;
  std::filesystem::path path_;
};

} // namespace holder::platform
