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

Project listings can be ordered and can include live card totals:

```python
for project in holder.projects.filter(name="Research").alphabetical().with_card_counts():
    print(project.name, project.card_count, project.root_card_count)

for project in holder.projects.recent():
    print(project.name, project.updated_at)
```

`order_by("name" | "created_at" | "updated_at", descending=True)` gives
the other orders. The default project listing keeps the daemon's usual order;
choosing an order sorts by that field. `with_card_counts()` populates counts
on projects returned by that listing. A direct `holder.projects.get(id)` does
not request counts, so its count attributes remain `None`.

## Browse recent cards and the card tree

`project.cards.recent(limit=20)` lists live cards with the most recently
updated first. `project.cards.roots()` and `card.children` list one level at a
time in sibling order. Each returns the usual `Card` objects and can be
refreshed after changes made elsewhere.

```python
for card in project.cards.recent(limit=10):
    print(card.title)

for root in project.cards.roots():
    print(root.title)
    for child in root.children:
        print("  ", child.title)

for root in project.cards.roots().with_child_counts():
    print(root.title, root.child_count)

for card in project.cards.filter(tag="research").with_content():
    print(card.title, card.content)

for step in card.breadcrumbs:
    print(step.kind, step.id, step.title)
```

These are paginated queries over current data, so changing the tree during
iteration may change later pages. Use `refresh()` to read a collection again.
`with_child_counts()` includes each listed card's immediate live child count;
without it, `child_count` is `None`. `card.breadcrumbs` reads the current live
path from the project to that card on each access. A broken ancestor chain is
reported as an error instead of a partial path.
`with_content()` loads the body of each card in the listing page, avoiding a
separate request when you access `card.content`. It can be combined with roots,
children, recent cards, tags, and child counts. The daemon reads each content
page under a project lock; if a body cannot be read, that page fails.

## Find a card by name or partial ID

`project.cards.resolve(reference)` finds a live card by its full ID, an
unambiguous ID prefix, or an exact, case-sensitive title. It returns `None`
when there is no match and raises `AmbiguousCardReferenceError` when more than
one card matches. Its `candidates` attribute gives two example IDs and titles.

```python
from holder.exceptions import AmbiguousCardReferenceError

try:
    card = project.cards.resolve("Roadmap")
except AmbiguousCardReferenceError as error:
    for candidate in error.candidates:
        print(candidate.id, candidate.title)
else:
    if card is not None:
        print(card.title)
```

Use `project.cards.lookup(reference, scope="either")` when you need to inspect
the match status, how it matched, or a card in Trash. It returns a
`CardReferenceResult` with `status`, `match_kind`, `card`, and `candidates`.
The `card` and candidates here are lightweight summaries; they do not load
card content. Scopes are `"live"` (the default), `"trashed"`, and `"either"`.
Both methods look across the project, even when called on a filtered card
collection. `project.cards.get(id)` remains the direct lookup for a known,
full card ID.

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

## Connect cards

Links keep the target's project ID as part of their identity. An external
target may remain unresolved on this device; listing a link does not load or
grant access to its target.

```python
source = project.cards.get("source-card-id")
link = source.links.add(
    "target-card-id", target_project_id="other-project-id", label="Related work"
)
print(link.source_project_id, link.target_project_id, link.target_id)
for link in source.links:
    print(link.target_type, link.kind, link.label)
source.links.remove("target-card-id", target_project_id="other-project-id")
source.links.refresh()  # Reread changes made elsewhere.
```

`add` updates a link with the same qualified target, type, and kind. Use
`to_type` and `kind` when linking to a local AI message, thread, or resource.
`remove` deletes exactly one qualified target/type/kind; there is no bulk
delete method in this high-level API.

`card.backlinks` reads local backlinks by default. Choose
`card.backlinks.filter(scope="external")` or
`card.backlinks.filter(scope="all_available")` for other source projects.
After fetching, `coverage` is `"target_project"` for local results or
`"available_projects"` for the other scopes. Those scopes include only
projects available to the daemon for this request, and never claim to cover
every possible project. Each collection caches its results until `refresh()`.

## Track milestones on a card

`card.milestones` includes every milestone on the live card, including undated
ones. Each milestone has a stable `id`, optional `kind` and `description`, and
an ordered tuple of labelled dates. Roles are free-form and can repeat.

```python
from datetime import date, datetime, timedelta, timezone

from holder import MilestoneDate

task = card.milestones.create(kind="task")  # No dates yet.
event = card.milestones.create(
    dates=[
        MilestoneDate("deadline", date(2026, 6, 2)),
        MilestoneDate("deadline", date(2026, 6, 3)),
        MilestoneDate(
            "occurred",
            datetime(2026, 6, 2, 12, 30, tzinfo=timezone(timedelta(hours=2, minutes=30))),
        ),
    ],
    description="Workshop",
)
event.update(description=None)  # Clear only the description.
event.update(dates=[])          # Clear all dates; keep the milestone.
task.remove()
card.milestones.refresh()       # Reread changes made elsewhere.
```

Use `datetime.date` for a calendar day with no timezone. Use a timezone-aware
`datetime.datetime` for an instant; its UTC offset is preserved, but a named
timezone is not stored. Datetimes must have whole seconds and a minute-aligned
offset. An update leaves omitted fields unchanged; `None` clears `kind` or
`description`, and an empty `dates` list clears every date. Supplying dates
replaces the full ordered list. No completion or outstanding state is inferred
from any role. The older scheduled timestamp route and project calendar view
do not represent all of these values, so this workflow uses the complete
card-level date route.

## Search milestone dates across a project

Use `project.milestones.between(start, end, role=...)` to find milestones on
live cards whose labelled date has that role and falls within the inclusive
range. Both bounds must be calendar dates or both must be timezone-aware
datetimes. The two value types are searched separately.

```python
from datetime import date, datetime, timezone

upcoming = project.milestones.between(
    date(2026, 6, 1), date(2026, 6, 30), role="deadline"
)
for milestone in upcoming:
    print(milestone.card_id, milestone.description, milestone.dates)
    # milestone.card fetches the card only if needed.

instants = project.milestones.between(
    datetime(2026, 6, 1, tzinfo=timezone.utc),
    datetime(2026, 6, 30, 23, 59, 59, tzinfo=timezone.utc),
    role="occurred",
)
```

Each matching milestone appears once, even if several of its dates match.
Its `dates` tuple contains all labelled dates, including those outside the
search range. Results are lazy and cached until `upcoming.refresh()`. The
returned `Milestone` supports `refresh()`, `update()`, and `remove()` just like
one from `card.milestones`; `milestone.card` fetches its live card on demand.
Milestones on trashed cards do not appear in these searches.

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

Browse a project's Trash through `project.trash`. The listing loads on first
use and caches its results until `.refresh()`:

```python
for item in project.trash:
    print(item.type, item.id, item.when, item.title or item.role)

trashed_card = project.trash.cards()[0]
card = trashed_card.restore()  # returns the live Card

for message in project.trash.messages():
    print(message.thread_id, message.role)
```

`project.trash.cards()` and `.messages()` ask the daemon to filter the
listing. Restoring an AI message returns its live `Message`. To remove one item, call
`item.delete_permanently()`. To remove all items in the project, call
`project.trash.empty()`; use `project.trash.cards().empty()` or
`project.trash.messages().empty()` to limit it to one type. These operations
only act on items currently in Trash. They do not erase older card revisions
from Git history. Refresh a listing after restoring or removing items.

## Resources, attachments, and files

Resources belong to a project and can be attached to several cards. Detaching
one leaves the project resource available; deleting the resource removes its
card links. Resource metadata maps each key to an ordered list of strings.

```python
resource = project.resources.create(
    type="website", label="Reference",
    metadata={"identifier": ["https://example.com"]},
)
card.attach(resource)                    # True if a link was created
for attached in card.attachments:        # lazy, paginated listing
    print(attached.label, attached.assets)
card.detach(resource)                    # resource still exists
resource.update(metadata={"identifier": []})  # remove this metadata key
```

Use storage locations for file assets. A location declaration is portable;
its binding holds a path or credentials on the device running the daemon.
Binding values are never returned by the API. For example, on a local daemon:

```python
location = project.storage_locations.create(
    name="Files", provider="local_directory"
)
location.bind({"root_path": "/path/to/asset-store"}, preview="Asset store")
location.test()                          # uploads, checks, and removes a probe
location.make_preferred()

job = card.import_file("/path/to/report.pdf")  # uses the preferred location
finished = job.wait(timeout=120)
if finished.status == "failed":
    raise RuntimeError(finished.error)
resource = finished.resource
resource.assets[0].download_to("report-copy.pdf")
```

The import source path must be readable **on the daemon's machine**. Connecting
to a remote daemon does not upload a file from the Python machine. Imports run
in the background; `wait()` polls and returns on completion or failure, and
raises `TimeoutError` at its deadline. Job records are held in daemon memory,
so a restart can make a job ID unavailable. Downloads stream to a temporary
file and replace the destination only after success.

For S3 compatible storage, pass portable `configuration` at creation and
device-specific credentials to `location.bind(values)`.
`project.storage_locations.preferred_id` reads this device's current
preference. A location with live asset placements cannot be deleted.

## Watch project changes

Use a checkpoint to close the gap between reading a snapshot and starting a
watch. The watcher yields typed invalidations; fetch an entity when you need
its current contents. A `deleted` flag means the entity disappeared from the
daemon's observed database, while moving a card to Trash is an ordinary change.

```python
import asyncio

from holder import EntityChange, Holder, ResyncRequired


async def watch_project(project):
    while True:
        checkpoint = project.changes.checkpoint()
        cards = list(project.cards.all())  # initial or refreshed snapshot
        print(f"Watching {len(cards)} cards")
        async with project.changes.watch(after=checkpoint) as watcher:
            async for notice in watcher:
                if isinstance(notice, ResyncRequired):
                    break  # take a new checkpoint and snapshot
                if isinstance(notice, EntityChange) and notice.entity == "card":
                    project.cards.refresh()
                    print(notice.id, notice.deleted)


with Holder() as holder:
    project = holder.projects.filter(name="Research")[0]
    asyncio.run(watch_project(project))
```

`holder.changes` watches all projects; `project.changes` filters to one.
`watch()` starts at the current tail when `after` is omitted. A `ChangeReady`
notice confirms the connection and includes a current checkpoint. A
`ResyncRequired` notice includes a checkpoint and ends that watcher; refresh
state before creating another. The cursor is process local and replay is
bounded. `watcher.cursor` advances after the consumer requests the next notice,
so replay may repeat the last delivered change if processing stopped midway.
The checkpoint carried by `ChangeReady` may be newer than its frame cursor;
use `watcher.cursor` when saving processing progress.
Applications should handle repeated invalidations. The watcher owns and closes
its async connection. Entity reads through `Holder` are synchronous and can
block an event loop; use `asyncio.to_thread` for substantial reads in an async
application. `project.activity` remains the saved project history, independent
of these live notifications.

## Sync a project and check its remote

`project.git` runs live Git operations. `project.sync` is the status snapshot
loaded with the project; use `project.git.status()` for current persisted state.

```python
probe = project.git.test_remote()  # or test_remote(url="https://example.org/repo.git")
if probe.reachable:
    result = project.git.sync()     # pull and reconcile, then push
    print(result.succeeded, result.pull.status, result.push.status)

print(project.git.status().last_sync_error)
```

Use `project.git.set_remote(url)` to configure a remote or
`project.git.set_remote(None)` to disconnect it. It returns whether the URL
changed and refreshes the project. `test_remote(url=...)` only probes the given
URL. `pull()` uses the checked-out branch; `push()` and `sync()` accept optional
`branch` and `set_upstream` arguments. Git failures arrive as structured
results even when the HTTP request succeeds: inspect `result.succeeded`,
`result.status`, and the phase details. A failed pull skips the push phase.

## Check encryption and recover a project

```python
check = project.check_encryption()
if not check.ok:
    print(check.unsafe_paths)

token = project.export_recovery_token(pin="your-pin")
# Store the token and PIN privately. To restore this project's key later:
project.import_recovery_token(token, pin="your-pin")

# On a device where the project may not yet be registered:
recovery = holder.projects.recover(token, pin="your-pin")
print(recovery.project.name, recovery.project_created)
print(recovery.remote_configured, recovery.pull_status, recovery.pull_error)
```

`recover()` can create a project and then fail to configure its suggested
remote or pull from it. Inspect the returned remote and pull fields. The
serialized `.hrk` token and PIN are not cached on returned objects. For an
encrypted project, both `push()` and `sync()` check project blobs before
pushing; `check_encryption()` provides the detailed list of unsafe paths.

`project.remove_from_device()` unregisters the project locally. Holder keeps
its files and encryption keys for explicit recovery; it does not purge them.

## Browse activity and recover a saved card version

Project activity shows saved changes across the project. Card history shows
changes to one card; nearby edits can be grouped into one `CardChange`, with
each exact save available through `change.saves`. Both collections load pages as
you iterate and cache results until `.refresh()`.

```python
for activity in project.activity.filter(kind="card"):
    print(activity.when, activity.message)
    for affected in activity.affected_objects:
        for item in affected.items:
            print(item.title or item.path, item.detail)

change = card.history[0]             # most recent displayed change
print(change.summary, change.when)
print(change.version().content)       # state at its final save
print(change.saves[0].version().content)  # state at an exact save
print(change.diff().text)             # change across the whole displayed entry
```

`card.version(revision)` reads an immutable historical `CardVersion` with
`exists`, `title`, `content`, and its canonical full `revision`. When `exists`
is false, the card was absent at that revision. A trashed card can still exist
in a historical version. Use `card.compare(before, after)` to compare any two
saved revisions; its result also has structured `lines` and `truncated`.
Revision arguments can be full or unique abbreviated commit IDs, `CardChange`,
`CardSave`, or `CardVersion` objects. A comparison uses the card's state at
each revision, even if that commit changed something else in the project.

```python
older = card.history[1]
newer = card.history[0]
print(card.compare(older, newer).summary)
result = card.restore_version(older)
print(result.restored_from_revision, result.result_revision)
```

Restoring a version writes a new commit; it does not remove later history.
It restores the complete historical card state, including links, milestones,
placement, and Trash state. `card.restore()` still means restore from Trash.
The `Card` object refreshes after restoring a live version; refresh other
previously loaded listings yourself. If the selected version is trashed, use
`project.cards.history(card_id)` to continue browsing without loading a live
card. This ID-based entry point also works when the current card is unavailable.
History requires a usable project Git repository and, for encrypted projects,
an available project key. Individual history responses have a 2 MiB limit.

## Work with intelligence

Use `project.intelligence` for project threads, messages, search, and runs.
Creating a thread or adding a message saves it immediately. `thread.ask()`
starts a run when you enter the stream context; the run saves its own messages.

```python
with Holder() as holder:
    project = holder.projects.get("project-id")
    thread = project.intelligence.threads.create(title="Planning")
    with thread.ask("Suggest next steps") as stream:
        print("Run:", stream.run_id)
        for event in stream:
            if event.kind == "chunk":
                print(event.text, end="", flush=True)
            elif event.kind == "resync_required":
                print("Refresh the run and start a new event stream")

    for result in project.intelligence.search("next steps"):
        print(result.snippet, result.message.content)
```

`project.intelligence.run(prompt)` starts without a thread. `thread.runs` and
`project.intelligence.runs` list saved runs. Call `run.events(after=cursor)` to
replay from an event ID; `stream.cursor` holds the last yielded event's
ID. A `resync_required` event means the cursor is too old: refresh the run and
start a fresh event stream. If a connection ends before `done` or `failed`,
reconnect with the cursor. Closing a stream does not cancel its run.

`thread.messages.add(content)` saves a manual message. Messages support
`update(content=...)`, `trash()`, `restore()`, outgoing `links`, and read-only
`backlinks`. Use `thread.messages.including_trashed()` to see trashed messages.
`project.intelligence.capture(prompt, response, thread=thread)` records an
exchange from outside Holder with provenance; omit `thread` to create one.
IDs from other projects raise `NotFoundError` in project-scoped lookups.

Use `holder.intelligence` for settings and local runtime state shared by all
projects on this device:

```python
with Holder() as holder:
    provider = holder.intelligence.providers.get("provider-id")
    provider.set_key("api-key")
    provider.set_enabled(True)

    models = holder.intelligence.models
    print(models.installed())
    models.configure(fast="model-name")
    job = models.pull("model-name")
    with job.events() as events:
        for event in events:
            print(event.kind, event.data)
```

Provider keys are sent to the daemon and are never stored on the Python
object. `provider.clear_key()` removes one; `provider.reset_enabled()` removes
its enabled override. `models.configure()` preserves omitted preferences and
clears a preference passed as `None`. `holder.intelligence.runners` lists and
manages local runners; `runners.create(name=..., base_url=...)` currently creates
an Ollama runner. Pass `runner_id` to `models.pull()` to choose one. Pull events
are replaceable progress snapshots; after a disconnect, call `job.refresh()`
and open a new stream without an old event ID. `holder.intelligence.status()`
reports active run and pull counts and, in `runners`, each runner's
availability.

`models.installed()` lists the models on every runner; each names its
`runner_id` and the `model_ref` to request it by. `models.recommended()` lists
catalog models this machine can run, best first, and `models.to_install()` the
ones a runner lacks. Both use the auto-local runner unless given `runner_id`.

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
