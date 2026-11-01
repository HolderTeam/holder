"""Errors particular to reading and restoring history."""

from authoring import HolderError


class HistoryUnavailable(HolderError):
    """The project's Git history cannot be read right now."""


class HistoryKeyUnavailable(HolderError):
    """This device cannot use the encrypted project's key to read its history."""


class RevisionAmbiguous(HolderError):
    """A commit ID prefix matches more than one revision; use a longer prefix or the full ID."""


class HistoryResponseTooLarge(HolderError):
    """The bounded History response would exceed 2 MiB."""
