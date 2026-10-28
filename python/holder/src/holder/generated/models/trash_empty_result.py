from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="TrashEmptyResult")


@_attrs_define
class TrashEmptyResult:
    """The project whose Trash was emptied.

    Attributes:
        project_id (str): The requested project, including when it had no trashed items.
    """

    project_id: str

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        trash_empty_result = cls(
            project_id=project_id,
        )

        return trash_empty_result
