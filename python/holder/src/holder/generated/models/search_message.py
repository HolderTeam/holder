from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="SearchMessage")


@_attrs_define
class SearchMessage:
    """An AI message matching a full-text query.

    Attributes:
        message_id (str): The matching message's ID.
        created_at (int): When the message was created.
        snippet (str): A highlighted excerpt from the match.
        rank (float): The FTS relevance score; lower scores rank first.
    """

    message_id: str
    created_at: int
    snippet: str
    rank: float

    def to_dict(self) -> dict[str, Any]:
        message_id = self.message_id

        created_at = self.created_at

        snippet = self.snippet

        rank = self.rank

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "message_id": message_id,
                "created_at": created_at,
                "snippet": snippet,
                "rank": rank,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        message_id = d.pop("message_id")

        created_at = d.pop("created_at")

        snippet = d.pop("snippet")

        rank = d.pop("rank")

        search_message = cls(
            message_id=message_id,
            created_at=created_at,
            snippet=snippet,
            rank=rank,
        )

        return search_message
