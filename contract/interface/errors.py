"""The error kinds operations fail with, besides an invalid argument.

Each names a condition the caller can act on, not how a transport reports it; app.py gives each
its HTTP status.
"""

from holder_contract import HolderError


class NotFound(HolderError):
    """The thing asked for does not exist, or is in the trash."""


class Conflict(HolderError):
    """The request conflicts with what exists: what would be created already exists, such as a card with the same ID or file, or the thing to change is not in the state the request needs, such as restoring a card that is not in the trash, or removing a tag that a card would still carry in its prose."""


class KeyringUnavailable(HolderError):
    """The keyring that holds an encrypted project's key cannot be used: it is locked, or this device has none."""


class KeyMissing(HolderError):
    """An encrypted project's key is not on this device; the project's recovery token restores it."""


class DataCorrupted(HolderError):
    """A project's stored data is damaged or unsafe: an encrypted file does not decrypt, or holds plaintext."""


class InvalidMove(HolderError):
    """A card cannot go where it was asked to: under itself or one of its descendants, beside a card that cannot be its sibling, or up a level from the top."""


class HierarchyInvalid(HolderError):
    """A card's ancestor chain is broken, cyclic or too deep to follow."""


class HistoryUnavailable(HolderError):
    """The project's Git history cannot be read right now."""


class HistoryKeyUnavailable(HolderError):
    """This device cannot use the encrypted project's key to read its history."""


class RevisionAmbiguous(HolderError):
    """A commit ID prefix matches more than one revision; use a longer prefix or the full ID."""


class NotTrashed(HolderError):
    """The item still exists but is not in Trash, so permanent removal is refused."""


class HistoryResponseTooLarge(HolderError):
    """The bounded History response would exceed 2 MiB."""


class RecoveryTokenInvalid(HolderError):
    """The recovery token or its PIN is invalid, or the token belongs to another project."""
