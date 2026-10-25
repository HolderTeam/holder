from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

T = TypeVar("T", bound="AiMessage")


@_attrs_define
class AiMessage:
    """
    Attributes:
        message_id (str):
        thread_id (str):
        role (str):
        source (str):
        provider (None | str):
        model (None | str):
        content (str):
        created_at (int):
        deleted_at (int | None):
        prompt_hash (None | str):
        meta_json (None | str):
    """

    message_id: str
    thread_id: str
    role: str
    source: str
    provider: None | str
    model: None | str
    content: str
    created_at: int
    deleted_at: int | None
    prompt_hash: None | str
    meta_json: None | str
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        message_id = self.message_id

        thread_id = self.thread_id

        role = self.role

        source = self.source

        provider: None | str
        provider = self.provider

        model: None | str
        model = self.model

        content = self.content

        created_at = self.created_at

        deleted_at: int | None
        deleted_at = self.deleted_at

        prompt_hash: None | str
        prompt_hash = self.prompt_hash

        meta_json: None | str
        meta_json = self.meta_json

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "message_id": message_id,
                "thread_id": thread_id,
                "role": role,
                "source": source,
                "provider": provider,
                "model": model,
                "content": content,
                "created_at": created_at,
                "deleted_at": deleted_at,
                "prompt_hash": prompt_hash,
                "meta_json": meta_json,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        message_id = d.pop("message_id")

        thread_id = d.pop("thread_id")

        role = d.pop("role")

        source = d.pop("source")

        def _parse_provider(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        provider = _parse_provider(d.pop("provider"))

        def _parse_model(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        model = _parse_model(d.pop("model"))

        content = d.pop("content")

        created_at = d.pop("created_at")

        def _parse_deleted_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        deleted_at = _parse_deleted_at(d.pop("deleted_at"))

        def _parse_prompt_hash(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        prompt_hash = _parse_prompt_hash(d.pop("prompt_hash"))

        def _parse_meta_json(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        meta_json = _parse_meta_json(d.pop("meta_json"))

        ai_message = cls(
            message_id=message_id,
            thread_id=thread_id,
            role=role,
            source=source,
            provider=provider,
            model=model,
            content=content,
            created_at=created_at,
            deleted_at=deleted_at,
            prompt_hash=prompt_hash,
            meta_json=meta_json,
        )

        ai_message.additional_properties = d
        return ai_message

    @property
    def additional_keys(self) -> list[str]:
        return list(self.additional_properties.keys())

    def __getitem__(self, key: str) -> Any:
        return self.additional_properties[key]

    def __setitem__(self, key: str, value: Any) -> None:
        self.additional_properties[key] = value

    def __delitem__(self, key: str) -> None:
        del self.additional_properties[key]

    def __contains__(self, key: str) -> bool:
        return key in self.additional_properties
