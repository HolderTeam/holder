"""The error kinds operations fail with, besides an invalid argument.

Each names a condition the caller can act on, not how a transport reports it; http.py gives each
its HTTP status.
"""

from holder_contract import HolderError


class NotFound(HolderError):
    """The thing asked for does not exist, or is in the trash."""


class Conflict(HolderError):
    """What would be created already exists, such as a card with the same ID or file."""


class KeyringUnavailable(HolderError):
    """The keyring that holds an encrypted project's key cannot be used: it is locked, or this device has none."""


class KeyMissing(HolderError):
    """An encrypted project's key is not on this device; the project's recovery token restores it."""


class DataCorrupted(HolderError):
    """A project's stored data is damaged or unsafe: an encrypted file does not decrypt, or holds plaintext."""
