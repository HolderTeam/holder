from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="CardHistoryVersion")


@_attrs_define
class CardHistoryVersion:
    """A card's saved state at one commit, including an explicit absent state.

    Attributes:
        exists (bool): Whether the card existed in this snapshot.
        oid (str): The full commit ID.
        title (str): The card's saved title, empty when absent.
        body (str): The card's saved body, empty when absent.
    """

    exists: bool
    oid: str
    title: str
    body: str

    def to_dict(self) -> dict[str, Any]:
        exists = self.exists

        oid = self.oid

        title = self.title

        body = self.body

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "exists": exists,
                "oid": oid,
                "title": title,
                "body": body,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        exists = d.pop("exists")

        oid = d.pop("oid")

        title = d.pop("title")

        body = d.pop("body")

        card_history_version = cls(
            exists=exists,
            oid=oid,
            title=title,
            body=body,
        )

        return card_history_version
