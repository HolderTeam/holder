from enum import StrEnum


class AiStreamResyncDataReason(StrEnum):
    HISTORY_UNAVAILABLE = "history_unavailable"

    def __str__(self) -> str:
        return str(self.value)
