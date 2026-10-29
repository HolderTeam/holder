from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="RecoveryTokenImportRequest")


@_attrs_define
class RecoveryTokenImportRequest:
    """PIN and serialized .hrk token to restore.

    Attributes:
        pin (str): User-provided PIN used to wrap or unwrap the key.
        recovery_token (str): Serialized .hrk token JSON.
    """

    pin: str
    recovery_token: str

    def to_dict(self) -> dict[str, Any]:
        pin = self.pin

        recovery_token = self.recovery_token

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "pin": pin,
                "recovery_token": recovery_token,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        pin = d.pop("pin")

        recovery_token = d.pop("recovery_token")

        recovery_token_import_request = cls(
            pin=pin,
            recovery_token=recovery_token,
        )

        return recovery_token_import_request
