from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.privacy_mode import PrivacyMode
from ..types import UNSET, Unset

T = TypeVar("T", bound="ProjectListItem")


@_attrs_define
class ProjectListItem:
    """A project in a listing.

    Attributes:
        project_id (str):
        name (str):
        root_path (str): The directory that holds the project's repository on this device.
        git_remote_url (None | str): The Git remote the project syncs with; null when it has none.
        git_provider (None | str): Who hosts the Git remote, such as github; null when unknown.
        privacy_mode (PrivacyMode):
        project_key_id (None | str): The ID of an encrypted project's key; null for a plain project.
        created_at (int): When the project was made.
        updated_at (int): When the project's settings last changed.
        card_count (int | Unset): The project's live cards; present when asked for.
        root_card_count (int | Unset): The project's live root cards; present when asked for.
    """

    project_id: str
    name: str
    root_path: str
    git_remote_url: None | str
    git_provider: None | str
    privacy_mode: PrivacyMode
    project_key_id: None | str
    created_at: int
    updated_at: int
    card_count: int | Unset = UNSET
    root_card_count: int | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        name = self.name

        root_path = self.root_path

        git_remote_url: None | str
        git_remote_url = self.git_remote_url

        git_provider: None | str
        git_provider = self.git_provider

        privacy_mode = self.privacy_mode.value

        project_key_id: None | str
        project_key_id = self.project_key_id

        created_at = self.created_at

        updated_at = self.updated_at

        card_count = self.card_count

        root_card_count = self.root_card_count

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "name": name,
                "root_path": root_path,
                "git_remote_url": git_remote_url,
                "git_provider": git_provider,
                "privacy_mode": privacy_mode,
                "project_key_id": project_key_id,
                "created_at": created_at,
                "updated_at": updated_at,
            }
        )
        if card_count is not UNSET:
            field_dict["card_count"] = card_count
        if root_card_count is not UNSET:
            field_dict["root_card_count"] = root_card_count

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        name = d.pop("name")

        root_path = d.pop("root_path")

        def _parse_git_remote_url(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        git_remote_url = _parse_git_remote_url(d.pop("git_remote_url"))

        def _parse_git_provider(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        git_provider = _parse_git_provider(d.pop("git_provider"))

        privacy_mode = PrivacyMode(d.pop("privacy_mode"))

        def _parse_project_key_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        project_key_id = _parse_project_key_id(d.pop("project_key_id"))

        created_at = d.pop("created_at")

        updated_at = d.pop("updated_at")

        card_count = d.pop("card_count", UNSET)

        root_card_count = d.pop("root_card_count", UNSET)

        project_list_item = cls(
            project_id=project_id,
            name=name,
            root_path=root_path,
            git_remote_url=git_remote_url,
            git_provider=git_provider,
            privacy_mode=privacy_mode,
            project_key_id=project_key_id,
            created_at=created_at,
            updated_at=updated_at,
            card_count=card_count,
            root_card_count=root_card_count,
        )

        return project_list_item
