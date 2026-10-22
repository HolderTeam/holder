#include "api/CardLinks.h"

#include "api/support/Time.h"
#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "card/LinkMutation.h"
#include "card/LinkRepo.h"
#include "card/LinkVisibility.h"
#include "project/ProjectRepo.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace holder::api::routes {
namespace {

bool should_include_external_backlink_source(
    holder::card::CardRepo& card_repo,
    holder::ai::AiMessageRepo& msg_repo,
    holder::ai::AiThreadRepo& thread_repo,
    const holder::model::CardLink& link,
    bool include_deleted
) {
  const auto as_card = card_repo.get(link.from_card_id);
  if (as_card.has_value() && as_card->project_id == link.project_id) {
    return include_deleted || !as_card->deleted_at.has_value();
  }
  const auto as_msg = msg_repo.get(link.from_card_id);
  if (!as_msg.has_value()) return false;
  const auto thread = thread_repo.get(as_msg->thread_id);
  return thread.has_value() && thread->project_id == link.project_id &&
         (include_deleted || !as_msg->deleted_at.has_value());
}

bool project_is_available(const holder::model::Project& project) {
  // The bearer token covers registered projects; only projects with local roots join this view.
  std::error_code error;
  return std::filesystem::is_directory(project.root_path, error);
}

} // namespace

CardLinkListResponse list_card_links(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string card_id,
    std::optional<int> include_deleted
) {
  holder::card::LinkRepo links(db);
  holder::card::CardRepo card_repo(db);
  const auto source = card_repo.get(card_id);
  if (!source.has_value()) throw CardLinkSourceNotFound{};
  holder::ai::AiMessageRepo msg_repo(db, fts);
  CardLinkListResponse response{true, {}};
  for (const auto& link : links.list_outgoing(source->project_id, card_id)) {
    if (include_deleted.value_or(0) != 1 && !holder::card::link_target_visible(db, link)) {
      continue;
    }
    response.data.push_back({link.from_card_id,
                             link.to_card_id,
                             link.target_project_id,
                             std::nullopt,
                             link.to_type,
                             link.kind,
                             std::optional<std::optional<std::string>>{std::in_place, link.label},
                             link.created_at});
  }
  return response;
}

CardBacklinkListResponse list_card_backlinks(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string card_id,
    std::optional<int> include_deleted_value,
    std::string scope
) {
  holder::card::CardRepo card_repo(db);
  const auto card = card_repo.get(card_id);
  if (!card.has_value()) throw CardLinkSourceNotFound{};
  holder::card::LinkRepo repo(db);
  holder::ai::AiMessageRepo msg_repo(db, fts);
  const bool include_deleted = include_deleted_value.value_or(0) == 1;
  const bool include_local = scope != "external";
  const bool include_external = scope != "local";
  std::vector<holder::model::CardLink> links;
  if (include_local) {
    links = repo.list_backlinks_typed(card->project_id, card->card_id, "card");
  }
  if (include_external) {
    holder::project::ProjectRepo projects(db);
    const auto target_project = projects.get(card->project_id);
    if (!target_project.has_value() || !project_is_available(*target_project)) {
      throw CardLinkSourceNotFound{};
    }
    std::vector<std::string> available_sources;
    for (const auto& project : projects.list()) {
      if (project.project_id != card->project_id && project_is_available(project)) {
        available_sources.push_back(project.project_id);
      }
    }
    auto external =
        repo.list_external_backlinks_typed(card->project_id, card->card_id, "card", available_sources);
    links.insert(links.end(), external.begin(), external.end());
  }
  std::sort(links.begin(), links.end(), [](const auto& left, const auto& right) {
    if (left.created_at != right.created_at) return left.created_at > right.created_at;
    if (left.project_id != right.project_id) return left.project_id < right.project_id;
    if (left.from_card_id != right.from_card_id) return left.from_card_id < right.from_card_id;
    return left.kind < right.kind;
  });

  CardBacklinkListResponse response{
      true, {}, scope, include_external ? "available_projects" : "target_project"};
  holder::ai::AiThreadRepo thread_repo(db);
  for (const auto& link : links) {
    const bool external = link.project_id != card->project_id;
    const bool include_source =
        external ? should_include_external_backlink_source(
                       card_repo, msg_repo, thread_repo, link, include_deleted
                   )
                 : include_deleted || holder::card::backlink_source_visible(db, link);
    if (!include_source) continue;
    response.data.push_back({link.from_card_id,
                             link.to_card_id,
                             external ? link.target_project_id : std::nullopt,
                             external ? std::optional<std::string>(link.project_id) : std::nullopt,
                             link.to_type,
                             link.kind,
                             std::optional<std::optional<std::string>>{std::in_place, link.label},
                             link.created_at});
  }
  return response;
}

CardLinkCreateResponse create_card_link(holder::platform::Db& db,
                                        holder::index::FtsIndexer* fts,
                                        holder::card::CardStore* card_store,
                                        std::string card_id,
                                        CardLinkCreateRequest request) {
  if (card_store == nullptr) throw CardStoreUnavailable{};
  const auto card = card_store->get(card_id);
  if (!card) throw CardLinkSourceNotFound{};

  holder::model::CardLink link;
  link.project_id = card->project_id;
  link.from_card_id = card->card_id;
  link.to_card_id = std::move(request.to_card_id);
  link.to_type = request.to_type.value_or("");
  if (link.to_type.empty()) link.to_type = "card";
  link.kind = request.kind.value_or("");
  if (link.kind.empty()) link.kind = "ref";
  link.label = request.label.value_or(std::nullopt);
  if (request.target_project_id && *request.target_project_id != card->project_id)
    link.target_project_id = std::move(request.target_project_id);
  link.created_at = request.created_at.value_or(0);
  if (link.created_at <= 0) link.created_at = support::now_epoch_seconds();

  holder::card::validate_link_target(
      db, link.project_id, link.to_card_id, link.to_type, link.target_project_id);
  try {
    holder::card::add_card_link(db, fts, link);
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  }
  // Updating an existing link keeps its creation time.
  for (const auto& saved : holder::card::LinkRepo(db).list_outgoing(link.project_id, card_id)) {
    if (saved.to_card_id == link.to_card_id && saved.to_type == link.to_type &&
        saved.kind == link.kind && saved.target_project_id == link.target_project_id) {
      link.created_at = saved.created_at;
      break;
    }
  }
  return {true,
          {link.from_card_id, link.to_card_id, link.target_project_id, std::nullopt, link.to_type,
           link.kind, std::optional<std::optional<std::string>>(link.label), link.created_at}};
}

holder::api::CardPatchResponse delete_card_links(holder::platform::Db& db,
                                                 holder::index::FtsIndexer* fts,
                                                 holder::card::CardStore* card_store,
                                                 std::string card_id,
                                                 std::optional<holder::api::CardLinkDeleteRequest> request) {
  if (card_store == nullptr) throw CardStoreUnavailable{};
  const auto card = card_store->get(card_id);
  if (!card) throw CardLinkSourceNotFound{};
  const auto selector = request.value_or(holder::api::CardLinkDeleteRequest{});
  const auto& [to_card_id, target_project_id, to_type, kind] = selector;
  if (target_project_id && (!to_card_id || !to_type || !kind))
    throw std::invalid_argument("to_card_id, to_type and kind are required with target_project_id.");

  try {
    holder::card::LinkRepo links(db);
    if (to_card_id && to_type && kind) {
      // Rewrites the card file itself, rejecting a selector that matches several target projects.
      holder::card::remove_card_link(db, fts, card_id, *to_card_id, *to_type, *kind,
                                     target_project_id);
    } else {
      if (to_card_id)
        links.delete_link(card->project_id, card_id, *to_card_id, to_type, kind);
      else
        links.delete_links_from(card->project_id, card_id);
      card_store->update_links(card_id, support::now_epoch_seconds());
    }
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  }
  return {true, {std::move(card_id)}};
}

} // namespace holder::api::routes
