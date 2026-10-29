from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.privacy_mode import PrivacyMode

if TYPE_CHECKING:
    from ..models.encryption_safety_check import EncryptionSafetyCheck


T = TypeVar("T", bound="ProjectEncryptionCheck")


@_attrs_define
class ProjectEncryptionCheck:
    """A project's privacy mode and the result of checking its files.

    Attributes:
        project_id (str):
        privacy_mode (PrivacyMode):
        check (EncryptionSafetyCheck): Whether an encrypted project's card files are safe to push.
    """

    project_id: str
    privacy_mode: PrivacyMode
    check: EncryptionSafetyCheck

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        privacy_mode = self.privacy_mode.value

        check = self.check.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "privacy_mode": privacy_mode,
                "check": check,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.encryption_safety_check import (
            EncryptionSafetyCheck,
        )

        d = dict(src_dict)
        project_id = d.pop("project_id")

        privacy_mode = PrivacyMode(d.pop("privacy_mode"))

        check = EncryptionSafetyCheck.from_dict(d.pop("check"))

        project_encryption_check = cls(
            project_id=project_id,
            privacy_mode=privacy_mode,
            check=check,
        )

        return project_encryption_check
