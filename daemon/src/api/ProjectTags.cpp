#include "api/ProjectTags.h"

#include "card/TagRepo.h"
#include "project/ProjectRepo.h"

namespace holder::api {

ProjectTagListResponse list_project_tags(holder::platform::Db& db, std::string project_id) {
  if (!holder::project::ProjectRepo(db).get(project_id).has_value()) throw ProjectNotFound{};

  holder::card::TagRepo tag_repo(db);
  ProjectTagListResponse response{true, {}};
  for (const auto& [tag, card_count] : tag_repo.list_project_tags(project_id)) {
    response.data.push_back({tag, card_count});
  }
  return response;
}

} // namespace holder::api
