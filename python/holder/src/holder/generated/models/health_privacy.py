from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="HealthPrivacy")


@_attrs_define
class HealthPrivacy:
    """The daemon's project encryption support.

    Attributes:
        backend (str): Encryption used for encrypted projects.
        project_mode_supported (bool): Whether the daemon supports per-project encryption.
    """

    backend: str
    project_mode_supported: bool

    def to_dict(self) -> dict[str, Any]:
        backend = self.backend

        project_mode_supported = self.project_mode_supported

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "backend": backend,
                "project_mode_supported": project_mode_supported,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        backend = d.pop("backend")

        project_mode_supported = d.pop("project_mode_supported")

        health_privacy = cls(
            backend=backend,
            project_mode_supported=project_mode_supported,
        )

        return health_privacy
