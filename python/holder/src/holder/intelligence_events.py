"""Synchronous, context-managed streams for intelligence runs and model pulls."""

from __future__ import annotations

import json
from collections.abc import Callable, Iterator
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, Self

import httpx

from ._streams import stream_failure
from .exceptions import ConnectionError, ProtocolError
from .generated import AuthenticatedClient
from .generated.api.default import (
    get_ai_runner_pull_job_id_events,
    get_ai_runs_run_id_events,
    post_ai_runs,
)
from .generated.models import AiRunCreateRequest
from .generated.streams import EventStream, ServerSentEvent, StreamError
from .generated.types import UNSET

if TYPE_CHECKING:
    from .client import Holder


@dataclass(frozen=True)
class IntelligenceEvent:
    """One streamed event, with an opaque cursor and its original payload."""

    kind: str
    data: dict[str, Any]
    cursor: str | None

    @property
    def text(self) -> str:
        """Text added by a chunk event, or an empty string for other events."""
        delta = self.data.get("delta") if self.kind == "chunk" else None
        return delta if isinstance(delta, str) else ""


def _event(event: ServerSentEvent[Any]) -> IntelligenceEvent:
    data = event.data
    if isinstance(data, str):
        # An event the contract doesn't name still carries a JSON object.
        try:
            data = json.loads(data)
        except ValueError:
            raise ProtocolError("Invalid intelligence event JSON") from None
        if not isinstance(data, dict):
            raise ProtocolError("Invalid intelligence event JSON")
    else:
        data = data.to_dict()
    return IntelligenceEvent(event.event, data, event.id)


class _EventStream(Iterator[IntelligenceEvent]):
    """One-reader stream. Closing it does not cancel the server operation.

    A stream that follows a run or pull reconnects by itself after a dropped
    connection, resuming after its cursor, until the daemon says the stream is
    finished. It ends at a terminal event.
    """

    def __init__(
        self,
        holder: Holder,
        open_stream: Callable[[AuthenticatedClient], EventStream[Any]],
        *,
        terminal: frozenset[str],
        after: str | None = None,
    ) -> None:
        if after == "":
            raise ValueError("after cursor must be nonempty")
        self._holder = holder
        self._open_stream = open_stream
        self._terminal = terminal
        self._stream: EventStream[Any] | None = None
        self._first: IntelligenceEvent | None = None
        self._done = False
        self.cursor = after

    def _read(self) -> IntelligenceEvent | None:
        assert self._stream is not None
        try:
            event = next(self._stream, None)
        except StreamError as error:
            raise stream_failure(
                error, "Expected an intelligence event stream"
            ) from None
        except httpx.TransportError:
            raise ConnectionError(
                "Intelligence stream disconnected; reconnect using its cursor"
            ) from None
        except (KeyError, TypeError, ValueError):
            raise ProtocolError("Invalid intelligence event") from None
        return None if event is None else _event(event)

    def __enter__(self) -> Self:
        if self._stream is not None:
            raise RuntimeError("Intelligence streams cannot be reused")
        stream = self._open_stream(self._holder._active())
        try:
            stream.__enter__()
        except StreamError as error:
            raise stream_failure(
                error, "Expected an intelligence event stream"
            ) from None
        except httpx.TransportError:
            raise ConnectionError("Could not read intelligence event stream") from None
        self._stream = stream
        try:
            first = self._read()
            if first is None:
                raise ConnectionError(
                    "Intelligence stream closed before its first event"
                )
        except BaseException:
            self.close()
            raise
        self._first = first
        return self

    def __exit__(self, *args: object) -> None:
        self.close()

    def close(self) -> None:
        self._done = True
        if self._stream is not None:
            self._stream.close()

    def __iter__(self) -> Self:
        return self

    def __next__(self) -> IntelligenceEvent:
        if self._stream is None:
            raise RuntimeError("Use intelligence streams inside with")
        if self._done:
            raise StopIteration
        if self._first is not None:
            event, self._first = self._first, None
        else:
            event = self._read()
        if event is None:
            self.close()
            raise ConnectionError(
                "Intelligence stream ended before a terminal event; reconnect using its cursor"
            )
        if event.cursor is not None:
            self.cursor = event.cursor
        if event.kind in self._terminal:
            self.close()
        return event


class RunStream(_EventStream):
    """Creates a run on entry, or follows an existing run by ID."""

    def __init__(
        self,
        holder: Holder,
        *,
        prompt: str | None = None,
        project_id: str | None = None,
        thread_id: str | None = None,
        model: str | None = None,
        provider: str | None = None,
        context: dict[str, Any] | None = None,
        run_id: str | None = None,
        after: str | None = None,
    ) -> None:
        open_stream: Callable[[AuthenticatedClient], EventStream[Any]]
        if run_id is None:
            if prompt is None or project_id is None:
                raise ValueError("prompt and project_id are required for a new run")
            body: dict[str, Any] = {
                "prompt": prompt,
                "project_id": project_id,
                "mode": "model" if model is not None else "auto",
            }
            if thread_id is not None:
                body["thread_id"] = thread_id
            if model is not None:
                body["model"] = model
            if provider is not None:
                body["provider"] = provider
            if context is not None:
                body["context"] = context
            request = AiRunCreateRequest.from_dict(body)

            def open_stream(client: AuthenticatedClient) -> EventStream[Any]:
                return post_ai_runs.stream(client=client, body=request)

        else:
            followed = run_id

            def open_stream(client: AuthenticatedClient) -> EventStream[Any]:
                return get_ai_runs_run_id_events.stream(
                    followed,
                    client=client,
                    last_event_id=after if after is not None else UNSET,
                )

        self.run_id = run_id
        self._creating = run_id is None
        super().__init__(
            holder,
            open_stream,
            after=after,
            terminal=frozenset(("done", "failed", "resync_required")),
        )

    def __enter__(self) -> Self:
        super().__enter__()
        if self._creating:
            first = self._first
            run_id = first.data.get("run_id") if first is not None else None
            if (
                first is None
                or first.kind != "run_started"
                or not isinstance(run_id, str)
                or not run_id
            ):
                self.close()
                raise ProtocolError("Run stream did not start with a run ID")
            self.run_id = run_id
        return self


class PullEventStream(_EventStream):
    """Streams replaceable progress snapshots for one local model download."""

    def __init__(self, holder: Holder, job_id: str, runner_id: str | None) -> None:
        def open_stream(client: AuthenticatedClient) -> EventStream[Any]:
            return get_ai_runner_pull_job_id_events.stream(
                job_id,
                client=client,
                runner_id=runner_id if runner_id is not None else UNSET,
            )

        super().__init__(
            holder, open_stream, terminal=frozenset(("completed", "failed"))
        )
