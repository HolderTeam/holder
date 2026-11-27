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

The contract-based Card API provides the same operations for a running Holder
installation or a private embedded workspace. Both sessions return detached
records. Use `session.list_projects()` to find a project and
`session.get_project(id)` to fetch its record.

To start with an empty private workspace:

```python
from holder import open_workspace

with open_workspace("private-holder-workspace") as session:
    project = session.create_plain_project(name="Research")
    project_id = project["project_id"]
    cards = session.card_operations
    card = cards.create_card(project_id, {"title": "An idea", "content": "My notes"})
    cards.update_card(project_id, card["card_id"], {"title": "A better title"})
    print(cards.list_cards(project_id)["items"])
```

Opening a private workspace requires a native Holder wheel or a same-checkout
Holder Kit build. Use a separate data directory for embedded work. To run this
code against the desktop Holder profile, open the session with `Holder()` or
`open_session()`. The desktop profile requires HTTP even when its daemon is stopped.
Opening a directory already in use by a daemon or another embedded session
raises a workspace conflict.
`create_plain_project` stores project files without encryption in both modes.

`list_cards` returns one page at a time. Pass its `next_cursor` with the same
filters to fetch the next page. For updates, omit fields to keep their values
and set `parent_card_id` to `None` to move a card to the root.

Use `cards.get_card_metadata(project_id, card_id)` for the card's title,
parent, and other metadata without reading its body.
`cards.get_card_breadcrumbs(project_id, card_id)` returns the path from the
project through the card's live ancestors to the card itself.

Use `cards.search_cards(project_id, '"café notes"')` to search live card
titles and bodies with an SQLite FTS5 query. Results are ordered by relevance
and include excerpts from the card body. Use `limit` and `offset` to read
additional results.

To look up an exact title, full ID, or eligible ID prefix, use
`cards.resolve_card_reference({"project_id": project_id, "reference": "An idea",
"scope": "live"})`. Use `trashed` to search Trash or `either` to search both.
The result reports a match, two example candidates for an ambiguous reference,
or no match.

Use `cards.move_card(project_id, card_id, {"intent": "into",
"target_card_id": another_card_id})` to move a card into another one.
For reordering among siblings, use `left`, `right`, `to_start`, or `to_end`.
Omit `parent_card_id` to reorder within the current parent or pass
`None` to reorder among root cards.

Use `cards.trash_card(project_id, card_id)` to move a card to Trash and
promote its children. Pass `children="trash_subtree"` to trash the descendants
too. `cards.restore_card(project_id, card_id)` restores one card at a time.
Both calls return the card's record after the change. To find a card to
restore later, use `session.list_trash(project_id, type="card")`. The listing
includes titles and IDs. Omit the filter to include trashed AI messages too.

Use `cards.add_card_tag(project_id, card_id, "research")` to add a tag and
`cards.remove_card_tag(project_id, card_id, "research")` to remove it from
the trailing tag line. Removing a tag mentioned in prose raises a conflict.
`session.list_project_tags(project_id)` returns tag names and live card
counts. Pass `tag="research"` to `list_cards` to find matching cards.

Use `cards.list_card_links(project_id, card_id)` for outgoing links and
`cards.list_card_backlinks(project_id, card_id)` for incoming links. Each
record identifies both ends by project, type, and ID. Backlinks default to
the current project. Pass `scope="external"` for other projects on this
device or `scope="all_available"` for both. Use `include_deleted=True` to
include trashed targets in outgoing links or trashed sources in backlinks.

Use `cards.add_card_link(project_id, card_id, {"target_type": "card",
"target_id": target_id})` to create a link. Adding the same link again updates
its label and preserves its creation time. Use
`cards.remove_card_link(project_id, card_id, "card", target_id)` to remove
that link and get the remaining outgoing links. Specify `kind` or
`target_project_id` when they differ from `"ref"` and the source project.
`session.list_link_kinds()` returns the built-in kinds and their forward and
reverse labels. You can also use custom kinds.

Use `cards.list_card_milestones(project_id, card_id)` to read a card's
milestones. To add one, pass an ordered list of dates to
`cards.add_card_milestone(project_id, card_id, {"dates": [{"role": "due",
"date": "2026-11-30"}]})`. Each entry takes a calendar `date` or an RFC 3339
`datetime`. Use `update_card_milestone` to change fields and
`remove_card_milestone` to remove one by ID. On update, omit fields to keep them,
use `None` to clear the kind or description, or pass `dates=[]` to clear dates.

To find milestones across a project, call
`session.list_milestones_by_day(project_id, first_day, last_day)` for calendar
dates or `session.list_milestones_by_instant(project_id, first, last)` for Unix
timestamps in seconds. Both ranges are inclusive. Pass `role="due"` to match
only dates with that role. Each matching milestone appears once.

Use `session.get_calendar(project_id, first_day, last_day,
time_zone="Europe/London")` to read milestones, newly created cards, and
updated cards in a window. Calendar dates stay on their labelled day. Holder
uses the selected time zone to decide which timestamps fall inside the window.

## Run with or without a local Holder installation

Use `open_session(fallback_workspace=path)` when a script can work on live
Holder data or on the private workspace you supply. It uses or starts the
local daemon when available. If the daemon is absent, it opens the private
workspace with the native extension.

```python
from pathlib import Path

from holder import open_session

with open_session(fallback_workspace=Path("private-holder-workspace")) as session:
    for project in session.list_projects():
        print(project["name"])
```

The backend is chosen when entering the session. A startup, authentication,
or request error is reported to the caller. Supply a URL and token to connect
to a specific daemon, or use `open_workspace(path)` to select private data
regardless of daemon availability.

## Export project records for analysis

Use the same call in a live session or a private workspace:

```python
from holder import open_session

with open_session() as session:
    records = session.export_project_records("your-project-id", include_content=True)

print(records["tag_memberships"])
```

The result includes the project record, live cards, outgoing links, tag
memberships, and card milestones. Links can point to other projects. Card
bodies are omitted by default. Records are ordinary Python dictionaries and
lists that you can serialize or use after closing the session. Each read uses
current data, so edits during export can affect the results. Use the Card
iterator below if you only need cards and want to process them a page at a time.

## Export Card records for analysis

`iter_card_records()` reads one project's live cards through the active session
and yields detached dictionaries. Use `session.list_projects()` to find the
project ID. Card bodies are optional, as in
[Holder Kit's record exports](../kit/docs/walkthrough.md#export-ordinary-python-records).

```python
import json

from holder import iter_card_records, open_session

project_id = "your-project-id"
with open_session() as session:
    records = iter_card_records(session.card_operations, project_id, include_content=True)
    with open("cards.jsonl", "w", encoding="utf-8") as output:
        for record in records:
            output.write(json.dumps(record, ensure_ascii=False) + "\n")
```

Use `list(records)` inside the session if you want the dictionaries directly
in a notebook. Each page reads the project's current state, so changes made
during export can appear in the result. The export contains card listing
records and optional Markdown bodies. For an encrypted project, including
bodies writes their decrypted text to the output file.

## Make an editable copy of a project's cards

Download a recovery snapshot through the daemon, then restore it as a new
project in a separate private workspace:

```python
from holder import open_session, open_workspace

with open_session() as session:
    snapshot = session.download_recovery_snapshot("your-project-id")

with open_workspace("private-research-workspace") as workspace:
    project = workspace.restore_recovery_snapshot(snapshot, name="Research copy")
    print(workspace.card_operations.list_cards(project["project_id"])["items"])
```

The download is capped at 2 MiB and includes live card bodies, hierarchy,
milestones, links, and linked resource declarations. Asset bytes stay in their
original storage locations. Restored projects have new IDs and are stored
without encryption. Treat a snapshot containing decrypted card bodies as
plaintext, including if you save the download to a file.

To save the snapshot and its attachments together, call
`session.download_recovery_bundle(project_id, "project.zip")`. The ZIP contains
the same recovery lines and one plaintext payload per distinct asset hash.
Holder verifies all attachments before replacing the destination. Restore the
archive with `workspace.restore_recovery_bundle("project.zip", name="Research copy")`.
The new project uses local asset copies inside the private workspace.

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
