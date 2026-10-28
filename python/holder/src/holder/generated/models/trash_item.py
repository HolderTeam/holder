from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.trash_item_type import TrashItemType

T = TypeVar("T", bound="TrashItem")


@_attrs_define
class TrashItem:
    """A trashed card or AI message.

    Attributes:
        type_ (TrashItemType):
        project_id (str): The project that owns the item.
        card_id (None | str): The card ID, present for a card.
        message_id (None | str): The message ID, present for an AI message.
        thread_id (None | str): The AI thread containing a message.
        title (None | str): The card title, if this is a card.
        role (None | str): The AI message role, if this is a message.
        deleted_at (int): When the item entered Trash.
        created_at (int | None): When the AI message was created.
        rel_path (None | str): The card's relative file path.
    """

    type_: TrashItemType
    project_id: str
    card_id: None | str
    message_id: None | str
    thread_id: None | str
    title: None | str
    role: None | str
    deleted_at: int
    created_at: int | None
    rel_path: None | str

    def to_dict(self) -> dict[str, Any]:
        type_ = self.type_.value

        project_id = self.project_id

        card_id: None | str
        card_id = self.card_id

        message_id: None | str
        message_id = self.message_id

        thread_id: None | str
        thread_id = self.thread_id

        title: None | str
        title = self.title

        role: None | str
        role = self.role

        deleted_at = self.deleted_at

        created_at: int | None
        created_at = self.created_at

        rel_path: None | str
        rel_path = self.rel_path

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "type": type_,
                "project_id": project_id,
                "card_id": card_id,
                "message_id": message_id,
                "thread_id": thread_id,
                "title": title,
                "role": role,
                "deleted_at": deleted_at,
                "created_at": created_at,
                "rel_path": rel_path,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        type_ = TrashItemType(d.pop("type"))

        project_id = d.pop("project_id")

        def _parse_card_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        card_id = _parse_card_id(d.pop("card_id"))

        def _parse_message_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        message_id = _parse_message_id(d.pop("message_id"))

        def _parse_thread_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        thread_id = _parse_thread_id(d.pop("thread_id"))

        def _parse_title(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        title = _parse_title(d.pop("title"))

        def _parse_role(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        role = _parse_role(d.pop("role"))

        deleted_at = d.pop("deleted_at")

        def _parse_created_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        created_at = _parse_created_at(d.pop("created_at"))

        def _parse_rel_path(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        rel_path = _parse_rel_path(d.pop("rel_path"))

        trash_item = cls(
            type_=type_,
            project_id=project_id,
            card_id=card_id,
            message_id=message_id,
            thread_id=thread_id,
            title=title,
            role=role,
            deleted_at=deleted_at,
            created_at=created_at,
            rel_path=rel_path,
        )

        return trash_item
