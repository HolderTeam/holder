from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="CardHistorySave")


@_attrs_define
class CardHistorySave:
    """One exact commit saved for a card.

    Attributes:
        oid (str): The full commit ID.
        parent_oids (list[str]): The commit's parents.
        authored_at (int): When the commit was authored.
        committed_at (int): When the commit was committed.
        message (str): The commit message.
    """

    oid: str
    parent_oids: list[str]
    authored_at: int
    committed_at: int
    message: str

    def to_dict(self) -> dict[str, Any]:
        oid = self.oid

        parent_oids = self.parent_oids

        authored_at = self.authored_at

        committed_at = self.committed_at

        message = self.message

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "oid": oid,
                "parent_oids": parent_oids,
                "authored_at": authored_at,
                "committed_at": committed_at,
                "message": message,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        oid = d.pop("oid")

        parent_oids = cast(list[str], d.pop("parent_oids"))

        authored_at = d.pop("authored_at")

        committed_at = d.pop("committed_at")

        message = d.pop("message")

        card_history_save = cls(
            oid=oid,
            parent_oids=parent_oids,
            authored_at=authored_at,
            committed_at=committed_at,
            message=message,
        )

        return card_history_save
