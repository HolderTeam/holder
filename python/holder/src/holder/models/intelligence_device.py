"""Device-wide intelligence providers, runners, and local models."""

from __future__ import annotations

from dataclasses import dataclass, field, fields
from typing import TYPE_CHECKING, Any, Self

from ..collections import Collection
from ..exceptions import NotFoundError
from ..generated.api.default import (
    delete_ai_providers_credentials_provider,
    delete_ai_providers_settings_provider,
    delete_ai_runners_runner_id,
    get_ai_capabilities,
    get_ai_local_models_config,
    get_ai_providers_catalog,
    get_ai_runner_pull_job_id,
    get_ai_runners,
    get_ai_runners_runner_id,
    get_ai_status,
    patch_ai_runners_runner_id,
    post_ai_runner_pull,
    post_ai_runners,
    post_ai_runners_runner_id_retry,
    put_ai_local_models_config,
    put_ai_providers_credentials,
    put_ai_providers_settings,
)
from ..generated.models import (
    AiLocalModelConfigUpdateRequest,
    AiProviderCredentialUpsertRequest,
    AiProviderSettingUpdateRequest,
    AiRunnerCreateRequest,
    AiRunnerCreateRequestKind,
    AiRunnerUpdateRequest,
    PostAiRunnerPullBody,
)
from ..generated.types import UNSET, Unset

if TYPE_CHECKING:
    from ..client import Holder
    from ..intelligence_events import PullEventStream
    from .project import Project


def _optional(value: Any) -> Any:
    return None if isinstance(value, Unset) else value


def _nonempty(name: str, value: str) -> None:
    if not isinstance(value, str) or not value:
        raise ValueError(f"{name} must be a nonempty string")


class HolderIntelligence:
    """AI runtime and configuration shared by projects on this device."""

    def __init__(self, holder: Holder) -> None:
        self._holder = holder
        self.providers = ProviderCollection(holder)
        self.runners = RunnerCollection(holder)
        self.models = LocalModels(holder)

    def status(self) -> IntelligenceStatus:
        data = self._holder._call(get_ai_status)
        return IntelligenceStatus(
            data.checked_at,
            data.runner_available,
            data.active_runs,
            data.active_pull_jobs,
            data.cloud_configured_providers,
            _optional(data.runner_error),
        )


@dataclass(frozen=True)
class IntelligenceStatus:
    checked_at: int
    runner_available: bool
    active_runs: int
    active_pull_jobs: int
    configured_providers: int
    runner_error: str | None


class ProviderCollection(Collection["Provider"]):
    """Cloud providers in the daemon catalog, with local configuration state."""

    def __init__(self, holder: Holder) -> None:
        super().__init__(holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._holder)

    def _fetch_page(self) -> tuple[list[Provider], None]:
        data = self._holder._call(get_ai_providers_catalog)
        return [Provider._from(self._holder, row) for row in data.providers], None

    def get(self, provider_id: str) -> Provider:
        _nonempty("provider_id", provider_id)
        for provider in self.all():
            if provider.id == provider_id:
                return provider
        raise NotFoundError(404, "not_found", f"Provider {provider_id} not found")


@dataclass(frozen=True, eq=False)
class Provider:
    id: str
    name: str
    enabled: bool
    configured: bool
    models: tuple[dict[str, Any], ...]
    setup_url: str | None
    docs_url: str | None
    _holder: Holder = field(repr=False, compare=False)

    @classmethod
    def _from(cls, holder: Holder, row: Any) -> Self:
        return cls(
            row.id,
            row.display_name,
            row.enabled,
            row.configured,
            tuple(item.to_dict() for item in row.models),
            _optional(row.setup_url),
            _optional(row.docs_url),
            holder,
        )

    def refresh(self) -> Self:
        fresh = self._holder.intelligence.providers.get(self.id)
        for item in fields(self):
            if item.name != "_holder":
                object.__setattr__(self, item.name, getattr(fresh, item.name))
        return self

    def set_key(self, api_key: str) -> Self:
        """Store a secret; only a masked preview is returned by the daemon."""
        _nonempty("api_key", api_key)
        self._holder._call(
            put_ai_providers_credentials,
            body=AiProviderCredentialUpsertRequest(provider=self.id, api_key=api_key),
            write=True,
        )
        return self.refresh()

    def clear_key(self) -> Self:
        self._holder._call(
            delete_ai_providers_credentials_provider, self.id, write=True
        )
        return self.refresh()

    def set_enabled(self, enabled: bool) -> Self:
        if not isinstance(enabled, bool):
            raise TypeError("enabled must be a bool")
        self._holder._call(
            put_ai_providers_settings,
            body=AiProviderSettingUpdateRequest(provider=self.id, enabled=enabled),
            write=True,
        )
        return self.refresh()

    def reset_enabled(self) -> Self:
        """Remove the device override and use the provider's default setting."""
        self._holder._call(delete_ai_providers_settings_provider, self.id, write=True)
        return self.refresh()


@dataclass(frozen=True)
class LocalModel:
    name: str
    digest: str | None
    size: int | None
    modified_at: str | None

    @classmethod
    def _from(cls, row: Any) -> Self:
        return cls(
            row.name,
            _optional(row.digest),
            _optional(row.size),
            _optional(row.modified_at),
        )


@dataclass(frozen=True)
class ModelPreferences:
    fast: str | None
    strong: str | None
    deep: str | None
    updated_at: int | None

    @classmethod
    def _from(cls, row: Any) -> Self:
        return cls(row.fast_model, row.strong_model, row.deep_model, row.updated_at)


class LocalModels:
    """Installed local models, preferences, and download jobs."""

    def __init__(self, holder: Holder) -> None:
        self._holder = holder

    def installed(self, *, project: Project | None = None) -> tuple[LocalModel, ...]:
        data = self._holder._call(
            get_ai_capabilities,
            project_id=project.id if project is not None else UNSET,
        )
        return tuple(LocalModel._from(row) for row in data.models)

    def recommended(
        self, *, project: Project | None = None
    ) -> tuple[dict[str, Any], ...]:
        data = self._holder._call(
            get_ai_capabilities,
            project_id=project.id if project is not None else UNSET,
        )
        return tuple(row.to_dict() for row in data.recommended_models)

    def preferences(self) -> ModelPreferences:
        return ModelPreferences._from(self._holder._call(get_ai_local_models_config))

    def configure(
        self,
        *,
        fast: str | None | Unset = UNSET,
        strong: str | None | Unset = UNSET,
        deep: str | None | Unset = UNSET,
    ) -> ModelPreferences:
        """Change selected preferences; None clears one, omission preserves it."""
        if all(isinstance(value, Unset) for value in (fast, strong, deep)):
            raise TypeError("Supply at least one model preference")
        for name, value in (("fast", fast), ("strong", strong), ("deep", deep)):
            if not isinstance(value, Unset) and value is not None:
                _nonempty(name, value)
        previous = self.preferences()
        data = self._holder._call(
            put_ai_local_models_config,
            body=AiLocalModelConfigUpdateRequest(
                fast_model=previous.fast if isinstance(fast, Unset) else fast,
                strong_model=previous.strong if isinstance(strong, Unset) else strong,
                deep_model=previous.deep if isinstance(deep, Unset) else deep,
            ),
            write=True,
        )
        return ModelPreferences._from(data)

    def pull(self, name: str, *, runner_id: str | None = None) -> ModelPullJob:
        """Start downloading a model and return a status/progress handle."""
        _nonempty("name", name)
        data = self._holder._call(
            post_ai_runner_pull,
            body=PostAiRunnerPullBody(
                model=name,
                runner_id=runner_id if runner_id is not None else UNSET,
            ),
            write=True,
        )
        return ModelPullJob(
            self._holder,
            data.job_id,
            data.model,
            _optional(data.runner_id),
            data.status,
            None,
            None,
        )


@dataclass(frozen=True, eq=False)
class ModelPullJob:
    _holder: Holder = field(repr=False, compare=False)
    job_id: str
    model: str
    runner_id: str | None
    status: str
    progress: float | None
    error: str | None

    @property
    def id(self) -> str:
        return self.job_id

    def refresh(self) -> Self:
        data = self._holder._call(
            get_ai_runner_pull_job_id,
            self.id,
            runner_id=self.runner_id if self.runner_id is not None else UNSET,
        )
        for name, value in (
            ("runner_id", _optional(data.runner_id)),
            ("status", data.status),
            ("progress", data.progress.percent),
            ("error", _optional(data.error)),
        ):
            object.__setattr__(self, name, value)
        return self

    def events(self) -> PullEventStream:
        """Watch replaceable progress snapshots; reconnect without an old cursor."""
        from ..intelligence_events import PullEventStream

        return PullEventStream(self._holder, self.id, self.runner_id)


class RunnerCollection(Collection["Runner"]):
    def __init__(self, holder: Holder) -> None:
        super().__init__(holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._holder)

    def _fetch_page(self) -> tuple[list[Runner], None]:
        data = self._holder._call(get_ai_runners)
        return [Runner._from(self._holder, row) for row in data.runners], None

    def get(self, runner_id: str) -> Runner:
        _nonempty("runner_id", runner_id)
        return Runner._from(
            self._holder, self._holder._call(get_ai_runners_runner_id, runner_id)
        )

    def create(self, *, name: str, base_url: str, enabled: bool = True) -> Runner:
        _nonempty("name", name)
        _nonempty("base_url", base_url)
        if not isinstance(enabled, bool):
            raise TypeError("enabled must be a bool")
        row = self._holder._call(
            post_ai_runners,
            body=AiRunnerCreateRequest(
                name=name,
                kind=AiRunnerCreateRequestKind.OLLAMA,
                base_url=base_url,
                enabled=enabled,
            ),
            write=True,
        )
        return Runner._from(self._holder, row)


@dataclass(frozen=True, eq=False)
class Runner:
    runner_id: str
    name: str
    kind: str
    source: str
    base_url: str | None
    enabled: bool
    available: bool
    models: tuple[LocalModel, ...]
    created_at: int
    updated_at: int
    error: str | None
    _holder: Holder = field(repr=False, compare=False)

    @property
    def id(self) -> str:
        return self.runner_id

    @classmethod
    def _from(cls, holder: Holder, row: Any) -> Self:
        return cls(
            row.runner_id,
            row.name,
            row.kind,
            row.source,
            row.base_url,
            row.enabled,
            row.runtime.available,
            tuple(LocalModel._from(model) for model in row.runtime.models),
            row.created_at,
            row.updated_at,
            row.runtime.error,
            holder,
        )

    def refresh(self) -> Self:
        fresh = self._holder.intelligence.runners.get(self.id)
        for item in fields(self):
            if item.name != "_holder":
                object.__setattr__(self, item.name, getattr(fresh, item.name))
        return self

    def update(
        self,
        *,
        name: str | Unset = UNSET,
        base_url: str | None | Unset = UNSET,
        enabled: bool | Unset = UNSET,
    ) -> Self:
        if all(isinstance(value, Unset) for value in (name, base_url, enabled)):
            raise TypeError("Supply a runner setting")
        if not isinstance(name, Unset):
            _nonempty("name", name)
        if not isinstance(base_url, Unset) and base_url is not None:
            _nonempty("base_url", base_url)
        if not isinstance(enabled, (Unset, bool)):
            raise TypeError("enabled must be a bool")
        row = self._holder._call(
            patch_ai_runners_runner_id,
            self.id,
            body=AiRunnerUpdateRequest(name=name, base_url=base_url, enabled=enabled),
            write=True,
        )
        fresh = Runner._from(self._holder, row)
        for item in fields(self):
            if item.name != "_holder":
                object.__setattr__(self, item.name, getattr(fresh, item.name))
        return self

    def retry(self) -> Self:
        self._holder._call(post_ai_runners_runner_id_retry, self.id, write=True)
        return self.refresh()

    def delete(self) -> None:
        self._holder._call(delete_ai_runners_runner_id, self.id, write=True)
