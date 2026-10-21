#include "api/AiThreadList.h"

#include "ai/AiThreadRepo.h"

#include <optional>
#include <string>

namespace holder::api {

AiThreadListResponse get_ai_threads(holder::platform::Db& db, std::string project_id) {
  AiThreadListResponse response{true, {}};
  for (const auto& thread : holder::ai::AiThreadRepo(db).list(project_id)) {
    response.data.push_back({
        thread.thread_id,
        thread.project_id,
        std::optional<std::optional<std::string>>{std::in_place, thread.card_id},
        thread.title,
        thread.created_at,
        thread.updated_at,
    });
  }
  return response;
}

} // namespace holder::api
