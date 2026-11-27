#include "platform/Tx.h"

namespace holder::platform {

Tx::Tx(Db& db)
    : Tx(db, Mode::Write) {}

Tx::Tx(Db& db, Mode mode)
    : db_(db) {
  db_.exec(mode == Mode::Read ? "BEGIN DEFERRED;" : "BEGIN IMMEDIATE;");
}

Tx::~Tx() {
  if (!committed_) {
    try {
      db_.exec("ROLLBACK;");
    } catch (...) {
      const bool rollback_ignored = true;
      (void)rollback_ignored;
    }
  }
}

void Tx::commit() {
  db_.exec("COMMIT;");
  committed_ = true;
}

} // namespace holder::platform
