from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="ByeResponse")


@_attrs_define
class ByeResponse:
    """Acknowledgement that a client is leaving.

    Attributes:
        ok (bool): The hint was received.
    """

    ok: bool

    def to_dict(self) -> dict[str, Any]:
        ok = self.ok

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "ok": ok,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        ok = d.pop("ok")

        bye_response = cls(
            ok=ok,
        )

        return bye_response
