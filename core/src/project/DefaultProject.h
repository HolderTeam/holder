#pragma once

#include "model/Project.h"
#include "platform/Db.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace holder::git {
class GitOps;
} // namespace holder::git

namespace holder::index {
class FtsIndexer;
} // namespace holder::index

namespace holder::project {

// The welcome card's title: WELCOME_CARD's first line without the heading's hashes.
std::string welcome_card_title();

// If any project already exists, does nothing and returns std::nullopt. Otherwise makes Home: an
// encrypted_git project named HOME_PROJECT_NAME holding one card whose content is WELCOME_CARD (see
// contract/interface/projects/home.py), and returns it. Callers in one process are serialized, so
// two contexts starting together make one Home.
std::optional<holder::model::Project> ensure_default_project(
    holder::platform::Db& db,
    const std::function<std::string()>& uuid_v4,
    const std::filesystem::path& projects_root,
    holder::index::FtsIndexer* fts = nullptr,
    holder::git::GitOps* git = nullptr
);

} // namespace holder::project
