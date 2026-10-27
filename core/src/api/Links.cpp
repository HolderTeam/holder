// The contract's link operations (contract/interface/links.py). The generated glue checks their
// arguments first; these do the work.
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/CardRepo.h"
#include "card/LinkRepo.h"
#include "card/LinkVisibility.h"
#include "platform/Tx.h"
#include "project/ProjectRepo.h"
#include "resource/ResourceRepo.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace holder::contract {
namespace {

// The contract's target types, in its order; each enum of them lists them in that order.
constexpr std::array<const char*, 4> TARGET_TYPES{"card", "ai_message", "ai_thread", "resource"};

template <class Type>
std::string type_name(Type type) {
  return TARGET_TYPES.at(static_cast<std::size_t>(type));
}

LinkTargetType target_type(const std::string& name) {
  const auto at = std::find(TARGET_TYPES.begin(), TARGET_TYPES.end(), name);
  if (at == TARGET_TYPES.end()) throw Error("a link has an unknown target type: " + name);
  return static_cast<LinkTargetType>(at - TARGET_TYPES.begin());
}

// A card's source, or an AI message's, in the link's own project; nothing for a source that is gone.
struct Source {
  LinkSourceType type;
  bool deleted;
};

std::optional<Source> source_of(holder_context& context, const holder::model::CardLink& link) {
  if (const auto card = holder::card::CardRepo(context.db).get(link.from_card_id);
      card && card->project_id == link.project_id)
    return Source{LinkSourceType::Card, card->deleted_at.has_value()};
  const auto message = holder::ai::AiMessageRepo(context.db, nullptr).get(link.from_card_id);
  if (!message) return std::nullopt;
  const auto thread = holder::ai::AiThreadRepo(context.db).get(message->thread_id);
  if (!thread || thread->project_id != link.project_id) return std::nullopt;
  return Source{LinkSourceType::AiMessage, message->deleted_at.has_value()};
}

Link to_link(const holder::model::CardLink& link, LinkSourceType source_type) {
  return Link{link.project_id, source_type, link.from_card_id, link.target_project_id.value_or(link.project_id),
              target_type(link.to_type), link.to_card_id, link.kind, link.label, link.created_at};
}

std::vector<Link> outgoing(holder_context& context, const std::string& project_id, const std::string& card_id,
                           bool include_deleted) {
  std::vector<Link> out;
  for (const auto& link : holder::card::LinkRepo(context.db).list_outgoing(project_id, card_id)) {
    if (include_deleted || holder::card::link_target_visible(context.db, link))
      out.push_back(to_link(link, LinkSourceType::Card));
  }
  return out;
}

bool on_this_device(const holder::model::Project& project) {
  std::error_code error;
  return std::filesystem::is_directory(project.root_path, error);
}

std::vector<Link> backlinks(holder_context& context, const std::string& project_id, const std::string& card_id,
                            ListCardBacklinksScope scope, bool include_deleted) {
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  holder::card::LinkRepo repo(context.db);
  std::vector<holder::model::CardLink> links;
  if (scope != ListCardBacklinksScope::External) links = repo.list_backlinks_typed(project_id, card_id, "card");
  if (scope != ListCardBacklinksScope::Local) {
    std::vector<std::string> others;
    for (const auto& project : holder::project::ProjectRepo(context.db).list())
      if (project.project_id != project_id && on_this_device(project)) others.push_back(project.project_id);
    auto external = repo.list_external_backlinks_typed(project_id, card_id, "card", others);
    links.insert(links.end(), external.begin(), external.end());
  }
  std::sort(links.begin(), links.end(), [](const auto& left, const auto& right) {
    if (left.created_at != right.created_at) return left.created_at > right.created_at;
    if (left.project_id != right.project_id) return left.project_id < right.project_id;
    if (left.from_card_id != right.from_card_id) return left.from_card_id < right.from_card_id;
    return left.kind < right.kind;
  });
  std::vector<Link> out;
  for (const auto& link : links) {
    const auto source = source_of(context, link);
    if (source && (include_deleted || !source->deleted)) out.push_back(to_link(link, source->type));
  }
  return out;
}

// A target in the card's own project must be there and not in the trash. A card in another project
// may be one this device does not have, but one it has must not be in the trash.
void check_target(holder_context& context, holder::card::CardStore& store, const std::string& project_id,
                  const std::string& type, const std::string& id, const std::string& target_project_id) {
  const auto missing = [&] { return NotFound("link target not found: " + type + " " + id); };
  if (type == "card") {
    const auto card = card_in(store, target_project_id, id);
    if (target_project_id != project_id && !card) return;
    if (!card || card->deleted_at) throw missing();
    return;
  }
  if (target_project_id != project_id) throw InvalidArgument("only a card can be in another project");
  if (type == "resource") {
    const auto resource = holder::resource::ResourceRepo(context.db).get(id);
    if (!resource || resource->project_id != project_id) throw missing();
    return;
  }
  if (type == "ai_thread") {
    const auto thread = holder::ai::AiThreadRepo(context.db).get(id);
    if (!thread || thread->project_id != project_id) throw missing();
    return;
  }
  const auto message = holder::ai::AiMessageRepo(context.db, nullptr).get(id);
  const auto thread = message ? holder::ai::AiThreadRepo(context.db).get(message->thread_id) : std::nullopt;
  if (!thread || thread->project_id != project_id || message->deleted_at) throw missing();
}

// The card's link to that target, kind and project, when it has one.
std::optional<holder::model::CardLink> find_link(holder_context& context, const std::string& project_id,
                                                 const std::string& card_id, const std::string& type,
                                                 const std::string& id, const std::string& kind,
                                                 const std::string& target_project_id) {
  for (auto& link : holder::card::LinkRepo(context.db).list_outgoing(project_id, card_id)) {
    if (link.to_type == type && link.to_card_id == id && link.kind == kind &&
        link.target_project_id.value_or(link.project_id) == target_project_id)
      return std::move(link);
  }
  return std::nullopt;
}

// Writes the card's links to its file. The index changes with the file or not at all, so a write
// that fails, such as one an encrypted project's missing key stops, leaves both as they were.
template <class Change>
void write_links(holder_context& context, holder::card::CardStore& store, const std::string& card_id,
                 Change&& change) {
  holder::platform::Tx tx(context.db);
  change(holder::card::LinkRepo(context.db));
  store.update_links(card_id, now_epoch_seconds());
  tx.commit();
}

Link add(holder_context& context, const std::string& project_id, const std::string& card_id, const NewLink& request) {
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  const auto type = type_name(request.target_type);
  const auto target_project_id = request.target_project_id.value_or(project_id);
  check_target(context, store, project_id, type, request.target_id, target_project_id);

  holder::model::CardLink link;
  link.project_id = project_id;
  link.from_card_id = card_id;
  link.to_card_id = request.target_id;
  link.to_type = type;
  link.kind = request.kind;
  link.label = request.label;
  if (target_project_id != project_id) link.target_project_id = target_project_id;
  link.created_at = request.created_at.value_or(now_epoch_seconds());
  // A link that already exists keeps when it was made, and one whose label is unchanged is not written.
  if (const auto existing =
          find_link(context, project_id, card_id, type, request.target_id, request.kind, target_project_id)) {
    if (existing->label == link.label) return to_link(*existing, LinkSourceType::Card);
    link.created_at = existing->created_at;
  }
  write_links(context, store, card_id,
              [&](holder::card::LinkRepo repo) { repo.upsert_links(project_id, card_id, {link}); });
  return to_link(link, LinkSourceType::Card);
}

std::vector<Link> remove(holder_context& context, const std::string& project_id, const std::string& card_id,
                         RemoveCardLinkTargetType target_type, const std::string& target_id, const std::string& kind,
                         const std::optional<std::string>& target_project) {
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  const auto type = type_name(target_type);
  const auto target_project_id = target_project.value_or(project_id);
  if (find_link(context, project_id, card_id, type, target_id, kind, target_project_id)) {
    write_links(context, store, card_id, [&](holder::card::LinkRepo repo) {
      repo.delete_link_exact(project_id, card_id, target_id, type, kind, target_project_id);
    });
  }
  return outgoing(context, project_id, card_id, false);
}

std::vector<Link> links(holder_context& context, const std::string& project_id, const std::string& card_id,
                        bool include_deleted) {
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  return outgoing(context, project_id, card_id, include_deleted);
}

} // namespace

std::vector<Link> list_card_links(holder_context& context, std::string project_id, std::string card_id,
                                  bool include_deleted) {
  return with_contract_errors([&] { return links(context, project_id, card_id, include_deleted); });
}

std::vector<Link> list_card_backlinks(holder_context& context, std::string project_id, std::string card_id,
                                      ListCardBacklinksScope scope, bool include_deleted) {
  return with_contract_errors([&] { return backlinks(context, project_id, card_id, scope, include_deleted); });
}

Link add_card_link(holder_context& context, std::string project_id, std::string card_id, const NewLink& link) {
  return with_contract_errors([&] { return add(context, project_id, card_id, link); });
}

std::vector<Link> remove_card_link(holder_context& context, std::string project_id, std::string card_id,
                                   RemoveCardLinkTargetType target_type, std::string target_id, std::string kind,
                                   std::optional<std::string> target_project_id) {
  return with_contract_errors(
      [&] { return remove(context, project_id, card_id, target_type, target_id, kind, target_project_id); });
}

} // namespace holder::contract
