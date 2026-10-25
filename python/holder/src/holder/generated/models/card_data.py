from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card_tag_occurrence import CardTagOccurrence


T = TypeVar("T", bound="CardData")


@_attrs_define
class CardData:
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
        content (str):
        tags (list[str]): The card's normalized tags.
        tag_occurrences (list[CardTagOccurrence]): Each tag written in content, in order.
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
    content: str
    tags: list[str]
    tag_occurrences: list[CardTagOccurrence]
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

        content = self.content

        tags = self.tags

        tag_occurrences = []
        for tag_occurrences_item_data in self.tag_occurrences:
            tag_occurrences_item = tag_occurrences_item_data.to_dict()
            tag_occurrences.append(tag_occurrences_item)

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
                "content": content,
                "tags": tags,
                "tag_occurrences": tag_occurrences,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card_tag_occurrence import CardTagOccurrence

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

        content = d.pop("content")

        tags = cast(list[str], d.pop("tags"))

        tag_occurrences = []
        _tag_occurrences = d.pop("tag_occurrences")
        for tag_occurrences_item_data in _tag_occurrences:
            tag_occurrences_item = CardTagOccurrence.from_dict(
                tag_occurrences_item_data
            )

            tag_occurrences.append(tag_occurrences_item)

        card_data = cls(
            card_id=card_id,
            project_id=project_id,
            title=title,
            rel_path=rel_path,
            sort_key=sort_key,
            created_at=created_at,
            updated_at=updated_at,
            parent_card_id=parent_card_id,
            deleted_at=deleted_at,
            content=content,
            tags=tags,
            tag_occurrences=tag_occurrences,
        )

        card_data.additional_properties = d
        return card_data

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
