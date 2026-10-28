// A stand-in for core: the Card operations over an in-memory map, enough to compile and run
// the generated C API end to end. Core's real implementation replaces this.
#include "Contract.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

struct holder_context {
  std::map<std::string, holder::contract::Card> cards;
  std::vector<holder::contract::Link> links;
  std::vector<holder::contract::Milestone> milestones;
  std::vector<holder::contract::Resource> resources;
  std::vector<holder::contract::Project> projects;
  int next_id = 1;
};

struct holder_error {
  std::string message;
};

namespace holder::contract {

int detail::fail(holder_error** out_error, int code, const std::string& message) {
  if (out_error != nullptr) *out_error = new holder_error{message};
  return code;
}

// The card, live or in the trash, when it is in the project.
Card& card_in(holder_context& context, const std::string& project_id, const std::string& card_id) {
  auto it = context.cards.find(card_id);
  if (it == context.cards.end() || it->second.project_id != project_id) throw NotFound("card not found: " + card_id);
  return it->second;
}

Card& live_card(holder_context& context, const std::string& project_id, const std::string& card_id) {
  auto& card = card_in(context, project_id, card_id);
  if (card.deleted_at) throw NotFound("card not found: " + card_id);
  return card;
}

Card get_card(holder_context& context, std::string project_id, std::string card_id) {
  return live_card(context, project_id, card_id);
}

Page<CardListItem> list_cards(holder_context& context, std::string project_id, std::optional<ListCardsParent> parent,
                              std::optional<std::string> tag, ListCardsOrder order, std::int32_t limit,
                              std::optional<std::string> cursor, bool include_content, bool include_child_count) {
  if (project_id != "p1") throw NotFound("project not found: " + project_id);
  std::vector<CardListItem> matching;
  for (const auto& [id, card] : context.cards) {
    if (card.deleted_at) continue;
    if (parent && std::holds_alternative<ListCardsParentRoots>(*parent) && card.parent_card_id) continue;
    if (parent && std::holds_alternative<std::string>(*parent) && card.parent_card_id != std::get<std::string>(*parent)) continue;
    if (tag && std::find(card.tags.begin(), card.tags.end(), *tag) == card.tags.end()) continue;
    CardListItem item{card, std::nullopt, std::nullopt};
    if (include_content) item.content = card.content;
    if (include_child_count) {
      item.child_count = static_cast<std::int32_t>(std::count_if(context.cards.begin(), context.cards.end(),
          [&](const auto& other) { return other.second.parent_card_id == card.card_id; }));
    }
    matching.push_back(item);
  }
  if (order == ListCardsOrder::UpdatedDesc) std::reverse(matching.begin(), matching.end());
  std::size_t start = cursor ? std::stoul(*cursor) : 0;
  Page<CardListItem> page;
  for (std::size_t i = start; i < matching.size() && page.items.size() < static_cast<std::size_t>(limit); ++i)
    page.items.push_back(matching[i]);
  if (start + page.items.size() < matching.size()) page.next_cursor = std::to_string(start + page.items.size());
  return page;
}

Card create_card(holder_context& context, std::string project_id, const NewCard& request) {
  if (project_id != "p1") throw NotFound("project not found: " + project_id);
  if (request.parent_card_id && !context.cards.contains(*request.parent_card_id))
    throw NotFound("parent not found: " + *request.parent_card_id);
  Card card;
  card.card_id = "c" + std::to_string(context.next_id++);
  card.project_id = project_id;
  card.title = request.title;
  card.rel_path = "cards/" + card.card_id + ".md";
  card.parent_card_id = request.parent_card_id;
  card.sort_key = request.sort_key.value_or(0.0);
  card.created_at = request.created_at.value_or(1793000000);
  card.updated_at = request.updated_at.value_or(card.created_at);
  card.content = request.content;
  if (card.content.find("#draft") != std::string::npos) {
    card.tags = {"draft"};
    auto at = static_cast<long long>(card.content.find("#draft"));
    card.tag_occurrences = {{"draft", at, at + 6}};
  }
  context.cards[card.card_id] = card;
  return card;
}

Card update_card(holder_context& context, std::string project_id, std::string card_id, const CardUpdate& update) {
  auto& card = live_card(context, project_id, card_id);
  if (update.title) card.title = *update.title;
  if (update.content) card.content = *update.content;
  if (update.parent_card_id) card.parent_card_id = *update.parent_card_id;
  if (update.sort_key) card.sort_key = *update.sort_key;
  if (update.updated_at) card.updated_at = *update.updated_at;
  return card;
}

Card trash_card(holder_context& context, std::string project_id, std::string card_id, TrashCardChildren children) {
  auto& card = live_card(context, project_id, card_id);
  card.deleted_at = children == TrashCardChildren::TrashSubtree ? 1793000200 : 1793000100;
  return card;
}

Card restore_card(holder_context& context, std::string project_id, std::string card_id) {
  auto& card = card_in(context, project_id, card_id);
  if (!card.deleted_at) throw Conflict("the card is not in the trash: " + card_id);
  card.deleted_at.reset();
  return card;
}

Card move_card(holder_context& context, std::string project_id, std::string card_id, const CardMove& move) {
  auto& card = live_card(context, project_id, card_id);
  if (move.intent == CardMoveIntent::Into) {
    if (!move.target_card_id) throw InvalidArgument("target_card_id is required for into");
    if (*move.target_card_id == card_id) throw InvalidMove("a card cannot go under itself");
    card.parent_card_id = *move.target_card_id;
  } else if (move.intent == CardMoveIntent::UpLevel) {
    if (!card.parent_card_id) throw InvalidMove("the card is already at the top level");
    card.parent_card_id = std::nullopt;
  } else if (move.parent_card_id) {
    card.parent_card_id = *move.parent_card_id;
  }
  return card;
}


Card add_card_tag(holder_context& context, std::string project_id, std::string card_id, std::string tag) {
  if (tag.front() == '#') throw InvalidArgument("not a tag: " + tag);
  auto& card = live_card(context, project_id, card_id);
  if (std::find(card.tags.begin(), card.tags.end(), tag) == card.tags.end()) {
    card.content += "\n#" + tag;
    card.tags.push_back(tag);
  }
  return card;
}

// The stand-in treats every tag as one in the card's prose.
Card remove_card_tag(holder_context& context, std::string project_id, std::string card_id, std::string tag) {
  auto& card = live_card(context, project_id, card_id);
  if (std::find(card.tags.begin(), card.tags.end(), tag) != card.tags.end())
    throw Conflict("the card carries #" + tag + " only in its prose");
  return card;
}

std::vector<TagCount> list_project_tags(holder_context& context, std::string project_id) {
  if (project_id != "p1") throw NotFound("project not found: " + project_id);
  std::map<std::string, std::int32_t> counts;
  for (const auto& [id, card] : context.cards)
    if (!card.deleted_at)
      for (const auto& tag : card.tags) ++counts[tag];
  std::vector<TagCount> out;
  for (const auto& [tag, count] : counts) out.push_back({tag, count});
  return out;
}
// Links, from the card to whatever it names: the stand-in neither checks targets nor lists backlinks.
std::vector<Link> list_card_links(holder_context& context, std::string project_id, std::string card_id,
                                  bool include_deleted) {
  (void)include_deleted;
  live_card(context, project_id, card_id);
  std::vector<Link> out;
  for (const auto& link : context.links)
    if (link.source_id == card_id) out.push_back(link);
  return out;
}

std::vector<Link> list_card_backlinks(holder_context& context, std::string project_id, std::string card_id,
                                      ListCardBacklinksScope scope, bool include_deleted) {
  (void)scope, (void)include_deleted;
  live_card(context, project_id, card_id);
  return {};
}

Link add_card_link(holder_context& context, std::string project_id, std::string card_id, const NewLink& request) {
  live_card(context, project_id, card_id);
  Link link{project_id, LinkSourceType::Card, card_id, request.target_project_id.value_or(project_id),
            static_cast<LinkTargetType>(request.target_type), request.target_id, request.kind, request.label,
            request.created_at.value_or(1793000300)};
  context.links.push_back(link);
  return link;
}

std::vector<Link> remove_card_link(holder_context& context, std::string project_id, std::string card_id,
                                   RemoveCardLinkTargetType target_type, std::string target_id, std::string kind,
                                   std::optional<std::string> target_project_id) {
  std::erase_if(context.links, [&](const Link& link) {
    return link.source_id == card_id && link.target_type == static_cast<LinkTargetType>(target_type) &&
           link.target_id == target_id && link.kind == kind &&
           link.target_project_id == target_project_id.value_or(project_id);
  });
  return list_card_links(context, project_id, card_id, false);
}
// Milestones, kept in the order they were made: the stand-in neither checks dates nor searches by them.
std::vector<Milestone> list_card_milestones(holder_context& context, std::string project_id, std::string card_id) {
  live_card(context, project_id, card_id);
  std::vector<Milestone> out;
  for (const auto& milestone : context.milestones)
    if (milestone.card_id == card_id) out.push_back(milestone);
  return out;
}

Milestone add_card_milestone(holder_context& context, std::string project_id, std::string card_id,
                             const NewMilestone& request) {
  live_card(context, project_id, card_id);
  Milestone milestone{"m" + std::to_string(context.next_id++), project_id, card_id, request.dates, request.kind,
                      request.description, 1793000400, 1793000400};
  context.milestones.push_back(milestone);
  return milestone;
}

Milestone update_card_milestone(holder_context& context, std::string project_id, std::string card_id,
                                std::string milestone_id, const MilestoneUpdate& update) {
  live_card(context, project_id, card_id);
  for (auto& milestone : context.milestones) {
    if (milestone.milestone_id != milestone_id || milestone.card_id != card_id) continue;
    if (update.dates) milestone.dates = *update.dates;
    if (update.kind) milestone.kind = *update.kind;
    if (update.description) milestone.description = *update.description;
    return milestone;
  }
  throw NotFound("milestone not found: " + milestone_id);
}

std::vector<Milestone> remove_card_milestone(holder_context& context, std::string project_id, std::string card_id,
                                             std::string milestone_id) {
  live_card(context, project_id, card_id);
  std::erase_if(context.milestones, [&](const Milestone& milestone) {
    return milestone.milestone_id == milestone_id && milestone.card_id == card_id;
  });
  return list_card_milestones(context, project_id, card_id);
}

std::vector<Milestone> list_milestones_by_day(holder_context& context, std::string project_id, std::string first_day,
                                              std::string last_day, std::optional<std::string> role) {
  (void)context, (void)project_id, (void)first_day, (void)last_day, (void)role;
  return {};
}

std::vector<Milestone> list_milestones_by_instant(holder_context& context, std::string project_id,
                                                  long long first_instant, long long last_instant,
                                                  std::optional<std::string> role) {
  (void)context, (void)project_id, (void)first_instant, (void)last_instant, (void)role;
  return {};
}

Calendar get_calendar(holder_context& context, std::string project_id, long long first_instant,
                      long long last_instant, std::string first_day, std::string last_day) {
  (void)first_instant, (void)last_instant, (void)first_day, (void)last_day;
  Calendar out;
  for (const auto& milestone : context.milestones)
    if (milestone.project_id == project_id)
      out.milestones.push_back({milestone, live_card(context, project_id, milestone.card_id).title});
  return out;
}
// Resources, kept in the order they were made: the stand-in keeps no files and no attachments.
Resource& resource_in(holder_context& context, const std::string& project_id, const std::string& resource_id) {
  for (auto& resource : context.resources)
    if (resource.resource_id == resource_id && resource.project_id == project_id) return resource;
  throw NotFound("resource not found: " + resource_id);
}

std::vector<Resource> list_resources(holder_context& context, std::string project_id) {
  std::vector<Resource> out;
  for (const auto& resource : context.resources)
    if (resource.project_id == project_id) out.push_back(resource);
  return out;
}

Resource get_resource(holder_context& context, std::string project_id, std::string resource_id) {
  return resource_in(context, project_id, resource_id);
}

Resource create_resource(holder_context& context, std::string project_id, const NewResource& request) {
  Resource resource{request.resource_id.value_or("r" + std::to_string(context.next_id++)),
                    project_id,
                    request.type,
                    request.label,
                    request.metadata.value_or(std::map<std::string, std::vector<std::string>>{}),
                    {},
                    request.created_at.value_or(1793000500),
                    request.updated_at.value_or(request.created_at.value_or(1793000500))};
  context.resources.push_back(resource);
  return resource;
}

Resource update_resource(holder_context& context, std::string project_id, std::string resource_id,
                         const ResourceUpdate& update) {
  auto& resource = resource_in(context, project_id, resource_id);
  if (update.type) resource.type = *update.type;
  if (update.label) resource.label = *update.label;
  if (update.metadata)
    for (const auto& [name, values] : *update.metadata) {
      if (values.empty())
        resource.metadata.erase(name);
      else
        resource.metadata[name] = values;
    }
  return resource;
}

Resource delete_resource(holder_context& context, std::string project_id, std::string resource_id) {
  const auto resource = resource_in(context, project_id, resource_id);
  std::erase_if(context.resources, [&](const Resource& r) { return r.resource_id == resource_id; });
  return resource;
}

std::vector<Link> list_resource_backlinks(holder_context& context, std::string project_id, std::string resource_id,
                                          bool include_deleted) {
  (void)include_deleted;
  resource_in(context, project_id, resource_id);
  return {};
}

std::vector<Resource> list_card_resources(holder_context& context, std::string project_id, std::string card_id) {
  live_card(context, project_id, card_id);
  return {};
}

std::vector<Resource> attach_card_resource(holder_context& context, std::string project_id, std::string card_id,
                                           std::string resource_id) {
  live_card(context, project_id, card_id);
  return {resource_in(context, project_id, resource_id)};
}

std::vector<Resource> detach_card_resource(holder_context& context, std::string project_id, std::string card_id,
                                           std::string resource_id) {
  (void)resource_id;
  return list_card_resources(context, project_id, card_id);
}
// Projects, kept in the order they were made: the stand-in keeps no repositories.
Project& project_named(holder_context& context, const std::string& project_id) {
  for (auto& project : context.projects)
    if (project.project_id == project_id) return project;
  throw NotFound("project not found: " + project_id);
}

std::vector<ProjectListItem> list_projects(holder_context& context, std::optional<std::string> name,
                                           std::optional<long long> updated_after,
                                           std::optional<long long> updated_before,
                                           std::optional<ListProjectsOrder> order, std::int32_t limit,
                                           std::int32_t offset, bool include_card_counts) {
  (void)updated_after, (void)updated_before, (void)order, (void)include_card_counts;
  std::vector<ProjectListItem> out;
  for (const auto& project : context.projects)
    if (!name || project.name.find(*name) != std::string::npos) out.push_back({project, std::nullopt, std::nullopt});
  out.erase(out.begin(), out.begin() + std::min<std::size_t>(out.size(), offset));
  if (out.size() > static_cast<std::size_t>(limit)) out.resize(limit);
  return out;
}

Project get_project(holder_context& context, std::string project_id) { return project_named(context, project_id); }

Project create_project(holder_context& context, const NewProject& request) {
  Project project{request.project_id.value_or("p" + std::to_string(context.next_id++)),
                  request.name,
                  request.root_path.value_or("/projects/" + request.name),
                  request.git_remote_url,
                  request.git_provider,
                  request.privacy_mode == NewProjectPrivacyMode::Plain ? ProjectPrivacyMode::Plain
                                                                       : ProjectPrivacyMode::EncryptedGit,
                  std::nullopt,
                  request.created_at.value_or(1793000600),
                  request.updated_at.value_or(request.created_at.value_or(1793000600))};
  context.projects.push_back(project);
  return project;
}

Project update_project(holder_context& context, std::string project_id, const ProjectUpdate& update) {
  auto& project = project_named(context, project_id);
  if (update.name) project.name = *update.name;
  if (update.git_remote_url) project.git_remote_url = *update.git_remote_url;
  if (update.git_provider) project.git_provider = *update.git_provider;
  return project;
}

Project delete_project(holder_context& context, std::string project_id) {
  const auto project = project_named(context, project_id);
  std::erase_if(context.projects, [&](const Project& p) { return p.project_id == project_id; });
  return project;
}
// Git sync: the stand-in has no remotes, so every project has none.
SyncState get_project_sync_state(holder_context& context, std::string project_id) {
  project_named(context, project_id);
  return {project_id, {}, {}, {}, 0, 0, {}, {}, {}, {}, 0, {}, 0, {}, {}};
}

SyncResult no_remote(holder_context& context, const std::string& project_id, bool pull, bool push) {
  project_named(context, project_id);
  return {project_id,
          SyncResultStatus::Failed,
          std::nullopt,
          {pull, pull ? PullPhaseStatus::RemoteUnset : PullPhaseStatus::NotAttempted, 0, 0, std::nullopt},
          {push && !pull, push && !pull ? PushPhaseStatus::RemoteUnset : PushPhaseStatus::NotAttempted, 0, 0,
           std::nullopt, std::nullopt}};
}

SyncResult pull_project(holder_context& context, std::string project_id) {
  return no_remote(context, project_id, true, false);
}

SyncResult push_project(holder_context& context, std::string project_id, const PushOptions& options) {
  (void)options;
  return no_remote(context, project_id, false, true);
}

SyncResult sync_project(holder_context& context, std::string project_id, const PushOptions& options) {
  (void)options;
  return no_remote(context, project_id, true, true);
}

RemoteTestResult test_project_remote(holder_context& context, std::string project_id, const RemoteTest& remote) {
  project_named(context, project_id);
  return {project_id, RemoteTestResultStatus::RemoteUnset, remote.remote_url, false, std::nullopt};
}

ProjectHistoryPage list_project_history(holder_context& context, std::string project_id, std::int32_t limit,
                                        std::optional<std::string> cursor, std::optional<ListProjectHistoryKind> kind) {
  project_named(context, project_id);
  (void)limit;
  (void)cursor;
  (void)kind;
  return {std::nullopt, {}, std::nullopt, false};
}

CardHistoryPage list_card_history(holder_context& context, std::string project_id, std::string card_id,
                                  std::int32_t limit, std::optional<std::string> cursor) {
  project_named(context, project_id);
  (void)card_id;
  (void)limit;
  (void)cursor;
  return {std::nullopt, {}, std::nullopt, false};
}

CardHistoryComparison compare_card_history(holder_context& context, std::string project_id,
                                           std::string card_id, std::string to_oid,
                                           std::optional<std::string> from_oid, CompareCardHistoryMode mode) {
  project_named(context, project_id);
  (void)card_id;
  (void)from_oid;
  (void)mode;
  CardHistoryVersion version{false, to_oid, "", ""};
  return {version, version, "", {}, false};
}

CardHistorySnapshot get_card_history_snapshot(holder_context& context, std::string project_id,
                                              std::string card_id, std::string oid) {
  project_named(context, project_id);
  return {card_id, {false, oid, "", ""}};
}

CardHistoryRestoreResult restore_card_history_snapshot(holder_context& context, std::string project_id,
                                                       std::string card_id, std::string oid) {
  project_named(context, project_id);
  return {card_id, oid, oid, "restored", 1, std::nullopt};
}
}  // namespace holder::contract

extern "C" holder_context* stub_context_new(void) { return new holder_context; }
extern "C" void stub_context_free(holder_context* context) { delete context; }
extern "C" const char* holder_error_message(const holder_error* error) { return error ? error->message.c_str() : ""; }
extern "C" void holder_error_destroy(holder_error* error) { delete error; }
