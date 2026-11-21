from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="TrashConflictCopyRequest")


@_attrs_define
class TrashConflictCopyRequest:
    """Confirmation of the cleanup effects returned when the resolution was saved.

    Attributes:
        cleanup_token (str): The token returned with the cleanup review.
    """

    cleanup_token: str

    def to_dict(self) -> dict[str, Any]:
        cleanup_token = self.cleanup_token

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "cleanup_token": cleanup_token,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        cleanup_token = d.pop("cleanup_token")

        trash_conflict_copy_request = cls(
            cleanup_token=cleanup_token,
        )

        return trash_conflict_copy_request
