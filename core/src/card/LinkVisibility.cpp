#include "card/LinkVisibility.h"

#include "ai/AiMessageRepo.h"
#include "card/CardRepo.h"

namespace holder::card {

bool link_target_visible(holder::platform::Db& db, const holder::model::CardLink& link) {
  if (link.to_type == "card") {
    if (link.target_project_id.has_value() && *link.target_project_id != link.project_id)
      return true;
    const auto target = CardRepo(db).get(link.to_card_id);
    return target.has_value() && !target->deleted_at.has_value();
  }
  if (link.to_type == "ai_message") {
    const auto target = holder::ai::AiMessageRepo(db, nullptr).get(link.to_card_id);
    return target.has_value() && !target->deleted_at.has_value();
  }
  return true;
}

bool backlink_source_visible(holder::platform::Db& db, const holder::model::CardLink& link) {
  if (const auto card = CardRepo(db).get(link.from_card_id)) return !card->deleted_at.has_value();
  if (const auto message = holder::ai::AiMessageRepo(db, nullptr).get(link.from_card_id)) {
    return !message->deleted_at.has_value();
  }
  return false;
}

} // namespace holder::card
