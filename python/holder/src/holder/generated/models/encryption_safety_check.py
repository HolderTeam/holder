from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="EncryptionSafetyCheck")


@_attrs_define
class EncryptionSafetyCheck:
    """Whether an encrypted project's card files are safe to push.

    Attributes:
        ok (bool): Whether every checked file passed.
        checked_files (int): Card files examined.
        unsafe_files (int): Card files that failed the safety check.
        unsafe_paths (list[str]): Project-relative paths of unsafe files.
        message (str): A short explanation of the result.
    """

    ok: bool
    checked_files: int
    unsafe_files: int
    unsafe_paths: list[str]
    message: str

    def to_dict(self) -> dict[str, Any]:
        ok = self.ok

        checked_files = self.checked_files

        unsafe_files = self.unsafe_files

        unsafe_paths = self.unsafe_paths

        message = self.message

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "ok": ok,
                "checked_files": checked_files,
                "unsafe_files": unsafe_files,
                "unsafe_paths": unsafe_paths,
                "message": message,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        ok = d.pop("ok")

        checked_files = d.pop("checked_files")

        unsafe_files = d.pop("unsafe_files")

        unsafe_paths = cast(list[str], d.pop("unsafe_paths"))

        message = d.pop("message")

        encryption_safety_check = cls(
            ok=ok,
            checked_files=checked_files,
            unsafe_files=unsafe_files,
            unsafe_paths=unsafe_paths,
            message=message,
        )

        return encryption_safety_check
