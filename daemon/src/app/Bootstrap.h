#pragma once

#include "model/Project.h"

#include <optional>
#include <string>

namespace holder::index {
class FtsIndexer;
} // namespace holder::index

namespace holder::platform {
class Db;
} // namespace holder::platform

namespace holder::core {
struct Paths;
} // namespace holder::core

namespace holder::app {

std::string generate_uuid_v4();

// Recover an empty projection from roots still registered for automatic discovery.
void recover_existing_projects(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::core::Paths& paths,
    bool require_durable_manifest
);

// If no project exists yet, makes Home with the contract's welcome card (see
// ensure_home_project in contract/interface/projects/operations.py). Does nothing otherwise.
std::optional<holder::model::Project> bootstrap_default_home_project(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts
);

} // namespace holder::app
