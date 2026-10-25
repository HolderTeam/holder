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
    """Named run events share run_id. Fields vary by event and provider.

    Attributes:
        run_id (str):
        status (str | Unset):
        created_at (int | Unset):
        delta (str | Unset):
        message (str | Unset):
        error (str | Unset):
        runner_id (str | Unset):
        model_ref (str | Unset):
        model (str | Unset):
        provider (str | Unset):
        context_compacted (bool | Unset): On a cloud progress event: whether the thread was summarized to fit the
            model's context.
        cooldown_until (int | Unset): On a cloud fallback: when the failed model may be tried again, in epoch seconds.
        router_model (str | Unset): On router_result: the model that ranked the candidates.
        ranked (list[str] | Unset): On router_result: the candidate models, best first.
        partial (bool | Unset): On a local fallback: whether the failed model had already streamed some output.
    """

    run_id: str
    status: str | Unset = UNSET
    created_at: int | Unset = UNSET
    delta: str | Unset = UNSET
    message: str | Unset = UNSET
    error: str | Unset = UNSET
    runner_id: str | Unset = UNSET
    model_ref: str | Unset = UNSET
    model: str | Unset = UNSET
    provider: str | Unset = UNSET
    context_compacted: bool | Unset = UNSET
    cooldown_until: int | Unset = UNSET
    router_model: str | Unset = UNSET
    ranked: list[str] | Unset = UNSET
    partial: bool | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        run_id = self.run_id

        status = self.status

        created_at = self.created_at

        delta = self.delta

        message = self.message

        error = self.error

        runner_id = self.runner_id

        model_ref = self.model_ref

        model = self.model

        provider = self.provider

        context_compacted = self.context_compacted

        cooldown_until = self.cooldown_until

        router_model = self.router_model

        ranked: list[str] | Unset = UNSET
        if not isinstance(self.ranked, Unset):
            ranked = self.ranked

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

        status = d.pop("status", UNSET)

        created_at = d.pop("created_at", UNSET)

        delta = d.pop("delta", UNSET)

        message = d.pop("message", UNSET)

        error = d.pop("error", UNSET)

        runner_id = d.pop("runner_id", UNSET)

        model_ref = d.pop("model_ref", UNSET)

        model = d.pop("model", UNSET)

        provider = d.pop("provider", UNSET)

        context_compacted = d.pop("context_compacted", UNSET)

        cooldown_until = d.pop("cooldown_until", UNSET)

        router_model = d.pop("router_model", UNSET)

        ranked = cast(list[str], d.pop("ranked", UNSET))

        partial = d.pop("partial", UNSET)

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
