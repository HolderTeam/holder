"""Errors particular to recovery tokens."""

from authoring import HolderError


class RecoveryTokenInvalid(HolderError):
    """The recovery token or its PIN is invalid, or the token belongs to another project."""
