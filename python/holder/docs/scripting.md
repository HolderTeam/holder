# More Holder scripting tasks

Start with the [README](../README.md) for installation and a first script.
The examples below assume you have a `project` or `card` inside a
`with Holder() as holder:` block.

## Create projects with options

```python
from pathlib import Path

project = holder.projects.create(
    name="Research",
    root_path=Path("/path/to/my/project"),
    privacy_mode="encrypted_git",
)
```

The daemon chooses IDs, timestamps and card placement. You can omit
`root_path` and `privacy_mode`; the daemon then uses its defaults, including
encrypted Git for privacy. You can also choose `privacy_mode="plain"`.
Creation returns a fully loaded object. Filters on a collection do not supply
values when you create an object through it; `card.children.create(...)` does
supply the parent.

## Work with tags

Tags belong to a project. Add and remove them on a card, or list cards sharing
a tag:

```python
card.tags.add("research")
card.tags.remove("draft")
tag = project.tags.get("research")
for related_card in tag.cards:
    print(related_card.title)
```

The daemon validates and normalizes tag names. `project.tags.get(name)` uses
the normalized name. Removing a tag edits its trailing tag line; a mention in
the body stays until you edit the content. Project tags include `card_count`
and are ordered by descending count, then name.

## Move, trash and restore cards

Supply exactly one target when moving a card:

```python
card.move(parent=another_card)  # append as a child
card.move(parent=None)          # append at the project's top level
card.move(before=sibling)
card.move(after=sibling)
```

Moving before or after another card adopts that card's parent. The daemon
checks project and hierarchy rules.

```python
card.trash()                          # its live children move up a level
card.trash(children="trash_subtree")  # its live descendants go to Trash too
card.restore()                        # restore this card alone
```

When you restore a card, children that were moved up stay where they are.

## Find and repair hidden cards

Older clients, manual file removal, or sync can leave cards under a missing or
trashed parent. Preview a repair before applying it:

```python
repair = project.repair_hierarchy()
print(repair.moved)
repair = project.repair_hierarchy(apply=True)
```

The daemon also repairs at startup and after every pull.

## Search and filter

```python
for result in project.cards.search("research notes"):
    print(result.title, result.snippet)
    print(result.card.content)
```

Search results are ordered by relevance and loaded as you iterate. The
`result.card` property fetches and caches the current card on first access;
the snippet reflects the search result. Search collections support indexing,
`.all()` and `.refresh()`. Search uses the daemon's full-text syntax and cannot
be combined with tag or hierarchy filters, so search through `project.cards`.
For listing, use `project.cards.filter(tag="research")`,
`project.cards.roots()`, or `card.children`. Tag filters can be combined with
hierarchy queries. Card listings exclude trashed cards and are always scoped
to a project.

Project filters are `name` (substring), `updated_after`, and `updated_before`.
`get(id)` fetches an object directly. Collection indexes must be nonnegative.

## Refresh data after changes

Collections load as you iterate and cache results they have visited. To see
changes made after a listing was loaded, call `.refresh()` on that collection:

```python
cards = project.cards
for card in cards:
    print(card.title)

project.cards.create(title="Another idea")
cards.refresh()
for card in cards:
    print(card.title)
```

`.all()` makes a fresh, independent query. Objects also have `.refresh()`.
Card bodies load when you read `card.content`. Explicit `.update(...)` writes
changes; card attributes cannot be assigned directly. Moves and tag changes
refresh the affected card, but other previously loaded listings still need `.refresh()`.
A move may also change nearby siblings' sort keys. Iteration observes live
data rather than a frozen snapshot.

## Connect to a specific daemon

For a daemon you already know how to reach, supply both its URL and bearer
token:

```python
import os

from holder import Holder

with Holder(url="http://127.0.0.1:11499", token=os.environ["HOLDER_TOKEN"]) as holder:
    for project in holder.projects:
        print(project.name)
```

Connection and API failures raise exceptions from `holder.exceptions`.
For direct endpoint access, see the [low-level client](low-level-client.md).
