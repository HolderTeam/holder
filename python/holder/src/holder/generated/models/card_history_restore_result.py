from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="CardHistoryRestoreResult")


@_attrs_define
class CardHistoryRestoreResult:
    """A card restored from a saved state by creating a new commit.

    Attributes:
        card_id (str):
        restored_from_oid (str): The full commit ID of the selected historical snapshot.
        result_oid (str): The full commit ID created by restoring it.
        title (str): The card's title after restoration.
        updated_at (int): When the restored card was updated.
        deleted_at (int | None): When the restored card is in Trash; null when live.
    """

    card_id: str
    restored_from_oid: str
    result_oid: str
    title: str
    updated_at: int
    deleted_at: int | None

    def to_dict(self) -> dict[str, Any]:
        card_id = self.card_id

        restored_from_oid = self.restored_from_oid

        result_oid = self.result_oid

        title = self.title

        updated_at = self.updated_at

        deleted_at: int | None
        deleted_at = self.deleted_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "card_id": card_id,
                "restored_from_oid": restored_from_oid,
                "result_oid": result_oid,
                "title": title,
                "updated_at": updated_at,
                "deleted_at": deleted_at,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        card_id = d.pop("card_id")

        restored_from_oid = d.pop("restored_from_oid")

        result_oid = d.pop("result_oid")

        title = d.pop("title")

        updated_at = d.pop("updated_at")

        def _parse_deleted_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        deleted_at = _parse_deleted_at(d.pop("deleted_at"))

        card_history_restore_result = cls(
            card_id=card_id,
            restored_from_oid=restored_from_oid,
            result_oid=result_oid,
            title=title,
            updated_at=updated_at,
            deleted_at=deleted_at,
        )

        return card_history_restore_result
