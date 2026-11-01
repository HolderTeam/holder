"""Shared public operation error groups."""

from ..errors import DataCorrupted, KeyMissing, KeyringUnavailable

KEYS = [KeyringUnavailable, KeyMissing, DataCorrupted]
