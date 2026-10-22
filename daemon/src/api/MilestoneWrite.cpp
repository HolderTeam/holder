#include "api/MilestoneWrite.h"

#include "api/CardMilestones.h"
#include "api/MilestonesWithDates.h"
#include "api/support/Time.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "card/MilestoneRepo.h"
#include "model/Milestone.h"
#include "model/MilestoneDate.h"

#include <algorithm>
#include <utility>

namespace holder::api {
namespace {

holder::card::CardStore& require_card_store(holder::card::CardStore* card_store) {
  if (card_store == nullptr) throw CardStoreUnavailable{};
  return *card_store;
}

holder::model::Card live_card(holder::platform::Db& db, const std::string& card_id) {
  auto card = holder::card::CardRepo(db).get(card_id);
  if (!card || card->deleted_at) throw MilestoneWriteNotFound("Card not found.");
  return std::move(*card);
}

// An empty kind or description is stored as none.
std::optional<std::string> text(std::optional<std::optional<std::string>> value) {
  auto result = value.value_or(std::nullopt);
  if (result && result->empty()) result.reset();
  return result;
}

std::vector<holder::model::MilestoneDate> milestone_dates(std::vector<MilestoneDateValue> values) {
  std::vector<holder::model::MilestoneDate> dates;
  dates.reserve(values.size());
  for (auto& value : values) {
    holder::model::MilestoneDate date{std::move(value.role), std::move(value.date),
                                      std::move(value.datetime)};
    holder::model::validate_milestone_date(date);
    dates.push_back(std::move(date));
  }
  return dates;
}

// Runs a card store write, giving a privacy failure its status.
template <typename Write> auto write_milestone(Write&& write) {
  try {
    return write();
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  }
}

holder::model::Milestone add(holder::platform::Db& db,
                             holder::card::CardStore* card_store,
                             const std::function<std::string()>& uuid_v4,
                             const std::string& card_id,
                             holder::model::Milestone milestone) {
  auto& store = require_card_store(card_store);
  const auto card = live_card(db, card_id);
  milestone.milestone_id = uuid_v4();
  milestone.project_id = card.project_id;
  milestone.card_id = card_id;
  milestone.created_at = support::now_epoch_seconds();
  milestone.updated_at = milestone.created_at;
  return write_milestone([&] { return store.add_milestone(milestone, milestone.created_at); });
}

holder::model::Milestone update(holder::platform::Db& db,
                                holder::card::CardStore* card_store,
                                const std::string& card_id,
                                const std::string& milestone_id,
                                const holder::card::MilestoneUpdate& change,
                                bool timestamp_view) {
  auto& store = require_card_store(card_store);
  const auto card = live_card(db, card_id);
  if (timestamp_view) {
    const auto milestones = holder::card::MilestoneRepo(db).list_for_card(card.project_id, card_id);
    const auto existing = std::find_if(milestones.begin(), milestones.end(), [&](const auto& m) {
      return m.milestone_id == milestone_id;
    });
    if (existing != milestones.end() && !has_timestamp_view(*existing))
      throw std::invalid_argument("Milestone requires the labelled dates route.");
  }
  auto updated = write_milestone([&] {
    return store.update_milestone(card.project_id, card_id, milestone_id, change,
                                  support::now_epoch_seconds());
  });
  if (!updated) throw MilestoneWriteNotFound("Milestone not found.");
  return std::move(*updated);
}

MilestoneRemoveResponse remove(holder::platform::Db& db,
                               holder::card::CardStore* card_store,
                               std::string card_id,
                               std::string milestone_id) {
  auto& store = require_card_store(card_store);
  const auto card = live_card(db, card_id);
  const bool removed = write_milestone([&] {
    return store.remove_milestone(card.project_id, card_id, milestone_id,
                                  support::now_epoch_seconds());
  });
  return {true, {std::move(card_id), std::move(milestone_id), removed}};
}

} // namespace

MilestoneWithDatesResponse post_cards_card_id_milestones_with_dates(
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    const std::function<std::string()>& uuid_v4,
    std::string card_id,
    MilestoneWithDatesCreateRequest request) {
  holder::model::Milestone milestone;
  milestone.dates = milestone_dates(std::move(request.dates));
  milestone.kind = text(std::move(request.kind));
  milestone.description = text(std::move(request.description));
  return {true, milestone_with_dates(add(db, card_store, uuid_v4, card_id, std::move(milestone)))};
}

MilestoneWithDatesResponse patch_cards_card_id_milestones_with_dates_milestone_id(
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    std::string card_id,
    std::string milestone_id,
    MilestoneWithDatesUpdateRequest request) {
  holder::card::MilestoneUpdate change;
  if (request.dates) change.dates = milestone_dates(std::move(*request.dates));
  change.has_kind = request.kind.has_value();
  change.kind = text(std::move(request.kind));
  change.has_description = request.description.has_value();
  change.description = text(std::move(request.description));
  return {true,
          milestone_with_dates(update(db, card_store, card_id, milestone_id, change, false))};
}

MilestoneRemoveResponse delete_cards_card_id_milestones_with_dates_milestone_id(
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    std::string card_id,
    std::string milestone_id) {
  return remove(db, card_store, std::move(card_id), std::move(milestone_id));
}

MilestoneResponse post_cards_card_id_milestones(holder::platform::Db& db,
                                                holder::card::CardStore* card_store,
                                                const std::function<std::string()>& uuid_v4,
                                                std::string card_id,
                                                MilestoneCreateRequest request) {
  const auto end_at = request.end_at.value_or(std::nullopt);
  if (end_at && *end_at < request.start_at)
    throw std::invalid_argument("end_at must not be before start_at.");
  holder::model::Milestone milestone;
  milestone.dates.push_back(
      holder::model::milestone_date_from_legacy("start", request.start_at, request.all_day));
  if (end_at)
    milestone.dates.push_back(
        holder::model::milestone_date_from_legacy("end", *end_at, request.all_day));
  milestone.kind = text(std::move(request.kind));
  milestone.description = text(std::move(request.description));
  return {true, timestamp_milestone(add(db, card_store, uuid_v4, card_id, std::move(milestone)))};
}

MilestoneResponse patch_cards_card_id_milestones_milestone_id(holder::platform::Db& db,
                                                              holder::card::CardStore* card_store,
                                                              std::string card_id,
                                                              std::string milestone_id,
                                                              MilestoneUpdateRequest request) {
  holder::card::MilestoneUpdate change;
  change.start_at = request.start_at;
  change.has_end_at = request.end_at.has_value();
  change.end_at = request.end_at.value_or(std::nullopt);
  change.all_day = request.all_day;
  change.has_kind = request.kind.has_value();
  change.kind = text(std::move(request.kind));
  change.has_description = request.description.has_value();
  change.description = text(std::move(request.description));
  return {true, timestamp_milestone(update(db, card_store, card_id, milestone_id, change, true))};
}

MilestoneRemoveResponse delete_cards_card_id_milestones_milestone_id(
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    std::string card_id,
    std::string milestone_id) {
  return remove(db, card_store, std::move(card_id), std::move(milestone_id));
}

} // namespace holder::api
