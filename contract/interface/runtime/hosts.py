"""Platform services core calls through a host-supplied adapter."""

from typing import Protocol

from authoring import Bytes, Int32, Int64, host_interface


@host_interface
class TimeZoneProvider(Protocol):
    """The device's time-zone rules, used where core has no native zone database."""

    def utc_offset(self, zone: str, utc_seconds: Int64) -> Int64 | None:
        """The zone's seconds east of UTC at an instant, or null when the zone is unknown."""

    def current_zone(self) -> str:
        """The device's current time-zone identifier, such as Europe/London."""


@host_interface
class KeyringProvider(Protocol):
    """A process-wide secure store for device secrets and project encryption keys."""

    def lookup(self, kind: Int32, service: str, account: str, project_id: str | None) -> str | None:
        """Return a secret, or null when absent. Failure is distinct from absence."""

    def store(self, kind: Int32, service: str, account: str, project_id: str | None,
              label: str, secret: str) -> None:
        """Store a secret; failure leaves the caller informed."""

    def remove(self, kind: Int32, service: str, account: str, project_id: str | None) -> None:
        """Remove a secret; an absent secret is not a failure."""


@host_interface
class SshSigner(Protocol):
    """Signs SSH authentication challenges with a host-owned private key."""

    def sign(self, data: Bytes) -> Bytes:
        """Return a DER-encoded P-256 ECDSA signature for the challenge bytes."""


@host_interface(error_codes=("unavailable", "authentication", "permission", "capacity",
                             "integrity", "conflict", "invalid_configuration", "transient"))
class StorageProvider(Protocol):
    """Stores opaque asset objects under a host-managed provider name."""

    def put(self, object_key: str, staged_file_path: str, stored_size: Int64,
            stored_sha256: str) -> None:
        """Upload a staged file. A typed failure leaves the source intact."""

    def get(self, object_key: str, destination_file_path: str) -> None:
        """Download an object to the destination file."""

    def exists(self, object_key: str) -> bool:
        """Return false when absent; an inability to check is a failure."""

    def remove(self, object_key: str) -> None:
        """Remove an object; absence is not a failure."""
