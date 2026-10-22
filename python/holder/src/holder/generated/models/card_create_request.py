from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="CardCreateRequest")


@_attrs_define
class CardCreateRequest:
    """Holder generates the card ID, so card_id, like any other unlisted property, is rejected.

    Attributes:
        project_id (str):
        title (str):
        content (str):
        created_at (int | Unset): Optional; server uses current time if omitted or 0.
        updated_at (int | Unset): Optional; server uses created_at if omitted or 0.
        parent_card_id (None | str | Unset):
        sort_key (float | Unset):
        rel_path (None | str | Unset): Optional; when supplied it must be the path Holder derives from the generated
            card ID.
    """

    project_id: str
    title: str
    content: str
    created_at: int | Unset = UNSET
    updated_at: int | Unset = UNSET
    parent_card_id: None | str | Unset = UNSET
    sort_key: float | Unset = UNSET
    rel_path: None | str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        title = self.title

        content = self.content

        created_at = self.created_at

        updated_at = self.updated_at

        parent_card_id: None | str | Unset
        if isinstance(self.parent_card_id, Unset):
            parent_card_id = UNSET
        else:
            parent_card_id = self.parent_card_id

        sort_key = self.sort_key

        rel_path: None | str | Unset
        if isinstance(self.rel_path, Unset):
            rel_path = UNSET
        else:
            rel_path = self.rel_path

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "title": title,
                "content": content,
            }
        )
        if created_at is not UNSET:
            field_dict["created_at"] = created_at
        if updated_at is not UNSET:
            field_dict["updated_at"] = updated_at
        if parent_card_id is not UNSET:
            field_dict["parent_card_id"] = parent_card_id
        if sort_key is not UNSET:
            field_dict["sort_key"] = sort_key
        if rel_path is not UNSET:
            field_dict["rel_path"] = rel_path

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        title = d.pop("title")

        content = d.pop("content")

        created_at = d.pop("created_at", UNSET)

        updated_at = d.pop("updated_at", UNSET)

        def _parse_parent_card_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        parent_card_id = _parse_parent_card_id(d.pop("parent_card_id", UNSET))

        sort_key = d.pop("sort_key", UNSET)

        def _parse_rel_path(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        rel_path = _parse_rel_path(d.pop("rel_path", UNSET))

        card_create_request = cls(
            project_id=project_id,
            title=title,
            content=content,
            created_at=created_at,
            updated_at=updated_at,
            parent_card_id=parent_card_id,
            sort_key=sort_key,
            rel_path=rel_path,
        )

        return card_create_request
