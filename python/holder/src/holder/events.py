"""Incremental asyncio change feed over GET /events; no entity or recovery policy."""

import asyncio
import math
from dataclasses import dataclass
from typing import Any, Self

import httpx

from .exceptions import ProtocolError
from .generated import AuthenticatedClient, Client
from .generated.api.default import stream_changes
from .generated.streams import AsyncEventStream
from .generated.types import UNSET


@dataclass(frozen=True)
class Event:
    """A change-feed event. Data is decoded as the contract declares it; IDs are opaque."""

    event: str
    data: Any
    id: str | None = None


class ChangeStream:
    """Context-managed asyncio iterator over daemon invalidations.

    Pass the same project filter used for /events/cursor. cursor advances when
    the consumer requests the next event (after processing the previous one).
    Persist cursor for a later subscription. A ready event uses its ID, never
    the newer checkpoint in its data. resync_required is yielded once and stops
    iteration without advancing cursor: refresh state before subscribing again.

    A failed first connection, a dropped connection and HTTPX transport errors
    reconnect after reconnect_delay seconds, resuming after the last event.
    HTTP errors and invalid content types raise the generated StreamError
    without retry, and malformed event data raises ProtocolError. Read timeout
    defaults to disabled; the client's other timeouts still apply. Cancellation,
    aclose(), and context exit close the active response. The supplied
    generated client remains caller-owned. Use asyncio task cancellation or
    aclose() to interrupt a pending read; only one reader is supported.
    """

    def __init__(
        self,
        client: Client | AuthenticatedClient,
        *,
        project_id: str | None = None,
        last_revision: str | None = None,
        last_event_id: str | None = None,
        reconnect_delay: float = 1.0,
        timeout: httpx.Timeout | None = None,
    ) -> None:
        if not math.isfinite(reconnect_delay) or reconnect_delay < 0:
            raise ValueError("reconnect_delay must be finite and nonnegative")
        self.cursor = last_event_id
        self._client = client
        self._project_id = project_id
        self._last_revision = last_revision
        self._delay = reconnect_delay
        self._timeout = timeout
        self._stream: AsyncEventStream[Any] | None = None
        # The ID of the event the consumer is processing, acknowledged when it asks for the next.
        self._delivered: str | None = None
        self._pending: asyncio.Task[Event] | None = None
        self._entered = False
        self._closed = False

    async def __aenter__(self) -> Self:
        if self._entered or self._closed:
            raise RuntimeError("ChangeStream contexts cannot be reused")
        self._entered = True
        return self

    async def __aexit__(self, *args: object) -> None:
        await self.aclose()

    def __aiter__(self) -> Self:
        return self

    async def __anext__(self) -> Event:
        if not self._entered:
            raise RuntimeError("Use ChangeStream inside async with")
        if self._closed:
            raise StopAsyncIteration
        if self._pending is not None:
            raise RuntimeError("ChangeStream supports only one reader")
        if self._delivered is not None:
            self.cursor, self._delivered = self._delivered, None
        pending = asyncio.create_task(self._next())
        self._pending = pending
        try:
            return await pending
        except BaseException:
            self._closed = True
            await self._close_stream()
            raise
        finally:
            self._pending = None

    async def aclose(self) -> None:
        """Stop iteration, including a blocked read or reconnect delay."""
        self._closed = True
        pending = self._pending
        if pending is not None:
            pending.cancel()
            try:
                await pending
            except (asyncio.CancelledError, StopAsyncIteration):
                pass
        await self._close_stream()

    async def _close_stream(self) -> None:
        stream, self._stream = self._stream, None
        if stream is not None:
            await stream.aclose()

    async def _connect(self) -> AsyncEventStream[Any]:
        while True:
            stream = stream_changes.astream(
                client=self._client,
                project_id=self._project_id if self._project_id is not None else UNSET,
                last_revision=(
                    self._last_revision if self._last_revision is not None else UNSET
                ),
                last_event_id=self.cursor if self.cursor is not None else UNSET,
            )
            stream.reconnect_delay = self._delay
            stream.timeout = self._timeout
            try:
                await stream.__aenter__()
            except httpx.TransportError:
                await stream.aclose()
                await asyncio.sleep(self._delay)
                continue
            self._stream = stream
            return stream

    async def _next(self) -> Event:
        stream = self._stream or await self._connect()
        try:
            event = await anext(stream)
        except (KeyError, TypeError, ValueError):
            raise ProtocolError("Invalid change event") from None
        if event.event == "resync_required":
            # Not acknowledged: the consumer must refresh state before subscribing again.
            self._closed = True
            await self._close_stream()
        else:
            self._delivered = event.id
        return Event(event.event, event.data, event.id)
