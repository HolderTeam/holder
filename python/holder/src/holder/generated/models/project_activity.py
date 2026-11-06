from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.history_affected_object import HistoryAffectedObject
    from ..models.history_author import HistoryAuthor


T = TypeVar("T", bound="ProjectActivity")


@_attrs_define
class ProjectActivity:
    """One Git commit affecting a project.

    Attributes:
        oid (str): The full commit object ID.
        parent_oids (list[str]): The commit's parents.
        author (HistoryAuthor): The author recorded on a Git commit.
        authored_at (int): When the commit was authored.
        committed_at (int): When the commit was committed.
        message (str): The commit message.
        affected_objects (list[HistoryAffectedObject]): Changed paths grouped by Holder object kind.
        is_merge (bool): Whether the commit has multiple parents.
    """

    oid: str
    parent_oids: list[str]
    author: HistoryAuthor
    authored_at: int
    committed_at: int
    message: str
    affected_objects: list[HistoryAffectedObject]
    is_merge: bool

    def to_dict(self) -> dict[str, Any]:
        oid = self.oid

        parent_oids = self.parent_oids

        author = self.author.to_dict()

        authored_at = self.authored_at

        committed_at = self.committed_at

        message = self.message

        affected_objects = []
        for affected_objects_item_data in self.affected_objects:
            affected_objects_item = affected_objects_item_data.to_dict()
            affected_objects.append(affected_objects_item)

        is_merge = self.is_merge

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "oid": oid,
                "parent_oids": parent_oids,
                "author": author,
                "authored_at": authored_at,
                "committed_at": committed_at,
                "message": message,
                "affected_objects": affected_objects,
                "is_merge": is_merge,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.history_affected_object import (
            HistoryAffectedObject,
        )
        from ..models.history_author import HistoryAuthor

        d = dict(src_dict)
        oid = d.pop("oid")

        parent_oids = cast(list[str], d.pop("parent_oids"))

        author = HistoryAuthor.from_dict(d.pop("author"))

        authored_at = d.pop("authored_at")

        committed_at = d.pop("committed_at")

        message = d.pop("message")

        affected_objects = []
        _affected_objects = d.pop("affected_objects")
        for affected_objects_item_data in _affected_objects:
            affected_objects_item = HistoryAffectedObject.from_dict(
                affected_objects_item_data
            )

            affected_objects.append(affected_objects_item)

        is_merge = d.pop("is_merge")

        project_activity = cls(
            oid=oid,
            parent_oids=parent_oids,
            author=author,
            authored_at=authored_at,
            committed_at=committed_at,
            message=message,
            affected_objects=affected_objects,
            is_merge=is_merge,
        )

        return project_activity
