from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="TrashDeleteResult")


@_attrs_define
class TrashDeleteResult:
    """The ID of a permanently removed item.

    Attributes:
        card_id (None | str): The removed card's ID, or null for a message.
        message_id (None | str): The removed AI message's ID, or null for a card.
    """

    card_id: None | str
    message_id: None | str

    def to_dict(self) -> dict[str, Any]:
        card_id: None | str
        card_id = self.card_id

        message_id: None | str
        message_id = self.message_id

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "card_id": card_id,
                "message_id": message_id,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)

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

        trash_delete_result = cls(
            card_id=card_id,
            message_id=message_id,
        )

        return trash_delete_result
