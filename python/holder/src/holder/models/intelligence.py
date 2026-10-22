"""Project-scoped intelligence threads, messages, search, and runs."""

from __future__ import annotations

import time
from dataclasses import dataclass, field, fields
from functools import cached_property
from typing import TYPE_CHECKING, Any, Self

from ..collections import Collection
from ..exceptions import NotFoundError, ProtocolError
from ..generated.api.default import (
    delete_ai_messages_message_id,
    delete_ai_messages_message_id_links,
    delete_ai_threads_thread_id,
    get_ai_messages,
    get_ai_messages_message_id,
    get_ai_messages_message_id_backlinks,
    get_ai_messages_message_id_links,
    get_ai_runs,
    get_ai_runs_run_id,
    get_ai_threads,
    get_ai_threads_thread_id,
    get_search_ai,
    patch_ai_messages_message_id,
    patch_ai_threads_thread_id,
    post_ai_messages,
    post_ai_messages_capture,
    post_ai_messages_message_id_links,
    post_ai_messages_message_id_restore,
    post_ai_threads,
)
from ..generated.models import (
    AiMessageCaptureRequest,
    AiMessageCreateRequest,
    AiMessageUpdateRequest,
    AiThreadCreateRequest,
    AiThreadUpdateRequest,
    CardLinkCreateRequest,
    CardLinkDeleteRequest,
)
from ..generated.models.get_ai_messages_include_deleted import (
    GetAiMessagesIncludeDeleted,
)
from ..generated.models.get_ai_messages_message_id_backlinks_include_deleted import (
    GetAiMessagesMessageIdBacklinksIncludeDeleted,
)
from ..generated.models.get_ai_messages_message_id_links_include_deleted import (
    GetAiMessagesMessageIdLinksIncludeDeleted,
)
from ..generated.types import UNSET, Unset
from .link import CardLink

if TYPE_CHECKING:
    from ..intelligence_events import RunStream
    from .project import Project


def _optional(value: Any) -> Any:
    return None if isinstance(value, Unset) else value


def _nonempty(name: str, value: str) -> None:
    if not isinstance(value, str) or not value:
        raise ValueError(f"{name} must be a nonempty string")


class ProjectIntelligence:
    """Threads, message search, and runs belonging to one project."""

    def __init__(self, project: Project) -> None:
        self._project = project
        self.threads = ThreadCollection(project)
        self.runs = RunCollection(project)

    def message(self, message_id: str) -> Message:
        """Fetch a message by ID, checking that its thread belongs to this project."""
        _nonempty("message_id", message_id)
        row = self._project._holder._call(get_ai_messages_message_id, message_id)
        self.threads.get(row.thread_id)
        return Message._from(self._project, row)

    def search(self, query: str) -> MessageSearchCollection:
        _nonempty("query", query)
        return MessageSearchCollection(self._project, query)

    def capture(
        self,
        prompt: str,
        response: str,
        *,
        thread: Thread | None = None,
        source: str = "manual_paste",
        provider: str | None = None,
        model: str | None = None,
        url: str | None = None,
    ) -> CapturedExchange:
        """Save a prompt and response produced outside Holder."""
        if not isinstance(prompt, str) or not isinstance(response, str):
            raise TypeError("prompt and response must be strings")
        _nonempty("source", source)
        if thread is not None and thread.project_id != self._project.id:
            raise ValueError("thread belongs to another project")
        data = self._project._holder._call(
            post_ai_messages_capture,
            body=AiMessageCaptureRequest(
                project_id=self._project.id,
                prompt=prompt,
                response=response,
                thread_id=thread.id if thread is not None else UNSET,
                source=source,
                provider=provider if provider is not None else UNSET,
                model=model if model is not None else UNSET,
                url=url if url is not None else UNSET,
            ),
            write=True,
        )
        return CapturedExchange(
            self._project,
            data.thread_id,
            data.user_message_id,
            data.assistant_message_id,
        )

    def run(
        self,
        prompt: str,
        *,
        thread: Thread | None = None,
        model: str | None = None,
        provider: str | None = None,
        context: dict[str, Any] | None = None,
    ) -> RunStream:
        """Prepare a streamed run. Open it with ``with`` to start execution."""
        from ..intelligence_events import RunStream

        _nonempty("prompt", prompt)
        if thread is not None and thread.project_id != self._project.id:
            raise ValueError("thread belongs to another project")
        if context is not None and not isinstance(context, dict):
            raise TypeError("context must be a dictionary")
        return RunStream(
            self._project._holder,
            prompt=prompt,
            project_id=self._project.id,
            thread_id=thread.id if thread is not None else None,
            model=model,
            provider=provider,
            context=context,
        )


class ThreadCollection(Collection["Thread"]):
    def __init__(self, project: Project) -> None:
        self._project = project
        super().__init__(project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project)

    def _fetch_page(self) -> tuple[list[Thread], None]:
        rows = self._holder._call(get_ai_threads, project_id=self._project.id)
        if any(row.project_id != self._project.id for row in rows):
            raise ProtocolError("Thread listing returned another project's thread")
        return [Thread._from(self._project, row) for row in rows], None

    def get(self, thread_id: str) -> Thread:
        row = self._holder._call(get_ai_threads_thread_id, thread_id)
        if row.project_id != self._project.id:
            raise NotFoundError(404, "not_found", "Thread not found in this project")
        return Thread._from(self._project, row)

    def create(self, *, title: str, card_id: str | None = None) -> Thread:
        _nonempty("title", title)
        data = self._holder._call(
            post_ai_threads,
            body=AiThreadCreateRequest(
                project_id=self._project.id,
                title=title,
                card_id=card_id if card_id is not None else UNSET,
            ),
            write=True,
        )
        return self.get(data.thread_id)


@dataclass(frozen=True, eq=False)
class Thread:
    thread_id: str
    project_id: str
    title: str
    created_at: int
    updated_at: int
    card_id: str | None
    _project: Project = field(repr=False, compare=False)

    @property
    def id(self) -> str:
        return self.thread_id

    @classmethod
    def _from(cls, project: Project, row: Any) -> Self:
        return cls(
            row.thread_id,
            row.project_id,
            row.title,
            row.created_at,
            row.updated_at,
            _optional(row.card_id),
            project,
        )

    @cached_property
    def messages(self) -> MessageCollection:
        return MessageCollection(self)

    @cached_property
    def runs(self) -> RunCollection:
        return RunCollection(self._project, thread_id=self.id)

    def ask(
        self,
        prompt: str,
        *,
        model: str | None = None,
        provider: str | None = None,
        context: dict[str, Any] | None = None,
    ) -> RunStream:
        """Start a run in this thread when its stream context is entered.

        The run writes its own user and assistant messages; do not add the
        prompt separately to ``messages``.
        """
        return self._project.intelligence.run(
            prompt, thread=self, model=model, provider=provider, context=context
        )

    def update(
        self, *, title: str | Unset = UNSET, card_id: str | None | Unset = UNSET
    ) -> Self:
        if isinstance(title, Unset) and isinstance(card_id, Unset):
            raise TypeError("Supply title or card_id")
        if not isinstance(title, Unset):
            _nonempty("title", title)
        self._project._holder._call(
            patch_ai_threads_thread_id,
            self.id,
            body=AiThreadUpdateRequest(
                updated_at=int(time.time()), title=title, card_id=card_id
            ),
            write=True,
        )
        return self.refresh()

    def refresh(self) -> Self:
        fresh = self._project.intelligence.threads.get(self.id)
        for item in fields(self):
            if item.name != "_project":
                object.__setattr__(self, item.name, getattr(fresh, item.name))
        return self

    def delete(self) -> None:
        self._project._holder._call(delete_ai_threads_thread_id, self.id, write=True)


class MessageCollection(Collection["Message"]):
    def __init__(self, thread: Thread, *, include_deleted: bool = False) -> None:
        self._thread = thread
        self._include_deleted = include_deleted
        super().__init__(thread._project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(
            self._thread, include_deleted=query.get("include_deleted", False)
        )

    def including_trashed(self) -> Self:
        return self._clone({"include_deleted": True})

    def _fetch_page(self) -> tuple[list[Message], None]:
        rows = self._holder._call(
            get_ai_messages,
            thread_id=self._thread.id,
            include_deleted=GetAiMessagesIncludeDeleted(int(self._include_deleted)),
        )
        if any(row.thread_id != self._thread.id for row in rows):
            raise ProtocolError("Message listing returned another thread's message")
        return [Message._from(self._thread._project, row) for row in rows], None

    def get(self, message_id: str) -> Message:
        message = self._thread._project.intelligence.message(message_id)
        if message.thread_id != self._thread.id:
            raise NotFoundError(404, "not_found", "Message not found in this thread")
        return message

    def add(
        self, content: str, *, role: str = "user", source: str = "manual"
    ) -> Message:
        if not isinstance(content, str):
            raise TypeError("content must be a string")
        _nonempty("role", role)
        _nonempty("source", source)
        data = self._holder._call(
            post_ai_messages,
            body=AiMessageCreateRequest(
                thread_id=self._thread.id,
                role=role,
                source=source,
                content=content,
            ),
            write=True,
        )
        return self.get(data.message_id)


@dataclass(frozen=True, eq=False)
class Message:
    message_id: str
    thread_id: str
    role: str
    source: str
    content: str
    created_at: int
    provider: str | None
    model: str | None
    deleted_at: int | None
    prompt_hash: str | None
    meta_json: str | None
    _project: Project = field(repr=False, compare=False)

    @property
    def id(self) -> str:
        return self.message_id

    @classmethod
    def _from(cls, project: Project, row: Any) -> Self:
        return cls(
            row.message_id,
            row.thread_id,
            row.role,
            row.source,
            row.content,
            row.created_at,
            row.provider,
            row.model,
            row.deleted_at,
            row.prompt_hash,
            row.meta_json,
            project,
        )

    @cached_property
    def links(self) -> MessageLinkCollection:
        return MessageLinkCollection(self)

    @cached_property
    def backlinks(self) -> MessageBacklinkCollection:
        return MessageBacklinkCollection(self)

    def update(self, *, content: str | Unset = UNSET) -> Self:
        if isinstance(content, Unset):
            raise TypeError("Supply content")
        if not isinstance(content, str):
            raise TypeError("content must be a string")
        self._project._holder._call(
            patch_ai_messages_message_id,
            self.id,
            body=AiMessageUpdateRequest(content=content),
            write=True,
        )
        return self.refresh()

    def refresh(self) -> Self:
        fresh = self._project.intelligence.message(self.id)
        for item in fields(self):
            if item.name != "_project":
                object.__setattr__(self, item.name, getattr(fresh, item.name))
        return self

    def trash(self) -> Self:
        self._project._holder._call(delete_ai_messages_message_id, self.id, write=True)
        return self.refresh()

    def restore(self) -> Self:
        self._project._holder._call(
            post_ai_messages_message_id_restore, self.id, write=True
        )
        return self.refresh()


class MessageLinkCollection(Collection[CardLink]):
    def __init__(self, message: Message, *, include_deleted: bool = False) -> None:
        self._message = message
        self._include_deleted = include_deleted
        super().__init__(message._project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(
            self._message, include_deleted=query.get("include_deleted", False)
        )

    def including_deleted(self) -> Self:
        return self._clone({"include_deleted": True})

    def _fetch_page(self) -> tuple[list[CardLink], None]:
        self._message._project.intelligence.message(self._message.id)
        rows = self._holder._call(
            get_ai_messages_message_id_links,
            self._message.id,
            include_deleted=GetAiMessagesMessageIdLinksIncludeDeleted(
                int(self._include_deleted)
            ),
        )
        return [CardLink._from(row, self._message._project.id) for row in rows], None

    def add(
        self,
        target_id: str,
        *,
        target_project_id: str | None = None,
        target_type: str = "card",
        kind: str = "ref",
        label: str | None = None,
    ) -> CardLink:
        _nonempty("target_id", target_id)
        _nonempty("target_type", target_type)
        _nonempty("kind", kind)
        self._message._project.intelligence.message(self._message.id)
        row = self._holder._call(
            post_ai_messages_message_id_links,
            self._message.id,
            body=CardLinkCreateRequest(
                to_card_id=target_id,
                target_project_id=target_project_id or UNSET,
                to_type=target_type,
                kind=kind,
                label=label,
            ),
            write=True,
        )
        self.refresh()
        return CardLink._from(row, self._message._project.id)

    def remove(
        self,
        target_id: str,
        *,
        target_project_id: str | None = None,
        target_type: str = "card",
        kind: str = "ref",
    ) -> Self:
        _nonempty("target_id", target_id)
        self._message._project.intelligence.message(self._message.id)
        self._holder._call(
            delete_ai_messages_message_id_links,
            self._message.id,
            body=CardLinkDeleteRequest(
                to_card_id=target_id,
                target_project_id=target_project_id or self._message._project.id,
                to_type=target_type,
                kind=kind,
            ),
            write=True,
        )
        return self.refresh()


class MessageBacklinkCollection(MessageLinkCollection):
    def _fetch_page(self) -> tuple[list[CardLink], None]:
        self._message._project.intelligence.message(self._message.id)
        rows = self._holder._call(
            get_ai_messages_message_id_backlinks,
            self._message.id,
            include_deleted=GetAiMessagesMessageIdBacklinksIncludeDeleted(
                int(self._include_deleted)
            ),
        )
        return [CardLink._from(row, self._message._project.id) for row in rows], None

    def add(self, *args: Any, **kwargs: Any) -> CardLink:
        raise TypeError("Backlinks are read-only")

    def remove(self, *args: Any, **kwargs: Any) -> Self:
        raise TypeError("Backlinks are read-only")


class MessageSearchCollection(Collection["MessageSearchResult"]):
    def __init__(self, project: Project, query: str) -> None:
        self._project = project
        super().__init__(project._holder, {"q": query})

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project, query["q"])

    def _fetch_page(self) -> tuple[list[MessageSearchResult], int | None]:
        offset = int(self._next or 0)
        rows = self._holder._call(
            get_search_ai,
            project_id=self._project.id,
            q=self._query["q"],
            limit=100,
            offset=offset,
        )
        return (
            [
                MessageSearchResult(
                    row.message_id, row.created_at, row.snippet, row.rank, self._project
                )
                for row in rows
            ],
            offset + len(rows) if len(rows) == 100 else None,
        )


@dataclass(frozen=True, eq=False)
class MessageSearchResult:
    message_id: str
    created_at: int
    snippet: str
    rank: float
    _project: Project = field(repr=False, compare=False)

    @cached_property
    def message(self) -> Message:
        return self._project.intelligence.message(self.message_id)


@dataclass(frozen=True)
class CapturedExchange:
    _project: Project = field(repr=False, compare=False)
    thread_id: str
    user_message_id: str
    assistant_message_id: str

    @cached_property
    def thread(self) -> Thread:
        return self._project.intelligence.threads.get(self.thread_id)

    @cached_property
    def user_message(self) -> Message:
        return self._project.intelligence.message(self.user_message_id)

    @cached_property
    def assistant_message(self) -> Message:
        return self._project.intelligence.message(self.assistant_message_id)


class RunCollection(Collection["Run"]):
    def __init__(self, project: Project, *, thread_id: str | None = None) -> None:
        self._project = project
        self._thread_id = thread_id
        super().__init__(project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project, thread_id=self._thread_id)

    def _fetch_page(self) -> tuple[list[Run], None]:
        rows = self._holder._call(
            get_ai_runs,
            project_id=self._project.id,
            thread_id=self._thread_id if self._thread_id is not None else UNSET,
        )
        if any(
            _optional(row.project_id) != self._project.id
            or (
                self._thread_id is not None
                and _optional(row.thread_id) != self._thread_id
            )
            for row in rows
        ):
            raise ProtocolError("Run listing returned a run outside its scope")
        return [Run._from(self._project, row) for row in rows], None

    def get(self, run_id: str) -> Run:
        row = self._holder._call(get_ai_runs_run_id, run_id)
        if _optional(row.project_id) != self._project.id or (
            self._thread_id is not None and _optional(row.thread_id) != self._thread_id
        ):
            raise NotFoundError(404, "not_found", "Run not found in this scope")
        return Run._from(self._project, row)


@dataclass(frozen=True, eq=False)
class Run:
    run_id: str
    project_id: str
    thread_id: str | None
    prompt: str
    mode: str
    status: str
    chosen_model: str | None
    message_id: str | None
    error: str | None
    created_at: int
    updated_at: int
    _project: Project = field(repr=False, compare=False)

    @property
    def id(self) -> str:
        return self.run_id

    @classmethod
    def _from(cls, project: Project, row: Any) -> Self:
        return cls(
            row.run_id,
            _optional(row.project_id),
            _optional(row.thread_id),
            row.prompt,
            row.mode,
            row.status,
            _optional(row.chosen_model),
            _optional(row.message_id),
            _optional(row.error),
            row.created_at,
            row.updated_at,
            project,
        )

    def refresh(self) -> Self:
        fresh = self._project.intelligence.runs.get(self.id)
        for item in fields(self):
            if item.name != "_project":
                object.__setattr__(self, item.name, getattr(fresh, item.name))
        return self

    def events(self, *, after: str | None = None) -> RunStream:
        """Replay or follow events for this run using an opaque event cursor."""
        from ..intelligence_events import RunStream

        return RunStream(self._project._holder, run_id=self.id, after=after)
