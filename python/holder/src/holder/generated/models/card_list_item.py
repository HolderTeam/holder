from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="CardListItem")


@_attrs_define
class CardListItem:
    """A card in a listing.

    Attributes:
        card_id (str):
        project_id (str):
        title (str): The card's title.
        rel_path (str): The card's file, relative to the project's root.
        parent_card_id (None | str): The card's parent; null for a root card.
        sort_key (float): The card's place among its siblings; lower comes first.
        created_at (int): When the card was created.
        updated_at (int): When the card was last changed.
        deleted_at (int | None): When the card was moved to the trash; null when it is not in the trash.
        child_count (int | Unset): The card's live children; present when asked for.
        content (str | Unset): The card's body; present when asked for.
    """

    card_id: str
    project_id: str
    title: str
    rel_path: str
    parent_card_id: None | str
    sort_key: float
    created_at: int
    updated_at: int
    deleted_at: int | None
    child_count: int | Unset = UNSET
    content: str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        card_id = self.card_id

        project_id = self.project_id

        title = self.title

        rel_path = self.rel_path

        parent_card_id: None | str
        parent_card_id = self.parent_card_id

        sort_key = self.sort_key

        created_at = self.created_at

        updated_at = self.updated_at

        deleted_at: int | None
        deleted_at = self.deleted_at

        child_count = self.child_count

        content = self.content

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "card_id": card_id,
                "project_id": project_id,
                "title": title,
                "rel_path": rel_path,
                "parent_card_id": parent_card_id,
                "sort_key": sort_key,
                "created_at": created_at,
                "updated_at": updated_at,
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

        def _parse_parent_card_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        parent_card_id = _parse_parent_card_id(d.pop("parent_card_id"))

        sort_key = d.pop("sort_key")

        created_at = d.pop("created_at")

        updated_at = d.pop("updated_at")

        def _parse_deleted_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        deleted_at = _parse_deleted_at(d.pop("deleted_at"))

        child_count = d.pop("child_count", UNSET)

        content = d.pop("content", UNSET)

        card_list_item = cls(
            card_id=card_id,
            project_id=project_id,
            title=title,
            rel_path=rel_path,
            parent_card_id=parent_card_id,
            sort_key=sort_key,
            created_at=created_at,
            updated_at=updated_at,
            deleted_at=deleted_at,
            child_count=child_count,
            content=content,
        )

        return card_list_item
