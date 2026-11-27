"""Opt-in integration against an isolated, disposable holderd process.

Set HOLDER_TEST_DAEMON to a built holderd executable. This never uses the user's
normal daemon, database, or projects.
"""

import hashlib
import json
import os
import subprocess
import sys
import tempfile
import time
from datetime import UTC, date, datetime, timedelta, timezone
from importlib import import_module
from importlib.resources import files
from importlib.util import find_spec
from pathlib import Path
from zipfile import ZipFile

import httpx
import pytest

from holder import (
    Holder,
    LinkKindLabels,
    MilestoneDate,
    ProjectRecord,
    open_session,
    open_workspace,
    to_dataframes,
    to_networkx,
)
from holder.cards import Card, CardListItem, CardMetadata, Link, Milestone, SearchCard
from holder.exceptions import APIError, ClosedError, ConnectionError, NotFoundError

DAEMON = os.environ.get("HOLDER_TEST_DAEMON")


@pytest.fixture
def daemon(tmp_path, monkeypatch):
    if not DAEMON:
        pytest.skip("Set HOLDER_TEST_DAEMON for live integration")
    binary = Path(DAEMON).resolve()
    for variable, directory in (
        ("XDG_DATA_HOME", "data"),
        ("XDG_CONFIG_HOME", "config"),
        ("XDG_CACHE_HOME", "cache"),
        ("HOLDER_PROJECTS_ROOT", "projects"),
        ("HOLDER_TEST_KEYSTORE_DIR", "keystore"),
    ):
        monkeypatch.setenv(variable, str(tmp_path / directory))
    # An explicit cwd supplies schema/catalog resources in source-development builds.
    cwd = os.environ.get("HOLDER_TEST_DAEMON_CWD", str(binary.parent.parent))
    with (tmp_path / "daemon.log").open("w") as log:
        process = subprocess.Popen(
            [str(binary), "--bind", "127.0.0.1", "--port", "0"],
            cwd=cwd,
            stdout=log,
            stderr=log,
        )
        try:
            info_path = tmp_path / "data/holder/server/holder.json"
            deadline = time.monotonic() + 15
            while True:
                if process.poll() is not None:
                    pytest.fail("Disposable daemon exited during startup")
                try:
                    info = json.loads(info_path.read_text())
                    url = f"http://127.0.0.1:{info['port']}"
                    token = info["auth_token"]
                    with httpx.Client(
                        base_url=url,
                        headers={"Authorization": f"Bearer {token}"},
                        timeout=2,
                    ) as probe:
                        if probe.get("/health").status_code == 200:
                            break
                except (OSError, ValueError, KeyError, httpx.TransportError):
                    pass
                if time.monotonic() >= deadline:
                    pytest.fail("Disposable daemon did not become healthy")
                time.sleep(0.05)
            with httpx.Client(
                base_url=url, headers={"Authorization": f"Bearer {token}"}, timeout=10
            ) as raw:
                yield url, token, raw, binary.parent / "holderctl"
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)


def create_project(raw, name):
    response = raw.post("/projects", json={"name": name, "privacy_mode": "plain"})
    assert response.status_code == 201
    return response.json()["data"]["project_id"]


def create_card(raw, project_id, index, parent=None):
    response = raw.post(
        f"/projects/{project_id}/cards",
        json={
            "title": f"Card {index}",
            "content": f"body {index}\n#research",
            "parent_card_id": parent,
        },
    )
    assert response.status_code == 201
    return response.json()["data"]["card_id"]


def test_create_read_edit_workflow_against_daemon(daemon):
    url, token, _, _ = daemon
    with Holder(url=url, token=token) as h:
        project = h.projects.create(name="Python workflow", privacy_mode="plain")
        assert project.id
        assert project.root_path
        assert project.created_at > 0
        assert h.projects.get(project.id).name == "Python workflow"
        assert list(project.cards) == []
        card = project.cards.create(title="Research", content="Notes\n#research")
        child = card.children.create(title="Follow-up")
        sibling = project.cards.roots().create(title="Another root")
        assert card.parent_card_id is None
        assert child.parent_card_id == card.id
        assert child.content == ""
        assert sibling.sort_key > card.sort_key
        assert list(project.cards) == []
        assert {c.id for c in project.cards.refresh()} == {
            card.id,
            child.id,
            sibling.id,
        }
        assert {c.id for c in project.cards.roots()} == {card.id, sibling.id}
        assert [c.id for c in card.children] == [child.id]
        assert [c.id for c in project.cards.filter(tag="Research")] == [card.id]
        assert child.update(title="Next step", content="More notes") is child
        assert project.cards.get(child.id).content == "More notes"
        assert child.title == "Next step"
        assert project.update(name="Renamed Python workflow") is project
        assert h.projects.get(project.id).name == "Renamed Python workflow"
        project.refresh()
        assert len(list(project.cards.all())) == 3
        assert len(list(project.cards.roots())) == 2


def test_tag_collections_and_body_mentions_against_daemon(daemon):
    url, token, _, _ = daemon
    with Holder(url=url, token=token) as h:
        project = h.projects.create(name="Python tags", privacy_mode="plain")
        other = h.projects.create(name="Other tags", privacy_mode="plain")
        card = project.cards.create(
            title="Tagged card", content="An inline #body mention.\n\n#draft"
        )
        sibling = project.cards.create(title="Related", content="Related notes")
        foreign = other.cards.create(title="Foreign", content="Foreign notes")
        assert {t.name for t in card.tags} == {"body", "draft"}
        assert {t.name for t in project.tags} == {"body", "draft"}
        cached_project_tags = project.tags
        card.tags.add("Research")
        sibling.tags.add("RESEARCH")
        foreign.tags.add("research")
        assert {t.name for t in card.tags} == {"body", "draft", "research"}
        assert "#research" in card.content
        assert {t.name for t in cached_project_tags} == {"body", "draft"}
        tag = project.tags.get("research")
        assert tag.card_count == 2
        assert tag.project_id == project.id
        assert {c.id for c in tag.cards} == {card.id, sibling.id}
        assert project.tags.refresh()[0].name == "research"
        assert {c.id for c in card.tags.get("research").cards} == {card.id, sibling.id}
        before = card.content
        card.tags.add("research")
        assert card.content == before
        card.tags.remove("DRAFT")
        assert {t.name for t in card.tags} == {"body", "research"}
        card.tags.remove("missing")
        before = card.content
        # Prose is never rewritten, so a tag that would stay in it is a conflict.
        with pytest.raises(APIError) as prose:
            card.tags.remove("body")
        assert prose.value.status == 409
        assert card.refresh().content == before
        assert {t.name for t in card.tags} == {"body", "research"}
        card.tags.remove("research")
        assert [t.name for t in card.tags] == ["body"]
        assert tag.refresh().card_count == 1
        assert {c.id for c in tag.cards.refresh()} == {sibling.id}
        card.update(content="The user removed the inline mention.")
        assert list(card.tags) == []


def test_qualified_card_links_and_available_backlinks_against_daemon(daemon):
    url, token, _, _ = daemon
    with Holder(url=url, token=token) as h:
        first = h.projects.create(name="Link source", privacy_mode="plain")
        second = h.projects.create(name="Link peer", privacy_mode="plain")
        source = first.cards.create(title="Source")
        target = first.cards.create(title="Target")
        peer = second.cards.create(title="Peer")

        local = source.links.add(target.id, label="Local")
        same_uuid_other_project = source.links.add(
            target.id, target_project_id=second.id, label="External fork"
        )
        unknown = source.links.add(
            "not-present", target_project_id="unregistered-project", label="Unresolved"
        )
        assert local.target_project_id == first.id
        assert same_uuid_other_project.target_project_id == second.id
        assert unknown.target_project_id == "unregistered-project"
        assert {row.target_project_id for row in source.links} == {
            first.id,
            second.id,
            "unregistered-project",
        }
        with pytest.raises(NotFoundError):
            h.projects.get("unregistered-project")

        peer.links.add(target.id, target_project_id=first.id, label="Backlink")
        assert {row.source_project_id for row in target.backlinks} == {first.id}
        external = target.backlinks.filter(scope="external")
        assert {row.source_project_id for row in external} == {second.id}
        assert all(row.source_type == "card" for row in external)
        both = target.backlinks.filter(scope="all_available")
        assert {row.source_project_id for row in both} == {first.id, second.id}

        # The same UUID in two qualified target identities must delete separately.
        source.links.remove(target.id, target_project_id=second.id)
        assert any(row.target_project_id == first.id for row in source.links)
        assert not any(row.target_project_id == second.id for row in source.links)
        assert len(list(target.backlinks)) == 1  # Cached until explicit refresh.

        source.links.add(target.id, target_project_id=second.id)

        root = Path(second.root_path)
        unavailable_root = root.with_name(root.name + "-unavailable")
        root.rename(unavailable_root)
        try:
            assert list(external.refresh()) == []
            assert any(
                row.target_project_id == "unregistered-project"
                for row in source.links.refresh()
            )
            assert any(row.target_project_id == second.id for row in source.links)
        finally:
            unavailable_root.rename(root)


def test_full_card_milestones_against_daemon(daemon):
    url, token, raw, _ = daemon
    with Holder(url=url, token=token) as h:
        project = h.projects.create(name="Python milestones", privacy_mode="plain")
        card = project.cards.create(title="Dates")
        milestones = card.milestones
        assert list(milestones) == []
        undated = milestones.create(kind="task", description="Track this")
        assert undated.dates == ()
        assert {item.id for item in milestones} == {undated.id}

        occurred = datetime(
            2026, 6, 2, 0, 30, tzinfo=timezone(timedelta(hours=2, minutes=30))
        )
        dated = milestones.create(
            dates=[
                MilestoneDate("deadline", date(2026, 6, 2)),
                MilestoneDate("deadline", date(2026, 6, 3)),
                MilestoneDate("occurred", occurred),
            ],
            description="A dated event",
        )
        assert dated.dates[0].value == date(2026, 6, 2)
        assert dated.dates[1].value == date(2026, 6, 3)
        assert dated.dates[2].value == occurred
        assert {item.id for item in milestones} == {undated.id, dated.id}

        # The calendar day stays June 2; the offset instant is June 1 in UTC.
        days = project.milestones.between(
            date(2026, 6, 2), date(2026, 6, 2), role="deadline"
        )
        assert [item.id for item in days] == [dated.id]
        utc = datetime(2026, 6, 1, 22, 0, tzinfo=UTC)
        instants = project.milestones.between(utc, utc)
        assert [item.id for item in instants] == [dated.id]
        def on_calendar(day: str, zone: str) -> list[str]:
            calendar = raw.get(
                f"/projects/{project.id}/calendar",
                params={"first_day": day, "last_day": day, "time_zone": zone},
            )
            assert calendar.status_code == 200
            return [
                item["milestone_id"] for item in calendar.json()["data"]["milestones"]
            ]

        # A day is the same wherever the caller is.
        assert on_calendar("2026-06-03", "Asia/Tokyo") == [dated.id]
        # The instant, 22:00 UTC on June 1, is June 1 in UTC but already June 2 in Tokyo.
        assert on_calendar("2026-06-01", "UTC") == [dated.id]
        assert on_calendar("2026-06-01", "Asia/Tokyo") == []

        assert dated.update(description=None) is dated
        assert dated.description is None
        assert dated.dates[2].value == occurred
        assert dated.update(dates=[]) is dated
        assert dated.dates == ()
        assert milestones.get(dated.id).dates == ()
        assert undated.remove() is None
        assert {item.id for item in milestones} == {dated.id}


def test_card_moves_against_daemon(daemon):
    url, token, raw, _ = daemon
    with Holder(url=url, token=token) as h:
        project = h.projects.create(name="Python moves", privacy_mode="plain")
        other = h.projects.create(name="Other moves", privacy_mode="plain")
        parent = project.cards.create(title="Parent")
        target = parent.children.create(title="Target")
        root = project.cards.create(title="Root")
        card = project.cards.create(title="Moving", content="Keep this body\n#research")
        child = card.children.create(title="Child")
        foreign = other.cards.create(title="Foreign")
        assert [c.id for c in parent.children] == [target.id]
        assert card.move(parent=parent) is card
        assert card.parent_card_id == parent.id
        assert card.sort_key > target.sort_key
        assert card.content == "Keep this body\n#research"
        assert child.refresh().parent_card_id == card.id
        # Evaluated listings retain their cache until explicitly refreshed.
        assert [c.id for c in parent.children] == [target.id]
        assert {c.id for c in parent.children.refresh()} == {target.id, card.id}
        card.move(before=target)
        assert card.parent_card_id == parent.id
        assert card.sort_key < target.sort_key
        card.move(after=target)
        assert card.sort_key > target.sort_key
        # Relative moves can change hierarchy, including to an existing root's level.
        card.move(before=root)
        assert card.parent_card_id is None
        assert card.sort_key < root.sort_key
        assert child.refresh().parent_card_id == card.id
        assert [c.id for c in parent.children.refresh()] == [target.id]
        # parent=None detaches a nested card, with its subtree, to the end of the top level.
        nested = target.children.create(title="Nested", content="Nested body\n#keep")
        grandchild = nested.children.create(title="Grandchild")
        assert nested.move(parent=None) is nested
        assert nested.parent_card_id is None
        assert nested.sort_key > root.sort_key
        assert nested.content == "Nested body\n#keep"
        assert [t.name for t in nested.tags] == ["keep"]
        assert grandchild.refresh().parent_card_id == nested.id
        assert list(target.children.refresh()) == []
        # An existing root moves to the end of the top level.
        root.move(parent=None)
        assert root.parent_card_id is None
        assert root.sort_key > nested.sort_key
        with pytest.raises(APIError) as cycle:
            card.move(parent=child)
        assert cycle.value.status == 422
        for kwargs in ({"parent": foreign}, {"before": foreign}, {"after": foreign}):
            with pytest.raises(NotFoundError):
                card.move(**kwargs)
        assert card.refresh().parent_card_id is None
        assert (
            raw.delete(f"/projects/{target.project_id}/cards/{target.id}").status_code
            == 200
        )
        with pytest.raises(NotFoundError):
            card.move(after=target)
        assert (
            raw.delete(f"/projects/{card.project_id}/cards/{card.id}").status_code
            == 200
        )
        with pytest.raises(NotFoundError):
            card.move(parent=parent)


def test_card_lifecycle_against_daemon(daemon):
    url, token, _, _ = daemon
    with Holder(url=url, token=token) as h:
        project = h.projects.create(name="Python lifecycle", privacy_mode="plain")
        parent = project.cards.create(title="Parent")
        child = parent.children.create(title="Child", content="Child body\n#keep")
        # Promotion: the child takes its trashed parent's place.
        assert parent.trash() is None
        assert child.refresh().parent_card_id is None
        assert child.content == "Child body\n#keep"
        with pytest.raises(NotFoundError):
            parent.refresh()
        # Restore is single-card: the promoted child stays where it is.
        assert parent.restore() is parent
        assert parent.parent_card_id is None
        assert child.refresh().parent_card_id is None

        top = project.cards.create(title="Top")
        middle = top.children.create(title="Middle")
        bottom = middle.children.create(title="Bottom")
        top.trash(children="trash_subtree")
        for card in (top, middle, bottom):
            with pytest.raises(NotFoundError):
                card.refresh()
        middle.restore()
        assert middle.parent_card_id is None
        with pytest.raises(NotFoundError):
            bottom.refresh()

        with pytest.raises(ValueError):
            child.trash(children="everything")
        # A healthy project has nothing to repair.
        repair = project.repair_hierarchy(apply=True)
        assert repair.applied is False
        assert repair.moved == []
        assert repair.cycle_card_ids == []


def test_card_search_against_daemon(daemon):
    url, token, raw, _ = daemon
    with Holder(url=url, token=token) as h:
        project = h.projects.create(name="Python search", privacy_mode="plain")
        other = h.projects.create(name="Other search", privacy_mode="plain")
        cards = [
            project.cards.create(
                title=f"Search card {i}",
                content=f"research notes number {i}" + (" café" if i == 0 else ""),
            )
            for i in range(101)
        ]
        child = cards[0].children.create(
            title="Child", content="research notes for a child"
        )
        other.cards.create(
            title="Foreign", content="research notes from another project"
        )
        project.cards.create(title="Unrelated", content="Nothing matching here")
        assert (
            raw.delete(f"/projects/{project.id}/cards/{cards[1].id}").status_code == 200
        )
        results = project.cards.search('"research notes"')
        rows = list(results)
        expected = {c.id for c in cards} - {cards[1].id}
        expected.add(child.id)
        assert len(rows) == 101
        assert {r.card_id for r in rows} == expected
        assert project.cards.search("café")[0].card_id == cards[0].id
        assert [r.rank for r in rows] == sorted(r.rank for r in rows)
        assert all("research" in r.snippet for r in rows)
        first = rows[0]
        assert first.card.project_id == project.id
        assert "research notes" in first.card.content
        assert list(project.cards.search("quartzunfindable")) == []
        with pytest.raises(APIError) as malformed:
            list(project.cards.search('"'))
        assert malformed.value.status == 400
        first.card.update(content="Changed to unrelated content")
        assert (
            raw.delete(f"/projects/{project.id}/cards/{rows[1].card_id}").status_code
            == 200
        )
        with pytest.raises(NotFoundError):
            _ = rows[1].card
        assert len(list(results)) == 101  # Already evaluated search stays cached.
        assert {r.card_id for r in results.refresh()} == expected - {
            first.card_id,
            rows[1].card_id,
        }


def test_daemon_card_pages(daemon):
    _, _, raw, _ = daemon
    project = create_project(raw, "Pages")
    ids = sorted(create_card(raw, project, i) for i in range(3))
    response = raw.get(f"/projects/{project}/cards", params={"limit": 2})
    assert response.status_code == 200
    page = response.json()["data"]
    assert [card["card_id"] for card in page["items"]] == ids[:2]
    response = raw.get(
        f"/projects/{project}/cards", params={"limit": 2, "cursor": page["next_cursor"]}
    )
    assert [card["card_id"] for card in response.json()["data"]["items"]] == ids[2:]
    assert response.json()["data"]["next_cursor"] is None


def test_project_scoped_collections_against_daemon(daemon):
    url, token, raw, cli = daemon
    project_id = create_project(raw, "Collection integration")
    other_project = create_project(raw, "Other")
    ids = [create_card(raw, project_id, i) for i in range(205)]
    child = create_card(raw, project_id, 205, ids[0])
    foreign = create_card(raw, other_project, 1)
    assert raw.delete(f"/projects/{project_id}/cards/{ids[1]}").status_code == 200
    with Holder(url=url, token=token) as h:
        project = h.projects.filter(name="Collection integration")[0]
        assert project.id == project_id
        cards = list(project.cards)
        assert len(cards) == 205
        assert {card.id for card in cards} == (set(ids) - {ids[1]}) | {child}
        assert len(list(project.cards.roots())) == 204
        assert project.cards.get(ids[0]).children[0].id == child
        assert len(list(project.cards.filter(tag="Research"))) == 205
        assert len(list(project.cards.roots().filter(tag="RESEARCH"))) == 204
        assert project.cards.get(ids[0]).children.filter(tag="ReSeArCh")[0].id == child
        with pytest.raises(NotFoundError):
            project.cards.get(foreign)
        card = project.cards.get(ids[0])
        assert card.content == "body 0\n#research"
        card.update(title="Updated through Python")
        assert card.title == "Updated through Python"
        assert card.content == "body 0\n#research"
    # Exercise the zero-argument discovery path against the same isolated env.
    with Holder(holderctl=str(cli)) as h:
        assert h.projects.get(project_id).name == "Collection integration"


def test_embedded_profile_ownership(daemon, tmp_path):
    if find_spec("holder._embedded._native"):
        package = "holder._embedded"
    elif find_spec("holderkit"):
        package = "holderkit"
    else:
        pytest.skip("Requires a native Holder wheel or a same-checkout Kit build")

    profile = tmp_path / "data" / "holder"
    with pytest.raises(ValueError, match="desktop Holder profile"), open_workspace(profile):
        pass

    # Bypass the Python policy so this also verifies daemon/native lock compatibility.
    native = import_module(f"{package}._native")
    schema = files(package).joinpath("_schema.sql").read_text(encoding="utf-8")
    with pytest.raises(native.HolderError, match="workspace is in use") as busy:
        native.Context(str(profile), schema)
    assert busy.value.native_status == 4090

    private = tmp_path / "private"
    probe = """
from importlib import import_module
from importlib.resources import files
import sys
import tempfile

package, workspace = sys.argv[1:]
native = import_module(f"{package}._native")
schema = files(package).joinpath("_schema.sql").read_text(encoding="utf-8")
try:
    with native.Context(workspace, schema):
        pass
except native.HolderError as error:
    if error.native_status != 4090:
        raise
    sys.exit(42)
"""

    def other_process_status():
        result = subprocess.run(
            [sys.executable, "-c", probe, package, str(private)],
            capture_output=True,
            text=True,
            timeout=15,
            check=False,
        )
        assert result.returncode in (0, 42), result.stderr
        return result.returncode

    with native.Context(str(private), schema):
        with open_workspace(private) as workspace:
            project = workspace.create_plain_project(name="Private project")
            with pytest.raises(APIError, match="workspace is in use"), open_workspace(private):
                pass
            assert workspace.get_project(project["project_id"]) == project
            assert other_process_status() == 42
        assert other_process_status() == 42

    assert other_process_status() == 0
    with open_workspace(private) as workspace:
        assert workspace.get_project(project["project_id"]) == project


def test_daemon_recovery_snapshot_download(daemon):
    url, token, raw, _ = daemon
    project_id = create_project(raw, "Snapshot download")
    route = f"/projects/{project_id}/recovery-snapshot"
    assert raw.get(route, headers={"Authorization": ""}).status_code == 401
    assert raw.get(route).headers["cache-control"] == "no-store"

    with Holder(url=url, token=token) as session:
        assert session.download_recovery_snapshot(project_id) == ""
        with pytest.raises(NotFoundError):
            session.download_recovery_snapshot("missing")
        original = session.card_operations.create_card(
            project_id, {"title": "Original", "content": "Research notes ☃"}
        )
        lines = session.download_recovery_snapshot(project_id).splitlines()
        for index in range(3):
            session.card_operations.create_card(
                project_id, {"title": f"Large card {index}", "content": "x" * (720 * 1024)}
            )
        with pytest.raises(APIError) as too_large:
            session.download_recovery_snapshot(project_id)

    assert len(lines) == 1
    row = json.loads(lines[0])
    assert row["format"] == "holder-recovery-snapshot"
    assert row["card"]["card_id"] == original["card_id"]
    assert row["card"]["content"] == original["content"]
    assert too_large.value.status == 413
    assert too_large.value.code == "snapshot_too_large"


def test_recovery_snapshot_round_trip(daemon, tmp_path):
    if not (find_spec("holder._embedded._native") or find_spec("holderkit")):
        pytest.skip("Requires a native Holder wheel or a same-checkout Kit build")

    url, token, _, _ = daemon
    with Holder(url=url, token=token) as session:
        source_id = session.create_plain_project(name="Snapshot source")["project_id"]
        source = session.card_operations.create_card(
            source_id, {"title": "Original", "content": "Live body"}
        )
        snapshot = session.download_recovery_snapshot(source_id)
        with open_workspace(tmp_path / "private-copy") as workspace:
            copy_id = workspace.restore_recovery_snapshot(snapshot, name="Offline copy")["project_id"]
            (copied,) = workspace.card_operations.list_cards(copy_id)["items"]
            assert copied["card_id"] != source["card_id"]
            changed = workspace.card_operations.update_card(
                copy_id, copied["card_id"], {"content": "Edited offline"}
            )
            assert changed["content"] == "Edited offline"
        assert session.card_operations.get_card(source_id, source["card_id"])["content"] == "Live body"


def test_copy_large_live_project_uses_captured_snapshot(daemon, tmp_path):
    if not (find_spec("holder._embedded._native") or find_spec("holderkit")):
        pytest.skip("Requires a native Holder wheel or a same-checkout Kit build")

    url, token, raw, _ = daemon
    source_id = create_project(raw, "Large snapshot")
    body = "x" * (750 * 1024)
    with (
        Holder(url=url, token=token) as session,
        open_workspace(tmp_path / "private-copy") as workspace,
    ):
        cards = session.card_operations
        root = cards.create_card(source_id, {"title": "Root", "content": body})
        for index in range(2):
            cards.create_card(
                source_id,
                {
                    "title": f"Child {index}",
                    "content": body,
                    "parent_card_id": root["card_id"],
                },
            )

        def edit_after_capture(response):
            if response.request.url.path.endswith("/recovery-snapshot"):
                assert response.request.url.params["stream"] == "true"
                assert int(response.headers["content-length"]) > 2 * 1024 * 1024
                assert response.headers["cache-control"] == "no-store"
                changed = raw.patch(
                    f"/projects/{source_id}/cards/{root['card_id']}",
                    json={"content": "New live body"},
                )
                assert changed.status_code == 200
                create_card(raw, source_id, "added after capture")

        session._active().get_httpx_client().event_hooks["response"].append(
            edit_after_capture
        )
        project = session.copy_project_to(source_id, workspace, name="Offline copy")
        assert project["project_id"] != source_id
        items = workspace.card_operations.list_cards(
            project["project_id"], include_content=True
        )["items"]
        copied = {item["title"]: item for item in items}
        assert set(copied) == {"Root", "Child 0", "Child 1"}
        assert all(item["content"] == body for item in items)
        assert copied["Root"]["card_id"] != root["card_id"]
        assert copied["Child 0"]["parent_card_id"] == copied["Root"]["card_id"]
        assert cards.get_card(source_id, root["card_id"])["content"] == "New live body"


@pytest.mark.parametrize("interrupted", [True, False])
def test_project_copy_cleans_up_failed_transfer_or_restore(
    tmp_path, monkeypatch, interrupted
):
    if not (find_spec("holder._embedded._native") or find_spec("holderkit")):
        pytest.skip("Requires a native Holder wheel or a same-checkout Kit build")

    class BadSnapshot(httpx.SyncByteStream):
        def __iter__(self):
            yield b"x" * (64 * 1024)
            if interrupted:
                raise httpx.ReadError("connection closed")

    def respond(request):
        assert request.url.params["stream"] == "true"
        return httpx.Response(
            200,
            headers={"content-type": "application/octet-stream"},
            stream=BadSnapshot(),
        )

    temporary = tmp_path / "spools"
    temporary.mkdir()
    monkeypatch.setattr(tempfile, "tempdir", str(temporary))
    with (
        Holder(
            url="http://test.invalid",
            token="test",
            transport=httpx.MockTransport(respond),
        ) as session,
        open_workspace(tmp_path / "private-copy") as workspace,
    ):
        with pytest.raises(ConnectionError if interrupted else APIError):
            session.copy_project_to("original", workspace, name="Failed copy")
        assert workspace.list_projects() == []
        assert list(temporary.iterdir()) == []


def test_large_recovery_bundle_keeps_verified_attachments(daemon, tmp_path):
    if not (find_spec("holder._embedded._native") or find_spec("holderkit")):
        pytest.skip("Requires a native Holder wheel or a same-checkout Kit build")

    url, token, raw, _ = daemon
    project_id = create_project(raw, "Large project with attachments")
    payload = b"Notebook attachment\n"
    digest = hashlib.sha256(payload).hexdigest()
    source = tmp_path / "attachment.txt"
    source.write_bytes(payload)
    location = raw.post(
        "/locations",
        json={
            "project_id": project_id,
            "name": "Test assets",
            "provider": "local_directory",
            "configuration": {},
        },
    )
    assert location.status_code == 201, location.text
    location_id = location.json()["data"]["location_id"]
    bound = raw.put(
        f"/locations/{location_id}/binding",
        json={
            "values": {"root_path": str(tmp_path / "source-assets")},
            "preview": str(tmp_path / "source-assets"),
        },
    )
    assert bound.status_code == 200, bound.text

    archive_path = tmp_path / "project.zip"
    body = "x" * (750 * 1024)
    with Holder(url=url, token=token) as session:
        cards = session.card_operations
        for index in range(3):
            card = cards.create_card(
                project_id, {"title": f"Notes {index}", "content": body}
            )
        response = raw.post(
            "/imports",
            json={
                "project_id": project_id,
                "card_id": card["card_id"],
                "location_id": location_id,
                "source_path": str(source),
            },
        )
        assert response.status_code == 202, response.text
        job_id = response.json()["data"]["job_id"]
        deadline = time.monotonic() + 5
        while True:
            status = raw.get(f"/imports/{job_id}")
            status.raise_for_status()
            job = status.json()["data"]
            if job["status"] == "completed":
                break
            assert job["status"] != "failed" and time.monotonic() < deadline, job
            time.sleep(0.02)
        session.download_recovery_bundle(project_id, archive_path)

    with ZipFile(archive_path) as archive:
        assert archive.getinfo("snapshot.jsonl").file_size > 2 * 1024 * 1024
        assert archive.read(f"assets/{digest}") == payload

    private = tmp_path / "private-bundle"
    with open_workspace(private) as workspace:
        copied = workspace.restore_recovery_bundle(
            archive_path, name="Offline attachments"
        )
        copy_id = copied["project_id"]
        assert copy_id != project_id
        records = workspace.export_project_records(copy_id, include_content=True)
        assert len(records["cards"]) == 3
        assert all(item["content"] == body for item in records["cards"])
        (link,) = records["links"]
        assert link["target_type"] == "resource"
        assert link["target_id"] != job["resource_id"]
        assert (private / "resource-store" / copy_id / digest).read_bytes() == payload

        corrupt = tmp_path / "corrupt.zip"
        with ZipFile(archive_path) as original, ZipFile(corrupt, "w") as damaged:
            for entry in ("manifest.json", "snapshot.jsonl"):
                damaged.writestr(entry, original.read(entry))
            damaged.writestr(f"assets/{digest}", b"x" * len(payload))
        with pytest.raises(APIError) as error:
            workspace.restore_recovery_bundle(corrupt, name="Corrupt attachment")
        assert error.value.code == "data_corrupted"
        assert [project["project_id"] for project in workspace.list_projects()] == [
            copy_id
        ]
        assert list((private / "resource-store").iterdir()) == [
            private / "resource-store" / copy_id
        ]


@pytest.mark.parametrize("archive", [False, True], ids=["lines", "archive"])
def test_embedded_recovery_snapshot_restore(tmp_path, archive):
    if not (find_spec("holder._embedded._native") or find_spec("holderkit")):
        pytest.skip("Requires a native Holder wheel or a same-checkout Kit build")

    def line(card_id, title, parent=None):
        return json.dumps(
            {
                "format": "holder-recovery-snapshot",
                "version": 1,
                "card": {
                    "card_id": card_id,
                    "project_id": "source-project",
                    "project_name": "Source project",
                    "privacy_mode": "plain",
                    "title": title,
                    "content": "Snapshot text",
                    "parent_card_id": parent,
                    "sort_key": 1,
                    "created_at": 1700000000,
                    "updated_at": 1700000001,
                    "links": [],
                    "milestones": [],
                    "resources": [],
                    "locations": [],
                },
            }
        )

    snapshot = "\n".join((line("child", "Child", "parent"), line("parent", "Parent")))
    with open_workspace(tmp_path / "private") as workspace:
        with pytest.raises(APIError):
            workspace.restore_recovery_snapshot("invalid snapshot", name="Invalid")
        assert workspace.list_projects() == []

        if archive:
            path = tmp_path / "snapshot.zip"
            with ZipFile(path, "w") as bundle:
                bundle.writestr(
                    "manifest.json",
                    json.dumps({
                        "format": "holder-recovery-bundle",
                        "version": 1,
                        "project_id": "source-project",
                    }),
                )
                bundle.writestr("snapshot.jsonl", snapshot)
            project = workspace.restore_recovery_bundle(path, name="Offline copy")
            with ZipFile(path, "a") as bundle:
                bundle.writestr("../unexpected", "unexpected")
            with pytest.raises(ValueError, match="asset entries"):
                workspace.restore_recovery_bundle(path, name="Invalid archive")
            assert workspace.list_projects() == [project]
        else:
            project = workspace.restore_recovery_snapshot(snapshot, name="Offline copy")
        assert project["project_id"] != "source-project"
        assert project["privacy_mode"] == "plain"
        cards = workspace.card_operations.list_cards(
            project["project_id"], include_content=True
        )["items"]
        by_title = {card["title"]: card for card in cards}
        assert by_title["Parent"]["card_id"] != "parent"
        assert by_title["Child"]["parent_card_id"] == by_title["Parent"]["card_id"]
        assert by_title["Child"]["content"] == "Snapshot text"


@pytest.fixture(params=["daemon", "embedded"])
def shared_card_session(request, tmp_path):
    backend = request.param
    if backend == "daemon":
        url, token, _, cli = request.getfixturevalue("daemon")
        session = (
            open_session(holderctl=str(cli), fallback_workspace=tmp_path / "unused")
            if cli.is_file()
            else open_session(url=url, token=token)
        )
    else:
        if not (find_spec("holder._embedded._native") or find_spec("holderkit")):
            pytest.skip("Requires a native Holder wheel or a same-checkout Kit build")
        data_dir = tmp_path / "workspace"
        session = open_session(
            fallback_workspace=data_dir, holderctl=str(tmp_path / "missing-holderctl")
        )

    return session


def test_shared_card_operations(shared_card_session):
    with shared_card_session as active:
        created_project = active.create_plain_project(name="Shared Card API")
        project_id = created_project["project_id"]
        assert created_project["privacy_mode"] == "plain"

        projects = active.list_projects()
        project = next(row for row in projects if row["name"] == "Shared Card API")
        assert project.keys() == ProjectRecord.__required_keys__
        assert project["project_id"] == project_id
        assert active.get_project(project_id) == project == created_project
        with pytest.raises(NotFoundError):
            active.get_project("missing")

        project_id = project["project_id"]
        cards = active.card_operations
        parent = cards.create_card(project_id, {"title": "Card 1"})
        parent_id = parent["card_id"]
        assert parent["content"] == ""
        assert parent["parent_card_id"] is None
        new_child = cards.create_card(
            project_id,
            {
                "title": "Card 2",
                "content": "body 2\n#research",
                "parent_card_id": parent_id,
                "sort_key": 12.5,
                "created_at": 1_700_000_000,
                "updated_at": 1_700_000_001,
            },
        )
        child_id = new_child["card_id"]
        assert new_child["sort_key"] == 12.5
        assert new_child["created_at"] == 1_700_000_000
        assert new_child["updated_at"] == 1_700_000_001
        with pytest.raises(NotFoundError):
            cards.create_card(project_id, {"title": "Orphan", "parent_card_id": "missing"})

        roots = cards.list_cards(project_id, parent="roots", include_child_count=True)
        assert roots["next_cursor"] is None
        assert [item["card_id"] for item in roots["items"]] == [parent_id]
        assert roots["items"][0]["child_count"] == 1
        assert roots["items"][0].keys() == CardListItem.__required_keys__ | {"child_count"}

        parent_page = cards.list_cards(project_id, limit=1, include_content=True)
        assert len(parent_page["items"]) == 1
        assert "content" in parent_page["items"][0]
        cursor = parent_page["next_cursor"]
        assert cursor is not None
        next_page = cards.list_cards(project_id, limit=1, include_content=True, cursor=cursor)
        assert next_page["next_cursor"] is None
        assert {item["card_id"] for item in parent_page["items"] + next_page["items"]} == {
            parent_id,
            child_id,
        }

        child = cards.get_card(project_id, child_id)
        assert child == new_child
        assert child.keys() == Card.__required_keys__
        metadata = cards.get_card_metadata(project_id, child_id)
        assert metadata == {key: child[key] for key in CardMetadata.__required_keys__}
        assert cards.get_card_breadcrumbs(project_id, child_id) == [
            {"kind": "project", "id": project_id, "title": "Shared Card API"},
            {"kind": "card", "id": parent_id, "title": "Card 1"},
            {"kind": "card", "id": child_id, "title": "Card 2"},
        ]
        with pytest.raises(NotFoundError):
            cards.get_card_metadata("missing", child_id)
        with pytest.raises(NotFoundError):
            cards.get_card_breadcrumbs("missing", child_id)
        assert child["parent_card_id"] == parent_id
        assert child["tags"] == ["research"]
        assert child["tag_occurrences"][0]["tag"] == "research"
        assert child["removable_tags"] == ["research"]
        assert cards.update_card(project_id, child_id, {}) == child
        edited = cards.update_card(project_id, child_id, {"title": "Edited"})
        assert edited["parent_card_id"] == parent_id
        assert edited["content"] == child["content"]
        detached = cards.update_card(project_id, child_id, {"parent_card_id": None})
        assert detached["parent_card_id"] is None
        assert detached["title"] == "Edited"
        assert cards.get_card_breadcrumbs(project_id, child_id) == [
            {"kind": "project", "id": project_id, "title": "Shared Card API"},
            {"kind": "card", "id": child_id, "title": "Edited"},
        ]
        assert cards.get_card(project_id, child_id) == detached

        with pytest.raises(NotFoundError) as error:
            cards.get_card(project_id, "missing")
        assert (error.value.status, error.value.code) == (404, "not_found")

        with pytest.raises(APIError) as error:
            cards.update_card(project_id, child_id, {"parent_card_id": child_id})
        assert (error.value.status, error.value.code) == (422, "invalid_move")

        assert active.list_trash(project_id) == []
        trashed = cards.trash_card(project_id, child_id)
        assert trashed.keys() == Card.__required_keys__
        assert trashed["deleted_at"] is not None
        assert trashed["content"] == detached["content"]
        assert trashed["tags"] == detached["tags"]
        with pytest.raises(NotFoundError):
            cards.get_card_metadata(project_id, child_id)
        with pytest.raises(NotFoundError):
            cards.get_card_breadcrumbs(project_id, child_id)
        with pytest.raises(NotFoundError):
            cards.get_card(project_id, child_id)
        with pytest.raises(NotFoundError):
            cards.trash_card(project_id, child_id)
        expected_trash = {
            "type": "card",
            "project_id": project_id,
            "card_id": child_id,
            "message_id": None,
            "thread_id": None,
            "title": trashed["title"],
            "role": None,
            "deleted_at": trashed["deleted_at"],
            "created_at": None,
            "rel_path": trashed["rel_path"],
        }
        assert active.list_trash(project_id) == [expected_trash]
        assert active.list_trash(project_id, type="all") == [expected_trash]
        assert active.list_trash(project_id, type="ai_message") == []
        assert active.list_trash(project_id, type="unknown") == []
        assert active.list_trash("missing") == []

        [entry] = active.list_trash(project_id, type="card")
        assert entry == expected_trash
        restore_id = entry["card_id"]
        assert restore_id is not None
        restored = cards.restore_card(project_id, restore_id)
        assert restored["deleted_at"] is None
        assert active.list_trash(project_id) == []
        assert cards.get_card(project_id, child_id) == restored
        with pytest.raises(APIError) as error:
            cards.restore_card(project_id, child_id)
        assert (error.value.status, error.value.code) == (409, "conflict")

        cards.update_card(project_id, child_id, {"parent_card_id": parent_id})
        cards.trash_card(project_id, parent_id)
        assert cards.get_card(project_id, child_id)["parent_card_id"] is None
        cards.restore_card(project_id, parent_id)
        assert cards.get_card(project_id, child_id)["parent_card_id"] is None

        cards.update_card(project_id, child_id, {"parent_card_id": parent_id})
        cards.trash_card(project_id, parent_id, children="trash_subtree")
        cards.restore_card(project_id, parent_id)
        with pytest.raises(NotFoundError):
            cards.get_card(project_id, child_id)
        assert cards.restore_card(project_id, child_id)["parent_card_id"] == parent_id

        extra_id = cards.create_card(project_id, {"title": "Another root"})["card_id"]
        # A supplied null parent means the root group. Omitting the parent
        # on subsequent reorders keeps the card in its current group.
        moved = cards.move_card(
            project_id, child_id, {"intent": "to_start", "parent_card_id": None}
        )
        assert moved["parent_card_id"] is None
        assert moved == cards.get_card(project_id, child_id)

        def roots_in_order():
            return [
                item["card_id"]
                for item in cards.list_cards(project_id, parent="roots", order="sort_key_asc")["items"]
            ]

        assert roots_in_order() == [child_id, parent_id, extra_id]
        cards.move_card(project_id, child_id, {"intent": "right"})
        assert roots_in_order() == [parent_id, child_id, extra_id]
        cards.move_card(project_id, child_id, {"intent": "to_end"})
        assert roots_in_order() == [parent_id, extra_id, child_id]
        cards.move_card(project_id, child_id, {"intent": "left"})
        assert roots_in_order() == [parent_id, child_id, extra_id]
        cards.move_card(project_id, child_id, {"intent": "before", "target_card_id": parent_id})
        assert roots_in_order() == [child_id, parent_id, extra_id]
        cards.move_card(project_id, child_id, {"intent": "after", "target_card_id": extra_id})
        assert roots_in_order() == [parent_id, extra_id, child_id]
        cards.move_card(project_id, extra_id, {"intent": "into", "target_card_id": parent_id})
        moved = cards.move_card(project_id, child_id, {"intent": "into", "target_card_id": extra_id})
        assert moved["parent_card_id"] == extra_id
        assert cards.move_card(project_id, child_id, {"intent": "up_level"})["parent_card_id"] == parent_id
        with pytest.raises(NotFoundError):
            cards.move_card(project_id, child_id, {"intent": "into", "target_card_id": "missing"})
        with pytest.raises(APIError) as error:
            cards.move_card(project_id, child_id, {"intent": "into", "target_card_id": child_id})
        assert (error.value.status, error.value.code) == (422, "invalid_move")

        tagged = cards.add_card_tag(project_id, parent_id, "Research")
        assert tagged["tags"] == ["research"]
        assert tagged == cards.add_card_tag(project_id, parent_id, "research")
        tagged = cards.add_card_tag(project_id, parent_id, "Draft/Ideas")
        assert tagged == cards.get_card(project_id, parent_id)
        assert active.list_project_tags(project_id) == [
            {"tag": "research", "card_count": 2},
            {"tag": "draft/ideas", "card_count": 1},
        ]
        matching = cards.list_cards(project_id, tag="draft/ideas")["items"]
        assert [card["card_id"] for card in matching] == [parent_id]
        untagged = cards.remove_card_tag(project_id, parent_id, "Draft/Ideas")
        assert untagged == cards.remove_card_tag(project_id, parent_id, "draft/ideas")
        assert untagged["tags"] == ["research"]
        assert active.list_project_tags(project_id) == [{"tag": "research", "card_count": 2}]
        with pytest.raises(NotFoundError):
            active.list_project_tags("missing")
        with pytest.raises(NotFoundError):
            cards.add_card_tag(project_id, "missing", "research")

        prose = cards.update_card(project_id, child_id, {"content": "Keep #research in this sentence."})
        with pytest.raises(APIError) as error:
            cards.remove_card_tag(project_id, child_id, "research")
        assert (error.value.status, error.value.code) == (409, "conflict")
        assert cards.get_card(project_id, child_id) == prose

        def lookup(reference, scope="live"):
            return cards.resolve_card_reference(
                {"project_id": project_id, "reference": reference, "scope": scope}
            )

        expected = {"card_id": child_id, "title": prose["title"], "deleted_at": None}
        assert lookup(child_id) == {"status": "resolved", "match_kind": "full_id", "card": expected}
        assert lookup(child_id[:-4].upper()) == {
            "status": "resolved", "match_kind": "id_prefix", "card": expected
        }
        assert lookup(prose["title"]) == {
            "status": "resolved", "match_kind": "exact_title", "card": expected
        }
        assert lookup(child_id, "trashed") == {"status": "not_found"}
        assert lookup("no such title") == {"status": "not_found"}

        cards.update_card(project_id, extra_id, {"title": prose["title"]})
        ambiguous = lookup(prose["title"])
        assert set(ambiguous) == {"status", "match_kind", "candidates"}
        assert ambiguous["status"] == "ambiguous"
        assert ambiguous["match_kind"] == "exact_title"
        assert {item["card_id"] for item in ambiguous["candidates"]} == {child_id, extra_id}
        trashed = cards.trash_card(project_id, child_id)
        assert lookup(child_id) == {"status": "not_found"}
        trashed_match = lookup(child_id, "trashed")
        assert trashed_match["card"] == {**expected, "deleted_at": trashed["deleted_at"]}
        assert lookup(child_id, "either") == trashed_match
        assert lookup(prose["title"])["card"]["card_id"] == extra_id
        cards.restore_card(project_id, child_id)

        query = '"café notes"'
        first_id = cards.create_card(project_id, {"title": "First café notes"})["card_id"]
        second_id = cards.create_card(
            project_id, {"title": "Second result", "content": "More café notes"}
        )["card_id"]
        other_project = active.create_plain_project(name="Other search project")
        other_id = cards.create_card(
            other_project["project_id"], {"title": "Other café notes"}
        )["card_id"]

        results = cards.search_cards(project_id, query)
        assert {row["card_id"] for row in results} == {first_id, second_id}
        assert [row["rank"] for row in results] == sorted(row["rank"] for row in results)
        assert all(row.keys() == SearchCard.__required_keys__ for row in results)
        assert "café" in next(row["snippet"] for row in results if row["card_id"] == second_id)
        assert cards.search_cards(project_id, query, limit=1) == results[:1]
        assert cards.search_cards(project_id, query, limit=1, offset=1) == results[1:]
        assert [row["card_id"] for row in cards.search_cards(other_project["project_id"], query)] == [other_id]
        assert cards.search_cards(project_id, "quartzunfindable") == []
        with pytest.raises(APIError) as error:
            cards.search_cards(project_id, '"')
        assert (error.value.status, error.value.code) == (400, "invalid_argument")
        cards.trash_card(project_id, first_id)
        assert [row["card_id"] for row in cards.search_cards(project_id, query)] == [second_id]

        other_project_id = other_project["project_id"]
        kinds = active.list_link_kinds()
        assert all(row.keys() == LinkKindLabels.__required_keys__ for row in kinds)
        kinds_by_name = {row["kind"]: row for row in kinds}
        assert len(kinds_by_name) == len(kinds)
        assert kinds_by_name["depends_on"] == {
            "kind": "depends_on",
            "forward_label": "Depends on",
            "reverse_label": "Required by",
        }

        original = cards.add_card_link(
            project_id,
            second_id,
            {"target_type": "card", "target_id": child_id, "created_at": 1_700_000_000},
        )
        assert original["kind"] == "ref"
        assert original["label"] is None
        assert original["created_at"] == 1_700_000_000
        updated = cards.add_card_link(
            project_id,
            second_id,
            {"target_type": "card", "target_id": child_id, "label": "Local", "created_at": 1_700_000_001},
        )
        assert updated == {**original, "label": "Local"}
        foreign = cards.add_card_link(
            other_project_id,
            other_id,
            {"target_type": "card", "target_id": child_id, "target_project_id": project_id},
        )

        [local] = cards.list_card_links(project_id, second_id)
        [external] = cards.list_card_links(other_project_id, other_id)
        assert local == updated
        assert external == foreign
        assert local.keys() == external.keys() == Link.__required_keys__
        assert (local["source_project_id"], local["source_id"], local["label"]) == (
            project_id, second_id, "Local"
        )
        assert (external["source_project_id"], external["source_id"], external["label"]) == (
            other_project_id, other_id, None
        )
        assert all(
            (link["target_project_id"], link["target_id"], link["target_type"])
            == (project_id, child_id, "card")
            for link in (local, external)
        )
        assert cards.list_card_backlinks(project_id, child_id) == [local]
        assert cards.list_card_backlinks(project_id, child_id, scope="external") == [external]
        all_links = cards.list_card_backlinks(project_id, child_id, scope="all_available")
        assert {link["source_id"] for link in all_links} == {second_id, other_id}

        cards.trash_card(project_id, second_id)
        assert cards.list_card_backlinks(project_id, child_id) == []
        assert cards.list_card_backlinks(project_id, child_id, include_deleted=True) == [local]
        with pytest.raises(NotFoundError):
            cards.list_card_links(project_id, second_id)
        cards.restore_card(project_id, second_id)

        cards.trash_card(project_id, child_id)
        assert cards.list_card_links(project_id, second_id) == []
        assert cards.list_card_links(project_id, second_id, include_deleted=True) == [local]
        assert cards.list_card_links(other_project_id, other_id) == [external]
        with pytest.raises(NotFoundError):
            cards.list_card_backlinks(project_id, child_id, include_deleted=True)

        cards.restore_card(project_id, child_id)
        sibling_link = cards.add_card_link(
            project_id, second_id, {"target_type": "card", "target_id": child_id, "kind": "related"}
        )
        assert sibling_link["kind"] == "related"
        assert sibling_link["kind"] not in kinds_by_name
        assert cards.remove_card_link(project_id, second_id, "card", child_id) == [sibling_link]
        assert cards.remove_card_link(project_id, second_id, "card", child_id) == [sibling_link]
        assert cards.remove_card_link(
            other_project_id, other_id, "card", child_id, target_project_id=project_id
        ) == []
        assert cards.list_card_backlinks(project_id, child_id, scope="all_available") == [sibling_link]
        with pytest.raises(NotFoundError):
            cards.add_card_link(project_id, second_id, {"target_type": "resource", "target_id": "missing"})

        assert cards.list_card_milestones(project_id, child_id) == []
        dated = cards.add_card_milestone(
            project_id,
            child_id,
            {
                "dates": [
                    {"role": "review", "date": "2026-11-30"},
                    {"role": "review", "date": "2026-12-01"},
                    {"role": "review", "datetime": "2026-11-30T09:30:00+01:00"},
                ],
                "kind": "plan",
                "description": "Check the draft",
            },
        )
        assert dated.keys() == Milestone.__required_keys__
        assert dated["dates"] == [
            {"role": "review", "date": "2026-11-30"},
            {"role": "review", "date": "2026-12-01"},
            {"role": "review", "datetime": "2026-11-30T09:30:00+01:00"},
        ]
        first_day, last_day = "2026-11-30", "2026-12-01"
        instant = int(datetime.fromisoformat("2026-11-30T09:30:00+01:00").timestamp())
        assert active.list_milestones_by_day(project_id, first_day, last_day) == [dated]
        assert active.list_milestones_by_day(project_id, first_day, first_day, role="review") == [dated]
        assert active.list_milestones_by_day(project_id, first_day, last_day, role="due") == []
        assert active.list_milestones_by_day(project_id, last_day, first_day) == []
        assert active.list_milestones_by_instant(project_id, instant, instant, role="review") == [dated]
        assert active.list_milestones_by_instant(project_id, instant + 1, instant + 1) == []
        assert active.list_milestones_by_instant(project_id, instant, instant, role="due") == []
        assert active.list_milestones_by_instant(project_id, instant + 1, instant) == []
        with pytest.raises(NotFoundError):
            active.list_milestones_by_day("missing", first_day, last_day)
        with pytest.raises(NotFoundError):
            active.list_milestones_by_instant("missing", instant, instant)
        cards.trash_card(project_id, child_id)
        assert active.list_milestones_by_day(project_id, first_day, last_day) == []
        assert active.list_milestones_by_instant(project_id, instant, instant) == []
        cards.restore_card(project_id, child_id)
        undated = cards.add_card_milestone(project_id, child_id, {"dates": []})
        assert (undated["dates"], undated["kind"], undated["description"]) == ([], None, None)
        milestone_id = dated["milestone_id"]
        assert cards.update_card_milestone(project_id, child_id, milestone_id, {}) == dated
        revised = cards.update_card_milestone(
            project_id, child_id, milestone_id, {"kind": None, "dates": []}
        )
        assert (revised["kind"], revised["dates"], revised["description"]) == (
            None, [], "Check the draft"
        )
        assert {row["milestone_id"] for row in cards.list_card_milestones(project_id, child_id)} == {
            milestone_id, undated["milestone_id"]
        }
        with pytest.raises(APIError) as error:
            cards.add_card_milestone(
                project_id, child_id, {"dates": [{"role": "due", "date": "2026-02-30"}]}
            )
        assert error.value.status == 400
        with pytest.raises(NotFoundError):
            cards.update_card_milestone(project_id, parent_id, milestone_id, {"description": None})
        assert cards.remove_card_milestone(project_id, child_id, milestone_id) == [undated]
        assert cards.remove_card_milestone(project_id, child_id, milestone_id) == [undated]
        cards.trash_card(project_id, child_id)
        with pytest.raises(NotFoundError):
            cards.list_card_milestones(project_id, child_id)
        cards.restore_card(project_id, child_id)
        assert cards.list_card_milestones(project_id, child_id) == [undated]

        day = "2026-03-29"  # The spring clock change makes this a 23-hour day in London.
        last_second = int(datetime.fromisoformat("2026-03-29T22:59:59+00:00").timestamp())
        created = cards.create_card(
            project_id, {"title": "Calendar creation", "created_at": last_second}
        )
        updated = cards.create_card(
            project_id,
            {
                "title": "Calendar update",
                "created_at": last_second - 86400,
                "updated_at": last_second,
            },
        )
        cards.create_card(
            project_id, {"title": "Next day in London", "created_at": last_second + 1}
        )
        inside = cards.add_card_milestone(
            project_id, created["card_id"],
            {"dates": [
                {"role": "due", "date": day},
                {"role": "review", "datetime": "2026-03-29T22:59:59Z"},
            ]},
        )
        outside = cards.add_card_milestone(
            project_id, updated["card_id"],
            {"dates": [{"role": "due", "datetime": "2026-03-29T23:00:00Z"}]},
        )

        updated = cards.update_card(
            project_id, updated["card_id"], {"content": "Revised", "updated_at": last_second}
        )
        assert updated["updated_at"] == last_second

        calendar = active.get_calendar(project_id, day, day, time_zone="Europe/London")
        assert calendar == {
            "milestones": [{**inside, "card_title": created["title"]}],
            "created_cards": [cards.get_card_metadata(project_id, created["card_id"])],
            "updated_cards": [cards.get_card_metadata(project_id, updated["card_id"])],
        }
        utc = active.get_calendar(project_id, day, day, time_zone="UTC")
        assert {row["milestone_id"] for row in utc["milestones"]} == {
            inside["milestone_id"], outside["milestone_id"]
        }
        assert active.get_calendar(project_id, day, "2026-03-28", time_zone="Europe/London") == {
            "milestones": [], "created_cards": [], "updated_cards": []
        }
        with pytest.raises(APIError) as error:
            active.get_calendar(project_id, day, day, time_zone="Invalid/Zone")
        assert error.value.status == 400
        with pytest.raises(NotFoundError):
            active.get_calendar("missing", day, day, time_zone="Europe/London")

    with pytest.raises(ClosedError):
        cards.get_card(project_id, child_id)
    with pytest.raises(ClosedError):
        active.list_projects()



def test_shared_card_operations_project_export(shared_card_session):
    with shared_card_session as active:
        project = active.create_plain_project(name="Export source")
        project_id = project["project_id"]
        assert active.export_project_records(project_id) == {
            "project": project,
            "cards": [],
            "links": [],
            "tag_memberships": [],
            "milestones": [],
        }
        with pytest.raises(NotFoundError):
            active.export_project_records("missing")

        cards = active.card_operations
        source = cards.create_card(
            project_id, {"title": "Source", "content": "Notes about #observations"}
        )
        target = cards.create_card(project_id, {"title": "Target", "content": "#observations"})
        removed = cards.create_card(project_id, {"title": "Removed", "content": "#discarded"})
        external_project = active.create_plain_project(name="Other export project")
        external = cards.create_card(
            external_project["project_id"], {"title": "External", "content": "#observations"}
        )
        source_id = source["card_id"]
        target_id = target["card_id"]
        removed_id = removed["card_id"]
        local_link = cards.add_card_link(
            project_id, source_id, {"target_type": "card", "target_id": target_id}
        )
        external_link = cards.add_card_link(
            project_id,
            source_id,
            {
                "target_type": "card",
                "target_id": external["card_id"],
                "target_project_id": external_project["project_id"],
            },
        )
        cards.add_card_link(
            project_id, source_id, {"target_type": "card", "target_id": removed_id}
        )
        milestone = cards.add_card_milestone(
            project_id, target_id, {"dates": [{"role": "due", "date": "2026-11-30"}]}
        )
        cards.add_card_milestone(project_id, removed_id, {"dates": []})
        cards.trash_card(project_id, removed_id)

        records = active.export_project_records(project_id)
        with_content = active.export_project_records(project_id, include_content=True)

    assert json.loads(json.dumps(records)) == records
    assert records["project"]["project_id"] == project_id
    assert {card["card_id"] for card in records["cards"]} == {source_id, target_id}
    assert all("content" not in card for card in records["cards"])
    assert {card["card_id"]: card["content"] for card in with_content["cards"]} == {
        source_id: source["content"],
        target_id: target["content"],
    }
    assert {link["target_id"] for link in records["links"]} == {
        target_id,
        external["card_id"],
    }
    assert local_link in records["links"]
    assert external_link in records["links"]
    assert records["milestones"] == [milestone]
    assert records["tag_memberships"] == [
        {"project_id": project_id, "card_id": card_id, "tag": "observations"}
        for card_id in sorted((source_id, target_id))
    ]
    assert all(
        with_content[name] == records[name]
        for name in ("project", "links", "tag_memberships", "milestones")
    )


def test_shared_card_operations_analysis_adapters(shared_card_session):
    pytest.importorskip("pandas")
    pytest.importorskip("networkx")
    with shared_card_session as active:
        project = active.create_plain_project(name="Notebook")
        project_id = project["project_id"]
        cards = active.card_operations
        first = cards.create_card(
            project_id, {"title": "Observation", "content": "A note\n#research"}
        )
        second = cards.create_card(project_id, {"title": "Hypothesis", "content": "A theory"})
        link = cards.add_card_link(
            project_id,
            first["card_id"],
            {"target_type": "card", "target_id": second["card_id"], "kind": "tests"},
        )
        records = active.export_project_records(project_id, include_content=True)

    tables = to_dataframes(records)
    by_id = tables["cards"].set_index("card_id")
    assert by_id.loc[first["card_id"], "content"] == first["content"]
    assert str(by_id["updated_at"].dt.tz) == "UTC"
    assert tables["tag_memberships"]["tag"].tolist() == ["research"]
    graph = to_networkx(records)
    source = (project_id, first["card_id"])
    target = (project_id, second["card_id"])
    assert graph.nodes[source]["title"] == first["title"]
    assert graph[source][target]["tests"] == link
