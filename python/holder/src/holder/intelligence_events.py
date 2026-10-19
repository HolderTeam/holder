"""Synchronous, context-managed streams for intelligence runs and model pulls."""

from __future__ import annotations

import json
from collections.abc import Iterator
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, Self
from urllib.parse import quote

import httpx

from .exceptions import (
    APIError,
    AuthenticationError,
    ConnectionError,
    NotFoundError,
    ProtocolError,
)

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


def _frames(response: httpx.Response) -> Iterator[IntelligenceEvent]:
    payload: list[str] = []
    name = "message"
    cursor = None
    first = True
    for line in response.iter_lines():
        if first:
            line = line.removeprefix("\ufeff")
            first = False
        if not line:
            if payload:
                try:
                    data = json.loads("\n".join(payload))
                except (TypeError, ValueError):
                    raise ProtocolError("Invalid intelligence event JSON") from None
                if not isinstance(data, dict):
                    raise ProtocolError("Invalid intelligence event JSON")
                yield IntelligenceEvent(name, data, cursor)
            payload, name, cursor = [], "message", None
            continue
        if line.startswith(":"):
            continue
        key, _, value = line.partition(":")
        value = value.removeprefix(" ")
        if key == "event":
            name = value or "message"
        elif key == "id" and "\x00" not in value:
            cursor = value or None
        elif key == "data":
            payload.append(value)


def _check_response(response: httpx.Response) -> None:
    if response.status_code != 200:
        try:
            error = response.json()["error"]
            code, message = error["code"], error["message"]
        except (ValueError, KeyError, TypeError):
            code, message = "error", f"HTTP {response.status_code}"
        error_type = (
            AuthenticationError
            if response.status_code == 401
            else NotFoundError
            if response.status_code == 404
            else APIError
        )
        raise error_type(response.status_code, code, message)
    if (
        response.headers.get("content-type", "").split(";")[0].strip().lower()
        != "text/event-stream"
    ):
        raise ProtocolError("Expected an intelligence event stream")


class _EventStream(Iterator[IntelligenceEvent]):
    """One-reader stream. Closing it does not cancel the server operation."""

    def __init__(
        self,
        holder: Holder,
        method: str,
        url: str,
        *,
        params: dict[str, str] | None = None,
        body: dict[str, Any] | None = None,
        after: str | None = None,
        terminal: frozenset[str],
    ) -> None:
        if after == "":
            raise ValueError("after cursor must be nonempty")
        self._holder = holder
        self._method = method
        self._url = url
        self._params = params
        self._body = body
        self._after = after
        self._terminal = terminal
        self._connection: Any = None
        self._events: Iterator[IntelligenceEvent] | None = None
        self._first: IntelligenceEvent | None = None
        self._done = False
        self.cursor = after

    def __enter__(self) -> Self:
        if self._connection is not None:
            raise RuntimeError("Intelligence streams cannot be reused")
        client = self._holder._active().get_httpx_client()
        timeout = httpx.Timeout(self._holder._timeout)
        timeout.read = None
        headers = {"Accept": "text/event-stream"}
        if self._after is not None:
            headers["Last-Event-ID"] = self._after
        connection = client.stream(
            self._method,
            self._url,
            params=self._params,
            json=self._body,
            headers=headers,
            timeout=timeout,
        )
        entered = False
        try:
            response = connection.__enter__()
            entered = True
            _check_response(response)
            events = _frames(response)
            first = next(events, None)
            if first is None:
                raise ConnectionError(
                    "Intelligence stream closed before its first event"
                )
        except httpx.TransportError:
            if entered:
                connection.__exit__(None, None, None)
            raise ConnectionError("Could not read intelligence event stream") from None
        except BaseException:
            if entered:
                connection.__exit__(None, None, None)
            raise
        self._connection = connection
        self._events = events
        self._first = first
        return self

    def __exit__(self, *args: object) -> None:
        self.close()

    def close(self) -> None:
        self._done = True
        if self._connection is not None:
            self._connection.__exit__(None, None, None)
            self._connection = None

    def __iter__(self) -> Self:
        return self

    def __next__(self) -> IntelligenceEvent:
        if self._events is None:
            raise RuntimeError("Use intelligence streams inside with")
        if self._done:
            raise StopIteration
        try:
            if self._first is not None:
                event, self._first = self._first, None
            else:
                event = next(self._events)
        except StopIteration:
            raise ConnectionError(
                "Intelligence stream ended before a terminal event; reconnect using its cursor"
            ) from None
        except httpx.TransportError:
            raise ConnectionError(
                "Intelligence stream disconnected; reconnect using its cursor"
            ) from None
        if event.cursor is not None:
            self.cursor = event.cursor
        if event.kind in self._terminal:
            self._done = True
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
            method, url = "POST", "/ai/runs"
        else:
            body = None
            method, url = "GET", f"/ai/runs/{quote(run_id, safe='')}/events"
        self.run_id = run_id
        self._creating = run_id is None
        super().__init__(
            holder,
            method,
            url,
            body=body,
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
        super().__init__(
            holder,
            "GET",
            f"/ai/runner/pull/{quote(job_id, safe='')}/events",
            params={"runner_id": runner_id} if runner_id is not None else None,
            terminal=frozenset(("completed", "failed", "resync_required")),
        )
