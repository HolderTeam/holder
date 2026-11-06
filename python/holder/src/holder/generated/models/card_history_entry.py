from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card_history_save import CardHistorySave
    from ..models.history_author import HistoryAuthor


T = TypeVar("T", bound="CardHistoryEntry")


@_attrs_define
class CardHistoryEntry:
    """One displayed card change, possibly grouping adjacent autosaves.

    Attributes:
        first_oid (str): The first commit in the entry.
        last_oid (str): The last commit in the entry.
        parent_oids (list[str]): Parents of the first commit.
        visible_parent_oids (list[str]): Direct parents represented on this page; omitted connections must not be
            inferred.
        author (HistoryAuthor): The author recorded on a Git commit.
        started_at (int): When the first commit was authored.
        ended_at (int): When the last commit was committed.
        kind (str): The kind of change shown.
        summary (str): A short description of the change.
        commit_count (int): The number of commits grouped in this entry.
        is_merge (bool): Whether this entry includes a merge commit.
        saves (list[CardHistorySave]): Exact saved commits in chronological order.
    """

    first_oid: str
    last_oid: str
    parent_oids: list[str]
    visible_parent_oids: list[str]
    author: HistoryAuthor
    started_at: int
    ended_at: int
    kind: str
    summary: str
    commit_count: int
    is_merge: bool
    saves: list[CardHistorySave]

    def to_dict(self) -> dict[str, Any]:
        first_oid = self.first_oid

        last_oid = self.last_oid

        parent_oids = self.parent_oids

        visible_parent_oids = self.visible_parent_oids

        author = self.author.to_dict()

        started_at = self.started_at

        ended_at = self.ended_at

        kind = self.kind

        summary = self.summary

        commit_count = self.commit_count

        is_merge = self.is_merge

        saves = []
        for saves_item_data in self.saves:
            saves_item = saves_item_data.to_dict()
            saves.append(saves_item)

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "first_oid": first_oid,
                "last_oid": last_oid,
                "parent_oids": parent_oids,
                "visible_parent_oids": visible_parent_oids,
                "author": author,
                "started_at": started_at,
                "ended_at": ended_at,
                "kind": kind,
                "summary": summary,
                "commit_count": commit_count,
                "is_merge": is_merge,
                "saves": saves,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card_history_save import CardHistorySave
        from ..models.history_author import HistoryAuthor

        d = dict(src_dict)
        first_oid = d.pop("first_oid")

        last_oid = d.pop("last_oid")

        parent_oids = cast(list[str], d.pop("parent_oids"))

        visible_parent_oids = cast(list[str], d.pop("visible_parent_oids"))

        author = HistoryAuthor.from_dict(d.pop("author"))

        started_at = d.pop("started_at")

        ended_at = d.pop("ended_at")

        kind = d.pop("kind")

        summary = d.pop("summary")

        commit_count = d.pop("commit_count")

        is_merge = d.pop("is_merge")

        saves = []
        _saves = d.pop("saves")
        for saves_item_data in _saves:
            saves_item = CardHistorySave.from_dict(saves_item_data)

            saves.append(saves_item)

        card_history_entry = cls(
            first_oid=first_oid,
            last_oid=last_oid,
            parent_oids=parent_oids,
            visible_parent_oids=visible_parent_oids,
            author=author,
            started_at=started_at,
            ended_at=ended_at,
            kind=kind,
            summary=summary,
            commit_count=commit_count,
            is_merge=is_merge,
            saves=saves,
        )

        return card_history_entry
