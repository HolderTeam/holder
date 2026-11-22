from enum import StrEnum


class ConflictVersionEditedOn(StrEnum):
    ANOTHER_DEVICE = "another_device"
    THIS_DEVICE = "this_device"
    UNKNOWN = "unknown"

    def __str__(self) -> str:
        return str(self.value)
