#include "card/LinkMutation.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/CardRepo.h"
#include "resource/ResourceRepo.h"

#include <stdexcept>

namespace holder::card {

void validate_link_target(
    holder::platform::Db& db,
    const std::string& project_id,
    const std::string& to_id,
    const std::string& to_type,
    const std::optional<std::string>& target_project_id
) {
  if (to_id.empty()) throw std::invalid_argument("Missing to_card_id.");
  if (target_project_id.has_value() && target_project_id->empty())
    throw std::invalid_argument("target_project_id must not be empty.");
  if (to_type == "card") {
    const auto effective_project_id = target_project_id.value_or(project_id);
    const auto target = CardRepo(db).get(to_id);
    if (effective_project_id != project_id &&
        (!target || target->project_id != effective_project_id))
      return; // An explicit external target can remain unresolved locally.
    if (!target) throw std::invalid_argument("Target card not found.");
    if (target->project_id != effective_project_id)
      throw std::invalid_argument("Target card is in a different project.");
    if (target->deleted_at) throw std::invalid_argument("Target card is trashed.");
    return;
  }
  if (target_project_id.has_value())
    throw std::invalid_argument("target_project_id is only supported for card targets.");
  if (to_type == "resource") {
    const auto target = holder::resource::ResourceRepo(db).get(to_id);
    if (!target) throw std::invalid_argument("Target resource not found.");
    if (target->project_id != project_id)
      throw std::invalid_argument("Target resource is in a different project.");
    return;
  }
  if (to_type == "ai_thread") {
    const auto target = holder::ai::AiThreadRepo(db).get(to_id);
    if (!target) throw std::invalid_argument("Target ai thread not found.");
    if (target->project_id != project_id)
      throw std::invalid_argument("Target ai thread is in a different project.");
    return;
  }
  if (to_type == "ai_message") {
    const auto message = holder::ai::AiMessageRepo(db, nullptr).get(to_id);
    if (!message) throw std::invalid_argument("Target ai message not found.");
    const auto thread = holder::ai::AiThreadRepo(db).get(message->thread_id);
    if (!thread) throw std::invalid_argument("Target ai message thread not found.");
    if (thread->project_id != project_id)
      throw std::invalid_argument("Target ai message is in a different project.");
    if (message->deleted_at) throw std::invalid_argument("Target ai message is trashed.");
    return;
  }
  throw std::invalid_argument("Unsupported to_type.");
}

} // namespace holder::card
