from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.privacy_mode import PrivacyMode
from ..types import UNSET, Unset

T = TypeVar("T", bound="NewProject")


@_attrs_define
class NewProject:
    """A project to make.

    Attributes:
        name (str):
        project_id (str | Unset):
        root_path (str | Unset): The directory for the project's repository; omit for one under this device's projects
            directory, named
            for the project.
        git_remote_url (None | str | Unset): The Git remote to sync with; null or omitted for none. One Git does not
            accept is an invalid argument.
        git_provider (None | str | Unset): Who hosts the Git remote; null or omitted when unknown.
        privacy_mode (PrivacyMode | Unset):
        created_at (int | Unset): When the project was made, for imports, after the epoch; omit for now.
        updated_at (int | Unset): When the project's settings last changed, for imports, after the epoch; omit for when
            it was made.
    """

    name: str
    project_id: str | Unset = UNSET
    root_path: str | Unset = UNSET
    git_remote_url: None | str | Unset = UNSET
    git_provider: None | str | Unset = UNSET
    privacy_mode: PrivacyMode | Unset = UNSET
    created_at: int | Unset = UNSET
    updated_at: int | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        name = self.name

        project_id = self.project_id

        root_path = self.root_path

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

        privacy_mode: str | Unset = UNSET
        if not isinstance(self.privacy_mode, Unset):
            privacy_mode = self.privacy_mode.value

        created_at = self.created_at

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "name": name,
            }
        )
        if project_id is not UNSET:
            field_dict["project_id"] = project_id
        if root_path is not UNSET:
            field_dict["root_path"] = root_path
        if git_remote_url is not UNSET:
            field_dict["git_remote_url"] = git_remote_url
        if git_provider is not UNSET:
            field_dict["git_provider"] = git_provider
        if privacy_mode is not UNSET:
            field_dict["privacy_mode"] = privacy_mode
        if created_at is not UNSET:
            field_dict["created_at"] = created_at
        if updated_at is not UNSET:
            field_dict["updated_at"] = updated_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        name = d.pop("name")

        project_id = d.pop("project_id", UNSET)

        root_path = d.pop("root_path", UNSET)

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

        _privacy_mode = d.pop("privacy_mode", UNSET)
        privacy_mode: PrivacyMode | Unset
        if isinstance(_privacy_mode, Unset):
            privacy_mode = UNSET
        else:
            privacy_mode = PrivacyMode(_privacy_mode)

        created_at = d.pop("created_at", UNSET)

        updated_at = d.pop("updated_at", UNSET)

        new_project = cls(
            name=name,
            project_id=project_id,
            root_path=root_path,
            git_remote_url=git_remote_url,
            git_provider=git_provider,
            privacy_mode=privacy_mode,
            created_at=created_at,
            updated_at=updated_at,
        )

        return new_project
