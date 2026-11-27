# From Holder Kit to `holder`

Use `holder-python` (`import holder`) for private embedded workspaces and for
scripts that connect to Holder. A compatible native wheel includes the embedded
backend. See [supported wheel targets](publishing.md) for platform details.

## Open data and choose a project

| In `holderkit` | In `holder` |
| --- | --- |
| `holderkit.open(data_dir)` | `with open_workspace(data_dir) as session` |
| `holderkit.create(name, workspace=path)` | `with create_managed_workspace(name, workspace=path) as work` |
| `holderkit.clone(url, workspace=path, ref=ref)` | `with clone_workspace(url, workspace=path, ref=ref) as work` |
| `holderkit.reopen(path)` | `with reopen_workspace(path) as work` |
| `context.projects.list()` | `session.list_projects()` |

A managed workspace provides `work.session` and `work.project_id`. Use
`work.project` for the project's current record and `work.revision` for its
current Git commit. End the context to close the session and keep the local
workspace. The managed workspace supports [previewing and publishing a commit](../README.md#publish-committed-work)
and [previewing and discarding a workspace](../README.md#discard-a-managed-workspace).

For data in your running Holder installation, use `Holder()` as the session.
[`open_session(fallback_workspace=path)`](../README.md#run-with-or-without-a-local-holder-installation)
can select the daemon or a private embedded workspace at startup.

## Work with cards

The shared Card operations are on `session.card_operations`. Supply the
project ID and, for a single card, the card ID. Reads return dictionaries you
can keep after the session closes. Fetch the card again for its current state.

| Workflow | Shared call |
| --- | --- |
| Read the metadata or body | `cards.get_card_metadata(project_id, card_id)` or `cards.get_card(project_id, card_id)` |
| Create or update a card | `cards.create_card(project_id, fields)` or `cards.update_card(project_id, card_id, changes)` |
| Find children or root cards | `cards.list_cards(project_id, parent=card_id)` or `cards.list_cards(project_id, parent="roots")` |
| Move a card | `cards.move_card(project_id, card_id, move)` |
| Read all tags or tags you can remove | `cards.get_card(project_id, card_id)["tags"]` or `["removable_tags"]` on the same record |
| Read connections | `cards.list_card_links(project_id, card_id)` and `cards.list_card_backlinks(project_id, card_id)` |
| Read milestone dates | `cards.list_card_milestones(project_id, card_id)` or `session.get_calendar(project_id, first_day, last_day)` |
| Trash or restore a card | `cards.trash_card(project_id, card_id)` or `cards.restore_card(project_id, card_id)` |
| List or permanently delete trashed cards | `session.list_trash(project_id, type="card")` or `session.delete_trash_item(project_id, card_id, type="card")` |

See [Shared Card operations](../README.md#shared-card-operations) for write
arguments and more examples.

## Export and analyse records

| In `holderkit` | In `holder` |
| --- | --- |
| `project.cards.to_records()` | `list(iter_card_records(session.card_operations, project_id))` |
| `project.cards(batch_size=256, tag="research")` | `iter_card_batches(session.card_operations, project_id, batch_size=256, tag="research")` |
| Project records for links, tag memberships, and milestones | `session.export_project_records(project_id)` |
| `project.to_dataframes()` or `project.to_networkx()` | Pass a project export to `to_dataframes(records)` or `to_networkx(records)` |
| `context.to_dataframes()` or `context.to_networkx()` | Pass `iter_project_records(session)` to either adapter |

Use `include_content=True` when you need bodies. `iter_project_records()`
can take `project_ids=[first_id, second_id]` for a selected set. The pandas
adapter returns `links` and `tag_memberships` tables alongside `projects`,
`cards`, and `milestones`. Graph node keys are `(project_id, card_id)` pairs.
Install `holder-python[pandas,graph]` to use both adapters.

```python
from holder import create_managed_workspace, to_dataframes

with create_managed_workspace("Research", workspace="research-workspace") as work:
    work.session.card_operations.create_card(
        work.project_id, {"title": "Observation", "content": "A note. #research"}
    )
    records = work.session.export_project_records(work.project_id, include_content=True)

tables = to_dataframes(records)
print(tables["cards"][["title", "content"]])
```
