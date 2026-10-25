#pragma once

#include <stdexcept>
#include <string>

namespace holder::model {

// A write that would replace something that already exists, such as a record or file with the
// same ID. The message starts with "conflict:".
struct ConflictError : std::runtime_error {
  explicit ConflictError(const std::string& what) : std::runtime_error("conflict: " + what) {}
};

} // namespace holder::model
