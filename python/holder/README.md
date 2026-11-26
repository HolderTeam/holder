# Holder for Python

Holder is an open-source, local-first knowledge app for organising your ideas
and digital information as index cards and the connections between them.

Use Python to read and update the projects and cards in your running Holder
installation. Changes you make in a script appear in Holder too.

## Get started

1. You need the Holder application on your computer.
Get it from https://www.holder.team/

2. You need Python 3.11 or newer.
If in doubt, get the latest version (i.e. 3.15) from:
https://www.python.org/downloads/

3. Install the Holder Python library with:

```sh
python3 -m pip install holder-python
```

Save this as `list_cards.py` and run it with `python3 list_cards.py`:

```python
from holder import Holder

with Holder() as holder:
    for project in holder.projects:
        print(project.name)
        for card in project.cards:
            print("  ", card.title)
```

`Holder()` finds or starts your local Holder daemon using `holderctl`. The
`with` block closes the Python connection when the script ends; it does not
stop Holder.

## Create and edit cards

This example creates a project and two cards. Run it once: each run creates
another project named `Research`.

```python
from holder import Holder

with Holder() as holder:
    project = holder.projects.create(name="Research")
    card = project.cards.create(title="An idea", content="My notes")
    child = card.children.create(title="Next step")
    child.update(content="Try it out")
    card.tags.add("research")
```

To work with an existing project, find it by name and get a card by its ID:

```python
from holder import Holder

with Holder() as holder:
    matches = holder.projects.filter(name="Research")
    project = matches[0]
    for card in project.cards:
        print(card.id, card.title)

    # Replace this with an ID printed above.
    card = project.cards.get("card-id-here")
    card.update(title="A better title")
```

The name filter matches part of a name. If there are no matches, `[0]` raises
`IndexError`. Card content is available as `card.content`; use `card.update(...)`
to save edits. Card attributes cannot be assigned directly.

## Find cards

Search within a project and inspect the matching text:

```python
from holder import Holder

with Holder() as holder:
    project = holder.projects.filter(name="Research")[0]
    for result in project.cards.search("research notes"):
        print(result.title, result.snippet)
```

You can also list cards with a tag using
`project.cards.filter(tag="research")`, or list just top-level cards with
`project.cards.roots()`. For the latest changes, use
`project.cards.recent(limit=20)`. Roots and `card.children` follow sibling order.
Use `.with_child_counts()` on a card collection for counts and
`.with_content()` to fetch card bodies in listing pages. `card.breadcrumbs`
gives its current project-to-card path. Project listings support
`.alphabetical()`, `.recent()`, and `.with_card_counts()`.
Use `project.cards.resolve("Roadmap")` to open a live card by exact title or
unambiguous ID prefix; `project.cards.lookup(...)` also reports ambiguous and
Trash matches. See [reference lookup](docs/scripting.md#find-a-card-by-name-or-partial-id).

## Shared Card operations

The contract-based Card API provides detached records and the same three
operations for a running Holder installation or a private embedded workspace.
Use `session.list_projects()` to find a project and `session.get_project(id)`
to fetch its record. The same code works with either backend:

```python
from holder import Holder
from holder.cards import CardOperations


def retitle(cards: CardOperations, project_id: str, card_id: str) -> None:
    cards.update_card(project_id, card_id, {"title": "A better title"})
    print(cards.get_card(project_id, card_id)["title"])


with Holder() as session:
    for project in session.list_projects():
        project_id = project["project_id"]
        page = session.card_operations.list_cards(project_id, limit=1)
        if page["items"]:
            retitle(session.card_operations, project_id, page["items"][0]["card_id"])
            break
```

With a native Holder wheel or a same-checkout Holder Kit build installed,
open an existing private workspace using
`with holder.open_workspace(data_dir) as session:` and pass
`session.card_operations` to the same function. Use a separate data directory for embedded
work. `list_cards` returns one page at a time. Pass its `next_cursor` with the
same filters to fetch the next page. For updates, omit fields to keep their
values and set `parent_card_id` to `None` to move a card to the root.

## More help

- [More scripting tasks](docs/scripting.md): links, milestones, project-wide
  milestone date searches, moves, Trash,
  resources, attachments, file imports, downloads, storage locations, tags,
  project Git sync, remote checks, recovery tokens, encryption checks,
  project removal, live change notifications, project activity, card history,
  intelligence threads, messages, search, runs, providers, and local models,
  collection refreshes, and connecting to a
  specific daemon.
- [Low-level client](docs/low-level-client.md): direct access to HTTP endpoints.
- [Development](docs/development.md): tests, builds, and generated code.
- [Publishing](docs/publishing.md): PyPI setup and release steps.

For offline analysis and bulk work without a running Holder installation, see
[Holder Kit](../kit/).
