from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="RecoveryTokenExportRequest")


@_attrs_define
class RecoveryTokenExportRequest:
    """PIN used to wrap the project key.

    Attributes:
        pin (str): User-provided PIN used to wrap or unwrap the key.
    """

    pin: str

    def to_dict(self) -> dict[str, Any]:
        pin = self.pin

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "pin": pin,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        pin = d.pop("pin")

        recovery_token_export_request = cls(
            pin=pin,
        )

        return recovery_token_export_request
