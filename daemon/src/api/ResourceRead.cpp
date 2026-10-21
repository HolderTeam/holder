#include "api/ResourceRead.h"

#include "card/CardRepo.h"
#include "card/LinkRepo.h"
#include "resource/ResourceRepo.h"

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace holder::api {
namespace {

using References = std::unordered_map<std::string, std::vector<ResourceReferencedByCardsItem>>;

// For each resource, the live cards linking to it with their sorted link kinds, ordered by title
// then card ID.
References resource_references(holder::platform::Db& db, const std::string& project_id) {
  std::unordered_map<std::string, holder::model::Card> cards_by_id;
  for (const auto& card : holder::card::CardRepo(db).list_all(project_id)) {
    if (!card.deleted_at.has_value()) cards_by_id.emplace(card.card_id, card);
  }

  std::unordered_map<std::string, std::map<std::string, ResourceReferencedByCardsItem>> by_resource;
  for (const auto& link : holder::card::LinkRepo(db).list_incoming_typed(project_id, "resource")) {
    const auto card = cards_by_id.find(link.from_card_id);
    if (card == cards_by_id.end()) continue;
    auto& reference = by_resource[link.to_card_id][link.from_card_id];
    reference.card_id = card->second.card_id;
    reference.title = card->second.title;
    reference.updated_at = card->second.updated_at;
    if (std::find(reference.link_kinds.begin(), reference.link_kinds.end(), link.kind) ==
        reference.link_kinds.end()) {
      reference.link_kinds.push_back(link.kind);
    }
  }

  References result;
  for (auto& [resource_id, by_card] : by_resource) {
    auto& references = result[resource_id];
    for (auto& [card_id, reference] : by_card) {
      std::sort(reference.link_kinds.begin(), reference.link_kinds.end());
      references.push_back(std::move(reference));
    }
    std::sort(references.begin(), references.end(), [](const auto& left, const auto& right) {
      if (left.title != right.title) return left.title < right.title;
      return left.card_id < right.card_id;
    });
  }
  return result;
}

} // namespace

Resource resource_item(const holder::model::ResourceBundle& bundle,
                       std::vector<ResourceReferencedByCardsItem> referenced_by_cards) {
  Resource item{
      bundle.resource.resource_id,
      bundle.resource.project_id,
      bundle.resource.type,
      bundle.resource.label,
      {bundle.resource.metadata},
      {},
      bundle.resource.created_at,
      bundle.resource.updated_at,
      std::move(referenced_by_cards),
  };
  for (const auto& asset : bundle.assets) {
    ResourceAsset value{
        asset.asset_id,
        asset.resource_id,
        asset.original_filename,
        asset.media_type,
        asset.byte_size,
        asset.plaintext_sha256,
        {},
        asset.created_at,
        asset.updated_at,
    };
    for (const auto& placement : asset.placements) {
      value.placements.push_back({placement.placement_id,
                                  placement.location_id,
                                  placement.encoding,
                                  placement.stored_byte_size,
                                  placement.created_at});
    }
    item.assets.push_back(std::move(value));
  }
  return item;
}

ResourceListResponse get_resources(holder::platform::Db& db,
                                   std::string project_id,
                                   std::optional<std::string> card_id,
                                   std::optional<int> limit,
                                   std::optional<int> offset) {
  if (card_id && card_id->empty()) card_id.reset();
  ResourceListResponse response{};
  response.ok = true;
  holder::resource::ResourceRepo repo(db);
  std::vector<holder::model::Resource> resources;
  if (card_id) {
    const auto card = holder::card::CardRepo(db).get(*card_id);
    if (!card || card->deleted_at) throw ResourceCardNotFound{};
    if (card->project_id != project_id) throw ResourceCardInOtherProject{};
    const int page_limit = limit.value_or(100);
    const int page_offset = offset.value_or(0);
    resources = repo.list_for_card(project_id, *card_id, page_limit, page_offset);
    response.card_id = card_id;
    response.limit = page_limit;
    response.offset = page_offset;
    response.next_offset.emplace();
    if (resources.size() == static_cast<std::size_t>(page_limit)) {
      *response.next_offset = static_cast<long long>(page_offset) + page_limit;
    }
  } else {
    if (limit || offset) throw ResourceListBadRequest("limit and offset require card_id.");
    resources = repo.list(project_id);
  }
  auto references = resource_references(db, project_id);
  for (const auto& resource : resources) {
    const auto found = references.find(resource.resource_id);
    response.data.push_back(resource_item(
        *repo.get_bundle(resource.resource_id),
        found != references.end() ? std::move(found->second)
                                  : std::vector<ResourceReferencedByCardsItem>{}));
  }
  return response;
}

Resource resource_with_references(holder::platform::Db& db,
                                  const holder::model::ResourceBundle& bundle) {
  auto references = resource_references(db, bundle.resource.project_id);
  const auto found = references.find(bundle.resource.resource_id);
  return resource_item(bundle,
                       found != references.end() ? std::move(found->second)
                                                 : std::vector<ResourceReferencedByCardsItem>{});
}

ResourceResponse get_resource(holder::platform::Db& db, std::string resource_id) {
  const auto bundle = holder::resource::ResourceRepo(db).get_bundle(resource_id);
  if (!bundle) throw ResourceNotFound{};
  return {true, resource_with_references(db, *bundle)};
}

} // namespace holder::api
