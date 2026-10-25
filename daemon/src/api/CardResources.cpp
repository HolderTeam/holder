#include "api/CardResources.h"

#include "api/support/Time.h"
#include "card/CardStore.h"
#include "card/LinkMutation.h"
#include "card/LinkRepo.h"
#include "resource/ResourceRepo.h"

#include <algorithm>
#include <utility>

namespace holder::api {
namespace {

// Attaches or detaches the resource, changing nothing when it is already that way.
ResourceAttachmentResponse set_attachment(holder::platform::Db& db,
                                          holder::index::FtsIndexer* fts,
                                          holder::card::CardStore* card_store,
                                          std::string card_id,
                                          const std::string& project_id,
                                          std::string resource_id,
                                          bool attaching) {
  if (card_store == nullptr) throw CardStoreUnavailable{};
  const auto card = card_store->get(card_id);
  if (!card || card->deleted_at) throw CardResourceNotFound("Live card not found.");
  const auto resource = holder::resource::ResourceRepo(db).get(resource_id);
  if (!resource && attaching) throw CardResourceNotFound("Resource not found.");
  if (card->project_id != project_id || (resource && resource->project_id != project_id))
    throw CardResourceProjectMismatch{};

  const auto outgoing = holder::card::LinkRepo(db).list_outgoing(project_id, card_id);
  const bool present = std::any_of(outgoing.begin(), outgoing.end(), [&](const auto& link) {
    return link.to_card_id == resource_id && link.to_type == "resource" &&
           link.kind == "attachment";
  });
  const bool changed = attaching != present;
  try {
    if (changed && attaching) {
      holder::model::CardLink link;
      link.project_id = project_id;
      link.from_card_id = card_id;
      link.to_card_id = resource_id;
      link.to_type = "resource";
      link.kind = "attachment";
      link.created_at = support::now_epoch_seconds();
      holder::card::add_card_link(db, fts, link);
    } else if (changed) {
      holder::card::remove_card_link(db, fts, card_id, resource_id, "resource", "attachment");
    }
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  }
  const auto outcome = attaching ? (changed ? "attached" : "already_attached")
                                 : (changed ? "detached" : "not_attached");
  return {true, {std::move(card_id), std::move(resource_id), changed, outcome}};
}

} // namespace

ResourceAttachmentResponse post_cards_card_id_resources(holder::platform::Db& db,
                                                        holder::index::FtsIndexer* fts,
                                                        holder::card::CardStore* card_store,
                                                        std::string card_id,
                                                        PostCardsCardIdResourcesRequest request) {
  return set_attachment(db, fts, card_store, std::move(card_id), request.project_id,
                        std::move(request.resource_id), true);
}

ResourceAttachmentResponse delete_cards_card_id_resources(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    holder::card::CardStore* card_store,
    std::string card_id,
    DeleteCardsCardIdResourcesRequest request) {
  return set_attachment(db, fts, card_store, std::move(card_id), request.project_id,
                        std::move(request.resource_id), false);
}

} // namespace holder::api
