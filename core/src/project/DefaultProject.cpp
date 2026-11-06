#include "project/DefaultProject.h"

#include "api/Contract.h"
#include "card/CardStore.h"
#include "identity/Uuid.h"
#include "model/Card.h"
#include "project/ProjectRepo.h"
#include "project/ProjectStore.h"

#include <chrono>
#include <mutex>

namespace holder::project {
namespace {

long long now_epoch_seconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch()
  )
      .count();
}

std::mutex& home_mutex() {
  static std::mutex mutex;
  return mutex;
}

} // namespace

std::string welcome_card_title() {
  const std::string_view content = holder::contract::WELCOME_CARD;
  auto line = content.substr(0, content.find('\n'));
  line.remove_prefix(std::min(line.find_first_not_of("# \t"), line.size()));
  return line.empty() ? std::string(holder::contract::HOME_PROJECT_NAME) : std::string(line);
}

std::optional<holder::model::Project> ensure_default_project(
    holder::platform::Db& db,
    const std::function<std::string()>& uuid_v4,
    const std::filesystem::path& projects_root,
    holder::index::FtsIndexer* fts,
    holder::git::GitOps* git
) {
  std::lock_guard lock(home_mutex());
  ProjectRepo repo(db);
  if (!repo.list().empty()) {
    return std::nullopt;
  }

  holder::model::Project project;
  project.name = holder::contract::HOME_PROJECT_NAME;
  project.privacy_mode = "encrypted_git";
  project.id_scheme = holder::model::IdScheme::Uuid7;

  ProjectStore store(db, git);
  auto created = store.create(std::move(project), uuid_v4, projects_root);

  holder::card::CardStore card_store(db, fts, /*fs=*/nullptr, git);
  holder::model::Card welcome;
  welcome.card_id = holder::identity::generate_id(created.id_scheme);
  welcome.project_id = created.project_id;
  welcome.title = welcome_card_title();
  welcome.created_at = now_epoch_seconds();
  welcome.updated_at = welcome.created_at;
  card_store.create(welcome, std::string(holder::contract::WELCOME_CARD));

  return created;
}

} // namespace holder::project
