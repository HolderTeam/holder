from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="NewCard")


@_attrs_define
class NewCard:
    """A card to create.

    Attributes:
        project_id (str): The project to create it in.
        title (str): The card's title.
        content (str | Unset): The card's body. Default: ''.
        parent_card_id (None | str | Unset): The card's parent; null or omitted for a root card.
        sort_key (float | Unset): The card's place among its siblings; omit to let Holder choose.
        created_at (int | Unset): When the card was created, for imports; omit for now.
        updated_at (int | Unset): When the card was last changed, for imports; omit to use created_at.
    """

    project_id: str
    title: str
    content: str | Unset = ""
    parent_card_id: None | str | Unset = UNSET
    sort_key: float | Unset = UNSET
    created_at: int | Unset = UNSET
    updated_at: int | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        title = self.title

        content = self.content

        parent_card_id: None | str | Unset
        if isinstance(self.parent_card_id, Unset):
            parent_card_id = UNSET
        else:
            parent_card_id = self.parent_card_id

        sort_key = self.sort_key

        created_at = self.created_at

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "title": title,
            }
        )
        if content is not UNSET:
            field_dict["content"] = content
        if parent_card_id is not UNSET:
            field_dict["parent_card_id"] = parent_card_id
        if sort_key is not UNSET:
            field_dict["sort_key"] = sort_key
        if created_at is not UNSET:
            field_dict["created_at"] = created_at
        if updated_at is not UNSET:
            field_dict["updated_at"] = updated_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        title = d.pop("title")

        content = d.pop("content", UNSET)

        def _parse_parent_card_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        parent_card_id = _parse_parent_card_id(d.pop("parent_card_id", UNSET))

        sort_key = d.pop("sort_key", UNSET)

        created_at = d.pop("created_at", UNSET)

        updated_at = d.pop("updated_at", UNSET)

        new_card = cls(
            project_id=project_id,
            title=title,
            content=content,
            parent_card_id=parent_card_id,
            sort_key=sort_key,
            created_at=created_at,
            updated_at=updated_at,
        )

        return new_card
