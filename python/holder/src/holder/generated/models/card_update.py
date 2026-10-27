from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="CardUpdate")


@_attrs_define
class CardUpdate:
    """Changes to a card. Each field left out keeps its value.

    Attributes:
        title (str | Unset): The card's new title.
        content (str | Unset): The card's new body.
        parent_card_id (None | str | Unset): The card's new parent; null makes it a root card.
        sort_key (float | Unset): The card's new place among its siblings.
        updated_at (int | Unset): When the card was changed, for imports; omit for now.
    """

    title: str | Unset = UNSET
    content: str | Unset = UNSET
    parent_card_id: None | str | Unset = UNSET
    sort_key: float | Unset = UNSET
    updated_at: int | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        title = self.title

        content = self.content

        parent_card_id: None | str | Unset
        if isinstance(self.parent_card_id, Unset):
            parent_card_id = UNSET
        else:
            parent_card_id = self.parent_card_id

        sort_key = self.sort_key

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update({})
        if title is not UNSET:
            field_dict["title"] = title
        if content is not UNSET:
            field_dict["content"] = content
        if parent_card_id is not UNSET:
            field_dict["parent_card_id"] = parent_card_id
        if sort_key is not UNSET:
            field_dict["sort_key"] = sort_key
        if updated_at is not UNSET:
            field_dict["updated_at"] = updated_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        title = d.pop("title", UNSET)

        content = d.pop("content", UNSET)

        def _parse_parent_card_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        parent_card_id = _parse_parent_card_id(d.pop("parent_card_id", UNSET))

        sort_key = d.pop("sort_key", UNSET)

        updated_at = d.pop("updated_at", UNSET)

        card_update = cls(
            title=title,
            content=content,
            parent_card_id=parent_card_id,
            sort_key=sort_key,
            updated_at=updated_at,
        )

        return card_update
