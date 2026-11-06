from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.google_drive_authorization import GoogleDriveAuthorization


T = TypeVar("T", bound="GoogleDriveAuthorizeResponse")


@_attrs_define
class GoogleDriveAuthorizeResponse:
    """The URL to start Google Drive authorization.

    Attributes:
        ok (bool): Whether the request succeeded.
        data (GoogleDriveAuthorization): Where to send the user to authorize Google Drive.
    """

    ok: bool
    data: GoogleDriveAuthorization

    def to_dict(self) -> dict[str, Any]:
        ok = self.ok

        data = self.data.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "ok": ok,
                "data": data,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.google_drive_authorization import (
            GoogleDriveAuthorization,
        )

        d = dict(src_dict)
        ok = d.pop("ok")

        data = GoogleDriveAuthorization.from_dict(d.pop("data"))

        google_drive_authorize_response = cls(
            ok=ok,
            data=data,
        )

        return google_drive_authorize_response
