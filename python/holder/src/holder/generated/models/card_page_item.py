from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="CardPageItem")


@_attrs_define
class CardPageItem:
    """
    Attributes:
        card_id (str):
        project_id (str):
        title (str):
        rel_path (str):
        sort_key (float):
        created_at (int):
        updated_at (int):
        parent_card_id (None | str):
        deleted_at (int | None):
        child_count (int | Unset): Live children, present when requested (count=true, or include_child_count=true on
            project card pages).
        content (str | Unset): The card's body. Present only when include_content is true.
    """

    card_id: str
    project_id: str
    title: str
    rel_path: str
    sort_key: float
    created_at: int
    updated_at: int
    parent_card_id: None | str
    deleted_at: int | None
    child_count: int | Unset = UNSET
    content: str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        card_id = self.card_id

        project_id = self.project_id

        title = self.title

        rel_path = self.rel_path

        sort_key = self.sort_key

        created_at = self.created_at

        updated_at = self.updated_at

        parent_card_id: None | str
        parent_card_id = self.parent_card_id

        deleted_at: int | None
        deleted_at = self.deleted_at

        child_count = self.child_count

        content = self.content

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "card_id": card_id,
                "project_id": project_id,
                "title": title,
                "rel_path": rel_path,
                "sort_key": sort_key,
                "created_at": created_at,
                "updated_at": updated_at,
                "parent_card_id": parent_card_id,
                "deleted_at": deleted_at,
            }
        )
        if child_count is not UNSET:
            field_dict["child_count"] = child_count
        if content is not UNSET:
            field_dict["content"] = content

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        card_id = d.pop("card_id")

        project_id = d.pop("project_id")

        title = d.pop("title")

        rel_path = d.pop("rel_path")

        sort_key = d.pop("sort_key")

        created_at = d.pop("created_at")

        updated_at = d.pop("updated_at")

        def _parse_parent_card_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        parent_card_id = _parse_parent_card_id(d.pop("parent_card_id"))

        def _parse_deleted_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        deleted_at = _parse_deleted_at(d.pop("deleted_at"))

        child_count = d.pop("child_count", UNSET)

        content = d.pop("content", UNSET)

        card_page_item = cls(
            card_id=card_id,
            project_id=project_id,
            title=title,
            rel_path=rel_path,
            sort_key=sort_key,
            created_at=created_at,
            updated_at=updated_at,
            parent_card_id=parent_card_id,
            deleted_at=deleted_at,
            child_count=child_count,
            content=content,
        )

        card_page_item.additional_properties = d
        return card_page_item

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
