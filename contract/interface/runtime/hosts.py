"""Platform services core calls through a host-supplied adapter."""

from typing import Protocol

from authoring import Int32, Int64, host_interface


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
