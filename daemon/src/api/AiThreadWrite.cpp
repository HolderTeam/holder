#include "api/AiThreadWrite.h"

#include "ai/AiThreadDurability.h"
#include "ai/AiThreadRepo.h"
#include "api/support/Time.h"
#include "card/CardRepo.h"
#include "project/ProjectRepo.h"

#include <utility>

namespace holder::api {
namespace {

// The schema's foreign key: a thread can only be attached to a card that exists.
void require_card(holder::platform::Db& db, const std::optional<std::string>& card_id) {
  if (card_id && !holder::card::CardRepo(db).get(*card_id))
    throw std::invalid_argument("Card not found.");
}

} // namespace

AiThreadCreateResponse post_ai_threads(holder::platform::Db& db,
                                       const std::function<std::string()>& uuid_v4,
                                       AiThreadCreateRequest request) {
  holder::model::AiThread thread;
  thread.thread_id = request.thread_id.value_or("");
  if (thread.thread_id.empty()) thread.thread_id = uuid_v4();
  thread.project_id = std::move(request.project_id);
  thread.title = std::move(request.title);
  thread.card_id = request.card_id.value_or(std::nullopt);
  thread.created_at = request.created_at.value_or(0);
  if (thread.created_at <= 0) thread.created_at = support::now_epoch_seconds();
  thread.updated_at = request.updated_at.value_or(0);
  if (thread.updated_at <= 0) thread.updated_at = thread.created_at;

  if (!holder::project::ProjectRepo(db).get(thread.project_id))
    throw std::invalid_argument("Project not found.");
  require_card(db, thread.card_id);
  holder::ai::AiThreadRepo repo(db);
  if (repo.get(thread.thread_id)) throw holder::model::ConflictError("ai thread already exists");
  repo.create(thread);
  holder::ai::persist_ai_thread(db, thread, "Add AI thread metadata");
  return {true, {std::move(thread.thread_id)}};
}

AiThreadPatchResponse patch_ai_threads_thread_id(holder::platform::Db& db,
                                                 std::string thread_id,
                                                 AiThreadUpdateRequest request) {
  holder::ai::AiThreadRepo repo(db);
  if (!repo.get(thread_id)) throw AiThreadNotFound{};
  if (request.card_id) require_card(db, *request.card_id);
  if (request.title && *request.title)
    repo.update_title(thread_id, **request.title, request.updated_at);
  else
    repo.touch_updated(thread_id, request.updated_at);
  if (request.card_id) repo.update_card_id(thread_id, *request.card_id);
  holder::ai::persist_ai_thread(db, repo.get(thread_id).value());
  return {true, {std::move(thread_id)}};
}

AiThreadPatchResponse delete_ai_threads_thread_id(holder::platform::Db& db, std::string thread_id) {
  holder::ai::AiThreadRepo repo(db);
  const auto thread = repo.get(thread_id);
  if (!thread) throw AiThreadNotFound{};
  holder::ai::remove_ai_thread_manifest(db, *thread);
  repo.remove(thread_id);
  return {true, {std::move(thread_id)}};
}

} // namespace holder::api
