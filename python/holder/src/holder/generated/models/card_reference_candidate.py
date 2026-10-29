from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="CardReferenceCandidate")


@_attrs_define
class CardReferenceCandidate:
    """A card that matches a reference.

    Attributes:
        card_id (str):
        title (str): The card's title.
        deleted_at (int | None): When the card entered Trash, or null while live.
    """

    card_id: str
    title: str
    deleted_at: int | None

    def to_dict(self) -> dict[str, Any]:
        card_id = self.card_id

        title = self.title

        deleted_at: int | None
        deleted_at = self.deleted_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "card_id": card_id,
                "title": title,
                "deleted_at": deleted_at,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        card_id = d.pop("card_id")

        title = d.pop("title")

        def _parse_deleted_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        deleted_at = _parse_deleted_at(d.pop("deleted_at"))

        card_reference_candidate = cls(
            card_id=card_id,
            title=title,
            deleted_at=deleted_at,
        )

        return card_reference_candidate
