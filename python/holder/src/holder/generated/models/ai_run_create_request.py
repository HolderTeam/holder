from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.ai_run_create_request_context_type_0 import (
        AiRunCreateRequestContextType0,
    )


T = TypeVar("T", bound="AiRunCreateRequest")


@_attrs_define
class AiRunCreateRequest:
    """
    Attributes:
        prompt (str):
        mode (str | Unset): auto or model
        model (None | str | Unset): Optional explicit model id; interpreted for local or cloud selected provider.
        provider (None | str | Unset): Optional cloud provider id when local runner is unavailable.
        project_id (None | str | Unset):
        thread_id (None | str | Unset):
        context (AiRunCreateRequestContextType0 | None | Unset):
        runner_id (None | str | Unset): The local runner to use; defaults to the automatic local runner.
    """

    prompt: str
    mode: str | Unset = UNSET
    model: None | str | Unset = UNSET
    provider: None | str | Unset = UNSET
    project_id: None | str | Unset = UNSET
    thread_id: None | str | Unset = UNSET
    context: AiRunCreateRequestContextType0 | None | Unset = UNSET
    runner_id: None | str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        from ..models.ai_run_create_request_context_type_0 import (
            AiRunCreateRequestContextType0,
        )

        prompt = self.prompt

        mode = self.mode

        model: None | str | Unset
        if isinstance(self.model, Unset):
            model = UNSET
        else:
            model = self.model

        provider: None | str | Unset
        if isinstance(self.provider, Unset):
            provider = UNSET
        else:
            provider = self.provider

        project_id: None | str | Unset
        if isinstance(self.project_id, Unset):
            project_id = UNSET
        else:
            project_id = self.project_id

        thread_id: None | str | Unset
        if isinstance(self.thread_id, Unset):
            thread_id = UNSET
        else:
            thread_id = self.thread_id

        context: dict[str, Any] | None | Unset
        if isinstance(self.context, Unset):
            context = UNSET
        elif isinstance(self.context, AiRunCreateRequestContextType0):
            context = self.context.to_dict()
        else:
            context = self.context

        runner_id: None | str | Unset
        if isinstance(self.runner_id, Unset):
            runner_id = UNSET
        else:
            runner_id = self.runner_id

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "prompt": prompt,
            }
        )
        if mode is not UNSET:
            field_dict["mode"] = mode
        if model is not UNSET:
            field_dict["model"] = model
        if provider is not UNSET:
            field_dict["provider"] = provider
        if project_id is not UNSET:
            field_dict["project_id"] = project_id
        if thread_id is not UNSET:
            field_dict["thread_id"] = thread_id
        if context is not UNSET:
            field_dict["context"] = context
        if runner_id is not UNSET:
            field_dict["runner_id"] = runner_id

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_run_create_request_context_type_0 import (
            AiRunCreateRequestContextType0,
        )

        d = dict(src_dict)
        prompt = d.pop("prompt")

        mode = d.pop("mode", UNSET)

        def _parse_model(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        model = _parse_model(d.pop("model", UNSET))

        def _parse_provider(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        provider = _parse_provider(d.pop("provider", UNSET))

        def _parse_project_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        project_id = _parse_project_id(d.pop("project_id", UNSET))

        def _parse_thread_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        thread_id = _parse_thread_id(d.pop("thread_id", UNSET))

        def _parse_context(
            data: object,
        ) -> AiRunCreateRequestContextType0 | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            try:
                if not isinstance(data, dict):
                    raise TypeError()
                context_type_0 = AiRunCreateRequestContextType0.from_dict(data)

                return context_type_0
            except (TypeError, ValueError, AttributeError, KeyError):
                pass
            return cast(AiRunCreateRequestContextType0 | None | Unset, data)

        context = _parse_context(d.pop("context", UNSET))

        def _parse_runner_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        runner_id = _parse_runner_id(d.pop("runner_id", UNSET))

        ai_run_create_request = cls(
            prompt=prompt,
            mode=mode,
            model=model,
            provider=provider,
            project_id=project_id,
            thread_id=thread_id,
            context=context,
            runner_id=runner_id,
        )

        ai_run_create_request.additional_properties = d
        return ai_run_create_request

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
