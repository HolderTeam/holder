from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..models.project_history_affected_object_kind import (
    ProjectHistoryAffectedObjectKind,
)

if TYPE_CHECKING:
    from ..models.project_history_affected_object_items_item import (
        ProjectHistoryAffectedObjectItemsItem,
    )


T = TypeVar("T", bound="ProjectHistoryAffectedObject")


@_attrs_define
class ProjectHistoryAffectedObject:
    """
    Attributes:
        kind (ProjectHistoryAffectedObjectKind):
        paths (list[str]):
        items (list[ProjectHistoryAffectedObjectItemsItem]): Historical display details for the affected paths, when
            available.
    """

    kind: ProjectHistoryAffectedObjectKind
    paths: list[str]
    items: list[ProjectHistoryAffectedObjectItemsItem]
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        kind = self.kind.value

        paths = self.paths

        items = []
        for items_item_data in self.items:
            items_item = items_item_data.to_dict()
            items.append(items_item)

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
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
        from ..models.project_history_affected_object_items_item import (
            ProjectHistoryAffectedObjectItemsItem,
        )

        d = dict(src_dict)
        kind = ProjectHistoryAffectedObjectKind(d.pop("kind"))

        paths = cast(list[str], d.pop("paths"))

        items = []
        _items = d.pop("items")
        for items_item_data in _items:
            items_item = ProjectHistoryAffectedObjectItemsItem.from_dict(
                items_item_data
            )

            items.append(items_item)

        project_history_affected_object = cls(
            kind=kind,
            paths=paths,
            items=items,
        )

        project_history_affected_object.additional_properties = d
        return project_history_affected_object

    @property
    def additional_keys(self) -> list[str]:
        return list(self.additional_properties.keys())

    def __getitem__(self, key: str) -> Any:
        return self.additional_properties[key]

    def __setitem__(self, key: str, value: Any) -> None:
        self.additional_properties[key] = value

    def __delitem__(self, key: str) -> None:
        del self.additional_properties[key]

    def __contains__(self, key: str) -> bool:
        return key in self.additional_properties
