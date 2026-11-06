// Recovery snapshots from contract/interface/recovery/snapshot.py: a project's live cards, read a
// page at a time as records and as lines in core's encoding, and restored from those lines into a
// new project.
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CardCursor.h"
#include "api/CoreErrors.h"

#include "capi/Context.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "card/LinkRepo.h"
#include "card/MilestoneRepo.h"
#include "identity/Uuid.h"
#include "model/MilestoneDate.h"
#include "project/ProjectRepo.h"
#include "resource/LocationRepo.h"
#include "resource/LocationStore.h"
#include "resource/ResourceRepo.h"
#include "resource/ResourceStore.h"

#include <nlohmann/json.hpp>

#include <climits>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace holder::contract {
namespace {

using nlohmann::json;

// Each line is one card: {"format": FORMAT, "version": VERSION, "card": {...}}, with the card's
// fields named as the contract's RecoverySnapshotCard names them. A reader refuses a line from a
// later version rather than guess at it.
constexpr const char* FORMAT = "holder-recovery-snapshot";
constexpr int VERSION = 1;

// -- Choices of literals, as the contract spells them --

const char* name(RecoverySnapshotLinkTargetType value) {
  switch (value) {
  case RecoverySnapshotLinkTargetType::Card: return "card";
  case RecoverySnapshotLinkTargetType::AiMessage: return "ai_message";
  case RecoverySnapshotLinkTargetType::AiThread: return "ai_thread";
  case RecoverySnapshotLinkTargetType::Resource: return "resource";
  }
  throw Error("unknown link target type");
}

RecoverySnapshotLinkTargetType link_target_type(const std::string& value) {
  if (value == "card") return RecoverySnapshotLinkTargetType::Card;
  if (value == "ai_message") return RecoverySnapshotLinkTargetType::AiMessage;
  if (value == "ai_thread") return RecoverySnapshotLinkTargetType::AiThread;
  if (value == "resource") return RecoverySnapshotLinkTargetType::Resource;
  throw InvalidArgument("unknown link target type: " + value);
}

const char* name(RecoverySnapshotPlacementEncoding value) {
  switch (value) {
  case RecoverySnapshotPlacementEncoding::Plain: return "plain";
  case RecoverySnapshotPlacementEncoding::HolderAssetV1: return "holder_asset_v1";
  }
  throw Error("unknown placement encoding");
}

RecoverySnapshotPlacementEncoding placement_encoding(const std::string& value) {
  if (value == "plain") return RecoverySnapshotPlacementEncoding::Plain;
  if (value == "holder_asset_v1") return RecoverySnapshotPlacementEncoding::HolderAssetV1;
  throw InvalidArgument("unknown placement encoding: " + value);
}

const char* name(RecoverySnapshotLocationProvider value) {
  switch (value) {
  case RecoverySnapshotLocationProvider::LocalDirectory: return "local_directory";
  case RecoverySnapshotLocationProvider::S3Compatible: return "s3_compatible";
  case RecoverySnapshotLocationProvider::GoogleDrive: return "google-drive";
  }
  throw Error("unknown storage provider");
}

RecoverySnapshotLocationProvider location_provider(const std::string& value) {
  if (value == "local_directory") return RecoverySnapshotLocationProvider::LocalDirectory;
  if (value == "s3_compatible") return RecoverySnapshotLocationProvider::S3Compatible;
  if (value == "google-drive") return RecoverySnapshotLocationProvider::GoogleDrive;
  throw InvalidArgument("unknown storage provider: " + value);
}

const char* name(RecoverySnapshotCardPrivacyMode value) {
  switch (value) {
  case RecoverySnapshotCardPrivacyMode::EncryptedGit: return "encrypted_git";
  case RecoverySnapshotCardPrivacyMode::Plain: return "plain";
  }
  throw Error("unknown privacy mode");
}

RecoverySnapshotCardPrivacyMode card_privacy_mode(const std::string& value) {
  if (value == "encrypted_git") return RecoverySnapshotCardPrivacyMode::EncryptedGit;
  if (value == "plain") return RecoverySnapshotCardPrivacyMode::Plain;
  throw InvalidArgument("unknown privacy mode: " + value);
}

// -- Core's records as the snapshot's --

RecoverySnapshotLocation to_snapshot(const holder::model::Location& location) {
  RecoverySnapshotLocation out;
  out.location_id = location.location_id;
  out.name = location.name;
  // A provider the contract does not name is core's own data gone wrong, not the caller's.
  try {
    out.provider = location_provider(location.provider);
  } catch (const InvalidArgument& error) {
    throw Error(error.what());
  }
  out.configuration = location.configuration;
  out.created_at = location.created_at;
  out.updated_at = location.updated_at;
  return out;
}

RecoverySnapshotResource to_snapshot(const holder::model::ResourceBundle& bundle) {
  RecoverySnapshotResource out;
  out.resource_id = bundle.resource.resource_id;
  out.type = bundle.resource.type;
  out.label = bundle.resource.label;
  out.metadata = bundle.resource.metadata;
  out.created_at = bundle.resource.created_at;
  out.updated_at = bundle.resource.updated_at;
  for (const auto& asset : bundle.assets) {
    RecoverySnapshotAsset item;
    item.asset_id = asset.asset_id;
    item.original_filename = asset.original_filename;
    item.media_type = asset.media_type;
    item.byte_size = asset.byte_size;
    item.plaintext_sha256 = asset.plaintext_sha256;
    item.created_at = asset.created_at;
    item.updated_at = asset.updated_at;
    for (const auto& placement : asset.placements) {
      RecoverySnapshotPlacement stored;
      stored.placement_id = placement.placement_id;
      stored.location_id = placement.location_id;
      stored.object_key = placement.object_key;
      try {
        stored.encoding = placement_encoding(placement.encoding);
      } catch (const InvalidArgument& error) {
        throw Error(error.what());
      }
      stored.stored_byte_size = placement.stored_byte_size;
      stored.stored_sha256 = placement.stored_sha256;
      stored.created_at = placement.created_at;
      item.placements.push_back(std::move(stored));
    }
    out.assets.push_back(std::move(item));
  }
  return out;
}

RecoverySnapshotCard to_snapshot(holder_context& context, const holder::model::Project& project,
                                 const holder::model::Card& card, std::string content) {
  RecoverySnapshotCard out;
  out.card_id = card.card_id;
  out.project_id = project.project_id;
  out.project_name = project.name;
  try {
    out.privacy_mode = card_privacy_mode(project.privacy_mode);
  } catch (const InvalidArgument& error) {
    throw Error(error.what());
  }
  out.title = card.title;
  out.content = std::move(content);
  out.parent_card_id = card.parent_card_id;
  out.sort_key = card.sort_key;
  out.created_at = card.created_at;
  out.updated_at = card.updated_at;

  holder::resource::ResourceRepo resources(context.db);
  holder::resource::LocationRepo locations(context.db);
  std::set<std::string> included_resources;
  std::set<std::string> included_locations;
  for (const auto& link : holder::card::LinkRepo(context.db).list_outgoing(project.project_id, card.card_id)) {
    RecoverySnapshotLink item;
    try {
      item.target_type = link_target_type(link.to_type);
    } catch (const InvalidArgument& error) {
      throw Error(error.what());
    }
    item.target_id = link.to_card_id;
    item.target_project_id = link.target_project_id;
    item.kind = link.kind;
    item.label = link.label;
    out.links.push_back(std::move(item));
    if (link.to_type != "resource" || !included_resources.insert(link.to_card_id).second) continue;
    const auto bundle = resources.get_bundle(link.to_card_id);
    if (!bundle) continue;
    out.resources.push_back(to_snapshot(*bundle));
    for (const auto& asset : bundle->assets) {
      for (const auto& placement : asset.placements) {
        if (!included_locations.insert(placement.location_id).second) continue;
        if (const auto location = locations.get(placement.location_id))
          out.locations.push_back(to_snapshot(*location));
      }
    }
  }
  for (const auto& milestone :
       holder::card::MilestoneRepo(context.db).list_for_card(project.project_id, card.card_id)) {
    RecoverySnapshotMilestone item;
    for (const auto& date : milestone.dates) item.dates.push_back({date.role, date.date, date.datetime});
    item.kind = milestone.kind;
    item.description = milestone.description;
    out.milestones.push_back(std::move(item));
  }
  return out;
}

// -- The snapshot's records as JSON --

json optional(const std::optional<std::string>& value) {
  return value ? json(*value) : json(nullptr);
}

json to_json(const RecoverySnapshotCard& card) {
  json links = json::array();
  for (const auto& link : card.links) {
    json item = {{"target_type", name(link.target_type)},
                 {"target_id", link.target_id},
                 {"kind", link.kind},
                 {"label", optional(link.label)}};
    if (link.target_project_id) item["target_project_id"] = *link.target_project_id;
    links.push_back(std::move(item));
  }
  json milestones = json::array();
  for (const auto& milestone : card.milestones) {
    json dates = json::array();
    for (const auto& date : milestone.dates) {
      json item = {{"role", date.role}};
      if (date.date) item["date"] = *date.date;
      if (date.datetime) item["datetime"] = *date.datetime;
      dates.push_back(std::move(item));
    }
    milestones.push_back(
        {{"dates", std::move(dates)}, {"kind", optional(milestone.kind)}, {"description", optional(milestone.description)}});
  }
  json resources = json::array();
  for (const auto& resource : card.resources) {
    json assets = json::array();
    for (const auto& asset : resource.assets) {
      json placements = json::array();
      for (const auto& placement : asset.placements) {
        placements.push_back({{"placement_id", placement.placement_id},
                              {"location_id", placement.location_id},
                              {"object_key", placement.object_key},
                              {"encoding", name(placement.encoding)},
                              {"stored_byte_size", placement.stored_byte_size},
                              {"stored_sha256", placement.stored_sha256},
                              {"created_at", placement.created_at}});
      }
      assets.push_back({{"asset_id", asset.asset_id},
                        {"original_filename", asset.original_filename},
                        {"media_type", asset.media_type},
                        {"byte_size", asset.byte_size},
                        {"plaintext_sha256", asset.plaintext_sha256},
                        {"placements", std::move(placements)},
                        {"created_at", asset.created_at},
                        {"updated_at", asset.updated_at}});
    }
    resources.push_back({{"resource_id", resource.resource_id},
                         {"type", resource.type},
                         {"label", resource.label},
                         {"metadata", resource.metadata},
                         {"assets", std::move(assets)},
                         {"created_at", resource.created_at},
                         {"updated_at", resource.updated_at}});
  }
  json locations = json::array();
  for (const auto& location : card.locations) {
    locations.push_back({{"location_id", location.location_id},
                         {"name", location.name},
                         {"provider", name(location.provider)},
                         {"configuration", location.configuration},
                         {"created_at", location.created_at},
                         {"updated_at", location.updated_at}});
  }
  return {{"card_id", card.card_id},
          {"project_id", card.project_id},
          {"project_name", card.project_name},
          {"privacy_mode", name(card.privacy_mode)},
          {"title", card.title},
          {"content", card.content},
          {"parent_card_id", optional(card.parent_card_id)},
          {"sort_key", card.sort_key},
          {"created_at", card.created_at},
          {"updated_at", card.updated_at},
          {"links", std::move(links)},
          {"milestones", std::move(milestones)},
          {"resources", std::move(resources)},
          {"locations", std::move(locations)}};
}

std::string encode_line(const RecoverySnapshotCard& card) {
  return json{{"format", FORMAT}, {"version", VERSION}, {"card", to_json(card)}}.dump();
}

// -- JSON as the snapshot's records. A line that does not hold a card is an invalid argument. --

const json& field(const json& object, const char* key) {
  if (!object.is_object()) throw InvalidArgument(std::string("expected an object holding ") + key);
  const auto found = object.find(key);
  if (found == object.end()) throw InvalidArgument(std::string("missing field: ") + key);
  return *found;
}

std::string text(const json& object, const char* key, bool non_empty = false) {
  const auto& value = field(object, key);
  if (!value.is_string()) throw InvalidArgument(std::string("expected a string: ") + key);
  auto out = value.get<std::string>();
  if (non_empty && out.empty()) throw InvalidArgument(std::string("must not be empty: ") + key);
  return out;
}

std::optional<std::string> nullable_text(const json& object, const char* key) {
  if (field(object, key).is_null()) return std::nullopt;
  return text(object, key);
}

long long integer(const json& object, const char* key) {
  const auto& value = field(object, key);
  if (!value.is_number_integer() || (value.is_number_unsigned() && value.get<unsigned long long>() > LLONG_MAX))
    throw InvalidArgument(std::string("expected an integer: ") + key);
  return value.get<long long>();
}

double number(const json& object, const char* key) {
  const auto& value = field(object, key);
  if (!value.is_number()) throw InvalidArgument(std::string("expected a number: ") + key);
  return value.get<double>();
}

const json& array(const json& object, const char* key) {
  const auto& value = field(object, key);
  if (!value.is_array()) throw InvalidArgument(std::string("expected an array: ") + key);
  return value;
}

std::map<std::string, std::string> string_map(const json& object, const char* key) {
  const auto& value = field(object, key);
  if (!value.is_object()) throw InvalidArgument(std::string("expected an object: ") + key);
  std::map<std::string, std::string> out;
  for (const auto& [entry, item] : value.items()) {
    if (!item.is_string()) throw InvalidArgument(std::string("expected strings in ") + key);
    out.emplace(entry, item.get<std::string>());
  }
  return out;
}

std::map<std::string, std::vector<std::string>> string_list_map(const json& object, const char* key) {
  const auto& value = field(object, key);
  if (!value.is_object()) throw InvalidArgument(std::string("expected an object: ") + key);
  std::map<std::string, std::vector<std::string>> out;
  for (const auto& [entry, items] : value.items()) {
    if (!items.is_array()) throw InvalidArgument(std::string("expected lists of strings in ") + key);
    auto& list = out[entry];
    for (const auto& item : items) {
      if (!item.is_string()) throw InvalidArgument(std::string("expected lists of strings in ") + key);
      list.push_back(item.get<std::string>());
    }
  }
  return out;
}

RecoverySnapshotCard card_from_json(const json& body) {
  RecoverySnapshotCard card;
  card.card_id = text(body, "card_id", true);
  card.project_id = text(body, "project_id", true);
  card.project_name = text(body, "project_name", true);
  card.privacy_mode = card_privacy_mode(text(body, "privacy_mode"));
  card.title = text(body, "title");
  card.content = text(body, "content");
  card.parent_card_id = nullable_text(body, "parent_card_id");
  if (card.parent_card_id && card.parent_card_id->empty()) throw InvalidArgument("parent_card_id must not be empty");
  card.sort_key = number(body, "sort_key");
  card.created_at = integer(body, "created_at");
  card.updated_at = integer(body, "updated_at");
  for (const auto& item : array(body, "links")) {
    RecoverySnapshotLink link;
    link.target_type = link_target_type(text(item, "target_type"));
    link.target_id = text(item, "target_id", true);
    if (item.contains("target_project_id")) link.target_project_id = text(item, "target_project_id", true);
    link.kind = text(item, "kind", true);
    link.label = nullable_text(item, "label");
    card.links.push_back(std::move(link));
  }
  for (const auto& item : array(body, "milestones")) {
    RecoverySnapshotMilestone milestone;
    for (const auto& value : array(item, "dates")) {
      MilestoneDate date{text(value, "role", true), std::nullopt, std::nullopt};
      if (value.contains("date")) date.date = text(value, "date");
      if (value.contains("datetime")) date.datetime = text(value, "datetime");
      try {
        holder::model::validate_milestone_date({date.role, date.date, date.datetime});
      } catch (const std::invalid_argument& error) {
        throw InvalidArgument(error.what());
      }
      milestone.dates.push_back(std::move(date));
    }
    milestone.kind = nullable_text(item, "kind");
    milestone.description = nullable_text(item, "description");
    card.milestones.push_back(std::move(milestone));
  }
  for (const auto& item : array(body, "resources")) {
    RecoverySnapshotResource resource;
    resource.resource_id = text(item, "resource_id", true);
    resource.type = text(item, "type", true);
    resource.label = text(item, "label");
    resource.metadata = string_list_map(item, "metadata");
    resource.created_at = integer(item, "created_at");
    resource.updated_at = integer(item, "updated_at");
    for (const auto& value : array(item, "assets")) {
      RecoverySnapshotAsset asset;
      asset.asset_id = text(value, "asset_id", true);
      asset.original_filename = text(value, "original_filename");
      asset.media_type = text(value, "media_type");
      asset.byte_size = integer(value, "byte_size");
      asset.plaintext_sha256 = text(value, "plaintext_sha256");
      asset.created_at = integer(value, "created_at");
      asset.updated_at = integer(value, "updated_at");
      for (const auto& stored : array(value, "placements")) {
        RecoverySnapshotPlacement placement;
        placement.placement_id = text(stored, "placement_id", true);
        placement.location_id = text(stored, "location_id", true);
        placement.object_key = text(stored, "object_key", true);
        placement.encoding = placement_encoding(text(stored, "encoding"));
        placement.stored_byte_size = integer(stored, "stored_byte_size");
        placement.stored_sha256 = text(stored, "stored_sha256", true);
        placement.created_at = integer(stored, "created_at");
        asset.placements.push_back(std::move(placement));
      }
      resource.assets.push_back(std::move(asset));
    }
    card.resources.push_back(std::move(resource));
  }
  for (const auto& item : array(body, "locations")) {
    RecoverySnapshotLocation location;
    location.location_id = text(item, "location_id", true);
    location.name = text(item, "name", true);
    location.provider = location_provider(text(item, "provider"));
    location.configuration = string_map(item, "configuration");
    location.created_at = integer(item, "created_at");
    location.updated_at = integer(item, "updated_at");
    card.locations.push_back(std::move(location));
  }
  return card;
}

RecoverySnapshotCard decode_line(const std::string& line) {
  json body;
  try {
    body = json::parse(line);
  } catch (const json::parse_error&) {
    throw InvalidArgument("a recovery snapshot line is not JSON");
  }
  if (!body.is_object() || body.value("format", json()) != FORMAT)
    throw InvalidArgument("a line is not from a recovery snapshot");
  if (integer(body, "version") != VERSION)
    throw InvalidArgument("a recovery snapshot line has a version this device cannot read");
  return card_from_json(field(body, "card"));
}

// -- The snapshot's records as core's, for restore --

holder::model::Location to_model(const RecoverySnapshotLocation& location, const std::string& project_id) {
  return {location.location_id, project_id, location.name, name(location.provider), location.configuration,
          location.created_at, location.updated_at};
}

holder::model::ResourceBundle to_model(const RecoverySnapshotResource& resource, const std::string& project_id) {
  holder::model::ResourceBundle bundle;
  bundle.resource = {resource.resource_id, project_id,          resource.type,      resource.label,
                     resource.metadata,    resource.created_at, resource.updated_at};
  for (const auto& asset : resource.assets) {
    holder::model::Asset item{asset.asset_id,   resource.resource_id,     asset.original_filename,
                              asset.media_type, asset.byte_size,          asset.plaintext_sha256,
                              asset.created_at, asset.updated_at,         {}};
    for (const auto& placement : asset.placements) {
      item.placements.push_back({placement.placement_id, asset.asset_id, placement.location_id,
                                 placement.object_key, name(placement.encoding), placement.stored_byte_size,
                                 placement.stored_sha256, placement.created_at});
    }
    bundle.assets.push_back(std::move(item));
  }
  return bundle;
}

holder::card::BatchCardInput to_model(const RecoverySnapshotCard& card) {
  holder::card::BatchCardInput item;
  item.card_id = card.card_id;
  item.title = card.title;
  item.content = card.content;
  item.created_at = card.created_at;
  item.updated_at = card.updated_at;
  item.parent_card_id = card.parent_card_id;
  item.sort_key = card.sort_key;
  for (const auto& link : card.links) {
    holder::model::CardLink out;
    out.to_card_id = link.target_id;
    out.to_type = name(link.target_type);
    out.kind = link.kind;
    out.label = link.label;
    out.target_project_id = link.target_project_id;
    item.links.push_back(std::move(out));
  }
  for (const auto& milestone : card.milestones) {
    holder::model::Milestone out;
    for (const auto& date : milestone.dates) out.dates.push_back({date.role, date.date, date.datetime});
    out.kind = milestone.kind;
    out.description = milestone.description;
    item.milestones.push_back(std::move(out));
  }
  return item;
}

std::vector<RecoverySnapshotCard> decode_snapshot(const std::string& snapshot) {
  std::vector<RecoverySnapshotCard> cards;
  std::set<std::string> card_ids;
  std::size_t start = 0;
  while (start <= snapshot.size()) {
    auto end = snapshot.find('\n', start);
    if (end == std::string::npos) end = snapshot.size();
    auto line = snapshot.substr(start, end - start);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty()) {
      cards.push_back(decode_line(line));
      if (!card_ids.insert(cards.back().card_id).second)
        throw InvalidArgument("a recovery snapshot holds a card twice: " + cards.back().card_id);
    }
    start = end + 1;
  }
  return cards;
}

void replace_all(std::string& text, const std::string& from, const std::string& to) {
  for (std::size_t at = 0; (at = text.find(from, at)) != std::string::npos; at += to.size())
    text.replace(at, from.size(), to);
}

RecoverySnapshotPage read(holder_context& context, const std::string& project_id,
                          const std::optional<std::string>& cursor, std::int32_t limit) {
  const auto project = holder::project::ProjectRepo(context.db).get(project_id);
  if (!project) throw NotFound("project not found: " + project_id);
  const json scope = {{"project_id", project_id}, {"order", "recovery_snapshot"}};
  holder::card::CardPageQuery query;
  query.order = holder::card::CardPageOrder::UpdatedDesc;
  if (cursor) query.cursor = decode_cursor(*cursor, scope, false);
  auto page = card_store(context).list_collection_page(project_id, query, limit, true);
  RecoverySnapshotPage out;
  for (auto& [card, content] : page.cards) {
    out.cards.push_back(to_snapshot(context, *project, card, std::move(content)));
    out.lines.push_back(encode_line(out.cards.back()));
  }
  if (page.next_cursor) {
    out.next_cursor = encode_cursor({{"version", 1},
                                     {"query", scope},
                                     {"card_id", page.next_cursor->card_id},
                                     {"updated_at", *page.next_cursor->updated_at}});
  }
  return out;
}

Project restore(holder_context& context, const RecoverySnapshotRestore& request) {
  // Everything is read and checked before the project is made.
  const auto cards = decode_snapshot(request.snapshot);
  std::map<std::string, RecoverySnapshotLocation> locations;
  std::map<std::string, RecoverySnapshotResource> resources;
  for (const auto& card : cards) {
    for (const auto& location : card.locations) locations.emplace(location.location_id, location);
    for (const auto& resource : card.resources) resources.emplace(resource.resource_id, resource);
  }
  for (const auto& [id, resource] : resources)
    for (const auto& asset : resource.assets)
      for (const auto& placement : asset.placements)
        if (!locations.contains(placement.location_id))
          throw InvalidArgument("a resource's placement names a storage location the snapshot does not hold: " +
                                placement.location_id);

  NewProject project;
  project.name = request.project_name;
  project.privacy_mode = request.privacy_mode == RecoverySnapshotRestorePrivacyMode::EncryptedGit
                             ? NewProjectPrivacyMode::EncryptedGit
                             : NewProjectPrivacyMode::Plain;
  const auto created = create_project(context, project);
  try {
    std::map<std::string, std::string> location_ids;
    for (const auto& [old_id, location] : locations) {
      auto model = to_model(location, created.project_id);
      model.location_id = location_ids[old_id] = holder::identity::uuid_v4();
      holder::resource::LocationStore(context.db, nullptr, context.git).put(model);
    }
    std::map<std::string, std::string> resource_ids;
    for (const auto& [old_id, resource] : resources) {
      auto bundle = to_model(resource, created.project_id);
      bundle.resource.resource_id = resource_ids[old_id] = holder::identity::uuid_v4();
      for (auto& asset : bundle.assets) {
        asset.asset_id = holder::identity::uuid_v4();
        asset.resource_id = bundle.resource.resource_id;
        for (auto& placement : asset.placements) {
          placement.placement_id = holder::identity::uuid_v4();
          placement.asset_id = asset.asset_id;
          placement.location_id = location_ids.at(placement.location_id);
        }
      }
      holder::resource::ResourceStore(context.db, nullptr, context.git).put(bundle);
    }
    std::vector<holder::card::BatchCardInput> items;
    for (const auto& card : cards) {
      auto item = to_model(card);
      for (const auto& [old_id, new_id] : resource_ids)
        replace_all(item.content, "holder://resource/" + old_id, "holder://resource/" + new_id);
      items.push_back(std::move(item));
    }
    card_store(context).create_batch(created.project_id, items, holder::identity::uuid_v4,
                                     request.commit_message, resource_ids);
  } catch (...) {
    // The project was made only for this restore: leave nothing of it behind.
    try {
      delete_project(context, created.project_id);
    } catch (...) {
    }
    std::error_code ignored;
    std::filesystem::remove_all(created.root_path, ignored);
    throw;
  }
  return get_project(context, created.project_id);
}

} // namespace

RecoverySnapshotPage read_recovery_snapshot(holder_context& context, std::string project_id,
                                            std::optional<std::string> cursor, std::int32_t limit) {
  return with_contract_errors([&] { return read(context, project_id, cursor, limit); });
}

Project restore_recovery_snapshot(holder_context& context, const RecoverySnapshotRestore& request) {
  return with_contract_errors([&] { return restore(context, request); });
}

} // namespace holder::contract
