"""Platform services core calls through a host-supplied adapter."""

from typing import Protocol

from authoring import Int64, host_interface


@host_interface
class TimeZoneProvider(Protocol):
    """The device's time-zone rules, used where core has no native zone database."""

    def utc_offset(self, zone: str, utc_seconds: Int64) -> Int64 | None:
        """The zone's seconds east of UTC at an instant, or null when the zone is unknown."""

    def current_zone(self) -> str:
        """The device's current time-zone identifier, such as Europe/London."""
