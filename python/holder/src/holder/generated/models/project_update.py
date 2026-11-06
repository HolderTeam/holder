from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="ProjectUpdate")


@_attrs_define
class ProjectUpdate:
    """A change to a project's settings. Each field left out keeps its value.

    Attributes:
        name (str | Unset):
        git_remote_url (None | str | Unset): The Git remote to sync with; null for none.
        git_provider (None | str | Unset): Who hosts the Git remote; null when unknown.
    """

    name: str | Unset = UNSET
    git_remote_url: None | str | Unset = UNSET
    git_provider: None | str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        name = self.name

        git_remote_url: None | str | Unset
        if isinstance(self.git_remote_url, Unset):
            git_remote_url = UNSET
        else:
            git_remote_url = self.git_remote_url

        git_provider: None | str | Unset
        if isinstance(self.git_provider, Unset):
            git_provider = UNSET
        else:
            git_provider = self.git_provider

        field_dict: dict[str, Any] = {}

        field_dict.update({})
        if name is not UNSET:
            field_dict["name"] = name
        if git_remote_url is not UNSET:
            field_dict["git_remote_url"] = git_remote_url
        if git_provider is not UNSET:
            field_dict["git_provider"] = git_provider

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        name = d.pop("name", UNSET)

        def _parse_git_remote_url(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        git_remote_url = _parse_git_remote_url(d.pop("git_remote_url", UNSET))

        def _parse_git_provider(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        git_provider = _parse_git_provider(d.pop("git_provider", UNSET))

        project_update = cls(
            name=name,
            git_remote_url=git_remote_url,
            git_provider=git_provider,
        )

        return project_update
