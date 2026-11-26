"""Opt-in integration against an isolated, disposable holderd process.

Set HOLDER_TEST_DAEMON to a built holderd executable. This never uses the user's
normal daemon, database, or projects.
"""

import json
import os
import subprocess
import time
from datetime import UTC, date, datetime, timedelta, timezone
from importlib.util import find_spec
from pathlib import Path

import httpx
import pytest

from holder import Holder, LinkKindLabels, MilestoneDate, ProjectRecord, open_session
from holder.cards import Card, CardListItem, CardMetadata, Link, SearchCard
from holder.exceptions import APIError, ClosedError, NotFoundError

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


@pytest.mark.parametrize("backend", ["daemon", "embedded"])
def test_shared_card_operations(backend, request, tmp_path):
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

    with session as active:
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

    with pytest.raises(ClosedError):
        cards.get_card(project_id, child_id)
    with pytest.raises(ClosedError):
        active.list_projects()
