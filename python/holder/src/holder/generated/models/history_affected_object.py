from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.history_object_kind import HistoryObjectKind

if TYPE_CHECKING:
    from ..models.history_affected_path import HistoryAffectedPath


T = TypeVar("T", bound="HistoryAffectedObject")


@_attrs_define
class HistoryAffectedObject:
    """Paths changed together in one commit, grouped by Holder object kind.

    Attributes:
        kind (HistoryObjectKind):
        paths (list[str]): The changed paths, retained for existing clients.
        items (list[HistoryAffectedPath]): The changed paths with historical display details.
    """

    kind: HistoryObjectKind
    paths: list[str]
    items: list[HistoryAffectedPath]

    def to_dict(self) -> dict[str, Any]:
        kind = self.kind.value

        paths = self.paths

        items = []
        for items_item_data in self.items:
            items_item = items_item_data.to_dict()
            items.append(items_item)

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "kind": kind,
                "paths": paths,
                "items": items,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.history_affected_path import HistoryAffectedPath

        d = dict(src_dict)
        kind = HistoryObjectKind(d.pop("kind"))

        paths = cast(list[str], d.pop("paths"))

        items = []
        _items = d.pop("items")
        for items_item_data in _items:
            items_item = HistoryAffectedPath.from_dict(items_item_data)

            items.append(items_item)

        history_affected_object = cls(
            kind=kind,
            paths=paths,
            items=items,
        )

        return history_affected_object
