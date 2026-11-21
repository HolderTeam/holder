#include "sync/CardMerge.h"

#include <algorithm>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace holder::sync {
namespace {

using holder::core::ParsedCardFile;
using holder::model::CardLink;
using holder::model::Milestone;

// The three-way rule for one value: unchanged sides defer to the side that changed it.
// Returns the side to keep, or null when both sides changed it differently.
template <class T, class Same>
const T* pick(const T& base, const T& local, const T& remote, Same same) {
  if (same(local, remote)) return &remote;
  if (same(local, base)) return &remote;
  if (same(remote, base)) return &local;
  return nullptr;
}

template <class T> bool equal(const T& a, const T& b) { return a == b; }

// A link is the same link when it points the same way with the same kind. Its label is the
// value both sides can change; created_at only records when it was first made.
using LinkKey = std::tuple<std::string, std::string, std::string, std::string>;

LinkKey link_key(const CardLink& link) {
  return {link.to_type, link.to_card_id, link.target_project_id.value_or(""), link.kind};
}

bool same_link(const CardLink* a, const CardLink* b) {
  if (a == nullptr || b == nullptr) return a == b;
  return a->label == b->label;
}

bool same_milestone(const Milestone* a, const Milestone* b) {
  if (a == nullptr || b == nullptr) return a == b;
  const auto same_dates = std::ranges::equal(a->dates, b->dates, [](const auto& x, const auto& y) {
    return std::tie(x.role, x.date, x.datetime) == std::tie(y.role, y.date, y.datetime);
  });
  return same_dates && a->kind == b->kind && a->description == b->description &&
         a->created_at == b->created_at;
}

// Indexes one version's items by key. A version listing the same key twice is not something
// this merge can reason about, so it reports failure instead.
template <class T, class Key, class KeyOf>
bool index_items(const std::vector<T>& items, KeyOf key_of, std::map<Key, const T*>& out) {
  for (const auto& item : items) {
    if (!out.emplace(key_of(item), &item).second) return false;
  }
  return true;
}

// Merges keyed collections (links or milestones) item by item. Items keep the remote order,
// followed by items only the local version has, in its order.
template <class T, class Key, class KeyOf, class Same>
std::optional<std::vector<T>> merge_items(
    const std::vector<T>& base,
    const std::vector<T>& local,
    const std::vector<T>& remote,
    KeyOf key_of,
    Same same
) {
  std::map<Key, const T*> base_items, local_items, remote_items;
  if (!index_items(base, key_of, base_items) || !index_items(local, key_of, local_items) ||
      !index_items(remote, key_of, remote_items))
    return std::nullopt;

  const auto find = [](const std::map<Key, const T*>& items, const Key& key) -> const T* {
    const auto it = items.find(key);
    return it == items.end() ? nullptr : it->second;
  };
  std::map<Key, const T*> kept;
  for (const auto* side : {&local_items, &remote_items, &base_items}) {
    for (const auto& [key, ignored] : *side) {
      if (kept.contains(key)) continue;
      const T* b = find(base_items, key);
      const T* l = find(local_items, key);
      const T* r = find(remote_items, key);
      const auto* chosen = pick(b, l, r, same);
      if (chosen == nullptr) return std::nullopt;
      kept.emplace(key, *chosen);
    }
  }

  std::vector<T> out;
  for (const auto* side : {&remote, &local}) {
    for (const auto& item : *side) {
      const auto key = key_of(item);
      auto it = kept.find(key);
      if (it == kept.end() || it->second == nullptr) continue;
      out.push_back(*it->second);
      it->second = nullptr;
    }
  }
  return out;
}

} // namespace

std::optional<ParsedCardFile> merge_card_versions(
    const ParsedCardFile& base,
    const ParsedCardFile& local,
    const ParsedCardFile& remote
) {
  for (const auto* version : {&base, &local, &remote}) {
    if (!version->has_front_matter) return std::nullopt;
  }
  const auto identity = [](const ParsedCardFile& version) {
    const auto& card = version.card;
    return std::tie(card.card_id, card.project_id, card.rel_path, card.created_at);
  };
  if (identity(local) != identity(base) || identity(remote) != identity(base)) return std::nullopt;

  // Moving a card to or from Trash on one side while the other edits it keeps both versions.
  if (local.card.deleted_at != base.card.deleted_at ||
      remote.card.deleted_at != base.card.deleted_at)
    return std::nullopt;

  const auto* title =
      pick(base.card.title, local.card.title, remote.card.title, equal<std::string>);
  const auto placement = [](const ParsedCardFile& version) {
    return std::make_pair(version.card.parent_card_id, version.card.sort_key);
  };
  const auto base_place = placement(base);
  const auto local_place = placement(local);
  const auto remote_place = placement(remote);
  const auto* place = pick(base_place, local_place, remote_place, equal<decltype(base_place)>);
  const auto* body = pick(base.body, local.body, remote.body, equal<std::string>);
  if (title == nullptr || place == nullptr || body == nullptr) return std::nullopt;

  auto links =
      merge_items<CardLink, LinkKey>(base.links, local.links, remote.links, link_key, same_link);
  auto milestones = merge_items<Milestone, std::string>(
      base.milestones,
      local.milestones,
      remote.milestones,
      [](const Milestone& milestone) {
        return milestone.milestone_id;
      },
      same_milestone
  );
  if (!links || !milestones) return std::nullopt;

  ParsedCardFile merged = remote;
  merged.card.title = *title;
  merged.card.parent_card_id = place->first;
  merged.card.sort_key = place->second;
  merged.card.updated_at = std::max(local.card.updated_at, remote.card.updated_at);
  merged.body = *body;
  merged.links = std::move(*links);
  merged.milestones = std::move(*milestones);
  return merged;
}

} // namespace holder::sync
