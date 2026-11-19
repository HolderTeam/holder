from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card_summary import CardSummary


T = TypeVar("T", bound="ConflictCopy")


@_attrs_define
class ConflictCopy:
    """A local card version preserved during a divergent pull.

    Attributes:
        original_card_id (str):
        original (CardSummary | None): The original's current title and Trash status, or null if it no longer exists.
        copy_card (CardSummary): Just enough to name a card, as in a reference or a backlink.
    """

    original_card_id: str
    original: CardSummary | None
    copy_card: CardSummary

    def to_dict(self) -> dict[str, Any]:
        from ..models.card_summary import CardSummary

        original_card_id = self.original_card_id

        original: dict[str, Any] | None
        if isinstance(self.original, CardSummary):
            original = self.original.to_dict()
        else:
            original = self.original

        copy_card = self.copy_card.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "original_card_id": original_card_id,
                "original": original,
                "copy_card": copy_card,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card_summary import CardSummary

        d = dict(src_dict)
        original_card_id = d.pop("original_card_id")

        def _parse_original(data: object) -> CardSummary | None:
            if data is None:
                return data
            try:
                if not isinstance(data, dict):
                    raise TypeError()
                original_type_0 = CardSummary.from_dict(data)

                return original_type_0
            except (TypeError, ValueError, AttributeError, KeyError):
                pass
            return cast(CardSummary | None, data)

        original = _parse_original(d.pop("original"))

        copy_card = CardSummary.from_dict(d.pop("copy_card"))

        conflict_copy = cls(
            original_card_id=original_card_id,
            original=original,
            copy_card=copy_card,
        )

        return conflict_copy
