#pragma once

#include <cstddef>
#include <string>

struct holder_context;

namespace holder::contract {

// Capture a project's recovery lines under one repository lock and database
// read transaction. Throws std::length_error if the result exceeds max_bytes.
std::string capture_recovery_snapshot(
    holder_context& context,
    const std::string& project_id,
    std::size_t max_bytes
);

} // namespace holder::contract
