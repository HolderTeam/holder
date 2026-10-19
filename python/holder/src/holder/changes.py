"""Project and library change notifications over the daemon event stream."""

from __future__ import annotations

import asyncio
import json
from collections.abc import AsyncIterator
from contextlib import AsyncExitStack
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, Literal, Self

import httpx

from .events import ChangeStream, Event
from .exceptions import (
    APIError,
    AuthenticationError,
    NotFoundError,
    ProtocolError,
)
from .generated import AuthenticatedClient
from .generated.api.default import get_event_cursor

if TYPE_CHECKING:
    from .client import Holder

EntityKind = Literal[
    "project", "card", "resource", "location", "thread", "message", "run"
]
_KINDS = frozenset(
    ("project", "card", "resource", "location", "thread", "message", "run")
)


@dataclass(frozen=True)
class ChangeCheckpoint:
    """Cursor to save before reading a snapshot and use when watching changes."""

    cursor: str
    git_revisions: dict[str, str | None]
    history_urls: dict[str, str]
    project_id: str | None = None


@dataclass(frozen=True)
class ChangeReady:
    """The stream is connected; checkpoint reflects the daemon's current state."""

    checkpoint: ChangeCheckpoint
    cursor: str


@dataclass(frozen=True)
class EntityChange:
    """An entity may have changed; fetch current state if it matters to you."""

    entity: EntityKind
    id: str
    project_id: str | None
    deleted: bool
    git_revision: str | None
    cursor: str


@dataclass(frozen=True)
class ResyncRequired:
    """Replay is unavailable; take a fresh snapshot before watching again."""

    reason: str
    checkpoint: ChangeCheckpoint
    last_revision: str | None


ChangeNotice = ChangeReady | EntityChange | ResyncRequired


def _checkpoint(data: Any, project_id: str | None) -> ChangeCheckpoint:
    if not isinstance(data, dict):
        raise ProtocolError("Invalid change checkpoint")
    cursor = data.get("cursor")
    revisions = data.get("git_revisions")
    urls = data.get("history_urls")
    if (
        not isinstance(cursor, str)
        or not cursor
        or not isinstance(revisions, dict)
        or not isinstance(urls, dict)
        or any(
            not isinstance(key, str)
            or (value is not None and not isinstance(value, str))
            for key, value in revisions.items()
        )
        or any(
            not isinstance(key, str) or not isinstance(value, str)
            for key, value in urls.items()
        )
    ):
        raise ProtocolError("Invalid change checkpoint")
    return ChangeCheckpoint(cursor, dict(revisions), dict(urls), project_id)


def _payload(frame: Event) -> dict[str, Any]:
    try:
        data = json.loads(frame.data)
    except (ValueError, TypeError):
        raise ProtocolError("Invalid change event JSON") from None
    if not isinstance(data, dict):
        raise ProtocolError("Invalid change event JSON")
    return data


def _notice(frame: Event, project_id: str | None) -> ChangeNotice:
    if not isinstance(frame.id, str) or not frame.id:
        raise ProtocolError("Change event omitted its cursor")
    data = _payload(frame)
    if frame.event == "ready":
        return ChangeReady(_checkpoint(data, project_id), frame.id)
    if frame.event == "resync_required":
        reason = data.get("reason")
        revision = data.get("last_revision")
        if not isinstance(reason, str) or (
            revision is not None and not isinstance(revision, str)
        ):
            raise ProtocolError("Invalid resync event")
        return ResyncRequired(reason, _checkpoint(data, project_id), revision)
    kind = data.get("entity")
    entity_id = data.get("entity_id")
    scope = data.get("project_id")
    deleted = data.get("deleted")
    revision = data.get("git_revision")
    if (
        not isinstance(kind, str)
        or kind not in _KINDS
        or frame.event != f"{kind}.changed"
        or not isinstance(entity_id, str)
        or not entity_id
        or (scope is not None and not isinstance(scope, str))
        or (project_id is not None and scope != project_id)
        or not isinstance(deleted, bool)
        or (revision is not None and not isinstance(revision, str))
    ):
        raise ProtocolError("Invalid entity change")
    return EntityChange(kind, entity_id, scope, deleted, revision, frame.id)


class Changes:
    """Get a checkpoint and watch library or project-scoped invalidations."""

    def __init__(self, holder: Holder, project_id: str | None = None) -> None:
        self._holder = holder
        self._project_id = project_id

    def checkpoint(self) -> ChangeCheckpoint:
        """Take this before reading entities to avoid a snapshot/watch gap."""
        data = self._holder._call(
            get_event_cursor,
            **({"project_id": self._project_id} if self._project_id else {}),
        )
        return _checkpoint(data.to_dict(), self._project_id)

    def watch(
        self,
        *,
        after: ChangeCheckpoint | str | None = None,
        reconnect_delay: float = 1.0,
    ) -> ChangeWatcher:
        """Open an async stream. Without after, start at the current tail."""
        if isinstance(after, ChangeCheckpoint):
            if after.project_id != self._project_id:
                raise ValueError("Checkpoint belongs to a different change scope")
            cursor = after.cursor
        elif after is None or isinstance(after, str):
            cursor = after
        else:
            raise TypeError("after must be a checkpoint, cursor string, or None")
        if cursor == "":
            raise ValueError("after cursor must be nonempty")
        return ChangeWatcher(self, cursor, reconnect_delay)


class ChangeWatcher(AsyncIterator[ChangeNotice]):
    """One-reader async watcher with its own connection and explicit resync."""

    def __init__(
        self, changes: Changes, cursor: str | None, reconnect_delay: float
    ) -> None:
        self._changes = changes
        self._cursor = cursor
        self._delay = reconnect_delay
        self._stack: AsyncExitStack | None = None
        self._stream: ChangeStream | None = None
        self._closed = False
        self._reading = False

    @property
    def cursor(self) -> str | None:
        """Last processed cursor; persist after requesting the next event."""
        return self._stream.cursor if self._stream is not None else self._cursor

    async def __aenter__(self) -> Self:
        if self._stack is not None or self._closed:
            raise RuntimeError("Change watchers cannot be reused")
        holder = self._changes._holder
        holder._active()
        transport = holder._transport
        if transport is not None and not isinstance(
            transport, httpx.AsyncBaseTransport
        ):
            raise TypeError("The configured transport does not support async requests")
        client = AuthenticatedClient(
            base_url=holder._url,
            token=holder._token,
            timeout=httpx.Timeout(holder._timeout),
            httpx_args={"transport": transport} if transport is not None else {},
        )
        stack = AsyncExitStack()
        try:
            await stack.enter_async_context(client)
            stream = ChangeStream(
                client,
                project_id=self._changes._project_id,
                last_event_id=self._cursor,
                reconnect_delay=self._delay,
            )
            await stack.enter_async_context(stream)
        except BaseException:
            await stack.aclose()
            raise
        self._stack = stack
        self._stream = stream
        return self

    async def __aexit__(self, *args: object) -> None:
        await self.aclose()

    def __aiter__(self) -> Self:
        return self

    async def __anext__(self) -> ChangeNotice:
        if self._stream is None:
            raise RuntimeError("Use ChangeWatcher inside async with")
        if self._closed:
            raise StopAsyncIteration
        if self._reading:
            raise RuntimeError("ChangeWatcher supports only one reader")
        self._reading = True
        try:
            self._changes._holder._active()
            try:
                frame = await anext(self._stream)
            except httpx.HTTPStatusError as exc:
                if exc.response.status_code == 401 and self._changes._holder._local:
                    checkpoint = await asyncio.to_thread(self._changes.checkpoint)
                    self._closed = True
                    return ResyncRequired("daemon_restarted", checkpoint, None)
                error_type = {401: AuthenticationError, 404: NotFoundError}.get(
                    exc.response.status_code, APIError
                )
                raise error_type(
                    exc.response.status_code, "http_error", "Change stream failed"
                ) from None
            notice = _notice(frame, self._changes._project_id)
            if isinstance(notice, ResyncRequired):
                self._closed = True
            return notice
        finally:
            self._reading = False

    async def aclose(self) -> None:
        """Stop a pending read and close the owned async connection."""
        self._closed = True
        if self._stack is not None:
            stack, self._stack = self._stack, None
            await stack.aclose()
