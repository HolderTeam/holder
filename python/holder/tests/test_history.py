"""Project activity and card history through the generated transport."""

import httpx
import pytest
from test_library import card_payload, holder_for, ok, project_payload

from holder import CardChange, CardVersion, ProjectActivity
from holder.exceptions import ClosedError, ProtocolError

OLD = "a" * 40
MID = "b" * 40
NEW = "c" * 40
RESTORED = "d" * 40


def version(oid, body, *, exists=True):
    return {"exists": exists, "oid": oid, "title": "History card", "body": body}


def comparison(before, after):
    return {
        "from": before,
        "to": after,
        "summary": "Body changed",
        "lines": [
            {"origin": "-", "text": "old", "old_line": 1, "new_line": None},
            {"origin": "+", "text": "new", "old_line": None, "new_line": 1},
        ],
        "truncated": False,
    }


def entry():
    return {
        "first_oid": MID,
        "last_oid": NEW,
        "parent_oids": [OLD],
        "visible_parent_oids": [],
        "author": {"name": "Ada", "email": "ada@example.test"},
        "started_at": 100,
        "ended_at": 110,
        "kind": "updated",
        "summary": "Updated body",
        "commit_count": 2,
        "is_merge": False,
        "saves": [
            {
                "oid": MID,
                "parent_oids": [OLD],
                "authored_at": 100,
                "committed_at": 100,
                "message": "First save",
            },
            {
                "oid": NEW,
                "parent_oids": [MID],
                "authored_at": 110,
                "committed_at": 110,
                "message": "Second save",
            },
        ],
    }


def test_project_activity_filter_details_pagination_and_cache():
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        requests.append(request)
        cursor = request.url.params.get("cursor")
        return ok(
            {
                "head_oid": NEW,
                "activities": [
                    {
                        "oid": NEW if cursor is None else OLD,
                        "parent_oids": [OLD],
                        "author": {"name": "Ada", "email": "ada@example.test"},
                        "authored_at": 90,
                        "committed_at": 100,
                        "message": "Attach notes",
                        "affected_objects": [
                            {
                                "kind": "card",
                                "paths": ["cards/example.md"],
                                "items": [
                                    {
                                        "path": "cards/example.md",
                                        "title": "History card",
                                        "detail": "Milestone: Review",
                                    }
                                ],
                            }
                        ],
                        "is_merge": False,
                    }
                ],
                "next_cursor": NEW if cursor is None else None,
                "scan_limited": False,
            }
        )

    with holder_for(respond) as holder:
        activity = holder.projects.get("p-0").activity.filter(kind="card")
        first, second = list(activity)
        assert isinstance(first, ProjectActivity)
        assert first.revision == NEW
        assert first.when.year == 1970
        assert first.affected_objects[0].items[0].detail == "Milestone: Review"
        assert second.revision == OLD
        assert len(list(activity)) == 2
        assert len(requests) == 2
        assert requests[0].url.params["kind"] == "card"
        assert requests[1].url.params["cursor"] == NEW
        assert len(list(activity.refresh())) == 2
        assert len(requests) == 4
        with pytest.raises(ValueError):
            activity.filter(kind="invalid")
    with pytest.raises(ClosedError):
        list(activity.refresh())


def test_card_history_grouped_change_exact_saves_and_comparison():
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        requests.append(request)
        if request.url.path.endswith("/snapshot"):
            return ok(
                {
                    "card_id": "c-0000",
                    "snapshot": version(request.url.params["oid"], "saved"),
                }
            )
        if request.url.path.endswith("/compare"):
            return ok(comparison(version(OLD, "old"), version(NEW, "new")))
        return ok(
            {
                "head_oid": NEW,
                "entries": [entry()],
                "next_cursor": None,
                "scan_limited": False,
            }
        )

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        history = project.cards.history("c-0000")  # No live card fetch required.
        change = history[0]
        assert isinstance(change, CardChange)
        assert change.commit_count == 2
        assert [save.revision for save in change.saves] == [MID, NEW]
        saved = change.saves[0].version()
        assert isinstance(saved, CardVersion)
        assert saved.content == "saved"
        assert saved.revision == MID
        assert change.version().revision == NEW
        diff = change.diff()
        assert diff.before.revision == OLD
        assert diff.after.revision == NEW
        assert diff.text == "-old\n+new"
        assert requests[-1].url.params["from"] == OLD
        assert requests[-1].url.params["to"] == NEW
        assert requests[-1].url.params["mode"] == "since"
        assert history.compare(change.saves[0], change).after.revision == NEW


def test_restore_version_writes_once_and_keeps_trash_restore_distinct():
    requests = []
    stored = card_payload(content=True)

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/cards/c-0000":
            return ok(stored)
        requests.append(request)
        if request.method == "POST":
            stored.update(title="Old title", content="old body")
            return ok(
                {
                    "card_id": "c-0000",
                    "restored_from_oid": OLD,
                    "result_oid": RESTORED,
                    "title": "Old title",
                    "updated_at": 120,
                    "deleted_at": None,
                }
            )
        raise AssertionError("Unexpected request")

    with holder_for(respond) as holder:
        card = holder.projects.get("p-0").cards.get("c-0000")
        result = card.restore_version(OLD)
        assert result.result_revision == RESTORED
        assert result.restored_from_revision == OLD
        assert card.title == "Old title"
        assert card.content == "old body"
        assert len(requests) == 1
        assert requests[0].url.path.endswith("/history/cards/c-0000/restore")
        assert requests[0].url.params["oid"] == OLD


def test_history_restore_failure_is_not_retried():
    writes = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        writes.append(request)
        raise httpx.ReadError("Interrupted", request=request)

    with holder_for(respond) as holder:
        history = holder.projects.get("p-0").cards.history("c-0000")
        from holder.exceptions import ConnectionError

        with pytest.raises(ConnectionError):
            history.restore_version(OLD)
        assert len(writes) == 1


def test_history_detects_repeated_cursor():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        return ok(
            {
                "head_oid": NEW,
                "entries": [entry()],
                "next_cursor": NEW,
                "scan_limited": False,
            }
        )

    with holder_for(respond) as holder, pytest.raises(ProtocolError):
        list(holder.projects.get("p-0").cards.history("c-0000"))


def test_scan_limited_empty_page_continues_to_next_cursor():
    cursors = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        cursor = request.url.params.get("cursor")
        cursors.append(cursor)
        if cursor is None:
            return ok(
                {
                    "head_oid": NEW,
                    "entries": [],
                    "next_cursor": MID,
                    "scan_limited": True,
                }
            )
        return ok(
            {
                "head_oid": NEW,
                "entries": [entry()],
                "next_cursor": None,
                "scan_limited": False,
            }
        )

    with holder_for(respond) as holder:
        history = holder.projects.get("p-0").cards.history("c-0000")
        assert [change.revision for change in history] == [NEW]
        assert cursors == [None, MID]
