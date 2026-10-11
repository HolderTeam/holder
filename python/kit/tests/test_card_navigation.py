from __future__ import annotations

from pathlib import Path
from typing import Any

import pytest

import holderkit
from holderkit import _native


def test_get_resolves_only_exact_ids_in_its_project(tmp_path: Path) -> None:
    with holderkit.open(tmp_path / 'data') as context:
        project = context.create_project('Lookup')
        other = context.create_project('Other')
        card = project.create_card('Title', 'Body')
        foreign = other.create_card('Foreign')
        found = project.cards.get(card.card_id)
        assert found == card and found.project is project
        card.update('Changed', 'New title')
        assert found.title == 'New title' and found.content == 'Changed'
        for invalid in ('missing', 'New title', card.card_id[:8], foreign.card_id):
            with pytest.raises(KeyError):
                project.cards.get(invalid)
        with pytest.raises(TypeError):
            project.cards.get(123)  # type: ignore[arg-type]
        with pytest.raises(ValueError):
            project.cards.get('')
        card.trash()
        trashed = project.cards.get(card.card_id)
        assert trashed.deleted_at is not None and trashed.project is project
        trashed.restore()
        assert found.deleted_at is None
        card.trash()
        card.purge()
        decoy = project.create_card(card.card_id, 'Decoy')
        with pytest.raises(KeyError):
            project.cards.get(card.card_id)
        assert project.cards.get(decoy.card_id) == decoy
        project.close()
        with pytest.raises(RuntimeError, match='closed'):
            project.cards.get(decoy.card_id)


def test_children_read_current_membership_in_core_order(tmp_path: Path) -> None:
    with holderkit.create('Tree', workspace=tmp_path / 'project') as project:
        parent = project.create_card('Parent')
        children = parent.children
        assert children.list() == []
        first = project.create_card('First', parent_card_id=parent.card_id)
        second = project.create_card('Second', parent_card_id=parent.card_id)
        grandchild = project.create_card('Grandchild', parent_card_id=first.card_id)
        unrelated = project.create_card('Unrelated')
        initial = children.list()
        assert initial == [first, second]
        assert all(c.project is project for c in initial)
        assert grandchild not in initial and unrelated not in initial
        second.move(before=first)
        assert children.list() == [second, first]
        assert initial == [first, second]
        second.update('', 'Renamed')
        assert initial[1].title == 'Renamed'
        second.move(before=parent)
        assert children.list() == [first]
        unrelated.move(into=parent)
        assert children.list() == [first, unrelated]
        unrelated.trash()
        assert children.list() == [first]
        unrelated.restore()
        assert unrelated in children.list()
        parent.trash()
        with pytest.raises(ValueError, match='Trash'):
            children.list()
        parent.restore()
        assert children.list() == []  # Restore does not undo child promotion.
        assert first.parent is None and grandchild.parent == first
        first.move(into=parent)
        assert children.list() == [first]
    with holderkit.reopen(tmp_path / 'project') as reopened:
        reopened_parent = reopened.cards.get(parent.card_id)
        current = reopened_parent.children.list()
        assert [c.card_id for c in current] == [first.card_id]
        assert current[0].project is reopened


def test_retained_children_reject_purged_or_closed_parent(tmp_path: Path) -> None:
    with holderkit.create('Lifecycle', workspace=tmp_path / 'project') as project:
        parent = project.create_card('Parent')
        children = parent.children
        parent.trash()
        parent.purge()
        project.create_card(parent.card_id, 'Decoy')
        with pytest.raises(KeyError):
            children.list()
        live = project.create_card('Live')
        retained = live.children
    with pytest.raises(RuntimeError, match='closed'):
        retained.list()


def test_navigation_reads_metadata_without_bodies_or_project_scans(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch,
) -> None:
    with holderkit.create('Metadata', workspace=tmp_path / 'project') as project:
        parent = project.create_card('Parent', 'Body')
        child = project.create_card('Child', 'Body', parent.card_id)
        project.create_card('Unrelated', 'Body')
        # Missing bodies must not prevent lookup or hierarchy reads.
        for card in project.cards.list():
            (Path(project.root_path) / card.rel_path).unlink()
        with pytest.raises(holderkit.HolderError):
            _ = child.content

        def forbidden(*args: Any, **kwargs: Any) -> Any:
            raise AssertionError('Navigation must not scan the project or read bodies')

        monkeypatch.setattr(_native.Context, 'list_cards', forbidden)
        monkeypatch.setattr(_native.Context, 'get_card_content', forbidden)
        monkeypatch.setattr(_native.Context, 'list_complete_cards_page', forbidden)
        assert project.cards.get(parent.card_id) == parent
        assert parent.children.list() == [child]
        assert parent.children.list()[0].parent == parent


def test_context_cards_keep_borrowed_owner_for_children(tmp_path: Path) -> None:
    with holderkit.open(tmp_path / 'data') as context:
        project = context.create_project('Borrowed')
        parent = context.create_card(project.project_id, 'Parent', '')
        child = project.create_card('Child', parent_card_id=parent.card_id)
        children = parent.children
        assert children.list() == [child]
        assert children.list()[0].project is parent.project
        parent.project.close()
        with pytest.raises(RuntimeError, match='closed'):
            children.list()
        assert project.cards.get(child.card_id).parent == project.cards.get(parent.card_id)
    with pytest.raises(RuntimeError, match='closed'):
        project.cards.get(child.card_id)
