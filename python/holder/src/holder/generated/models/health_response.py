from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.health_data import HealthData


T = TypeVar("T", bound="HealthResponse")


@_attrs_define
class HealthResponse:
    """The daemon's health response.

    Attributes:
        ok (bool): Whether the request succeeded.
        data (HealthData): Current health and version information.
    """

    ok: bool
    data: HealthData

    def to_dict(self) -> dict[str, Any]:
        ok = self.ok

        data = self.data.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "ok": ok,
                "data": data,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.health_data import HealthData

        d = dict(src_dict)
        ok = d.pop("ok")

        data = HealthData.from_dict(d.pop("data"))

        health_response = cls(
            ok=ok,
            data=data,
        )

        return health_response
