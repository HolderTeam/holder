from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="AiRunEventData")


@_attrs_define
class AiRunEventData:
    """Named run events share run_id. Fields vary by event and provider..

    Attributes:
        run_id (str): The run id.
        status (None | str | Unset): The status.
        created_at (int | None | Unset): The created at.
        delta (None | str | Unset): The delta.
        message (None | str | Unset): The message.
        error (None | str | Unset): The error.
        runner_id (None | str | Unset): The runner id.
        model_ref (None | str | Unset): The model ref.
        model (None | str | Unset): The model.
        provider (None | str | Unset): The provider.
        context_compacted (bool | None | Unset): On a cloud progress event: whether the thread was summarized to fit the
            model's context.
        cooldown_until (int | None | Unset): On a cloud fallback: when the failed model may be tried again, in epoch
            seconds.
        router_model (None | str | Unset): On router_result: the model that ranked the candidates.
        ranked (list[str] | None | Unset): On router_result: the candidate models, best first.
        partial (bool | None | Unset): On a local fallback: whether the failed model had already streamed some output.
    """

    run_id: str
    status: None | str | Unset = UNSET
    created_at: int | None | Unset = UNSET
    delta: None | str | Unset = UNSET
    message: None | str | Unset = UNSET
    error: None | str | Unset = UNSET
    runner_id: None | str | Unset = UNSET
    model_ref: None | str | Unset = UNSET
    model: None | str | Unset = UNSET
    provider: None | str | Unset = UNSET
    context_compacted: bool | None | Unset = UNSET
    cooldown_until: int | None | Unset = UNSET
    router_model: None | str | Unset = UNSET
    ranked: list[str] | None | Unset = UNSET
    partial: bool | None | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        run_id = self.run_id

        status: None | str | Unset
        if isinstance(self.status, Unset):
            status = UNSET
        else:
            status = self.status

        created_at: int | None | Unset
        if isinstance(self.created_at, Unset):
            created_at = UNSET
        else:
            created_at = self.created_at

        delta: None | str | Unset
        if isinstance(self.delta, Unset):
            delta = UNSET
        else:
            delta = self.delta

        message: None | str | Unset
        if isinstance(self.message, Unset):
            message = UNSET
        else:
            message = self.message

        error: None | str | Unset
        if isinstance(self.error, Unset):
            error = UNSET
        else:
            error = self.error

        runner_id: None | str | Unset
        if isinstance(self.runner_id, Unset):
            runner_id = UNSET
        else:
            runner_id = self.runner_id

        model_ref: None | str | Unset
        if isinstance(self.model_ref, Unset):
            model_ref = UNSET
        else:
            model_ref = self.model_ref

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

        context_compacted: bool | None | Unset
        if isinstance(self.context_compacted, Unset):
            context_compacted = UNSET
        else:
            context_compacted = self.context_compacted

        cooldown_until: int | None | Unset
        if isinstance(self.cooldown_until, Unset):
            cooldown_until = UNSET
        else:
            cooldown_until = self.cooldown_until

        router_model: None | str | Unset
        if isinstance(self.router_model, Unset):
            router_model = UNSET
        else:
            router_model = self.router_model

        ranked: list[str] | None | Unset
        if isinstance(self.ranked, Unset):
            ranked = UNSET
        elif isinstance(self.ranked, list):
            ranked = self.ranked

        else:
            ranked = self.ranked

        partial: bool | None | Unset
        if isinstance(self.partial, Unset):
            partial = UNSET
        else:
            partial = self.partial

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "run_id": run_id,
            }
        )
        if status is not UNSET:
            field_dict["status"] = status
        if created_at is not UNSET:
            field_dict["created_at"] = created_at
        if delta is not UNSET:
            field_dict["delta"] = delta
        if message is not UNSET:
            field_dict["message"] = message
        if error is not UNSET:
            field_dict["error"] = error
        if runner_id is not UNSET:
            field_dict["runner_id"] = runner_id
        if model_ref is not UNSET:
            field_dict["model_ref"] = model_ref
        if model is not UNSET:
            field_dict["model"] = model
        if provider is not UNSET:
            field_dict["provider"] = provider
        if context_compacted is not UNSET:
            field_dict["context_compacted"] = context_compacted
        if cooldown_until is not UNSET:
            field_dict["cooldown_until"] = cooldown_until
        if router_model is not UNSET:
            field_dict["router_model"] = router_model
        if ranked is not UNSET:
            field_dict["ranked"] = ranked
        if partial is not UNSET:
            field_dict["partial"] = partial

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        run_id = d.pop("run_id")

        def _parse_status(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        status = _parse_status(d.pop("status", UNSET))

        def _parse_created_at(data: object) -> int | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(int | None | Unset, data)

        created_at = _parse_created_at(d.pop("created_at", UNSET))

        def _parse_delta(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        delta = _parse_delta(d.pop("delta", UNSET))

        def _parse_message(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        message = _parse_message(d.pop("message", UNSET))

        def _parse_error(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        error = _parse_error(d.pop("error", UNSET))

        def _parse_runner_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        runner_id = _parse_runner_id(d.pop("runner_id", UNSET))

        def _parse_model_ref(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        model_ref = _parse_model_ref(d.pop("model_ref", UNSET))

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

        def _parse_context_compacted(data: object) -> bool | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(bool | None | Unset, data)

        context_compacted = _parse_context_compacted(d.pop("context_compacted", UNSET))

        def _parse_cooldown_until(data: object) -> int | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(int | None | Unset, data)

        cooldown_until = _parse_cooldown_until(d.pop("cooldown_until", UNSET))

        def _parse_router_model(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        router_model = _parse_router_model(d.pop("router_model", UNSET))

        def _parse_ranked(data: object) -> list[str] | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            try:
                if not isinstance(data, list):
                    raise TypeError()
                ranked_type_0 = cast(list[str], data)

                return ranked_type_0
            except (TypeError, ValueError, AttributeError, KeyError):
                pass
            return cast(list[str] | None | Unset, data)

        ranked = _parse_ranked(d.pop("ranked", UNSET))

        def _parse_partial(data: object) -> bool | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(bool | None | Unset, data)

        partial = _parse_partial(d.pop("partial", UNSET))

        ai_run_event_data = cls(
            run_id=run_id,
            status=status,
            created_at=created_at,
            delta=delta,
            message=message,
            error=error,
            runner_id=runner_id,
            model_ref=model_ref,
            model=model,
            provider=provider,
            context_compacted=context_compacted,
            cooldown_until=cooldown_until,
            router_model=router_model,
            ranked=ranked,
            partial=partial,
        )

        ai_run_event_data.additional_properties = d
        return ai_run_event_data

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
