"""Errors particular to the Trash."""

from authoring import HolderError


class NotTrashed(HolderError):
    """The item still exists but is not in Trash, so permanent removal is refused."""
