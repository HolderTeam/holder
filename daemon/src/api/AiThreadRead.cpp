#include "api/AiThreadRead.h"

#include "ai/AiThreadRepo.h"

#include <optional>
#include <string>

namespace holder::api {
namespace {

AiThread thread_item(const holder::model::AiThread& thread) {
  return {
      thread.thread_id,
      thread.project_id,
      std::optional<std::optional<std::string>>{std::in_place, thread.card_id},
      thread.title,
      thread.created_at,
      thread.updated_at,
  };
}

} // namespace

AiThreadListResponse get_ai_threads(holder::platform::Db& db, std::string project_id) {
  AiThreadListResponse response{true, {}};
  for (const auto& thread : holder::ai::AiThreadRepo(db).list(project_id)) {
    response.data.push_back(thread_item(thread));
  }
  return response;
}

AiThreadGetResponse get_ai_thread(holder::platform::Db& db, std::string thread_id) {
  const auto thread = holder::ai::AiThreadRepo(db).get(thread_id);
  if (!thread) throw AiThreadNotFound{};
  return {true, thread_item(*thread)};
}

} // namespace holder::api
