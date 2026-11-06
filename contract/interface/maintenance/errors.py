"""Errors particular to project maintenance."""

from authoring import HolderError


class RebuildFailed(HolderError):
    """A project's files cannot be read back into the database."""
