from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.tag_occurrence import TagOccurrence


T = TypeVar("T", bound="Card")


@_attrs_define
class Card:
    """A card, with its content and tags.

    Attributes:
        card_id (str): The card's ID.
        project_id (str): The project the card belongs to.
        title (str): The card's title.
        rel_path (str): The card's file, relative to the project's root.
        parent_card_id (None | str): The card's parent; null for a root card.
        sort_key (float): The card's place among its siblings; lower comes first.
        created_at (int): When the card was created.
        updated_at (int): When the card was last changed.
        deleted_at (int | None): When the card was moved to the trash; null when it is not in the trash.
        content (str): The card's body.
        tags (list[str]): The card's normalized tags.
        tag_occurrences (list[TagOccurrence]): Each tag written in the content, in order.
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
    content: str
    tags: list[str]
    tag_occurrences: list[TagOccurrence]

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

        content = self.content

        tags = self.tags

        tag_occurrences = []
        for tag_occurrences_item_data in self.tag_occurrences:
            tag_occurrences_item = tag_occurrences_item_data.to_dict()
            tag_occurrences.append(tag_occurrences_item)

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
                "content": content,
                "tags": tags,
                "tag_occurrences": tag_occurrences,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.tag_occurrence import TagOccurrence

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

        content = d.pop("content")

        tags = cast(list[str], d.pop("tags"))

        tag_occurrences = []
        _tag_occurrences = d.pop("tag_occurrences")
        for tag_occurrences_item_data in _tag_occurrences:
            tag_occurrences_item = TagOccurrence.from_dict(tag_occurrences_item_data)

            tag_occurrences.append(tag_occurrences_item)

        card = cls(
            card_id=card_id,
            project_id=project_id,
            title=title,
            rel_path=rel_path,
            parent_card_id=parent_card_id,
            sort_key=sort_key,
            created_at=created_at,
            updated_at=updated_at,
            deleted_at=deleted_at,
            content=content,
            tags=tags,
            tag_occurrences=tag_occurrences,
        )

        return card
