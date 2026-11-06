from __future__ import annotations

from pathlib import Path
from typing import Any, Callable

import pytest

import holderkit
from holderkit import _native


def test_card_tags_share_project_semantics_and_detached_exports(tmp_path: Path) -> None:
    with holderkit.create('Tags', workspace=tmp_path / 'project') as project:
        card = project.create_card('Card', 'Body #prose\n\n#todo')
        other = project.create_card('Other', '#other')
        tags = card.tags
        assert tags.list() == project.tags.list(card.card_id)
        assert tags.list_editable() == ['todo']
        assert tags.add('EXPERIMENT') == holderkit.TagAddResult.ADDED
        assert tags.add('experiment') == holderkit.TagAddResult.ALREADY_PRESENT
        with pytest.raises(holderkit.HolderError, match='prose'):
            tags.remove('prose')
        assert tags.remove('todo') == holderkit.TagRemoveResult.REMOVED
        assert tags.remove('missing') == holderkit.TagRemoveResult.NOT_PRESENT
        assert '#experiment' in card.content
        records = tags.to_records()
        assert records == [r for r in project.tags.to_records() if r['card_id'] == card.card_id]
        assert all(r['card_id'] != other.card_id for r in records)
        assert all(tuple(r) == holderkit.TAG_RECORD_FIELDS for r in records)
        project.tags.remove(card.card_id, 'experiment')
        assert 'experiment' not in tags.list()
        assert any(r['tag'] == 'experiment' for r in records)
        with pytest.raises(ValueError):
            tags.add('#bad')
    assert records[0]['project_id'] == project.project_id


def test_card_connections_take_cards_and_export_only_their_source(tmp_path: Path) -> None:
    with holderkit.open(tmp_path / 'data') as context:
        project = context.create_project('Connections')
        other_project = context.create_project('Other')
        source = project.create_card('Source', '')
        target = project.create_card('Target', '')
        external = other_project.create_card('External', '')
        connections = source.connections
        connections.add(target, kind='depends_on', label='First')
        connections.add(target, kind='depends_on', label='Updated')
        connections.add(external, kind='references')
        target.connections.add(source, kind='references')
        records = connections.to_records()
        assert len(records) == 2
        assert all(tuple(r) == holderkit.CONNECTION_RECORD_FIELDS for r in records)
        assert {r['to_card_id'] for r in records} == {target.card_id, external.card_id}
        external_record = next(r for r in records if r['to_card_id'] == external.card_id)
        assert external_record['target_project_id'] == other_project.project_id
        assert all(r['from_card_id'] == source.card_id for r in records)
        assert next(r['label'] for r in records if r['to_card_id'] == target.card_id) == 'Updated'
        assert records == [r for r in project.connections.to_records() if r['from_card_id'] == source.card_id]
        connections.remove(target, kind='depends_on')
        connections.remove(target, kind='depends_on')
        assert len(connections.to_records()) == 1
        assert len(records) == 2
        external.trash()
        with pytest.raises(ValueError, match='Trash'):
            connections.add(external, kind='references')
        connections.remove(external, kind='references')
        assert connections.to_records() == []
        assert target.connections.to_records()[0]['to_card_id'] == source.card_id


def test_connections_reject_foreign_context_and_stale_target(tmp_path: Path) -> None:
    with holderkit.create('Source', workspace=tmp_path / 'source') as project:
        source = project.create_card('Source', '')
        target = project.create_card('Target', '')
        with holderkit.create('Foreign', workspace=tmp_path / 'foreign') as foreign:
            another = foreign.create_card('Other', '')
            # Even identical IDs in another context cannot redirect a mutation.
            forged = holderkit.Card(foreign, target.card_id)
            for invalid in (another, forged):
                for operation in (source.connections.add, source.connections.remove):
                    with pytest.raises(ValueError, match='Context'):
                        operation(invalid, kind='references')
        with pytest.raises(TypeError, match='Card'):
            source.connections.add(target.card_id, kind='references')  # type: ignore[arg-type]
        with pytest.raises(TypeError, match='Card'):
            source.connections.remove(target.card_id, kind='references')  # type: ignore[arg-type]
        target.trash()
        target.purge()
        project.create_card(target.card_id, 'Decoy')
        with pytest.raises(KeyError):
            source.connections.add(target, kind='references')
        assert source.connections.to_records() == []


def test_card_milestones_match_project_results_and_isolate_ownership(tmp_path: Path) -> None:
    with holderkit.create('Dates', workspace=tmp_path / 'project') as project:
        card = project.create_card('Card', '')
        other = project.create_card('Other', '')
        milestones = card.milestones
        assert milestones.list() == milestones.to_records() == []
        added = milestones.add([{'role': 'review', 'date': '2026-06-02'}], kind='review', description='Check')
        assert [added] == project.milestones.list(card.card_id)
        assert tuple(added) == holderkit.MILESTONE_RECORD_FIELDS
        identity = added['milestone_id']
        foreign = other.milestones.add([{'role': 'due', 'date': '2026-07-01'}])['milestone_id']
        updated = milestones.update(identity, {'description': None, 'dates': [{'role': 'review', 'date': '2026-06-03'}]})
        assert updated['description'] is None and updated['dates'] == [{'role': 'review', 'date': '2026-06-03'}]
        with pytest.raises(holderkit.HolderError, match='milestone not found'):
            milestones.update(foreign, {'kind': 'wrong'})
        milestones.remove(foreign)  # Another card's milestone: removing it through this card changes nothing.
        assert other.milestones.list()[0]['dates'] == [{'role': 'due', 'date': '2026-07-01'}]
        with pytest.raises(ValueError):
            milestones.update(identity, {'dates': [{'role': 'review', 'date': 'June'}]})
        with pytest.raises(TypeError):
            milestones.update(identity, {'kind': True})  # type: ignore[typeddict-item]
        assert milestones.list()[0] == updated
        assert milestones.remove(identity) == []
        assert milestones.remove(identity) == []
        assert milestones.to_records() == []
    assert updated['card_id'] == card.card_id


def _operations(card: holderkit.Card) -> list[Callable[[], object]]:
    tags, connections, milestones = card.tags, card.connections, card.milestones
    return [tags.list, tags.list_editable, tags.to_records, lambda: tags.add('work'),
            lambda: tags.remove('work'), connections.to_records,
            lambda: connections.add(card, kind='references'), lambda: connections.remove(card, kind='references'),
            milestones.list, milestones.to_records, lambda: milestones.add(),
            lambda: milestones.update('missing', {}), lambda: milestones.remove('missing')]


def test_retained_adapters_follow_trash_restore_purge_and_close(tmp_path: Path) -> None:
    project = holderkit.create('State', workspace=tmp_path / 'project')
    card = project.create_card('Card', '')
    tags, connections, milestones = card.tags, card.connections, card.milestones
    operations = _operations(card)
    card.trash()
    for operation in operations:
        with pytest.raises(ValueError, match='Trash'):
            operation()
    card.restore()
    tags.add('restored')
    assert tags.list() == ['restored']
    assert connections.to_records() == [] and milestones.list() == []
    card.trash()
    card.purge()
    for operation in operations:
        with pytest.raises(KeyError):
            operation()
    project.close()
    for operation in operations:
        with pytest.raises(RuntimeError, match='closed'):
            operation()


def test_context_owner_close_and_target_owner_close(tmp_path: Path) -> None:
    with holderkit.open(tmp_path / 'data') as context:
        project = context.create_project('Borrowed')
        source = project.create_card('Source', '')
        target = context.create_card(project.project_id, 'Target', '')
        target.project.close()
        assert not project.closed
        with pytest.raises(RuntimeError, match='closed'):
            source.connections.add(target, kind='references')
        adapters = _operations(source)
    for operation in adapters:
        with pytest.raises(RuntimeError, match='closed'):
            operation()


def test_card_exports_do_not_scan_other_cards_or_request_complete_bodies(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    with holderkit.create('Exports', workspace=tmp_path / 'project') as project:
        card = project.create_card('Card', '#work')
        target = project.create_card('Target', '')
        card.connections.add(target, 'references')
        card.milestones.add([{'role': 'due', 'date': '2026-06-02'}])
        (Path(project.root_path) / card.rel_path).unlink()
        def forbidden(*args: Any, **kwargs: Any) -> Any:
            raise AssertionError('Card export must not scan a project or read bodies')
        monkeypatch.setattr(_native.Context, 'list_cards', forbidden)
        monkeypatch.setattr(_native.Context, 'get_card_content', forbidden)
        monkeypatch.setattr(_native.Context, 'list_cards_page', forbidden)
        # Generated Card tag details require the card body to be readable.
        with pytest.raises(holderkit.HolderError, match='card content missing'):
            card.tags.to_records()
        assert card.connections.to_records()[0]['to_card_id'] == target.card_id
        assert card.milestones.to_records()[0]['dates'] == [{'role': 'due', 'date': '2026-06-02'}]


def test_card_dataframes_are_scoped_and_detached(tmp_path: Path) -> None:
    pd = pytest.importorskip('pandas')
    with holderkit.create('Frames', workspace=tmp_path / 'project') as project:
        card = project.create_card('Card', '#work')
        other = project.create_card('Other', '#other')
        card.connections.add(other, 'references')
        card.milestones.add([{'role': 'due', 'date': '2026-06-02'}])
        frames = [card.tags.to_dataframe(), card.connections.to_dataframe(), card.milestones.to_dataframe()]
        assert list(frames[0].columns) == list(holderkit.TAG_RECORD_FIELDS)
        assert list(frames[1].columns) == list(holderkit.CONNECTION_RECORD_FIELDS)
        assert list(frames[2].columns) == list(holderkit.MILESTONE_RECORD_FIELDS)
        assert all(frame.shape[0] == 1 for frame in frames)
        assert frames[2].iloc[0]['dates'] == [{'role': 'due', 'date': '2026-06-02'}]
        frames[0].loc[0, 'tag'] = 'local'
        assert card.tags.list() == ['work']
        empty = other.milestones.to_dataframe()
        assert empty.empty and list(empty.columns) == list(holderkit.MILESTONE_RECORD_FIELDS)
    assert frames[0].iloc[0]['tag'] == 'local'
