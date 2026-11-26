#pragma once
#include "platform/Db.h"

namespace holder::platform {

class Tx {
 public:
  enum class Mode { Write, Read };

  explicit Tx(Db& db);
  Tx(Db& db, Mode mode);
  ~Tx();

  Tx(const Tx&) = delete;
  Tx& operator=(const Tx&) = delete;

  void commit();

 private:
  Db& db_;
  bool committed_ = false;
};

} // namespace holder::platform
