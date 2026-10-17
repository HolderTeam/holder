from enum import StrEnum


class CardBacklinkListResponseScope(StrEnum):
    ALL_AVAILABLE = "all_available"
    EXTERNAL = "external"
    LOCAL = "local"

    def __str__(self) -> str:
        return str(self.value)
