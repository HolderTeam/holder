from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.card_move_intent import CardMoveIntent
from ..types import UNSET, Unset

T = TypeVar("T", bound="CardMove")


@_attrs_define
class CardMove:
    """Where to move a card, relative to another card or among its siblings.

    Attributes:
        intent (CardMoveIntent): into makes the card target_card_id's last child; before and after place it beside
            target_card_id, as its
            sibling; left and right swap it with its previous or next sibling; to_start and to_end make it the first or
            last of its siblings; up_level makes it its grandparent's last child.
        target_card_id (str | Unset):
        parent_card_id (None | str | Unset): For left, right, to_start and to_end: the parent among whose children to
            place it, null for the root
            cards; omit for its current parent. Ignored by the other intents.
    """

    intent: CardMoveIntent
    target_card_id: str | Unset = UNSET
    parent_card_id: None | str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        intent = self.intent.value

        target_card_id = self.target_card_id

        parent_card_id: None | str | Unset
        if isinstance(self.parent_card_id, Unset):
            parent_card_id = UNSET
        else:
            parent_card_id = self.parent_card_id

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "intent": intent,
            }
        )
        if target_card_id is not UNSET:
            field_dict["target_card_id"] = target_card_id
        if parent_card_id is not UNSET:
            field_dict["parent_card_id"] = parent_card_id

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        intent = CardMoveIntent(d.pop("intent"))

        target_card_id = d.pop("target_card_id", UNSET)

        def _parse_parent_card_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        parent_card_id = _parse_parent_card_id(d.pop("parent_card_id", UNSET))

        card_move = cls(
            intent=intent,
            target_card_id=target_card_id,
            parent_card_id=parent_card_id,
        )

        return card_move
