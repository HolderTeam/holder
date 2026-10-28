from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="HistoryAffectedPath")


@_attrs_define
class HistoryAffectedPath:
    """A changed path with any display details found in its historical version.

    Attributes:
        path (str): The path relative to the project root.
        title (str | Unset): A historical title, when available.
        detail (str | Unset): A historical detail, when available.
    """

    path: str
    title: str | Unset = UNSET
    detail: str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        path = self.path

        title = self.title

        detail = self.detail

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "path": path,
            }
        )
        if title is not UNSET:
            field_dict["title"] = title
        if detail is not UNSET:
            field_dict["detail"] = detail

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        path = d.pop("path")

        title = d.pop("title", UNSET)

        detail = d.pop("detail", UNSET)

        history_affected_path = cls(
            path=path,
            title=title,
            detail=detail,
        )

        return history_affected_path
