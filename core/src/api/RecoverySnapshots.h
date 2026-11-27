#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

struct holder_context;

namespace holder::contract {

// Capture a project's recovery lines under one repository lock and database
// read transaction. Throws std::length_error if the result exceeds max_bytes.
std::string capture_recovery_snapshot(
    holder_context& context,
    const std::string& project_id,
    std::size_t max_bytes
);

// The sink receives one encoded line at a time while the capture is consistent.
void write_recovery_snapshot(
    holder_context& context,
    const std::string& project_id,
    const std::function<void(std::string_view)>& write
);

} // namespace holder::contract
