from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="GoogleDriveAuthorization")


@_attrs_define
class GoogleDriveAuthorization:
    """Where to send the user to authorize Google Drive.

    Attributes:
        authorization_url (str): The Google authorization URL.
    """

    authorization_url: str

    def to_dict(self) -> dict[str, Any]:
        authorization_url = self.authorization_url

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "authorization_url": authorization_url,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        authorization_url = d.pop("authorization_url")

        google_drive_authorization = cls(
            authorization_url=authorization_url,
        )

        return google_drive_authorization
