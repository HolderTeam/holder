# Holder for Python

Holder is an open-source, local-first knowledge app for organising your ideas
and digital information as index cards and the connections between them.

Use Python to read and update the projects and cards in your running Holder
installation. Changes you make in a script appear in Holder too.

## Get started

1. To work with your running Holder installation, install the Holder application.
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
To work without the application, see [Run with or without a local Holder
installation](#run-with-or-without-a-local-holder-installation).

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

Opening a private workspace requires a native Holder wheel. See
[building native wheels](docs/development.md#build-the-linux-wheel-variants)
for source development. Use a separate data directory for embedded work. To
run this code against the desktop Holder profile, open the session with `Holder()` or
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

Use `session.delete_trash_item(project_id, card_id, type="card")` to permanently
remove an item already in Trash. The same call accepts `type="ai_message"`
with a message ID. It returns the removed ID. Older card revisions remain
available in Git history.

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

Use `iter_project_records(session)` to export every project visible in a
session, one project at a time. Pass `project_ids=[first_id, second_id]` to
select particular projects in that order.

### Use pandas or NetworkX

Install the extras with `pip install 'holder-python[pandas,graph]'`. This
notebook example also works with an embedded workspace opened via
`open_workspace(path)`.

```python
from holder import open_session, to_dataframes, to_networkx

with open_session() as session:
    records = session.export_project_records("your-project-id")

tables = to_dataframes(records)
display(tables["cards"][["title", "updated_at"]])
graph = to_networkx(records)
print(graph.number_of_nodes(), graph.number_of_edges())
```

The pandas adapter returns tables named `projects`, `cards`, `links`,
`tag_memberships`, and `milestones`. Timestamps are UTC. Request
`include_content=True` on the export if you need bodies in the cards table
and graph nodes.

The directed multigraph contains live cards and their explicit card links.
Nodes use `(project_id, card_id)` keys. Links to cards outside the export get
placeholder nodes with `exported=False`. Edges retain the link kind and use it
as their key.

Both adapters also take an iterable of project exports. Run this inside an
open session to build one graph for all of its projects:

```python
from holder import iter_project_records, open_session, to_networkx

with open_session() as session:
    graph = to_networkx(iter_project_records(session))
```

## Export Card records for analysis

`iter_card_records()` reads one project's live cards through the active session
and yields detached dictionaries. Use `session.list_projects()` to find the
project ID. Pass `include_content=True` to include card bodies.

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

Both `iter_card_records()` and `iter_card_batches()` accept `tag`, `parent`,
and `order`. Pass `parent="roots"` for root cards or a card ID for its direct
children. Order by card ID (`card_id_asc`, the default), most recently updated
(`updated_desc`), or sibling position (`sort_key_asc`).

```python
from holder import iter_card_batches, open_workspace

with open_workspace("research-workspace") as session:
    for batch in iter_card_batches(
        session.card_operations, project_id, batch_size=256,
        tag="research", parent="roots", order="updated_desc",
    ):
        print([card["title"] for card in batch])
```

`iter_card_batches()` yields a list at a time, and only the last list may be
shorter than `batch_size`.

## Make an editable copy of a project's cards

Copy a consistent snapshot of the live project into a new project in a separate
private workspace:

```python
from holder import Holder, open_workspace

with Holder() as session, open_workspace("private-research-workspace") as workspace:
    project = session.copy_project_to("your-project-id", workspace, name="Research copy")
    print(workspace.card_operations.list_cards(project["project_id"])["items"])
```

The copy includes live card bodies, hierarchy, milestones, links, and linked
resource declarations. Asset bytes stay in their original storage locations.
Restored projects have new IDs and are stored without encryption.

`copy_project_to` can copy projects larger than 2 MiB. It needs temporary disk
space on the daemon and Python machines. Both clean up their temporary files
when the operation ends or raises an error. If the download is interrupted,
the workspace has no new project. Once native restore starts, it runs
synchronously and removes the new project if a restore error occurs.
The temporary files contain plaintext, including decrypted card bodies.
Capture and restore process one record at a time. Restore also keeps IDs and
hierarchy in memory to remap references. For a capture that takes longer than
the default timeout, set a larger `timeout` on `Holder()`.

`download_recovery_snapshot` is available separately for snapshots up to 2 MiB
that you want as a Python string.

To take local copies of the attachments too, use
`session.download_recovery_bundle(project_id, "project.zip")` and
`workspace.restore_recovery_bundle("project.zip", name="Research copy")`.
The ZIP uses the same streamed snapshot and stores one plaintext payload per
distinct asset hash. Holder verifies attachments against the captured metadata
before replacing the ZIP destination, and verifies them again when restoring
them to new local placements in the private workspace.

## Clone and reopen a private Git workspace

Use `clone_workspace()` to work with the committed data of a plain Holder
project from a Git remote. Supply a new private directory and optionally
a branch, tag, or commit. The shared Card API is available on the embedded
session inside the workspace.

```python
from holder import clone_workspace, reopen_workspace

with clone_workspace(
    "ssh://git@example.org/research.git",
    workspace="research-workspace",
    ref="main",
) as work:
    work.session.card_operations.create_card(
        work.project_id, {"title": "Local notes", "content": "Review the evidence."}
    )

with reopen_workspace("research-workspace") as work:
    records = work.session.export_project_records(work.project_id)
```

Use `create_managed_workspace(name, workspace=path)` to start with an empty
plain project. Both operations require Git and an embedded Holder build.
Cloning uses a detached checkout and keeps the project's existing IDs.
Closing the context keeps local edits, and reopening uses the local checkout.
Clone accepts HTTPS, SSH, and Git remotes and checks the checkout before
importing it into the private database.

### Publish committed work

Preview the destination and pin the commit you reviewed when you publish.

```python
with reopen_workspace("research-workspace") as work:
    preview = work.preview_push(branch="analysis/review")
    print(preview)
    published = work.push(branch=preview.branch, expected_revision=preview.revision)
```

For a cloned workspace the destination defaults to the clone source. Supply
`remote_url=` to choose another destination, or to publish a workspace you
created locally. The first push creates a new branch. Subsequent pushes to
that destination and branch must fast-forward. Holder refuses the advertised
default branch and publishes only the selected commit. Local files and edits
stay in the workspace. If the connection drops during a push, retry the same
destination, branch, and revision to record a push the remote already accepted.

### Discard a managed workspace

Use `work.preview_discard()` to see the exact directory, whether any commits
lack a recorded publication, and whether the checkout has uncommitted or
untracked changes. To permanently remove that directory, call
`work.discard(confirm=True)`. Both work with an active or closed handle.
Discard closes the private session and deletes its local project, database,
and artifacts. Remote branches and assets stored outside the workspace stay
where they are. Publication records describe pushes that succeeded at the time.

## More help

- [From Holder Kit to `holder`](docs/from-holder-kit.md) maps Kit workflows to
  the shared Python API.
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
