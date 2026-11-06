from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="SearchCard")


@_attrs_define
class SearchCard:
    """A card matching a full-text query.

    Attributes:
        card_id (str): The matching card's ID.
        title (str): The card's current title.
        updated_at (int): When the card was last updated.
        created_at (int): When the card was created.
        snippet (str): A highlighted excerpt from the match.
        rank (float): The FTS relevance score; lower scores rank first.
    """

    card_id: str
    title: str
    updated_at: int
    created_at: int
    snippet: str
    rank: float

    def to_dict(self) -> dict[str, Any]:
        card_id = self.card_id

        title = self.title

        updated_at = self.updated_at

        created_at = self.created_at

        snippet = self.snippet

        rank = self.rank

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "card_id": card_id,
                "title": title,
                "updated_at": updated_at,
                "created_at": created_at,
                "snippet": snippet,
                "rank": rank,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        card_id = d.pop("card_id")

        title = d.pop("title")

        updated_at = d.pop("updated_at")

        created_at = d.pop("created_at")

        snippet = d.pop("snippet")

        rank = d.pop("rank")

        search_card = cls(
            card_id=card_id,
            title=title,
            updated_at=updated_at,
            created_at=created_at,
            snippet=snippet,
            rank=rank,
        )

        return search_card
