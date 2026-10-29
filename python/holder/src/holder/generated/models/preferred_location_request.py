from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="PreferredLocationRequest")


@_attrs_define
class PreferredLocationRequest:
    """Set this device's preferred location for a project.

    Attributes:
        project_id (str): The owning project.
        location_id (str): The location to prefer.
    """

    project_id: str
    location_id: str

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        location_id = self.location_id

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "location_id": location_id,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        location_id = d.pop("location_id")

        preferred_location_request = cls(
            project_id=project_id,
            location_id=location_id,
        )

        return preferred_location_request
