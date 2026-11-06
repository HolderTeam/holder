from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="RemoteTest")


@_attrs_define
class RemoteTest:
    """A remote to test.

    Attributes:
        remote_url (str | Unset):
    """

    remote_url: str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        remote_url = self.remote_url

        field_dict: dict[str, Any] = {}

        field_dict.update({})
        if remote_url is not UNSET:
            field_dict["remote_url"] = remote_url

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        remote_url = d.pop("remote_url", UNSET)

        remote_test = cls(
            remote_url=remote_url,
        )

        return remote_test
