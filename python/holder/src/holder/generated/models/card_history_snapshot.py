from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card_history_version import CardHistoryVersion


T = TypeVar("T", bound="CardHistorySnapshot")


@_attrs_define
class CardHistorySnapshot:
    """A card's state at a selected revision.

    Attributes:
        card_id (str):
        snapshot (CardHistoryVersion): A card's saved state at one commit, including an explicit absent state.
    """

    card_id: str
    snapshot: CardHistoryVersion

    def to_dict(self) -> dict[str, Any]:
        card_id = self.card_id

        snapshot = self.snapshot.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "card_id": card_id,
                "snapshot": snapshot,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card_history_version import CardHistoryVersion

        d = dict(src_dict)
        card_id = d.pop("card_id")

        snapshot = CardHistoryVersion.from_dict(d.pop("snapshot"))

        card_history_snapshot = cls(
            card_id=card_id,
            snapshot=snapshot,
        )

        return card_history_snapshot
