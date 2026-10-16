#pragma once

#include "model/CardLink.h"

#include <string>

namespace holder::platform {
class Db;
}
namespace holder::index {
class FtsIndexer;
}

namespace holder::card {

// Card-sourced link mutations and their shared target validation.
void add_card_link(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::model::CardLink& link
);
void remove_card_link(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const std::string& from_card_id,
    const std::string& to_id,
    const std::string& to_type,
    const std::string& kind
);
void validate_link_target(
    holder::platform::Db& db,
    const std::string& project_id,
    const std::string& to_id,
    const std::string& to_type,
    const std::optional<std::string>& target_project_id = std::nullopt
);

} // namespace holder::card
