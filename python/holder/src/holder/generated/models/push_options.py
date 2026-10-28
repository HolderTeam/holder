from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="PushOptions")


@_attrs_define
class PushOptions:
    """How to push.

    Attributes:
        branch (str | Unset): The branch to push; omit for the repository's default.
        set_upstream (bool | Unset): Whether the remote branch becomes the local branch's upstream. Default: True.
    """

    branch: str | Unset = UNSET
    set_upstream: bool | Unset = True

    def to_dict(self) -> dict[str, Any]:
        branch = self.branch

        set_upstream = self.set_upstream

        field_dict: dict[str, Any] = {}

        field_dict.update({})
        if branch is not UNSET:
            field_dict["branch"] = branch
        if set_upstream is not UNSET:
            field_dict["set_upstream"] = set_upstream

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        branch = d.pop("branch", UNSET)

        set_upstream = d.pop("set_upstream", UNSET)

        push_options = cls(
            branch=branch,
            set_upstream=set_upstream,
        )

        return push_options
