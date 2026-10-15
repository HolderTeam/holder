#include "card/LinkMutation.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "card/LinkRepo.h"
#include "resource/ResourceRepo.h"

#include <chrono>
#include <stdexcept>

namespace holder::card {

namespace {
long long now_seconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch()
  )
      .count();
}
} // namespace

void validate_link_target(
    holder::platform::Db& db,
    const std::string& project_id,
    const std::string& to_id,
    const std::string& to_type
) {
  if (to_id.empty()) throw std::invalid_argument("Missing to_card_id.");
  if (to_type == "card") {
    const auto target = CardRepo(db).get(to_id);
    if (!target) throw std::invalid_argument("Target card not found.");
    if (target->project_id != project_id)
      throw std::invalid_argument("Target card is in a different project.");
    if (target->deleted_at) throw std::invalid_argument("Target card is trashed.");
    return;
  }
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

void add_card_link(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::model::CardLink& link
) {
  const auto source = CardRepo(db).get(link.from_card_id);
  if (!source || source->deleted_at) throw std::invalid_argument("Live source card not found.");
  if (source->project_id != link.project_id)
    throw std::invalid_argument("Source card is in a different project.");
  if (link.kind.empty()) throw std::invalid_argument("kind must not be empty");
  validate_link_target(db, link.project_id, link.to_card_id, link.to_type);
  LinkRepo links(db);
  auto updated = link;
  for (const auto& existing : links.list_outgoing(link.project_id, link.from_card_id)) {
    if (existing.to_card_id == link.to_card_id && existing.to_type == link.to_type &&
        existing.kind == link.kind) {
      if (existing.label == link.label) return;
      updated.created_at = existing.created_at;
      break;
    }
  }
  links.upsert_links(link.project_id, link.from_card_id, {updated});
  CardStore(db, fts).update_links(link.from_card_id, now_seconds());
}

void remove_card_link(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const std::string& from_card_id,
    const std::string& to_id,
    const std::string& to_type,
    const std::string& kind
) {
  const auto source = CardRepo(db).get(from_card_id);
  if (!source || source->deleted_at) throw std::invalid_argument("Live source card not found.");
  if (to_id.empty() || kind.empty()) throw std::invalid_argument("to_id and kind are required");
  LinkRepo links(db);
  bool present = false;
  for (const auto& existing : links.list_outgoing(source->project_id, from_card_id)) {
    if (existing.to_card_id == to_id && existing.to_type == to_type && existing.kind == kind) {
      present = true;
      break;
    }
  }
  if (!present) return;
  links.delete_link(source->project_id, from_card_id, to_id, to_type, kind);
  CardStore(db, fts).update_links(from_card_id, now_seconds());
}

} // namespace holder::card
