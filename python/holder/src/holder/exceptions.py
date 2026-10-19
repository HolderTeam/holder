"""Errors raised by the high-level Holder API."""

from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .models.card_reference import CardReferenceCandidate


class HolderError(Exception):
    """Base error for the high-level client."""


class ConnectionError(HolderError):
    """Daemon discovery, startup or transport failed."""


class ClosedError(HolderError):
    """The client is outside its active context."""


class ProtocolError(HolderError):
    """The daemon returned an invalid or incompatible response."""


class APIError(HolderError):
    """A failed daemon request, with its status and error envelope."""

    def __init__(self, status: int, code: str, message: str) -> None:
        self.status = status
        self.code = code
        self.message = message
        super().__init__(f"{status} {code}: {message}")


class AuthenticationError(APIError):
    """The daemon rejected the credentials."""


class NotFoundError(APIError):
    """The requested object was not found within its scope."""


class AmbiguousCardReferenceError(HolderError):
    """A card reference matched more than one card in the project."""

    def __init__(
        self, reference: str, candidates: tuple[CardReferenceCandidate, ...]
    ) -> None:
        self.reference = reference
        self.candidates = candidates
        super().__init__(f"Card reference {reference!r} is ambiguous")
