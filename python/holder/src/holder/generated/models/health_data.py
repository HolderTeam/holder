from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.health_privacy import HealthPrivacy


T = TypeVar("T", bound="HealthData")


@_attrs_define
class HealthData:
    """Current health and version information.

    Attributes:
        db_ok (bool): Whether the database is available.
        uptime_ms (int): Milliseconds since the HTTP server started.
        api_version (str): The API version.
        server_version (str): The daemon version.
        pid (int): The daemon process ID.
        privacy (HealthPrivacy): The daemon's project encryption support.
    """

    db_ok: bool
    uptime_ms: int
    api_version: str
    server_version: str
    pid: int
    privacy: HealthPrivacy

    def to_dict(self) -> dict[str, Any]:
        db_ok = self.db_ok

        uptime_ms = self.uptime_ms

        api_version = self.api_version

        server_version = self.server_version

        pid = self.pid

        privacy = self.privacy.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "db_ok": db_ok,
                "uptime_ms": uptime_ms,
                "api_version": api_version,
                "server_version": server_version,
                "pid": pid,
                "privacy": privacy,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.health_privacy import HealthPrivacy

        d = dict(src_dict)
        db_ok = d.pop("db_ok")

        uptime_ms = d.pop("uptime_ms")

        api_version = d.pop("api_version")

        server_version = d.pop("server_version")

        pid = d.pop("pid")

        privacy = HealthPrivacy.from_dict(d.pop("privacy"))

        health_data = cls(
            db_ok=db_ok,
            uptime_ms=uptime_ms,
            api_version=api_version,
            server_version=server_version,
            pid=pid,
            privacy=privacy,
        )

        return health_data
