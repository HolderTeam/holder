#pragma once

#include <optional>
#include <string>

namespace holder::platform {
class Db;
}

namespace holder::card {

// Shared target validation used by the daemon's AI-message link writes.
void validate_link_target(
    holder::platform::Db& db,
    const std::string& project_id,
    const std::string& to_id,
    const std::string& to_type,
    const std::optional<std::string>& target_project_id = std::nullopt
);

} // namespace holder::card
